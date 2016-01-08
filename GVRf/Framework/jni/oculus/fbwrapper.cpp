#include "fbwrapper.h"
#include "util/gvr_log.h"

#include "VrApi.h"

#include <EGL/egl.h>

#include <malloc.h>

typedef void (GL_APIENTRY*PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)(GLenum target, GLsizei samples,
        GLenum internalformat, GLsizei width, GLsizei height);
typedef void (GL_APIENTRY*PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)(GLenum target, GLenum attachment,
        GLenum textarget, GLuint texture, GLint level, GLsizei samples);

void FbWrapper::clearFb() {
    Width = 0;
    Height = 0;
    Multisamples = 0;
    TextureSwapChainLength = 0;
    TextureSwapChainIndex = 0;
    ColorTextureSwapChain = NULL;
    DepthBuffers = NULL;
    FrameBuffers = NULL;
}

bool FbWrapper::createFb(const ovrTextureFormat colorFormat, const int width, const int height,
        const int multisamples) {
    Width = width;
    Height = height;
    Multisamples = multisamples;

    ColorTextureSwapChain = vrapi_CreateTextureSwapChain(VRAPI_TEXTURE_TYPE_2D, colorFormat, width, height, 1, true);
    TextureSwapChainLength = vrapi_GetTextureSwapChainLength(ColorTextureSwapChain);
    DepthBuffers = (GLuint *) malloc(TextureSwapChainLength * sizeof(GLuint));
    FrameBuffers = (GLuint *) malloc(TextureSwapChainLength * sizeof(GLuint));

    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT =
            (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC) eglGetProcAddress("glRenderbufferStorageMultisampleEXT");
    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT =
            (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC) eglGetProcAddress("glFramebufferTexture2DMultisampleEXT");

    for (int i = 0; i < TextureSwapChainLength; i++) {
        // Create the color buffer texture.
        const GLuint colorTexture = vrapi_GetTextureSwapChainHandle(ColorTextureSwapChain, i);
        GL(glBindTexture(GL_TEXTURE_2D, colorTexture));
        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GL(glBindTexture(GL_TEXTURE_2D, 0));

        if (multisamples > 1&& glRenderbufferStorageMultisampleEXT != NULL
        && glFramebufferTexture2DMultisampleEXT != NULL) {
            // Create multisampled depth buffer.
            GL(glGenRenderbuffers(1, &DepthBuffers[i]));
            GL(glBindRenderbuffer(GL_RENDERBUFFER, DepthBuffers[i]));
            GL(glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, multisamples, GL_DEPTH_COMPONENT24, width, height));
            GL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

            // Create the frame buffer.
            GL(glGenFramebuffers(1, &FrameBuffers[i]));
            GL(glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffers[i]));
            GL(
                    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0, multisamples));
            GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, DepthBuffers[i]));
            GL(GLenum renderFramebufferStatus = glCheckFramebufferStatus( GL_FRAMEBUFFER ));
            GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
                LOGE("Incomplete frame buffer object: %s", GlFrameBufferStatusString(renderFramebufferStatus));
                return false;
            }
        } else {
            // Create depth buffer.
            GL(glGenRenderbuffers(1, &DepthBuffers[i]));
            GL(glBindRenderbuffer(GL_RENDERBUFFER, DepthBuffers[i]));
            GL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height));
            GL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

            // Create the frame buffer.
            GL(glGenFramebuffers(1, &FrameBuffers[i]));
            GL(glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffers[i]));
            GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, DepthBuffers[i]));
            GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0));
            GLenum renderFramebufferStatus = GL(glCheckFramebufferStatus(GL_FRAMEBUFFER));
            GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
                LOGE("Incomplete frame buffer object: %s", GlFrameBufferStatusString(renderFramebufferStatus));
                return false;
            }
        }
    }
    return true;
}

void FbWrapper::destroyFb() {
    GL(glDeleteFramebuffers(TextureSwapChainLength, FrameBuffers));
    GL(glDeleteRenderbuffers(TextureSwapChainLength, DepthBuffers));
    vrapi_DestroyTextureSwapChain(ColorTextureSwapChain);

    free(DepthBuffers);
    free(FrameBuffers);

    clearFb();
}

void FbWrapper::setCurrentFb() {
    GL(glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffers[TextureSwapChainIndex]));
}

void FbWrapper::setNoneFb() {
    GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

// Discard the depth buffer, so the tiler won't need to write it back out to memory.
// Flush this frame worth of commands.
void FbWrapper::resolveFb() {
    const GLenum depthAttachment[1] = { GL_DEPTH_ATTACHMENT };
    GL(glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, depthAttachment));
    GL(glFlush());
}

// Advance to the next texture from the set.
void FbWrapper::advanceFb() {
    TextureSwapChainIndex = (TextureSwapChainIndex + 1) % TextureSwapChainLength;
}

const char * FbWrapper::GlFrameBufferStatusString(GLenum status) {
    switch (status) {
    case GL_FRAMEBUFFER_UNDEFINED:
        return "GL_FRAMEBUFFER_UNDEFINED";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
    case GL_FRAMEBUFFER_UNSUPPORTED:
        return "GL_FRAMEBUFFER_UNSUPPORTED";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
    default:
        return "unknown";
    }
}


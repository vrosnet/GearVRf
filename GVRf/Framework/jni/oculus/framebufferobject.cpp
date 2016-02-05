/* Copyright 2015 Samsung Electronics Co., LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "framebufferobject.h"
#include "util/gvr_log.h"
#include "VrApi.h"
#include <EGL/egl.h>
#include <malloc.h>


typedef void (GL_APIENTRY*PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)(GLenum target, GLsizei samples,
        GLenum internalformat, GLsizei width, GLsizei height);
typedef void (GL_APIENTRY*PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)(GLenum target, GLenum attachment,
        GLenum textarget, GLuint texture, GLint level, GLsizei samples);


void FrameBufferObject::clear() {
    mWidth = 0;
    mHeight = 0;
    mMultisamples = 0;
    mTextureSwapChainLength = 0;
    mTextureSwapChainIndex = 0;
    mColorTextureSwapChain = NULL;
    mDepthBuffers = NULL;
    mFrameBuffers = NULL;
}

bool FrameBufferObject::create(const ovrTextureFormat colorFormat, const int width, const int height,
        const int multisamples) {
    mWidth = width;
    mHeight = height;
    mMultisamples = multisamples;

    mColorTextureSwapChain = vrapi_CreateTextureSwapChain(VRAPI_TEXTURE_TYPE_2D, colorFormat, width, height, 1, true);
    mTextureSwapChainLength = vrapi_GetTextureSwapChainLength(mColorTextureSwapChain);
    mDepthBuffers = (GLuint *) malloc(mTextureSwapChainLength * sizeof(GLuint));
    mFrameBuffers = (GLuint *) malloc(mTextureSwapChainLength * sizeof(GLuint));

    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT =
            (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC) eglGetProcAddress("glRenderbufferStorageMultisampleEXT");
    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT =
            (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC) eglGetProcAddress("glFramebufferTexture2DMultisampleEXT");

    for (int i = 0; i < mTextureSwapChainLength; i++) {
        // Create the color buffer texture.
        const GLuint colorTexture = vrapi_GetTextureSwapChainHandle(mColorTextureSwapChain, i);
        GL(glBindTexture(GL_TEXTURE_2D, colorTexture));
        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GL(glBindTexture(GL_TEXTURE_2D, 0));

        if (multisamples
                > 1&& glRenderbufferStorageMultisampleEXT != NULL && glFramebufferTexture2DMultisampleEXT != NULL) {
            // Create multisampled depth buffer.
            GL(glGenRenderbuffers(1, &mDepthBuffers[i]));
            GL(glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffers[i]));
            GL(glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, multisamples, GL_DEPTH_COMPONENT24, width, height));
            GL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

            // Create the frame buffer.
            GL(glGenFramebuffers(1, &mFrameBuffers[i]));
            GL(glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[i]));
            GL(glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture,
                    0, multisamples));
            GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffers[i]));
            GL(GLenum renderFramebufferStatus = glCheckFramebufferStatus( GL_FRAMEBUFFER ));
            GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
                LOGE("Incomplete frame buffer object: %s", frameBufferStatusString(renderFramebufferStatus));
                return false;
            }
        } else {
            // Create depth buffer.
            GL(glGenRenderbuffers(1, &mDepthBuffers[i]));
            GL(glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffers[i]));
            GL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height));
            GL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

            // Create the frame buffer.
            GL(glGenFramebuffers(1, &mFrameBuffers[i]));
            GL(glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[i]));
            GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffers[i]));
            GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0));
            GLenum renderFramebufferStatus = GL(glCheckFramebufferStatus(GL_FRAMEBUFFER));
            GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
                LOGE("Incomplete frame buffer object: %s", frameBufferStatusString(renderFramebufferStatus));
                return false;
            }
        }
    }
    return true;
}

void FrameBufferObject::destroy() {
    GL(glDeleteFramebuffers(mTextureSwapChainLength, mFrameBuffers));
    GL(glDeleteRenderbuffers(mTextureSwapChainLength, mDepthBuffers));
    vrapi_DestroyTextureSwapChain(mColorTextureSwapChain);

    free(mDepthBuffers);
    free(mFrameBuffers);

    clear();
}

void FrameBufferObject::setCurrent() {
    GL(glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[mTextureSwapChainIndex]));
}

void FrameBufferObject::setNone() {
    GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void FrameBufferObject::resolve() {
    const GLenum depthAttachment[1] = { GL_DEPTH_ATTACHMENT };
    // Discard the depth buffer, so the tiler won't need to write it back out to memory.
    GL(glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, depthAttachment));
    // Flush this frame worth of commands.
    GL(glFlush());
}

void FrameBufferObject::advance() {
    mTextureSwapChainIndex = (mTextureSwapChainIndex + 1) % mTextureSwapChainLength;
}


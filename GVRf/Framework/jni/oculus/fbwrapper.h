/*
 * fbwrapper.h
 *
 *  Created on: Jan 8, 2016
 *      Author: mmarinov
 */

#ifndef OCULUS_FBWRAPPER_H_
#define OCULUS_FBWRAPPER_H_

#include <GLES3/gl3.h>

#include "VrApi_Types.h"

class FbWrapper {
public:

    void clearFb();bool createFb(const ovrTextureFormat colorFormat, const int width, const int height,
            const int multisamples);
    void destroyFb();
    void setCurrentFb();
    static void setNoneFb();
    void resolveFb();
    void advanceFb();

private:
    const char * GlFrameBufferStatusString(GLenum status);

public:
    int Width;
    int Height;
    int Multisamples;
    int TextureSwapChainLength;
    int TextureSwapChainIndex = 0;
    ovrTextureSwapChain * ColorTextureSwapChain;
    GLuint * DepthBuffers;
    GLuint * FrameBuffers;
};

#endif /* OCULUS_FBWRAPPER_H_ */

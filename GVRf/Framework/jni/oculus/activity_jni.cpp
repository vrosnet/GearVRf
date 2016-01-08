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

#include "activity_jni.h"
#include "objects/scene_object.h"

#include <sstream>

#include <jni.h>

#include <glm/gtc/type_ptr.hpp>

#include "SystemActivities.h"
#include "VrApi_Helpers.h"
#include "VrApi_Types.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "GLES3/gl3.h"
#include "GLES3/gl3ext.h"


static const char* activityClassName = "org/gearvrf/GVRActivity";
static const char* activityHandlerRenderingCallbacksClassName = "org/gearvrf/GVRActivity$ActivityHandlerRenderingCallbacks";
static const char* app_settings_name = "org/gearvrf/utility/VrAppSettings";

namespace gvr {

extern "C" {
JNIEXPORT long JNICALL Java_org_gearvrf_VrapiActivityHandler_nativeOnCreate(JNIEnv * jni, jclass clazz,
        jobject activity, jobject callbacks) {
    return reinterpret_cast<long>(new GVRActivity(*jni, activity, callbacks));
}

JNIEXPORT void JNICALL Java_org_gearvrf_VrapiActivityHandler_nativeLeaveVrApi(JNIEnv * jni, jclass clazz, jlong appPtr)
{
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->leaveVrApi();
}

JNIEXPORT void JNICALL Java_org_gearvrf_VrapiActivityHandler_nativeOnDestroy(JNIEnv * jni, jclass clazz, jlong appPtr)
{
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->onDestroy();
}

JNIEXPORT void JNICALL Java_org_gearvrf_VrapiActivityHandler_nativeOnSurfaceCreated(
        JNIEnv * jni, jclass clazz, jlong appPtr)
{
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->setupOculusJava(jni);
    activity->onSurfaceCreated();
}

JNIEXPORT void JNICALL Java_org_gearvrf_VrapiActivityHandler_nativeOnDrawFrame(
        JNIEnv * jni, jclass clazz, jlong appPtr)
{
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->onDrawFrame();
}

JNIEXPORT void JNICALL Java_org_gearvrf_GVRActivity_nativeSetCamera(
        JNIEnv * jni, jclass clazz, jlong appPtr, jlong jcamera)
{
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    Camera* camera = reinterpret_cast<Camera*>(jcamera);
    activity->camera = camera;
}

JNIEXPORT void JNICALL Java_org_gearvrf_GVRActivity_nativeSetCameraRig(
        JNIEnv * jni, jclass clazz, jlong appPtr, jlong cameraRig)
{
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->setCameraRig(cameraRig);
}

JNIEXPORT void JNICALL Java_org_gearvrf_GVRActivity_nativeOnDock(
        JNIEnv * jni, jclass clazz, jlong appPtr)
{
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->headRotationProvider_.onDock();
}

JNIEXPORT void JNICALL Java_org_gearvrf_GVRActivity_nativeOnUndock(
        JNIEnv * jni, jclass clazz, jlong appPtr)
{
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->headRotationProvider_.onUndock();
}
}


void ovrFramebuffer_Clear( ovrFramebuffer * frameBuffer )
{
    frameBuffer->Width = 0;
    frameBuffer->Height = 0;
    frameBuffer->Multisamples = 0;
    frameBuffer->TextureSwapChainLength = 0;
    frameBuffer->TextureSwapChainIndex = 0;
    frameBuffer->ColorTextureSwapChain = NULL;
    frameBuffer->DepthBuffers = NULL;
    frameBuffer->FrameBuffers = NULL;
}

typedef void (GL_APIENTRY* PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);

const char * GlFrameBufferStatusString( GLenum status )
{
    switch ( status )
    {
        case GL_FRAMEBUFFER_UNDEFINED:                      return "GL_FRAMEBUFFER_UNDEFINED";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:          return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:  return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_UNSUPPORTED:                    return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:         return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        default:                                            return "unknown";
    }
}

bool ovrFramebuffer_Create( ovrFramebuffer * frameBuffer, const ovrTextureFormat colorFormat, const int width, const int height, const int multisamples )
{
    frameBuffer->Width = width;
    frameBuffer->Height = height;
    frameBuffer->Multisamples = multisamples;

    frameBuffer->ColorTextureSwapChain = vrapi_CreateTextureSwapChain( VRAPI_TEXTURE_TYPE_2D, colorFormat, width, height, 1, true );
    frameBuffer->TextureSwapChainLength = vrapi_GetTextureSwapChainLength( frameBuffer->ColorTextureSwapChain );
    frameBuffer->DepthBuffers = (GLuint *)malloc( frameBuffer->TextureSwapChainLength * sizeof( GLuint ) );
    frameBuffer->FrameBuffers = (GLuint *)malloc( frameBuffer->TextureSwapChainLength * sizeof( GLuint ) );

    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT =
        (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)eglGetProcAddress( "glRenderbufferStorageMultisampleEXT" );
    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT =
        (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)eglGetProcAddress( "glFramebufferTexture2DMultisampleEXT" );

    for ( int i = 0; i < frameBuffer->TextureSwapChainLength; i++ )
    {
        // Create the color buffer texture.
        const GLuint colorTexture = vrapi_GetTextureSwapChainHandle( frameBuffer->ColorTextureSwapChain, i );
        GL( glBindTexture( GL_TEXTURE_2D, colorTexture ) );
        GL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) );
        GL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) );
        GL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
        GL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );
        GL( glBindTexture( GL_TEXTURE_2D, 0 ) );

        if ( multisamples > 1 && glRenderbufferStorageMultisampleEXT != NULL && glFramebufferTexture2DMultisampleEXT != NULL )
        {
            // Create multisampled depth buffer.
            GL( glGenRenderbuffers( 1, &frameBuffer->DepthBuffers[i] ) );
            GL( glBindRenderbuffer( GL_RENDERBUFFER, frameBuffer->DepthBuffers[i] ) );
            GL( glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER, multisamples, GL_DEPTH_COMPONENT24, width, height ) );
            GL( glBindRenderbuffer( GL_RENDERBUFFER, 0 ) );

            // Create the frame buffer.
            GL( glGenFramebuffers( 1, &frameBuffer->FrameBuffers[i] ) );
            GL( glBindFramebuffer( GL_FRAMEBUFFER, frameBuffer->FrameBuffers[i] ) );
            GL( glFramebufferTexture2DMultisampleEXT( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0, multisamples ) );
            GL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frameBuffer->DepthBuffers[i] ) );
            GL( GLenum renderFramebufferStatus = glCheckFramebufferStatus( GL_FRAMEBUFFER ) );
            GL( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
            if ( renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE )
            {
                LOGE( "Incomplete frame buffer object: %s", GlFrameBufferStatusString( renderFramebufferStatus ) );
                return false;
            }
        }
        else
        {
            // Create depth buffer.
            GL( glGenRenderbuffers( 1, &frameBuffer->DepthBuffers[i] ) );
            GL( glBindRenderbuffer( GL_RENDERBUFFER, frameBuffer->DepthBuffers[i] ) );
            GL( glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height ) );
            GL( glBindRenderbuffer( GL_RENDERBUFFER, 0 ) );

            // Create the frame buffer.
            GL( glGenFramebuffers( 1, &frameBuffer->FrameBuffers[i] ) );
            GL( glBindFramebuffer( GL_FRAMEBUFFER, frameBuffer->FrameBuffers[i] ) );
            GL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frameBuffer->DepthBuffers[i] ) );
            GL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0 ) );
            GL( GLenum renderFramebufferStatus = glCheckFramebufferStatus( GL_FRAMEBUFFER ) );
            GL( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
            if ( renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE )
            {
                LOGE( "Incomplete frame buffer object: %s", GlFrameBufferStatusString( renderFramebufferStatus ) );
                return false;
            }
        }
    }

    return true;
}

void ovrFramebuffer_Destroy( ovrFramebuffer * frameBuffer )
{
    GL( glDeleteFramebuffers( frameBuffer->TextureSwapChainLength, frameBuffer->FrameBuffers ) );
    GL( glDeleteRenderbuffers( frameBuffer->TextureSwapChainLength, frameBuffer->DepthBuffers ) );
    vrapi_DestroyTextureSwapChain( frameBuffer->ColorTextureSwapChain );

    free( frameBuffer->DepthBuffers );
    free( frameBuffer->FrameBuffers );

    ovrFramebuffer_Clear( frameBuffer );
}

void ovrFramebuffer_SetCurrent( ovrFramebuffer * frameBuffer )
{
    GL( glBindFramebuffer( GL_FRAMEBUFFER, frameBuffer->FrameBuffers[frameBuffer->TextureSwapChainIndex] ) );
}

void ovrFramebuffer_SetNone()
{
    GL( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
}

void ovrFramebuffer_Resolve( ovrFramebuffer * frameBuffer )
{
    // Discard the depth buffer, so the tiler won't need to write it back out to memory.
    const GLenum depthAttachment[1] = { GL_DEPTH_ATTACHMENT };
    GL( glInvalidateFramebuffer( GL_FRAMEBUFFER, 1, depthAttachment ) );

    // Flush this frame worth of commands.
    GL(glFlush());
}

void ovrFramebuffer_Advance( ovrFramebuffer * frameBuffer )
{
    // Advance to the next texture from the set.
    frameBuffer->TextureSwapChainIndex = ( frameBuffer->TextureSwapChainIndex + 1 ) % frameBuffer->TextureSwapChainLength;
}


//=============================================================================
//                             GVRActivity
//=============================================================================

template <class R> GVRActivityT<R>::GVRActivityT(JNIEnv& jni, jobject activity, jobject callbacks)
    : mainThreadJni_(&jni) {

    activity_ = jni.NewGlobalRef(activity);
    activityRenderingCallbacks_ = jni.NewGlobalRef(callbacks);

    activityClass_ = GetGlobalClassReference( activityClassName );
    activityRenderingCallbacksClass_ = GetGlobalClassReference(activityHandlerRenderingCallbacksClassName);
    vrAppSettingsClass = GetGlobalClassReference(app_settings_name);

//    oneTimeShutdownMethodId = GetMethodID("oneTimeShutDown", "()V");
//
    onDrawEyeMethodId = GetMethodId(activityRenderingCallbacksClass_, "onDrawEye", "(I)V");

//    onKeyEventNativeMethodId = GetMethodID("onKeyEventNative", "(II)Z");
    updateSensoredSceneMethodId = GetMethodId(activityClass_, "updateSensoredScene", "()Z");
    //getAppSettingsMethodId = GetMethodID("getAppSettings", "()Lorg/gearvrf/utility/VrAppSettings;");

    setupOculusJava(mainThreadJni_);

    SystemActivities_Init(&oculusJava_);
    const ovrInitParms initParms = vrapi_DefaultInitParms(&oculusJava_);
    int32_t initResult = vrapi_Initialize(&initParms);
    if (VRAPI_INITIALIZE_SUCCESS != initResult) {
        char const * msg = initResult == VRAPI_INITIALIZE_PERMISSIONS_ERROR ?
                                        "Thread priority security exception. Make sure the APK is signed." :
                                        "VrApi initialization error.";
        SystemActivities_DisplayError( &oculusJava_, SYSTEM_ACTIVITIES_FATAL_ERROR_OSIG, __FILE__, msg );
    }
}

template <class R> jmethodID GVRActivityT<R>::GetStaticMethodID( jclass clazz, const char * name,
        const char * signature) {
    jmethodID mid = mainThreadJni_->GetStaticMethodID(clazz, name, signature);
    if (!mid) {
        //FAIL("couldn't get %s", name);
    }
    return mid;
}

template <class R> jmethodID GVRActivityT<R>::GetMethodId(const jclass clazz, const char* name, const char* signature) {
    const jmethodID mid = mainThreadJni_->GetMethodID(clazz, name, signature);
    if (nullptr == mid) {
        std::terminate();
    }
    return mid;
}

template <class PredictionTrait> jclass GVRActivityT<PredictionTrait>::GetGlobalClassReference(const char * className) const {
    jclass lc = mainThreadJni_->FindClass(className);
    if (lc == 0) {
        //FAIL( "FindClass( %s ) failed", className);
    }
    // Turn it into a global ref, so we can safely use it in the VR thread
    jclass gc = (jclass) mainThreadJni_->NewGlobalRef(lc);
    mainThreadJni_->DeleteLocalRef(lc);

    return gc;
}

//template <class R> void GVRActivityT<R>::Configure(OVR::ovrSettings & settings)
//{
//    //General settings.
////    JNIEnv *env = app->GetJava()->Env;
////    jobject vrSettings = env->CallObjectMethod(app->GetJava()->ActivityObject,
////            getAppSettingsMethodId);
////    jint framebufferPixelsWide = env->GetIntField(vrSettings,
////            env->GetFieldID(vrAppSettingsClass, "framebufferPixelsWide", "I"));
////    if (framebufferPixelsWide == -1) {
////        app->GetJava()->Env->SetIntField(vrSettings,
////                env->GetFieldID(vrAppSettingsClass, "framebufferPixelsWide",
////                        "I"), settings.FramebufferPixelsWide);
////    } else {
////        settings.FramebufferPixelsWide = framebufferPixelsWide;
////    }
////    jint framebufferPixelsHigh = env->GetIntField(vrSettings,
////            env->GetFieldID(vrAppSettingsClass, "framebufferPixelsHigh", "I"));
////    if (framebufferPixelsHigh == -1) {
////        env->SetIntField(vrSettings,
////                env->GetFieldID(vrAppSettingsClass, "framebufferPixelsHigh",
////                        "I"), settings.FramebufferPixelsHigh);
////    } else {
////        settings.FramebufferPixelsHigh = framebufferPixelsHigh;
////    }
////    settings.ShowLoadingIcon = env->GetBooleanField(vrSettings,
////            env->GetFieldID(vrAppSettingsClass, "showLoadingIcon", "Z"));
////    settings.UseSrgbFramebuffer = env->GetBooleanField(vrSettings,
////            env->GetFieldID(vrAppSettingsClass, "useSrgbFramebuffer", "Z"));
////    settings.UseProtectedFramebuffer = env->GetBooleanField(vrSettings,
////            env->GetFieldID(vrAppSettingsClass, "useProtectedFramebuffer",
////                    "Z"));
////
////    //Settings for EyeBufferParms.
////    jobject eyeParmsSettings = env->GetObjectField(vrSettings,
////            env->GetFieldID(vrAppSettingsClass, "eyeBufferParms",
////                    "Lorg/gearvrf/utility/VrAppSettings$EyeBufferParms;"));
////    jclass eyeParmsClass = env->GetObjectClass(eyeParmsSettings);
////    settings.EyeBufferParms.multisamples = env->GetIntField(eyeParmsSettings, env->GetFieldID(eyeParmsClass, "multiSamples", "I"));
////    settings.EyeBufferParms.resolveDepth = env->GetBooleanField(eyeParmsSettings, env->GetFieldID(eyeParmsClass, "resolveDepth", "Z"));
////
////    jint resolutionWidth = env->GetIntField(eyeParmsSettings, env->GetFieldID(eyeParmsClass, "resolutionWidth", "I"));
////    if(resolutionWidth == -1){
////        env->SetIntField(eyeParmsSettings, env->GetFieldID(eyeParmsClass, "resolutionWidth", "I"), settings.EyeBufferParms.resolutionWidth);
////    }else{
////        settings.EyeBufferParms.resolutionWidth = resolutionWidth;
////    }
////
////    jint resolutionHeight = env->GetIntField(eyeParmsSettings, env->GetFieldID(eyeParmsClass, "resolutionHeight", "I"));
////    if(resolutionHeight == -1){
////        env->SetIntField(eyeParmsSettings, env->GetFieldID(eyeParmsClass, "resolutionHeight", "I"), settings.EyeBufferParms.resolutionHeight);
////    }else{
////        settings.EyeBufferParms.resolutionHeight = resolutionHeight;
////    }
////
////    jobject depthFormat = env->GetObjectField(eyeParmsSettings, env->GetFieldID(eyeParmsClass, "depthFormat", "Lorg/gearvrf/utility/VrAppSettings$EyeBufferParms$DepthFormat;"));
////    jmethodID getValueID;
////    getValueID = env->GetMethodID(env->GetObjectClass(depthFormat),"getValue","()I");
////    int depthFormatValue = (int)env->CallIntMethod(depthFormat, getValueID);
////    switch(depthFormatValue){
////    case 0:
////        settings.EyeBufferParms.depthFormat = OVR::DEPTH_0;
////        break;
////    case 1:
////        settings.EyeBufferParms.depthFormat = OVR::DEPTH_16;
////        break;
////    case 2:
////        settings.EyeBufferParms.depthFormat = OVR::DEPTH_24;
////        break;
////    case 3:
////        settings.EyeBufferParms.depthFormat = OVR::DEPTH_24_STENCIL_8;
////        break;
////    default:
////        break;
////    }
////    jobject colorFormat = env->GetObjectField(eyeParmsSettings, env->GetFieldID(eyeParmsClass, "colorFormat", "Lorg/gearvrf/utility/VrAppSettings$EyeBufferParms$ColorFormat;"));
////    getValueID = env->GetMethodID(env->GetObjectClass(colorFormat),"getValue","()I");
////    int colorFormatValue = (int)env->CallIntMethod(colorFormat, getValueID);
////    switch(colorFormatValue){
////    case 0:
////        settings.EyeBufferParms.colorFormat = OVR::COLOR_565;
////        break;
////    case 1:
////        settings.EyeBufferParms.colorFormat = OVR::COLOR_5551;
////        break;
////    case 2:
////        settings.EyeBufferParms.colorFormat = OVR::COLOR_4444;
////        break;
////    case 3:
////        settings.EyeBufferParms.colorFormat = OVR::COLOR_8888;
////        break;
////    case 4:
////        settings.EyeBufferParms.colorFormat = OVR::COLOR_8888_sRGB;
////        break;
////    default:
////        break;
////    }
////
////
////    //Settings for ModeParms
////    jobject modeParms = env->GetObjectField(vrSettings, env->GetFieldID(vrAppSettingsClass, "modeParms", "Lorg/gearvrf/utility/VrAppSettings$ModeParms;"));
////    jclass modeParmsClass = env->GetObjectClass(modeParms);
////    settings.ModeParms.AllowPowerSave = env->GetBooleanField(modeParms, env->GetFieldID(modeParmsClass, "allowPowerSave", "Z"));
////    settings.ModeParms.ResetWindowFullscreen = env->GetBooleanField(modeParms, env->GetFieldID(modeParmsClass, "resetWindowFullScreen","Z"));
////    jobject performanceParms = env->GetObjectField(vrSettings, env->GetFieldID(vrAppSettingsClass, "performanceParms", "Lorg/gearvrf/utility/VrAppSettings$PerformanceParms;"));
////    jclass performanceParmsClass = env->GetObjectClass(performanceParms);
////    settings.PerformanceParms.GpuLevel = env->GetIntField(performanceParms, env->GetFieldID(performanceParmsClass, "gpuLevel", "I"));
////    settings.PerformanceParms.CpuLevel = env->GetIntField(performanceParms, env->GetFieldID(performanceParmsClass, "cpuLevel", "I"));
////
////    // Settings for HeadModelParms
////    jobject headModelParms = env->GetObjectField(vrSettings, env->GetFieldID(vrAppSettingsClass, "headModelParms", "Lorg/gearvrf/utility/VrAppSettings$HeadModelParms;" ));
////    jclass headModelParmsClass = env->GetObjectClass(headModelParms);
////    float interpupillaryDistance = (float)env->GetFloatField(headModelParms, env->GetFieldID(headModelParmsClass, "interpupillaryDistance", "F"));
////    if(interpupillaryDistance != interpupillaryDistance){
////        //Value not set in Java side, current Value is NaN
////        //Need to copy the system settings to java side.
////        env->SetFloatField(headModelParms, env->GetFieldID(headModelParmsClass, "interpupillaryDistance", "F"), settings.HeadModelParms.InterpupillaryDistance);
////    }else{
////        settings.HeadModelParms.InterpupillaryDistance = interpupillaryDistance;
////    }
////    float eyeHeight = (float)env->GetFloatField(headModelParms, env->GetFieldID(headModelParmsClass, "eyeHeight", "F"));
////    if(eyeHeight != eyeHeight){
////        //same as interpupilaryDistance
////        env->SetFloatField(headModelParms, env->GetFieldID(headModelParmsClass, "eyeHeight", "F"), settings.HeadModelParms.EyeHeight);
////    }else{
////        settings.HeadModelParms.EyeHeight = eyeHeight;
////    }
////    float headModelDepth = (float)env->GetFloatField(headModelParms, env->GetFieldID(headModelParmsClass, "headModelDepth", "F"));
////    if(headModelDepth != headModelDepth){
////            //same as interpupilaryDistance
////        env->SetFloatField(headModelParms, env->GetFieldID(headModelParmsClass, "headModelDepth", "F"), settings.HeadModelParms.HeadModelDepth);
////    }else{
////        settings.HeadModelParms.HeadModelDepth = headModelDepth;
////    }
////    float headModelHeight = (float)env->GetFloatField(headModelParms, env->GetFieldID(headModelParmsClass, "headModelHeight", "F"));
////    if(headModelHeight != headModelHeight){
////            //same as interpupilaryDistance
////        env->SetFloatField(headModelParms, env->GetFieldID(headModelParmsClass, "headModelHeight", "F"), settings.HeadModelParms.HeadModelHeight);
////    }else{
////        settings.HeadModelParms.HeadModelHeight = headModelHeight;
////    }
////    if (env->GetStaticBooleanField(vrAppSettingsClass,
////            env->GetStaticFieldID(vrAppSettingsClass, "isShowDebugLog", "Z"))) {
////        std::stringstream logInfo;
////        logInfo << "====== General Configuration ======" << std::endl;
////        if (settings.FramebufferPixelsHigh == 0
////                && settings.FramebufferPixelsWide == 0) {
////            logInfo
////                    << "FramebufferPixelsHigh = screen size; FramebufferPixelsWide = screen size \n";
////        } else {
////            logInfo << "FramebufferPixelsHigh = "
////                    << settings.FramebufferPixelsHigh
////                    << "; FrameBufferPixelsWide = "
////                    << settings.FramebufferPixelsWide << std::endl;
////        }
////        logInfo << "ShowLoadingIcon = " << settings.ShowLoadingIcon
////                << "; UseProtectedFramebuffer = "
////                << settings.UseProtectedFramebuffer << "; UseSrgbFramebuffer = "
////                << settings.UseSrgbFramebuffer << "\n";
////        logInfo << "====== Eye Buffer Configuration ======\n";
////        logInfo << "colorFormat = ";
////        switch (settings.EyeBufferParms.colorFormat) {
////        case 0:
////            logInfo << "COLOR_565";
////            break;
////        case 1:
////            logInfo << "COLOR_5551";
////            break;
////        case 2:
////            logInfo << "COLOR_4444";
////            break;
////        case 3:
////            logInfo << "COLOR_8888";
////            break;
////        case 4:
////            logInfo << "COLOR_8888_sRGB";
////            break;
////        default:
////            break;
////        }
////        logInfo << "; depthFormat = ";
////        switch (settings.EyeBufferParms.depthFormat) {
////        case 0:
////            logInfo << "DEPTH_0";
////            break;
////        case 1:
////            logInfo << "DEPTH_16";
////            break;
////        case 2:
////            logInfo << "DEPTH_24";
////            break;
////        case 3:
////            logInfo << "DEPTH_24_STENCIL_8";
////            break;
////        default:
////            break;
////        }
////        logInfo << "; ResolveDepth = " << settings.EyeBufferParms.resolveDepth
////                << "; multiSample = " << settings.EyeBufferParms.multisamples
////                << "; resolutionWidth = " << settings.EyeBufferParms.resolutionWidth
////                << "; resolutionHeight = " << settings.EyeBufferParms.resolutionHeight
////                << std::endl;
////        logInfo << "====== Head Model Configuration ======" << std::endl;
////        logInfo << "EyeHeight = " << settings.HeadModelParms.EyeHeight
////                << "; HeadModelDepth = "
////                << settings.HeadModelParms.HeadModelDepth
////                << "; HeadModelHeight = "
////                << settings.HeadModelParms.HeadModelHeight
////                << "; InterpupillaryDistance = "
////                << settings.HeadModelParms.InterpupillaryDistance << std::endl;
////        logInfo << "====== Mode Configuration ======" << std::endl;
////        logInfo << "AllowPowerSave = " << settings.ModeParms.AllowPowerSave
////                << "; ResetWindowFullscreen = "
////                << settings.ModeParms.ResetWindowFullscreen << std::endl;
////        logInfo << "====== Performance Configuration ======"
////                << "; CpuLevel = " << settings.PerformanceParms.CpuLevel
////                << "; GpuLevel = " << settings.PerformanceParms.GpuLevel << std::endl;
////
////        LOGI("%s", logInfo.str().c_str());
////    }
//}

template <class R> void GVRActivityT<R>::OneTimeShutdown()
{
//    app->GetJava()->Env->CallVoidMethod(app->GetJava()->ActivityObject, oneTimeShutdownMethodId);

    // Free GL resources
}

//template <class R> OVR::Matrix4f GVRActivityT<R>::Frame( const OVR::VrFrame & vrFrame )
//{
////    JNIEnv* jni = app->GetJava()->Env;
//
//	//This is called once while DrawEyeView is called twice, when eye=0 and eye 1.
//	//So camera is set in java as one of left and right camera.
//	//Centerview camera matrix can be retrieved from its parent, CameraRig
//    glm::mat4 vp_matrix = camera->getCenterViewMatrix();
//
//    ovrMatrix4f view2;
//
//    view2.M[0][0] = vp_matrix[0][0];
//    view2.M[1][0] = vp_matrix[0][1];
//    view2.M[2][0] = vp_matrix[0][2];
//    view2.M[3][0] = vp_matrix[0][3];
//    view2.M[0][1] = vp_matrix[1][0];
//    view2.M[1][1] = vp_matrix[1][1];
//    view2.M[2][1] = vp_matrix[1][2];
//    view2.M[3][1] = vp_matrix[1][3];
//    view2.M[0][2] = vp_matrix[2][0];
//    view2.M[1][2] = vp_matrix[2][1];
//    view2.M[2][2] = vp_matrix[2][2];
//    view2.M[3][2] = vp_matrix[2][3];
//    view2.M[0][3] = vp_matrix[3][0];
//    view2.M[1][3] = vp_matrix[3][1];
//    view2.M[2][3] = vp_matrix[3][2];
//    view2.M[3][3] = vp_matrix[3][3];
//
//    return view2;
//}

//template <class R> bool GVRActivityT<R>::OnKeyEvent(const int keyCode, const int repeatCode,
//        const OVR::KeyEventType eventType) {
//
////    bool handled = app->GetJava()->Env->CallBooleanMethod(app->GetJava()->ActivityObject,
////            onKeyEventNativeMethodId, keyCode, (int)eventType);
////
////    // if not handled back key long press, show global menu
////    if (handled == false && keyCode == OVR::OVR_KEY_BACK && eventType == OVR::KEY_EVENT_LONG_PRESS) {
////        app->StartSystemActivity(PUI_GLOBAL_MENU);
////    }
////
////    return handled;
//    return false;
//}

template <class R> bool GVRActivityT<R>::updateSensoredScene() {
    return oculusJava_.Env->CallBooleanMethod(oculusJava_.ActivityObject, updateSensoredSceneMethodId);
}

template <class R> void GVRActivityT<R>::setCameraRig(jlong cameraRig) {
    cameraRig_ = reinterpret_cast<CameraRig*>(cameraRig);
    sensoredSceneUpdated_ = false;
}

#define NUM_MULTI_SAMPLES   4
template <class R> void GVRActivityT<R>::onSurfaceCreated() {
    ovrModeParms parms = vrapi_DefaultModeParms(&oculusJava_);
    parms.ResetWindowFullscreen = true;
    oculusMobile_ = vrapi_EnterVrMode( &parms );

    for ( int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++ )
    {
        bool b = ovrFramebuffer_Create( &FrameBuffer[eye],
                                VRAPI_TEXTURE_FORMAT_8888,
                                vrapi_GetSystemPropertyInt(&oculusJava_, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH ),
                                vrapi_GetSystemPropertyInt(&oculusJava_, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT ),
                                NUM_MULTI_SAMPLES );
    }

    ProjectionMatrix = ovrMatrix4f_CreateProjectionFov(
                                        vrapi_GetSystemPropertyFloat(&oculusJava_, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_X ),
                                        vrapi_GetSystemPropertyFloat(&oculusJava_, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_Y ),
                                        0.0f, 0.0f, 1.0f, 0.0f );
    TexCoordsTanAnglesMatrix = ovrMatrix4f_TanAngleMatrixFromProjection( &ProjectionMatrix );
}

template <class R> void GVRActivityT<R>::onDrawFrame() {
    ovrFrameParms parms = vrapi_DefaultFrameParms(&oculusJava_, VRAPI_FRAME_INIT_DEFAULT, vrapi_GetTimeInSeconds(), NULL );
    parms.FrameIndex = ++frameIndex;
    parms.MinimumVsyncs = 1;
    parms.PerformanceParms = vrapi_DefaultPerformanceParms();
    parms.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

    const double predictedDisplayTime = vrapi_GetPredictedDisplayTime( oculusMobile_, frameIndex );
    const ovrTracking baseTracking = vrapi_GetPredictedTracking( oculusMobile_, predictedDisplayTime );

    const ovrHeadModelParms headModelParms = vrapi_DefaultHeadModelParms();
    const ovrTracking tracking = vrapi_ApplyHeadModel( &headModelParms, &baseTracking );

    // Render the eye images.
    for ( int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++ )
    {
        ovrTracking updatedTracking = vrapi_GetPredictedTracking(oculusMobile_, tracking.HeadPose.TimeInSeconds );
        updatedTracking.HeadPose.Pose.Position = tracking.HeadPose.Pose.Position;
        //ovrTracking updatedTracking = *tracking;

        ovrFramebuffer * frameBuffer = &FrameBuffer[eye];
        ovrFramebuffer_SetCurrent( frameBuffer );
        GL(glViewport(0, 0, frameBuffer->Width, frameBuffer->Height));
        GL(glScissor(0, 0, frameBuffer->Width, frameBuffer->Height));

        //todo
        if (!sensoredSceneUpdated_ && headRotationProvider_.receivingUpdates()) {
            sensoredSceneUpdated_ = updateSensoredScene();
        }
        headRotationProvider_.predict(*this, parms, (1 == eye ? 4.0f : 3.5f) / 60.0f);
        oculusJava_.Env->CallVoidMethod(activityRenderingCallbacks_, onDrawEyeMethodId, eye);

        ovrFramebuffer_Resolve( frameBuffer );

        parms.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Textures[eye].ColorTextureSwapChain = frameBuffer->ColorTextureSwapChain;
        parms.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Textures[eye].TextureSwapChainIndex = frameBuffer->TextureSwapChainIndex;
        for ( int layer = 0; layer < VRAPI_FRAME_LAYER_TYPE_MAX; layer++ )
        {
            parms.Layers[layer].Textures[eye].TexCoordsFromTanAngles = TexCoordsTanAnglesMatrix;
            parms.Layers[layer].Textures[eye].HeadPose = updatedTracking.HeadPose;
        }

        ovrFramebuffer_Advance( frameBuffer );
    }

    ovrFramebuffer_SetNone();

    vrapi_SubmitFrame(oculusMobile_, &parms);
}

template <class R> void GVRActivityT<R>::setupOculusJava(JNIEnv* env) {
    oculusJava_.Env = env;
    env->GetJavaVM(&oculusJava_.Vm);
    oculusJava_.ActivityObject = activity_;
}

template <class R> void GVRActivityT<R>::leaveVrApi() {
    vrapi_LeaveVrMode(oculusMobile_);
}

template <class R> void GVRActivityT<R>::onDestroy() {
    mainThreadJni_->DeleteGlobalRef(activity_);
    mainThreadJni_->DeleteGlobalRef(activityRenderingCallbacks_);

    mainThreadJni_->DeleteGlobalRef(activityClass_);
    mainThreadJni_->DeleteGlobalRef(activityRenderingCallbacksClass_);
    mainThreadJni_->DeleteGlobalRef(vrAppSettingsClass);
}

}

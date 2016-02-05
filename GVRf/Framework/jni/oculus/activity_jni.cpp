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
static const char* activityHandlerRenderingCallbacksClassName = "org/gearvrf/ActivityHandlerRenderingCallbacks";
static const char* app_settings_name = "org/gearvrf/utility/VrAppSettings";

namespace gvr {

extern "C" {
JNIEXPORT long JNICALL Java_org_gearvrf_VrapiActivityHandler_nativeOnCreate(JNIEnv * jni, jclass clazz,
        jobject activity, jobject vrAppSettings, jobject callbacks) {
    GVRActivity* gvrActivity = new GVRActivity(*jni, activity, vrAppSettings, callbacks);
    if (gvrActivity->initializeVrApi()) {
        return reinterpret_cast<long>(gvrActivity);
    } else {
        delete gvrActivity;
        return 0;
    }
}

JNIEXPORT void JNICALL Java_org_gearvrf_VrapiActivityHandler_nativeLeaveVrMode(JNIEnv * jni, jclass clazz,
        jlong appPtr) {
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->leaveVrMode();
}

JNIEXPORT void JNICALL Java_org_gearvrf_GVRActivity_nativeOnDestroy(JNIEnv * jni, jclass clazz, jlong appPtr) {
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->onDestroy();
}

JNIEXPORT void JNICALL Java_org_gearvrf_VrapiActivityHandler_nativeOnSurfaceCreated(JNIEnv * jni, jclass clazz,
        jlong appPtr) {
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->initializeOculusJava(*jni, activity->oculusJavaGlThread_);
    activity->onSurfaceCreated();
}

JNIEXPORT void JNICALL Java_org_gearvrf_VrapiActivityHandler_nativeOnSurfaceChanged(JNIEnv * jni, jclass clazz,
        jlong appPtr) {
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->initializeOculusJava(*jni, activity->oculusJavaGlThread_);
    activity->onSurfaceChanged();
}

JNIEXPORT void JNICALL Java_org_gearvrf_VrapiActivityHandler_nativeOnDrawFrame(JNIEnv * jni, jclass clazz,
        jlong appPtr) {
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->onDrawFrame();
}

JNIEXPORT void JNICALL Java_org_gearvrf_VrapiActivityHandler_nativeOnDestroy(JNIEnv * jni, jclass clazz,
        jlong appPtr) {
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    delete activity;
}

JNIEXPORT void JNICALL Java_org_gearvrf_VrapiActivityHandler_nativeShowGlobalMenu(JNIEnv * jni, jclass clazz, jlong appPtr) {
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->showGlobalMenu();
}

JNIEXPORT void JNICALL Java_org_gearvrf_VrapiActivityHandler_nativeShowConfirmQuit(JNIEnv * jni, jclass clazz, jlong appPtr) {
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->showConfirmQuit();
}

JNIEXPORT void JNICALL Java_org_gearvrf_GVRActivity_nativeSetCamera(JNIEnv * jni, jclass clazz, jlong appPtr,
        jlong jcamera) {
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    Camera* camera = reinterpret_cast<Camera*>(jcamera);
    activity->camera = camera;
}

JNIEXPORT void JNICALL Java_org_gearvrf_GVRActivity_nativeSetCameraRig(JNIEnv * jni, jclass clazz, jlong appPtr,
        jlong cameraRig) {
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->setCameraRig(cameraRig);
}

JNIEXPORT void JNICALL Java_org_gearvrf_GVRActivity_nativeOnDock(JNIEnv * jni, jclass clazz, jlong appPtr) {
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->headRotationProvider_.onDock();
}

JNIEXPORT void JNICALL Java_org_gearvrf_GVRActivity_nativeOnUndock(JNIEnv * jni, jclass clazz, jlong appPtr) {
    GVRActivity *activity = reinterpret_cast<GVRActivity*>(appPtr);
    activity->headRotationProvider_.onUndock();
}

}

//=============================================================================
//                             GVRActivity
//=============================================================================

template<class R> GVRActivityT<R>::GVRActivityT(JNIEnv& jni, jobject activity, jobject vrAppSettings,
        jobject callbacks) : jniMainThread_(&jni)
{
    activity_ = jni.NewGlobalRef(activity);
    vrAppSettings_ = jni.NewGlobalRef(vrAppSettings);
    activityRenderingCallbacks_ = jni.NewGlobalRef(callbacks);

    activityClass_ = GetGlobalClassReference(activityClassName);
    activityRenderingCallbacksClass_ = GetGlobalClassReference(activityHandlerRenderingCallbacksClassName);
    vrAppSettingsClass_ = GetGlobalClassReference(app_settings_name);

    onDrawEyeMethodId = GetMethodId(activityRenderingCallbacksClass_, "onDrawEye", "(I)V");
    updateSensoredSceneMethodId = GetMethodId(activityClass_, "updateSensoredScene", "()Z");
}

template<class R> GVRActivityT<R>::~GVRActivityT() {
   jniMainThread_->DeleteGlobalRef(vrAppSettingsClass_);
   jniMainThread_->DeleteGlobalRef(activityRenderingCallbacksClass_);
   jniMainThread_->DeleteGlobalRef(activityClass_);

   jniMainThread_->DeleteGlobalRef(activityRenderingCallbacks_);
   jniMainThread_->DeleteGlobalRef(vrAppSettings_);
   jniMainThread_->DeleteGlobalRef(activity_);
}

template<class R> bool GVRActivityT<R>::initializeVrApi() {
    initializeOculusJava(*jniMainThread_, oculusJavaMainThread_);
    SystemActivities_Init(&oculusJavaMainThread_);

    const ovrInitParms initParms = vrapi_DefaultInitParms(&oculusJavaMainThread_);
    int32_t initResult = vrapi_Initialize(&initParms);
    if (VRAPI_INITIALIZE_SUCCESS != initResult) {
        char const * msg =
                initResult == VRAPI_INITIALIZE_PERMISSIONS_ERROR ?
                        "Thread priority security exception. Make sure the APK is signed." :
                        "VrApi initialization error.";
        SystemActivities_DisplayError(&oculusJavaMainThread_, SYSTEM_ACTIVITIES_FATAL_ERROR_OSIG, __FILE__, msg);
        return false;
    }
    return true;
}

template<class R> jmethodID GVRActivityT<R>::GetStaticMethodID(jclass clazz, const char * name,
        const char * signature) {
    jmethodID mid = jniMainThread_->GetStaticMethodID(clazz, name, signature);
    if (!mid) {
        LOGE("Unable to find static method %s", name);
        std::terminate();
    }
    return mid;
}

template<class R> jmethodID GVRActivityT<R>::GetMethodId(const jclass clazz, const char* name, const char* signature) {
    const jmethodID mid = jniMainThread_->GetMethodID(clazz, name, signature);
    if (nullptr == mid) {
        LOGE("Unable to find method %s", name);
        std::terminate();
    }
    return mid;
}

template<class PredictionTrait> jclass GVRActivityT<PredictionTrait>::GetGlobalClassReference(
        const char * className) const {
    jclass lc = jniMainThread_->FindClass(className);
    if (0 == lc) {
        LOGE("Unable to find class %s", className);
        std::terminate();
    }
    // Turn it into a global ref, so we can safely use it in the VR thread
    jclass gc = (jclass) jniMainThread_->NewGlobalRef(lc);
    jniMainThread_->DeleteLocalRef(lc);

    return gc;
}

template <class R> void GVRActivityT<R>::getFramebufferConfiguration(int& fbWidthOut, int& fbHeightOut,
        const int fbWidthDefault, const int fbHeightDefault, int& multiSamplesOut, ovrTextureFormat& textureFormatOut)
{
    JNIEnv& env = *oculusJavaGlThread_.Env;

    LOGV("GVRActivity: --- framebuffer configuration ---");

    jfieldID fid = env.GetFieldID(vrAppSettingsClass_, "framebufferPixelsWide", "I");
    fbWidthOut = env.GetIntField(vrAppSettings_, fid);
    if (-1 == fbWidthOut) {
        env.SetIntField(vrAppSettings_, fid, fbWidthDefault);
        fbWidthOut = fbWidthDefault;
    }
    LOGV("GVRActivity: --- width %d", fbWidthOut);

    fid = env.GetFieldID(vrAppSettingsClass_, "framebufferPixelsHigh", "I");
    fbHeightOut = env.GetIntField(vrAppSettings_, fid);
    if (-1 == fbHeightOut) {
        env.SetIntField(vrAppSettings_, fid, fbHeightDefault);
        fbHeightOut = fbHeightDefault;
    }
    LOGV("GVRActivity: --- height: %d", fbHeightOut);

    fid = env.GetFieldID(vrAppSettingsClass_, "eyeBufferParms", "Lorg/gearvrf/utility/VrAppSettings$EyeBufferParms;");
    jobject eyeParmsSettings = env.GetObjectField(vrAppSettings_, fid);
    jclass eyeParmsClass = env.GetObjectClass(eyeParmsSettings);
    fid = env.GetFieldID(eyeParmsClass, "multiSamples", "I");
    multiSamplesOut = env.GetIntField(eyeParmsSettings, fid);
    LOGV("GVRActivity: --- multisamples: %d", multiSamplesOut);

    fid = env.GetFieldID(eyeParmsClass, "colorFormat", "Lorg/gearvrf/utility/VrAppSettings$EyeBufferParms$ColorFormat;");
    jobject textureFormat = env.GetObjectField(eyeParmsSettings, fid);
    jmethodID mid = env.GetMethodID(env.GetObjectClass(textureFormat),"getValue","()I");
    int textureFormatValue = env.CallIntMethod(textureFormat, mid);
    switch (textureFormatValue){
    case 0:
        textureFormatOut = VRAPI_TEXTURE_FORMAT_565;
        break;
    case 1:
        textureFormatOut = VRAPI_TEXTURE_FORMAT_5551;
        break;
    case 2:
        textureFormatOut = VRAPI_TEXTURE_FORMAT_4444;
        break;
    case 3:
        textureFormatOut = VRAPI_TEXTURE_FORMAT_8888;
        break;
    case 4:
        textureFormatOut = VRAPI_TEXTURE_FORMAT_8888_sRGB;
        break;
    case 5:
        textureFormatOut = VRAPI_TEXTURE_FORMAT_RGBA16F;
        break;
    default:
        LOGE("fatal error: unknown texture format");
        std::terminate();
    }
    LOGV("GVRActivity: --- texture format: %d", textureFormatOut);
    LOGV("GVRActivity: ---------------------------------");
}

template<class R> void GVRActivityT<R>::getModeConfiguration(bool& allowPowerSaveOut,
        bool& resetWindowFullscreenOut) {
    JNIEnv& env = *oculusJavaGlThread_.Env;

    LOGV("GVRActivity: --- mode configuration ---");

    jfieldID fid = env.GetFieldID(vrAppSettingsClass_, "modeParms", "Lorg/gearvrf/utility/VrAppSettings$ModeParms;");
    jobject modeParms = env.GetObjectField(vrAppSettings_, fid);
    jclass modeParmsClass = env.GetObjectClass(modeParms);

    allowPowerSaveOut = env.GetBooleanField(modeParms, env.GetFieldID(modeParmsClass, "allowPowerSave", "Z"));
    LOGV("GVRActivity: --- allowPowerSave: %d", allowPowerSaveOut);
    resetWindowFullscreenOut = env.GetBooleanField(modeParms, env.GetFieldID(modeParmsClass, "resetWindowFullScreen","Z"));
    LOGV("GVRActivity: --- resetWindowFullscreen: %d", resetWindowFullscreenOut);

    LOGV("GVRActivity: --------------------------");
}

/**
 * @todo showLoadingIcon is ignored; implemented by the appFw; do we care about it? not used by pure - maybe
 * use it for the logo?
 */

//template <class R> void GVRActivityT<R>::Configure(OVR::ovrSettings & settings)
//{
////    //Settings for EyeBufferParms.
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
////
////
////    //Settings for ModeParms
////    jobject modeParms = env->GetObjectField(vrSettings, env->GetFieldID(vrAppSettingsClass, "modeParms", "Lorg/gearvrf/utility/VrAppSettings$ModeParms;"));
////    jclass modeParmsClass = env->GetObjectClass(modeParms);
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

template <class R> void GVRActivityT<R>::showGlobalMenu() {
    LOGV("GVRActivity::showGlobalMenu");
    SystemActivities_StartSystemActivity(&oculusJavaMainThread_, PUI_GLOBAL_MENU, NULL);
}

template <class R> void GVRActivityT<R>::showConfirmQuit() {
    LOGV("GVRActivity::showConfirmQuit");
    SystemActivities_StartSystemActivity(&oculusJavaMainThread_, PUI_CONFIRM_QUIT, NULL);
}

template<class R> bool GVRActivityT<R>::updateSensoredScene() {
    return oculusJavaGlThread_.Env->CallBooleanMethod(oculusJavaGlThread_.ActivityObject, updateSensoredSceneMethodId);
}

template<class R> void GVRActivityT<R>::setCameraRig(jlong cameraRig) {
    cameraRig_ = reinterpret_cast<CameraRig*>(cameraRig);
    sensoredSceneUpdated_ = false;
}

template<class R> void GVRActivityT<R>::onSurfaceCreated() {
    LOGV("GVRActivity::onSurfaceCreated");
}

template<class R> void GVRActivityT<R>::onSurfaceChanged() {
    LOGV("GVRActivityT::onSurfaceChanged");

    if (nullptr == oculusMobile_) {
        enterVrMode();

        int width, height, multisamples;
        ovrTextureFormat textureFormat;
        getFramebufferConfiguration(width, height,
                vrapi_GetSystemPropertyInt(&oculusJavaGlThread_, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH),
                vrapi_GetSystemPropertyInt(&oculusJavaGlThread_, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT),
                multisamples, textureFormat);

        for (int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++) {
            bool b = FrameBuffer[eye].create(textureFormat, width, height, multisamples);
        }

        ProjectionMatrix = ovrMatrix4f_CreateProjectionFov(
                vrapi_GetSystemPropertyFloat(&oculusJavaGlThread_, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_X),
                vrapi_GetSystemPropertyFloat(&oculusJavaGlThread_, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_Y), 0.0f, 0.0f, 1.0f,
                0.0f);
        TexCoordsTanAnglesMatrix = ovrMatrix4f_TanAngleMatrixFromProjection(&ProjectionMatrix);
    }
}

template<class R> void GVRActivityT<R>::onDrawFrame() {
    if (nullptr == oculusMobile_) {
        return;//use a notification for surfaceDestruction instead?
    }

    ovrFrameParms parms = vrapi_DefaultFrameParms(&oculusJavaGlThread_, VRAPI_FRAME_INIT_DEFAULT, vrapi_GetTimeInSeconds(),
            NULL);
    parms.FrameIndex = ++frameIndex;
    parms.MinimumVsyncs = 1;
    parms.PerformanceParms = vrapi_DefaultPerformanceParms();
    parms.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

    const double predictedDisplayTime = vrapi_GetPredictedDisplayTime(oculusMobile_, frameIndex);
    const ovrTracking baseTracking = vrapi_GetPredictedTracking(oculusMobile_, predictedDisplayTime);

    const ovrHeadModelParms headModelParms = vrapi_DefaultHeadModelParms();
    const ovrTracking tracking = vrapi_ApplyHeadModel(&headModelParms, &baseTracking);

    // Render the eye images.
    for (int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++) {
        ovrTracking updatedTracking = vrapi_GetPredictedTracking(oculusMobile_, tracking.HeadPose.TimeInSeconds);
        updatedTracking.HeadPose.Pose.Position = tracking.HeadPose.Pose.Position;
        //ovrTracking updatedTracking = *tracking;

        FrameBuffer[eye].setCurrent();
        GL(glViewport(0, 0, FrameBuffer[eye].mWidth, FrameBuffer[eye].mHeight));
        GL(glScissor(0, 0, FrameBuffer[eye].mWidth, FrameBuffer[eye].mHeight));

        //todo
        if (!sensoredSceneUpdated_ && headRotationProvider_.receivingUpdates()) {
            sensoredSceneUpdated_ = updateSensoredScene();
        }
        headRotationProvider_.predict(*this, parms, (1 == eye ? 4.0f : 3.5f) / 60.0f);
        oculusJavaGlThread_.Env->CallVoidMethod(activityRenderingCallbacks_, onDrawEyeMethodId, eye);

        FrameBuffer[eye].resolve();

        parms.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Textures[eye].ColorTextureSwapChain =
                FrameBuffer[eye].mColorTextureSwapChain;
        parms.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Textures[eye].TextureSwapChainIndex =
                FrameBuffer[eye].mTextureSwapChainIndex;
        for (int layer = 0; layer < VRAPI_FRAME_LAYER_TYPE_MAX; layer++) {
            parms.Layers[layer].Textures[eye].TexCoordsFromTanAngles = TexCoordsTanAnglesMatrix;
            parms.Layers[layer].Textures[eye].HeadPose = updatedTracking.HeadPose;
        }

        FrameBuffer[eye].advance();
    }

    FrameBufferObject::setNone();

    vrapi_SubmitFrame(oculusMobile_, &parms);
}

template<class R> void GVRActivityT<R>::initializeOculusJava(JNIEnv& env, ovrJava& oculusJava) {
    oculusJava.Env = &env;
    env.GetJavaVM(&oculusJava.Vm);
    oculusJava.ActivityObject = activity_;
}

template<class R> void GVRActivityT<R>::leaveVrMode() {
    LOGV("GVRActivity::leaveVrMode");

    if (nullptr != oculusMobile_) {
        for (int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++) {
            FrameBuffer[eye].destroy();
        }

        vrapi_LeaveVrMode(oculusMobile_);
        oculusMobile_ = nullptr;
    } else {
        LOGW("GVRActivity::leaveVrMode: ignored, have not entered vrMode");
    }
}

template<class R> void GVRActivityT<R>::enterVrMode() {
    LOGV("GVRActivity::enterVrMode");

    if (oculusMobile_) {
        LOGW("GVRActivity::enterVrMode: ignored, already entered vrMode");
        return;
    }

    ovrModeParms parms = vrapi_DefaultModeParms(&oculusJavaGlThread_);
    getModeConfiguration(parms.AllowPowerSave, parms.ResetWindowFullscreen);

    oculusMobile_ = vrapi_EnterVrMode(&parms);
}

template<class R> void GVRActivityT<R>::onDestroy() {
    LOGV("GVRActivity::onDestroy");

    SystemActivities_Shutdown(&oculusJavaMainThread_);
    vrapi_Shutdown();

    jniMainThread_->DeleteGlobalRef(activity_);
    jniMainThread_->DeleteGlobalRef(activityRenderingCallbacks_);

    jniMainThread_->DeleteGlobalRef(activityClass_);
    jniMainThread_->DeleteGlobalRef(activityRenderingCallbacksClass_);
    jniMainThread_->DeleteGlobalRef(vrAppSettingsClass_);
}

}

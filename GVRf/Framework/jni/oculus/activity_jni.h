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


#ifndef ACTIVITY_JNI_H
#define ACTIVITY_JNI_H

#include "view_manager.h"
#include "sensor/ksensor/k_sensor.h"
#include "../objects/components/camera.h"
#include "../objects/components/camera_rig.h"

#include "glm/glm.hpp"

#include "VrApi.h"
#include "VrApi_Types.h"
#include "VrApi_Helpers.h"

#include <android/native_window_jni.h>

namespace gvr {

//#define USE_FEATURE_KSENSOR

class OculusHeadRotation;
class KSensorHeadRotation;

typedef struct
{
    int                     Width;
    int                     Height;
    int                     Multisamples;
    int                     TextureSwapChainLength;
    int                     TextureSwapChainIndex;
    ovrTextureSwapChain *   ColorTextureSwapChain;
    GLuint *                DepthBuffers;
    GLuint *                FrameBuffers;
} ovrFramebuffer;


template <class R> class GVRActivityT
{
public:
    GVRActivityT(JNIEnv& jni, jobject activity);
    virtual ~GVRActivityT() {
        //todo
    }

//    virtual void        Configure( OVR::ovrSettings & settings );
    virtual void        OneTimeShutdown();
//    virtual OVR::Matrix4f    Frame( const OVR::VrFrame & vrFrame );
//    virtual bool        OnKeyEvent( const int keyCode, const int repeatCount, const OVR::KeyEventType eventType );
    bool                updateSensoredScene();
    void                setCameraRig(jlong cameraRig);
    void                initJni();

    // When launched by an intent, we may be viewing a partial
    // scene for debugging, so always clear the screen to grey
    // before drawing, instead of letting partial renders show through.
    bool                forceScreenClear;
    bool                ModelLoaded;

    GVRViewManager*     viewManager = nullptr;

    Camera*             camera = nullptr;
    CameraRig*          cameraRig_ = nullptr;   // this needs a global ref on the java object; todo
    bool                sensoredSceneUpdated_ = false;
    R                   headRotationProvider_;

private:
    JNIEnv*             uiJni;            // for use by the Java UI thread

    jclass              activityClass = nullptr;    // must be looked up from main thread or FindClass() will fail

    jclass              vrAppSettingsClass = nullptr;
    jclass              eyeBufferParmsClass = nullptr;

    jmethodID           getAppSettingsMethodId = nullptr;

    jmethodID           oneTimeShutdownMethodId = nullptr;

    jmethodID           drawEyeViewMethodId = nullptr;

    jmethodID           onKeyEventNativeMethodId = nullptr;
    jmethodID           updateSensoredSceneMethodId = nullptr;

    jclass              GetGlobalClassReference( const char * className ) const;
    jmethodID           GetMethodID( const char * name, const char * signature );
    jmethodID           GetStaticMethodID( jclass activityClass, const char * name, const char * signature );

public:
    void onSurfaceCreated(ANativeWindow* nativeWindow);
    void onDrawFrame();
    void setupOculusJava(JNIEnv* env);
    void onPause();

    jobject activity_;

    ovrJava oculusJava_;
    ovrMobile* oculusMobile_ = nullptr;
    long long frameIndex = 1;
    ovrFramebuffer  FrameBuffer[VRAPI_FRAME_LAYER_EYE_MAX];
    ovrMatrix4f     ProjectionMatrix;
    ovrMatrix4f     TexCoordsTanAnglesMatrix;
};


#ifdef USE_FEATURE_KSENSOR
typedef GVRActivityT<KSensorHeadRotation> GVRActivity;
#else
typedef GVRActivityT<OculusHeadRotation> GVRActivity;
#endif

class KSensorHeadRotation {
public:
//    void predict(GVRActivityT<KSensorHeadRotation>& gvrActivity, const ovrFrameParms&, const float time) {
//        if (nullptr != gvrActivity.cameraRig_) {
//            if (nullptr == sensor_.get()) {
//                gvrActivity.cameraRig_->predict(time);
//            } else {
//                sensor_->convertTo(rotationSensorData_);
//                gvrActivity.cameraRig_->predict(time, rotationSensorData_);
//            }
//        } else {
//            gvrActivity.cameraRig_->setRotation(glm::quat());
//        }
//    }
    bool receivingUpdates() {
        return rotationSensorData_.hasBeenUpdated();
    }
    void onDock() {
        sensor_.reset(new KSensor());
        sensor_->start();
    }
    void onUndock() {
        if (nullptr != sensor_.get()) {
            sensor_->stop();
            sensor_.reset(nullptr);
        }
    }

public:
    std::unique_ptr<KSensor> sensor_;
    RotationSensorData rotationSensorData_;
};

class OculusHeadRotation {
    bool docked_ = false;
public:
    void predict(GVRActivityT<OculusHeadRotation>& gvrActivity, const ovrFrameParms& frameParms, const float time) {
        if (docked_) {
            ovrMobile* ovr = gvrActivity.oculusMobile_;
            ovrTracking tracking = vrapi_GetPredictedTracking(ovr, vrapi_GetPredictedDisplayTime(ovr, frameParms.FrameIndex));
            ovrHeadModelParms headModelParms = vrapi_DefaultHeadModelParms();
            tracking = vrapi_ApplyHeadModel(&headModelParms, &tracking);

            const ovrQuatf& orientation = tracking.HeadPose.Pose.Orientation;
            glm::quat quat(orientation.w, orientation.x, orientation.y, orientation.z);
            gvrActivity.cameraRig_->setRotation(glm::conjugate(glm::inverse(quat)));
        } else if (nullptr != gvrActivity.cameraRig_) {
            gvrActivity.cameraRig_->predict(time);
        } else {
            gvrActivity.cameraRig_->setRotation(glm::quat());
        }
    }
    bool receivingUpdates() {
        return docked_;
    }
    void onDock() {
        docked_ = true;
    }
    void onUndock() {
        docked_ = false;
    }
};

}
#endif

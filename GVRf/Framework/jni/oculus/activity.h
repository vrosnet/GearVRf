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
#include "framebufferobject.h"
#include "../objects/components/camera.h"
#include "../objects/components/camera_rig.h"
#include "../util/configuration_helper.h"
#include "head_rotation_provider.h"
#include "VrApi_Types.h"

namespace gvr {

class GVRActivity
{
public:
    GVRActivity(JNIEnv& jni, jobject activity, jobject vrAppSettings, jobject callbacks);
    ~GVRActivity();

    bool updateSensoredScene();
    void setCameraRig(jlong cameraRig);

    GVRViewManager viewManager_;

    Camera* camera = nullptr;
    CameraRig* cameraRig_ = nullptr;   // this needs a global ref on the java object; todo
    bool sensoredSceneUpdated_ = false;
    HeadRotationProvider headRotationProvider_;

private:
    JNIEnv* envMainThread_ = nullptr;           // for use by the Java UI thread

    jclass activityClass_ = nullptr;            // must be looked up from main thread or FindClass() will fail
    jclass activityRenderingCallbacksClass_ = nullptr;

    jmethodID onDrawEyeMethodId = nullptr;
    jmethodID updateSensoredSceneMethodId = nullptr;

    jobject activity_;
    jobject activityRenderingCallbacks_;

    ConfigurationHelper configurationHelper_;

    ovrJava oculusJavaMainThread_;
    ovrJava oculusJavaGlThread_;
    ovrMobile* oculusMobile_ = nullptr;
    long long frameIndex = 1;
    FrameBufferObject FrameBuffer[VRAPI_FRAME_LAYER_EYE_MAX];
    ovrMatrix4f ProjectionMatrix;
    ovrMatrix4f TexCoordsTanAnglesMatrix;
    ovrPerformanceParms oculusPerformanceParms_;
    ovrHeadModelParms oculusHeadModelParms_;

    void initializeOculusJava(JNIEnv& env, ovrJava& oculusJava);

public:
    void onSurfaceCreated(JNIEnv& env);
    void onSurfaceChanged(JNIEnv& env);
    void onDrawFrame();
    int initializeVrApi();
    void leaveVrMode();

    void showGlobalMenu();
    void showConfirmQuit();

    ovrMobile* getOculusContext() { return oculusMobile_; }
    ovrHeadModelParms* getOculusHeadModelParms() { return &oculusHeadModelParms_; }
};

}
#endif

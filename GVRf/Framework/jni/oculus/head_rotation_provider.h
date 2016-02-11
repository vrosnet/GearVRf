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
#ifndef HEAD_ROTATION_PROVIDER_H
#define HEAD_ROTATION_PROVIDER_H

#include "VrApi_Types.h"

/**
 * To switch to gvrf's native orientation provider, uncomment the
 * following line
 */
//#define USE_FEATURE_KSENSOR

#ifdef USE_FEATURE_KSENSOR
#include "objects/rotation_sensor_data.h"
#include "sensor/ksensor/k_sensor.h"
#endif

namespace gvr {

class GVRActivity;

#ifdef USE_FEATURE_KSENSOR
class KSensorHeadRotation {
public:
    void predict(GVRActivity& gvrActivity, const ovrFrameParms&, const float time);
    bool receivingUpdates();
    void onDock();
    void onUndock();

public:
    std::unique_ptr<KSensor> sensor_;
    RotationSensorData rotationSensorData_;
};

typedef KSensorHeadRotation HeadRotationProvider;

#else
class OculusHeadRotation {
    bool docked_ = false;
public:
    void predict(GVRActivity& gvrActivity, const ovrFrameParms& frameParms, const float time);
    bool receivingUpdates();
    void onDock();
    void onUndock();
};

typedef OculusHeadRotation HeadRotationProvider;
#endif

}
#endif

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

package org.gearvrf;

import org.gearvrf.utility.VrAppSettings;

import android.app.Activity;

final class GVRActivityNative {

    static {
        System.loadLibrary("gvrf");
    }

    private long mPtr;

    static GVRActivityNative createObject(Activity act, VrAppSettings vrAppSettings,
            ActivityHandlerRenderingCallbacks callbacks) {
        final GVRActivityNative result = new GVRActivityNative(act, vrAppSettings, callbacks);
        if (0 != result.getNative()) {
            return result;
        }
        return null;
    }

    private GVRActivityNative(Activity act, VrAppSettings vrAppSettings, ActivityHandlerRenderingCallbacks callbacks) {
        mPtr = onCreate(act, vrAppSettings, callbacks);
    }

    void onDestroy() {
        onDestroy(mPtr);
    }

    void setCamera(GVRCamera camera) {
        setCamera(mPtr, camera.getNative());
    }

    void setCameraRig(GVRCameraRig cameraRig) {
        setCameraRig(mPtr, cameraRig.getNative());
    }

    void onUndock() {
        onUndock(mPtr);
    }

    void onDock() {
        onDock(mPtr);
    }

    long getNative() {
        return mPtr;
    }

    private static native void setCamera(long appPtr, long camera);

    private static native void setCameraRig(long appPtr, long cameraRig);

    private static native void onDock(long appPtr);

    private static native void onUndock(long appPtr);

    private static native void onDestroy(long appPtr);

    private static native long onCreate(Activity act, VrAppSettings vrAppSettings, ActivityHandlerRenderingCallbacks callbacks);

}

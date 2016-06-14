/* Copyright 2016 Samsung Electronics Co., LTD
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

final class VrapiActivityHandlerNative implements ActivityHandlerNative {

    static {
        System.loadLibrary("gvrf");
    }

    private long mPtr;

    static VrapiActivityHandlerNative createObject(Activity act, VrAppSettings vrAppSettings,
                                                   VrapiRenderingCallbacks callbacks) {
        return new VrapiActivityHandlerNative(act, vrAppSettings, callbacks);
    }

    private VrapiActivityHandlerNative(Activity act, VrAppSettings vrAppSettings, VrapiRenderingCallbacks callbacks) {
        mPtr = onCreate(act, vrAppSettings, callbacks);
    }

    @Override
    public void onDestroy() {
        onDestroy(mPtr);
    }

    @Override
    public void setCamera(GVRCamera camera) {
        setCamera(mPtr, camera.getNative());
    }

    @Override
    public void setCameraRig(GVRCameraRig cameraRig) {
        setCameraRig(mPtr, cameraRig.getNative());
    }

    @Override
    public void onUndock() {
        onUndock(mPtr);
    }

    @Override
    public void onDock() {
        onDock(mPtr);
    }

    @Override
    public long getPtr() {
        return mPtr;
    }

    private static native void setCamera(long appPtr, long camera);

    private static native void setCameraRig(long appPtr, long cameraRig);

    private static native void onDock(long appPtr);

    private static native void onUndock(long appPtr);

    private static native void onDestroy(long appPtr);

    private static native long onCreate(Activity act, VrAppSettings vrAppSettings, VrapiRenderingCallbacks callbacks);

}

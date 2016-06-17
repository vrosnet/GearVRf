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

abstract class GVRActivityBaseNative {

    static {
        System.loadLibrary("gvrf");
    }

    private final long mPtr;

    GVRActivityBaseNative(final long ptr) {
        mPtr = ptr;
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

    protected static native void setCamera(long appPtr, long camera);

    protected static native void setCameraRig(long appPtr, long cameraRig);

    protected static native void onDock(long appPtr);

    protected static native void onUndock(long appPtr);

    protected static native void onDestroy(long appPtr);

}

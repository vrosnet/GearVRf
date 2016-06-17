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

class GVRActivityNative extends GVRActivityBaseNative {

    static GVRActivityNative createObject(Activity act, VrAppSettings vrAppSettings,
            ActivityHandlerRenderingCallbacks callbacks) {
        return new GVRActivityNative(act, vrAppSettings, callbacks);
    }

    private GVRActivityNative(Activity act, VrAppSettings vrAppSettings, ActivityHandlerRenderingCallbacks callbacks) {
        super(onCreate(act, vrAppSettings, callbacks));
    }

    protected static native long onCreate(Activity act, VrAppSettings vrAppSettings, ActivityHandlerRenderingCallbacks callbacks);
}

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

import android.content.res.Configuration;
import android.view.KeyEvent;

import org.gearvrf.utility.Log;
import org.gearvrf.utility.VrAppSettings;

/**
 * {@inheritDoc}
 */
final class OvrActivityDelegate implements GVRActivity.GVRActivityDelegate {
    private GVRActivity mThiz;
    private GVRViewManager mActiveViewManager;
    private GVRActivityNative mActivityNative;

    @Override
    public void onCreate(GVRActivity thiz) {
        mThiz = thiz;

        mActivityNative = new GVRActivityNative(mThiz, mThiz.getAppSettings(), mRenderingCallbacks);

        try {
            mActivityHandler = new VrapiActivityHandler(thiz, mActivityNative, mRenderingCallbacks);
        } catch (final Exception ignored) {
            // will fall back to mono rendering in that case
            mForceMonoscopic = true;
        }
    }

    @Override
    public GVRActivityNative getActivityNative() {
        return mActivityNative;
    }

    @Override
    public GVRViewManager makeViewManager(final GVRXMLParser xmlParser) {
        return new GVRViewManager(mThiz, mThiz.getScript(), xmlParser);
    }

    @Override
    public GVRMonoscopicViewManager makeMonoscopicViewManager(final GVRXMLParser xmlParser) {
        return new GVRMonoscopicViewManager(mThiz, mThiz.getScript(), xmlParser);
    }

    @Override
    public void onPause() {
        if (null != mActivityHandler) {
            mActivityHandler.onPause();
        }
    }

    @Override
    public void onResume() {
        if (null != mActivityHandler) {
            mActivityHandler.onResume();
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
    }

    @Override
    public void setScript(GVRScript gvrScript, String dataFileName) {
        if (mThiz.getAppSettings().getMonoscopicModeParams().isMonoscopicMode()) {
            mActivityHandler = null;
        } else if (null != mActivityHandler) {
            mActivityHandler.onSetScript();
        }
    }

    @Override
    public void setViewManager(GVRViewManager viewManager) {
        mActiveViewManager = viewManager;
    }

    @Override
    public void onInitAppSettings(VrAppSettings appSettings) {
        if (mForceMonoscopic) {
            appSettings.getMonoscopicModeParams().setMonoscopicMode(true);
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (KeyEvent.KEYCODE_BACK == keyCode) {
            event.startTracking();
            return true;
        }
        return false;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (!mThiz.isPaused() && KeyEvent.KEYCODE_BACK == keyCode) {
            if (null != mActivityHandler) {
                return mActivityHandler.onBack();
            }
        }
        return false;
    }

    @Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        if (KeyEvent.KEYCODE_BACK == keyCode) {
            if (null != mActivityHandler) {
                return mActivityHandler.onBackLongPress();
            }
        }
        return false;
    }

    private final ActivityHandlerRenderingCallbacks mRenderingCallbacks = new ActivityHandlerRenderingCallbacks() {
        @Override
        public void onSurfaceCreated() {
            mActiveViewManager.onSurfaceCreated();
        }

        @Override
        public void onSurfaceChanged(int width, int height) {
            mActiveViewManager.onSurfaceChanged(width, height);
        }

        @Override
        public void onBeforeDrawEyes() {
            mActiveViewManager.beforeDrawEyes();
            mActiveViewManager.onDrawFrame();
        }

        @Override
        public void onAfterDrawEyes() {
            mActiveViewManager.afterDrawEyes();
        }

        @Override
        public void onDrawEye(int eye) {
            try {
                mActiveViewManager.onDrawEyeView(eye);
            } catch (final Exception e) {
                Log.e(TAG, "error in onDrawEyeView", e);
            }
        }
    };

    private ActivityHandler mActivityHandler;
    private boolean mForceMonoscopic;

    private final static String TAG = "OvrActivityDelegate";
}

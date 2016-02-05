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

import org.gearvrf.scene_objects.GVRViewSceneObject;
import org.gearvrf.scene_objects.view.GVRView;
import org.gearvrf.utility.DockEventReceiver;
import org.gearvrf.utility.Log;
import org.gearvrf.utility.VrAppSettings;

import com.oculus.vrappframework.VrActivity;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.os.Build;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;

/**
 * The typical GVRF application will have a single Android {@link Activity},
 * which <em>must</em> descend from {@link GVRActivity}, not directly from
 * {@code Activity}.
 * 
 * {@code GVRActivity} creates and manages the internal classes which use sensor
 * data to manage a viewpoint, and thus present an appropriate stereoscopic view
 * of your scene graph. {@code GVRActivity} also gives GVRF a full-screen window
 * in landscape orientation with no title bar.
 */
public class GVRActivity extends Activity {

    private static final String TAG = Log.tag(GVRActivity.class);

    // these values are copy of enum KeyEventType in VrAppFramework/Native_Source/Input.h
    public static final int KEY_EVENT_NONE = 0;
    public static final int KEY_EVENT_SHORT_PRESS = 1;
    public static final int KEY_EVENT_DOUBLE_TAP = 2;
    public static final int KEY_EVENT_LONG_PRESS = 3;
    public static final int KEY_EVENT_DOWN = 4;
    public static final int KEY_EVENT_UP = 5;
    public static final int KEY_EVENT_MAX = 6;

    private GVRViewManager mViewManager;
    private GVRCamera mCamera;
    private VrAppSettings mAppSettings;
    private long mPtr;

    // Group of views that are going to be drawn
    // by some GVRViewSceneObject to the scene.
    private ViewGroup mRenderableViewGroup = null;

    static {
        System.loadLibrary("gvrf");
    }

    public interface ActivityHandlerRenderingCallbacks {
        public void onSurfaceCreated();

        public void onSurfaceChanged(int width, int height);

        public void onBeforeDrawEyes();

        public void onDrawEye(int eye);

        public void onAfterDrawEyes();
    }

    public interface ActivityHandler {
        public long onCreate();

        public void onPause();

        public void onResume();

        public void onDestroy();

        public boolean onBack();

        public boolean onBackLongPress();
    }

    static native void nativeSetCamera(long appPtr, long camera);

    static native void nativeSetCameraRig(long appPtr, long cameraRig);

    static native void nativeOnDock(long appPtr);

    static native void nativeOnUndock(long appPtr);

    static native void nativeOnDestroy(long appPtr);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        android.util.Log.i(TAG, "onCreate " + Integer.toHexString(hashCode()));
        /*
         * Removes the title bar and the status bar.
         */
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        mAppSettings = new VrAppSettings();
        super.onCreate(savedInstanceState);

        mDockEventReceiver = new DockEventReceiver(this, mRunOnDock, mRunOnUndock);
        mRenderableViewGroup = (ViewGroup) findViewById(android.R.id.content).getRootView();

        mPtr = mActivityHandler.onCreate();
        if (0 == mPtr) {
            Log.e(TAG, "Unable to create native peer!");
            finish();
        }
    }

    protected void onInitAppSettings(VrAppSettings appSettings) {
    }

    public VrAppSettings getAppSettings() {
        return mAppSettings;
    }

    @Override
    protected void onPause() {
        android.util.Log.i(TAG, "onPause " + Integer.toHexString(hashCode()));
        if (mViewManager != null) {
            mViewManager.onPause();
        }
        if (null != mDockEventReceiver) {
            mDockEventReceiver.stop();
        }
        mActivityHandler.onPause();
        super.onPause();
    }

    @Override
    protected void onResume() {
        android.util.Log.i(TAG, "onResume " + Integer.toHexString(hashCode()));
        super.onResume();
        if (mViewManager != null) {
            mViewManager.onResume();
        }
        if (null != mDockEventReceiver) {
            mDockEventReceiver.start();
        }
        mActivityHandler.onResume();
    }

    @Override
    protected void onDestroy() {
        android.util.Log.i(TAG, "onDestroy " + Integer.toHexString(hashCode()));
        if (mViewManager != null) {
            mViewManager.onDestroy();
        }
        nativeOnDestroy(mPtr);
        super.onDestroy();
    }

    /**
     * Links {@linkplain GVRScript a script} to the activity; sets the version;
     * sets the lens distortion compensation parameters.
     * 
     * @param gvrScript
     *            An instance of {@link GVRScript} to handle callbacks on the GL
     *            thread.
     * @param distortionDataFileName
     *            Name of the XML file containing the device parameters. We
     *            currently only support the Galaxy Note 4 because that is the
     *            only shipping device with the proper GL extensions. When more
     *            devices support GVRF, we will publish new device files, along
     *            with app-level auto-detect guidelines. This approach will let
     *            you support new devices, using the same version of GVRF that
     *            you have already tested and approved.
     * 
     *            <p>
     *            The XML filename is relative to the application's
     *            {@code assets} directory, and can specify a file in a
     *            directory under the application's {@code assets} directory.
     */
    public void setScript(GVRScript gvrScript, String distortionDataFileName) {
        if (getRequestedOrientation() == ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE) {

            GVRXMLParser xmlParser = new GVRXMLParser(getAssets(),
                    distortionDataFileName, mAppSettings);
            onInitAppSettings(mAppSettings);
            if (isVrSupported() && !mAppSettings.getMonoScopicModeParms().isMonoScopicMode()) {
                mViewManager = new GVRViewManager(this, gvrScript, xmlParser);
            } else {
                mViewManager = new GVRMonoscopicViewManager(this, gvrScript,
                        xmlParser);
            }
        } else {
            throw new IllegalArgumentException(
                    "You can not set orientation to portrait for GVRF apps.");
        }
    }

    /**
     * Sets whether to force rendering to be single-eye, monoscopic view.
     * 
     * @param force
     *            If true, will create a GVRMonoscopicViewManager when
     *            {@linkplain setScript setScript()} is called. If false, will
     *            proceed to auto-detect whether the device supports VR
     *            rendering and choose the appropriate ViewManager. This call
     *            will only have an effect if it is called before
     *            {@linkplain #setScript(GVRScript, String) setScript()}.
     * @deprecated
     */
    @Deprecated
    public void setForceMonoscopic(boolean force) {
        mAppSettings.monoScopicModeParms.setMonoScopicMode(force);
    }

    /**
     * Returns whether a monoscopic view was asked to be forced during
     * {@linkplain #setScript(GVRScript, String) setScript()}.
     * 
     * @see setForceMonoscopic
     * @deprecated
     */
    @Deprecated
    public boolean getForceMonoscopic() {
        return mAppSettings.monoScopicModeParms.isMonoScopicMode();
    }

    private boolean isVrSupported() {
        if (isNote4() || isS6() || isS6Edge() || isS6EdgePlus() || isNote5()) {
            return true;
        }

        return false;
    }

    public boolean isNote4() {
        return Build.MODEL.contains("SM-N910")
                || Build.MODEL.contains("SM-N916");
    }

    public boolean isS6() {
        return Build.MODEL.contains("SM-G920");
    }

    public boolean isS6Edge() {
        return Build.MODEL.contains("SM-G925");
    }

    public boolean isS6EdgePlus() {
        return Build.MODEL.contains("SM-G928");
    }

    public boolean isNote5() {
        return Build.MODEL.contains("SM-N920");
    }

    public long getAppPtr() {
        return mPtr;
    }

    void oneTimeShutDown() {
        Log.e(TAG, " oneTimeShutDown from native layer");
    }

    void setCamera(GVRCamera camera) {
        mCamera = camera;
        nativeSetCamera(mPtr, camera.getNative());
    }

    void setCameraRig(GVRCameraRig cameraRig) {
        nativeSetCameraRig(mPtr, cameraRig.getNative());
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        boolean handled = mViewManager.dispatchKeyEvent(event);
        if (handled == false) {
            handled = super.dispatchKeyEvent(event);
        }
        return handled;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (KeyEvent.KEYCODE_BACK == keyCode) {
            event.startTracking();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (KeyEvent.KEYCODE_BACK == keyCode) {
            return mActivityHandler.onBack();
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        if (KeyEvent.KEYCODE_BACK == keyCode) {
            return mActivityHandler.onBackLongPress();
        }
        return super.onKeyLongPress(keyCode, event);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event) {
        boolean handled = mViewManager.dispatchMotionEvent(event);
        if (handled == false) {
            handled = super.dispatchGenericMotionEvent(event);
        }
        return handled;
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        boolean handled = mViewManager.dispatchMotionEvent(event);
        if (handled == false) {
            handled = super.dispatchTouchEvent(event);// VrActivity's
        }
        return handled;
    }

    boolean updateSensoredScene() {
        return mViewManager.updateSensoredScene();
    }

    /**
     * It is a convenient function to add a {@link GVRView} to Android hierarchy
     * view. UI thread will call {@link GVRView#draw(android.graphics.Canvas)}
     * to refresh the view when necessary.
     *
     * @param view Is a {@link GVRView} that draw itself into some
     *            {@link GVRViewSceneObject}.
     */
    public void registerView(View view) {
        mRenderableViewGroup.addView(view);
    }

    /**
     * Remove a child view of Android hierarchy view .
     * 
     * @param view View to be removed.
     */
    public void unregisterView(View view) {
        mRenderableViewGroup.removeView(view);
    }

    public GVRContext getGVRContext() {
        return mViewManager;
    }

    private final Runnable mRunOnDock = new Runnable() {
        @Override
        public void run() {
            if (0 != mPtr) {
                nativeOnDock(mPtr);
            }
        }
    };

    private final Runnable mRunOnUndock = new Runnable() {
        @Override
        public void run() {
            if (0 != mPtr) {
                nativeOnUndock(mPtr);
            }
        }
    };

    private DockEventReceiver mDockEventReceiver;

    private final ActivityHandlerRenderingCallbacks mRenderingCallbacks = new ActivityHandlerRenderingCallbacks() {
        @Override
        public void onSurfaceCreated() {
            mViewManager.onSurfaceCreated();
        }

        @Override
        public void onSurfaceChanged(int width, int height) {
            mViewManager.onSurfaceChanged(width, height);
        }

        @Override
        public void onBeforeDrawEyes() {
            mViewManager.beforeDrawEyes();
            mViewManager.onDrawFrame();
        }

        @Override
        public void onAfterDrawEyes() {
            mViewManager.afterDrawEyes();
        }

        @Override
        public void onDrawEye(int eye) {
            try {
                mViewManager.onDrawEyeView(eye);
            } catch (final Exception e) {
                Log.e(TAG, "error in onDrawEyeView", e);
            }
        }
    };
    private ActivityHandler mActivityHandler = new VrapiActivityHandler(this, mRenderingCallbacks);
}

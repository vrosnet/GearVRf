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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.gearvrf.scene_objects.GVRViewSceneObject;
import org.gearvrf.scene_objects.view.GVRView;
import org.gearvrf.script.IScriptable;
import org.gearvrf.utility.DockEventReceiver;
import org.gearvrf.utility.GrowBeforeQueueThreadPoolExecutor;
import org.gearvrf.utility.Log;
import org.gearvrf.utility.Threads;
import org.gearvrf.utility.VrAppSettings;
import org.joml.Vector2f;

import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
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
public class GVRActivity extends Activity implements IEventReceiver, IScriptable {

    protected static final String TAG = "GVRActivity";

    // these values are copy of enum KeyEventType in VrAppFramework/Native_Source/Input.h
    public static final int KEY_EVENT_NONE = 0;
    public static final int KEY_EVENT_SHORT_PRESS = 1;
    public static final int KEY_EVENT_DOUBLE_TAP = 2;
    public static final int KEY_EVENT_LONG_PRESS = 3;
    public static final int KEY_EVENT_DOWN = 4;
    public static final int KEY_EVENT_UP = 5;
    public static final int KEY_EVENT_MAX = 6;

    private GVRViewManager mViewManager;
    private GVRScript mGVRScript;
    private GVRMain mGVRMain;
    private VrAppSettings mAppSettings;
    private static View mFullScreenView = null;

    // Group of views that are going to be drawn
    // by some GVRViewSceneObject to the scene.
    private ViewGroup mRenderableViewGroup = null;
    private GVRActivityNative mActivityNative;
    private boolean mPaused = true;

    // Send to listeners and scripts but not this object itself
    private static final int SEND_EVENT_MASK =
            GVREventManager.SEND_MASK_ALL & ~GVREventManager.SEND_MASK_OBJECT;

    private GVREventReceiver mEventReceiver = new GVREventReceiver(this);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        android.util.Log.i(TAG, "onCreate " + Integer.toHexString(hashCode()));
        super.onCreate(savedInstanceState);
        mAppSettings = new VrAppSettings(); //needs to happen early

        final InputStream inputStream = getResources().openRawResource(R.raw.backends);
        final BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
        String line = null;
        try {
            line = reader.readLine();
            final Class<?> aClass = Class.forName(line);
            mDelegate = (GVRActivityDelegate) aClass.newInstance();
        } catch (final Exception e) {
            Log.e(TAG, "fatal error:", e);
            finish();
            return;
        }

        mDelegate.onCreate(this);

        if (null != Threads.getThreadPool()) {
            Threads.getThreadPool().shutdownNow();
        }
        Threads.setThreadPool(new GrowBeforeQueueThreadPoolExecutor("gvrf"));

        /*
         * Removes the title bar and the status bar.
         */
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

        mRenderableViewGroup = (ViewGroup) findViewById(android.R.id.content).getRootView();
        mDockEventReceiver = new DockEventReceiver(this,
                new Runnable() {
                    @Override
                    public void run() {
                        handleOnDock();
                    }
                }, new Runnable() {
                    @Override
                    public void run() {
                        handleOnUndock();
                    }
                });
        mDockEventReceiver.start();

        mActivityNative = mDelegate.getActivityNative();
    }

    protected void onInitAppSettings(VrAppSettings appSettings) {
        mDelegate.onInitAppSettings(appSettings);
    }

    private void onConfigure() {
        GVRConfigurationManager.onInitialize(this, getAppSettings());
        GVRConfigurationManager.getInstance().invalidate();

        onInitAppSettings(mAppSettings);
        startDockEventReceiver();
    }

    public final VrAppSettings getAppSettings() {
        return mAppSettings;
    }

    final GVRViewManager getViewManager() {
        return mViewManager;
    }

    final boolean isPaused() {
        return mPaused;
    }

    @Override
    protected void onPause() {
        android.util.Log.i(TAG, "onPause " + Integer.toHexString(hashCode()));

        mDelegate.onPause();
        mPaused = true;
        if (mViewManager != null) {
            mViewManager.onPause();

            mViewManager.getEventManager().sendEventWithMask(
                    SEND_EVENT_MASK,
                    this,
                    IActivityEvents.class,
                    "onPause");
        }
        super.onPause();
    }

    @Override
    protected void onResume() {
        android.util.Log.i(TAG, "onResume " + Integer.toHexString(hashCode()));

        mDelegate.onResume();
        mPaused = false;
        super.onResume();
        if (mViewManager != null) {
            mViewManager.onResume();

            mViewManager.getEventManager().sendEventWithMask(
                    SEND_EVENT_MASK,
                    this,
                    IActivityEvents.class,
                    "onResume");
        }
    }

    @Override
    protected void onDestroy() {
        android.util.Log.i(TAG, "onDestroy " + Integer.toHexString(hashCode()));
        if (mViewManager != null) {
            mViewManager.onDestroy();

            mViewManager.getEventManager().sendEventWithMask(
                    SEND_EVENT_MASK,
                    this,
                    IActivityEvents.class,
                    "onDestroy");
        }
        if (null != mDockEventReceiver) {
            mDockEventReceiver.stop();
        }

        mActivityNative.onDestroy();
        mActivityNative = null;
        super.onDestroy();
    }

    /**
     * Links {@linkplain GVRScript a script} to the activity; sets the version;
     * 
     * @param gvrScript
     *            An instance of {@link GVRScript} to handle callbacks on the GL
     *            thread.
     * @param dataFileName
     *            Name of the XML file containing the framebuffer parameters. 
     * 
     *            <p>
     *            The XML filename is relative to the application's
     *            {@code assets} directory, and can specify a file in a
     *            directory under the application's {@code assets} directory.
     * @deprecated
     */
    public void setScript(GVRScript gvrScript, String dataFileName) {
        this.mGVRScript = gvrScript;
        if (getRequestedOrientation() == ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE) {
            GVRXMLParser xmlParser = new GVRXMLParser(getAssets(),
                    dataFileName, mAppSettings);
            onConfigure();
            mDelegate.setScript(gvrScript, dataFileName);

            if (!mAppSettings.getMonoscopicModeParams().isMonoscopicMode()) {
                mViewManager = mDelegate.makeViewManager(xmlParser);
            } else {
                mViewManager = mDelegate.makeMonoscopicViewManager(xmlParser);
            }
            mDelegate.setViewManager(mViewManager);

            if (gvrScript instanceof GVRMain) {
                mViewManager.setUseTheFrameworkThread(true);
            }

            mViewManager.getEventManager().sendEventWithMask(
                    SEND_EVENT_MASK,
                    this,
                    IActivityEvents.class,
                    "onSetScript", gvrScript);

            if (null != mDockEventReceiver && !mAppSettings.getMonoscopicModeParams().isMonoscopicMode()) {
                getGVRContext().registerDrawFrameListener(new GVRDrawFrameListener() {
                    @Override
                    public void onDrawFrame(float frameTime) {
                        if (GVRConfigurationManager.getInstance().isHmtConnected()) {
                            handleOnDock();
                            getGVRContext().unregisterDrawFrameListener(this);
                        }
                    }
                });
            }
        } else {
            throw new IllegalArgumentException(
                    "You can not set orientation to portrait for GVRF apps.");
        }
    }

    /**
     * Invalidating just the GVRView associated with the GVRViewSceneObject
     * incorrectly set the clip rectangle to just that view. To fix this,
     * we have to create a full screen android View and invalidate this
     * to restore the clip rectangle.
     */
    public View getFullScreenView() {
        if (mFullScreenView != null) {
            return mFullScreenView;
        }

        final DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metrics);
        final int screenWidthPixels = Math.max(metrics.widthPixels, metrics.heightPixels);
        final int screenHeightPixels = Math.min(metrics.widthPixels, metrics.heightPixels);

        final ViewGroup.LayoutParams layout = new ViewGroup.LayoutParams(screenWidthPixels, screenHeightPixels);
        mFullScreenView = new View(this);
        mFullScreenView.setLayoutParams(layout);
        mRenderableViewGroup.addView(mFullScreenView);

        return mFullScreenView;
    }

    /**
     * Gets the {@linkplain GVRScript a script} linked to the activity.
     * @return the {@link GVRScript}.
     * @deprecated
     */
    public final GVRScript getScript() {
        return mGVRScript;
    }

    /**
     * Links {@linkplain GVRMain} to the activity; sets the version;
     * 
     * @param gvrMain
     *            An instance of {@link GVRMain} to handle callbacks on the framework
     *            thread.
     * @param dataFileName
     *            Name of the XML file containing the framebuffer parameters. 
     *            <p>
     *            The XML filename is relative to the application's
     *            {@code assets} directory, and can specify a file in a
     *            directory under the application's {@code assets} directory.
     */
    public final void setMain(GVRMain gvrMain, String dataFileName) {
        this.mGVRScript = gvrMain;
        this.mGVRMain = gvrMain;
        setScript(gvrMain, dataFileName);
    }

    /**
     * Gets the {@linkplain GVRMain} linked to the activity.
     * @return the {@link GVRMain}.
     */
    public final GVRMain getMain() {
        return mGVRMain;
    }

    /**
     * Sets whether to force rendering to be single-eye, monoscopic view.
     * 
     * @param force
     *            If true, will create a GVRMonoscopicViewManager when
     *            {@linkplain setMain setMain()} is called. If false, will
     *            proceed to auto-detect whether the device supports VR
     *            rendering and choose the appropriate ViewManager. This call
     *            will only have an effect if it is called before
     *            {@linkplain #setMain(GVRMain, String) setMain()}.
     * @deprecated
     */
    @Deprecated
    public final void setForceMonoscopic(boolean force) {
        mAppSettings.monoscopicModeParams.setMonoscopicMode(force);
    }

    /**
     * Returns whether a monoscopic view was asked to be forced during
     * {@linkplain #setMain(GVRMain, String) setMain()}.
     * 
     * @see setForceMonoscopic
     * @deprecated
     */
    @Deprecated
    public final boolean getForceMonoscopic() {
        return mAppSettings.monoscopicModeParams.isMonoscopicMode();
    }

    final long getNative() {
        return mActivityNative.getNative();
    }

    final GVRActivityNative getActivityNative() {
        return mActivityNative;
    }

    final void setCameraRig(GVRCameraRig cameraRig) {
        mActivityNative.setCameraRig(cameraRig);
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
        if (mDelegate.onKeyDown(keyCode, event)) {
            return true;
        }

        if (KeyEvent.KEYCODE_BACK == keyCode) {
            event.startTracking();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (mDelegate.onKeyUp(keyCode, event)) {
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        if (mDelegate.onKeyLongPress(keyCode, event)) {
            return true;
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

        mViewManager.getEventManager().sendEventWithMask(
                SEND_EVENT_MASK,
                this,
                IActivityEvents.class,
                "dispatchTouchEvent", event);

        return handled;
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        mDelegate.onConfigurationChanged(newConfig);

        if (mViewManager != null) {
            mViewManager.getEventManager().sendEventWithMask(
                    SEND_EVENT_MASK,
                    this,
                    IActivityEvents.class,
                    "onConfigurationChanged", newConfig);
        }

        super.onConfigurationChanged(newConfig);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (mViewManager != null) {
            mViewManager.getEventManager().sendEventWithMask(
                    SEND_EVENT_MASK,
                    this,
                    IActivityEvents.class,
                    "onTouchEvent", event);
        }

        return super.onTouchEvent(event);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        if (mViewManager != null) {
            mViewManager.getEventManager().sendEventWithMask(
                    SEND_EVENT_MASK,
                    this,
                    IActivityEvents.class,
                    "onWindowFocusChanged", hasFocus);
        }

        super.onWindowFocusChanged(hasFocus);
    }

    final boolean updateSensoredScene() {
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
    public final void registerView(final View view) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                /* The full screen should be updated
                otherwise just the children's bounds may be refreshed. */
                mRenderableViewGroup.setClipChildren(false);

                mRenderableViewGroup.addView(view);
            }
        });
    }

    /**
     * Remove a child view of Android hierarchy view .
     * 
     * @param view View to be removed.
     */
    public final void unregisterView(final View view) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mRenderableViewGroup.removeView(view);
            }
        });
    }

    public final GVRContext getGVRContext() {
        return mViewManager;
    }

    @Override
    public final GVREventReceiver getEventReceiver() {
        return mEventReceiver;
    }

    private boolean mIsDocked = false;

    protected final void handleOnDock() {
        Log.i(TAG, "handleOnDock");
        final Runnable r = new Runnable() {
            @Override
            public void run() {
                if (!mIsDocked) {
                    mIsDocked = true;

                    if (null != mActivityNative) {
                        mActivityNative.onDock();
                    }

                    for (final DockListener dl : mDockListeners) {
                        dl.onDock();
                    }
                }
            }
        };
        runOnUiThread(r);
    }

    protected final void handleOnUndock() {
        Log.i(TAG, "handleOnUndock");
        final Runnable r = new Runnable() {
            @Override
            public void run() {
                if (mIsDocked) {
                    mIsDocked = false;

                    if (null != mActivityNative) {
                        mActivityNative.onUndock();
                    }

                    for (final DockListener dl : mDockListeners) {
                        dl.onUndock();
                    }
                }
            }
        };
        runOnUiThread(r);
    }

    static interface DockListener {
        void onDock();
        void onUndock();
    }

    private final List<DockListener> mDockListeners = new CopyOnWriteArrayList<DockListener>();

    final void addDockListener(final DockListener dl) {
        mDockListeners.add(dl);
    }

    private DockEventReceiver mDockEventReceiver;

    private void startDockEventReceiver() {
        mDockEventReceiver = GVRConfigurationManager.getInstance().makeDockEventReceiver(this,
                new Runnable() {
                    @Override
                    public void run() {
                        handleOnDock();
                    }
                }, new Runnable() {
                    @Override
                    public void run() {
                        handleOnUndock();
                    }
                });
        if (null != mDockEventReceiver) {
            mDockEventReceiver.start();
        }
    }

    private GVRActivityDelegate mDelegate;

    public interface GVRActivityDelegate {
        void onCreate(GVRActivity thiz);
        void onPause();
        void onResume();
        void onConfigurationChanged(final Configuration newConfig);

        boolean onKeyDown(int keyCode, KeyEvent event);
        boolean onKeyUp(int keyCode, KeyEvent event);
        boolean onKeyLongPress(int keyCode, KeyEvent event);

        void setScript(GVRScript gvrScript, String dataFileName);
        void setViewManager(GVRViewManager viewManager);
        void onInitAppSettings(VrAppSettings appSettings);

        GVRActivityNative getActivityNative();
        GVRViewManager makeViewManager(final GVRXMLParser xmlParser);
        GVRMonoscopicViewManager makeMonoscopicViewManager(final GVRXMLParser xmlParser);
    }
}

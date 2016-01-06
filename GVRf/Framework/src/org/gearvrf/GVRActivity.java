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

import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

import org.gearvrf.scene_objects.GVRViewSceneObject;
import org.gearvrf.scene_objects.view.GVRView;
import org.gearvrf.utility.DockEventReceiver;
import org.gearvrf.utility.Log;
import org.gearvrf.utility.VrAppSettings;

import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.EGLConfigChooser;
import android.os.Build;
import android.os.Bundle;
import android.os.HandlerThread;
import android.view.Choreographer;
import android.view.Choreographer.FrameCallback;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
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

    private GVRViewManager mGVRViewManager = null;
    private GVRCamera mCamera;
    private VrAppSettings mAppSettings;
    private long mPtr;

    // Group of views that are going to be drawn
    // by some GVRViewSceneObject to the scene.
    private ViewGroup mRenderableViewGroup = null;
    private GLSurfaceView mSurfaceView;
    private SurfaceView sv;

    static {
        System.loadLibrary("gvrf");
    }

    public static native long nativeOnCreate(GVRActivity act);
    public static native void nativeOnSurfaceCreated(long ptr, Surface s);
    public static native void nativeOnDrawFrame(long ptr);
    public static native void nativeOnPause(long ptr);

    static native void nativeSetCamera(long appPtr, long camera);
    static native void nativeSetCameraRig(long appPtr, long cameraRig);
    static native void nativeOnDock(long appPtr);
    static native void nativeOnUndock(long appPtr);
    ReentrantLock lock = new ReentrantLock();
    Condition condition = lock.newCondition();
    volatile boolean running;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
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

        mPtr = nativeOnCreate(this);

        sv = new SurfaceView(this);
        setContentView(sv);
        sv.getHolder().addCallback(new Callback() {
            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                // TODO Auto-generated method stub
                
            }
            
            @Override
            public void surfaceCreated(final SurfaceHolder holder) {
                Runnable r = new Runnable() {
                    @Override
                    public void run() {
                        lock.lock();
                        try {
                            running = true;
                            createPbufferContext(holder);
                            nativeOnSurfaceCreated(mPtr, holder.getSurface());

                            mGVRViewManager.onSurfaceCreated();
                            startChoreographerThreadIfNotStarted();

                            while (running) {
                                condition.await();

                                Log.i("mmarinov", "run : 1");
                                mGVRViewManager.beforeDrawEyes();
                                Log.i("mmarinov", "run : 2");
                                mGVRViewManager.onDrawFrame();
                                Log.i("mmarinov", "run : 3");
                                nativeOnDrawFrame(mPtr);
                                Log.i("mmarinov", "run : 4");
                                mGVRViewManager.afterDrawEyes();
                                Log.i("mmarinov", "run : 5");
                            }
                        } catch(final InterruptedException e){
                            Log.i("mmarinov", "interrupted: ");
                        } finally {
                            lock.unlock();
                        }
                        nativeOnPause(mPtr);
                    }
                };
                new Thread(r, "gl-for-sv-"+Integer.toHexString(sv.hashCode())).start();
            }
            
            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                // TODO Auto-generated method stub
                
            }
        });


//        mSurfaceView = new GLSurfaceView(this);
//        mSurfaceView.setEGLContextClientVersion(3);
//        mSurfaceView.setEGLConfigChooser(configChooser);

//        mSurfaceView.setRenderer(new Renderer() {
//            @Override
//            public void onSurfaceCreated(GL10 gl, EGLConfig config) {
//                Log.i("mmarinov", "onSurfaceCreated : ");
//                //mSurfaceView.getHolder().setFormat(PixelFormat.TRANSLUCENT);
////                mGVRViewManager.onSurfaceCreated();
////                GVRActivity.onSurfaceCreated(mPtr);
////
////                int width = 2560, height = 1440;
////                width = getAppSettings().getFramebufferPixelsWide();
////                height = getAppSettings().getFramebufferPixelsHigh();
////                mSurfaceView.getHolder().setFixedSize(width, height);
////
////                startChoreographerThreadIfNotStarted();
////                Log.i("mmarinov", "onSurfaceCreated : done");
//            }
//            @Override
//            public void onSurfaceChanged(GL10 gl, int ignored1, int ignored2) {
//                Log.i("mmarinov", "onSurfaceChanged : ");
//                mGVRViewManager.onSurfaceCreated();
//                GVRActivity.onSurfaceCreated(mPtr);
//
//                int width = 2560, height = 1440;
//                width = getAppSettings().getFramebufferPixelsWide();
//                height = getAppSettings().getFramebufferPixelsHigh();
//                mSurfaceView.getHolder().setFixedSize(width, height);
//
//                startChoreographerThreadIfNotStarted();
//                Log.i("mmarinov", "onSurfaceChanged : done");
//            }
//            @Override
//            public void onDrawFrame(GL10 gl) {
//                Log.i("mmarinov", "java:onDrawFrame : ");
//                mGVRViewManager.onDrawFrame();
//
//                mGVRViewManager.beforeDrawEyes();
//                GVRActivity.onDrawFrame(mPtr);
//                mGVRViewManager.afterDrawEyes();
//            }
//        });
//        mSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
//        setContentView(mSurfaceView);
    }

    private HandlerThread choreographerThread;
    private final void startChoreographerThreadIfNotStarted() {
        if (null == choreographerThread) {
            choreographerThread = new HandlerThread(
                    "gvrf-choreographer-" + Integer.toHexString(hashCode())) {
                @Override
                protected void onLooperPrepared() {
                    // Force callbacks from the Choreographer to happen off the
                    // UI thread.
                    Choreographer.getInstance().removeFrameCallback(frameCallback);
                    Choreographer.getInstance().postFrameCallback(frameCallback);
                }

                @Override
                public void run() {
                    try {
                        super.run();
                    } finally {
                        Choreographer.getInstance().removeFrameCallback(frameCallback);
                    }
                }
            };
            choreographerThread.start();
        }
    }

    protected void onInitAppSettings(VrAppSettings appSettings) {
    }

    public VrAppSettings getAppSettings(){
        return mAppSettings;
    }

    @Override
    protected void onPause() {
        if (mGVRViewManager != null) {
            mGVRViewManager.onPause();
        }
        if (null != mDockEventReceiver) {
            mDockEventReceiver.stop();
        }
        if (null != mSurfaceView) {
            mSurfaceView.onPause();
        }

        running = false;
        lock.lock();
        try {
            condition.signal();
        } finally {
            lock.unlock();
        }
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (null != mSurfaceView) {
            mSurfaceView.onResume();
        }
        if (mGVRViewManager != null) {
            mGVRViewManager.onResume();
        }
        if (null != mDockEventReceiver) {
            mDockEventReceiver.start();
        }
    }

    @Override
    protected void onDestroy() {
        if (mGVRViewManager != null) {
            mGVRViewManager.onDestroy();
        }

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
                mGVRViewManager = new GVRViewManager(this, gvrScript, xmlParser);
            } else {
                mGVRViewManager = new GVRMonoscopicViewManager(this, gvrScript,
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
     *            
     * @deprecated
     * 
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

    public long getAppPtr(){
        return mPtr;
    }

    void oneTimeShutDown() {
        Log.e(TAG, " oneTimeShutDown from native layer");

        GVRHybridObject.closeAll();
        NativeGLDelete.processQueues();
    }

    void onDrawEyeView(int eye) {
        try {
            mGVRViewManager.onDrawEyeView(eye);
        } catch (final Exception e) {
            Log.i("mmarinov", "onDrawEyeView : exception: " + e.getMessage());
            e.printStackTrace();
        }
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
//        boolean handled = mGVRViewManager.dispatchKeyEvent(event);
//        if (handled == false) {
//            handled = super.dispatchKeyEvent(event);// VrActivity's
//        }
//        return handled;
        return false;
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event) {
//        boolean handled = mGVRViewManager.dispatchMotionEvent(event);
//        if (handled == false) {
//            handled = super.dispatchGenericMotionEvent(event);// VrActivity's
//        }
//        return handled;
        return false;
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
//        boolean handled = mGVRViewManager.dispatchMotionEvent(event);
//
//        if (handled == false) {
//            handled = super.dispatchTouchEvent(event);// VrActivity's
//        }
//        /*
//         * Situation: while the super class VrActivity is implementing
//         * dispatchTouchEvent() without calling its own super
//         * dispatchTouchEvent(), we still need to call the
//         * VRTouchPadGestureDetector onTouchEvent. Call it here, similar way
//         * like in place of viewGroup.onInterceptTouchEvent()
//         */
//        onTouchEvent(event);
//
//        return handled;
        return false;
    }

    boolean onKeyEventNative(int keyCode, int eventType) {

        /*
         * Currently VrLib does not call Java onKeyDown()/onKeyUp() in the
         * Activity class. In stead, it calls VrAppInterface->OnKeyEvent if
         * defined in the native side, to give a chance to the app before it
         * intercepts. With this implementation, the developers can expect
         * consistently their key event methods are called as usual in case they
         * want to use the events. The parameter eventType matches with the
         * native side. It can be more than two, DOWN and UP, if the native
         * supports in the future.
         */

        switch (eventType) {
        case KEY_EVENT_SHORT_PRESS:
            return onKeyShortPress(keyCode);
        case KEY_EVENT_DOUBLE_TAP:
            return onKeyDoubleTap(keyCode);
        case KEY_EVENT_LONG_PRESS:
            return onKeyLongPress(keyCode, new KeyEvent(KeyEvent.ACTION_DOWN, keyCode));
        case KEY_EVENT_DOWN:
            return onKeyDown(keyCode, new KeyEvent(KeyEvent.ACTION_DOWN, keyCode));
        case KEY_EVENT_UP:
            return onKeyUp(keyCode, new KeyEvent(KeyEvent.ACTION_UP, keyCode));
        case KEY_EVENT_MAX:
            return onKeyMax(keyCode);
        default:
            return false;
        }
    }

    public boolean onKeyShortPress(int keyCode) {
        return false;
    }

    public boolean onKeyDoubleTap(int keyCode) {
        return false;
    }

    public boolean onKeyMax(int keyCode) {
        return false;
    }

    boolean updateSensoredScene() {
        return mGVRViewManager.updateSensoredScene();
    }

    /**
     * It is a convenient function to add a {@link GVRView} to Android hierarchy view.
     * UI thread will call {@link GVRView#draw(android.graphics.Canvas)} to refresh the view
     * when necessary.
     *
     * @param view Is a {@link GVRView} that draw itself into some {@link GVRViewSceneObject}.
     */
    public void registerView(View view) {
        mRenderableViewGroup.addView(view);
    }

    /**
     * Remove a child view of Android hierarchy view .
     * @param view View to be removed.
     */
    public void unregisterView(View view) {
        mRenderableViewGroup.removeView(view);
    }

    private final Runnable mRunOnDock = new Runnable() {
        @Override
        public void run() {
            nativeOnDock(getAppPtr());
        }
    };

    private final Runnable mRunOnUndock = new Runnable() {
        @Override
        public void run() {
            nativeOnUndock(getAppPtr());
        }
    };

    private DockEventReceiver mDockEventReceiver;


    EGLSurface pbufferSurface;
    EGLSurface mainSurface;
    private void createPbufferContext(SurfaceHolder holder) {
        EGL10 egl = (EGL10) EGLContext.getEGL();

        EGLDisplay eglDisplay = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
        int[] version = new int[2];
        if(!egl.eglInitialize(eglDisplay, version)) {
            throw new RuntimeException("eglInitialize failed");
        }

        EGLConfig config = configChooser.chooseConfig(egl, eglDisplay);

        final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
        int[] contextAttribs =
        {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL10.EGL_NONE
        };

        EGLContext context = egl.eglCreateContext( eglDisplay, config, EGL10.EGL_NO_CONTEXT, contextAttribs);
        if (context == EGL10.EGL_NO_CONTEXT )
        {
            throw new RuntimeException("eglCreateContext failed; error 0x" + Integer.toHexString(egl.eglGetError()));
        }
        int[] surfaceAttribs = {
            EGL10.EGL_WIDTH, 16,
            EGL10.EGL_HEIGHT, 16,
            EGL10.EGL_NONE
        };

        pbufferSurface = egl.eglCreatePbufferSurface( eglDisplay, config, surfaceAttribs);
        if ( EGL10.EGL_NO_SURFACE == pbufferSurface)
        {
            egl.eglDestroyContext( eglDisplay, context );
            throw new RuntimeException("Pixel buffer surface not created; error 0x" + Integer.toHexString(egl.eglGetError()));
        }
        if (!egl.eglMakeCurrent( eglDisplay, pbufferSurface, pbufferSurface, context ))
        {
            egl.eglDestroySurface( eglDisplay, pbufferSurface);
            egl.eglDestroyContext( eglDisplay, context );
            throw new RuntimeException("Failed to make current context; error 0x" + Integer.toHexString(egl.eglGetError()));
        }
        Log.i("mmarinov", "createPbufferContext : pbuffer " + Integer.toHexString(pbufferSurface.hashCode()));

        //////////
        {
            int[] surfaceAttribs2 = { EGL10.EGL_NONE };
            mainSurface = egl.eglCreateWindowSurface( eglDisplay, config, holder, surfaceAttribs2);
            if ( mainSurface == EGL10.EGL_NO_SURFACE )
            {
                Log.e(TAG, "eglCreateWindowSurface() failed: 0x%X", egl.eglGetError());
                return;
            }
        if (!egl.eglMakeCurrent( eglDisplay, mainSurface, mainSurface, context )) {
            Log.e(TAG, "eglMakeCurrent() failed: 0x%X", egl.eglGetError());
            return;
        }
        Log.i("mmarinov", "createPbufferContext : mainSurface " + Integer.toHexString(mainSurface.hashCode()));
        }
    }

    EGLConfigChooser configChooser = new EGLConfigChooser() {
        @Override
        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {
            int[] numberConfigs = new int[1];
            if (!egl.eglGetConfigs(display, null, 0, numberConfigs)) {
                throw new RuntimeException("Unable to retrieve number of egl configs available.");
            }
            EGLConfig[] configs = new EGLConfig[numberConfigs[0]];
            if (!egl.eglGetConfigs(display, configs, configs.length, numberConfigs)) {
                throw new RuntimeException("Unable to retrieve egl configs available.");
            }

            int[] configAttribs =
                {
                    EGL10.EGL_ALPHA_SIZE, 8, // need alpha for the multi-pass timewarp compositor
                    EGL10.EGL_BLUE_SIZE,  8,
                    EGL10.EGL_GREEN_SIZE, 8,
                    EGL10.EGL_RED_SIZE,   8,
                    EGL10.EGL_DEPTH_SIZE, 0,
                    EGL10.EGL_SAMPLES,    0,
                    EGL10.EGL_NONE
                };
            

            EGLConfig config = null;
            for (int i = 0; i < numberConfigs[0]; ++i)
            {
                int[] value = new int[1];

                final int EGL_OPENGL_ES3_BIT_KHR = 0x0040;
                if (!egl.eglGetConfigAttrib(display, configs[i], EGL10.EGL_RENDERABLE_TYPE, value)) {
                    Log.i(TAG, "eglGetConfigAttrib for EGL_RENDERABLE_TYPE failed");
                    continue;
                }
                if ( ( value[0] & EGL_OPENGL_ES3_BIT_KHR ) != EGL_OPENGL_ES3_BIT_KHR )
                {
                    continue;
                }

                if (!egl.eglGetConfigAttrib(display, configs[i], EGL10.EGL_SURFACE_TYPE, value )) {
                    Log.i(TAG, "eglGetConfigAttrib for EGL_SURFACE_TYPE failed");
                    continue;
                }
                if ( ( value[0] & ( EGL10.EGL_WINDOW_BIT | EGL10.EGL_PBUFFER_BIT ) ) != ( EGL10.EGL_WINDOW_BIT | EGL10.EGL_PBUFFER_BIT ) )
                {
                    continue;
                }

                int j = 0;
                for ( ; configAttribs[j] != EGL10.EGL_NONE; j += 2 )
                {
                    if (!egl.eglGetConfigAttrib(display, configs[i], configAttribs[j], value )) {
                        Log.i(TAG, "eglGetConfigAttrib for " + configAttribs[j] + " failed");
                        continue;
                    }
                    if ( value[0] != configAttribs[j + 1] )
                    {
                        break;
                    }
                }
                if ( configAttribs[j] == EGL10.EGL_NONE )
                {
                    config = configs[i];
                    break;
                }
            }
            return config;
        }
    };

    int i=0;
    private FrameCallback frameCallback = new FrameCallback() {
        @Override
        public void doFrame(long frameTimeNanos) {
            //mSurfaceView.requestRender();

            lock.lock();
            try {
                condition.signal();
            } finally {
                lock.unlock();
            }

            Choreographer.getInstance().postFrameCallback(this);
        }
    };
}

package org.gearvrf;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;

import org.gearvrf.utility.Log;

import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.EGLConfigChooser;
import android.opengl.GLSurfaceView.EGLContextFactory;
import android.opengl.GLSurfaceView.EGLWindowSurfaceFactory;
import android.opengl.GLSurfaceView.Renderer;
import android.os.HandlerThread;
import android.view.Choreographer;
import android.view.Choreographer.FrameCallback;
import android.view.Surface;

public class VrapiActivityHandler {

    private static final String TAG = "VrapiActivityHandler";

    private final GVRActivity mActivity;
    private long mPtr;
    private GLSurfaceView mSurfaceView;
    private GVRViewManager mViewManager;
    EGLSurface pbufferSurface;
    EGLSurface mainSurface;

    public VrapiActivityHandler(GVRActivity activity) {
        mActivity = activity;
    }

    public long onCreate() {
        mPtr = nativeOnCreate(mActivity);

        mSurfaceView = new GLSurfaceView(mActivity);
        mSurfaceView.setPreserveEGLContextOnPause(true);
        mSurfaceView.setEGLContextClientVersion(3);

        mSurfaceView.setEGLContextFactory(new EGLContextFactory() {
            @Override
            public void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context) {
            }
            
            @Override
            public EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig eglConfig) {
                final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
                int[] contextAttribs =
                {
                    EGL_CONTEXT_CLIENT_VERSION, 3,
                    EGL10.EGL_NONE
                };

                EGLContext context = egl.eglCreateContext( display, eglConfig, EGL10.EGL_NO_CONTEXT, contextAttribs);
                if (context == EGL10.EGL_NO_CONTEXT )
                {
                    throw new RuntimeException("eglCreateContext failed; error 0x" + Integer.toHexString(egl.eglGetError()));
                }
                return context;
            }
        });
        mSurfaceView.setEGLConfigChooser(mConfigChooser);
        mSurfaceView.setEGLWindowSurfaceFactory(new EGLWindowSurfaceFactory() {
            @Override
            public void destroySurface(EGL10 egl, EGLDisplay display, EGLSurface surface) {
                egl.eglDestroySurface(display, surface);
            }
            @Override
            public EGLSurface createWindowSurface(EGL10 egl, EGLDisplay display, EGLConfig config,
                    Object ignoredNativeWindow) {
                int[] surfaceAttribs = {
                        EGL10.EGL_WIDTH, 16,
                        EGL10.EGL_HEIGHT, 16,
                        EGL10.EGL_NONE
                    };
                pbufferSurface = egl.eglCreatePbufferSurface( display, config, surfaceAttribs);
                if ( EGL10.EGL_NO_SURFACE == pbufferSurface)
                {
                    throw new RuntimeException("Pixel buffer surface not created; error 0x" + Integer.toHexString(egl.eglGetError()));
                }
                return pbufferSurface;
            }
        });

        mSurfaceView.setRenderer(new Renderer() {
            EGLConfig mConfig;
            @Override
            public void onSurfaceCreated(GL10 gl, EGLConfig config) {
                Log.i("mmarinov", "onSurfaceCreated : ");

                mConfig = config;
//                EGL10 egl = (EGL10) EGLContext.getEGL();
//                EGLDisplay display = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
//                EGLContext context = egl.eglGetCurrentContext();
//                createWindowSurface(mSurfaceView.getHolder(), egl, display, config, context);
//
//                nativeOnSurfaceCreated(mPtr, mSurfaceView.getHolder().getSurface());
//
//                mGVRViewManager.onSurfaceCreated();
//                startChoreographerThreadIfNotStarted();

                //mSurfaceView.getHolder().setFormat(PixelFormat.TRANSLUCENT);
            }
            @Override
            public void onSurfaceChanged(GL10 gl, int ignored1, int ignored2) {
//                Log.i("mmarinov", "onSurfaceChanged : ");

                EGL10 egl = (EGL10) EGLContext.getEGL();
                EGLDisplay display = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
                EGLContext context = egl.eglGetCurrentContext();

                //this is the display surface timewarp will hijack
                mainSurface = egl.eglCreateWindowSurface(display, mConfig, mSurfaceView.getHolder(),
                        new int[] {
                                EGL10.EGL_NONE
                });
                if (mainSurface == EGL10.EGL_NO_SURFACE) {
                    Log.e(TAG, "eglCreateWindowSurface() failed: 0x%X", egl.eglGetError());
                    return;
                }
                if (!egl.eglMakeCurrent(display, mainSurface, mainSurface, context)) {
                    Log.e(TAG, "eglMakeCurrent() failed: 0x%X", egl.eglGetError());
                    return;
                }
                nativeOnSurfaceCreated(mPtr, mSurfaceView.getHolder().getSurface());

                //necessary to explicitly make the pbuffer current;todo?
                if (!egl.eglMakeCurrent( display, pbufferSurface, pbufferSurface, context ))
                {
                    egl.eglDestroySurface( display, pbufferSurface);
                    egl.eglDestroyContext( display, context );
                    throw new RuntimeException("Failed to make current context; error 0x" + Integer.toHexString(egl.eglGetError()));
                }

                mViewManager.onSurfaceCreated();
                startChoreographerThreadIfNotStarted();
            }
            @Override
            public void onDrawFrame(GL10 gl) {
                mViewManager.beforeDrawEyes();
                mViewManager.onDrawFrame();
                nativeOnDrawFrame(mPtr);
                mViewManager.afterDrawEyes();
            }
        });
        mSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        mActivity.setContentView(mSurfaceView);

        return mPtr;
    }

    public void onPause() {
        if (null != mSurfaceView) {
            mSurfaceView.onPause();
        }
        choreographerThread.quit();
        choreographerThread = null;
    }

    public void onResume() {
        startChoreographerThreadIfNotStarted();
        if (null != mSurfaceView) {
            mSurfaceView.onResume();
        }
    }

    public void setViewManager(GVRViewManager viewManager) {
        mViewManager = viewManager; //??
    }

    private final EGLConfigChooser mConfigChooser = new EGLConfigChooser() {
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

    private final FrameCallback mFrameCallback = new FrameCallback() {
        @Override
        public void doFrame(long frameTimeNanos) {
            mSurfaceView.requestRender();
            Choreographer.getInstance().postFrameCallback(this);
        }
    };

    private HandlerThread choreographerThread;
    private final void startChoreographerThreadIfNotStarted() {
        if (null == choreographerThread) {
            choreographerThread = new HandlerThread(
                    "gvrf-choreographer-" + Integer.toHexString(hashCode())) {
                @Override
                protected void onLooperPrepared() {
                    // Force callbacks from the Choreographer to happen off the
                    // UI thread.
                    Choreographer.getInstance().removeFrameCallback(mFrameCallback);
                    Choreographer.getInstance().postFrameCallback(mFrameCallback);
                }

                @Override
                public void run() {
                    try {
                        super.run();
                    } finally {
                        Choreographer.getInstance().removeFrameCallback(mFrameCallback);
                    }
                }
            };
            choreographerThread.start();
        }
    }

    private static native long nativeOnCreate(GVRActivity act);
    private static native void nativeOnSurfaceCreated(long ptr, Surface s);
    private static native void nativeOnDrawFrame(long ptr);
    private static native void nativeOnPause(long ptr);
}

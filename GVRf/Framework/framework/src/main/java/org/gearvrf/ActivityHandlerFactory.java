package org.gearvrf;

public class ActivityHandlerFactory {
    public static ActivityHandler makeActivityHandler(GVRActivity activity) {
        try {
            return new VrapiActivityHandler(activity);
        } catch (Exception e) {
            return new MonoscopicActivityHandler(activity);
        }
    }

    private static final class MonoscopicActivityHandler implements ActivityHandler {
        private final VrapiActivityHandlerNative mNative;

        MonoscopicActivityHandler(GVRActivity activity) {
            mNative = VrapiActivityHandlerNative.createObject(activity, activity.getAppSettings(),
                    new VrapiRenderingCallbacks() {
                        @Override
                        public void onSurfaceCreated() {
                        }

                        @Override
                        public void onSurfaceChanged(int width, int height) {
                        }

                        @Override
                        public void onBeforeDrawEyes() {
                        }

                        @Override
                        public void onDrawEye(int eye) {
                        }

                        @Override
                        public void onAfterDrawEyes() {
                        }
                    });
        }

        @Override
        public void onPause() {
        }

        @Override
        public void onResume() {
        }

        @Override
        public void onSetScript(GVRViewManager viewManager) {
        }

        @Override
        public boolean onBack() {
            return false;
        }

        @Override
        public boolean onBackLongPress() {
            return false;
        }

        @Override
        public ActivityHandlerNative getNative() {
            return mNative;
        }

        @Override
        public boolean isMonoscopic() {
            return true;
        }
    }

}

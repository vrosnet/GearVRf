package org.gearvrf;

public interface ActivityHandler {
    public void onPause();

    public void onResume();

    public void onSetScript(GVRViewManager viewManager);

    public boolean onBack();

    public boolean onBackLongPress();

    public ActivityHandlerNative getNative();

    public boolean isMonoscopic();
}
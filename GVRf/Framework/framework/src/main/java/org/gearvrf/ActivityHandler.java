package org.gearvrf;

import android.view.KeyEvent;

public interface ActivityHandler {
    public void onPause();

    public void onResume();

    public void onSetScript(GVRViewManager viewManager);

    public boolean onKeyUp(int keyCode, KeyEvent event);

    public boolean onBackLongPress();

    public ActivityHandlerNative getNative();

    public boolean isMonoscopic();
}
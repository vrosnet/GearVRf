package org.gearvrf;

interface ActivityHandler {
    public void onPause();

    public void onResume();

    public void onDestroy();

    public void onSetScript();

    public boolean onBack();

    public boolean onBackLongPress();
}
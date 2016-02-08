package org.gearvrf;

interface ActivityHandler {
    /**
     * 
     * @return non-0 on success
     */
    public long onCreate();

    public void onPause();

    public void onResume();

    public void onDestroy();

    public void onSetScript();

    public boolean onBack();

    public boolean onBackLongPress();
}
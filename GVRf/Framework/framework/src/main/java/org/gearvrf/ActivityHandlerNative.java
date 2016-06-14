package org.gearvrf;

public interface ActivityHandlerNative {

    void onDestroy();

    void setCamera(GVRCamera camera);

    void setCameraRig(GVRCameraRig cameraRig);

    void onUndock();

    void onDock();

    long getPtr();
}

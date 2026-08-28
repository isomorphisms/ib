package org.isomorphisms.ib.prepaint;

import android.app.NativeActivity;
import android.content.Intent;
import android.view.View;

/** Android owns lifecycle delivery; D owns application behavior and state. */
public final class IbActivity extends NativeActivity implements View.OnClickListener {
    static {
        System.loadLibrary("ib");
    }

    private static native void nativeActivityResult(int requestCode, int resultCode, Intent data);
    private static native void nativeNewIntent(Intent intent);
    private static native void nativeAction(int action);

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        nativeActivityResult(requestCode, resultCode, data);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        nativeNewIntent(intent);
    }

    @Override
    public void onClick(View view) {
        nativeAction(view.getId());
    }
}

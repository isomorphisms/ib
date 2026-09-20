package org.isomorphisms.ib.webview;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

public final class IncrementalLargePageLauncherActivity extends Activity {
    private static final String TARGET_URL =
        "https://console.cloud.google.com/agent-platform/studio/multimodal"
            + "?authuser=5"
            + "&project=isomorphismes-youtube-shorts"
            + "&supportedpurview=project"
            + "&model=gemini-3.7-flash"
            + "&region=global";

    @Override
    protected void onCreate(Bundle saved_instance_state) {
        super.onCreate(saved_instance_state);

        Intent intent = new Intent(this, IncrementalLargePageActivity.class);
        intent.putExtra("url", TARGET_URL);
        intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        startActivity(intent);
        finish();
    }
}

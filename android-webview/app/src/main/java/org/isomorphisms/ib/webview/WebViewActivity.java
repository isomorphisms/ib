package org.isomorphisms.ib.webview;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.os.Process;
import android.webkit.CookieManager;
import android.webkit.RenderProcessGoneDetail;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebViewRenderProcess;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;

public final class WebViewActivity extends Activity {
    private FixtureServer fixture_server;
    private LinearLayout web_container;
    private TextView status;
    private WebView web_view;
    private Checkpoint checkpoint;
    private int renderer_gone_events;
    private boolean reconstructing;
    private boolean reconstruction_observed;
    private String last_renderer_exit = "none";

    @Override
    protected void onCreate(Bundle saved_instance_state) {
        super.onCreate(saved_instance_state);

        try {
            fixture_server = new FixtureServer();
            fixture_server.start();
        } catch (IOException exception) {
            throw new IllegalStateException("could not start loopback acceptance fixture", exception);
        }

        build_ui();
        attach_webview();
        append_status(device_receipt());
        web_view.loadUrl(fixture_server.start_url());
    }

    @Override
    protected void onDestroy() {
        if (web_view != null) {
            web_container.removeView(web_view);
            web_view.destroy();
            web_view = null;
        }
        if (fixture_server != null) {
            fixture_server.close();
            fixture_server = null;
        }
        super.onDestroy();
    }

    private void build_ui() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);

        TextView heading = new TextView(this);
        heading.setText("IB #59 WebView acceptance");
        heading.setTextSize(20);
        heading.setPadding(dp(12), dp(10), dp(12), dp(4));
        root.addView(heading);

        LinearLayout first_row = controls_row();
        Button seed = button("Seed fields");
        seed.setOnClickListener(view -> seed_test_fields());
        first_row.addView(seed, weighted_button_params());

        Button protect = button("Protect");
        protect.setOnClickListener(view -> protect_current_transaction());
        first_row.addView(protect, weighted_button_params());

        Button pressure = button("Pressure");
        pressure.setOnClickListener(view -> startActivity(new Intent(this, PressureActivity.class)));
        first_row.addView(pressure, weighted_button_params());
        root.addView(first_row);

        LinearLayout second_row = controls_row();
        Button check_live = button("Check live");
        check_live.setOnClickListener(view -> check_live_state());
        second_row.addView(check_live, weighted_button_params());

        Button kill_renderer = button("Kill renderer");
        kill_renderer.setOnClickListener(view -> kill_renderer());
        second_row.addView(kill_renderer, weighted_button_params());

        Button check_recovery = button("Check recovery");
        check_recovery.setOnClickListener(view -> check_recovery_state());
        second_row.addView(check_recovery, weighted_button_params());
        root.addView(second_row);

        status = new TextView(this);
        status.setTextSize(12);
        status.setMinLines(4);
        status.setMaxLines(9);
        status.setTextIsSelectable(true);
        status.setPadding(dp(12), dp(4), dp(12), dp(6));
        root.addView(status);

        web_container = new LinearLayout(this);
        web_container.setOrientation(LinearLayout.VERTICAL);
        root.addView(
            web_container,
            new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                0,
                1.0f
            )
        );

        setContentView(root);
    }

    private void attach_webview() {
        WebView view = new WebView(this);
        view.setSaveEnabled(false);

        WebSettings settings = view.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setAllowFileAccess(false);
        settings.setAllowContentAccess(false);
        settings.setJavaScriptCanOpenWindowsAutomatically(false);
        settings.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);

        CookieManager.getInstance().setAcceptCookie(true);

        view.setWebViewClient(new WebViewClient() {
            @Override
            public void onPageFinished(WebView finished_view, String url) {
                super.onPageFinished(finished_view, url);
                if (reconstructing && checkpoint != null && checkpoint.url.equals(url)) {
                    String script = "(function(){"
                        + "var e=document.getElementById('ordinary_field');"
                        + "if(!e){return 'missing-form';}"
                        + "e.value=" + checkpoint.ordinary_json + ";"
                        + "window.scrollTo(" + checkpoint.scroll_x + "," + checkpoint.scroll_y + ");"
                        + "return 'restored';"
                        + "})()";
                    finished_view.evaluateJavascript(script, result -> {
                        reconstructing = false;
                        reconstruction_observed = "\"restored\"".equals(result);
                        append_status("death.reconstruction=" + (reconstruction_observed ? "observed" : "failed"));
                    });
                }
            }

            @Override
            public boolean onRenderProcessGone(WebView dead_view, RenderProcessGoneDetail detail) {
                renderer_gone_events += 1;
                last_renderer_exit = "didCrash=" + detail.didCrash()
                    + ",priority=" + detail.rendererPriorityAtExit();
                append_status("renderer-gone " + last_renderer_exit);

                if (dead_view == web_view) {
                    web_container.removeView(dead_view);
                    dead_view.destroy();
                    web_view = null;
                    reconstructing = checkpoint != null;
                    attach_webview();
                    web_view.loadUrl(checkpoint != null ? checkpoint.url : fixture_server.start_url());
                }
                return true;
            }
        });

        web_view = view;
        web_container.addView(
            view,
            new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT
            )
        );
    }

    private void seed_test_fields() {
        if (web_view == null) {
            return;
        }
        web_view.evaluateJavascript(
            "(function(){"
                + "var ordinary=document.getElementById('ordinary_field');"
                + "var secret=document.getElementById('secret_field');"
                + "if(!ordinary||!secret){return false;}"
                + "ordinary.value='ordinary-fixture-state';"
                + "secret.value='synthetic-secret-not-for-persistence';"
                + "return true;"
                + "})()",
            result -> append_status("fields.seeded=" + result)
        );
    }

    private void protect_current_transaction() {
        if (web_view == null) {
            return;
        }
        String current_url = web_view.getUrl();
        if (current_url == null) {
            append_status("protected=false no-current-url");
            return;
        }

        web_view.evaluateJavascript(
            "(function(){var e=document.getElementById('ordinary_field');return e?e.value:null;})()",
            ordinary_json -> web_view.evaluateJavascript(
                "window.heap_canary || null",
                heap_json -> {
                    if ("null".equals(ordinary_json) || "null".equals(heap_json)) {
                        append_status("protected=false form-not-ready");
                        return;
                    }
                    checkpoint = new Checkpoint(
                        current_url,
                        web_view.getScrollX(),
                        web_view.getScrollY(),
                        ordinary_json,
                        heap_json
                    );
                    web_view.setRendererPriorityPolicy(WebView.RENDERER_PRIORITY_IMPORTANT, false);
                    append_status("protected=true checkpoint=ordinary+scroll+url;secret=excluded");
                }
            )
        );
    }

    private void check_live_state() {
        if (checkpoint == null) {
            append_status("switch.check=unavailable protect-first");
            return;
        }
        check_session_async(session_survived -> read_page_state((ordinary_json, secret_present, heap_json) -> {
            boolean form_survived = checkpoint.ordinary_json.equals(ordinary_json);
            boolean heap_same = checkpoint.heap_json.equals(heap_json);
            boolean renderer_survived = renderer_gone_events == 0 && heap_same;
            append_status(
                "switch.session=" + word(session_survived, "survived", "lost")
                    + " form=" + word(form_survived, "survived", "changed")
                    + " secret-live=" + word(secret_present, "present", "absent")
                    + " renderer=" + word(renderer_survived, "survived", "not-proven")
                    + " renderer-gone-events=" + renderer_gone_events
            );
        }));
    }

    private void kill_renderer() {
        if (web_view == null) {
            append_status("death.request=unavailable no-webview");
            return;
        }
        if (Build.VERSION.SDK_INT < 29) {
            append_status("death.request=unavailable api<29");
            return;
        }

        WebViewRenderProcess renderer = web_view.getWebViewRenderProcess();
        if (renderer == null) {
            append_status("death.request=unavailable isolated-renderer=false");
            return;
        }

        boolean requested = renderer.terminate();
        append_status("death.request=" + (requested ? "sent" : "refused"));
    }

    private void check_recovery_state() {
        if (checkpoint == null) {
            append_status("death.check=unavailable protect-first");
            return;
        }
        check_session_async(session_survived -> read_page_state((ordinary_json, secret_present, heap_json) -> {
            boolean ordinary_reconstructed = checkpoint.ordinary_json.equals(ordinary_json);
            boolean heap_recreated = !checkpoint.heap_json.equals(heap_json) && !"null".equals(heap_json);
            append_status(
                "death.session=" + word(session_survived, "survived", "lost")
                    + " ordinary-form=" + word(ordinary_reconstructed, "reconstructed", "missing")
                    + " sensitive-form=" + word(!secret_present, "not-restored", "unexpectedly-present")
                    + " heap=" + word(heap_recreated, "recreated", "not-proven")
                    + " reconstruction=" + word(reconstruction_observed, "observed", "not-proven")
                    + " renderer-gone-events=" + renderer_gone_events
                    + " last-exit={" + last_renderer_exit + "}"
            );
        }));
    }

    private void read_page_state(PageStateCallback callback) {
        if (web_view == null) {
            append_status("page-state=unavailable");
            return;
        }
        web_view.evaluateJavascript(
            "(function(){var e=document.getElementById('ordinary_field');return e?e.value:null;})()",
            ordinary_json -> web_view.evaluateJavascript(
                "(function(){var e=document.getElementById('secret_field');return !!(e&&e.value.length>0);})()",
                secret_json -> web_view.evaluateJavascript(
                    "window.heap_canary || null",
                    heap_json -> callback.on_state(
                        ordinary_json,
                        Boolean.parseBoolean(secret_json),
                        heap_json
                    )
                )
            )
        );
    }

    private void check_session_async(BooleanCallback callback) {
        String cookie = CookieManager.getInstance().getCookie(fixture_server.probe_url());
        new Thread(() -> {
            boolean authenticated = false;
            HttpURLConnection connection = null;
            try {
                connection = (HttpURLConnection) new URL(fixture_server.probe_url()).openConnection();
                connection.setConnectTimeout(1500);
                connection.setReadTimeout(1500);
                connection.setInstanceFollowRedirects(false);
                if (cookie != null) {
                    connection.setRequestProperty("Cookie", cookie);
                }
                authenticated = connection.getResponseCode() == 200;
            } catch (IOException ignored) {
                authenticated = false;
            } finally {
                if (connection != null) {
                    connection.disconnect();
                }
            }
            boolean result = authenticated;
            runOnUiThread(() -> callback.on_result(result));
        }, "ib-session-probe").start();
    }

    private String device_receipt() {
        ActivityManager activity_manager = (ActivityManager) getSystemService(ACTIVITY_SERVICE);
        String provider = "unknown";
        if (WebView.getCurrentWebViewPackage() != null) {
            provider = WebView.getCurrentWebViewPackage().packageName
                + ":" + WebView.getCurrentWebViewPackage().versionName;
        }
        return "host-pid=" + Process.myPid()
            + " api=" + Build.VERSION.SDK_INT
            + " low-ram=" + activity_manager.isLowRamDevice()
            + " webview=" + provider;
    }

    private void append_status(String line) {
        CharSequence previous = status.getText();
        String combined = previous.length() == 0 ? line : previous + "\n" + line;
        String[] lines = combined.split("\n");
        int first = Math.max(0, lines.length - 9);
        StringBuilder bounded = new StringBuilder();
        for (int index = first; index < lines.length; index++) {
            if (bounded.length() > 0) {
                bounded.append('\n');
            }
            bounded.append(lines[index]);
        }
        status.setText(bounded.toString());
    }

    private LinearLayout controls_row() {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        return row;
    }

    private Button button(String label) {
        Button button = new Button(this);
        button.setText(label);
        button.setAllCaps(false);
        return button;
    }

    private LinearLayout.LayoutParams weighted_button_params() {
        return new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1.0f);
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private static String word(boolean condition, String yes, String no) {
        return condition ? yes : no;
    }

    private interface BooleanCallback {
        void on_result(boolean value);
    }

    private interface PageStateCallback {
        void on_state(String ordinary_json, boolean secret_present, String heap_json);
    }

    private static final class Checkpoint {
        final String url;
        final int scroll_x;
        final int scroll_y;
        final String ordinary_json;
        final String heap_json;

        Checkpoint(String url, int scroll_x, int scroll_y, String ordinary_json, String heap_json) {
            this.url = url;
            this.scroll_x = scroll_x;
            this.scroll_y = scroll_y;
            this.ordinary_json = ordinary_json;
            this.heap_json = heap_json;
        }
    }
}

package org.isomorphisms.ib.webview;

import android.Manifest;
import android.app.Activity;
import android.app.PictureInPictureParams;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Process;
import android.os.SystemClock;
import android.util.Rational;
import android.view.View;
import android.webkit.CookieManager;
import android.webkit.JavascriptInterface;
import android.webkit.RenderProcessGoneDetail;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.text.InputType;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

public final class IncrementalLargePageActivity extends Activity {
    private static final int NOTIFICATION_PERMISSION_REQUEST = 74;
    private static final String DEFAULT_URL =
        "https://console.cloud.google.com/auth/scopes?project=cockswain&authuser=5";

    private final Handler handler = new Handler(Looper.getMainLooper());
    private final Runnable periodic_sample = new Runnable() {
        @Override
        public void run() {
            sample_page("periodic");
            handler.postDelayed(this, 5000);
        }
    };

    private LinearLayout web_container;
    private TextView heading;
    private LinearLayout address_controls;
    private EditText url_input;
    private LinearLayout controls;
    private LinearLayout receipt_controls;
    private TextView status;
    private WebView web_view;
    private File journal_file;
    private String target_url;
    private long started_at;
    private int last_progress_bucket = -1;
    private boolean form_dirty;
    private boolean destroyed;
    private boolean notification_permission_requested;
    private boolean picture_in_picture_available;

    @Override
    protected void onCreate(Bundle saved_instance_state) {
        super.onCreate(saved_instance_state);

        if (Build.VERSION.SDK_INT >= 28) {
            WebView.setDataDirectorySuffix("incremental-large-page");
        }

        String supplied_url = getIntent().getStringExtra("url");
        target_url = supplied_url == null || supplied_url.trim().isEmpty() ? DEFAULT_URL : supplied_url;
        started_at = SystemClock.elapsedRealtime();
        journal_file = create_journal_file();

        build_ui();
        configure_picture_in_picture();
        record(
            "run",
            "target=" + safe_target_identity(target_url) + " host-pid=" + Process.myPid()
        );
        record(
            "liveness",
            "picture-in-picture-available=" + picture_in_picture_available
        );
        ensure_incremental_service();
        attach_webview();
        web_view.loadUrl(target_url);
        handler.postDelayed(periodic_sample, 5000);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        ensure_incremental_service();
        record("activity", "launcher-reentry existing-page-preserved");
        sample_page("launcher-reentry");
    }

    @Override
    public void onRequestPermissionsResult(
        int request_code,
        String[] permissions,
        int[] grant_results
    ) {
        super.onRequestPermissionsResult(request_code, permissions, grant_results);
        if (request_code != NOTIFICATION_PERMISSION_REQUEST) {
            return;
        }

        boolean granted =
            grant_results.length > 0 && grant_results[0] == PackageManager.PERMISSION_GRANTED;
        record(
            "liveness",
            "notification-permission=" + (granted ? "granted" : "not-granted")
        );
        start_incremental_service();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (status != null) {
            record("activity", "foreground");
        }
    }

    @Override
    protected void onPause() {
        if (status != null) {
            record(
                "activity",
                isInPictureInPictureMode()
                    ? "picture-in-picture-paused"
                    : "hidden"
            );
        }
        // Deliberately do not pause WebView timers here.  Picture-in-picture
        // keeps the document visibly attached while another app is foreground.
        super.onPause();
    }

    @Override
    public void onUserLeaveHint() {
        super.onUserLeaveHint();
        if (Build.VERSION.SDK_INT < 31) {
            enter_visible_background("user-leave");
        }
    }

    @Override
    public void onPictureInPictureModeChanged(
        boolean in_picture_in_picture,
        Configuration new_configuration
    ) {
        super.onPictureInPictureModeChanged(in_picture_in_picture, new_configuration);
        int chrome_visibility = in_picture_in_picture ? View.GONE : View.VISIBLE;
        heading.setVisibility(chrome_visibility);
        address_controls.setVisibility(chrome_visibility);
        controls.setVisibility(chrome_visibility);
        receipt_controls.setVisibility(chrome_visibility);
        status.setVisibility(chrome_visibility);
        record("activity", "picture-in-picture=" + in_picture_in_picture);
    }

    @Override
    protected void onDestroy() {
        destroyed = true;
        handler.removeCallbacks(periodic_sample);
        destroy_webview();
        if (isFinishing()) {
            IncrementalLoadService.stop(this);
        }
        super.onDestroy();
    }

    private void build_ui() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);

        heading = new TextView(this);
        heading.setText("IB incremental large-page render");
        heading.setTextSize(19);
        heading.setPadding(dp(12), dp(10), dp(12), dp(4));
        root.addView(heading);

        address_controls = new LinearLayout(this);
        address_controls.setOrientation(LinearLayout.HORIZONTAL);

        url_input = new EditText(this);
        url_input.setSingleLine(true);
        url_input.setInputType(
            InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_URI
        );
        url_input.setText(target_url);
        url_input.setSelectAllOnFocus(true);
        address_controls.addView(
            url_input,
            new LinearLayout.LayoutParams(
                0,
                LinearLayout.LayoutParams.WRAP_CONTENT,
                1.0f
            )
        );

        Button go = button("Go");
        go.setOnClickListener(view -> navigate_to_entered_url());
        address_controls.addView(
            go,
            new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            )
        );
        root.addView(address_controls);

        controls = new LinearLayout(this);
        controls.setOrientation(LinearLayout.HORIZONTAL);

        Button reload = button("Reload");
        reload.setOnClickListener(view -> explicit_reload());
        controls.addView(reload, weighted_button_params());

        Button background = button("Keep loading");
        background.setOnClickListener(view -> {
            record("user", "visible-background-requested");
            enter_visible_background("button");
        });
        controls.addView(background, weighted_button_params());

        Button sample = button("Sample");
        sample.setOnClickListener(view -> sample_page("manual"));
        controls.addView(sample, weighted_button_params());
        root.addView(controls);

        receipt_controls = new LinearLayout(this);
        receipt_controls.setOrientation(LinearLayout.HORIZONTAL);

        Button copy_receipt = button("Copy receipt");
        copy_receipt.setOnClickListener(view -> copy_journal_to_clipboard());
        receipt_controls.addView(copy_receipt, weighted_button_params());
        root.addView(receipt_controls);

        status = new TextView(this);
        status.setTextSize(11);
        status.setMinLines(5);
        status.setMaxLines(8);
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

    private void ensure_incremental_service() {
        if (
            Build.VERSION.SDK_INT >= 33
                && checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS)
                    != PackageManager.PERMISSION_GRANTED
                && !notification_permission_requested
        ) {
            notification_permission_requested = true;
            record("liveness", "notification-permission-requested");
            requestPermissions(
                new String[] {Manifest.permission.POST_NOTIFICATIONS},
                NOTIFICATION_PERMISSION_REQUEST
            );
            return;
        }
        start_incremental_service();
    }

    private void start_incremental_service() {
        IncrementalLoadService.start(this);
        boolean notification_granted =
            Build.VERSION.SDK_INT < 33
                || checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS)
                    == PackageManager.PERMISSION_GRANTED;
        record(
            "liveness",
            "foreground-service-requested notification-permission="
                + (notification_granted ? "granted" : "not-granted")
        );
    }

    private void configure_picture_in_picture() {
        picture_in_picture_available = getPackageManager().hasSystemFeature(
            PackageManager.FEATURE_PICTURE_IN_PICTURE
        );
        if (!picture_in_picture_available) {
            append_status(
                "Picture-in-Picture is unavailable; hidden WebView survival cannot be guaranteed"
            );
            return;
        }
        setPictureInPictureParams(picture_in_picture_params());
    }

    private PictureInPictureParams picture_in_picture_params() {
        PictureInPictureParams.Builder builder = new PictureInPictureParams.Builder()
            .setAspectRatio(new Rational(1, 1));
        if (Build.VERSION.SDK_INT >= 31) {
            builder.setAutoEnterEnabled(true);
            builder.setSeamlessResizeEnabled(false);
        }
        return builder.build();
    }

    private void enter_visible_background(String reason) {
        if (!picture_in_picture_available || isInPictureInPictureMode()) {
            record(
                "liveness",
                "picture-in-picture-entry reason=" + reason
                    + " available=" + picture_in_picture_available
                    + " already-active=" + isInPictureInPictureMode()
            );
            return;
        }
        boolean entered = enterPictureInPictureMode(picture_in_picture_params());
        record(
            "liveness",
            "picture-in-picture-entry reason=" + reason + " entered=" + entered
        );
    }

    private void attach_webview() {
        if (web_view != null) {
            return;
        }

        WebView view = new WebView(this);
        view.setSaveEnabled(false);
        view.setRendererPriorityPolicy(WebView.RENDERER_PRIORITY_IMPORTANT, false);

        WebSettings settings = view.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setAllowFileAccess(false);
        settings.setAllowContentAccess(false);
        settings.setJavaScriptCanOpenWindowsAutomatically(false);
        settings.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);

        CookieManager cookies = CookieManager.getInstance();
        cookies.setAcceptCookie(true);
        cookies.setAcceptThirdPartyCookies(view, true);

        view.addJavascriptInterface(new PageBridge(), "IBIncremental");
        view.setWebChromeClient(new WebChromeClient() {
            @Override
            public void onProgressChanged(WebView changed_view, int new_progress) {
                super.onProgressChanged(changed_view, new_progress);
                int bucket = Math.min(100, (new_progress / 10) * 10);
                if (bucket != last_progress_bucket) {
                    last_progress_bucket = bucket;
                    record("progress", "percent=" + bucket);
                }
            }
        });
        view.setWebViewClient(new WebViewClient() {
            @Override
            public void onPageStarted(WebView started_view, String url, android.graphics.Bitmap favicon) {
                super.onPageStarted(started_view, url, favicon);
                form_dirty = false;
                last_progress_bucket = -1;
                record("navigation", "started " + safe_target_identity(url));
            }

            @Override
            public void onPageCommitVisible(WebView committed_view, String url) {
                super.onPageCommitVisible(committed_view, url);
                record("paint", "commit-visible " + safe_target_identity(url));
                install_form_dirty_tracker(committed_view);
                sample_page("commit-visible");
            }

            @Override
            public void onPageFinished(WebView finished_view, String url) {
                super.onPageFinished(finished_view, url);
                record("navigation", "page-finished " + safe_target_identity(url));
                install_form_dirty_tracker(finished_view);
                sample_page("page-finished");
            }

            @Override
            public void onReceivedError(
                WebView error_view,
                WebResourceRequest request,
                WebResourceError error
            ) {
                super.onReceivedError(error_view, request, error);
                if (request.isForMainFrame()) {
                    record("error", "main-frame code=" + error.getErrorCode());
                }
            }

            @Override
            public boolean onRenderProcessGone(WebView dead_view, RenderProcessGoneDetail detail) {
                record(
                    "renderer-gone",
                    "didCrash=" + detail.didCrash()
                        + " priority=" + detail.rendererPriorityAtExit()
                        + " form-dirty=" + form_dirty
                        + " reload=requires-user"
                );
                if (dead_view == web_view) {
                    web_container.removeView(dead_view);
                    dead_view.destroy();
                    web_view = null;
                }
                append_status(
                    form_dirty
                        ? "renderer gone; edited form state was not replayed"
                        : "renderer gone; tap Reload to reconstruct explicitly"
                );
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

    private void navigate_to_entered_url() {
        String requested = url_input.getText().toString().trim();
        if (requested.isEmpty()) {
            append_status("enter a URL first");
            return;
        }

        Uri parsed = Uri.parse(requested);
        String scheme = parsed.getScheme();
        if (!"https".equalsIgnoreCase(scheme) && !"http".equalsIgnoreCase(scheme)) {
            append_status("URL must start with http:// or https://");
            return;
        }

        target_url = requested;
        form_dirty = false;
        if (web_view == null) {
            attach_webview();
        }
        record("user", "navigate target=" + safe_target_identity(target_url));
        web_view.loadUrl(target_url);
    }

    private void explicit_reload() {
        record("user", "explicit-reload form-dirty=" + form_dirty);
        form_dirty = false;
        if (web_view == null) {
            attach_webview();
        }
        web_view.loadUrl(target_url);
    }

    private void install_form_dirty_tracker(WebView view) {
        String script = "(function(){"
            + "if(window.__ib_incremental_form_tracker){return 'already';}"
            + "window.__ib_incremental_form_tracker=true;"
            + "var dirty=function(){try{IBIncremental.formDirty();}catch(e){}};"
            + "document.addEventListener('input',dirty,true);"
            + "document.addEventListener('change',dirty,true);"
            + "return 'installed';"
            + "})()";
        view.evaluateJavascript(script, result -> record("forms", "tracker=" + result));
    }

    private void sample_page(String reason) {
        if (destroyed || web_view == null) {
            return;
        }
        String script = "(function(){"
            + "if(!window.__ib_incremental_heap_canary){"
            + "window.__ib_incremental_heap_canary=String(Math.round(performance.timeOrigin))+'-'+String(Math.random()).slice(2);"
            + "}"
            + "var n=performance.getEntriesByType('navigation')[0];"
            + "var r=performance.getEntriesByType('resource');"
            + "var p=performance.getEntriesByType('paint');"
            + "var fcp=-1;"
            + "for(var i=0;i<p.length;i++){if(p[i].name==='first-contentful-paint'){fcp=Math.round(p[i].startTime);}}"
            + "var controls=document.querySelectorAll('input,select,textarea,button').length;"
            + "return ["
            + "'heap='+window.__ib_incremental_heap_canary,"
            + "'ready='+document.readyState,"
            + "'resources='+r.length,"
            + "'domInteractive='+(n?Math.round(n.domInteractive):-1),"
            + "'dcl='+(n?Math.round(n.domContentLoadedEventEnd):-1),"
            + "'load='+(n?Math.round(n.loadEventEnd):-1),"
            + "'transfer='+(n?Math.round(n.transferSize):-1),"
            + "'encoded='+(n?Math.round(n.encodedBodySize):-1),"
            + "'fcp='+fcp,"
            + "'forms='+document.forms.length,"
            + "'controls='+controls"
            + "].join(';');"
            + "})()";
        web_view.evaluateJavascript(
            script,
            result -> record(
                "sample",
                "reason=" + reason + " host-pid=" + Process.myPid() + " " + result
            )
        );
    }

    private void copy_journal_to_clipboard() {
        record("receipt", "copy-request host-pid=" + Process.myPid());
        try {
            String journal = new String(
                java.nio.file.Files.readAllBytes(journal_file.toPath()),
                StandardCharsets.UTF_8
            );
            ClipboardManager clipboard =
                (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
            clipboard.setPrimaryClip(
                ClipData.newPlainText("IB incremental large-page receipt", journal)
            );
            append_status("receipt copied; paste it into chat");
        } catch (IOException exception) {
            append_status("receipt copy failed: " + exception.getClass().getSimpleName());
        }
    }

    private File create_journal_file() {
        File root = new File(getFilesDir(), "incremental-large-page");
        File run = new File(root, Long.toString(System.currentTimeMillis()));
        if (!run.mkdirs() && !run.isDirectory()) {
            throw new IllegalStateException("could not create incremental page run directory");
        }
        return new File(run, "progress.tsv");
    }

    private void record(String event, String detail) {
        long elapsed = SystemClock.elapsedRealtime() - started_at;
        String clean_event = clean(event);
        String clean_detail = clean(detail);
        String line = elapsed + "\t" + clean_event + "\t" + clean_detail + "\n";
        try (FileOutputStream output = new FileOutputStream(journal_file, true)) {
            output.write(line.getBytes(StandardCharsets.UTF_8));
        } catch (IOException exception) {
            append_status("journal write failed: " + exception.getClass().getSimpleName());
            return;
        }
        append_status(elapsed + "ms " + clean_event + " " + clean_detail);
    }

    private void append_status(String line) {
        if (status == null) {
            return;
        }
        CharSequence previous = status.getText();
        String combined = previous.length() == 0 ? line : previous + "\n" + line;
        String[] lines = combined.split("\n");
        int first = Math.max(0, lines.length - 8);
        StringBuilder bounded = new StringBuilder();
        for (int index = first; index < lines.length; index++) {
            if (bounded.length() > 0) {
                bounded.append('\n');
            }
            bounded.append(lines[index]);
        }
        status.setText(bounded.toString());
    }

    private void destroy_webview() {
        if (web_view == null) {
            return;
        }
        web_container.removeView(web_view);
        web_view.destroy();
        web_view = null;
    }

    private static String safe_target_identity(String url) {
        if (url == null) {
            return "unknown";
        }
        Uri parsed = Uri.parse(url);
        String scheme = parsed.getScheme();
        String host = parsed.getHost();
        String path = parsed.getEncodedPath();
        if (scheme == null || host == null) {
            return "opaque";
        }
        return scheme + "://" + host + (path == null ? "" : path);
    }

    private static String clean(String text) {
        if (text == null) {
            return "null";
        }
        return text.replace('\t', ' ').replace('\n', ' ').replace('\r', ' ');
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

    private final class PageBridge {
        @JavascriptInterface
        public void formDirty() {
            runOnUiThread(() -> {
                if (!form_dirty) {
                    form_dirty = true;
                    record("forms", "dirty=true values=not-read");
                }
            });
        }
    }
}

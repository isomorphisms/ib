package org.isomorphisms.ib.webview;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.webkit.CookieManager;
import android.webkit.JavascriptInterface;
import android.webkit.RenderProcessGoneDetail;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

public final class IncrementalLargePageActivity extends Activity {
    private static final String DEFAULT_URL =
        "https://console.cloud.google.com/agent-platform/studio/multimodal"
            + "?project=isomorphismes-youtube-shorts"
            + "&supportedpurview=project"
            + "&model=gemini-3.7-flash"
            + "&region=global";

    private final Handler handler = new Handler(Looper.getMainLooper());
    private final Runnable periodic_sample = new Runnable() {
        @Override
        public void run() {
            sample_page("periodic");
            handler.postDelayed(this, 5000);
        }
    };

    private LinearLayout web_container;
    private TextView status;
    private WebView web_view;
    private File journal_file;
    private String target_url;
    private long started_at;
    private int last_progress_bucket = -1;
    private boolean form_dirty;
    private boolean destroyed;

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
        record("run", "target=" + safe_target_identity(target_url));
        IncrementalLoadService.start(this);
        record("liveness", "foreground-service-requested");
        attach_webview();
        web_view.loadUrl(target_url);
        handler.postDelayed(periodic_sample, 5000);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        IncrementalLoadService.start(this);
        record("activity", "launcher-reentry existing-page-preserved");
        sample_page("launcher-reentry");
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
            record("activity", "background");
        }
        // Deliberately do not pause WebView timers here.  This experiment asks
        // whether a slow page can keep making progress while its task is not in
        // the foreground.  Android may still throttle or kill the process.
        super.onPause();
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

        TextView heading = new TextView(this);
        heading.setText("IB incremental large-page render");
        heading.setTextSize(19);
        heading.setPadding(dp(12), dp(10), dp(12), dp(4));
        root.addView(heading);

        LinearLayout controls = new LinearLayout(this);
        controls.setOrientation(LinearLayout.HORIZONTAL);

        Button reload = button("Reload");
        reload.setOnClickListener(view -> explicit_reload());
        controls.addView(reload, weighted_button_params());

        Button background = button("Background");
        background.setOnClickListener(view -> {
            record("user", "background-requested");
            moveTaskToBack(true);
        });
        controls.addView(background, weighted_button_params());

        Button sample = button("Sample");
        sample.setOnClickListener(view -> sample_page("manual"));
        controls.addView(sample, weighted_button_params());
        root.addView(controls);

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
            result -> record("sample", "reason=" + reason + " " + result)
        );
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

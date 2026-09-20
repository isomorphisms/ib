package org.isomorphisms.ib.webview;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

import org.junit.Test;

public final class IncrementalBackgroundContractTest {
    @Test
    public void production_sources_preserve_page_and_keep_process_active() throws IOException {
        verify(
            read("src/main/AndroidManifest.xml"),
            read("src/main/java/org/isomorphisms/ib/webview/IncrementalLargePageActivity.java"),
            read("src/main/java/org/isomorphisms/ib/webview/IncrementalLargePageLauncherActivity.java"),
            read("src/main/java/org/isomorphisms/ib/webview/IncrementalLoadService.java")
        );
    }

    @Test
    public void rejects_launcher_that_can_create_a_second_page_activity() {
        expect_rejection(
            minimal_manifest().replace(" android:launchMode=\"singleTask\"", ""),
            safe_activity(),
            safe_launcher(),
            safe_service(),
            "incremental page activity must be singleTask"
        );
    }

    @Test
    public void rejects_launcher_reentry_that_reloads_the_page() {
        expect_rejection(
            minimal_manifest(),
            safe_activity().replace(
                "sample_page(\"launcher-reentry\");",
                "web_view.loadUrl(target_url);"
            ),
            safe_launcher(),
            safe_service(),
            "launcher re-entry must preserve the existing page"
        );
    }

    @Test
    public void rejects_service_that_never_enters_the_foreground() {
        expect_rejection(
            minimal_manifest(),
            safe_activity(),
            safe_launcher(),
            safe_service().replace("startForeground(", "publishNotification("),
            "incremental load service must enter the foreground"
        );
    }

    private static void verify(
        String manifest,
        String activity,
        String launcher,
        String service
    ) {
        require(
            manifest.contains("android:launchMode=\"singleTask\""),
            "incremental page activity must be singleTask"
        );
        require(
            manifest.contains("android:name=\".IncrementalLoadService\"")
                && manifest.contains("android:foregroundServiceType=\"dataSync\"")
                && count(manifest, "android:process=\":incremental\"") >= 2,
            "activity and foreground service must share the incremental process"
        );
        require(
            launcher.contains("FLAG_ACTIVITY_REORDER_TO_FRONT")
                && launcher.contains("FLAG_ACTIVITY_SINGLE_TOP"),
            "launcher must bring the existing incremental activity forward"
        );

        String reentry = method(activity, "protected void onNewIntent", "protected void onResume");
        require(
            !reentry.contains("loadUrl(")
                && reentry.contains("sample_page(\"launcher-reentry\")"),
            "launcher re-entry must preserve the existing page"
        );
        require(
            activity.contains("IncrementalLoadService.start(this)")
                && activity.contains(
                    "setRendererPriorityPolicy(WebView.RENDERER_PRIORITY_IMPORTANT, false)"
                )
                && activity.contains("__ib_incremental_heap_canary"),
            "incremental page must protect both host and renderer processes"
        );
        require(
            service.contains("startForeground(")
                && service.contains("return START_NOT_STICKY"),
            "incremental load service must enter the foreground"
        );
    }

    private static String read(String relative_path) throws IOException {
        Path direct = Path.of(relative_path);
        Path resolved = Files.exists(direct) ? direct : Path.of("app").resolve(relative_path);
        return Files.readString(resolved, StandardCharsets.UTF_8);
    }

    private static String method(String source, String start_marker, String end_marker) {
        int start = source.indexOf(start_marker);
        int end = source.indexOf(end_marker, start + start_marker.length());
        require(start >= 0 && end > start, "required lifecycle method is missing");
        return source.substring(start, end);
    }

    private static int count(String text, String wanted) {
        int found = 0;
        int offset = 0;
        while ((offset = text.indexOf(wanted, offset)) >= 0) {
            found++;
            offset += wanted.length();
        }
        return found;
    }

    private static void require(boolean condition, String diagnostic) {
        if (!condition) {
            throw new AssertionError(diagnostic);
        }
    }

    private static void expect_rejection(
        String manifest,
        String activity,
        String launcher,
        String service,
        String expected_diagnostic
    ) {
        try {
            verify(manifest, activity, launcher, service);
            fail("known-bad fixture passed");
        } catch (AssertionError error) {
            assertEquals(expected_diagnostic, error.getMessage());
        }
    }

    private static String minimal_manifest() {
        return "<activity android:name=\".IncrementalLargePageActivity\""
            + " android:launchMode=\"singleTask\" android:process=\":incremental\" />"
            + "<service android:name=\".IncrementalLoadService\""
            + " android:foregroundServiceType=\"dataSync\""
            + " android:process=\":incremental\" />";
    }

    private static String safe_activity() {
        return "void onCreate(){IncrementalLoadService.start(this);"
            + "view.setRendererPriorityPolicy(WebView.RENDERER_PRIORITY_IMPORTANT, false);"
            + "String marker=\"__ib_incremental_heap_canary\";}"
            + " protected void onNewIntent(){sample_page(\"launcher-reentry\");}"
            + " protected void onResume(){}";
    }

    private static String safe_launcher() {
        return "FLAG_ACTIVITY_REORDER_TO_FRONT | FLAG_ACTIVITY_SINGLE_TOP";
    }

    private static String safe_service() {
        return "void start(){startForeground(1, notification()); return START_NOT_STICKY;}";
    }
}

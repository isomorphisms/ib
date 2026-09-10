import org.isomorphisms.ib.material3.ViewExposure;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Arrays;
import java.util.List;

public final class ViewExposureTest {
    private static int assertions = 0;

    private static void check(boolean condition, String message) {
        assertions++;
        if (!condition) {
            throw new AssertionError(message);
        }
    }

    private static ViewExposure.VisibleFragment visible(
            String id, int top, int visiblePixels, int fragmentPixels) {
        return new ViewExposure.VisibleFragment(id, top, visiblePixels, fragmentPixels);
    }

    private static ViewExposure.ViewportSnapshot snapshot(
            int firstIndex, int firstOffset, int height, ViewExposure.VisibleFragment... fragments) {
        return new ViewExposure.ViewportSnapshot(
                Arrays.asList(fragments), firstIndex, firstOffset, height);
    }

    private static ViewExposure.Summary stationary(File root, long durationMs) throws Exception {
        ViewExposure.Store store = new ViewExposure.Store(root);
        ViewExposure.Session session = new ViewExposure.Session();
        String id = "stationary";
        ViewExposure.ViewportSnapshot full = snapshot(0, 0, 100, visible(id, 0, 100, 100));
        store.record(session.observe(1_000L, 0L, full));
        store.record(session.observe(1_000L + durationMs, durationMs, full));
        store.flush();
        return store.summary(id);
    }

    private static void testStationary(File root) throws Exception {
        ViewExposure.Summary six = stationary(new File(root, "six"), 6_000L);
        check(six.viewUnits == 0L, "6 seconds must be 0 view units");
        check(six.weightedExposureUs == 6_000_000L, "6 seconds exposure mismatch");

        ViewExposure.Summary seven = stationary(new File(root, "seven"), 7_000L);
        check(seven.viewUnits == 1L, "7 seconds must be 1 view unit");
        check(seven.remainderUs == 0L, "7 seconds remainder must be zero");

        ViewExposure.Summary fourteen = stationary(new File(root, "fourteen"), 14_000L);
        check(fourteen.viewUnits == 2L, "14 seconds must be 2 view units");

        ViewExposure.Summary eighteen = stationary(new File(root, "eighteen"), 18_000L);
        check(eighteen.viewUnits == 2L, "18 seconds must be 2 view units");
        check(eighteen.remainderUs == 4_000_000L, "18 seconds must retain 4 seconds");
    }

    private static void testSmallScrolling(File root) throws Exception {
        ViewExposure.Store store = new ViewExposure.Store(root);
        ViewExposure.Session session = new ViewExposure.Session();
        String id = "small-scroll";
        store.record(session.observe(1_000L, 0L,
                snapshot(0, 0, 100, visible(id, 0, 100, 100))));
        store.record(session.observe(2_000L, 1_000L,
                snapshot(0, 10, 100, visible(id, -10, 90, 100))));
        store.record(session.observe(3_000L, 2_000L,
                snapshot(0, 20, 100, visible(id, -20, 80, 100))));
        store.flush();
        ViewExposure.Summary summary = store.summary(id);
        check(summary.weightedExposureUs > 1_600_000L,
                "small scroll exposure must accumulate instead of restart");
        check(summary.weightedExposureUs < 2_000_000L,
                "visibility taper should reduce small-scroll exposure");
    }

    private static void testPartialAndShared(File root) throws Exception {
        ViewExposure.Store store = new ViewExposure.Store(root);
        ViewExposure.Session session = new ViewExposure.Session();
        ViewExposure.ViewportSnapshot view = snapshot(0, 0, 200,
                visible("full", 0, 100, 100),
                visible("partial", 150, 50, 100),
                visible("shared", 100, 100, 100));
        store.record(session.observe(1_000L, 0L, view));
        store.record(session.observe(11_000L, 10_000L, view));
        store.flush();
        long full = store.summary("full").weightedExposureUs;
        long partial = store.summary("partial").weightedExposureUs;
        long shared = store.summary("shared").weightedExposureUs;
        check(partial < full, "partial visibility must accumulate less than full visibility");
        check(full == 10_000_000L, "full fragment should receive full interval");
        check(shared == 10_000_000L, "two fragments may accumulate in the same interval");
    }

    private static void testEnteringLeavingAndFastScroll(File root) throws Exception {
        ViewExposure.Store store = new ViewExposure.Store(root);
        ViewExposure.Session session = new ViewExposure.Session();
        store.record(session.observe(1_000L, 0L,
                snapshot(0, 0, 100, visible("anchor", 0, 100, 100))));
        store.record(session.observe(2_000L, 1_000L,
                snapshot(0, 10, 100,
                        visible("anchor", -10, 90, 100),
                        visible("enter", 90, 10, 100))));
        store.record(session.observe(3_000L, 2_000L,
                snapshot(0, 20, 100,
                        visible("anchor", -20, 80, 100),
                        visible("enter", 80, 20, 100))));
        store.record(session.observe(4_000L, 3_000L,
                snapshot(1, 0, 100, visible("other", 0, 100, 100))));
        store.record(session.observe(5_000L, 4_000L,
                snapshot(1, 0, 100, visible("other", 0, 100, 100))));
        store.flush();
        long enter = store.summary("enter").weightedExposureUs;
        check(enter > 0L, "entering fragment must begin accumulating exposure");
        check(enter < 500_000L, "fast departure must suppress transient passage");

        ViewExposure.Store fastStore = new ViewExposure.Store(new File(root, "fast"));
        ViewExposure.Session fast = new ViewExposure.Session();
        fastStore.record(fast.observe(1_000L, 0L,
                snapshot(0, 0, 100, visible("flyby", 0, 100, 100))));
        fastStore.record(fast.observe(2_000L, 1_000L,
                snapshot(10, 0, 100, visible("elsewhere", 0, 100, 100))));
        fastStore.flush();
        check(fastStore.summary("flyby").weightedExposureUs == 0L,
                "fast scroll must not receive stationary exposure");
    }

    private static void testReturnAndRestart(File root) throws Exception {
        ViewExposure.Store store = new ViewExposure.Store(root);
        ViewExposure.Session first = new ViewExposure.Session();
        ViewExposure.ViewportSnapshot a = snapshot(0, 0, 100, visible("return", 0, 100, 100));
        store.record(first.observe(1_000L, 0L, a));
        store.record(first.observe(5_000L, 4_000L, a));
        store.flush();

        ViewExposure.Session away = new ViewExposure.Session();
        ViewExposure.ViewportSnapshot b = snapshot(0, 0, 100, visible("other", 0, 100, 100));
        store.record(away.observe(6_000L, 5_000L, b));
        store.record(away.observe(10_000L, 9_000L, b));
        store.flush();

        ViewExposure.Session returned = new ViewExposure.Session();
        store.record(returned.observe(11_000L, 10_000L, a));
        store.record(returned.observe(15_000L, 14_000L, a));
        store.flush();
        ViewExposure.Summary cumulative = store.summary("return");
        check(cumulative.viewUnits == 1L, "return visit must continue cumulative exposure");
        check(cumulative.remainderUs == 1_000_000L, "return visit remainder mismatch");

        ViewExposure.Store restarted = new ViewExposure.Store(root);
        ViewExposure.Summary afterRestart = restarted.summary("return");
        check(afterRestart.weightedExposureUs == cumulative.weightedExposureUs,
                "exposure must survive store reinitialization");
    }

    private static String exerciseRealPensieve(File asset, File ibHome) throws Exception {
        String body = Files.readString(asset.toPath(), StandardCharsets.UTF_8).trim();
        List<ViewExposure.Fragment> fragments = ViewExposure.fragmentsForArticle("2203.11355", body);
        check(fragments.size() >= 3, "real Pensieve fixture must produce multiple fragments");

        File history = new File(ibHome, "pensieve/view-history");
        ViewExposure.Store store = new ViewExposure.Store(history);
        ViewExposure.Session session = new ViewExposure.Session();
        ViewExposure.Fragment f0 = fragments.get(0);
        ViewExposure.Fragment f1 = fragments.get(1);
        ViewExposure.Fragment f2 = fragments.get(2);

        store.record(session.observe(10_000L, 0L,
                snapshot(0, 0, 300,
                        visible(f0.id, 0, 100, 100),
                        visible(f1.id, 100, 100, 100),
                        visible(f2.id, 200, 100, 100))));
        store.record(session.observe(12_500L, 2_500L,
                snapshot(0, 20, 300,
                        visible(f0.id, -20, 80, 100),
                        visible(f1.id, 80, 100, 100),
                        visible(f2.id, 180, 100, 100))));
        store.record(session.observe(15_000L, 5_000L,
                snapshot(0, 40, 300,
                        visible(f0.id, -40, 60, 100),
                        visible(f1.id, 60, 100, 100),
                        visible(f2.id, 160, 100, 100))));
        store.record(session.observe(18_000L, 8_000L,
                snapshot(0, 40, 300,
                        visible(f0.id, -40, 60, 100),
                        visible(f1.id, 60, 100, 100),
                        visible(f2.id, 160, 100, 100))));
        store.flush();

        ViewExposure.Store restarted = new ViewExposure.Store(history);
        ViewExposure.Summary summary = restarted.summary(f1.id);
        check(summary.viewUnits == 1L, "real fragment must cross seven-second threshold");
        check(summary.remainderUs == 1_000_000L, "real fragment must retain one-second remainder");

        System.out.println("REAL_FRAGMENT_0=" + f0.id);
        System.out.println("REAL_FRAGMENT_1=" + f1.id);
        System.out.println("REAL_FRAGMENT_2=" + f2.id);
        System.out.println("REAL_EXPOSURE_US=" + summary.weightedExposureUs);
        System.out.println("REAL_VIEW_UNITS=" + summary.viewUnits);
        System.out.println("REAL_REMAINDER_US=" + summary.remainderUs);
        return f1.id;
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 2) {
            throw new IllegalArgumentException("usage: ViewExposureTest ASSET IB_HOME");
        }
        File scratch = new File(args[1], "test-scratch");
        testStationary(new File(scratch, "stationary"));
        testSmallScrolling(new File(scratch, "small-scroll"));
        testPartialAndShared(new File(scratch, "partial-shared"));
        testEnteringLeavingAndFastScroll(new File(scratch, "enter-leave"));
        testReturnAndRestart(new File(scratch, "return-restart"));
        exerciseRealPensieve(new File(args[0]), new File(args[1]));
        System.out.println("view exposure tests: PASS (" + assertions + " assertions)");
    }
}

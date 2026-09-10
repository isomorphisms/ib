package org.isomorphisms.ib.material3;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Reader-facing fragment exposure accounting.
 *
 * This records what the viewport presented. It deliberately does not claim that a
 * person looked at, read, understood, or attended to the fragment.
 */
public final class ViewExposure {
    public static final long VIEW_UNIT_US = 7_000_000L;
    public static final int ONE_WEIGHT_PPM = 1_000_000;

    private ViewExposure() {}

    public static final class Fragment {
        public final String id;
        public final int sourceStart;
        public final int sourceEnd;
        public final String text;

        Fragment(String id, int sourceStart, int sourceEnd, String text) {
            this.id = id;
            this.sourceStart = sourceStart;
            this.sourceEnd = sourceEnd;
            this.text = text;
        }
    }

    public static final class VisibleFragment {
        public final String fragmentId;
        public final int topOffsetPx;
        public final int visiblePixels;
        public final int fragmentPixels;

        public VisibleFragment(
                String fragmentId,
                int topOffsetPx,
                int visiblePixels,
                int fragmentPixels) {
            this.fragmentId = fragmentId;
            this.topOffsetPx = topOffsetPx;
            this.visiblePixels = Math.max(0, visiblePixels);
            this.fragmentPixels = Math.max(1, fragmentPixels);
        }
    }

    public static final class ViewportSnapshot {
        public final List<VisibleFragment> visibleFragments;
        public final int firstVisibleItem;
        public final int firstVisibleOffsetPx;
        public final int viewportHeightPx;

        public ViewportSnapshot(
                List<VisibleFragment> visibleFragments,
                int firstVisibleItem,
                int firstVisibleOffsetPx,
                int viewportHeightPx) {
            this.visibleFragments = Collections.unmodifiableList(new ArrayList<>(visibleFragments));
            this.firstVisibleItem = firstVisibleItem;
            this.firstVisibleOffsetPx = firstVisibleOffsetPx;
            this.viewportHeightPx = Math.max(1, viewportHeightPx);
        }
    }

    public static final class Observation {
        public final long endEpochMs;
        public final long elapsedMs;
        public final String fragmentId;
        public final int visiblePixels;
        public final int fragmentPixels;
        public final int visibilityPpm;
        public final int stabilityPpm;
        public final long weightedUs;
        public final int firstVisibleItem;
        public final int firstVisibleOffsetPx;
        public final int viewportHeightPx;
        public final int movementPx;

        Observation(
                long endEpochMs,
                long elapsedMs,
                String fragmentId,
                int visiblePixels,
                int fragmentPixels,
                int visibilityPpm,
                int stabilityPpm,
                long weightedUs,
                int firstVisibleItem,
                int firstVisibleOffsetPx,
                int viewportHeightPx,
                int movementPx) {
            this.endEpochMs = endEpochMs;
            this.elapsedMs = elapsedMs;
            this.fragmentId = fragmentId;
            this.visiblePixels = visiblePixels;
            this.fragmentPixels = fragmentPixels;
            this.visibilityPpm = visibilityPpm;
            this.stabilityPpm = stabilityPpm;
            this.weightedUs = weightedUs;
            this.firstVisibleItem = firstVisibleItem;
            this.firstVisibleOffsetPx = firstVisibleOffsetPx;
            this.viewportHeightPx = viewportHeightPx;
            this.movementPx = movementPx;
        }

        String toTsv() {
            return endEpochMs + "\t"
                    + elapsedMs + "\t"
                    + fragmentId + "\t"
                    + visiblePixels + "\t"
                    + fragmentPixels + "\t"
                    + visibilityPpm + "\t"
                    + stabilityPpm + "\t"
                    + weightedUs + "\t"
                    + firstVisibleItem + "\t"
                    + firstVisibleOffsetPx + "\t"
                    + viewportHeightPx + "\t"
                    + movementPx;
        }
    }

    public static final class Summary {
        public final long weightedExposureUs;
        public final long viewUnits;
        public final long remainderUs;

        Summary(long weightedExposureUs) {
            this.weightedExposureUs = Math.max(0L, weightedExposureUs);
            this.viewUnits = this.weightedExposureUs / VIEW_UNIT_US;
            this.remainderUs = this.weightedExposureUs % VIEW_UNIT_US;
        }
    }

    /**
     * Maintains only the previous viewport sample. Small scrolls therefore change
     * weights continuously instead of starting a new visit or resetting a clock.
     */
    public static final class Session {
        private ViewportSnapshot previous;
        private long previousElapsedMs;

        public List<Observation> observe(
                long endEpochMs,
                long elapsedRealtimeMs,
                ViewportSnapshot current) {
            if (previous == null) {
                previous = current;
                previousElapsedMs = elapsedRealtimeMs;
                return Collections.emptyList();
            }

            long elapsedMs = elapsedRealtimeMs - previousElapsedMs;
            if (elapsedMs <= 0L) {
                previous = current;
                previousElapsedMs = elapsedRealtimeMs;
                return Collections.emptyList();
            }

            int movementPx = estimateMovementPx(previous, current);
            int stabilityPpm = stabilityWeightPpm(movementPx, current.viewportHeightPx);

            Map<String, VisibleFragment> previousVisible = byId(previous.visibleFragments);
            Map<String, VisibleFragment> currentVisible = byId(current.visibleFragments);
            Set<String> fragmentIds = new LinkedHashSet<>();
            fragmentIds.addAll(previousVisible.keySet());
            fragmentIds.addAll(currentVisible.keySet());

            List<Observation> observations = new ArrayList<>();
            for (String fragmentId : fragmentIds) {
                VisibleFragment before = previousVisible.get(fragmentId);
                VisibleFragment after = currentVisible.get(fragmentId);
                int beforePpm = visibilityPpm(before, previous.viewportHeightPx);
                int afterPpm = visibilityPpm(after, current.viewportHeightPx);
                int visibilityPpm = (beforePpm + afterPpm) / 2;
                if (visibilityPpm <= 0) {
                    continue;
                }

                VisibleFragment geometry = after != null ? after : before;
                long weightedUs = weightedMicroseconds(elapsedMs, visibilityPpm, stabilityPpm);
                observations.add(new Observation(
                        endEpochMs,
                        elapsedMs,
                        fragmentId,
                        geometry.visiblePixels,
                        geometry.fragmentPixels,
                        visibilityPpm,
                        stabilityPpm,
                        weightedUs,
                        current.firstVisibleItem,
                        current.firstVisibleOffsetPx,
                        current.viewportHeightPx,
                        movementPx));
            }

            previous = current;
            previousElapsedMs = elapsedRealtimeMs;
            return observations;
        }

        public void reset() {
            previous = null;
            previousElapsedMs = 0L;
        }
    }

    /** Append-only local reader history. Writes are batched by the caller. */
    public static final class Store {
        private final File root;
        private final File observationsFile;
        private final List<Observation> pending = new ArrayList<>();

        public Store(File root) {
            this.root = root;
            this.observationsFile = new File(root, "observations.tsv");
        }

        public synchronized void record(List<Observation> observations) {
            pending.addAll(observations);
        }

        public synchronized int pendingCount() {
            return pending.size();
        }

        public synchronized void flush() throws IOException {
            if (pending.isEmpty()) {
                return;
            }
            if (!root.isDirectory() && !root.mkdirs() && !root.isDirectory()) {
                throw new IOException("could not create view-history directory: " + root);
            }

            FileOutputStream output = new FileOutputStream(observationsFile, true);
            try {
                BufferedWriter writer = new BufferedWriter(
                        new OutputStreamWriter(output, StandardCharsets.UTF_8));
                for (Observation observation : pending) {
                    writer.write(observation.toTsv());
                    writer.newLine();
                }
                writer.flush();
                output.getFD().sync();
                pending.clear();
            } finally {
                output.close();
            }
        }

        public synchronized Summary summary(String fragmentId) throws IOException {
            long weightedUs = 0L;
            if (observationsFile.isFile()) {
                BufferedReader reader = new BufferedReader(new InputStreamReader(
                        new FileInputStream(observationsFile), StandardCharsets.UTF_8));
                try {
                    String line;
                    while ((line = reader.readLine()) != null) {
                        String[] fields = line.split("\\t", -1);
                        if (fields.length < 8 || !fragmentId.equals(fields[2])) {
                            continue;
                        }
                        try {
                            weightedUs += Long.parseLong(fields[7]);
                        } catch (NumberFormatException ignored) {
                            // A hard crash may leave one torn tail record. Earlier synced rows remain useful.
                        }
                    }
                } finally {
                    reader.close();
                }
            }
            for (Observation observation : pending) {
                if (fragmentId.equals(observation.fragmentId)) {
                    weightedUs += observation.weightedUs;
                }
            }
            return new Summary(weightedUs);
        }

        public File observationsFile() {
            return observationsFile;
        }
    }

    /**
     * The first reader adapter treats each non-empty displayed text line as one
     * fragment. Identity is source-range based and includes a digest of the arXiv
     * identity plus displayed representation bytes, not a filesystem pathname.
     */
    public static List<Fragment> fragmentsForArticle(String arxivId, String body) {
        if (body == null || body.isEmpty()) {
            return Collections.emptyList();
        }

        String sourceDigest = sha256Hex("arxiv:" + arxivId + "\n" + body).substring(0, 24);
        List<Fragment> fragments = new ArrayList<>();
        int cursor = 0;
        while (cursor <= body.length()) {
            int newline = body.indexOf('\n', cursor);
            int rawEnd = newline >= 0 ? newline : body.length();
            int start = cursor;
            int end = rawEnd;
            while (start < end && Character.isWhitespace(body.charAt(start))) {
                start++;
            }
            while (end > start && Character.isWhitespace(body.charAt(end - 1))) {
                end--;
            }
            if (start < end) {
                String id = "frag-" + sourceDigest + "-" + start + "-" + end;
                fragments.add(new Fragment(id, start, end, body.substring(start, end)));
            }
            if (newline < 0) {
                break;
            }
            cursor = newline + 1;
        }
        return Collections.unmodifiableList(fragments);
    }

    static int visibilityPpm(VisibleFragment fragment, int viewportHeightPx) {
        if (fragment == null || fragment.visiblePixels <= 0) {
            return 0;
        }
        int usefulExtent = Math.max(1, Math.min(fragment.fragmentPixels, Math.max(1, viewportHeightPx)));
        double fraction = Math.min(1.0, fragment.visiblePixels / (double) usefulExtent);
        return (int) Math.round(fraction * ONE_WEIGHT_PPM);
    }

    public static int stabilityWeightPpm(int movementPx, int viewportHeightPx) {
        double ratio = Math.abs((double) movementPx) / Math.max(1, viewportHeightPx);
        if (ratio >= 0.75) {
            return 0;
        }
        if (ratio >= 0.40) {
            return 250_000;
        }
        if (ratio >= 0.15) {
            return 700_000;
        }
        return ONE_WEIGHT_PPM;
    }

    static int estimateMovementPx(ViewportSnapshot before, ViewportSnapshot after) {
        Map<String, VisibleFragment> previousVisible = byId(before.visibleFragments);
        for (VisibleFragment current : after.visibleFragments) {
            VisibleFragment previous = previousVisible.get(current.fragmentId);
            if (previous != null) {
                return Math.abs(current.topOffsetPx - previous.topOffsetPx);
            }
        }
        if (!before.visibleFragments.isEmpty() && !after.visibleFragments.isEmpty()) {
            return Math.max(before.viewportHeightPx, after.viewportHeightPx);
        }
        return 0;
    }

    static long weightedMicroseconds(long elapsedMs, int visibilityPpm, int stabilityPpm) {
        double weighted = elapsedMs * 1000.0
                * (visibilityPpm / (double) ONE_WEIGHT_PPM)
                * (stabilityPpm / (double) ONE_WEIGHT_PPM);
        return Math.max(0L, Math.round(weighted));
    }

    private static Map<String, VisibleFragment> byId(List<VisibleFragment> visibleFragments) {
        Map<String, VisibleFragment> result = new LinkedHashMap<>();
        for (VisibleFragment fragment : visibleFragments) {
            result.put(fragment.fragmentId, fragment);
        }
        return result;
    }

    private static String sha256Hex(String value) {
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            byte[] bytes = digest.digest(value.getBytes(StandardCharsets.UTF_8));
            StringBuilder hex = new StringBuilder(bytes.length * 2);
            for (byte valueByte : bytes) {
                hex.append(String.format("%02x", valueByte & 0xff));
            }
            return hex.toString();
        } catch (NoSuchAlgorithmException error) {
            throw new IllegalStateException("SHA-256 unavailable", error);
        }
    }
}

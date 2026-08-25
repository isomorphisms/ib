package org.isomorphisms.ib.prepaint;

import java.io.IOException;
import java.io.StringReader;
import java.nio.charset.StandardCharsets;

final class TermuxPrepaintResult {
    static final int MAX_ARTIFACT_BYTES = 64 * 1024;
    static final int TERMUX_NO_INTERNAL_ERROR = -1;

    final PrepaintDocument document;
    final String error;

    private TermuxPrepaintResult(PrepaintDocument document, String error) {
        this.document = document;
        this.error = error;
    }

    static TermuxPrepaintResult validate(String requestedUrl, String stdout,
            String stdoutOriginalLength, String stderr, String stderrOriginalLength,
            int exitCode, int internalError, String internalMessage) {
        if (!UrlRecognition.isAbsoluteHttpUrl(requestedUrl)) {
            return failure("The active IB URL is invalid.");
        }
        if (internalError != TERMUX_NO_INTERNAL_ERROR) {
            return failure(visibleError("Termux could not run the IB command", internalMessage));
        }
        if (stdout == null || stdoutOriginalLength == null) {
            return failure("Termux returned no verifiable prepaint output.");
        }
        if (!lengthMatches(stdout, stdoutOriginalLength)) {
            return failure("Termux truncated the prepaint output.");
        }
        String visibleStderr = stderr == null ? "" : stderr;
        if (stderrOriginalLength == null || !lengthMatches(visibleStderr, stderrOriginalLength)) {
            return failure("Termux truncated the prepaint error output.");
        }
        if (exitCode != 0) {
            return failure(visibleError(
                    "IB prepaint exited with status " + exitCode, visibleStderr));
        }
        if (!visibleStderr.isEmpty()) {
            return failure(visibleError("IB prepaint reported an error", visibleStderr));
        }
        if (stdout.indexOf('\0') >= 0) {
            return failure("IB prepaint returned binary data.");
        }
        if (stdout.getBytes(StandardCharsets.UTF_8).length > MAX_ARTIFACT_BYTES) {
            return failure("IB prepaint exceeded the 64 KiB result budget.");
        }

        final PrepaintDocument parsed;
        try {
            parsed = PrepaintDocument.parse(new StringReader(stdout));
        } catch (IOException error) {
            return failure(error.getMessage());
        }
        if (!PrepaintDocument.ARTIFACT_SOURCE.equals(parsed.sourceKind)) {
            return failure("Termux did not return a structured prepaint artifact.");
        }
        PrepaintDocument.Revision finalRevision =
                parsed.revisions.get(parsed.revisions.size() - 1);
        if (!finalRevision.complete) {
            return failure("IB prepaint returned no complete final revision.");
        }
        for (PrepaintDocument.Revision revision : parsed.revisions) {
            if (!requestedUrl.equals(revision.requestedUrl)) {
                return failure("IB prepaint returned a different requested URL.");
            }
        }
        return new TermuxPrepaintResult(parsed, null);
    }

    boolean accepted() {
        return document != null;
    }

    private static boolean lengthMatches(String value, String claimedLength) {
        try {
            return value.length() == Long.parseLong(claimedLength);
        } catch (NumberFormatException ignored) {
            return false;
        }
    }

    private static String visibleError(String prefix, String detail) {
        if (detail == null || detail.trim().isEmpty()) {
            return prefix + ".";
        }
        String oneLine = detail.trim().replace('\n', ' ').replace('\r', ' ');
        if (oneLine.length() > 240) {
            oneLine = oneLine.substring(0, 240);
        }
        return prefix + ": " + oneLine;
    }

    private static TermuxPrepaintResult failure(String error) {
        String visible = error == null || error.trim().isEmpty()
                ? "IB prepaint returned an invalid artifact." : error;
        return new TermuxPrepaintResult(null, visible);
    }
}

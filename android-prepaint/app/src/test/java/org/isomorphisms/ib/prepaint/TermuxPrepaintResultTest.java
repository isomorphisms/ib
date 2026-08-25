package org.isomorphisms.ib.prepaint;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class TermuxPrepaintResultTest {
    private static final String URL = "https://example.test/page?q=one";

    @Test
    public void acceptsOneCompleteMatchingArtifact() {
        String artifact = artifact(URL, "complete");
        TermuxPrepaintResult result = validate(artifact,
                Integer.toString(artifact.length()), "", "0", 0, -1);

        assertTrue(result.accepted());
        assertEquals("Visible result", result.document.revisions.get(0).title);
    }

    @Test
    public void rejectsTruncatedBinderOutput() {
        String artifact = artifact(URL, "complete");
        TermuxPrepaintResult result = validate(artifact,
                Integer.toString(artifact.length() + 20), "", "0", 0, -1);

        assertFalse(result.accepted());
        assertEquals("Termux truncated the prepaint output.", result.error);
    }

    @Test
    public void rejectsFailedAndMalformedResults() {
        TermuxPrepaintResult failed =
                validate("", "0", "fetch failed", "12", 7, -1);
        assertFalse(failed.accepted());
        assertTrue(failed.error.contains("fetch failed"));
        assertFalse(validate("plain text", "10", "", "0", 0, -1).accepted());
    }

    @Test
    public void rejectsAStaleUrlOrIncompleteFinalRevision() {
        assertFalse(validate(artifact("https://other.test/", "complete"),
                null, "", "0", 0, -1).accepted());
        assertFalse(validate(artifact(URL, "partial"),
                null, "", "0", 0, -1).accepted());
    }

    @Test
    public void rejectsAnInvalidActiveUrl() {
        String artifact = artifact(URL, "complete");
        assertFalse(TermuxPrepaintResult.validate(null, artifact,
                Integer.toString(artifact.length()), "", "0", 0, -1, null).accepted());
    }

    @Test
    public void rejectsMoreThanSixtyFourKibibytes() {
        StringBuilder large = new StringBuilder(70_000);
        large.append("ib-prepaint\t1\nrevision\t1\tcomplete\nrequested-url\t")
                .append(URL).append("\ntext\t");
        while (large.length() < 66_000) {
            large.append('x');
        }
        large.append("\nend\n");
        String artifact = large.toString();

        assertFalse(validate(artifact, Integer.toString(artifact.length()),
                "", "0", 0, -1).accepted());
    }

    private static TermuxPrepaintResult validate(String stdout,
            String stdoutLength, String stderr, String stderrLength,
            int exitCode, int internalError) {
        String claimedStdout = stdoutLength == null
                ? Integer.toString(stdout.length()) : stdoutLength;
        return TermuxPrepaintResult.validate(URL, stdout, claimedStdout,
                stderr, stderrLength, exitCode, internalError, null);
    }

    private static String artifact(String requestedUrl, String state) {
        return "ib-prepaint\t1\n"
                + "revision\t1\t" + state + "\n"
                + "requested-url\t" + requestedUrl + "\n"
                + "resolved-url\t" + requestedUrl + "\n"
                + "title\tVisible result\n"
                + "text\tFetched through ICU.\n"
                + "end\n";
    }
}

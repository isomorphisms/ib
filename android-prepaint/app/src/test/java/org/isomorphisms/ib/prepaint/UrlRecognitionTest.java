package org.isomorphisms.ib.prepaint;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class UrlRecognitionTest {
    @Test
    public void recognizesOnlyAbsoluteVisibleHttpUrls() {
        assertTrue(UrlRecognition.isAbsoluteHttpUrl("https://example.com/a?q=b#c"));
        assertTrue(UrlRecognition.isAbsoluteHttpUrl("http://127.0.0.1:8080/"));
        assertFalse(UrlRecognition.isAbsoluteHttpUrl("/relative/path"));
        assertFalse(UrlRecognition.isAbsoluteHttpUrl("ftp://example.com/file"));
        assertFalse(UrlRecognition.isAbsoluteHttpUrl("https://example.com/a b"));
        assertFalse(UrlRecognition.isAbsoluteHttpUrl("https://user@example.com/private"));
    }

    @Test
    public void searchUsesPlusForSpacesAndPercentEncodesUtf8() {
        assertEquals("https://www.google.com/search?q=small+fast+browser",
                UrlRecognition.googleSearchUrl("small fast browser"));
        assertEquals("https://www.google.com/search?q=caf%C3%A9",
                UrlRecognition.googleSearchUrl("caf\u00e9"));
    }

    @Test
    public void resolvesPageRelativeLinksBeforeIcuHandoff() {
        assertEquals("https://example.com/jobs/42",
                UrlRecognition.resolve("https://example.com/deployments", "/jobs/42"));
        assertEquals("https://other.test/page",
                UrlRecognition.resolve("https://example.com/deployments", "https://other.test/page"));
    }

    @Test
    public void rendersAnExplicitIcuGetCommand() {
        assertEquals("icu get 'https://example.com/a?q=b'",
                UrlRecognition.icuGetCommand("https://example.com/a?q=b"));
    }
}

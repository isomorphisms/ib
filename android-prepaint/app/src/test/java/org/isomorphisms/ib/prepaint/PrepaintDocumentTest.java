package org.isomorphisms.ib.prepaint;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import java.io.IOException;
import java.io.StringReader;

import org.junit.Test;

public final class PrepaintDocumentTest {
    @Test
    public void parsesCompleteReplacementRevisions() throws Exception {
        PrepaintDocument document = parse(
                "ib-prepaint\t1\n"
                + "revision\t4\tpartial\n"
                + "title\tFirst paint\n"
                + "heading\t1\tStatus\n"
                + "row\tservice\thealthy\n"
                + "end\n"
                + "revision\t5\tcomplete\n"
                + "title\tFull paint\n"
                + "image\tdata:image/png;base64,AA==\tplot\tFigure 1\t/full-figure\n"
                + "end\n");

        assertEquals(2, document.revisions.size());
        assertEquals(4, document.revisions.get(0).sequence);
        assertFalse(document.revisions.get(0).complete);
        assertEquals(2, document.revisions.get(0).blocks.size());
        assertEquals(5, document.revisions.get(1).sequence);
        assertTrue(document.revisions.get(1).complete);
        assertEquals(PrepaintDocument.Block.IMAGE,
                document.revisions.get(1).blocks.get(0).kind);
        assertEquals("/full-figure",
                document.revisions.get(1).blocks.get(0).values.get(3));
    }

    @Test
    public void unescapesTextWithoutChangingFieldBoundaries() throws Exception {
        PrepaintDocument document = parse(
                "ib-prepaint\t1\n"
                + "revision\t1\tcomplete\n"
                + "text\tone\\ttwo\\nthree\\\\four\n"
                + "end\n");

        assertEquals("one\ttwo\nthree\\four",
                document.revisions.get(0).blocks.get(0).values.get(0));
    }

    @Test
    public void acceptsAnUnlinkedImageFromTheOriginalVersionOneShape() throws Exception {
        PrepaintDocument document = parse(
                "ib-prepaint\t1\n"
                + "revision\t1\tcomplete\n"
                + "image\tcontent://image\talternate\tcaption\n"
                + "end\n");

        assertEquals("", document.revisions.get(0).blocks.get(0).values.get(3));
    }

    @Test
    public void rejectsNonIncreasingRevisionSequence() {
        assertThrows(IOException.class, () -> parse(
                "ib-prepaint\t1\n"
                + "revision\t2\tpartial\nend\n"
                + "revision\t2\tcomplete\nend\n"));
    }

    @Test
    public void rejectsMarkupInsteadOfTreatingItAsAWebPage() {
        assertThrows(IOException.class, () -> parse("<html><body>not a prepaint</body></html>"));
    }

    @Test
    public void prepaintsOrdinaryTextAsParagraphs() throws Exception {
        PrepaintDocument document = parseOrPlainText(
                "First paragraph.\nStill first.\n\nSecond paragraph.\n",
                "notes.txt");

        assertEquals(PrepaintDocument.TEXT_SOURCE, document.sourceKind);
        assertEquals("notes.txt", document.revisions.get(0).title);
        assertEquals(2, document.revisions.get(0).blocks.size());
        assertEquals("First paragraph.\nStill first.",
                document.revisions.get(0).blocks.get(0).values.get(0));
        assertEquals("Second paragraph.",
                document.revisions.get(0).blocks.get(1).values.get(0));
    }

    @Test
    public void prepaintsAPlainTextUrlListAsLinks() throws Exception {
        PrepaintDocument document = parseOrPlainText(
                "https://example.com/one\nhttp://example.net/two?q=three\n",
                "urls.txt");

        assertEquals(2, document.revisions.get(0).blocks.size());
        assertEquals(PrepaintDocument.Block.LINK,
                document.revisions.get(0).blocks.get(0).kind);
        assertEquals("https://example.com/one",
                document.revisions.get(0).blocks.get(0).values.get(1));
        assertEquals(PrepaintDocument.Block.LINK,
                document.revisions.get(0).blocks.get(1).kind);
    }

    @Test
    public void malformedStructuredArtifactDoesNotFallBackToPlainText() {
        assertThrows(IOException.class, () -> parseOrPlainText(
                "ib-prepaint\t2\nrevision\t1\tcomplete\nend\n", "bad.prepaint"));
    }

    @Test
    public void rejectsNulBearingInputInsteadOfPaintingBinaryData() {
        assertThrows(IOException.class, () -> parseOrPlainText(
                "plain\0text", "not-text.bin"));
    }

    private static PrepaintDocument parse(String text) throws IOException {
        return PrepaintDocument.parse(new StringReader(text));
    }

    private static PrepaintDocument parseOrPlainText(String text, String title)
            throws IOException {
        return PrepaintDocument.parseOrPlainText(new StringReader(text), title);
    }
}

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
        assertTrue(document.revisions.get(0).blocks.get(0)
                instanceof PrepaintDocument.HeadingBlock);
        PrepaintDocument.HeadingBlock heading = (PrepaintDocument.HeadingBlock)
                document.revisions.get(0).blocks.get(0);
        assertEquals(PrepaintDocument.HeadingLevel.ONE, heading.level);
        assertEquals("Status", heading.text);
        assertTrue(document.revisions.get(0).blocks.get(1)
                instanceof PrepaintDocument.RowBlock);
        PrepaintDocument.RowBlock row = (PrepaintDocument.RowBlock)
                document.revisions.get(0).blocks.get(1);
        assertEquals(2, row.cells.size());
        assertTrue(document.revisions.get(1).blocks.get(0)
                instanceof PrepaintDocument.ImageBlock);
        PrepaintDocument.ImageBlock image = (PrepaintDocument.ImageBlock)
                document.revisions.get(1).blocks.get(0);
        assertEquals("/full-figure", image.linkTarget);
    }

    @Test
    public void unescapesTextWithoutChangingFieldBoundaries() throws Exception {
        PrepaintDocument document = parse(
                "ib-prepaint\t1\n"
                + "revision\t1\tcomplete\n"
                + "text\tone\\ttwo\\nthree\\\\four\n"
                + "end\n");

        PrepaintDocument.TextBlock text = (PrepaintDocument.TextBlock)
                document.revisions.get(0).blocks.get(0);
        assertEquals("one\ttwo\nthree\\four", text.text);
    }

    @Test
    public void acceptsAnUnlinkedImageFromTheOriginalVersionOneShape() throws Exception {
        PrepaintDocument document = parse(
                "ib-prepaint\t1\n"
                + "revision\t1\tcomplete\n"
                + "image\tcontent://image\talternate\tcaption\n"
                + "end\n");

        PrepaintDocument.ImageBlock image = (PrepaintDocument.ImageBlock)
                document.revisions.get(0).blocks.get(0);
        assertEquals("", image.linkTarget);
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

        assertEquals(PrepaintDocument.SourceKind.PLAIN_TEXT, document.sourceKind);
        assertEquals("notes.txt", document.revisions.get(0).title);
        assertEquals(2, document.revisions.get(0).blocks.size());
        PrepaintDocument.TextBlock first = (PrepaintDocument.TextBlock)
                document.revisions.get(0).blocks.get(0);
        PrepaintDocument.TextBlock second = (PrepaintDocument.TextBlock)
                document.revisions.get(0).blocks.get(1);
        assertEquals("First paragraph.\nStill first.", first.text);
        assertEquals("Second paragraph.", second.text);
    }

    @Test
    public void prepaintsAPlainTextUrlListAsLinks() throws Exception {
        PrepaintDocument document = parseOrPlainText(
                "https://example.com/one\nhttp://example.net/two?q=three\n",
                "urls.txt");

        assertEquals(2, document.revisions.get(0).blocks.size());
        assertTrue(document.revisions.get(0).blocks.get(0)
                instanceof PrepaintDocument.LinkBlock);
        PrepaintDocument.LinkBlock first = (PrepaintDocument.LinkBlock)
                document.revisions.get(0).blocks.get(0);
        assertEquals("https://example.com/one", first.target);
        assertTrue(document.revisions.get(0).blocks.get(1)
                instanceof PrepaintDocument.LinkBlock);
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

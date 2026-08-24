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
                + "image\tdata:image/png;base64,AA==\tplot\tFigure 1\n"
                + "end\n");

        assertEquals(2, document.revisions.size());
        assertEquals(4, document.revisions.get(0).sequence);
        assertFalse(document.revisions.get(0).complete);
        assertEquals(2, document.revisions.get(0).blocks.size());
        assertEquals(5, document.revisions.get(1).sequence);
        assertTrue(document.revisions.get(1).complete);
        assertEquals(PrepaintDocument.Block.IMAGE,
                document.revisions.get(1).blocks.get(0).kind);
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

    private static PrepaintDocument parse(String text) throws IOException {
        return PrepaintDocument.parse(new StringReader(text));
    }
}

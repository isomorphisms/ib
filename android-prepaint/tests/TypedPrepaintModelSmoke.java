package org.isomorphisms.ib.prepaint;

import java.io.IOException;
import java.io.StringReader;

public final class TypedPrepaintModelSmoke {
    public static void main(String[] arguments) throws Exception {
        PrepaintDocument document = PrepaintDocument.parse(new StringReader(
                "ib-prepaint\t1\n"
                + "revision\t4\tcomplete\n"
                + "heading\t2\tTyped heading\n"
                + "text\tordinary prose\n"
                + "link\tlabel\t/target\n"
                + "row\tleft\tright\n"
                + "form\tSearch\t/search\n"
                + "image\tcontent://figure\talt\tcaption\t/figure\n"
                + "end\n"));

        check(document.sourceKind == PrepaintDocument.SourceKind.ARTIFACT,
                "structured input must retain artifact source kind");
        check(document.revisions.get(0).blocks.get(0)
                        instanceof PrepaintDocument.HeadingBlock,
                "heading record must become HeadingBlock");
        PrepaintDocument.HeadingBlock heading = (PrepaintDocument.HeadingBlock)
                document.revisions.get(0).blocks.get(0);
        check(heading.level == PrepaintDocument.HeadingLevel.TWO,
                "heading level must retain its checked six-case value");
        check("Typed heading".equals(heading.text), "heading text changed");
        check(document.revisions.get(0).blocks.get(1)
                        instanceof PrepaintDocument.TextBlock,
                "text record must become TextBlock");
        check(document.revisions.get(0).blocks.get(2)
                        instanceof PrepaintDocument.LinkBlock,
                "link record must become LinkBlock");
        check(document.revisions.get(0).blocks.get(3)
                        instanceof PrepaintDocument.RowBlock,
                "row record must become RowBlock");
        check(document.revisions.get(0).blocks.get(4)
                        instanceof PrepaintDocument.FormBlock,
                "form record must become FormBlock");
        check(document.revisions.get(0).blocks.get(5)
                        instanceof PrepaintDocument.ImageBlock,
                "image record must become ImageBlock");

        PrepaintDocument.ImageBlock image = (PrepaintDocument.ImageBlock)
                document.revisions.get(0).blocks.get(5);
        check("content://figure".equals(image.source), "image source changed");
        check("alt".equals(image.alternateText), "image alternate text changed");
        check("caption".equals(image.caption), "image caption changed");
        check("/figure".equals(image.linkTarget), "image link target changed");

        PrepaintDocument plain = PrepaintDocument.parseOrPlainText(
                new StringReader("ordinary prose"), "notes.txt");
        check(plain.sourceKind == PrepaintDocument.SourceKind.PLAIN_TEXT,
                "plain input must retain plain-text source kind");
        expectParseFailure("heading\t0\ttoo low", "heading level 0");
        expectParseFailure("heading\t7\ttoo high", "heading level 7");
        expectParseFailure("row", "empty row");
    }

    private static void expectParseFailure(String record, String description)
            throws Exception {
        try {
            PrepaintDocument.parse(new StringReader(
                    "ib-prepaint\t1\nrevision\t1\tcomplete\n"
                    + record + "\nend\n"));
            throw new AssertionError(description + " was accepted");
        } catch (IOException expected) {
            // The wire boundary must reject before constructing a semantic block.
        }
    }

    private static void check(boolean condition, String message) {
        if (!condition) {
            throw new AssertionError(message);
        }
    }
}

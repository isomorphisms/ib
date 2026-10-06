package org.isomorphisms.ib.prepaint;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

final class PrepaintDocument {
    static final String FORMAT = "ib-prepaint";
    static final String VERSION = "1";
    private static final long MAX_CHARACTERS = 4L * 1024L * 1024L;
    private static final int MAX_REVISIONS = 32;
    private static final int MAX_BLOCKS_PER_REVISION = 4096;

    final List<Revision> revisions;
    final SourceKind sourceKind;

    private PrepaintDocument(List<Revision> revisions, SourceKind sourceKind) {
        this.revisions = Collections.unmodifiableList(new ArrayList<>(revisions));
        this.sourceKind = sourceKind;
    }

    static PrepaintDocument parseOrPlainText(Reader source, String title) throws IOException {
        String text = readBoundedText(source);
        if (!text.isEmpty() && text.charAt(0) == '\ufeff') {
            text = text.substring(1);
        }
        rejectNul(text);

        if (startsLikeStructuredArtifact(text)) {
            return parse(new StringReader(text));
        }
        return fromPlainText(text, title);
    }

    static PrepaintDocument fromPlainText(String text, String title) {
        String normalized = text.replace("\r\n", "\n").replace('\r', '\n');
        List<Block> blocks = new ArrayList<>();
        StringBuilder paragraph = new StringBuilder();

        String[] lines = normalized.split("\n", -1);
        for (String line : lines) {
            String trimmed = line.trim();
            if (trimmed.isEmpty()) {
                finishParagraph(paragraph, blocks);
            } else if (UrlRecognition.isAbsoluteHttpUrl(trimmed)) {
                finishParagraph(paragraph, blocks);
                blocks.add(new LinkBlock(trimmed, trimmed));
            } else {
                if (paragraph.length() != 0) {
                    paragraph.append('\n');
                }
                paragraph.append(line);
            }
        }
        finishParagraph(paragraph, blocks);

        String visibleTitle = title == null || title.trim().isEmpty()
                ? "Plain text" : title.trim();
        Revision revision = new Revision(0, true, "", "", visibleTitle, blocks);
        return new PrepaintDocument(Collections.singletonList(revision), SourceKind.PLAIN_TEXT);
    }

    static PrepaintDocument parse(Reader source) throws IOException {
        BufferedReader reader = new BufferedReader(new BoundedReader(source, MAX_CHARACTERS));
        String line;
        int lineNumber = 0;
        boolean sawHeader = false;
        long lastSequence = -1;
        RevisionBuilder current = null;
        List<Revision> revisions = new ArrayList<>();

        while ((line = reader.readLine()) != null) {
            lineNumber += 1;
            if (line.isEmpty() || line.startsWith("#")) {
                continue;
            }
            List<String> fields = splitFields(line, lineNumber);
            String record = fields.get(0);

            if (!sawHeader) {
                require(fields.size() == 2
                        && FORMAT.equals(fields.get(0))
                        && VERSION.equals(fields.get(1)), lineNumber,
                        "expected ib-prepaint version 1 header");
                sawHeader = true;
                continue;
            }

            if ("revision".equals(record)) {
                require(current == null, lineNumber, "nested revision");
                require(fields.size() == 3, lineNumber,
                        "revision needs sequence and partial/complete state");
                long sequence = parseSequence(fields.get(1), lineNumber);
                require(sequence > lastSequence, lineNumber,
                        "revision sequence must increase");
                String state = fields.get(2);
                require("partial".equals(state) || "complete".equals(state), lineNumber,
                        "revision state must be partial or complete");
                current = new RevisionBuilder(sequence, "complete".equals(state));
                lastSequence = sequence;
                continue;
            }

            if ("end".equals(record)) {
                require(fields.size() == 1, lineNumber, "end takes no fields");
                require(current != null, lineNumber, "end outside revision");
                require(revisions.size() < MAX_REVISIONS, lineNumber, "too many revisions");
                revisions.add(current.build());
                current = null;
                continue;
            }

            require(current != null, lineNumber, "record outside revision");
            require(current.blocks.size() < MAX_BLOCKS_PER_REVISION, lineNumber,
                    "too many blocks in revision");
            switch (record) {
                case "title":
                    require(fields.size() == 2, lineNumber, "title needs text");
                    current.title = fields.get(1);
                    break;
                case "requested-url":
                    require(fields.size() == 2, lineNumber, "requested-url needs a value");
                    current.requestedUrl = fields.get(1);
                    break;
                case "resolved-url":
                    require(fields.size() == 2, lineNumber, "resolved-url needs a value");
                    current.resolvedUrl = fields.get(1);
                    break;
                case "heading":
                    require(fields.size() == 3, lineNumber, "heading needs level and text");
                    HeadingLevel level = parseHeadingLevel(fields.get(1), lineNumber);
                    current.blocks.add(new HeadingBlock(level, fields.get(2)));
                    break;
                case "text":
                    require(fields.size() == 2, lineNumber, "text needs a value");
                    current.blocks.add(new TextBlock(fields.get(1)));
                    break;
                case "link":
                    require(fields.size() == 3, lineNumber, "link needs label and target");
                    current.blocks.add(new LinkBlock(fields.get(1), fields.get(2)));
                    break;
                case "row":
                    require(fields.size() >= 2, lineNumber, "row needs at least one cell");
                    current.blocks.add(new RowBlock(fields.subList(1, fields.size())));
                    break;
                case "form":
                    require(fields.size() == 3, lineNumber, "form needs label and action");
                    current.blocks.add(new FormBlock(fields.get(1), fields.get(2)));
                    break;
                case "image":
                    require(fields.size() == 4 || fields.size() == 5, lineNumber,
                            "image needs source, alternate text, caption, and optional link");
                    current.blocks.add(new ImageBlock(
                            fields.get(1),
                            fields.get(2),
                            fields.get(3),
                            fields.size() == 4 ? "" : fields.get(4)));
                    break;
                default:
                    throw parseError(lineNumber, "unknown record " + record);
            }
        }

        require(sawHeader, lineNumber, "missing header");
        require(current == null, lineNumber, "unterminated revision");
        require(!revisions.isEmpty(), lineNumber, "no revisions");
        return new PrepaintDocument(revisions, SourceKind.ARTIFACT);
    }

    private static void finishParagraph(StringBuilder paragraph, List<Block> blocks) {
        if (paragraph.length() == 0) {
            return;
        }
        blocks.add(new TextBlock(paragraph.toString()));
        paragraph.setLength(0);
    }

    private static String readBoundedText(Reader source) throws IOException {
        StringBuilder text = new StringBuilder();
        char[] buffer = new char[8192];
        long characters = 0;
        int amount;
        while ((amount = source.read(buffer)) != -1) {
            characters += amount;
            if (characters > MAX_CHARACTERS) {
                throw new IOException("prepaint exceeds 4 MiB text budget");
            }
            text.append(buffer, 0, amount);
        }
        return text.toString();
    }

    private static boolean startsLikeStructuredArtifact(String text) {
        int lineEnd = text.indexOf('\n');
        String firstLine = lineEnd < 0 ? text : text.substring(0, lineEnd);
        if (firstLine.endsWith("\r")) {
            firstLine = firstLine.substring(0, firstLine.length() - 1);
        }
        return firstLine.startsWith(FORMAT);
    }

    private static void rejectNul(String text) throws IOException {
        if (text.indexOf('\0') >= 0) {
            throw new IOException("selected file is not plain UTF-8 text");
        }
    }

    private static long parseSequence(String value, int lineNumber) throws IOException {
        try {
            long sequence = Long.parseLong(value);
            require(sequence >= 0, lineNumber, "revision sequence must be nonnegative");
            return sequence;
        } catch (NumberFormatException error) {
            throw parseError(lineNumber, "invalid revision sequence");
        }
    }

    private static HeadingLevel parseHeadingLevel(String value, int lineNumber)
            throws IOException {
        try {
            int level = Integer.parseInt(value);
            return HeadingLevel.fromWireNumber(level, lineNumber);
        } catch (NumberFormatException error) {
            throw parseError(lineNumber, "invalid heading level");
        }
    }

    private static List<String> splitFields(String line, int lineNumber) throws IOException {
        List<String> fields = new ArrayList<>();
        StringBuilder field = new StringBuilder();
        boolean escaping = false;
        for (int index = 0; index < line.length(); index += 1) {
            char value = line.charAt(index);
            if (escaping) {
                switch (value) {
                    case 'n': field.append('\n'); break;
                    case 'r': field.append('\r'); break;
                    case 't': field.append('\t'); break;
                    case '\\': field.append('\\'); break;
                    default: throw parseError(lineNumber, "invalid escape \\" + value);
                }
                escaping = false;
            } else if (value == '\\') {
                escaping = true;
            } else if (value == '\t') {
                fields.add(field.toString());
                field.setLength(0);
            } else {
                field.append(value);
            }
        }
        require(!escaping, lineNumber, "trailing escape");
        fields.add(field.toString());
        return fields;
    }

    private static void require(boolean condition, int lineNumber, String message)
            throws IOException {
        if (!condition) {
            throw parseError(lineNumber, message);
        }
    }

    private static IOException parseError(int lineNumber, String message) {
        return new IOException("prepaint line " + lineNumber + ": " + message);
    }

    enum SourceKind {
        ARTIFACT,
        PLAIN_TEXT
    }

    enum HeadingLevel {
        ONE(1),
        TWO(2),
        THREE(3),
        FOUR(4),
        FIVE(5),
        SIX(6);

        final int wireNumber;

        HeadingLevel(int wireNumber) {
            this.wireNumber = wireNumber;
        }

        private static HeadingLevel fromWireNumber(int number, int lineNumber)
                throws IOException {
            for (HeadingLevel level : values()) {
                if (level.wireNumber == number) {
                    return level;
                }
            }
            throw parseError(lineNumber, "heading level must be between 1 and 6");
        }
    }

    static final class Revision {
        final long sequence;
        final boolean complete;
        final String requestedUrl;
        final String resolvedUrl;
        final String title;
        final List<Block> blocks;

        private Revision(long sequence, boolean complete, String requestedUrl,
                         String resolvedUrl, String title, List<Block> blocks) {
            this.sequence = sequence;
            this.complete = complete;
            this.requestedUrl = requestedUrl;
            this.resolvedUrl = resolvedUrl;
            this.title = title;
            this.blocks = Collections.unmodifiableList(new ArrayList<>(blocks));
        }
    }

    abstract static class Block {
        private Block() {
        }
    }

    static final class HeadingBlock extends Block {
        final HeadingLevel level;
        final String text;

        private HeadingBlock(HeadingLevel level, String text) {
            this.level = level;
            this.text = text;
        }
    }

    static final class TextBlock extends Block {
        final String text;

        private TextBlock(String text) {
            this.text = text;
        }
    }

    static final class LinkBlock extends Block {
        final String label;
        final String target;

        private LinkBlock(String label, String target) {
            this.label = label;
            this.target = target;
        }
    }

    static final class RowBlock extends Block {
        final List<String> cells;

        private RowBlock(List<String> cells) {
            if (cells.isEmpty()) {
                throw new IllegalArgumentException("row must contain at least one cell");
            }
            this.cells = Collections.unmodifiableList(new ArrayList<>(cells));
        }
    }

    static final class FormBlock extends Block {
        final String label;
        final String action;

        private FormBlock(String label, String action) {
            this.label = label;
            this.action = action;
        }
    }

    static final class ImageBlock extends Block {
        final String source;
        final String alternateText;
        final String caption;
        final String linkTarget;

        private ImageBlock(String source, String alternateText, String caption,
                           String linkTarget) {
            this.source = source;
            this.alternateText = alternateText;
            this.caption = caption;
            this.linkTarget = linkTarget;
        }
    }

    private static final class RevisionBuilder {
        final long sequence;
        final boolean complete;
        String requestedUrl = "";
        String resolvedUrl = "";
        String title = "";
        final List<Block> blocks = new ArrayList<>();

        RevisionBuilder(long sequence, boolean complete) {
            this.sequence = sequence;
            this.complete = complete;
        }

        Revision build() {
            return new Revision(sequence, complete, requestedUrl, resolvedUrl, title, blocks);
        }
    }

    private static final class BoundedReader extends Reader {
        private final Reader source;
        private long remaining;

        BoundedReader(Reader source, long maximumCharacters) {
            this.source = source;
            this.remaining = maximumCharacters;
        }

        @Override
        public int read(char[] buffer, int offset, int length) throws IOException {
            if (remaining == 0) {
                throw new IOException("prepaint exceeds 4 MiB text budget");
            }
            int boundedLength = (int) Math.min((long) length, remaining);
            int amount = source.read(buffer, offset, boundedLength);
            if (amount > 0) {
                remaining -= amount;
            }
            return amount;
        }

        @Override
        public void close() throws IOException {
            source.close();
        }
    }
}

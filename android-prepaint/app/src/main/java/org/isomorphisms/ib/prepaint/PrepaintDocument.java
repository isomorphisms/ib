package org.isomorphisms.ib.prepaint;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.Reader;
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

    private PrepaintDocument(List<Revision> revisions) {
        this.revisions = Collections.unmodifiableList(new ArrayList<>(revisions));
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
                    int level = parseHeadingLevel(fields.get(1), lineNumber);
                    current.blocks.add(Block.heading(level, fields.get(2)));
                    break;
                case "text":
                    require(fields.size() == 2, lineNumber, "text needs a value");
                    current.blocks.add(Block.of(Block.TEXT, fields.get(1)));
                    break;
                case "link":
                    require(fields.size() == 3, lineNumber, "link needs label and target");
                    current.blocks.add(Block.of(Block.LINK, fields.get(1), fields.get(2)));
                    break;
                case "row":
                    require(fields.size() >= 2, lineNumber, "row needs at least one cell");
                    current.blocks.add(Block.of(Block.ROW,
                            fields.subList(1, fields.size()).toArray(new String[0])));
                    break;
                case "form":
                    require(fields.size() == 3, lineNumber, "form needs label and action");
                    current.blocks.add(Block.of(Block.FORM, fields.get(1), fields.get(2)));
                    break;
                case "image":
                    require(fields.size() == 4, lineNumber,
                            "image needs source, alternate text, and caption");
                    current.blocks.add(Block.of(Block.IMAGE,
                            fields.get(1), fields.get(2), fields.get(3)));
                    break;
                default:
                    throw parseError(lineNumber, "unknown record " + record);
            }
        }

        require(sawHeader, lineNumber, "missing header");
        require(current == null, lineNumber, "unterminated revision");
        require(!revisions.isEmpty(), lineNumber, "no revisions");
        return new PrepaintDocument(revisions);
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

    private static int parseHeadingLevel(String value, int lineNumber) throws IOException {
        try {
            int level = Integer.parseInt(value);
            require(level >= 1 && level <= 6, lineNumber,
                    "heading level must be between 1 and 6");
            return level;
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

    static final class Revision {
        final long sequence;
        final boolean complete;
        final String requestedUrl;
        final String resolvedUrl;
        final String title;
        final List<Block> blocks;

        Revision(long sequence, boolean complete, String requestedUrl,
                 String resolvedUrl, String title, List<Block> blocks) {
            this.sequence = sequence;
            this.complete = complete;
            this.requestedUrl = requestedUrl;
            this.resolvedUrl = resolvedUrl;
            this.title = title;
            this.blocks = Collections.unmodifiableList(new ArrayList<>(blocks));
        }
    }

    static final class Block {
        static final String HEADING = "heading";
        static final String TEXT = "text";
        static final String LINK = "link";
        static final String ROW = "row";
        static final String FORM = "form";
        static final String IMAGE = "image";

        final String kind;
        final int level;
        final List<String> values;

        private Block(String kind, int level, String... values) {
            this.kind = kind;
            this.level = level;
            List<String> copy = new ArrayList<>();
            Collections.addAll(copy, values);
            this.values = Collections.unmodifiableList(copy);
        }

        static Block heading(int level, String text) {
            return new Block(HEADING, level, text);
        }

        static Block of(String kind, String... values) {
            return new Block(kind, 0, values);
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

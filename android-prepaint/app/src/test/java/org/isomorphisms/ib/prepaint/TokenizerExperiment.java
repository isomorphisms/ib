package org.isomorphisms.ib.prepaint;

import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Objects;

/**
 * Byte-level tokenizer experiment for issue #38.
 *
 * <p>This is deliberately a lexical experiment, not an HTML conformance tokenizer. Its contract
 * is the exact event stream produced here. The scalar oracle handles one byte at a time; the
 * table-driven candidate bulk-scans bytes whose transition is APPEND_RUN and dispatches only at
 * exceptional bytes. UTF-8 bytes are handed off unchanged rather than decoded here.</p>
 */
final class TokenizerExperiment {
    private TokenizerExperiment() {}

    enum State {
        DATA,
        TAG,
        DOUBLE_QUOTED,
        SINGLE_QUOTED,
        ENTITY_DATA,
        ENTITY_TAG,
        ENTITY_DOUBLE_QUOTED,
        ENTITY_SINGLE_QUOTED
    }

    enum EventKind {
        RUN,
        TAG_OPEN,
        TAG_CLOSE,
        DOUBLE_QUOTE_OPEN,
        DOUBLE_QUOTE_CLOSE,
        SINGLE_QUOTE_OPEN,
        SINGLE_QUOTE_CLOSE,
        ENTITY,
        MALFORMED_ENTITY,
        MALFORMED_LT_IN_TAG,
        NUL,
        UTF8_HANDOFF,
        MALFORMED_EOF
    }

    static final class Event {
        final EventKind kind;
        final State context;
        final int offset;
        final byte[] bytes;

        Event(EventKind kind, State context, int offset, byte[] bytes) {
            this.kind = Objects.requireNonNull(kind);
            this.context = Objects.requireNonNull(context);
            this.offset = offset;
            this.bytes = Objects.requireNonNull(bytes);
        }

        static Event one(EventKind kind, State context, int offset, int value) {
            return new Event(kind, context, offset, new byte[] {(byte) value});
        }

        String ascii() {
            return new String(bytes, StandardCharsets.ISO_8859_1);
        }

        @Override
        public boolean equals(Object other) {
            if (!(other instanceof Event)) {
                return false;
            }
            Event event = (Event) other;
            return kind == event.kind
                    && context == event.context
                    && offset == event.offset
                    && Arrays.equals(bytes, event.bytes);
        }

        @Override
        public int hashCode() {
            return Objects.hash(kind, context, offset, Arrays.hashCode(bytes));
        }

        @Override
        public String toString() {
            StringBuilder text = new StringBuilder();
            for (byte value : bytes) {
                int b = value & 0xff;
                if (b >= 0x20 && b <= 0x7e) {
                    text.append((char) b);
                } else {
                    text.append(String.format("\\x%02x", b));
                }
            }
            return kind + "@" + offset + "/" + context + ":" + text;
        }
    }

    static final class Result {
        final List<Event> events;
        final int bulkBytes;
        final int exceptionalDispatches;

        Result(List<Event> events, int bulkBytes, int exceptionalDispatches) {
            this.events = Collections.unmodifiableList(new ArrayList<>(events));
            this.bulkBytes = bulkBytes;
            this.exceptionalDispatches = exceptionalDispatches;
        }
    }

    static Result scalar(byte[] input) {
        return scalar(List.of(input));
    }

    static Result scalar(List<byte[]> chunks) {
        Scalar scanner = new Scalar();
        for (byte[] chunk : chunks) {
            scanner.feed(chunk);
        }
        return scanner.finish();
    }

    static Result tableDriven(byte[] input) {
        return tableDriven(List.of(input));
    }

    static Result tableDriven(List<byte[]> chunks) {
        TableDriven scanner = new TableDriven();
        for (byte[] chunk : chunks) {
            scanner.feed(chunk);
        }
        return scanner.finish();
    }

    private abstract static class Scanner {
        State state = State.DATA;
        int offset;
        final List<Event> events = new ArrayList<>();

        private State pendingRunState;
        private int pendingRunOffset;
        private final ByteArrayOutputStream pendingRun = new ByteArrayOutputStream();

        private State pendingUtf8State;
        private int pendingUtf8Offset;
        private final ByteArrayOutputStream pendingUtf8 = new ByteArrayOutputStream();

        State entityReturn;
        int entityOffset;
        final ByteArrayOutputStream entity = new ByteArrayOutputStream();

        abstract void feed(byte[] chunk);

        Result finishWithStats(int bulkBytes, int exceptionalDispatches) {
            flushRun();
            flushUtf8();
            if (isEntity(state)) {
                emitEntity(EventKind.MALFORMED_ENTITY);
                state = entityReturn;
            }
            if (state != State.DATA) {
                events.add(new Event(EventKind.MALFORMED_EOF, state, offset, new byte[0]));
            }
            return new Result(events, bulkBytes, exceptionalDispatches);
        }

        void appendRun(State context, byte[] input, int start, int end) {
            if (start == end) {
                return;
            }
            flushUtf8();
            if (pendingRun.size() == 0) {
                pendingRunState = context;
                pendingRunOffset = offset;
            } else if (pendingRunState != context) {
                flushRun();
                pendingRunState = context;
                pendingRunOffset = offset;
            }
            pendingRun.write(input, start, end - start);
            offset += end - start;
        }

        void appendRunByte(State context, int value) {
            flushUtf8();
            if (pendingRun.size() == 0) {
                pendingRunState = context;
                pendingRunOffset = offset;
            } else if (pendingRunState != context) {
                flushRun();
                pendingRunState = context;
                pendingRunOffset = offset;
            }
            pendingRun.write(value);
            offset++;
        }

        void appendUtf8(State context, int value) {
            flushRun();
            if (pendingUtf8.size() == 0) {
                pendingUtf8State = context;
                pendingUtf8Offset = offset;
            } else if (pendingUtf8State != context) {
                flushUtf8();
                pendingUtf8State = context;
                pendingUtf8Offset = offset;
            }
            pendingUtf8.write(value);
            offset++;
        }

        void flushRun() {
            if (pendingRun.size() != 0) {
                events.add(new Event(EventKind.RUN, pendingRunState, pendingRunOffset,
                        pendingRun.toByteArray()));
                pendingRun.reset();
            }
        }

        void flushUtf8() {
            if (pendingUtf8.size() != 0) {
                events.add(new Event(EventKind.UTF8_HANDOFF, pendingUtf8State, pendingUtf8Offset,
                        pendingUtf8.toByteArray()));
                pendingUtf8.reset();
            }
        }

        void emitOne(EventKind kind, State context, int value) {
            flushRun();
            flushUtf8();
            events.add(Event.one(kind, context, offset, value));
            offset++;
        }

        void startEntity(State returnState) {
            flushRun();
            flushUtf8();
            entityReturn = returnState;
            entityOffset = offset;
            entity.reset();
            entity.write('&');
            offset++;
            state = entityState(returnState);
        }

        void appendEntity(int value) {
            entity.write(value);
            offset++;
        }

        void emitEntity(EventKind kind) {
            flushRun();
            flushUtf8();
            events.add(new Event(kind, entityReturn, entityOffset, entity.toByteArray()));
            entity.reset();
        }

        void endEntity() {
            emitEntity(EventKind.ENTITY);
            state = entityReturn;
        }

        void breakEntity() {
            emitEntity(EventKind.MALFORMED_ENTITY);
            state = entityReturn;
        }
    }

    private static final class Scalar extends Scanner {
        int dispatches;

        @Override
        void feed(byte[] chunk) {
            int i = 0;
            while (i < chunk.length) {
                int b = chunk[i] & 0xff;
                dispatches++;
                if (isEntity(state)) {
                    if (b == ';') {
                        appendEntity(b);
                        endEntity();
                        i++;
                    } else if (entityBoundary(state, b)) {
                        breakEntity();
                    } else {
                        appendEntity(b);
                        i++;
                    }
                    continue;
                }

                State context = state;
                if (b == 0) {
                    emitOne(EventKind.NUL, context, b);
                    i++;
                } else if (b >= 0x80) {
                    appendUtf8(context, b);
                    i++;
                } else {
                    switch (state) {
                        case DATA:
                            if (b == '<') {
                                emitOne(EventKind.TAG_OPEN, State.DATA, b);
                                state = State.TAG;
                            } else if (b == '&') {
                                startEntity(State.DATA);
                            } else {
                                appendRunByte(State.DATA, b);
                            }
                            i++;
                            break;
                        case TAG:
                            if (b == '>') {
                                emitOne(EventKind.TAG_CLOSE, State.TAG, b);
                                state = State.DATA;
                            } else if (b == '"') {
                                emitOne(EventKind.DOUBLE_QUOTE_OPEN, State.TAG, b);
                                state = State.DOUBLE_QUOTED;
                            } else if (b == '\'') {
                                emitOne(EventKind.SINGLE_QUOTE_OPEN, State.TAG, b);
                                state = State.SINGLE_QUOTED;
                            } else if (b == '&') {
                                startEntity(State.TAG);
                            } else if (b == '<') {
                                emitOne(EventKind.MALFORMED_LT_IN_TAG, State.TAG, b);
                            } else {
                                appendRunByte(State.TAG, b);
                            }
                            i++;
                            break;
                        case DOUBLE_QUOTED:
                            if (b == '"') {
                                emitOne(EventKind.DOUBLE_QUOTE_CLOSE, State.DOUBLE_QUOTED, b);
                                state = State.TAG;
                            } else if (b == '&') {
                                startEntity(State.DOUBLE_QUOTED);
                            } else {
                                appendRunByte(State.DOUBLE_QUOTED, b);
                            }
                            i++;
                            break;
                        case SINGLE_QUOTED:
                            if (b == '\'') {
                                emitOne(EventKind.SINGLE_QUOTE_CLOSE, State.SINGLE_QUOTED, b);
                                state = State.TAG;
                            } else if (b == '&') {
                                startEntity(State.SINGLE_QUOTED);
                            } else {
                                appendRunByte(State.SINGLE_QUOTED, b);
                            }
                            i++;
                            break;
                        default:
                            throw new AssertionError(state);
                    }
                }
            }
        }

        Result finish() {
            return finishWithStats(0, dispatches);
        }
    }

    private enum Class {
        OTHER,
        LT,
        GT,
        AMP,
        DOUBLE_QUOTE,
        SINGLE_QUOTE,
        SEMICOLON,
        SPACE,
        NUL,
        HIGH
    }

    private enum Action {
        APPEND_RUN,
        APPEND_ENTITY,
        TAG_OPEN,
        TAG_CLOSE,
        START_ENTITY,
        DOUBLE_QUOTE_OPEN,
        DOUBLE_QUOTE_CLOSE,
        SINGLE_QUOTE_OPEN,
        SINGLE_QUOTE_CLOSE,
        END_ENTITY,
        BREAK_ENTITY,
        MALFORMED_LT,
        NUL,
        UTF8
    }

    private static final Class[] BYTE_CLASS = makeByteClasses();
    private static final Action[][] TABLE = makeTable();

    private static final class TableDriven extends Scanner {
        int bulkBytes;
        int dispatches;

        @Override
        void feed(byte[] chunk) {
            int i = 0;
            while (i < chunk.length) {
                if (!isEntity(state)) {
                    int end = scanOrdinaryRun(chunk, i, state);
                    if (end > i) {
                        appendRun(state, chunk, i, end);
                        bulkBytes += end - i;
                        i = end;
                        if (i == chunk.length) {
                            continue;
                        }
                    }
                }

                int b = chunk[i] & 0xff;
                Action action = TABLE[state.ordinal()][BYTE_CLASS[b].ordinal()];
                dispatches++;
                switch (action) {
                    case APPEND_RUN:
                        appendRunByte(state, b);
                        i++;
                        break;
                    case APPEND_ENTITY:
                        appendEntity(b);
                        i++;
                        break;
                    case TAG_OPEN:
                        emitOne(EventKind.TAG_OPEN, State.DATA, b);
                        state = State.TAG;
                        i++;
                        break;
                    case TAG_CLOSE:
                        emitOne(EventKind.TAG_CLOSE, State.TAG, b);
                        state = State.DATA;
                        i++;
                        break;
                    case START_ENTITY:
                        startEntity(state);
                        i++;
                        break;
                    case DOUBLE_QUOTE_OPEN:
                        emitOne(EventKind.DOUBLE_QUOTE_OPEN, State.TAG, b);
                        state = State.DOUBLE_QUOTED;
                        i++;
                        break;
                    case DOUBLE_QUOTE_CLOSE:
                        emitOne(EventKind.DOUBLE_QUOTE_CLOSE, State.DOUBLE_QUOTED, b);
                        state = State.TAG;
                        i++;
                        break;
                    case SINGLE_QUOTE_OPEN:
                        emitOne(EventKind.SINGLE_QUOTE_OPEN, State.TAG, b);
                        state = State.SINGLE_QUOTED;
                        i++;
                        break;
                    case SINGLE_QUOTE_CLOSE:
                        emitOne(EventKind.SINGLE_QUOTE_CLOSE, State.SINGLE_QUOTED, b);
                        state = State.TAG;
                        i++;
                        break;
                    case END_ENTITY:
                        appendEntity(b);
                        endEntity();
                        i++;
                        break;
                    case BREAK_ENTITY:
                        breakEntity();
                        break;
                    case MALFORMED_LT:
                        emitOne(EventKind.MALFORMED_LT_IN_TAG, State.TAG, b);
                        i++;
                        break;
                    case NUL:
                        emitOne(EventKind.NUL, state, b);
                        i++;
                        break;
                    case UTF8:
                        appendUtf8(state, b);
                        i++;
                        break;
                    default:
                        throw new AssertionError(action);
                }
            }
        }

        private static int scanOrdinaryRun(byte[] input, int start, State state) {
            int i = start;
            while (i < input.length) {
                int b = input[i] & 0xff;
                if (TABLE[state.ordinal()][BYTE_CLASS[b].ordinal()] != Action.APPEND_RUN) {
                    break;
                }
                i++;
            }
            return i;
        }

        Result finish() {
            return finishWithStats(bulkBytes, dispatches);
        }
    }

    private static Class[] makeByteClasses() {
        Class[] classes = new Class[256];
        Arrays.fill(classes, Class.OTHER);
        for (int i = 0x80; i < 256; i++) {
            classes[i] = Class.HIGH;
        }
        classes[0] = Class.NUL;
        classes['<'] = Class.LT;
        classes['>'] = Class.GT;
        classes['&'] = Class.AMP;
        classes['"'] = Class.DOUBLE_QUOTE;
        classes['\''] = Class.SINGLE_QUOTE;
        classes[';'] = Class.SEMICOLON;
        classes[' '] = Class.SPACE;
        classes['\t'] = Class.SPACE;
        classes['\n'] = Class.SPACE;
        classes['\r'] = Class.SPACE;
        classes['\f'] = Class.SPACE;
        return classes;
    }

    private static Action[][] makeTable() {
        Action[][] table = new Action[State.values().length][Class.values().length];
        for (Action[] row : table) {
            Arrays.fill(row, Action.APPEND_RUN);
        }

        set(table, State.DATA, Class.LT, Action.TAG_OPEN);
        set(table, State.DATA, Class.AMP, Action.START_ENTITY);

        set(table, State.TAG, Class.GT, Action.TAG_CLOSE);
        set(table, State.TAG, Class.AMP, Action.START_ENTITY);
        set(table, State.TAG, Class.DOUBLE_QUOTE, Action.DOUBLE_QUOTE_OPEN);
        set(table, State.TAG, Class.SINGLE_QUOTE, Action.SINGLE_QUOTE_OPEN);
        set(table, State.TAG, Class.LT, Action.MALFORMED_LT);

        set(table, State.DOUBLE_QUOTED, Class.DOUBLE_QUOTE, Action.DOUBLE_QUOTE_CLOSE);
        set(table, State.DOUBLE_QUOTED, Class.AMP, Action.START_ENTITY);

        set(table, State.SINGLE_QUOTED, Class.SINGLE_QUOTE, Action.SINGLE_QUOTE_CLOSE);
        set(table, State.SINGLE_QUOTED, Class.AMP, Action.START_ENTITY);

        for (State state : State.values()) {
            set(table, state, Class.NUL, Action.NUL);
            set(table, state, Class.HIGH, Action.UTF8);
        }

        for (State state : List.of(State.ENTITY_DATA, State.ENTITY_TAG,
                State.ENTITY_DOUBLE_QUOTED, State.ENTITY_SINGLE_QUOTED)) {
            Arrays.fill(table[state.ordinal()], Action.APPEND_ENTITY);
            set(table, state, Class.SEMICOLON, Action.END_ENTITY);
            set(table, state, Class.NUL, Action.BREAK_ENTITY);
            set(table, state, Class.HIGH, Action.BREAK_ENTITY);
            set(table, state, Class.AMP, Action.BREAK_ENTITY);
            set(table, state, Class.SPACE, Action.BREAK_ENTITY);
        }

        set(table, State.ENTITY_DATA, Class.LT, Action.BREAK_ENTITY);
        set(table, State.ENTITY_TAG, Class.LT, Action.BREAK_ENTITY);
        set(table, State.ENTITY_TAG, Class.GT, Action.BREAK_ENTITY);
        set(table, State.ENTITY_TAG, Class.DOUBLE_QUOTE, Action.BREAK_ENTITY);
        set(table, State.ENTITY_TAG, Class.SINGLE_QUOTE, Action.BREAK_ENTITY);
        set(table, State.ENTITY_DOUBLE_QUOTED, Class.DOUBLE_QUOTE, Action.BREAK_ENTITY);
        set(table, State.ENTITY_SINGLE_QUOTED, Class.SINGLE_QUOTE, Action.BREAK_ENTITY);
        return table;
    }

    private static void set(Action[][] table, State state, Class clazz, Action action) {
        table[state.ordinal()][clazz.ordinal()] = action;
    }

    private static boolean isEntity(State state) {
        return state == State.ENTITY_DATA
                || state == State.ENTITY_TAG
                || state == State.ENTITY_DOUBLE_QUOTED
                || state == State.ENTITY_SINGLE_QUOTED;
    }

    private static State entityState(State returnState) {
        switch (returnState) {
            case DATA:
                return State.ENTITY_DATA;
            case TAG:
                return State.ENTITY_TAG;
            case DOUBLE_QUOTED:
                return State.ENTITY_DOUBLE_QUOTED;
            case SINGLE_QUOTED:
                return State.ENTITY_SINGLE_QUOTED;
            default:
                throw new AssertionError(returnState);
        }
    }

    private static boolean entityBoundary(State entityState, int b) {
        if (b == 0 || b >= 0x80 || b == '&' || isSpace(b)) {
            return true;
        }
        switch (entityState) {
            case ENTITY_DATA:
                return b == '<';
            case ENTITY_TAG:
                return b == '<' || b == '>' || b == '"' || b == '\'';
            case ENTITY_DOUBLE_QUOTED:
                return b == '"';
            case ENTITY_SINGLE_QUOTED:
                return b == '\'';
            default:
                throw new AssertionError(entityState);
        }
    }

    private static boolean isSpace(int b) {
        return b == ' ' || b == '\t' || b == '\n' || b == '\r' || b == '\f';
    }
}

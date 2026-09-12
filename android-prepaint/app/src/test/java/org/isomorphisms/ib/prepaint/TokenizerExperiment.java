package org.isomorphisms.ib.prepaint;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Objects;

/**
 * Narrow byte-level experiment for issue #38. This is not an HTML conformance tokenizer.
 * The scalar path is the semantic oracle; the candidate uses state×byte-class dispatch and
 * bulk-scans only transitions marked APPEND_RUN. UTF-8 is handed off as raw bytes.
 */
final class TokenizerExperiment {
    private TokenizerExperiment() {}

    enum State {
        DATA, TAG, DOUBLE_QUOTED, SINGLE_QUOTED,
        ENTITY_DATA, ENTITY_TAG, ENTITY_DOUBLE_QUOTED, ENTITY_SINGLE_QUOTED
    }

    enum EventKind {
        RUN, TAG_OPEN, TAG_CLOSE,
        DOUBLE_QUOTE_OPEN, DOUBLE_QUOTE_CLOSE,
        SINGLE_QUOTE_OPEN, SINGLE_QUOTE_CLOSE,
        ENTITY, MALFORMED_ENTITY, MALFORMED_LT_IN_TAG,
        NUL, UTF8_HANDOFF, MALFORMED_EOF
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

        @Override
        public boolean equals(Object other) {
            if (!(other instanceof Event)) return false;
            Event event = (Event) other;
            return kind == event.kind && context == event.context && offset == event.offset
                    && Arrays.equals(bytes, event.bytes);
        }

        @Override
        public String toString() {
            StringBuilder text = new StringBuilder();
            for (byte value : bytes) {
                int b = value & 0xff;
                if (b >= 0x20 && b <= 0x7e) text.append((char) b);
                else text.append(String.format("\\x%02x", b));
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

    static Result scalar(byte[] input) { return scalar(List.of(input)); }
    static Result scalar(List<byte[]> chunks) { return run(chunks, false); }
    static Result tableDriven(byte[] input) { return tableDriven(List.of(input)); }
    static Result tableDriven(List<byte[]> chunks) { return run(chunks, true); }

    private static Result run(List<byte[]> chunks, boolean tableDriven) {
        Scanner scanner = new Scanner(tableDriven);
        for (byte[] chunk : chunks) scanner.feed(chunk);
        return scanner.finish();
    }

    private enum Class {
        OTHER, LT, GT, AMP, DOUBLE_QUOTE, SINGLE_QUOTE, SEMICOLON, SPACE, NUL, HIGH
    }

    private enum Action {
        APPEND_RUN, APPEND_ENTITY,
        TAG_OPEN, TAG_CLOSE, START_ENTITY,
        DOUBLE_QUOTE_OPEN, DOUBLE_QUOTE_CLOSE,
        SINGLE_QUOTE_OPEN, SINGLE_QUOTE_CLOSE,
        END_ENTITY, BREAK_ENTITY, MALFORMED_LT, NUL, UTF8
    }

    private static final Class[] BYTE_CLASS = makeByteClasses();
    private static final Action[][] TABLE = makeTable();

    private static final class Scanner {
        final boolean tableDriven;
        final List<Event> events = new ArrayList<>();
        State state = State.DATA;
        State entityReturn;
        int offset;
        int bulkBytes;
        int dispatches;

        State runState;
        int runOffset;
        final ByteArrayOutputStream run = new ByteArrayOutputStream();

        State utf8State;
        int utf8Offset;
        final ByteArrayOutputStream utf8 = new ByteArrayOutputStream();

        int entityOffset;
        final ByteArrayOutputStream entity = new ByteArrayOutputStream();

        Scanner(boolean tableDriven) {
            this.tableDriven = tableDriven;
        }

        void feed(byte[] chunk) {
            int i = 0;
            while (i < chunk.length) {
                if (tableDriven && !isEntity(state)) {
                    int end = scanOrdinaryRun(chunk, i, state);
                    if (end > i) {
                        appendRun(chunk, i, end);
                        bulkBytes += end - i;
                        i = end;
                        if (i == chunk.length) continue;
                    }
                }

                int b = chunk[i] & 0xff;
                Action action = tableDriven
                        ? TABLE[state.ordinal()][BYTE_CLASS[b].ordinal()]
                        : scalarAction(state, b);
                dispatches++;
                if (apply(action, b)) i++;
            }
        }

        Result finish() {
            flushRun();
            flushUtf8();
            if (isEntity(state)) {
                emitEntity(EventKind.MALFORMED_ENTITY);
                state = entityReturn;
            }
            if (state != State.DATA) {
                events.add(new Event(EventKind.MALFORMED_EOF, state, offset, new byte[0]));
            }
            return new Result(events, bulkBytes, dispatches);
        }

        boolean apply(Action action, int b) {
            switch (action) {
                case APPEND_RUN:
                    appendRunByte(b);
                    return true;
                case APPEND_ENTITY:
                    entity.write(b);
                    offset++;
                    return true;
                case TAG_OPEN:
                    emitOne(EventKind.TAG_OPEN, State.DATA, b);
                    state = State.TAG;
                    return true;
                case TAG_CLOSE:
                    emitOne(EventKind.TAG_CLOSE, State.TAG, b);
                    state = State.DATA;
                    return true;
                case START_ENTITY:
                    startEntity();
                    return true;
                case DOUBLE_QUOTE_OPEN:
                    emitOne(EventKind.DOUBLE_QUOTE_OPEN, State.TAG, b);
                    state = State.DOUBLE_QUOTED;
                    return true;
                case DOUBLE_QUOTE_CLOSE:
                    emitOne(EventKind.DOUBLE_QUOTE_CLOSE, State.DOUBLE_QUOTED, b);
                    state = State.TAG;
                    return true;
                case SINGLE_QUOTE_OPEN:
                    emitOne(EventKind.SINGLE_QUOTE_OPEN, State.TAG, b);
                    state = State.SINGLE_QUOTED;
                    return true;
                case SINGLE_QUOTE_CLOSE:
                    emitOne(EventKind.SINGLE_QUOTE_CLOSE, State.SINGLE_QUOTED, b);
                    state = State.TAG;
                    return true;
                case END_ENTITY:
                    entity.write(b);
                    offset++;
                    emitEntity(EventKind.ENTITY);
                    state = entityReturn;
                    return true;
                case BREAK_ENTITY:
                    emitEntity(EventKind.MALFORMED_ENTITY);
                    state = entityReturn;
                    return false;
                case MALFORMED_LT:
                    emitOne(EventKind.MALFORMED_LT_IN_TAG, State.TAG, b);
                    return true;
                case NUL:
                    emitOne(EventKind.NUL, state, b);
                    return true;
                case UTF8:
                    appendUtf8(b);
                    return true;
                default:
                    throw new AssertionError(action);
            }
        }

        void appendRun(byte[] input, int start, int end) {
            flushUtf8();
            beginRun();
            run.write(input, start, end - start);
            offset += end - start;
        }

        void appendRunByte(int b) {
            flushUtf8();
            beginRun();
            run.write(b);
            offset++;
        }

        void beginRun() {
            if (run.size() == 0) {
                runState = state;
                runOffset = offset;
            } else if (runState != state) {
                flushRun();
                runState = state;
                runOffset = offset;
            }
        }

        void appendUtf8(int b) {
            flushRun();
            if (utf8.size() == 0) {
                utf8State = state;
                utf8Offset = offset;
            } else if (utf8State != state) {
                flushUtf8();
                utf8State = state;
                utf8Offset = offset;
            }
            utf8.write(b);
            offset++;
        }

        void startEntity() {
            flushRun();
            flushUtf8();
            entityReturn = state;
            entityOffset = offset;
            entity.reset();
            entity.write('&');
            offset++;
            state = entityState(entityReturn);
        }

        void emitOne(EventKind kind, State context, int b) {
            flushRun();
            flushUtf8();
            events.add(new Event(kind, context, offset, new byte[] {(byte) b}));
            offset++;
        }

        void emitEntity(EventKind kind) {
            flushRun();
            flushUtf8();
            events.add(new Event(kind, entityReturn, entityOffset, entity.toByteArray()));
            entity.reset();
        }

        void flushRun() {
            if (run.size() == 0) return;
            events.add(new Event(EventKind.RUN, runState, runOffset, run.toByteArray()));
            run.reset();
        }

        void flushUtf8() {
            if (utf8.size() == 0) return;
            events.add(new Event(EventKind.UTF8_HANDOFF, utf8State, utf8Offset, utf8.toByteArray()));
            utf8.reset();
        }
    }

    private static Action scalarAction(State state, int b) {
        if (isEntity(state)) {
            if (b == ';') return Action.END_ENTITY;
            if (entityBoundary(state, b)) return Action.BREAK_ENTITY;
            return Action.APPEND_ENTITY;
        }
        if (b == 0) return Action.NUL;
        if (b >= 0x80) return Action.UTF8;
        switch (state) {
            case DATA:
                if (b == '<') return Action.TAG_OPEN;
                if (b == '&') return Action.START_ENTITY;
                return Action.APPEND_RUN;
            case TAG:
                if (b == '>') return Action.TAG_CLOSE;
                if (b == '&') return Action.START_ENTITY;
                if (b == '"') return Action.DOUBLE_QUOTE_OPEN;
                if (b == '\'') return Action.SINGLE_QUOTE_OPEN;
                if (b == '<') return Action.MALFORMED_LT;
                return Action.APPEND_RUN;
            case DOUBLE_QUOTED:
                if (b == '"') return Action.DOUBLE_QUOTE_CLOSE;
                if (b == '&') return Action.START_ENTITY;
                return Action.APPEND_RUN;
            case SINGLE_QUOTED:
                if (b == '\'') return Action.SINGLE_QUOTE_CLOSE;
                if (b == '&') return Action.START_ENTITY;
                return Action.APPEND_RUN;
            default:
                throw new AssertionError(state);
        }
    }

    private static int scanOrdinaryRun(byte[] input, int start, State state) {
        int i = start;
        while (i < input.length) {
            int b = input[i] & 0xff;
            if (TABLE[state.ordinal()][BYTE_CLASS[b].ordinal()] != Action.APPEND_RUN) break;
            i++;
        }
        return i;
    }

    private static Class[] makeByteClasses() {
        Class[] classes = new Class[256];
        Arrays.fill(classes, Class.OTHER);
        for (int i = 0x80; i < 256; i++) classes[i] = Class.HIGH;
        classes[0] = Class.NUL;
        classes['<'] = Class.LT;
        classes['>'] = Class.GT;
        classes['&'] = Class.AMP;
        classes['"'] = Class.DOUBLE_QUOTE;
        classes['\''] = Class.SINGLE_QUOTE;
        classes[';'] = Class.SEMICOLON;
        for (int b : new int[] {' ', '\t', '\n', '\r', '\f'}) classes[b] = Class.SPACE;
        return classes;
    }

    private static Action[][] makeTable() {
        Action[][] table = new Action[State.values().length][Class.values().length];
        for (Action[] row : table) Arrays.fill(row, Action.APPEND_RUN);

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
            for (Class boundary : List.of(Class.NUL, Class.HIGH, Class.AMP, Class.SPACE)) {
                set(table, state, boundary, Action.BREAK_ENTITY);
            }
        }
        set(table, State.ENTITY_DATA, Class.LT, Action.BREAK_ENTITY);
        for (Class boundary : List.of(Class.LT, Class.GT, Class.DOUBLE_QUOTE, Class.SINGLE_QUOTE)) {
            set(table, State.ENTITY_TAG, boundary, Action.BREAK_ENTITY);
        }
        set(table, State.ENTITY_DOUBLE_QUOTED, Class.DOUBLE_QUOTE, Action.BREAK_ENTITY);
        set(table, State.ENTITY_SINGLE_QUOTED, Class.SINGLE_QUOTE, Action.BREAK_ENTITY);
        return table;
    }

    private static void set(Action[][] table, State state, Class clazz, Action action) {
        table[state.ordinal()][clazz.ordinal()] = action;
    }

    private static boolean isEntity(State state) {
        return state == State.ENTITY_DATA || state == State.ENTITY_TAG
                || state == State.ENTITY_DOUBLE_QUOTED || state == State.ENTITY_SINGLE_QUOTED;
    }

    private static State entityState(State returnState) {
        switch (returnState) {
            case DATA: return State.ENTITY_DATA;
            case TAG: return State.ENTITY_TAG;
            case DOUBLE_QUOTED: return State.ENTITY_DOUBLE_QUOTED;
            case SINGLE_QUOTED: return State.ENTITY_SINGLE_QUOTED;
            default: throw new AssertionError(returnState);
        }
    }

    private static boolean entityBoundary(State state, int b) {
        if (b == 0 || b >= 0x80 || b == '&' || isSpace(b)) return true;
        switch (state) {
            case ENTITY_DATA: return b == '<';
            case ENTITY_TAG: return b == '<' || b == '>' || b == '"' || b == '\'';
            case ENTITY_DOUBLE_QUOTED: return b == '"';
            case ENTITY_SINGLE_QUOTED: return b == '\'';
            default: throw new AssertionError(state);
        }
    }

    private static boolean isSpace(int b) {
        return b == ' ' || b == '\t' || b == '\n' || b == '\r' || b == '\f';
    }
}

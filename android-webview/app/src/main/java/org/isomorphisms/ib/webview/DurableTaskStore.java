package org.isomorphisms.ib.webview;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.nio.file.AtomicMoveNotSupportedException;
import java.nio.file.DirectoryIteratorException;
import java.nio.file.DirectoryStream;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.NoSuchFileException;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.nio.file.StandardOpenOption;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.UUID;
import java.util.regex.Pattern;

/** App-private ordinary-state persistence for explicitly protected tasks only. */
final class DurableTaskStore {
    enum TabProtection {
        ORDINARY,
        PROTECTED_AUTHENTICATED_TRANSACTION
    }

    // Bounds apply to encoded bytes and directory entries, before parsing or sorting.
    static final int MAX_RECORD_BYTES = 64 * 1024;
    static final int MAX_TASK_ENTRIES = 1024;
    static final int MAX_HISTORY_BYTES = 4 * 1024 * 1024;
    private static final Pattern PATH_IDENTITY = Pattern.compile("[A-Za-z0-9._-]{1,128}");
    private final Path files_root;
    private final Path state_root;

    static final class DiscoveryFailure {
        final String task_id;
        final String reason;

        DiscoveryFailure(String task_id, String reason) {
            this.task_id = task_id;
            this.reason = reason;
        }
    }

    static final class Discovery {
        final List<DurableTaskRecord> records;
        final List<DiscoveryFailure> failures;
        final DurableTaskRecord latest;

        private Discovery(
            List<DurableTaskRecord> records,
            List<DiscoveryFailure> failures,
            DurableTaskRecord latest
        ) {
            this.records = Collections.unmodifiableList(new ArrayList<>(records));
            this.failures = Collections.unmodifiableList(new ArrayList<>(failures));
            this.latest = latest;
        }
    }

    DurableTaskStore(Path files_root) {
        this.files_root = files_root.toAbsolutePath().normalize();
        state_root = this.files_root.resolve("state");
    }

    void save(DurableTaskRecord record, TabProtection protection) throws IOException {
        if (protection != TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION) {
            throw new IllegalArgumentException("ordinary tabs remain renderer-lifetime state");
        }
        // Validate both identities and payloads before creating either hierarchy.
        Path task_directory = task_directory(record.task_id);
        Path tab_directory = tab_directory(record.tab_id);
        byte[] task_bytes = bounded_text(record.serialize(), MAX_RECORD_BYTES);
        byte[] tab_bytes = bounded_text(tab_manifest(record), MAX_RECORD_BYTES);
        create_store_directory(task_directory);
        create_store_directory(tab_directory);
        atomic_write(task_directory.resolve("task.txt"), task_bytes);
        atomic_write(tab_directory.resolve("tab.txt"), tab_bytes);
    }

    void append_navigation(DurableTaskRecord record) throws IOException {
        path_identity(record.task_id);
        Path tab_directory = tab_directory(record.tab_id);
        String line = record.navigation_id + "\t"
            + record.navigation.recovery.record_text + "\t"
            + record.navigation.neutral_url + "\n";
        byte[] bytes = bounded_text(line, MAX_RECORD_BYTES);
        create_store_directory(tab_directory);
        Path destination = tab_directory.resolve("history.log");
        check_regular_destination(destination);
        try (FileChannel output = FileChannel.open(
            destination, StandardOpenOption.CREATE, StandardOpenOption.WRITE,
            StandardOpenOption.APPEND, LinkOption.NOFOLLOW_LINKS
        )) {
            if (output.size() > MAX_HISTORY_BYTES - bytes.length) {
                throw new IOException("durable navigation journal exceeds byte limit");
            }
            write_all(output, bytes);
            output.force(true);
        }
    }

    /** All valid records, ordered by task identity, with individual failures retained. */
    Discovery discover() throws IOException {
        Path tasks = state_root.resolve("tasks");
        if (!existing_store_directory(tasks)) {
            return new Discovery(Collections.emptyList(), Collections.emptyList(), null);
        }
        List<Path> entries = new ArrayList<>();
        try (DirectoryStream<Path> directory = Files.newDirectoryStream(tasks)) {
            for (Path entry : directory) {
                if (entries.size() == MAX_TASK_ENTRIES) {
                    // Never report a bounded subset as complete discovery.
                    throw new IOException("durable task discovery exceeds entry limit");
                }
                entries.add(entry);
            }
        } catch (DirectoryIteratorException exception) {
            throw new IOException("durable task directory enumeration failed", exception);
        }
        entries.sort(Comparator.comparing(path -> path.getFileName().toString()));
        List<DurableTaskRecord> records = new ArrayList<>();
        List<DiscoveryFailure> failures = new ArrayList<>();
        DurableTaskRecord latest = null;
        long latest_modified = Long.MIN_VALUE;
        for (Path entry : entries) {
            String identity = entry.getFileName().toString();
            try {
                path_identity(identity);
                if (!existing_store_directory(entry)) {
                    throw new IOException("task entry disappeared during discovery");
                }
                Path record_path = entry.resolve("task.txt");
                DurableTaskRecord record = DurableTaskRecord.parse(read_bounded(record_path));
                if (!identity.equals(record.task_id)) {
                    throw new IllegalArgumentException("task record identity differs from directory");
                }
                path_identity(record.tab_id);
                long modified = Files.getLastModifiedTime(
                    record_path, LinkOption.NOFOLLOW_LINKS
                ).toMillis();
                records.add(record);
                // Stable identity order also makes equal-timestamp selection deterministic.
                if (latest == null || modified > latest_modified) {
                    latest = record;
                    latest_modified = modified;
                }
            } catch (IOException | IllegalArgumentException exception) {
                String label = PATH_IDENTITY.matcher(identity).matches()
                    ? identity : "invalid-path-identity";
                // Refusal receipts must not echo attacker-controlled serialized fields
                // or OS exception paths into ordinary task logs.
                String reason = exception instanceof IllegalArgumentException
                    ? "invalid-record-or-identity" : "unreadable-or-refused-record";
                failures.add(new DiscoveryFailure(label, reason));
            }
        }
        return new Discovery(records, failures, latest);
    }

    // Compatibility selection for the current one-task harness. Consumers needing
    // complete discovery and diagnostics must use discover(), not this projection.
    DurableTaskRecord discover_latest() throws IOException {
        return discover().latest;
    }

    String read_task_record(String task_id) throws IOException {
        return read_bounded(task_directory(task_id).resolve("task.txt"));
    }

    private Path task_directory(String task_id) {
        return state_root.resolve("tasks").resolve(path_identity(task_id));
    }

    private Path tab_directory(String tab_id) {
        return state_root.resolve("tabs").resolve(path_identity(tab_id));
    }

    private static String path_identity(String value) {
        if (value == null || value.equals(".") || value.equals("..")
            || !PATH_IDENTITY.matcher(value).matches()) {
            throw new IllegalArgumentException("durable identity is not a safe path component");
        }
        return value;
    }

    /** Trust the Android-provided root; reject links in every IB-owned child. */
    private boolean existing_store_directory(Path directory) throws IOException {
        if (!directory.startsWith(files_root)) {
            throw new IOException("durable path is outside selected files root");
        }
        // Android owns getFilesDir() and its parent path. On real devices an
        // ancestor such as /data/data can be a platform-managed symlink. Follow
        // those ancestors only to verify the trusted app-private root exists.
        // Never follow links in the IB-owned state tree below that root.
        BasicFileAttributes root_attributes;
        try {
            root_attributes = Files.readAttributes(files_root, BasicFileAttributes.class);
        } catch (NoSuchFileException exception) {
            return false;
        }
        if (!root_attributes.isDirectory()) {
            throw new IOException("selected files root is not a directory");
        }

        Path current = files_root;
        for (Path component : files_root.relativize(directory)) {
            current = current.resolve(component);
            BasicFileAttributes attributes;
            try {
                attributes = Files.readAttributes(
                    current, BasicFileAttributes.class, LinkOption.NOFOLLOW_LINKS
                );
            } catch (NoSuchFileException exception) {
                // Absence is the only condition that permits an empty discovery.
                // Access denial and other observation failures stay explicit.
                return false;
            }
            if (!attributes.isDirectory() || attributes.isSymbolicLink()) {
                throw new IOException("durable directory is not a non-linked directory");
            }
        }
        return true;
    }

    private void create_store_directory(Path directory) throws IOException {
        if (!existing_store_directory(files_root)) {
            throw new IOException("selected files root does not exist");
        }
        Path current = files_root;
        for (Path component : files_root.relativize(directory)) {
            current = current.resolve(component);
            if (!Files.exists(current, LinkOption.NOFOLLOW_LINKS)) {
                Files.createDirectory(current);
            }
            if (!existing_store_directory(current)) {
                throw new IOException("durable directory disappeared during creation");
            }
        }
    }

    private void check_regular_destination(Path destination) throws IOException {
        if (!existing_store_directory(destination.getParent())) {
            throw new IOException("durable parent directory is missing");
        }
        if (Files.exists(destination, LinkOption.NOFOLLOW_LINKS)) {
            BasicFileAttributes attributes = Files.readAttributes(
                destination, BasicFileAttributes.class, LinkOption.NOFOLLOW_LINKS
            );
            if (!attributes.isRegularFile() || attributes.isSymbolicLink()) {
                throw new IOException("durable record is not a non-linked regular file");
            }
        }
    }

    private String read_bounded(Path record_path) throws IOException {
        check_regular_destination(record_path);
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        try (FileChannel input = FileChannel.open(
            record_path, StandardOpenOption.READ, LinkOption.NOFOLLOW_LINKS
        )) {
            if (input.size() > MAX_RECORD_BYTES) {
                throw new IOException("durable task record exceeds byte limit");
            }
            ByteBuffer buffer = ByteBuffer.allocate(4096);
            while (true) {
                int count = input.read(buffer);
                if (count < 0) {
                    break;
                }
                if (count > MAX_RECORD_BYTES - bytes.size()) {
                    throw new IOException("durable task record exceeds byte limit");
                }
                bytes.write(buffer.array(), 0, count);
                buffer.clear();
            }
        }
        String text;
        try {
            text = StandardCharsets.UTF_8.newDecoder()
                .onMalformedInput(CodingErrorAction.REPORT)
                .onUnmappableCharacter(CodingErrorAction.REPORT)
                .decode(ByteBuffer.wrap(bytes.toByteArray())).toString();
        } catch (CharacterCodingException exception) {
            throw new IOException("durable task record is not valid UTF-8", exception);
        }
        if (!text.endsWith("\n")) {
            throw new IOException("durable task record has an incomplete final line");
        }
        return text;
    }

    private static byte[] bounded_text(String text, int limit) throws IOException {
        byte[] bytes = text.getBytes(StandardCharsets.UTF_8);
        if (bytes.length > limit) {
            throw new IOException("durable output exceeds byte limit");
        }
        return bytes;
    }

    private static String tab_manifest(DurableTaskRecord record) {
        return "schema\tib-tab-v1\n"
            + "id\t" + record.tab_id + "\n"
            + "task_id\t" + record.task_id + "\n"
            + "state\tprotected\n"
            + "current_event\t" + record.navigation_id + "\n"
            + "priority\tprotected-authenticated-transaction\n"
            + "renderer\tauto\n";
    }

    private void atomic_write(Path destination, byte[] bytes) throws IOException {
        check_regular_destination(destination);
        Path temporary = destination.resolveSibling(
            destination.getFileName() + "." + UUID.randomUUID() + ".tmp"
        );
        try {
            try (FileChannel output = FileChannel.open(
                temporary, StandardOpenOption.CREATE_NEW, StandardOpenOption.WRITE,
                LinkOption.NOFOLLOW_LINKS
            )) {
                write_all(output, bytes);
                output.force(true);
            }
            check_regular_destination(destination);
            Files.move(
                temporary, destination, StandardCopyOption.ATOMIC_MOVE,
                StandardCopyOption.REPLACE_EXISTING
            );
        } catch (AtomicMoveNotSupportedException exception) {
            throw new IOException("durable task store requires an atomic same-directory rename", exception);
        } finally {
            Files.deleteIfExists(temporary);
        }
    }

    private static void write_all(FileChannel output, byte[] bytes) throws IOException {
        ByteBuffer buffer = ByteBuffer.wrap(bytes);
        while (buffer.hasRemaining()) {
            output.write(buffer);
        }
    }
}

package org.isomorphisms.ib.webview;

import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.AtomicMoveNotSupportedException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.Comparator;
import java.util.UUID;
import java.util.regex.Pattern;
import java.util.stream.Stream;

/** App-private ordinary-state persistence for explicitly protected tasks only. */
final class DurableTaskStore {
    enum TabProtection {
        ORDINARY,
        PROTECTED_AUTHENTICATED_TRANSACTION
    }

    private static final Pattern PATH_IDENTITY = Pattern.compile("[A-Za-z0-9._-]{1,128}");

    private final Path state_root;

    DurableTaskStore(Path files_root) {
        state_root = files_root.resolve("state");
    }

    void save(DurableTaskRecord record, TabProtection protection) throws IOException {
        if (protection != TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION) {
            throw new IllegalArgumentException("ordinary tabs remain renderer-lifetime state");
        }
        Path task_directory = task_directory(record.task_id);
        Path tab_directory = tab_directory(record.tab_id);
        Files.createDirectories(task_directory);
        Files.createDirectories(tab_directory);
        atomic_write(task_directory.resolve("task.txt"), record.serialize());
        atomic_write(tab_directory.resolve("tab.txt"), tab_manifest(record));
    }

    void append_navigation(DurableTaskRecord record) throws IOException {
        Path tab_directory = tab_directory(record.tab_id);
        Files.createDirectories(tab_directory);
        String line = record.navigation_id + "\t"
            + record.navigation.recovery.record_text + "\t"
            + record.navigation.neutral_url + "\n";
        append_and_sync(tab_directory.resolve("history.log"), line);
    }

    DurableTaskRecord discover_latest() throws IOException {
        Path tasks = state_root.resolve("tasks");
        if (!Files.isDirectory(tasks)) {
            return null;
        }
        Path latest;
        try (Stream<Path> entries = Files.list(tasks)) {
            latest = entries
                .filter(Files::isDirectory)
                .map(path -> path.resolve("task.txt"))
                .filter(Files::isRegularFile)
                .max(Comparator.comparingLong(DurableTaskStore::last_modified_or_zero))
                .orElse(null);
        }
        if (latest == null) {
            return null;
        }
        String text = new String(Files.readAllBytes(latest), StandardCharsets.UTF_8);
        try {
            return DurableTaskRecord.parse(text);
        } catch (IllegalArgumentException exception) {
            throw new IOException("latest durable task record is invalid", exception);
        }
    }

    String read_task_record(String task_id) throws IOException {
        return new String(
            Files.readAllBytes(task_directory(task_id).resolve("task.txt")),
            StandardCharsets.UTF_8
        );
    }

    private Path task_directory(String task_id) {
        return state_root.resolve("tasks").resolve(path_identity(task_id));
    }

    private Path tab_directory(String tab_id) {
        return state_root.resolve("tabs").resolve(path_identity(tab_id));
    }

    private static String path_identity(String value) {
        if (!PATH_IDENTITY.matcher(value).matches()) {
            throw new IllegalArgumentException("durable identity is not a safe path component");
        }
        return value;
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

    private static void atomic_write(Path destination, String text) throws IOException {
        Path temporary = destination.resolveSibling(
            destination.getFileName() + "." + UUID.randomUUID() + ".tmp"
        );
        byte[] bytes = text.getBytes(StandardCharsets.UTF_8);
        try {
            try (FileOutputStream output = new FileOutputStream(temporary.toFile())) {
                output.write(bytes);
                output.getFD().sync();
            }
            Files.move(
                temporary,
                destination,
                StandardCopyOption.ATOMIC_MOVE,
                StandardCopyOption.REPLACE_EXISTING
            );
        } catch (AtomicMoveNotSupportedException exception) {
            throw new IOException("durable task store requires an atomic same-directory rename", exception);
        } finally {
            Files.deleteIfExists(temporary);
        }
    }

    private static void append_and_sync(Path destination, String text) throws IOException {
        try (FileOutputStream output = new FileOutputStream(destination.toFile(), true)) {
            output.write(text.getBytes(StandardCharsets.UTF_8));
            output.getFD().sync();
        }
    }

    private static long last_modified_or_zero(Path path) {
        try {
            return Files.getLastModifiedTime(path).toMillis();
        } catch (IOException exception) {
            return 0;
        }
    }
}

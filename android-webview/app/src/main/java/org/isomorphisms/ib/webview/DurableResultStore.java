package org.isomorphisms.ib.webview;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.AtomicMoveNotSupportedException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.LinkOption;
import java.nio.file.attribute.PosixFilePermissions;
import java.nio.file.StandardCopyOption;
import java.util.Arrays;
import java.util.UUID;

/** Small app-private immutable result store shared by Longview and provider experiments. */
public final class DurableResultStore {
    public static final String RESULT_ID = "hello-v1";
    public static final byte[] RESULT_BYTES = "hello\n".getBytes(StandardCharsets.UTF_8);
    public static final int MAX_RESULT_BYTES = 4096;

    private final Path root;
    private final PublicationIO publication_io;

    // Narrow filesystem seam for deterministic schedules and I/O faults.
    // It has no admission, identity, retry, or fallback policy.
    interface PublicationIO {
        void stage(Path temporary, byte[] bytes) throws IOException;
        void publish(Path temporary, Path target) throws IOException;
    }

    static final PublicationIO FILESYSTEM = new PublicationIO() {
        public void stage(Path temporary, byte[] bytes) throws IOException {
            write_synced(temporary, bytes);
        }

        public void publish(Path temporary, Path target) throws IOException {
            Files.createLink(target, temporary);
        }
    };

    public DurableResultStore(Path files_root) {
        this(files_root, FILESYSTEM);
    }

    DurableResultStore(Path files_root, PublicationIO publication_io) {
        root = files_root.resolve("durable-results");
        this.publication_io = publication_io;
    }

    public Path commit_fixture() throws IOException {
        return commit_immutable(RESULT_ID, RESULT_BYTES);
    }

    public Path commit_immutable(String result_id, byte[] bytes) throws IOException {
        validate_result_id(result_id);
        if (bytes == null) {
            throw new IllegalArgumentException("durable result bytes are required");
        }
        if (bytes.length > MAX_RESULT_BYTES) {
            throw new IllegalArgumentException("durable result exceeds byte limit");
        }

        // The caller retains its mutable array. Publication owns one bounded
        // snapshot, including the bytes used to verify an idempotent retry.
        byte[] snapshot = Arrays.copyOf(bytes, bytes.length);
        Files.createDirectories(root);
        Path target = result_path(result_id);
        if (Files.exists(target)) {
            verify_equal(target, snapshot);
            return target;
        }

        Path temporary = root.resolve("." + result_id + "." + UUID.randomUUID() + ".tmp");
        try {
            publication_io.stage(temporary, snapshot);
            Files.setPosixFilePermissions(temporary, PosixFilePermissions.fromString("r--------"));
            try {
                // ATOMIC_MOVE may replace an existing target even without
                // REPLACE_EXISTING. A hard link is atomic create-if-absent.
                publication_io.publish(temporary, target);
            } catch (java.nio.file.FileAlreadyExistsException exception) {
                // Verify the winner below; unequal writers must lose.
            }
            verify_equal(target, snapshot);
            return target;
        } finally {
            Files.deleteIfExists(temporary);
        }
    }

    public Path require_result(String result_id) throws IOException {
        Path result = result_path(result_id);
        if (!Files.isRegularFile(result, LinkOption.NOFOLLOW_LINKS)) {
            throw new IOException("durable result is not committed: " + result_id);
        }
        if (Files.size(result) > MAX_RESULT_BYTES) {
            throw new IOException("durable result exceeds byte limit");
        }
        return result;
    }

    public byte[] read_bounded(String result_id, int maximum_bytes) throws IOException {
        if (maximum_bytes < 0 || maximum_bytes > MAX_RESULT_BYTES) {
            throw new IllegalArgumentException("invalid durable reader limit");
        }
        return read_path_bounded(require_result(result_id), maximum_bytes);
    }

    public long next_provider_generation() throws IOException {
        Files.createDirectories(root);
        Path generation_path = root.resolve("provider-generation");
        long previous = 0;
        if (Files.exists(generation_path)) {
            String text = new String(
                Files.readAllBytes(generation_path),
                StandardCharsets.US_ASCII
            ).trim();
            if (!text.isEmpty()) {
                previous = Long.parseLong(text);
            }
        }

        long next = Math.addExact(previous, 1);
        Path temporary = root.resolve(".provider-generation." + UUID.randomUUID() + ".tmp");
        write_synced(
            temporary,
            (Long.toString(next) + "\n").getBytes(StandardCharsets.US_ASCII)
        );
        move_atomically(temporary, generation_path, true);
        return next;
    }

    private Path result_path(String result_id) {
        validate_result_id(result_id);
        return root.resolve(result_id + ".txt");
    }

    private static void validate_result_id(String result_id) {
        if (result_id == null
            || result_id.isEmpty()
            || ".".equals(result_id)
            || "..".equals(result_id)
            || !result_id.matches("[A-Za-z0-9._-]+")) {
            throw new IllegalArgumentException("invalid durable result id");
        }
    }

    private static void verify_equal(Path target, byte[] expected) throws IOException {
        byte[] actual = read_path_bounded(target, MAX_RESULT_BYTES);
        if (!Arrays.equals(actual, expected)) {
            throw new IOException("existing durable result differs from committed bytes");
        }
    }

    private static byte[] read_path_bounded(Path target, int maximum_bytes) throws IOException {
        if (!Files.isRegularFile(target, LinkOption.NOFOLLOW_LINKS)
            || Files.size(target) > maximum_bytes) {
            throw new IOException("durable reader limit exceeded or result absent");
        }
        try (InputStream input = Files.newInputStream(target);
             ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            // Read one extra byte to detect growth, but never emit a prefix.
            byte[] buffer = new byte[maximum_bytes + 1];
            int count;
            while ((count = input.read(buffer, 0, buffer.length - output.size())) != -1) {
                output.write(buffer, 0, count);
                if (output.size() > maximum_bytes) {
                    throw new IOException("durable reader limit exceeded");
                }
            }
            return output.toByteArray();
        }
    }

    private static void write_synced(Path path, byte[] bytes) throws IOException {
        try (FileOutputStream output = new FileOutputStream(path.toFile())) {
            output.write(bytes);
            output.getFD().sync();
        }
    }

    private static void move_atomically(Path from, Path to, boolean replace) throws IOException {
        try {
            if (replace) {
                Files.move(
                    from,
                    to,
                    StandardCopyOption.ATOMIC_MOVE,
                    StandardCopyOption.REPLACE_EXISTING
                );
            } else {
                Files.move(from, to, StandardCopyOption.ATOMIC_MOVE);
            }
        } catch (AtomicMoveNotSupportedException exception) {
            if (replace) {
                Files.move(from, to, StandardCopyOption.REPLACE_EXISTING);
            } else {
                Files.move(from, to);
            }
        }
    }
}

package org.isomorphisms.ib.webview;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import org.junit.Test;

public final class DurableResultStoreTest {
    private static byte[] bytes(String value) {
        return value.getBytes(StandardCharsets.UTF_8);
    }

    private static void await(CountDownLatch latch) throws IOException {
        try {
            if (!latch.await(5, TimeUnit.SECONDS)) {
                throw new IOException("publication schedule timed out");
            }
        } catch (InterruptedException exception) {
            Thread.currentThread().interrupt();
            throw new IOException("publication schedule interrupted", exception);
        }
    }

    private static void concurrent_publication(boolean equal) throws Exception {
        Path root = Files.createTempDirectory("ib-durable-race");
        CountDownLatch staged = new CountDownLatch(2);
        CountDownLatch release = new CountDownLatch(1);
        DurableResultStore.PublicationIO scheduled = new DurableResultStore.PublicationIO() {
            public void stage(Path temporary, byte[] bytes) throws IOException {
                DurableResultStore.FILESYSTEM.stage(temporary, bytes);
            }

            public void publish(Path temporary, Path target) throws IOException {
                staged.countDown();
                await(release);
                DurableResultStore.FILESYSTEM.publish(temporary, target);
            }
        };
        byte[] first_bytes = bytes("first\n");
        byte[] second_bytes = equal ? first_bytes : bytes("unequal second\n");
        ExecutorService writers = Executors.newFixedThreadPool(2);
        try {
            Future<Boolean> first = writers.submit(() -> commit_outcome(root, scheduled, first_bytes));
            Future<Boolean> second = writers.submit(() -> commit_outcome(root, scheduled, second_bytes));
            await(staged);
            // Real staged inodes exist, but no result can be opened yet.
            try {
                new DurableResultStore(root).require_result("race");
                fail("interrupted/stalled staging became visible");
            } catch (IOException expected_absent) {
                assertTrue(expected_absent.getMessage().contains("not committed"));
            }
            release.countDown();
            boolean first_won = first.get(5, TimeUnit.SECONDS);
            boolean second_won = second.get(5, TimeUnit.SECONDS);
            assertEquals(equal ? 2 : 1, (first_won ? 1 : 0) + (second_won ? 1 : 0));
            byte[] winner = first_won ? first_bytes : second_bytes;
            for (int read = 0; read < 3; read++) {
                assertArrayEquals(winner, new DurableResultStore(root).read_bounded("race", 64));
            }
            try (java.util.stream.Stream<Path> paths = Files.list(root.resolve("durable-results"))) {
                assertEquals(1, paths.count());
            }
        } finally {
            release.countDown();
            writers.shutdownNow();
        }
    }

    private static boolean commit_outcome(
        Path root, DurableResultStore.PublicationIO io, byte[] bytes
    ) throws IOException {
        try {
            new DurableResultStore(root, io).commit_immutable("race", bytes);
            return true;
        } catch (IOException exception) {
            if (!exception.getMessage().equals("existing durable result differs from committed bytes")) {
                throw exception;
            }
            return false;
        }
    }

    @Test(timeout = 15000)
    public void concurrent_unequal_writers_cannot_replace_winner() throws Exception {
        concurrent_publication(false);
    }

    @Test(timeout = 15000)
    public void concurrent_equal_writers_are_idempotent() throws Exception {
        concurrent_publication(true);
    }

    @Test
    public void staging_io_failure_never_commits_partial_bytes() throws Exception {
        Path root = Files.createTempDirectory("ib-durable-no-space");
        DurableResultStore.PublicationIO no_space = new DurableResultStore.PublicationIO() {
            public void stage(Path temporary, byte[] bytes) throws IOException {
                Files.write(temporary, new byte[] { bytes[0] });
                throw new java.nio.file.FileSystemException(
                    temporary.toString(), null, "No space left on device (ENOSPC)"
                );
            }

            public void publish(Path temporary, Path target) {
                fail("failed staging invoked publication");
            }
        };
        try {
            new DurableResultStore(root, no_space).commit_immutable("no-space", bytes("complete\n"));
            fail("partial staging was accepted");
        } catch (IOException expected_no_space) {
            assertTrue(expected_no_space.getMessage().contains("ENOSPC"));
        }
        assertFalse(Files.exists(root.resolve("durable-results/no-space.txt")));
        try (java.util.stream.Stream<Path> paths = Files.list(root.resolve("durable-results"))) {
            assertEquals(0, paths.count());
        }
    }

    @Test
    public void bounded_reader_refuses_and_committed_snapshot_is_immutable() throws Exception {
        Path root = Files.createTempDirectory("ib-durable-bounded");
        byte[] source = bytes("original\n");
        byte[] expected = source.clone();
        DurableResultStore.PublicationIO changed_source = new DurableResultStore.PublicationIO() {
            public void stage(Path temporary, byte[] snapshot) throws IOException {
                source[0] = 'X';
                DurableResultStore.FILESYSTEM.stage(temporary, snapshot);
            }

            public void publish(Path temporary, Path target) throws IOException {
                DurableResultStore.FILESYSTEM.publish(temporary, target);
            }
        };
        new DurableResultStore(root, changed_source).commit_immutable("snapshot", source);
        DurableResultStore reader = new DurableResultStore(root);
        assertArrayEquals(expected, reader.read_bounded("snapshot", 64));
        try {
            reader.read_bounded("snapshot", 2);
            fail("bounded reader returned partial output");
        } catch (IOException expected_limit) {
            assertTrue(expected_limit.getMessage().contains("limit"));
        }
        assertArrayEquals(expected, reader.read_bounded("snapshot", 64));
    }

    @Test
    public void fixture_is_immutable_and_reopenable() throws Exception {
        Path root = Files.createTempDirectory("ib-durable-result");
        DurableResultStore first = new DurableResultStore(root);
        Path result = first.commit_fixture();

        assertArrayEquals(DurableResultStore.RESULT_BYTES, Files.readAllBytes(result));

        DurableResultStore second = new DurableResultStore(root);
        assertEquals(result, second.commit_fixture());
        assertArrayEquals(
            DurableResultStore.RESULT_BYTES,
            Files.readAllBytes(second.require_result(DurableResultStore.RESULT_ID))
        );
    }

    @Test
    public void bounded_named_results_are_immutable_and_reopenable() throws Exception {
        Path root = Files.createTempDirectory("ib-durable-named-result");
        DurableResultStore store = new DurableResultStore(root);
        byte[] expected = "schema\tib-longview-useful-v1\nstatus\tuseful\n"
            .getBytes(StandardCharsets.UTF_8);

        Path result = store.commit_immutable("longview-heavy-v1", expected);
        assertArrayEquals(expected, Files.readAllBytes(result));
        assertEquals(result, store.commit_immutable("longview-heavy-v1", expected));

        try {
            store.commit_immutable(
                "longview-heavy-v1",
                "different\n".getBytes(StandardCharsets.UTF_8)
            );
            fail("conflicting result bytes must be rejected");
        } catch (IOException expected_conflict) {
            // Expected: immutable result identity cannot silently change bytes.
        }

        try {
            store.commit_immutable("../escape", expected);
            fail("unsafe result id must be rejected");
        } catch (IllegalArgumentException expected_invalid_id) {
            // Expected.
        }
    }

    @Test
    public void provider_generation_survives_store_reconstruction() throws Exception {
        Path root = Files.createTempDirectory("ib-durable-generation");

        assertEquals(1, new DurableResultStore(root).next_provider_generation());
        assertEquals(2, new DurableResultStore(root).next_provider_generation());
        assertEquals(3, new DurableResultStore(root).next_provider_generation());
    }
}

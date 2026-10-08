package org.isomorphisms.ib.webview;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.attribute.FileTime;
import java.util.Arrays;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

public final class DurableTaskRecordTest {
    @Rule
    public final TemporaryFolder temporary = new TemporaryFolder();

    @Test
    public void renderer_replacement_preserves_browser_owned_identities() {
        DurableTaskRecord live = fixture().attach_initial_renderer("renderer-1", 2);
        DurableTaskRecord replacement = live
            .renderer_was_lost(3)
            .attach_replacement_renderer("renderer-2", 4);

        assertEquals(live.task_id, replacement.task_id);
        assertEquals(live.tab_id, replacement.tab_id);
        assertEquals(live.navigation_id, replacement.navigation_id);
        assertEquals(DurableTaskRecord.Continuity.RENDERER_REPLACEMENT, replacement.continuity);
        assertEquals(DurableTaskRecord.SessionResult.NOT_PROVEN, replacement.session_result);
        assertEquals(DurableTaskRecord.HeapResult.NOT_AVAILABLE, replacement.heap_result);
        assertEquals(
            DurableTaskRecord.ReconstructionResult.PAGE_NOT_CONFIRMED,
            replacement.reconstruction_result
        );
        assertEquals(live.renderer_generation + 1, replacement.renderer_generation);
    }

    @Test
    public void host_restart_discovers_the_same_durable_task() throws Exception {
        DurableTaskStore store = new DurableTaskStore(temporary.getRoot().toPath());
        DurableTaskRecord before = fixture().attach_initial_renderer("renderer-1", 2);
        store.save(
            before,
            DurableTaskStore.TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION
        );
        store.append_navigation(before);

        DurableTaskRecord discovered = store.discover_latest();
        DurableTaskRecord restarted = discovered.opened_by_host("host-2", 202, 3);
        store.save(
            restarted,
            DurableTaskStore.TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION
        );
        DurableTaskRecord reread = store.discover_latest();

        assertEquals(before.task_id, reread.task_id);
        assertEquals(before.tab_id, reread.tab_id);
        assertEquals(before.navigation_id, reread.navigation_id);
        assertEquals(DurableTaskRecord.Continuity.HOST_PROCESS_RESTART, reread.continuity);
        assertEquals(before.host_generation + 1, reread.host_generation);
        assertEquals(before.activity_generation + 1, reread.activity_generation);
        assertEquals("", reread.renderer_id);
        assertEquals(DurableTaskRecord.SessionResult.NOT_PROVEN, reread.session_result);
        assertEquals(DurableTaskRecord.HeapResult.NOT_AVAILABLE, reread.heap_result);
    }

    @Test
    public void activity_recreation_is_not_reported_as_host_restart() {
        DurableTaskRecord recreated = fixture().opened_by_host("host-1", 101, 2);

        assertEquals(DurableTaskRecord.Continuity.ACTIVITY_RECREATION, recreated.continuity);
        assertEquals(1, recreated.host_generation);
        assertEquals(2, recreated.activity_generation);
    }

    @Test
    public void reconstructed_page_does_not_claim_authenticated_continuity() {
        DurableTaskRecord reconstructed = fixture()
            .opened_by_host("host-2", 202, 2)
            .attach_replacement_renderer("renderer-2", 3)
            .page_committed(4);

        assertEquals(DurableTaskRecord.SessionResult.NOT_PROVEN, reconstructed.session_result);
        assertEquals(
            DurableTaskRecord.ReconstructionResult.PAGE_NOT_CONFIRMED,
            reconstructed.reconstruction_result
        );
    }

    @Test
    public void remote_site_failure_is_an_explicit_incomplete_state() {
        DurableTaskRecord blocked = fixture().mark_remote_site_blocked(2);

        assertEquals(DurableTaskRecord.Continuity.REMOTE_SITE_BLOCKED, blocked.continuity);
        assertEquals(DurableTaskRecord.SessionResult.UNAVAILABLE, blocked.session_result);
        assertEquals(
            DurableTaskRecord.ReconstructionResult.INCOMPLETE,
            blocked.reconstruction_result
        );
    }

    @Test
    public void query_secrets_and_one_time_codes_cannot_enter_the_record() {
        DurableTaskRecord record = fixture();
        String serialized = record.serialize();

        assertEquals("https://example.test/", record.navigation.neutral_url);
        assertEquals(
            DurableNavigation.Recovery.POTENTIALLY_SENSITIVE_URL_MATERIAL_REDACTED,
            record.navigation.recovery
        );
        assertFalse(serialized.contains("one-time-secret"));
        assertFalse(serialized.contains("code="));
        assertThrows(
            IllegalArgumentException.class,
            () -> DurableTaskRecord.parse(serialized + "authorization_code\tone-time-secret\n")
        );
    }

    @Test
    public void authorization_adapter_persists_only_metadata_and_protected_references() {
        DurableTaskRecord.AuthorizationContinuation authorization =
            new DurableTaskRecord.AuthorizationContinuation(
                "authorization-1",
                "native-adapter-1",
                "provider-1",
                "client-1",
                Arrays.asList("scope-a", "scope-b"),
                "https://127.0.0.1/return",
                "protected-state-reference-1",
                "protected-nonce-reference-1",
                "protected-pkce-verifier-reference-1",
                "pkce-challenge-reference-1",
                DurableTaskRecord.AuthorizationPhase.WAITING_FOR_RETURN,
                123456
            );
        String serialized = fixture().with_authorization(authorization, 2).serialize();
        DurableTaskRecord round_trip = DurableTaskRecord.parse(serialized);

        assertFalse(serialized.contains("authorization_code"));
        assertFalse(serialized.contains("access_token"));
        assertFalse(serialized.contains("refresh_token"));
        assertEquals("native-adapter-1", round_trip.authorization.adapter_id);
        assertEquals(2, round_trip.authorization.scopes.size());
        assertEquals(
            DurableTaskRecord.AuthorizationPhase.WAITING_FOR_RETURN,
            round_trip.authorization.phase
        );
    }

    @Test
    public void ordinary_tabs_do_not_acquire_durable_task_files() throws Exception {
        DurableTaskStore store = new DurableTaskStore(temporary.getRoot().toPath());

        assertThrows(
            IllegalArgumentException.class,
            () -> store.save(fixture(), DurableTaskStore.TabProtection.ORDINARY)
        );
        assertNull(store.discover_latest());
        assertFalse(Files.exists(temporary.getRoot().toPath().resolve("state/tasks")));
    }

    @Test
    public void unsafe_path_identity_is_refused_before_file_creation() throws Exception {
        DurableTaskStore store = new DurableTaskStore(temporary.getRoot().toPath());
        DurableTaskRecord unsafe = DurableTaskRecord.start(
            "../escape",
            "tab-1",
            "navigation-1",
            DurableNavigation.from_user_url("https://example.test/path"),
            "host-1",
            101,
            1
        );

        assertThrows(
            IllegalArgumentException.class,
            () -> store.save(
                unsafe,
                DurableTaskStore.TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION
            )
        );
        assertFalse(Files.exists(temporary.getRoot().toPath().resolve("escape")));
    }

    @Test
    public void dot_identities_refuse_before_any_state_creation() throws Exception {
        Path root = temporary.getRoot().toPath();
        DurableTaskStore store = new DurableTaskStore(root);
        Path outside = root.resolve("unchanged.txt");
        Files.write(outside, "before".getBytes(StandardCharsets.UTF_8));
        for (String identity : Arrays.asList(".", "..")) {
            assertThrows(IllegalArgumentException.class, () -> store.save(
                named_fixture(identity, "tab-1"),
                DurableTaskStore.TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION
            ));
            assertThrows(IllegalArgumentException.class, () -> store.save(
                named_fixture("task-1", identity),
                DurableTaskStore.TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION
            ));
            assertThrows(IllegalArgumentException.class, () -> store.read_task_record(identity));
            assertThrows(IllegalArgumentException.class, () -> store.append_navigation(
                named_fixture("task-1", identity)
            ));
        }
        assertFalse(Files.exists(root.resolve("state")));
        assertEquals("before", new String(Files.readAllBytes(outside), StandardCharsets.UTF_8));
    }

    @Test
    public void corrupt_newest_keeps_all_older_valid_records_and_failure() throws Exception {
        Path root = temporary.getRoot().toPath();
        DurableTaskStore store = new DurableTaskStore(root);
        DurableTaskRecord first = named_fixture("task-a", "tab-a");
        DurableTaskRecord second = named_fixture("task-b", "tab-b");
        store.save(second, DurableTaskStore.TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION);
        store.save(first, DurableTaskStore.TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION);
        Path first_path = root.resolve("state/tasks/task-a/task.txt");
        Path second_path = root.resolve("state/tasks/task-b/task.txt");
        Files.setLastModifiedTime(first_path, FileTime.fromMillis(1));
        Files.setLastModifiedTime(second_path, FileTime.fromMillis(2));
        Path corrupt = root.resolve("state/tasks/task-c/task.txt");
        Files.createDirectories(corrupt.getParent());
        Files.write(corrupt, "schema\tbroken\n".getBytes(StandardCharsets.UTF_8));
        Files.setLastModifiedTime(corrupt, FileTime.fromMillis(3));

        DurableTaskStore.Discovery discovered = store.discover();
        assertEquals(2, discovered.records.size());
        assertEquals("task-a", discovered.records.get(0).task_id);
        assertEquals("task-b", discovered.records.get(1).task_id);
        assertEquals(first.serialize(), discovered.records.get(0).serialize());
        assertEquals(second.serialize(), discovered.records.get(1).serialize());
        assertEquals("task-b", discovered.latest.task_id);
        assertEquals(1, discovered.failures.size());
        assertEquals("task-c", discovered.failures.get(0).task_id);
        assertEquals("task-b", store.discover_latest().task_id);
        assertEquals(first.serialize(), store.read_task_record("task-a"));
        assertEquals(second.serialize(), store.read_task_record("task-b"));
        assertEquals("task-a", store.discover().records.get(0).task_id);
    }

    @Test
    public void equal_timestamps_have_stable_selection_and_discovery_order() throws Exception {
        Path root = temporary.getRoot().toPath();
        DurableTaskStore store = new DurableTaskStore(root);
        for (String identity : Arrays.asList("task-z", "task-a")) {
            store.save(named_fixture(identity, "tab-" + identity),
                DurableTaskStore.TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION);
            Files.setLastModifiedTime(root.resolve("state/tasks/" + identity + "/task.txt"),
                FileTime.fromMillis(7));
        }
        assertEquals("task-a", store.discover().latest.task_id);
        assertEquals("task-z", store.discover().records.get(1).task_id);
    }

    @Test
    public void linked_state_parent_refuses_writes_and_discovery() throws Exception {
        Path outside = temporary.newFolder("outside").toPath();
        Path selected = temporary.newFolder("selected").toPath();
        Path sentinel = outside.resolve("sentinel.txt");
        Files.write(sentinel, "unchanged".getBytes(StandardCharsets.UTF_8));
        Files.createSymbolicLink(selected.resolve("state"), outside);
        DurableTaskStore store = new DurableTaskStore(selected);
        assertThrows(IOException.class, () -> store.save(fixture(),
            DurableTaskStore.TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION));
        assertThrows(IOException.class, store::discover);
        assertFalse(Files.exists(outside.resolve("tasks")));
        assertFalse(Files.exists(outside.resolve("tabs")));
        assertEquals("unchanged", new String(Files.readAllBytes(sentinel), StandardCharsets.UTF_8));
    }

    @Test
    public void linked_task_directory_and_record_are_individual_failures() throws Exception {
        Path root = temporary.getRoot().toPath();
        DurableTaskStore store = new DurableTaskStore(root);
        store.save(fixture(), DurableTaskStore.TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION);
        Path outside = temporary.newFolder("linked-outside").toPath();
        Path outside_record = outside.resolve("task.txt");
        Files.write(outside_record, named_fixture("task-linked", "tab-linked").serialize()
            .getBytes(StandardCharsets.UTF_8));
        Files.createSymbolicLink(root.resolve("state/tasks/task-linked"), outside);
        Path file_linked = root.resolve("state/tasks/task-file-link");
        Files.createDirectory(file_linked);
        Files.createSymbolicLink(file_linked.resolve("task.txt"), outside_record);

        DurableTaskStore.Discovery discovery = store.discover();
        assertEquals(1, discovery.records.size());
        assertEquals(2, discovery.failures.size());
        assertThrows(IOException.class, () -> store.read_task_record("task-linked"));
        assertThrows(IOException.class, () -> store.read_task_record("task-file-link"));
        assertThrows(IOException.class, () -> store.save(
            named_fixture("task-linked", "tab-linked"),
            DurableTaskStore.TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION));
        assertEquals(named_fixture("task-linked", "tab-linked").serialize(),
            new String(Files.readAllBytes(outside_record), StandardCharsets.UTF_8));
    }

    @Test
    public void linked_history_refuses_append_without_changing_link_target() throws Exception {
        Path root = temporary.getRoot().toPath();
        DurableTaskStore store = new DurableTaskStore(root);
        store.save(fixture(), DurableTaskStore.TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION);
        Path outside = root.resolve("history-outside.txt");
        Files.write(outside, "unchanged".getBytes(StandardCharsets.UTF_8));
        Files.createSymbolicLink(root.resolve("state/tabs/tab-1/history.log"), outside);
        assertThrows(IOException.class, () -> store.append_navigation(fixture()));
        assertEquals("unchanged", new String(Files.readAllBytes(outside), StandardCharsets.UTF_8));
    }

    @Test
    public void oversized_truncated_and_mismatched_records_do_not_hide_valid_record() throws Exception {
        Path root = temporary.getRoot().toPath();
        DurableTaskStore store = new DurableTaskStore(root);
        store.save(fixture(), DurableTaskStore.TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION);
        for (String identity : Arrays.asList(
            "task-oversized", "task-truncated", "task-wrong-id", "task-invalid-utf8"
        )) {
            Files.createDirectory(root.resolve("state/tasks/" + identity));
        }
        Files.write(root.resolve("state/tasks/task-oversized/task.txt"),
            new byte[DurableTaskStore.MAX_RECORD_BYTES + 1]);
        Files.write(root.resolve("state/tasks/task-truncated/task.txt"),
            fixture().serialize().trim().getBytes(StandardCharsets.UTF_8));
        Files.write(root.resolve("state/tasks/task-wrong-id/task.txt"),
            fixture().serialize().getBytes(StandardCharsets.UTF_8));
        Files.write(root.resolve("state/tasks/task-invalid-utf8/task.txt"),
            new byte[] {(byte) 0xc3, (byte) 0x28, (byte) '\n'});
        DurableTaskStore.Discovery discovery = store.discover();
        assertEquals(1, discovery.records.size());
        assertEquals(4, discovery.failures.size());
        assertEquals("task-1", discovery.latest.task_id);
        assertThrows(IOException.class, () -> store.read_task_record("task-oversized"));
        assertThrows(IOException.class, () -> store.read_task_record("task-truncated"));
    }

    @Test
    public void incomplete_temporary_replacement_does_not_replace_committed_record() throws Exception {
        Path root = temporary.getRoot().toPath();
        DurableTaskStore store = new DurableTaskStore(root);
        store.save(fixture(), DurableTaskStore.TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION);
        Files.write(root.resolve("state/tasks/task-1/task.txt.interrupted.tmp"),
            "schema\tunfinished".getBytes(StandardCharsets.UTF_8));
        assertEquals(fixture().serialize(), store.read_task_record("task-1"));
        assertEquals(1, store.discover().records.size());
        assertTrue(store.discover().failures.isEmpty());
    }

    @Test
    public void excessive_entry_count_refuses_instead_of_returning_a_complete_subset() throws Exception {
        Path root = temporary.getRoot().toPath();
        DurableTaskStore store = new DurableTaskStore(root);
        store.save(fixture(), DurableTaskStore.TabProtection.PROTECTED_AUTHENTICATED_TRANSACTION);
        for (int index = 0; index < DurableTaskStore.MAX_TASK_ENTRIES; index++) {
            Files.createDirectory(root.resolve("state/tasks/empty-" + index));
        }
        assertThrows(IOException.class, store::discover);
        assertEquals(fixture().serialize(), store.read_task_record("task-1"));
    }

    private static DurableTaskRecord named_fixture(String task_id, String tab_id) {
        return DurableTaskRecord.start(task_id, tab_id, "navigation-1",
            DurableNavigation.from_user_url("https://example.test/path"), "host-1", 101, 1);
    }

    @Test
    public void fixed_record_round_trips_without_form_or_secret_fields() {
        DurableTaskRecord exact = DurableTaskRecord.start(
            "task-1",
            "tab-1",
            "navigation-1",
            DurableNavigation.from_user_url("https://example.test/"),
            "host-1",
            101,
            1
        );
        String serialized = exact.serialize();
        DurableTaskRecord parsed = DurableTaskRecord.parse(serialized);

        assertEquals(expected_start_record(), serialized);
        assertEquals("task-1", parsed.task_id);
        assertEquals("tab-1", parsed.tab_id);
        assertFalse(serialized.contains("password"));
        assertFalse(serialized.contains("field_value"));
        assertFalse(serialized.contains("cookie"));
    }

    private static String expected_start_record() {
        return "schema\tib-long-view-task-v1\n"
            + "task_id\ttask-1\n"
            + "tab_id\ttab-1\n"
            + "navigation_id\tnavigation-1\n"
            + "neutral_url\thttps://example.test/\n"
            + "origin\thttps://example.test/\n"
            + "address_reconstruction\texact-neutral\n"
            + "continuity\tlive-renderer-survival\n"
            + "session_result\tnot-proven\n"
            + "form_result\tnone-permitted\n"
            + "heap_result\tnot-available\n"
            + "reconstruction_result\tpending\n"
            + "renderer_id\t\n"
            + "host_process_id\thost-1\n"
            + "host_generation\t1\n"
            + "activity_generation\t1\n"
            + "renderer_generation\t0\n"
            + "authorization_adapter\tnone\n";
    }

    private static DurableTaskRecord fixture() {
        return DurableTaskRecord.start(
            "task-1",
            "tab-1",
            "navigation-1",
            DurableNavigation.from_user_url(
                "https://example.test/oauth/callback?code=one-time-secret#fragment"
            ),
            "host-1",
            101,
            1
        );
    }
}

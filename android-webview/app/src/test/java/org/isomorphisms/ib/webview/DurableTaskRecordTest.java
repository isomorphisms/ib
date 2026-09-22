package org.isomorphisms.ib.webview;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import java.nio.file.Files;
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

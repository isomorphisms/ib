package org.isomorphisms.ib.webview;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

/**
 * Android representation of the ordinary-state record owned by
 * {@code IB.LongViewTask}. It contains no page field values or credentials.
 */
final class DurableTaskRecord {
    static final String SCHEMA = "ib-long-view-task-v1";

    enum Continuity {
        LIVE_RENDERER("live-renderer-survival"),
        ACTIVITY_RECREATION("activity-recreation"),
        RENDERER_REPLACEMENT("renderer-replacement"),
        HOST_PROCESS_RESTART("host-process-restart"),
        REMOTE_SITE_BLOCKED("remote-site-blocked");

        final String record_text;

        Continuity(String record_text) {
            this.record_text = record_text;
        }

        static Continuity from_record(String text) {
            return enum_from_record(values(), text, value -> value.record_text, "continuity");
        }
    }

    enum SessionResult {
        NOT_PROVEN("not-proven"),
        AUTHENTICATED_CONFIRMED("authenticated-confirmed"),
        UNAVAILABLE("unavailable");

        final String record_text;

        SessionResult(String record_text) {
            this.record_text = record_text;
        }

        static SessionResult from_record(String text) {
            return enum_from_record(values(), text, value -> value.record_text, "session result");
        }
    }

    enum FormResult {
        NONE_PERMITTED("none-permitted"),
        PERMITTED_AVAILABLE("permitted-available"),
        PERMITTED_RESTORED("permitted-restored"),
        REPEAT_REQUIRED("repeat-required");

        final String record_text;

        FormResult(String record_text) {
            this.record_text = record_text;
        }

        static FormResult from_record(String text) {
            return enum_from_record(values(), text, value -> value.record_text, "form result");
        }
    }

    enum HeapResult {
        LIVE_OBSERVED("live-observed"),
        RECREATED_OBSERVED("recreated-observed"),
        NOT_AVAILABLE("not-available");

        final String record_text;

        HeapResult(String record_text) {
            this.record_text = record_text;
        }

        static HeapResult from_record(String text) {
            return enum_from_record(values(), text, value -> value.record_text, "heap result");
        }
    }

    enum ReconstructionResult {
        LIVE_PAGE("live-page"),
        PENDING("pending"),
        PAGE_NOT_CONFIRMED("page-not-confirmed"),
        CONFIRMED("confirmed"),
        INCOMPLETE("incomplete");

        final String record_text;

        ReconstructionResult(String record_text) {
            this.record_text = record_text;
        }

        static ReconstructionResult from_record(String text) {
            return enum_from_record(values(), text, value -> value.record_text, "reconstruction result");
        }
    }

    enum AuthorizationPhase {
        NOT_STARTED("not-started"),
        WAITING_FOR_USER("waiting-for-user"),
        WAITING_FOR_RETURN("waiting-for-return"),
        RETURNED_WITHOUT_CODE_STORAGE("returned-without-code-storage"),
        EXPIRED("expired"),
        INVALIDATED("invalidated");

        final String record_text;

        AuthorizationPhase(String record_text) {
            this.record_text = record_text;
        }

        static AuthorizationPhase from_record(String text) {
            return enum_from_record(values(), text, value -> value.record_text, "authorization phase");
        }
    }

    static final class AuthorizationContinuation {
        final String transaction_id;
        final String adapter_id;
        final String provider_id;
        final String client_id;
        final List<String> scopes;
        final String redirect_target;
        final String state_reference;
        final String nonce_reference;
        final String pkce_verifier_reference;
        final String pkce_challenge_reference;
        final AuthorizationPhase phase;
        final long expires_at_epoch_ms;

        AuthorizationContinuation(
            String transaction_id,
            String adapter_id,
            String provider_id,
            String client_id,
            List<String> scopes,
            String redirect_target,
            String state_reference,
            String nonce_reference,
            String pkce_verifier_reference,
            String pkce_challenge_reference,
            AuthorizationPhase phase,
            long expires_at_epoch_ms
        ) {
            this.transaction_id = safe_field(transaction_id, "authorization transaction identity");
            this.adapter_id = safe_field(adapter_id, "authorization adapter identity");
            this.provider_id = safe_field(provider_id, "authorization provider identity");
            this.client_id = safe_field(client_id, "authorization client identity");
            List<String> safe_scopes = new ArrayList<>();
            for (String scope : scopes) {
                safe_scopes.add(safe_field(scope, "authorization scope"));
            }
            this.scopes = Collections.unmodifiableList(safe_scopes);
            this.redirect_target = DurableNavigation.from_user_url(redirect_target).neutral_url;
            this.state_reference = optional_safe_field(state_reference, "authorization state reference");
            this.nonce_reference = optional_safe_field(nonce_reference, "authorization nonce reference");
            this.pkce_verifier_reference = optional_safe_field(
                pkce_verifier_reference,
                "authorization PKCE verifier reference"
            );
            this.pkce_challenge_reference = optional_safe_field(
                pkce_challenge_reference,
                "authorization PKCE challenge reference"
            );
            this.phase = Objects.requireNonNull(phase, "authorization phase");
            this.expires_at_epoch_ms = nonnegative(expires_at_epoch_ms, "authorization expiry");
        }
    }

    private static final Set<String> REQUIRED_KEYS = unmodifiable_set(
        "schema", "task_id", "tab_id", "navigation_id", "neutral_url", "origin",
        "address_reconstruction", "continuity", "session_result", "form_result",
        "heap_result", "reconstruction_result", "renderer_id", "host_process_id",
        "host_generation", "activity_generation", "renderer_generation",
        "authorization_adapter"
    );

    private static final Set<String> AUTHORIZATION_KEYS = unmodifiable_set(
        "authorization_transaction_id", "authorization_provider_id",
        "authorization_client_id", "authorization_redirect",
        "authorization_state_reference", "authorization_nonce_reference",
        "authorization_pkce_verifier_reference",
        "authorization_pkce_challenge_reference", "authorization_phase",
        "authorization_expires_at_epoch_ms"
    );

    final String task_id;
    final String tab_id;
    final String navigation_id;
    final DurableNavigation navigation;
    final Continuity continuity;
    final SessionResult session_result;
    final FormResult form_result;
    final HeapResult heap_result;
    final ReconstructionResult reconstruction_result;
    final String renderer_id;
    final String host_process_id;
    final int host_pid;
    final long host_generation;
    final long activity_generation;
    final long renderer_generation;
    final long updated_at_epoch_ms;
    final AuthorizationContinuation authorization;

    private DurableTaskRecord(
        String task_id,
        String tab_id,
        String navigation_id,
        DurableNavigation navigation,
        Continuity continuity,
        SessionResult session_result,
        FormResult form_result,
        HeapResult heap_result,
        ReconstructionResult reconstruction_result,
        String renderer_id,
        String host_process_id,
        int host_pid,
        long host_generation,
        long activity_generation,
        long renderer_generation,
        long updated_at_epoch_ms,
        AuthorizationContinuation authorization
    ) {
        this.task_id = safe_field(task_id, "task identity");
        this.tab_id = safe_field(tab_id, "tab identity");
        this.navigation_id = safe_field(navigation_id, "navigation identity");
        this.navigation = Objects.requireNonNull(navigation, "navigation");
        this.continuity = Objects.requireNonNull(continuity, "continuity");
        this.session_result = Objects.requireNonNull(session_result, "session result");
        this.form_result = Objects.requireNonNull(form_result, "form result");
        this.heap_result = Objects.requireNonNull(heap_result, "heap result");
        this.reconstruction_result = Objects.requireNonNull(
            reconstruction_result,
            "reconstruction result"
        );
        this.renderer_id = optional_safe_field(renderer_id, "renderer identity");
        this.host_process_id = safe_field(host_process_id, "host process identity");
        this.host_pid = host_pid;
        this.host_generation = nonnegative(host_generation, "host generation");
        this.activity_generation = nonnegative(activity_generation, "activity generation");
        this.renderer_generation = nonnegative(renderer_generation, "renderer generation");
        this.updated_at_epoch_ms = nonnegative(updated_at_epoch_ms, "updated time");
        this.authorization = authorization;
    }

    static DurableTaskRecord start(
        String task_id,
        String tab_id,
        String navigation_id,
        DurableNavigation navigation,
        String host_process_id,
        int host_pid,
        long now
    ) {
        return new DurableTaskRecord(
            task_id, tab_id, navigation_id, navigation,
            Continuity.LIVE_RENDERER, SessionResult.NOT_PROVEN,
            FormResult.NONE_PERMITTED, HeapResult.NOT_AVAILABLE,
            ReconstructionResult.PENDING, "", host_process_id, host_pid,
            1, 1, 0, now, null
        );
    }

    DurableTaskRecord attach_initial_renderer(String renderer_id, long now) {
        return copy(
            navigation_id, navigation, Continuity.LIVE_RENDERER, session_result,
            form_result, HeapResult.NOT_AVAILABLE, ReconstructionResult.LIVE_PAGE,
            renderer_id, host_process_id, host_pid, host_generation,
            activity_generation, renderer_generation + 1, now, authorization
        );
    }

    DurableTaskRecord renderer_was_lost(long now) {
        FormResult next_form = form_result == FormResult.PERMITTED_AVAILABLE
            ? FormResult.PERMITTED_AVAILABLE
            : FormResult.REPEAT_REQUIRED;
        return copy(
            navigation_id, navigation, Continuity.RENDERER_REPLACEMENT,
            SessionResult.NOT_PROVEN, next_form, HeapResult.NOT_AVAILABLE,
            ReconstructionResult.PENDING, "", host_process_id, host_pid,
            host_generation, activity_generation, renderer_generation, now, authorization
        );
    }

    DurableTaskRecord attach_replacement_renderer(String renderer_id, long now) {
        return copy(
            navigation_id, navigation, continuity, SessionResult.NOT_PROVEN,
            form_result, HeapResult.NOT_AVAILABLE, ReconstructionResult.PAGE_NOT_CONFIRMED,
            renderer_id, host_process_id, host_pid, host_generation,
            activity_generation, renderer_generation + 1, now, authorization
        );
    }

    DurableTaskRecord opened_by_host(String new_host_process_id, int new_host_pid, long now) {
        boolean same_process = host_process_id.equals(new_host_process_id);
        return copy(
            navigation_id, navigation,
            same_process ? Continuity.ACTIVITY_RECREATION : Continuity.HOST_PROCESS_RESTART,
            SessionResult.NOT_PROVEN, form_result, HeapResult.NOT_AVAILABLE,
            ReconstructionResult.PENDING, "", new_host_process_id, new_host_pid,
            same_process ? host_generation : host_generation + 1,
            activity_generation + 1, renderer_generation, now, authorization
        );
    }

    DurableTaskRecord begin_navigation(
        String new_navigation_id,
        DurableNavigation new_navigation,
        long now
    ) {
        return copy(
            new_navigation_id, new_navigation, Continuity.LIVE_RENDERER,
            SessionResult.NOT_PROVEN, FormResult.NONE_PERMITTED,
            HeapResult.NOT_AVAILABLE, ReconstructionResult.PENDING, renderer_id,
            host_process_id, host_pid, host_generation, activity_generation,
            renderer_generation, now, authorization
        );
    }

    DurableTaskRecord observe_current_navigation(DurableNavigation current_navigation, long now) {
        return copy(
            navigation_id, current_navigation, continuity, session_result, form_result,
            heap_result, reconstruction_result, renderer_id, host_process_id, host_pid,
            host_generation, activity_generation, renderer_generation, now, authorization
        );
    }

    DurableTaskRecord page_committed(long now) {
        FormResult next_form = form_result == FormResult.PERMITTED_AVAILABLE
            ? FormResult.PERMITTED_RESTORED
            : form_result;
        return copy(
            navigation_id, navigation, continuity, session_result, next_form,
            heap_result,
            continuity == Continuity.LIVE_RENDERER
                ? ReconstructionResult.LIVE_PAGE
                : ReconstructionResult.PAGE_NOT_CONFIRMED,
            renderer_id, host_process_id, host_pid, host_generation,
            activity_generation, renderer_generation, now, authorization
        );
    }

    DurableTaskRecord observe_heap(boolean same_live_heap, long now) {
        HeapResult observed;
        if (!same_live_heap) {
            observed = HeapResult.RECREATED_OBSERVED;
        } else if (
            continuity == Continuity.LIVE_RENDERER
                && heap_result != HeapResult.RECREATED_OBSERVED
        ) {
            observed = HeapResult.LIVE_OBSERVED;
        } else {
            observed = heap_result;
        }
        return copy(
            navigation_id, navigation, continuity, session_result, form_result,
            observed,
            reconstruction_result, renderer_id, host_process_id, host_pid,
            host_generation, activity_generation, renderer_generation, now, authorization
        );
    }

    DurableTaskRecord confirm_authenticated(long now) {
        return copy(
            navigation_id, navigation, continuity, SessionResult.AUTHENTICATED_CONFIRMED,
            form_result, heap_result, ReconstructionResult.CONFIRMED, renderer_id,
            host_process_id, host_pid, host_generation, activity_generation,
            renderer_generation, now, authorization
        );
    }

    DurableTaskRecord mark_remote_site_blocked(long now) {
        return copy(
            navigation_id, navigation, Continuity.REMOTE_SITE_BLOCKED,
            SessionResult.UNAVAILABLE, form_result, heap_result,
            ReconstructionResult.INCOMPLETE, renderer_id, host_process_id, host_pid,
            host_generation, activity_generation, renderer_generation, now, authorization
        );
    }

    DurableTaskRecord with_authorization(AuthorizationContinuation authorization, long now) {
        return copy(
            navigation_id, navigation, continuity, session_result, form_result,
            heap_result, reconstruction_result, renderer_id, host_process_id, host_pid,
            host_generation, activity_generation, renderer_generation, now, authorization
        );
    }

    String serialize() {
        StringBuilder output = new StringBuilder();
        line(output, "schema", SCHEMA);
        line(output, "task_id", task_id);
        line(output, "tab_id", tab_id);
        line(output, "navigation_id", navigation_id);
        line(output, "neutral_url", navigation.neutral_url);
        line(output, "origin", navigation.origin);
        line(output, "address_reconstruction", navigation.recovery.record_text);
        line(output, "continuity", continuity.record_text);
        line(output, "session_result", session_result.record_text);
        line(output, "form_result", form_result.record_text);
        line(output, "heap_result", heap_result.record_text);
        line(output, "reconstruction_result", reconstruction_result.record_text);
        line(output, "renderer_id", renderer_id);
        line(output, "host_process_id", host_process_id);
        line(output, "host_generation", Long.toString(host_generation));
        line(output, "activity_generation", Long.toString(activity_generation));
        line(output, "renderer_generation", Long.toString(renderer_generation));
        if (authorization == null) {
            line(output, "authorization_adapter", "none");
        } else {
            line(output, "authorization_adapter", authorization.adapter_id);
            line(output, "authorization_transaction_id", authorization.transaction_id);
            line(output, "authorization_provider_id", authorization.provider_id);
            line(output, "authorization_client_id", authorization.client_id);
            for (String scope : authorization.scopes) {
                line(output, "authorization_scope", scope);
            }
            line(output, "authorization_redirect", authorization.redirect_target);
            line(output, "authorization_state_reference", authorization.state_reference);
            line(output, "authorization_nonce_reference", authorization.nonce_reference);
            line(output, "authorization_pkce_verifier_reference", authorization.pkce_verifier_reference);
            line(output, "authorization_pkce_challenge_reference", authorization.pkce_challenge_reference);
            line(output, "authorization_phase", authorization.phase.record_text);
            line(output, "authorization_expires_at_epoch_ms", Long.toString(authorization.expires_at_epoch_ms));
        }
        return output.toString();
    }

    static DurableTaskRecord parse(String text) {
        Map<String, String> fields = new LinkedHashMap<>();
        List<String> scopes = new ArrayList<>();
        for (String line : text.split("\\n", -1)) {
            if (line.isEmpty()) {
                continue;
            }
            int separator = line.indexOf('\t');
            if (separator <= 0) {
                throw new IllegalArgumentException("durable task line lacks one field separator");
            }
            String key = line.substring(0, separator);
            String value = line.substring(separator + 1);
            if ("authorization_scope".equals(key)) {
                scopes.add(safe_field(value, "authorization scope"));
                continue;
            }
            if (!REQUIRED_KEYS.contains(key) && !AUTHORIZATION_KEYS.contains(key)) {
                throw new IllegalArgumentException("unknown durable task field: " + key);
            }
            if (fields.put(key, value) != null) {
                throw new IllegalArgumentException("duplicate durable task field: " + key);
            }
        }
        for (String required : REQUIRED_KEYS) {
            if (!fields.containsKey(required)) {
                throw new IllegalArgumentException("missing durable task field: " + required);
            }
        }
        if (!SCHEMA.equals(fields.get("schema"))) {
            throw new IllegalArgumentException("unsupported durable task schema");
        }
        DurableNavigation.Recovery recovery = DurableNavigation.Recovery.from_record(
            fields.get("address_reconstruction")
        );
        DurableNavigation navigation = DurableNavigation.from_persisted(
            fields.get("neutral_url"),
            fields.get("origin"),
            recovery
        );
        AuthorizationContinuation authorization = parse_authorization(fields, scopes);
        return new DurableTaskRecord(
            fields.get("task_id"), fields.get("tab_id"), fields.get("navigation_id"),
            navigation, Continuity.from_record(fields.get("continuity")),
            SessionResult.from_record(fields.get("session_result")),
            FormResult.from_record(fields.get("form_result")),
            HeapResult.from_record(fields.get("heap_result")),
            ReconstructionResult.from_record(fields.get("reconstruction_result")),
            fields.get("renderer_id"), fields.get("host_process_id"), 0,
            parse_long(fields.get("host_generation"), "host generation"),
            parse_long(fields.get("activity_generation"), "activity generation"),
            parse_long(fields.get("renderer_generation"), "renderer generation"), 0,
            authorization
        );
    }

    private static AuthorizationContinuation parse_authorization(
        Map<String, String> fields,
        List<String> scopes
    ) {
        String adapter = fields.get("authorization_adapter");
        if ("none".equals(adapter)) {
            for (String key : AUTHORIZATION_KEYS) {
                if (fields.containsKey(key)) {
                    throw new IllegalArgumentException("authorization metadata exists without adapter");
                }
            }
            if (!scopes.isEmpty()) {
                throw new IllegalArgumentException("authorization scopes exist without adapter");
            }
            return null;
        }
        for (String key : AUTHORIZATION_KEYS) {
            if (!fields.containsKey(key)) {
                throw new IllegalArgumentException("missing authorization field: " + key);
            }
        }
        return new AuthorizationContinuation(
            fields.get("authorization_transaction_id"), adapter,
            fields.get("authorization_provider_id"), fields.get("authorization_client_id"),
            scopes, fields.get("authorization_redirect"),
            fields.get("authorization_state_reference"),
            fields.get("authorization_nonce_reference"),
            fields.get("authorization_pkce_verifier_reference"),
            fields.get("authorization_pkce_challenge_reference"),
            AuthorizationPhase.from_record(fields.get("authorization_phase")),
            parse_long(fields.get("authorization_expires_at_epoch_ms"), "authorization expiry")
        );
    }

    private DurableTaskRecord copy(
        String navigation_id,
        DurableNavigation navigation,
        Continuity continuity,
        SessionResult session_result,
        FormResult form_result,
        HeapResult heap_result,
        ReconstructionResult reconstruction_result,
        String renderer_id,
        String host_process_id,
        int host_pid,
        long host_generation,
        long activity_generation,
        long renderer_generation,
        long now,
        AuthorizationContinuation authorization
    ) {
        return new DurableTaskRecord(
            task_id, tab_id, navigation_id, navigation, continuity, session_result,
            form_result, heap_result, reconstruction_result, renderer_id,
            host_process_id, host_pid, host_generation, activity_generation,
            renderer_generation, now, authorization
        );
    }

    private static String safe_field(String value, String role) {
        Objects.requireNonNull(value, role);
        if (value.isEmpty() || value.indexOf('\t') >= 0 || value.indexOf('\n') >= 0 || value.indexOf('\r') >= 0) {
            throw new IllegalArgumentException(role + " is empty or contains a record separator");
        }
        return value;
    }

    private static String optional_safe_field(String value, String role) {
        if (value == null || value.isEmpty()) {
            return "";
        }
        return safe_field(value, role);
    }

    private static long nonnegative(long value, String role) {
        if (value < 0) {
            throw new IllegalArgumentException(role + " must be nonnegative");
        }
        return value;
    }

    private static void line(StringBuilder output, String key, String value) {
        output.append(key).append('\t').append(optional_safe_field(value, key)).append('\n');
    }

    private static int parse_int(String value, String role) {
        try {
            return Integer.parseInt(value);
        } catch (NumberFormatException exception) {
            throw new IllegalArgumentException(role + " is not an integer", exception);
        }
    }

    private static long parse_long(String value, String role) {
        try {
            return nonnegative(Long.parseLong(value), role);
        } catch (NumberFormatException exception) {
            throw new IllegalArgumentException(role + " is not an integer", exception);
        }
    }

    private static Set<String> unmodifiable_set(String... values) {
        return Collections.unmodifiableSet(new HashSet<>(Arrays.asList(values)));
    }

    private interface RecordText<T> {
        String text(T value);
    }

    private static <T> T enum_from_record(
        T[] values,
        String text,
        RecordText<T> record_text,
        String role
    ) {
        for (T value : values) {
            if (record_text.text(value).equals(text)) {
                return value;
            }
        }
        throw new IllegalArgumentException("unknown " + role);
    }
}

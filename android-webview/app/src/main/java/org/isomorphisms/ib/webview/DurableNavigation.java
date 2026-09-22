package org.isomorphisms.ib.webview;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Locale;
import java.util.Objects;

/** A restart target that cannot carry user-info, query, or fragment secrets. */
final class DurableNavigation {
    enum Recovery {
        EXACT_NEUTRAL("exact-neutral"),
        POTENTIALLY_SENSITIVE_URL_MATERIAL_REDACTED("path-query-or-fragment-redacted"),
        UNAVAILABLE("unavailable");

        final String record_text;

        Recovery(String record_text) {
            this.record_text = record_text;
        }

        static Recovery from_record(String text) {
            for (Recovery value : values()) {
                if (value.record_text.equals(text)) {
                    return value;
                }
            }
            throw new IllegalArgumentException("unknown address reconstruction result");
        }
    }

    final String neutral_url;
    final String origin;
    final Recovery recovery;

    private DurableNavigation(String neutral_url, String origin, Recovery recovery) {
        this.neutral_url = neutral_url;
        this.origin = origin;
        this.recovery = recovery;
    }

    static DurableNavigation from_user_url(String text) {
        Objects.requireNonNull(text, "url");
        try {
            URI parsed = new URI(text.trim());
            String scheme = parsed.getScheme();
            String host = parsed.getHost();
            if (scheme == null || host == null || parsed.getUserInfo() != null) {
                throw new IllegalArgumentException("URL must have an HTTP(S) origin without user-info");
            }
            String normalized_scheme = scheme.toLowerCase(Locale.ROOT);
            if (!"https".equals(normalized_scheme) && !"http".equals(normalized_scheme)) {
                throw new IllegalArgumentException("URL must use HTTP or HTTPS");
            }
            String path = parsed.getRawPath();
            boolean path_was_redacted = path != null && !path.isEmpty() && !"/".equals(path);
            URI neutral = new URI(
                normalized_scheme,
                null,
                host.toLowerCase(Locale.ROOT),
                parsed.getPort(),
                "/",
                null,
                null
            );
            URI origin = new URI(
                normalized_scheme,
                null,
                host.toLowerCase(Locale.ROOT),
                parsed.getPort(),
                "/",
                null,
                null
            );
            Recovery recovery = !path_was_redacted
                    && parsed.getRawQuery() == null
                    && parsed.getRawFragment() == null
                ? Recovery.EXACT_NEUTRAL
                : Recovery.POTENTIALLY_SENSITIVE_URL_MATERIAL_REDACTED;
            return new DurableNavigation(neutral.toASCIIString(), origin.toASCIIString(), recovery);
        } catch (URISyntaxException exception) {
            throw new IllegalArgumentException("URL is not a valid restart target", exception);
        }
    }

    static DurableNavigation unavailable() {
        return new DurableNavigation("", "", Recovery.UNAVAILABLE);
    }

    static DurableNavigation from_persisted(String neutral_url, String origin, Recovery recovery) {
        if (recovery == Recovery.UNAVAILABLE) {
            return unavailable();
        }
        DurableNavigation checked = from_user_url(neutral_url);
        if (!checked.origin.equals(origin) || checked.recovery != Recovery.EXACT_NEUTRAL) {
            throw new IllegalArgumentException("persisted neutral navigation is inconsistent");
        }
        return new DurableNavigation(checked.neutral_url, checked.origin, recovery);
    }
}

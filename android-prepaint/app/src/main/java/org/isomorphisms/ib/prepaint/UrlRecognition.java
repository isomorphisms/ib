package org.isomorphisms.ib.prepaint;

import java.net.URI;
import java.net.URISyntaxException;
import java.net.URLEncoder;
import java.io.UnsupportedEncodingException;
import java.nio.charset.StandardCharsets;
import java.util.Locale;

final class UrlRecognition {
    private static final String GOOGLE_SEARCH = "https://www.google.com/search?q=";
    private static final int MAX_URL_CHARACTERS = 2048;

    private UrlRecognition() {
    }

    static boolean isAbsoluteHttpUrl(String candidate) {
        if (candidate == null || candidate.isEmpty()
                || candidate.length() > MAX_URL_CHARACTERS) {
            return false;
        }
        for (int index = 0; index < candidate.length(); index += 1) {
            char value = candidate.charAt(index);
            if (value <= 0x20 || value >= 0x7f) {
                return false;
            }
        }

        String lower = candidate.toLowerCase(Locale.ROOT);
        if (!lower.startsWith("http://") && !lower.startsWith("https://")) {
            return false;
        }
        try {
            URI parsed = new URI(candidate);
            return parsed.isAbsolute()
                    && parsed.getHost() != null
                    && !parsed.getHost().isEmpty()
                    && parsed.getRawUserInfo() == null
                    && !parsed.getRawAuthority().startsWith("[");
        } catch (URISyntaxException ignored) {
            return false;
        }
    }

    static String googleSearchUrl(String query) {
        try {
            return GOOGLE_SEARCH + URLEncoder.encode(query, StandardCharsets.UTF_8.name());
        } catch (UnsupportedEncodingException impossible) {
            throw new AssertionError("UTF-8 is required by Android", impossible);
        }
    }

    static String navigationUrl(String input) {
        String trimmed = input == null ? "" : input.trim();
        return isAbsoluteHttpUrl(trimmed) ? trimmed : googleSearchUrl(trimmed);
    }

    static String resolve(String base, String target) {
        if (isAbsoluteHttpUrl(target)) {
            return target;
        }
        if (base == null || base.isEmpty()) {
            return target;
        }
        try {
            return new URI(base).resolve(target).toASCIIString();
        } catch (URISyntaxException | IllegalArgumentException ignored) {
            return target;
        }
    }
}

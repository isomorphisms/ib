# Native network sources

The ARMv7 APK builds these inputs from source rather than relying on a phone,
Termux, or host-installed `curl`:

| Input | Pinned revision | SHA-256 | License |
| --- | --- | --- | --- |
| curl | 8.21.0 | `aa1b66a70eace83dc624508745646c08ae561de512ab403adffb93ac87fc72e6` | curl license |
| Mbed TLS | 4.2.0 | `2bed9d713b4668f76553b097e72b8aa30bc8f112a940d7ae228d524bbde6ffea` | Apache-2.0 or GPL-2.0-or-later |
| Mozilla CA extract | 2026-08-13 | `f66dff1bdf8f96060b8177976f8b7d9254bc89bc4db933d769f7384d28480bc9` | MPL-2.0 |

`build-network-deps-armv7.sh` downloads those exact archives, verifies every
digest before extraction, disables curl protocols other than HTTP(S), and links
the resulting archives into `libib.so`. Runtime policy currently accepts HTTPS
only. The curl and Mbed TLS license texts are copied from their verified source
archives into the APK assets during the build.

Primary project pages:

- <https://curl.se/download.html>
- <https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-4.2.0>
- <https://curl.se/docs/caextract.html>

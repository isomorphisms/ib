# Controlled host ICU → Arena observations — 2026-10-31

The actual compiled ICU transport performed HTTP and validated HTTPS against
controlled local servers, followed a redirect, returned its observed response
metadata and original body through IB's maintained capture frame, and committed
the body through the canonical userspace Arena. The independent ai-ci reader
verified both persisted HTML ranges. Android app_process and Shizuku were test
wrappers; no Android Binder or physical transaction is claimed.

Inputs: ICU `22730ba7e93d9fecc37f7da440cbdd086deb3d15`, canonical userspace
Arena `05a0cb7246c5a9f46abfbe92925812e7b607f454`, ai-ci's existing independent
`ib-source/verify.sh` at `c051b9a` (tree-equivalent to its published
`ac06d67dd82cdf82c59ea06f9378ddccb09e8bc7` checkpoint). Local execution used
qualified ICK `c61e448251744a2f40ad743ebef1a027bdcd2f9d` on x86-64 Linux.
The hosted workflow separately uses its declared ICK fbe86e23 pin.

| Observation / control | State | Actual result |
| --- | --- | --- |
| HTTPS response and redirect | PASS | Requested `https://localhost:18933/start`, resolved `https://localhost:18933/final`, status 200, one observed redirect |
| HTTP response and redirect | PASS | Requested `http://127.0.0.1:18932/start`, resolved `/final` at the same origin, status 200 |
| Original and committed body | PASS | 19 bytes; SHA-256 `d15386d1be1496f127ee5f1b34385ef49845c326ef1c43bf2280c6d47974a07e` |
| Observed media | PASS | `text/html; charset=utf-8`, identity content encoding, valid UTF-8 |
| Independent committed-source readback | PASS | ai-ci accepted source `secure`, generation 17, and source `plain`, generation 18, with that same length and digest |
| Binary source | PASS | Exact NUL/0xff bytes retained; HTML admission refused by the independent reader |
| Untrusted TLS certificate | PASS | Rejected; no receipt or commit-pointer advance |
| HTTP 404 and truncation | PASS | Rejected; no receipt or commit-pointer advance |
| Wrong pinned native hash | PASS | Rejected before source publication |
| Last good source after failures | PASS | Reopened and matched without another acquisition |
| Complete redirect chain provenance | NOT_RUN | Transport records final URL and count; individual hop evidence is not yet retained |
| Android DEX, Binder, real Shizuku, native UI and TAB_P10 | NOT_RUN | Outside this host test |

The updated wrapper requires the real `--http1.1 get` command shape. The file
adapter accepts the original eight observations or the paired full-response
extensions; range, missing/duplicated extension and unknown transfer-decoding
controls refuse without advancing the Arena. It does not ignore arbitrary
metadata keys. Fixture TLS keys are not included in retained response evidence.

Exact executable/input SHA-256 values:

| Input | SHA-256 |
| --- | --- |
| ICU native source | `77d808a1930c147d9831b264190cf8b6f2067cbffe149a70c66e13d043f3fe30` |
| Compiled ICU shared library | `878f74f2e2b0d217d5bce1767e41e3ce1b0826f466bb0e5c102e08ca4799410c` |
| Narrow native transport probe | `6cb2d304b774492e5ef6a56f8e2bb1b7bfd2fe8beff29854f161bec8c9a4df76` |
| Updated host integration test | `162f2a04af15f5c109820c8e7ef8c16e9a53092d7129270a84d5c8a5a0f7650a` |

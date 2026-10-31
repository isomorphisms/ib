# Native source-owner host observations — 2026-10-31

This record binds the native inputs below to local host execution. It does not
qualify the complete reader, Android Binder, a network response or TAB_P10.
The response metadata is test data. Canonical bytes are independently read
through the pinned ai-ci verifier; no fixture is represented as a live fetch.

Host: x86-64 Linux. Compiler: qualified ICK
`c61e448251744a2f40ad743ebef1a027bdcd2f9d`, GCC source
`6294f1d9e7536e5ffcde09d1528c918d63abfef5`. The official downloaded compiler
archive SHA-256 is
`a3d4b48be01f416c2b8822841669c0673f11f1216d3e745c2651a4a3c12f0d2e`.
Driver SHA-256:
`45805e3295f2709737fcc87985a8a2341aa8663ff8e4e398d2a9d78dc1283e10`.
This is separate from the workflow's ICK fbe86e23 qualification.

Grease root: `f19c94c6df18cddbdc1e81463e5bd689533e3c13`; source gitlink:
`6d29702a10ea9eb72a43950554dbcd4174d07a89`; Cat Food bootstrap:
`609a9628d5a52860f956bf62e0914a0cd03292ae`. Its foreign CPython 2 source uses
the explicit GNU C11 compatibility profile with ICK. Native receiver/source
tests use `-std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -pthread`.

Canonical userspace Arena:
`05a0cb7246c5a9f46abfbe92925812e7b607f454`. Independent ai-ci source:
`ac06d67dd82cdf82c59ea06f9378ddccb09e8bc7`, its existing `ib-source/verify.sh`.
This is ordinary-file userspace storage, not kernel appendfat or SD-card evidence.

| Check | Result | Observed exit / expectation |
| --- | --- | --- |
| Private ingress and binary cross-process FD exchange | PASS | 0; prior UID, ID, generation, empty, oversized and producer-error refusals retained |
| Permanent revocation before arm, before transfer and during transfer | PASS | 0; restoring a wanted generation cannot reopen the revoked instance |
| Compiled mutation removing cancellation from session matching | PASS | Rejected with exit 134 at `ib_ingress_begin` after the exact revocation stage marker |
| Native retained-source pipe with synthetic pointer fixture | PASS | 0; fixture explicitly labeled synthetic |
| Actual canonical userspace append, independent slice and ai-ci readback | PASS | 0; `checked_arena=actual_appends` |
| Source generation 1 reopened at navigation generation 3 | PASS | 0; source receipt unchanged; stale in-flight read rejected while last-good state retained |
| Compiled mutation using navigation generation as retained generation | PASS | Rejected with exit 134 at `ib_arena_source_owner_open_retained` for both synthetic and canonical backends |
| Real HTTP response observations, Android Binder, ART, APK and tablet | NOT_RUN | Not established by these host checks |

The 14-byte canonical fixture digest is
`b66ad039b030e48f78bb1efbd6a37fec93180e6d8646e7813793c6d22b815c66`.
It is source test data, not a digest of example.com.

## Exact native inputs

| File | SHA-256 |
| --- | --- |
| `native/android/ib_shizuku_arena_ingress.c` | `cc86fc04d509336e7cedc42e0d6e8bcab996c87aef232d3c77f9b6b67146531e` |
| `native/android/ib_shizuku_arena_ingress.h` | `26c15c042f49f411f6e718afb0dc518e3582788dd2c45825a2f9c493cccdc296` |
| `native/android/ib_arena_source_owner.c` | `c00a9155390e7e6bdb5f58d7af675e1be17e7df379a76abf8c51c2d1f8acd3ae` |
| `native/android/ib_arena_source_owner.h` | `7685f9ef082405fc15d4b9525769414663ebf076768e252759360344cf5117e5` |
| `tests/test_android_arena_source_owner.c` | `531a0f104379248b5e3368f2e73d57800ab4d8b58409d975cb8d6bb29bc8ff4c` |
| `tests/test_shizuku_arena_ingress_host.c` | `4ffd27dd4644ae4baaad2b9f83162b51cf725a981ee74a02511d9fec08985c8d` |

## Generated executable identities

| Executable | SHA-256 |
| --- | --- |
| Ingress test | `bc03ecf236a8ab84ffbe4ca8d4c0efc71119c27b559d9e2dc313fd67de34acde` |
| Revocation mutant | `54036e47e829a759ddaf3ae50563542c26632f626262c50d1eb009fb67bdeb50` |
| Source-owner test | `93b9c3a6621f69e4bbb3623ca11c0719c72dda8678c7ef520f60603d105983d8` |
| Generation mutant | `72f3af712bfa32b50d4df2fb16904863df3960ce207fe50da9b6e3e98583056f` |
| Canonical Arena command | `a573f5093f512630159b54af0ea029a8dbf7c9baccfac81ac4ca6c0b2a9f71f4` |
| Independent byte slice | `95a881c954631aac8bc968af9f8f27942fb64c8248c42c85ecaacecc4a2ece48` |

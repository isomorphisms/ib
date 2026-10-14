# IB macOS target (experimental)

Tracking: [IB #149](https://github.com/isomorphisms/ib/issues/149). This
implementation branch is **stacked on the unmerged Linux/shared-core PR #148**;
do not take its changes as already in `main`.

## Architecture

- **Idriç** owns browser resource, tab, task, navigation, event, source/fragment,
  persistence, and renderer lifecycle semantics. The common core in #148 also
  supplies exact source-linked copy and UTF-16 index conversion.
- **Grease and ICU** own OS-visible orchestration and acquisition. macOS's
  AppKit/WKWebView layers must only project state and report typed input/events.
- `native/macos/ib_platform.c` is a stateless native ABI transport boundary;
  `src/IB/MacOS/Platform.idric` is its Idriç wrapper. It accepts a borrowed
  UTF-8 C string only during the call; the wrapper refuses embedded NUL.
- `IBPrepaint.m` is the first **read-only AppKit frontend adapter**. It consumes
  the already documented `ib-prepaint 1` disposable view stream, not HTML or
  canonical browser records. Text is selectable and copyable. Links show their
  targets, but do **not** navigate outside the browser core. It shows the latest
  complete/partial supplied revision. This slice does not animate intermediate
  revisions.

The AppKit prototype accepts requested/resolved URL metadata, titles, headings,
text, links and rows with backslash decoding and bounded UTF-8. It refuses
image, form and unknown records instead of discarding information or claiming
functional support. It does not acquire URLs or synthesize history.

## Build and bounded checks

On macOS arm64 or x86-64 with Xcode Command Line Tools:

```sh
sh bin/check_macos_target.grease
```

The script builds the C Mach-O shared library, exercises real ABI/UTF-8 checks,
compiles the AppKit frontend, verifies the independent UTF-8 projection fixture,
rejects wrong-order, malformed escape, unsupported-image and oversized inputs,
and reports the host architecture and SDK.

For a local exploratory `.app` artifact, set `IB_MACOS_OUTPUT` to an
existing or new output folder when running the script. The app takes a local
`.prepaint` file path and may be opened from Terminal:

```sh
"/path/to/IB Prepaint.app/Contents/MacOS/IBPrepaint" \
  "/path/to/sample.prepaint"
```

The native builds are checked on separate `macos-15` (arm64) and
`macos-15-intel` (x86-64) CI hosts. Their artifacts are unsigned,
unnotarized, provisional test outputs; they are not an IB browser release.

## Required work before macOS is a functioning IB browser

1. Qualify the maintained Idriç/Chez executable and ICK/C ABI as a **Darwin**
   artifact with pinned compiler/SDK and actual generated-program FFI. A C
   compilation success does not qualify the Idriç wrapper.
2. Consume canonical Idriç source-backed projections, not just a file-based
   standalone fixture. Implement view revisions, source-bound selections,
   images/colors, forms and event callbacks with proper source coordinates.
3. Route actual link/input events to the Idriç command layer; integrate ICU's
   HTTPS, redirects and extraction. WKWebView is a *replaceable renderer*,
   never a second owner of navigation history, auth or tasks.
4. Reuse disk journal and ordinary/protected tab reconstruction; terminate
   the page worker and host to validate exact task/tab/navigation recovery.
5. Add independent AI-CI oracle/mutation checks and exact-source parity on
   Android A1/C67 and Linux. Physical UI launch, real network, security and
   recovery each require **separate** evidence.

Status: native build/contract test authored; actual hosted macOS result and
Idriç integration are not established merely by opening this PR. Do not claim
cross-platform or production acceptance from this slice.

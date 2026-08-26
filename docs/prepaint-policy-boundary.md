# Page pre-paint, browser policy, and workbench diagnostics

The page pre-painter and the developer workbench answer different questions.
They must not be merged into one screen merely because both can be rendered as
dark text and rows.

## Page pre-paint

The page surface contains information that came from the selected resource:

- article paragraphs and headings;
- links and their resolved targets;
- page forms and controls when the browser can represent their action honestly;
- table/data rows;
- already-fetched images, unchanged;
- a small browser-owned search/navigation control outside page content.

The page surface does not contain fixture-universe counts, hot/warm/cold tab
graphs, deployment topology invented by a test, DNS traces, route hops, cache
classification, or storage totals. A real page may itself contain a topology
image; that is ordinary page content. The bundled APK sample should not use such
an image because it falsely suggests that IB adds the diagnostic to every page.

Opening a plain UTF-8 file is a real pre-paint path. Ordinary prose is paintable
without an `ib-prepaint` envelope. A file containing one absolute HTTP(S) URL per
line becomes a link list. A saved HTML file still belongs to the Idriç HTML
extractor; the Android display harness must not grow a second HTML parser.

The HTML extractor retains links nested inside article paragraphs and retains
an enclosing anchor on an image as a linked-image item. A remote `src` is only a
fetch candidate: the background ICU path must validate the response and replace
it with an already-fetched source before the Android harness paints the image.

## Browser policy and the pre-decision probe

Before starting a heavyweight renderer, browser policy can inspect bounded
evidence:

- whether a cached pre-paint is already useful;
- known resource count and known/declared bytes;
- prior measurements for the site and device;
- DNS lookup state;
- a deliberately sampled route diagnostic;
- whether the page clearly requires JavaScript or another missing capability.

`bin/prepaint_predecision.grease` is a first shell boundary for this evidence.
It gives `dig` three seconds and asks for A plus NS data. It gives `traceroute`
ten seconds, eight hops, and one probe per hop. Missing, filtered, or failed
diagnostics remain data; they do not block the cheap paint.

DNS time and traceroute hop latency are not estimates of total page-load time.
They do not reveal asset count, transfer bandwidth, server computation, browser
main-thread work, JavaScript work, decoding, or layout. The probe therefore
labels them as diagnostics. Its only initial duration estimate is the explicit
lower bound:

```text
ceil(known asset bytes / assumed measured bytes per second)
```

At five minutes or more, the stub keeps the useful pre-paint and requires a user
or later policy decision before heavyweight escalation. A future policy should
use observed per-device/site throughput instead of inventing a network rate.

This probe is opt-in or sampled. `dig` and especially `traceroute` must not run on
every navigation.

## Developer workbench / inspector

The workbench is where browser-owned facts and diagnostic controls belong:

- logical-page universe and hot/warm/cold counts;
- resident renderer working set and memory estimates;
- cache/storage classification and bounded inspection;
- resource graph size and known byte totals;
- DNS and route probe results;
- the pre-decision evidence and reason for allowing or deferring escalation;
- controls for eviction, cache clearing, simulated pressure, and replay.

These facts may explain why a page stayed in pre-paint mode, but they are not the
page. A small status affordance can link from the page to the inspector without
injecting the diagnostic graph into page content.

## Search handoff

Search is browser chrome, not a fake form copied out of a workbench fixture:

```text
query words
  -> application/x-www-form-urlencoded query (spaces become +)
  -> https://www.google.com/search?q=...
  -> icu get URL
  -> Idriç text/link/image extraction
  -> disposable pre-paint
  -> Android display
```

`bin/icu_search.grease` records the shell boundary. In the old shell idiom,
`$*` is the expansion that joins positional parameters using the first `IFS`
character; `PS1` is only the interactive prompt. The current stub uses `jq` for
UTF-8 percent encoding and then changes encoded spaces to `+`. ICU, not curl or
WebView, performs the GET.

The standalone display APK has no Internet permission and cannot execute the
host ICU binary. It may show and copy the exact request so the interaction is
testable, but it must not pretend that local request construction is a fetched
result. The integrated browser shell will own the executable ICU/Android bridge.

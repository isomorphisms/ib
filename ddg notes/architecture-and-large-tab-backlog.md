# DDG notes: page-load architecture and a large tab backlog

These notes come from inspecting the `isomorphisms/ddg` fork of DuckDuckGo Android while thinking about IB's intended use case.

The main conclusion is **not** that DuckDuckGo is slow merely because it has many features. The more interesting issue is where work is placed in the navigation and tab-management paths.

IB has a different primary use case: a large durable backlog of pages that are worth remembering, with only a small number of pages actually active at once. An open tab is often closer to “I intend to read/check/use this later” than “keep a renderer alive for this right now.” The browser should preserve that intention without requiring bookmark/history housekeeping.

## Two different performance questions

Keep these separate.

### 1. Why is one page slow to become visible?

This is the navigation / pre-paint critical path.

Relevant DDG path:

- app/browser setup before `loadUrl()`;
- WebView construction and configuration;
- request interception on each resource;
- document/page-start privacy and JS injection;
- WebView/network/page execution;
- first visible commit and eventual page finish.

DDG's `BrowserWebViewClient.shouldInterceptRequest()` crosses a synchronous boundary for every WebView resource request. It uses `runBlocking`, hops to the main dispatcher to read `webView.url`, then runs DDG request interception.

The interceptor may perform malicious-site checks, request filtering, ad-click handling, HTTPS upgrading, GPC handling, allow/block-list checks, tracker detection, CNAME detection, and surrogate handling. This is especially important on pages with many subresources because the path is exercised repeatedly.

At `onPageStarted`, DDG also resolves active content-scope experiments and then invokes every `JsInjectorPlugin.onPageStarted()` before recording JS injection complete.

This is real hot-path work. Upstream DuckDuckGo performance PRs explicitly optimize it rather than simply deleting features:

- `duckduckgo/Android#9271`: `RealContentScopeScripts.getScript()` ran on the main thread for every page load. A cache-miss rebuild performed four sequential `String.replace()` passes over an approximately 515 KB content-scope JS blob. The PR replaced that with a single-pass assembly.
- `duckduckgo/Android#9335`: reduced per-navigation `getScript()` work, including quadratic string accumulation across roughly 22 plugins, repeated JSON/serialization work, repeated cohort reads, and unnecessary `runBlocking` when no content-scope experiment was active.
- `duckduckgo/Android#9351`: stopped resolving content-scope experiments from scratch on every navigation. The old path materialized roughly 580 feature toggles, performed roughly 1,140 key string splits, then enrolled/evaluated 22 content-scope experiment sub-toggles. The result is now cached until privacy configuration changes.
- `duckduckgo/Android#9350`: moved inbound content-scope JS messages off the WebView JavaBridge thread. The old synchronous bridge blocked page JS while doing work including a `runBlocking` hop to the main thread and reflective JSON parsing.
- `duckduckgo/Android#9161`: optimized tracker detection because `TrackerDetector.evaluate()` runs for every network request. Among other changes it removed a per-request database query and several repeated scans/allocations.

So DDG's own work supports a useful design rule for IB:

> Work that is logically “browser policy” can still be expensive if it is repeated on every navigation, every plugin, or every subresource request. Cache stable decisions and keep the per-request/per-navigation path very small.

### Cold launch is another subcase

DDG cold launch has additional browser-shell work before normal navigation. `LaunchBridgeActivity` goes through `LaunchViewModel.start()`, which seeds test state if needed and waits for installation-referrer data before deciding to open `BrowserActivity`.

That can affect “browser dead -> open URL -> see page,” but it cannot explain a slow navigation inside an already-running browser. IB benchmarks should therefore separate cold launch from warm navigation.

## DDG's own page-load measurements

DDG's page-load wide event treats these as distinct timings:

- page start -> page visible;
- page start -> page finish;
- page start -> escape from an early fixed progress state;
- page start -> content-scope experiments resolved;
- page start -> all JS injector plugins complete.

It also records:

- whether content-scope injection optimization is enabled;
- whether content-scope messaging optimization is enabled;
- whether experiment caching is enabled;
- number of active requests when the load starts;
- concurrent requests when it finishes;
- WebView version and foreground/background state.

The content-scope timings use millisecond buckets `0, 5, 10, 25, 50, 100, 200, 400, 800, 1600, 3200, 6400`, specifically because DDG considered the existing one-second page-load buckets far too coarse for this work.

One developer example in `duckduckgo/Android#9406` on Reddit recorded approximately:

- content-scope experiments resolved: 25 ms bucket;
- all JS injection complete: 50 ms bucket;
- page visible about 200 ms after the page-load flow began.

That is one diagnostic run, not an aggregate benchmark result. Its main significance is that DDG considers tens of milliseconds in this path worth measuring and optimizing.

## 2. Why does the same page get slower when there are ~200 tabs?

This is a different problem.

DDG does **not** intentionally keep 200 active WebView fragments. Its custom `FragmentStateAdapter` has `TabManager.MAX_ACTIVE_TABS = 15`; hidden fragments are retained only up to that limit and the oldest are removed after the limit is reached.

That is already much better than equating every logical tab with a resident renderer.

However, the metadata/control path still sees the whole tab population:

- `TabsDao.liveTabs()` selects the complete ordered list of open tab records;
- `BrowserViewModel.tabsFlow` maps the entire emitted list into `TabModel` values;
- selected-tab index calculation scans the tab list;
- `TabPagerAdapter` stores the whole list and compares old/new tab ID lists;
- its `containsItem()` lookup scans the list;
- tab URL/title/access updates feed back through the same tab-state machinery.

This means a foreground-tab change can cause work proportional to the number of logical tabs even though only a bounded number of WebViews are resident.

For a normal browser with dozens of tabs this may be acceptable. It is a poor fit for a browser whose deliberate use case includes hundreds or thousands of durable “remember this” items.

## IB interpretation

IB should make the distinction stronger than DDG does:

### Logical page / intention

Cheap, durable state on disk:

- canonical URL;
- current/final URL when known;
- title and small metadata;
- visit history;
- last-access time;
- user-interest / considered state;
- cached preview / pre-render references;
- renderer/session snapshot when useful;
- relationships to source pages or investigation tasks.

There may be hundreds or thousands of these.

### Resident renderer working set

Expensive, short-lived state:

- live renderer process/session;
- parsed DOM;
- JS heap;
- decoded images;
- layout/rendering state;
- active network work.

Only a small working set should be resident. The current workbench target of roughly 3–10 resident tabs is much closer to the intended IB use case than DDG's 15-active-fragment ceiling.

The cost of navigating one foreground tab should therefore be close to independent of whether the durable backlog contains 20, 200, or 10,000 URLs.

A 10,000-URL store is not a reason to instantiate 10,000 tab objects in a UI adapter or repeatedly map/compare the entire list during a foreground navigation.

## The backlog is not bookmarks

The intended backlog is not primarily a bookmark-management system.

A page can remain open because it is:

- something to read later;
- something to check when there is time;
- a product/book/job/tool to reconsider;
- a source connected to an ongoing investigation;
- something that was interesting enough not to discard yet;
- useful context whose importance is much greater than an arbitrary page from the open web.

The browser should preserve that signal automatically. The user should not have to stop doing the real task in order to classify bookmarks, curate folders, or remember which history entry mattered.

This suggests treating the durable backlog itself as an explicit interest signal. Items already retained/considered by the user should generally receive more prefetch, indexing, preview generation, archive recovery, and semantic attention than random URLs that merely exist on the web.

The backlog can age and hibernate without being forgotten.

## Performance invariant for IB

A useful regression target is:

> Increasing the durable logical-page population must not materially increase the foreground navigation critical path, except for deliberately bounded index/database work.

Test the same target page with at least:

- 1 logical tab;
- 5;
- 15;
- 30;
- 100;
- 200;
- eventually 1,000 and 10,000 durable URL records.

Measure both:

1. foreground `load/navigation requested -> first visible useful content`;
2. renderer/session activation cost for a hibernated page.

If latency rises until the resident-renderer cap and then flattens, renderer residency is the problem.

If latency continues to rise with the durable tab count, some metadata/database/list/index path is accidentally O(total logical tabs) on the foreground path.

That second failure is exactly what IB's architecture should prevent.

## DDG remains useful as a reference

The DDG inspection is still valuable even if IB remains the better architecture for this use case.

DDG demonstrates several useful boundaries:

- system WebView can be wrapped with substantial privacy/navigation policy;
- WebView sessions can be serialized separately from durable tab records;
- renderer/fragments can be bounded rather than kept for every logical tab;
- page-visible and page-finish are different events;
- browser privacy features need not be deleted to get faster: stable work can be cached and repeated hot-path work can be reduced.

The main thing not to inherit blindly is the assumption that the entire open-tab collection is small enough to flow through ordinary Android list/fragment machinery on foreground state changes.

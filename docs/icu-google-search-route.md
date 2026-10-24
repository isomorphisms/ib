# External search through IB and ICU

Requested on 2026-10-24 after the physical MIRO C67 ChordReader development
APK installed and launched. Selecting **Search web** currently leaves the
application through a generic Android browser intent; the user sees DDG.
That is not an IB search or an ICU acquisition.

## Browser-owned route

```text
ChordReader search input (unescaped UTF-8 terms)
  → explicit Android ACTION_SEARCH directed to the installed IB package
    (SearchManager query extra: "query"; never ACTION_VIEW or implicit browser)
  → IB validates the incoming search and records the search occurrence
  → IB.WebSearch.google_icu_url builds one request target
  → existing IB Binder-shaped ICU capability
  → ICU GET over HTTPS
  → checked response metadata and exact source bytes in the canonical Arena
  → IB reader's authorized extraction, history and view
```

`IB.WebSearch.google_icu_url` is the **pure typed request-address decision**.
It uses `https://google.com/search?q=` followed by UTF-8
application/x-www-form-urlencoded query terms. Ordinary spaces are `+`
(the requested `IFS=+` joining convention). `+`, `&`, `#`, `/`,
apostrophes, percent signs and non-ASCII characters are percent-escaped as
UTF-8 bytes. Empty/all-whitespace input, control characters and URLs above
2,048 characters are refused. Raw user text never becomes shell syntax.

The source application must pass the **original, unescaped** query once.
Only IB constructs the provider URL, so callers do not each implement
Google encoding or own the default provider. ChordReader may append its
user-visible `chords` search context before handoff, but IB owns the URL.

## Status and acceptance boundaries

- The typed URL constructor and refusal/encoding cases are now on this
  proposed IB branch, exercised by `Smoke.idric` / the core CI lane.
- The current NativeReader branch `native/no-java-frontend` is a text reader;
  its `org.isomorphisms.ib.prepaint` package receives plain-text shares,
  **not** SEARCH, ICU responses, or rendered search results. This package is
  a development identity, not a permanent production app identity.
- The real ICU→Shizuku→Arena work is in the existing IB #152/#153/#155
  stack, with source extraction at IB #154 and the checked NativeReader at
  #147. It is **not** currently a working C67 search-results APK.
- A production handoff requires an exported SEARCH receiver with appropriate
  query validation and a completed native/checked Idriç-to-ICU navigation
  route. An Android Activity that merely displays the query is **not**
  equivalent to completing search. Do not add Java or a WebView transport.
- No implicit `ACTION_VIEW`, DDG/Startpage, app-side Java URLConnection, or
  hidden alternative browser is an acceptable fallback for this route. If IB
  cannot accept the request, show an explicit unavailable state in ChordReader.
- Live Google may return consent, bot-check, HTML without explicit UTF-8
  charset, or other responses the existing Arena admission policy refuses.
  Record the actual HTTP/metadata outcome. Do not weaken ICU/Arena checks
  merely to claim a search succeeded.
- Installation of ChordReader on C67 and its launch are **user-reported PASS**
  for the prior candidate. These do not qualify this not-yet-built integration.

## Acceptance

A qualified C67 build must show one unescaped source term request; the exact
Google URL with encoded spaces/Unicode and reserved characters; an
authenticated routing handoff to IB; ICU rather than a generic Android
browser making the HTTPS request; an admitted committed response or
accurately reported refusal; history/source identity; click/open navigation;
and complete physical-device tests. A fake URL builder or hosted mock does
not accept the phone path.

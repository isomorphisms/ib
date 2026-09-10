# IB Material 3 arXiv viewer stub

This is a deliberately separate Material 3 / Jetpack Compose experiment for IB.
It does not replace `android-prepaint/` and does not change the pre-paint display
contract.

The viewport consumes the **Pensieve**, not the Cauldron. Cauldron owns fetched
source material and provenance; the renderer should not know whether readable text
came from a PDF, HTML, an abstract page, or some later distillation method.

The viewport now also returns local display observations to Pensieve state:

```text
Pensieve
   |
   v
Scaffold
└── Column
    ├── TopAppBar
    ├── LazyColumn
    │   ├── arXiv 1107.0595
    │   ├── arXiv 2203.11355
    │   ├── arXiv 1901.09021
    │   ├── arXiv 1606.05336
    │   └── arXiv 2305.00241
    └── Surface
        └── SelectionContainer
            └── LazyColumn
                ├── article heading
                └── stable source-range fragments
                       |
                       v
                 view observations
                       |
                       v
                 Pensieve view history
```

For the deterministic APK stub, a frozen Pensieve-shaped snapshot lives under
`app/src/main/assets/pensieve/arxiv/`. The app discovers the item directories
there rather than carrying a second article list in Kotlin.

For each item the viewport reads `title`, then chooses the first available text
representation in this order:

```text
text/from-pdf.txt
text/from-html.txt
text/from-abstract.txt
```

That ordering is a display choice, not a claim that PDF is canonically superior.
The important boundary is that all three are Pensieve representations. The
viewport does not open `paper.pdf`, parse HTML, fetch arXiv, or inspect Cauldron.

The five asset directories correspond to the first 0.2 arXiv corpus. At present
only `2203.11355` contains a bundled text representation: it is the deterministic
PDF fixture used by the 0.2 acceptance test to prove
Cauldron -> `pdftotext` -> `Pensieve/.../text/from-pdf.txt`. The other four entries
remain visible by arXiv ID until their real frozen Pensieve representations are
materialized. Their titles are therefore ID labels, not invented paper titles.

The paper body is Material `Text`, not `TextField`: reading, selection, and copying
are the current requirements. Editing the source paper is not. Each non-empty
source line is currently one displayed fragment in the reader's `LazyColumn`.
That cut is intentionally replaceable.

There is no network permission and no HTML parser in this prototype.

## View observations and exposure

`ViewExposure` is model-facing code with no Compose dependency. The Compose reader
feeds it viewport snapshots containing stable fragment ids, visible pixel extent,
layout position, and movement between samples. The model records observations and
derives weighted fragment exposure from them.

A complete **view unit** is currently seven seconds of weighted exposure. Partial
visibility contributes proportionally. Small repeated scrolling continues the same
cumulative exposure; it does not start a new visit. Large fast movement is reduced
or suppressed. Exposure below seven seconds is retained, and returning to a
fragment later continues its previous total.

The Android adapter samples every 500 ms and batches durable appends approximately
every two seconds, with additional flushes when the reader is disposed or the
Activity stops. It does not synchronously write storage on every frame.

This is evidence about what IB displayed, not a measure of human attention,
comprehension, or eye position. Reader history is private local state under the
app's Pensieve state and never changes canonical article bytes or fragment
identity.

See `../docs/view-exposure.md` for the record format, weighting rule, durability
boundary, query command, and relationship to the persistent model in
`../docs/storage-model.md`.

## Live Pensieve later

The asset tree is only the deterministic snapshot adapter. A later integration can
replace `AssetManager` with a filesystem/process adapter for the live 0.2 Pensieve
without changing the viewport's `PensieveArticle` model. The Android renderer
should still receive distilled Pensieve text rather than reach backward into
Cauldron.

The view-observation path is likewise independent of asset versus live storage:
reader adapters emit fragment ids plus viewport evidence, while the local Pensieve
state owns persistence and queryability.

## Deferred interaction: movable pieces

Do **not** add dragging to the article or text model yet.

A later pass may make every visible piece independently movable by adding a
presentation/interaction layer above the semantic objects, for example:

```text
article/text/image/control
        |
        v
rendered piece
        |
        +-- position
        +-- movable
        +-- drag state
```

That layer should be orthogonal: `PensieveArticle`, article text, list membership,
and the eventual IB information objects should not acquire drag coordinates or
pointer-event fields merely because one Android renderer supports moving pieces.

When that pass happens, start by proving movement on one generic rendered-piece
wrapper, then apply the wrapper to text, article rows, images, and controls. Avoid
creating parallel types such as `DraggableText`, `DraggableImage`, and
`DraggableArticle`.

## Android stack

This stub currently declares:

- Compose BOM `2026.08.00`;
- Material 3 from that BOM;
- AndroidX Activity Compose `1.13.0`;
- compile SDK 37;
- Android Gradle Plugin 9.1.2;
- Kotlin/Compose compiler plugin 2.4.20.

`targetSdk` remains 36 for this experiment. The purpose here is to establish the
viewport shape, not to turn the Material prototype into the canonical IB Android
architecture.

## Build

With JDK 17, Android SDK 37, and a Gradle version accepted by AGP 9.1.2:

```text
cd android-material3
gradle --no-daemon :app:assembleDebug
```

The expected APK is `app/build/outputs/apk/debug/app-debug.apk`.

The host-side exposure model and real Pensieve fixture can be exercised without an
Android SDK:

```text
sh tests/test_view_exposure.grease
```

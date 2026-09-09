# IB Material 3 arXiv viewer stub

This is a deliberately separate Material 3 / Jetpack Compose experiment for IB.
It does not replace `android-prepaint/` and does not change the pre-paint display
contract.

The viewport consumes the **Pensieve**, not the Cauldron. Cauldron owns fetched
source material and provenance; the renderer should not know whether readable text
came from a PDF, HTML, an abstract page, or some later distillation method.

The first viewport is intentionally small:

```text
Pensieve
   |
   v
Scaffold
└── Row
    ├── left LazyColumn
    │   ├── Pinned
    │   └── Recent
    └── Surface
        └── SelectionContainer
            └── scrollable Column
                └── Text
```

The left rail models two independent pieces of presentation state:

- `recentOrder`: selecting an item moves it to the front;
- `pinnedOrder`: pinning puts an item in the pinned section without rewriting its
  underlying recent position.

Pinned items are omitted from the visible Recent section only while they are pinned.
Selecting a pinned item still updates its underlying recency, so unpinning returns it
to the correct current place in Recent. The pin and recency state is intentionally
in-memory in this renderer experiment; it is not added to `PensieveArticle` and does
not claim to be canonical browser history or durable Pensieve state.

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
are the current requirements. Editing the source paper is not. The enclosing
`Surface` provides the visual text-box boundary.

There is no network permission and no HTML parser in this prototype.

## Live Pensieve later

The asset tree is only the deterministic snapshot adapter. A later integration can
replace `AssetManager` with a filesystem/process adapter for the live 0.2 Pensieve
without changing the viewport's `PensieveArticle` model. The Android renderer
should still receive distilled Pensieve text rather than reach backward into
Cauldron.

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

# IB Material 3 arXiv viewer stub

This is a deliberately separate Material 3 / Jetpack Compose experiment for IB.
It does not replace `android-prepaint/` and does not change the pre-paint display
contract.

The first viewport is intentionally small:

```text
Scaffold
└── Column
    ├── TopAppBar
    ├── LazyColumn
    │   ├── frozen arXiv article 1
    │   ├── frozen arXiv article 2
    │   ├── frozen arXiv article 3
    │   ├── frozen arXiv article 4
    │   └── frozen arXiv article 5
    └── Surface
        └── SelectionContainer
            └── scrollable Column
                └── Text
```

The five records in `MainActivity.kt` are placeholders. They are deliberately
named as fixtures instead of inventing arXiv IDs or titles. Replace them with the
actual frozen Cauldron records when that corpus is connected.

The paper body is `Text`, not `TextField`: reading, selection, and copying are the
current requirements. Editing the source paper is not. The enclosing `Surface`
provides the visual text-box boundary.

There is no network permission and no HTML parser in this prototype.

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

That layer should be orthogonal: `FrozenArticle`, article text, list membership,
and the eventual IB information objects should not acquire drag coordinates or
pointer-event fields merely because one Android renderer supports moving pieces.

When that pass happens, start by proving movement on one generic rendered-piece
wrapper, then apply the wrapper to text, article rows, images, and controls. Avoid
creating parallel types such as `DraggableText`, `DraggableImage`, and
`DraggableArticle`.

## Android stack

This stub follows the current stable Compose setup at the time it was written:

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

# Prepaint display interchange

`ib-prepaint` is a disposable, renderer-neutral stream for the small Android
prepaint harness. It is not canonical history, a DOM serialization, or a promise
that this will be IB's permanent cache encoding.

## Version 1

The file is UTF-8, tab-separated, and begins with:

```text
ib-prepaint    1
```

The whitespace between fields above represents one literal tab. Each revision is
a complete replacement projection, not a patch:

```text
revision    1    partial
requested-url    https://example.test/requested
resolved-url    https://example.test/resolved
title    Example
heading    1    First heading
text    Visible paragraph text
link    Link label    /target
row    first cell    second cell
form    Search    /search
image    content://already-fetched-image    Alternate text    Caption
end
```

The revision sequence must strictly increase. Its state is `partial` or
`complete`. A consumer can paint a partial revision immediately and replace it
with each later revision without retaining or interpreting the original HTML.

Backslash escapes are `\\`, `\t`, `\n`, and `\r`. Heading levels are 1 through 6.
Rows have one or more cells. An image has an already-fetched source, alternate
text, and caption.

The harness bounds one artifact to 4 MiB of UTF-8-decoded text, 32 revisions, and
4,096 information blocks per revision. Those are viewer safety limits, not an
invitation for the producer to fill them.

The Android harness accepts these local image sources:

- `content:` and `file:` URIs;
- `data:image/...;base64,...` values;
- `asset:` and `resource:` names for deterministic bundled fixtures.

It never fetches an image from the network. It preserves the decoded image's
colors and aspect ratio, fits it within the phone width, and applies no tint,
inversion, CSS, or crop.

The harness's **Open** control reads an artifact through Android's document picker.
**Replay** reopens that document before replaying its revisions, so a producer can
replace the disposable file and the viewer can display the new projection without
being rebuilt.

Version 1 directly covers the present Idriç `InformationView` fields: requested
URL, resolved URL, title, headings, text blocks, links, table rows, and forms. The
`image` record is the narrow addition needed to exercise the requested
text-plus-images paint surface while image extraction is developed independently.

Deleting this stream and every referenced cached image must leave tabs, history,
labels, snapshots, and other durable browser records intact.

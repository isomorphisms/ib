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
image    content://already-fetched-image    Alternate text    Caption    /optional-link
end
```

The revision sequence must strictly increase. Its state is `partial` or
`complete`. A consumer can paint a partial revision immediately and replace it
with each later revision without retaining or interpreting the original HTML.

Backslash escapes are `\\`, `\t`, `\n`, and `\r`. Heading levels are 1 through 6.
Rows have one or more cells. An image has an already-fetched source, alternate
text, caption, and an optional navigation target. The fifth field may be omitted
for an unlinked image. Keeping the target allows an HTML anchor wrapping an image
to remain both a painted image and a usable link.

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

The harness's **Open** control reads a file through Android's document picker. A
file whose first line starts with `ib-prepaint` is parsed strictly as this
interchange. Any other bounded UTF-8 text file is painted as plain text: blank
lines delimit paragraphs and a line consisting of one absolute `http://` or
`https://` URL becomes a link. This fallback is for actual text, URL lists, and
already-extracted article bodies; it is not a second HTML parser.

**Replay** reopens a selected document before replaying its revisions, so a
producer can replace the disposable file and the viewer can display the new
projection without being rebuilt. If a replacement becomes unreadable, the last
valid page stays visible.

Version 1 directly covers the present Idriç `InformationView` fields: requested
URL, resolved URL, title, headings, text blocks, links, table rows, and forms. The
`image` record is the narrow addition needed to exercise the requested
text-plus-images paint surface while image extraction is developed independently.

Deleting this stream and every referenced cached image must leave tabs, history,
labels, snapshots, and other durable browser records intact.

The display harness may expose a search/navigation control, but fetching is not
part of this format. The browser core resolves that request, ICU fetches bytes,
and a producer emits a later pre-paint. DNS/route/resource-cost diagnostics belong
to browser policy or the developer inspector, never as synthetic page content.

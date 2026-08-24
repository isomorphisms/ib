package org.isomorphisms.ib.prepaint;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Base64;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

public final class PrepaintActivity extends Activity {
    private static final int OPEN_PREPAINT = 7;
    private static final long REPAINT_DELAY_MILLIS = 1400;
    private static final int BACKGROUND = Color.rgb(18, 21, 24);
    private static final int SURFACE = Color.rgb(28, 33, 38);
    private static final int FOREGROUND = Color.rgb(245, 247, 248);
    private static final int SECONDARY = Color.rgb(185, 193, 200);
    private static final int LINK = Color.rgb(139, 196, 255);
    private static final int RULE = Color.rgb(57, 66, 74);

    private final Handler handler = new Handler(Looper.getMainLooper());
    private final Runnable advance = this::advanceRevision;

    private Typeface hack;
    private PrepaintDocument document;
    private LinearLayout page;
    private ScrollView scroll;
    private TextView status;
    private int revisionIndex;
    private Uri openedDocument;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        configureWindow();
        hack = Typeface.createFromAsset(getAssets(), "fonts/Hack-Regular.ttf");
        setContentView(buildScreen());
        try {
            document = loadSample();
            replayRepaint();
        } catch (IOException error) {
            showFailure(error.getMessage());
        }
    }

    @Override
    protected void onDestroy() {
        handler.removeCallbacks(advance);
        super.onDestroy();
    }

    private void configureWindow() {
        Window window = getWindow();
        window.setStatusBarColor(BACKGROUND);
        window.setNavigationBarColor(BACKGROUND);
        window.getDecorView().setSystemUiVisibility(0);
    }

    private View buildScreen() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(BACKGROUND);

        LinearLayout chrome = new LinearLayout(this);
        chrome.setOrientation(LinearLayout.HORIZONTAL);
        chrome.setGravity(Gravity.CENTER_VERTICAL);
        chrome.setPadding(dp(14), dp(8), dp(8), dp(8));
        chrome.setBackgroundColor(SURFACE);

        status = text("IB PREPAINT", 12, SECONDARY);
        chrome.addView(status, new LinearLayout.LayoutParams(0,
                ViewGroup.LayoutParams.WRAP_CONTENT, 1));

        Button open = chromeButton("Open");
        open.setOnClickListener(ignored -> openPrepaint());
        chrome.addView(open);

        Button replay = chromeButton("Replay");
        LinearLayout.LayoutParams replayLayout = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        replayLayout.setMargins(dp(7), 0, 0, 0);
        replay.setOnClickListener(ignored -> reloadAndReplay());
        chrome.addView(replay, replayLayout);
        root.addView(chrome, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setBackgroundColor(BACKGROUND);
        page = new LinearLayout(this);
        page.setOrientation(LinearLayout.VERTICAL);
        page.setPadding(dp(18), dp(18), dp(18), dp(40));
        scroll.addView(page, new ScrollView.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        root.addView(scroll, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        return root;
    }

    private PrepaintDocument loadSample() throws IOException {
        try (InputStream input = getAssets().open("sample.prepaint");
             InputStreamReader reader = new InputStreamReader(input, StandardCharsets.UTF_8)) {
            return PrepaintDocument.parse(reader);
        }
    }

    private void openPrepaint() {
        Intent request = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        request.addCategory(Intent.CATEGORY_OPENABLE);
        request.setType("*/*");
        startActivityForResult(request, OPEN_PREPAINT);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != OPEN_PREPAINT || resultCode != RESULT_OK
                || data == null || data.getData() == null) {
            return;
        }
        Uri selected = data.getData();
        try {
            getContentResolver().takePersistableUriPermission(
                    selected, Intent.FLAG_GRANT_READ_URI_PERMISSION);
        } catch (SecurityException | IllegalArgumentException ignored) {
            // The one-time read grant is enough for this deliberately temporary harness.
        }
        openedDocument = selected;
        reloadAndReplay();
    }

    private void reloadAndReplay() {
        try {
            document = openedDocument == null ? loadSample() : loadDocument(openedDocument);
            replayRepaint();
        } catch (IOException | SecurityException error) {
            showFailure(error.getMessage());
        }
    }

    private PrepaintDocument loadDocument(Uri source) throws IOException {
        try (InputStream input = getContentResolver().openInputStream(source)) {
            if (input == null) {
                throw new IOException("The selected prepaint could not be opened.");
            }
            try (InputStreamReader reader = new InputStreamReader(input, StandardCharsets.UTF_8)) {
                return PrepaintDocument.parse(reader);
            }
        }
    }

    private void replayRepaint() {
        if (document == null) {
            return;
        }
        handler.removeCallbacks(advance);
        revisionIndex = 0;
        renderRevision(document.revisions.get(revisionIndex));
        scheduleAdvance();
    }

    private void advanceRevision() {
        if (document == null || revisionIndex + 1 >= document.revisions.size()) {
            return;
        }
        revisionIndex += 1;
        renderRevision(document.revisions.get(revisionIndex));
        scheduleAdvance();
    }

    private void scheduleAdvance() {
        if (revisionIndex + 1 < document.revisions.size()) {
            handler.postDelayed(advance, REPAINT_DELAY_MILLIS);
        }
    }

    private void renderRevision(PrepaintDocument.Revision revision) {
        int oldScroll = scroll.getScrollY();
        page.removeAllViews();
        status.setText("IB PREPAINT  " + (revisionIndex + 1) + "/"
                + document.revisions.size() + "  ·  "
                + (revision.complete ? "complete" : "partial"));

        if (!revision.title.isEmpty()) {
            TextView title = text(revision.title, 25, FOREGROUND);
            title.setTypeface(hack, Typeface.BOLD);
            addBlock(title, 0, 0, 0, 11);
        }
        String visibleUrl = revision.resolvedUrl.isEmpty()
                ? revision.requestedUrl : revision.resolvedUrl;
        if (!visibleUrl.isEmpty()) {
            TextView url = text(visibleUrl, 12, SECONDARY);
            url.setTextIsSelectable(true);
            addBlock(url, 0, 0, 0, 19);
        }

        for (PrepaintDocument.Block block : revision.blocks) {
            addInformationBlock(block);
        }
        scroll.post(() -> scroll.scrollTo(0, oldScroll));
    }

    private void addInformationBlock(PrepaintDocument.Block block) {
        switch (block.kind) {
            case PrepaintDocument.Block.HEADING:
                float headingSize = block.level == 1 ? 20 : block.level == 2 ? 17 : 15;
                TextView heading = text(block.values.get(0), headingSize, FOREGROUND);
                heading.setTypeface(hack, Typeface.BOLD);
                addBlock(heading, 0, block.level == 1 ? 8 : 4, 0, 8);
                break;
            case PrepaintDocument.Block.TEXT:
                TextView paragraph = text(block.values.get(0), 15, FOREGROUND);
                paragraph.setLineSpacing(0, 1.18f);
                paragraph.setTextIsSelectable(true);
                addBlock(paragraph, 0, 0, 0, 14);
                break;
            case PrepaintDocument.Block.LINK:
                TextView link = text(block.values.get(0) + "\n" + block.values.get(1), 14, LINK);
                link.setTextIsSelectable(true);
                addBlock(link, 0, 2, 0, 14);
                break;
            case PrepaintDocument.Block.ROW:
                addRow(block);
                break;
            case PrepaintDocument.Block.FORM:
                TextView form = text("[ " + block.values.get(0) + " ]  →  "
                        + block.values.get(1), 14, FOREGROUND);
                form.setPadding(dp(12), dp(10), dp(12), dp(10));
                form.setBackground(box(SURFACE, dp(4)));
                addBlock(form, 0, 3, 0, 14);
                break;
            case PrepaintDocument.Block.IMAGE:
                addImage(block);
                break;
            default:
                throw new IllegalStateException("unhandled block " + block.kind);
        }
    }

    private void addRow(PrepaintDocument.Block block) {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.TOP);
        row.setPadding(dp(1), dp(1), dp(1), dp(1));
        row.setBackgroundColor(RULE);
        for (String cellText : block.values) {
            TextView cell = text(cellText, 13, FOREGROUND);
            cell.setTextIsSelectable(true);
            cell.setPadding(dp(9), dp(9), dp(9), dp(9));
            cell.setBackgroundColor(SURFACE);
            LinearLayout.LayoutParams cellLayout = new LinearLayout.LayoutParams(
                    0, ViewGroup.LayoutParams.WRAP_CONTENT, 1);
            cellLayout.setMargins(dp(1), dp(1), dp(1), dp(1));
            row.addView(cell, cellLayout);
        }
        addBlock(row, 0, 0, 0, 3);
    }

    private void addImage(PrepaintDocument.Block block) {
        String source = block.values.get(0);
        String alternate = block.values.get(1);
        String caption = block.values.get(2);
        Drawable drawable = loadDrawable(source);
        if (drawable == null) {
            TextView missing = text("[image] " + alternate, 13, SECONDARY);
            addBlock(missing, 0, 3, 0, 12);
            return;
        }

        ImageView image = new ImageView(this);
        image.setAdjustViewBounds(true);
        image.setScaleType(ImageView.ScaleType.FIT_CENTER);
        image.setContentDescription(alternate);
        image.setImageDrawable(drawable);
        addBlock(image, 0, 5, 0, caption.isEmpty() ? 14 : 5);

        if (!caption.isEmpty()) {
            TextView captionView = text(caption, 12, SECONDARY);
            captionView.setLineSpacing(0, 1.12f);
            addBlock(captionView, 0, 0, 0, 14);
        }
    }

    private Drawable loadDrawable(String source) {
        try {
            if (source.startsWith("resource:")) {
                String name = source.substring("resource:".length());
                int identifier = getResources().getIdentifier(name, "drawable", getPackageName());
                return identifier == 0 ? null : getDrawable(identifier);
            }
            if (source.startsWith("asset:")) {
                try (InputStream input = getAssets().open(source.substring("asset:".length()))) {
                    return Drawable.createFromStream(input, source);
                }
            }
            if (source.startsWith("data:image/") && source.contains(",")) {
                int comma = source.indexOf(',');
                byte[] bytes = Base64.decode(source.substring(comma + 1), Base64.DEFAULT);
                try (InputStream input = new ByteArrayInputStream(bytes)) {
                    return Drawable.createFromStream(input, "inline prepaint image");
                }
            }
            Uri uri = Uri.parse(source);
            if ("content".equals(uri.getScheme()) || "file".equals(uri.getScheme())) {
                try (InputStream input = getContentResolver().openInputStream(uri)) {
                    return input == null ? null : Drawable.createFromStream(input, source);
                }
            }
        } catch (IOException | IllegalArgumentException | SecurityException ignored) {
            return null;
        }
        return null;
    }

    private void showFailure(String message) {
        handler.removeCallbacks(advance);
        page.removeAllViews();
        status.setText("IB PREPAINT  ·  invalid artifact");
        TextView failure = text(message == null ? "Could not read prepaint." : message,
                14, FOREGROUND);
        addBlock(failure, 0, 0, 0, 0);
    }

    private TextView text(String value, float sp, int color) {
        TextView view = new TextView(this);
        view.setText(value);
        view.setTextSize(sp);
        view.setTextColor(color);
        view.setTypeface(hack);
        view.setIncludeFontPadding(false);
        return view;
    }

    private Button chromeButton(String label) {
        Button button = new Button(this);
        button.setAllCaps(false);
        button.setText(label);
        button.setTextColor(FOREGROUND);
        button.setTextSize(12);
        button.setTypeface(hack);
        button.setMinHeight(0);
        button.setMinWidth(0);
        button.setPadding(dp(14), dp(7), dp(14), dp(7));
        button.setBackground(box(RULE, dp(5)));
        return button;
    }

    private void addBlock(View view, int left, int top, int right, int bottom) {
        LinearLayout.LayoutParams layout = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        layout.setMargins(dp(left), dp(top), dp(right), dp(bottom));
        page.addView(view, layout);
    }

    private GradientDrawable box(int color, int radius) {
        GradientDrawable drawable = new GradientDrawable();
        drawable.setColor(color);
        drawable.setCornerRadius(radius);
        return drawable;
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }
}

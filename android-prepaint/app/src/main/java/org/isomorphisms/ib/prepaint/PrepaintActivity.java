package org.isomorphisms.ib.prepaint;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.OpenableColumns;
import android.text.InputType;
import android.util.Base64;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;

public final class PrepaintActivity extends Activity {
    private static final int OPEN_PREPAINT = 7;
    private static final int GRANT_TERMUX_COMMAND = 8;
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
    private EditText searchInput;
    private int revisionIndex;
    private Uri openedDocument;
    private PrepaintDocument.Revision currentRevision;
    private String permissionPendingUrl;
    private String permissionPendingTitle;

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
        if (!handleTermuxResult(getIntent())
                && TermuxPrepaintClient.hasActiveRequest(this)) {
            status.setText("IB PREPAINT  ·  fetching through ICU");
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        handleTermuxResult(intent);
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
        root.addView(buildSearchBar(), new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        return root;
    }

    private View buildSearchBar() {
        LinearLayout bar = new LinearLayout(this);
        bar.setOrientation(LinearLayout.HORIZONTAL);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        bar.setPadding(dp(9), dp(7), dp(9), dp(9));
        bar.setBackgroundColor(SURFACE);

        searchInput = new EditText(this);
        searchInput.setSingleLine(true);
        searchInput.setHint("Search or URL");
        searchInput.setHintTextColor(SECONDARY);
        searchInput.setTextColor(FOREGROUND);
        searchInput.setTextSize(14);
        searchInput.setTypeface(hack);
        searchInput.setInputType(InputType.TYPE_CLASS_TEXT);
        searchInput.setImeOptions(EditorInfo.IME_ACTION_SEARCH);
        searchInput.setPadding(dp(11), dp(8), dp(11), dp(8));
        searchInput.setBackground(box(BACKGROUND, dp(5)));
        searchInput.setOnEditorActionListener((view, action, event) -> {
            if (action == EditorInfo.IME_ACTION_SEARCH) {
                requestSearch();
                return true;
            }
            return false;
        });
        bar.addView(searchInput, new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));

        Button search = chromeButton("Go");
        LinearLayout.LayoutParams searchLayout = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        searchLayout.setMargins(dp(7), 0, 0, 0);
        search.setOnClickListener(ignored -> requestSearch());
        bar.addView(search, searchLayout);
        return bar;
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
        request.setType("text/*");
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
        try {
            PrepaintDocument selectedDocument = loadDocument(selected, displayName(selected));
            openedDocument = selected;
            document = selectedDocument;
            replayRepaint();
        } catch (IOException | SecurityException error) {
            showOpenFailure(error.getMessage());
        }
    }

    private void reloadAndReplay() {
        if (openedDocument == null) {
            replayRepaint();
            return;
        }
        try {
            document = loadDocument(openedDocument, displayName(openedDocument));
            replayRepaint();
        } catch (IOException | SecurityException error) {
            showOpenFailure(error.getMessage());
        }
    }

    private PrepaintDocument loadDocument(Uri source, String title) throws IOException {
        try (InputStream input = getContentResolver().openInputStream(source)) {
            if (input == null) {
                throw new IOException("The selected text could not be opened.");
            }
            try (InputStreamReader reader = new InputStreamReader(input,
                    StandardCharsets.UTF_8.newDecoder()
                            .onMalformedInput(CodingErrorAction.REPORT)
                            .onUnmappableCharacter(CodingErrorAction.REPORT))) {
                return PrepaintDocument.parseOrPlainText(reader, title);
            }
        }
    }

    private String displayName(Uri source) {
        try (Cursor cursor = getContentResolver().query(source,
                new String[] {OpenableColumns.DISPLAY_NAME}, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int column = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (column >= 0) {
                    String name = cursor.getString(column);
                    if (name != null && !name.trim().isEmpty()) {
                        return name;
                    }
                }
            }
        } catch (RuntimeException ignored) {
            // A provider is allowed to omit display metadata.
        }
        String segment = source.getLastPathSegment();
        return segment == null || segment.isEmpty() ? "Plain text" : segment;
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
        currentRevision = revision;
        int oldScroll = scroll.getScrollY();
        page.removeAllViews();
        if (PrepaintDocument.TEXT_SOURCE.equals(document.sourceKind)) {
            status.setText("IB PREPAINT  ·  plain text");
        } else {
            status.setText("IB PREPAINT  " + (revisionIndex + 1) + "/"
                    + document.revisions.size() + "  ·  "
                    + (revision.complete ? "complete" : "partial"));
        }

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
                String label = block.values.get(0);
                String target = block.values.get(1);
                TextView link = text(label.equals(target) ? target : label + "\n" + target,
                        14, LINK);
                link.setPadding(dp(8), dp(5), dp(8), dp(5));
                link.setBackground(box(SURFACE, dp(4)));
                if (!target.isEmpty()) {
                    link.setOnClickListener(ignored -> requestLink(target));
                }
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
        String target = block.values.get(3);
        Drawable drawable = loadDrawable(source);
        if (drawable == null) {
            TextView missing = text("[image] " + alternate, 13, SECONDARY);
            if (!target.isEmpty()) {
                missing.setTextColor(LINK);
                missing.setOnClickListener(ignored -> requestLink(target));
            }
            addBlock(missing, 0, 3, 0, 12);
            return;
        }

        ImageView image = new ImageView(this);
        image.setAdjustViewBounds(true);
        image.setScaleType(ImageView.ScaleType.FIT_CENTER);
        image.setContentDescription(alternate);
        image.setImageDrawable(drawable);
        if (!target.isEmpty()) {
            image.setClickable(true);
            image.setFocusable(true);
            image.setOnClickListener(ignored -> requestLink(target));
        }
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

    private void showOpenFailure(String message) {
        String visible = message == null ? "Could not read the selected text." : message;
        Toast.makeText(this, visible, Toast.LENGTH_LONG).show();
    }

    private void requestSearch() {
        String input = searchInput.getText().toString().trim();
        if (input.isEmpty()) {
            Toast.makeText(this, "Type a search or URL first.", Toast.LENGTH_SHORT).show();
            return;
        }
        String url = UrlRecognition.navigationUrl(input);
        String title = UrlRecognition.isAbsoluteHttpUrl(input)
                ? input : "Search: " + input;
        requestNavigation(url, title);
    }

    private void requestLink(String target) {
        String base = currentRevision == null ? "" : currentRevision.resolvedUrl;
        if (base.isEmpty() && currentRevision != null) {
            base = currentRevision.requestedUrl;
        }
        String resolved = UrlRecognition.resolve(base, target);
        if (!UrlRecognition.isAbsoluteHttpUrl(resolved)) {
            Toast.makeText(this, "This link has no absolute HTTP target yet.",
                    Toast.LENGTH_LONG).show();
            return;
        }
        requestNavigation(resolved, "Link request");
    }

    private void requestNavigation(String url, String title) {
        if (!UrlRecognition.isAbsoluteHttpUrl(url)) {
            Toast.makeText(this, "The HTTP(S) address is invalid or exceeds 2,048 characters.",
                    Toast.LENGTH_LONG).show();
            return;
        }
        if (checkSelfPermission(TermuxPrepaintClient.TERMUX_PERMISSION)
                != PackageManager.PERMISSION_GRANTED) {
            permissionPendingUrl = url;
            permissionPendingTitle = title;
            requestPermissions(new String[] {TermuxPrepaintClient.TERMUX_PERMISSION},
                    GRANT_TERMUX_COMMAND);
            return;
        }
        startTermuxNavigation(url, title);
    }

    private void startTermuxNavigation(String url, String title) {
        String error = TermuxPrepaintClient.start(this, url);
        if (error != null) {
            Toast.makeText(this, error, Toast.LENGTH_LONG).show();
            restoreStatus();
            return;
        }
        status.setText("IB PREPAINT  ·  fetching through ICU");
        Toast.makeText(this, title, Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
            int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode != GRANT_TERMUX_COMMAND) {
            return;
        }
        String url = permissionPendingUrl;
        String title = permissionPendingTitle;
        permissionPendingUrl = null;
        permissionPendingTitle = null;
        if (grantResults.length > 0
                && grantResults[0] == PackageManager.PERMISSION_GRANTED
                && url != null) {
            startTermuxNavigation(url, title == null ? url : title);
        } else {
            Toast.makeText(this,
                    "Grant IB permission to run its prepaint command in Termux.",
                    Toast.LENGTH_LONG).show();
            restoreStatus();
        }
    }

    private boolean handleTermuxResult(Intent intent) {
        if (intent == null
                || !TermuxPrepaintClient.RESULT_ACTION.equals(intent.getAction())) {
            return false;
        }
        final long requestId;
        final String nonce;
        final String requestedUrl;
        try {
            requestId = intent.getLongExtra(TermuxPrepaintClient.EXTRA_REQUEST_ID, -1L);
            nonce = intent.getStringExtra(TermuxPrepaintClient.EXTRA_REQUEST_NONCE);
            requestedUrl = intent.getStringExtra(TermuxPrepaintClient.EXTRA_REQUEST_URL);
        } catch (RuntimeException malformedEnvelope) {
            return true;
        }
        if (!TermuxPrepaintClient.isActiveResult(
                this, requestId, nonce, requestedUrl)) {
            return true;
        }

        TermuxPrepaintResult validated;
        try {
            Bundle result = intent.getBundleExtra("result");
            if (result == null) {
                validated = TermuxPrepaintResult.validate(requestedUrl,
                        null, null, null, null, -1, 0,
                        "Termux returned no result bundle");
            } else {
                validated = TermuxPrepaintResult.validate(requestedUrl,
                        result.getString("stdout"),
                        result.getString("stdout_original_length"),
                        result.getString("stderr"),
                        result.getString("stderr_original_length"),
                        result.getInt("exitCode", -1),
                        result.getInt("err", 0),
                        result.getString("errmsg"));
            }
        } catch (RuntimeException malformedResult) {
            validated = TermuxPrepaintResult.validate(requestedUrl,
                    null, null, null, null, -1, 0,
                    "Termux returned a malformed result bundle");
        }
        TermuxPrepaintClient.clearIfActive(this, requestId, nonce);
        if (!validated.accepted()) {
            Toast.makeText(this, validated.error, Toast.LENGTH_LONG).show();
            restoreStatus();
            return true;
        }

        document = validated.document;
        openedDocument = null;
        replayRepaint();
        return true;
    }

    private void restoreStatus() {
        if (document == null || currentRevision == null) {
            status.setText("IB PREPAINT");
            return;
        }
        if (PrepaintDocument.TEXT_SOURCE.equals(document.sourceKind)) {
            status.setText("IB PREPAINT  ·  plain text");
        } else {
            status.setText("IB PREPAINT  " + (revisionIndex + 1) + "/"
                    + document.revisions.size() + "  ·  "
                    + (currentRevision.complete ? "complete" : "partial"));
        }
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

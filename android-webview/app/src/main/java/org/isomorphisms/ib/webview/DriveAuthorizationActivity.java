package org.isomorphisms.ib.webview;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.IntentSender;
import android.net.Uri;
import android.os.Bundle;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.google.android.gms.auth.api.identity.AuthorizationClient;
import com.google.android.gms.auth.api.identity.AuthorizationRequest;
import com.google.android.gms.auth.api.identity.AuthorizationResult;
import com.google.android.gms.auth.api.identity.Identity;
import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.common.api.Scope;

import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

/** Narrow consent adapter. Browser state, credentials and corpus bytes live elsewhere. */
public final class DriveAuthorizationActivity extends Activity {
    private static final int CONSENT_REQUEST = 75;
    private static final String READ = "https://www.googleapis.com/auth/drive.readonly";
    private static final String FILE = "https://www.googleapis.com/auth/drive.file";
    private AuthorizationClient client;
    private Button authorize;
    private TextView status;
    private Request request;
    private boolean busy;
    private boolean destroyed;

    private static final class Request {
        final String clientId;
        final String state;
        final int port;
        final List<Scope> scopes;
        Request(String clientId, String state, int port, List<Scope> scopes) {
            this.clientId = clientId;
            this.state = state;
            this.port = port;
            this.scopes = scopes;
        }
    }

    @Override protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        client = Identity.getAuthorizationClient(this);
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        status = new TextView(this);
        authorize = new Button(this);
        authorize.setText("Authorize Drive");
        authorize.setEnabled(false);
        authorize.setOnClickListener(view -> startConsent());
        layout.addView(status);
        layout.addView(authorize);
        setContentView(layout);
        if (saved != null) {
            status.setText("Authorization was interrupted. Start a fresh handoff.");
            finish();
            return;
        }
        receive(getIntent());
    }

    @Override protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (busy) {
            status.setText("Authorization is active; finish or cancel it before another request.");
            return;
        }
        setIntent(intent);
        receive(intent);
    }

    private void receive(Intent intent) {
        request = null;
        authorize.setEnabled(false);
        Uri uri = intent.getData();
        if (!Intent.ACTION_VIEW.equals(intent.getAction()) || uri == null ||
            !"ib".equals(uri.getScheme()) || !"google-drive-authorize".equals(uri.getHost()) ||
            uri.getUserInfo() != null || uri.getPort() != -1 || uri.getFragment() != null ||
            (uri.getPath() != null && !uri.getPath().isEmpty())) {
            status.setText("Drive authorization handoff rejected.");
            return;
        }
        for (String key : uri.getQueryParameterNames()) {
            if (!(key.equals("client_id") || key.equals("scope") || key.equals("state") || key.equals("port")) ||
                uri.getQueryParameters(key).size() != 1) {
                status.setText("Drive authorization handoff rejected.");
                return;
            }
        }
        String clientId = uri.getQueryParameter("client_id");
        String state = uri.getQueryParameter("state");
        String scope = uri.getQueryParameter("scope");
        int port = port(uri.getQueryParameter("port"));
        if (!safe(clientId, 30, 512) || !clientId.endsWith(".apps.googleusercontent.com") ||
            !safe(state, 32, 256) || port == 0 ||
            !(READ.equals(scope) || (READ + " " + FILE).equals(scope))) {
            status.setText("Drive authorization handoff rejected.");
            return;
        }
        List<Scope> scopes = new ArrayList<>();
        scopes.add(new Scope(READ));
        if ((READ + " " + FILE).equals(scope)) scopes.add(new Scope(FILE));
        request = new Request(clientId, state, port, scopes);
        status.setText(scopes.size() == 1 ? "Read existing Drive files. Tap to authorize." :
            "Read Drive files and create application-owned archives. Tap to authorize.");
        authorize.setEnabled(true);
    }

    private void startConsent() {
        final Request selected = request;
        if (selected == null || busy) return;
        busy = true;
        authorize.setEnabled(false);
        AuthorizationRequest authorization = AuthorizationRequest.builder()
            .setRequestedScopes(selected.scopes)
            .requestOfflineAccess(selected.clientId)
            .setPrompt(AuthorizationRequest.Prompt.CONSENT)
            .build();
        client.authorize(authorization).addOnSuccessListener(result -> {
            if (destroyed || request != selected || !busy) return;
            if (!result.hasResolution()) {
                complete(selected, result);
                return;
            }
            PendingIntent resolution = result.getPendingIntent();
            if (resolution == null) { failed(); return; }
            try {
                startIntentSenderForResult(resolution.getIntentSender(), CONSENT_REQUEST, null, 0, 0, 0);
            } catch (IntentSender.SendIntentException failure) { failed(); }
        }).addOnFailureListener(failure -> { if (!destroyed && request == selected) failed(); });
    }

    @Override protected void onActivityResult(int code, int result, Intent data) {
        super.onActivityResult(code, result, data);
        if (code != CONSENT_REQUEST) return;
        // A destroyed/recreated adapter must restart explicit consent, never borrow an old result.
        if (!busy || request == null || result != RESULT_OK || data == null) { failed(); return; }
        try { complete(request, client.getAuthorizationResultFromIntent(data)); }
        catch (ApiException failure) { failed(); }
    }

    private void complete(final Request selected, AuthorizationResult result) {
        for (Scope scope : selected.scopes) {
            if (!result.getGrantedScopes().contains(scope.getScopeUri())) { failed(); return; }
        }
        final String code = result.getServerAuthCode();
        if (code == null || code.isEmpty()) { failed(); return; }
        // The result never enters history, journal, clipboard, intent extras or saved state.
        new Thread(() -> {
            HttpURLConnection connection = null;
            boolean delivered = false;
            try {
                Uri callback = new Uri.Builder().scheme("http")
                    .encodedAuthority("127.0.0.1:" + selected.port)
                    .path("/google-drive-callback").appendQueryParameter("code", code)
                    .appendQueryParameter("state", selected.state).build();
                connection = (HttpURLConnection) new URL(callback.toString()).openConnection();
                connection.setConnectTimeout(5000);
                connection.setReadTimeout(5000);
                connection.setUseCaches(false);
                connection.setInstanceFollowRedirects(false);
                connection.setRequestProperty("Cache-Control", "no-store");
                delivered = connection.getResponseCode() == 200;
            } catch (Exception failure) {
                // Provider/transport diagnostics must never include a callback or code.
            } finally { if (connection != null) connection.disconnect(); }
            final boolean accepted = delivered;
            runOnUiThread(() -> {
                if (destroyed || request != selected) return;
                busy = false;
                request = null;
                authorize.setEnabled(false);
                status.setText(accepted ? "Authorization delivered to the local credential writer." :
                    "Authorization delivery failed. Start a fresh handoff.");
            });
        }, "ib-drive-auth-loopback").start();
    }

    private void failed() {
        busy = false;
        request = null;
        authorize.setEnabled(false);
        status.setText("Authorization failed or was cancelled. Start a fresh handoff.");
    }
    @Override protected void onDestroy() { destroyed = true; request = null; super.onDestroy(); }

    private static int port(String text) {
        if (text == null || !text.matches("[0-9]{1,5}")) return 0;
        int value = Integer.parseInt(text);
        return value >= 1 && value <= 65535 ? value : 0;
    }
    private static boolean safe(String text, int minimum, int maximum) {
        return text != null && text.length() >= minimum && text.length() <= maximum &&
            text.matches("[A-Za-z0-9._-]+");
    }
}

package org.isomorphisms.ib.prepaint;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Build;

import java.util.UUID;

final class TermuxPrepaintClient {
    static final String TERMUX_PERMISSION = "com.termux.permission.RUN_COMMAND";
    static final String RESULT_ACTION = "org.isomorphisms.ib.prepaint.TERMUX_RESULT";
    static final String EXTRA_REQUEST_ID = "ib.request.id";
    static final String EXTRA_REQUEST_NONCE = "ib.request.nonce";
    static final String EXTRA_REQUEST_URL = "ib.request.url";

    private static final String TERMUX_PACKAGE = "com.termux";
    private static final String TERMUX_SERVICE = "com.termux.app.RunCommandService";
    private static final String RUN_COMMAND_ACTION = "com.termux.RUN_COMMAND";
    private static final String EXTRA_COMMAND_PATH = "com.termux.RUN_COMMAND_PATH";
    private static final String EXTRA_ARGUMENTS = "com.termux.RUN_COMMAND_ARGUMENTS";
    private static final String EXTRA_BACKGROUND = "com.termux.RUN_COMMAND_BACKGROUND";
    private static final String EXTRA_PENDING_INTENT =
            "com.termux.RUN_COMMAND_PENDING_INTENT";
    private static final String EXTRA_COMMAND_LABEL =
            "com.termux.RUN_COMMAND_COMMAND_LABEL";
    private static final String COMMAND_PATH = "~/opt/ib/bin/termux_prepaint_url.grease";

    private static final String PREFERENCES = "termux-prepaint";
    private static final String ACTIVE_ID = "active-id";
    private static final String ACTIVE_NONCE = "active-nonce";
    private static final String ACTIVE_URL = "active-url";
    private static final String NEXT_ID = "next-id";

    private TermuxPrepaintClient() {
    }

    static String start(Activity activity, String url) {
        SharedPreferences preferences = preferences(activity);
        long requestId = preferences.getLong(NEXT_ID, 0L) + 1L;
        String nonce = UUID.randomUUID().toString();
        if (!preferences.edit()
                .putLong(NEXT_ID, requestId)
                .putLong(ACTIVE_ID, requestId)
                .putString(ACTIVE_NONCE, nonce)
                .putString(ACTIVE_URL, url)
                .commit()) {
            return "Could not record the IB request.";
        }

        Intent result = new Intent(activity, PrepaintActivity.class)
                .setAction(RESULT_ACTION)
                .putExtra(EXTRA_REQUEST_ID, requestId)
                .putExtra(EXTRA_REQUEST_NONCE, nonce)
                .putExtra(EXTRA_REQUEST_URL, url);
        int pendingFlags = PendingIntent.FLAG_ONE_SHOT | PendingIntent.FLAG_CANCEL_CURRENT;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            pendingFlags |= PendingIntent.FLAG_MUTABLE;
        }
        PendingIntent callback = PendingIntent.getActivity(activity,
                (int) (requestId & 0x7fffffff), result, pendingFlags);

        Intent command = new Intent()
                .setClassName(TERMUX_PACKAGE, TERMUX_SERVICE)
                .setAction(RUN_COMMAND_ACTION)
                .putExtra(EXTRA_COMMAND_PATH, COMMAND_PATH)
                .putExtra(EXTRA_ARGUMENTS, new String[] {url})
                .putExtra(EXTRA_BACKGROUND, true)
                .putExtra(EXTRA_PENDING_INTENT, callback)
                .putExtra(EXTRA_COMMAND_LABEL, "IB prepaint");
        try {
            ComponentName started = activity.startService(command);
            if (started == null) {
                clearIfActive(activity, requestId, nonce);
                return "Termux is not installed or its command service is unavailable.";
            }
            return null;
        } catch (SecurityException error) {
            clearIfActive(activity, requestId, nonce);
            return "Grant IB permission to run commands in Termux.";
        } catch (RuntimeException error) {
            clearIfActive(activity, requestId, nonce);
            return "Could not start the Termux IB command.";
        }
    }

    static boolean isActiveResult(Context context, long requestId, String nonce, String url) {
        SharedPreferences preferences = preferences(context);
        return requestId > 0
                && requestId == preferences.getLong(ACTIVE_ID, -1L)
                && nonce != null
                && nonce.equals(preferences.getString(ACTIVE_NONCE, null))
                && url != null
                && url.equals(preferences.getString(ACTIVE_URL, null));
    }

    static boolean hasActiveRequest(Context context) {
        return preferences(context).getLong(ACTIVE_ID, -1L) > 0;
    }

    static void clearIfActive(Context context, long requestId, String nonce) {
        SharedPreferences preferences = preferences(context);
        if (requestId == preferences.getLong(ACTIVE_ID, -1L)
                && nonce != null
                && nonce.equals(preferences.getString(ACTIVE_NONCE, null))) {
            preferences.edit()
                    .remove(ACTIVE_ID)
                    .remove(ACTIVE_NONCE)
                    .remove(ACTIVE_URL)
                    .apply();
        }
    }

    private static SharedPreferences preferences(Context context) {
        return context.getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE);
    }
}

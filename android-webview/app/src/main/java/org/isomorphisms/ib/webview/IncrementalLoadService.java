package org.isomorphisms.ib.webview;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.Icon;
import android.os.IBinder;

public final class IncrementalLoadService extends Service {
    private static final String CHANNEL_ID = "ib-incremental-page";
    private static final String ACTION_STOP =
        "org.isomorphisms.ib.webview.STOP_INCREMENTAL_LOAD";
    private static final int NOTIFICATION_ID = 1127;

    public static void start(Activity activity) {
        Intent intent = new Intent(activity, IncrementalLoadService.class);
        activity.startForegroundService(intent);
    }

    public static void stop(Context context) {
        context.stopService(new Intent(context, IncrementalLoadService.class));
    }

    @Override
    public void onCreate() {
        super.onCreate();
        NotificationChannel channel = new NotificationChannel(
            CHANNEL_ID,
            "IB protected page loading",
            NotificationManager.IMPORTANCE_LOW
        );
        channel.setDescription("Keeps an explicitly opened IB page loading while it is off-screen.");
        getSystemService(NotificationManager.class).createNotificationChannel(channel);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int start_id) {
        if (intent != null && ACTION_STOP.equals(intent.getAction())) {
            stopForeground(STOP_FOREGROUND_REMOVE);
            stopSelf();
            return START_NOT_STICKY;
        }

        startForeground(NOTIFICATION_ID, notification());
        return START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private Notification notification() {
        Intent open_intent = new Intent(this, IncrementalLargePageActivity.class);
        open_intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        PendingIntent open_page = PendingIntent.getActivity(
            this,
            0,
            open_intent,
            PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE
        );

        Intent stop_intent = new Intent(this, IncrementalLoadService.class);
        stop_intent.setAction(ACTION_STOP);
        PendingIntent stop_loading = PendingIntent.getService(
            this,
            1,
            stop_intent,
            PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE
        );

        return new Notification.Builder(this, CHANNEL_ID)
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setContentTitle("IB is keeping the page alive")
            .setContentText("Google Cloud Console is still loading off-screen")
            .setContentIntent(open_page)
            .setOngoing(true)
            .setCategory(Notification.CATEGORY_PROGRESS)
            .addAction(
                new Notification.Action.Builder(
                    Icon.createWithResource(this, android.R.drawable.ic_menu_close_clear_cancel),
                    "Stop",
                    stop_loading
                ).build()
            )
            .build();
    }
}

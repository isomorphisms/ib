package org.isomorphisms.ib.prepaint;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;

import java.util.WeakHashMap;

public final class PrepaintApplication extends Application
        implements Application.ActivityLifecycleCallbacks {
    private final WeakHashMap<Activity, PrepaintNavigation> installed = new WeakHashMap<>();

    @Override
    public void onCreate() {
        super.onCreate();
        registerActivityLifecycleCallbacks(this);
    }

    @Override
    public void onActivityStarted(Activity activity) {
        if (!(activity instanceof PrepaintActivity) || installed.containsKey(activity)) {
            return;
        }
        PrepaintNavigation navigation = PrepaintNavigation.install(activity);
        if (navigation != null) {
            installed.put(activity, navigation);
        }
    }

    @Override
    public void onActivityDestroyed(Activity activity) {
        PrepaintNavigation navigation = installed.remove(activity);
        if (navigation != null) {
            navigation.destroy();
        }
    }

    @Override public void onActivityCreated(Activity activity, Bundle state) { }
    @Override public void onActivityResumed(Activity activity) { }
    @Override public void onActivityPaused(Activity activity) { }
    @Override public void onActivityStopped(Activity activity) { }
    @Override public void onActivitySaveInstanceState(Activity activity, Bundle state) { }
}

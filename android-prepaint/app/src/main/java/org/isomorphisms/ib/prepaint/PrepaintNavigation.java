package org.isomorphisms.ib.prepaint;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ValueAnimator;
import android.app.Activity;
import android.graphics.Typeface;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

final class PrepaintNavigation {
    static final long TRANSITION_MILLIS = 150;
    private static final int NAVIGATION_WIDTH_DP = 136;

    private final LinearLayout navigation;
    private final Button toggle;
    private final int openWidth;

    private boolean open = true;
    private ValueAnimator animator;

    private PrepaintNavigation(LinearLayout navigation, Button toggle, int openWidth) {
        this.navigation = navigation;
        this.toggle = toggle;
        this.openWidth = openWidth;
        updateToggle();
    }

    static PrepaintNavigation install(Activity activity) {
        ViewGroup content = activity.findViewById(android.R.id.content);
        if (!(content instanceof FrameLayout) || content.getChildCount() != 1) {
            return null;
        }
        View existing = content.getChildAt(0);
        if (!(existing instanceof LinearLayout)) {
            return null;
        }
        LinearLayout root = (LinearLayout) existing;
        if (root.getChildCount() == 0 || !(root.getChildAt(0) instanceof LinearLayout)) {
            return null;
        }
        LinearLayout chrome = (LinearLayout) root.getChildAt(0);
        Button openButton = findButton(chrome, "Open");
        Button replayButton = findButton(chrome, "Replay");
        if (openButton == null || replayButton == null) {
            return null;
        }

        chrome.removeView(openButton);
        chrome.removeView(replayButton);
        content.removeView(root);

        LinearLayout shell = new LinearLayout(activity);
        shell.setOrientation(LinearLayout.HORIZONTAL);
        shell.setBackgroundColor(activity.getColor(R.color.prepaint_background));

        LinearLayout navigation = new LinearLayout(activity);
        navigation.setOrientation(LinearLayout.VERTICAL);
        navigation.setGravity(Gravity.TOP);
        navigation.setPadding(dp(activity, 8), dp(activity, 10), dp(activity, 8), dp(activity, 10));
        navigation.setBackgroundColor(activity.getColor(R.color.prepaint_surface));

        TextView heading = new TextView(activity);
        heading.setText("IB");
        heading.setTextSize(12);
        heading.setTextColor(activity.getColor(R.color.prepaint_secondary));
        heading.setTypeface(Typeface.createFromAsset(activity.getAssets(), "fonts/Hack-Regular.ttf"));
        heading.setIncludeFontPadding(false);
        LinearLayout.LayoutParams headingLayout = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        headingLayout.setMargins(dp(activity, 7), 0, dp(activity, 7), dp(activity, 10));
        navigation.addView(heading, headingLayout);

        navigation.addView(openButton, navigationButtonLayout(activity, 0));
        navigation.addView(replayButton, navigationButtonLayout(activity, 7));

        Button toggle = new Button(activity);
        toggle.setAllCaps(false);
        toggle.setTextColor(activity.getColor(R.color.prepaint_foreground));
        toggle.setTextSize(14);
        toggle.setTypeface(Typeface.createFromAsset(activity.getAssets(), "fonts/Hack-Regular.ttf"));
        toggle.setMinHeight(0);
        toggle.setMinWidth(0);
        toggle.setPadding(dp(activity, 10), dp(activity, 7), dp(activity, 10), dp(activity, 7));
        toggle.setBackground(buttonBackground(activity));
        toggle.setFocusable(true);
        LinearLayout.LayoutParams toggleLayout = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        toggleLayout.setMargins(0, 0, dp(activity, 8), 0);
        chrome.addView(toggle, 0, toggleLayout);

        int navigationWidth = dp(activity, NAVIGATION_WIDTH_DP);
        shell.addView(navigation, new LinearLayout.LayoutParams(
                navigationWidth, ViewGroup.LayoutParams.MATCH_PARENT));
        shell.addView(root, new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.MATCH_PARENT, 1));
        content.addView(shell, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        PrepaintNavigation installed = new PrepaintNavigation(navigation, toggle, navigationWidth);
        toggle.setOnClickListener(ignored -> installed.setOpen(!installed.open));
        return installed;
    }

    private void setOpen(boolean wantedOpen) {
        open = wantedOpen;
        updateToggle();
        cancelAnimator();

        int targetWidth = open ? openWidth : 0;
        ViewGroup.LayoutParams layout = navigation.getLayoutParams();
        int startWidth = layout.width;
        if (open) {
            navigation.setVisibility(View.VISIBLE);
        }

        if (startWidth == targetWidth || !ValueAnimator.areAnimatorsEnabled()) {
            setWidth(targetWidth);
            navigation.setVisibility(open ? View.VISIBLE : View.GONE);
            return;
        }

        ValueAnimator next = ValueAnimator.ofInt(startWidth, targetWidth);
        animator = next;
        next.setDuration(TRANSITION_MILLIS);
        next.addUpdateListener(value -> setWidth((Integer) value.getAnimatedValue()));
        next.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                if (animator != animation) {
                    return;
                }
                animator = null;
                setWidth(targetWidth);
                if (!open) {
                    navigation.setVisibility(View.GONE);
                }
            }
        });
        next.start();
    }

    private void updateToggle() {
        toggle.setText(open ? "<" : ">");
        toggle.setContentDescription(open ? "Close navigation" : "Open navigation");
    }

    private void setWidth(int width) {
        ViewGroup.LayoutParams layout = navigation.getLayoutParams();
        layout.width = width;
        navigation.setLayoutParams(layout);
    }

    private void cancelAnimator() {
        if (animator == null) {
            return;
        }
        ValueAnimator old = animator;
        animator = null;
        old.cancel();
    }

    void destroy() {
        cancelAnimator();
    }

    private static Button findButton(LinearLayout parent, String text) {
        for (int index = 0; index < parent.getChildCount(); index += 1) {
            View child = parent.getChildAt(index);
            if (child instanceof Button && text.contentEquals(((Button) child).getText())) {
                return (Button) child;
            }
        }
        return null;
    }

    private static LinearLayout.LayoutParams navigationButtonLayout(Activity activity, int top) {
        LinearLayout.LayoutParams layout = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        layout.setMargins(0, dp(activity, top), 0, 0);
        return layout;
    }

    private static android.graphics.drawable.GradientDrawable buttonBackground(Activity activity) {
        android.graphics.drawable.GradientDrawable background =
                new android.graphics.drawable.GradientDrawable();
        background.setColor(activity.getColor(R.color.prepaint_rule));
        background.setCornerRadius(dp(activity, 5));
        return background;
    }

    private static int dp(Activity activity, int value) {
        return Math.round(value * activity.getResources().getDisplayMetrics().density);
    }
}

#include "material.h"

enum { WB_BUTTON = 0x7f03000f, WB_SCROLL = 0x7f030010,
       WB_ENTRIES = 0x7f030011, READER_SCROLL = 0x7f03000e,
       WB_FIRST_SOURCE = 0x00f10000, WB_LIMIT = 128 };

static jobject workbench_view(JNIEnv *e, jobject activity, jint identity) {
    return (*e)->CallObjectMethod(e, activity,
        m3_method(e, activity, "findViewById", "(I)Landroid/view/View;"), identity);
}

static void style_branch(JNIEnv *e, jobject view, M3Theme theme, int depth, unsigned *remaining) {
    if (!view || depth > 16 || !*remaining || (*e)->ExceptionCheck(e)) return;
    *remaining ← *remaining - 1;
    jclass text ← (*e)->FindClass(e, "android/widget/TextView");
    jclass button ← (*e)->FindClass(e, "android/widget/Button");
    jclass group ← (*e)->FindClass(e, "android/view/ViewGroup");
    if ((*e)->IsInstanceOf(e, view, text))
        (*e)->CallVoidMethod(e, view, m3_method(e, view, "setTextColor", "(I)V"), theme.on_surface);
    if ((*e)->IsInstanceOf(e, view, button)) m3_control(e, view, theme, 0);
    if ((*e)->IsInstanceOf(e, view, group)) {
        (*e)->CallVoidMethod(e, view, m3_method(e, view, "setBackgroundColor", "(I)V"), theme.surface);
        jint count ← (*e)->CallIntMethod(e, view, m3_method(e, view, "getChildCount", "()I"));
        for (jint i ← 0; i < count && *remaining; i++) {
            jobject child ← (*e)->CallObjectMethod(e, view,
                m3_method(e, view, "getChildAt", "(I)Landroid/view/View;"), i);
            style_branch(e, child, theme, depth + 1, remaining);
            (*e)->DeleteLocalRef(e, child);
        }
    }
    (*e)->DeleteLocalRef(e, text); (*e)->DeleteLocalRef(e, button); (*e)->DeleteLocalRef(e, group);
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_ReaderBridge_style(JNIEnv *e, jclass ignored, jobject activity) {
    (void)ignored;
    if ((*e)->PushLocalFrame(e, 64) < 0) return;
    M3Theme theme ← m3_theme(e, activity);
    jobject root ← workbench_view(e, activity, 0x01020002); /* android.R.id.content */
    unsigned budget ← 384;
    style_branch(e, root, theme, 0, &budget);
    jobject window ← (*e)->CallObjectMethod(e, activity,
        m3_method(e, activity, "getWindow", "()Landroid/view/Window;"));
    (*e)->CallVoidMethod(e, window, m3_method(e, window, "setStatusBarColor", "(I)V"), theme.surface);
    (*e)->CallVoidMethod(e, window, m3_method(e, window, "setNavigationBarColor", "(I)V"), theme.surface);
    jobject decor ← (*e)->CallObjectMethod(e, window, m3_method(e, window, "getDecorView", "()Landroid/view/View;"));
    jint flags ← (*e)->CallIntMethod(e, decor, m3_method(e, decor, "getSystemUiVisibility", "()I"));
    (*e)->CallVoidMethod(e, decor, m3_method(e, decor, "setSystemUiVisibility", "(I)V"),
        (flags & ~0x2010) | (theme.dark ? 0 : 0x2010));
    (*e)->PopLocalFrame(e, NULL);
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_ReaderBridge_workbenchBegin(JNIEnv *e, jclass ignored,
        jobject activity, jstring revision) {
    (void)ignored;
    if ((*e)->PushLocalFrame(e, 16) < 0) return;
    jobject entries ← workbench_view(e, activity, WB_ENTRIES);
    (*e)->CallVoidMethod(e, entries, m3_method(e, entries, "removeAllViews", "()V"));
    (*e)->CallVoidMethod(e, entries, m3_method(e, entries, "setTag", "(Ljava/lang/Object;)V"), revision);
    M3Theme theme ← m3_theme(e, activity);
    jobject label ← m3_new(e, activity, "android/widget/TextView");
    jstring title ← (*e)->NewStringUTF(e, "Retained visits\nRead a verified source, then return to this history. This view does not restore a live site session.");
    m3_text(e, label, title, theme, 16.0f);
    m3_add(e, entries, label, -1, -2, 0.0f);
    (*e)->PopLocalFrame(e, NULL);
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_ReaderBridge_workbenchItem(JNIEnv *e, jclass ignored,
        jobject activity, jint position, jstring label) {
    (void)ignored;
    if (position < 0 || position >= WB_LIMIT || (*e)->PushLocalFrame(e, 24) < 0) return;
    jobject entries ← workbench_view(e, activity, WB_ENTRIES);
    M3Theme theme ← m3_theme(e, activity);
    jobject button ← m3_button(e, activity, activity, theme, WB_FIRST_SOURCE + position, "Read retained source", 0);
    m3_text(e, button, label, theme, 14.0f);
    (*e)->CallVoidMethod(e, button, m3_method(e, button, "setContentDescription", "(Ljava/lang/CharSequence;)V"), label);
    m3_add(e, entries, button, -1, -2, 0.0f);
    (*e)->PopLocalFrame(e, NULL);
}

JNIEXPORT jstring JNICALL
Java_org_isomorphisms_ib_ReaderBridge_workbenchRevision(JNIEnv *e, jclass ignored, jobject activity) {
    (void)ignored;
    jobject entries ← workbench_view(e, activity, WB_ENTRIES);
    return (jstring)(*e)->CallObjectMethod(e, entries, m3_method(e, entries, "getTag", "()Ljava/lang/Object;"));
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_ReaderBridge_workbenchVisible(JNIEnv *e, jclass ignored, jobject activity, jint visible) {
    (void)ignored;
    jobject workbench ← workbench_view(e, activity, WB_SCROLL);
    jobject reader ← workbench_view(e, activity, READER_SCROLL);
    (*e)->CallVoidMethod(e, workbench, m3_method(e, workbench, "setVisibility", "(I)V"), visible ? 0 : 8);
    (*e)->CallVoidMethod(e, reader, m3_method(e, reader, "setVisibility", "(I)V"), visible ? 8 : 0);
    /* Visibility preserves the existing NativeTextView, selection and scroll. */
    jobject button ← workbench_view(e, activity, WB_BUTTON);
    jstring label ← (*e)->NewStringUTF(e, visible ? "Reader" : "History");
    (*e)->CallVoidMethod(e, button, m3_method(e, button, "setText", "(Ljava/lang/CharSequence;)V"), label);
    (*e)->DeleteLocalRef(e, label);
}

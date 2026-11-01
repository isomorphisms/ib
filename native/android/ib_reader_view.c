/* SDK storage, gestures, styling and clipboard only. Checked Idric owns the
 * immutable source/display coordinates and every link/copy decision. */
#include <jni.h>
#include <android/log.h>
#include <math.h>
#include <stdint.h>

#define BODY_VIEW 2130903040
#define ANDROID_COPY 16908321

static jmethodID sdk_method(JNIEnv *e, const char *owner, const char *name, const char *signature) {
    if ((*e)->ExceptionCheck(e)) return NULL;
    jclass type = (*e)->FindClass(e, owner);
    return type ? (*e)->GetMethodID(e, type, name, signature) : NULL;
}

static jobject reader_body(JNIEnv *e, jobject activity) {
    jmethodID find = sdk_method(e, "android/app/Activity", "findViewById", "(I)Landroid/view/View;");
    jobject body = activity && find ? (*e)->CallObjectMethod(e, activity, find, BODY_VIEW) : NULL;
    jclass type = body ? (*e)->FindClass(e, "org/isomorphisms/ib/NativeTextView") : NULL;
    return type && (*e)->IsInstanceOf(e, body, type) && !(*e)->ExceptionCheck(e) ? body : NULL;
}

static jfieldID view_field(JNIEnv *e, jobject view, const char *name, const char *type) {
    if ((*e)->ExceptionCheck(e)) return NULL;
    jclass owner = view ? (*e)->GetObjectClass(e, view) : NULL;
    return owner ? (*e)->GetFieldID(e, owner, name, type) : NULL;
}

static jobject activity_context(JNIEnv *e, jobject view) {
    jmethodID get = sdk_method(e, "android/view/View", "getContext", "()Landroid/content/Context;");
    jobject context = get ? (*e)->CallObjectMethod(e, view, get) : NULL;
    jclass type = context ? (*e)->FindClass(e, "android/app/Activity") : NULL;
    return type && (*e)->IsInstanceOf(e, context, type) ? context : NULL;
}

static void report_refusal(JNIEnv *e, jobject view, const char *message) {
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    jobject activity = activity_context(e, view);
    jclass generated = activity ? (*e)->FindClass(e, "Idric/Generated") : NULL;
    jmethodID report = generated ? (*e)->GetStaticMethodID(e, generated, "reader_acquisition_failed",
        "(Landroid/app/Activity;Ljava/lang/String;)V") : NULL;
    jstring reason = report ? (*e)->NewStringUTF(e, message) : NULL;
    if (reason) (*e)->CallStaticVoidMethod(e, generated, report, activity, reason);
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
}

static int projection_texts(JNIEnv *e, jobject view, jstring *source, jstring *display, jstring *coordinates) {
    jfieldID field = view_field(e, view, "projection", "Lorg/isomorphisms/ib/NativeTextProjection;");
    jobject projection = field ? (*e)->GetObjectField(e, view, field) : NULL;
    jclass type = projection ? (*e)->GetObjectClass(e, projection) : NULL;
    jfieldID original = type ? (*e)->GetFieldID(e, type, "sourceText", "Ljava/lang/String;") : NULL;
    jfieldID painted = original ? (*e)->GetFieldID(e, type, "displayText", "Ljava/lang/String;") : NULL;
    jfieldID mapping = painted ? (*e)->GetFieldID(e, type, "coordinatesText", "Ljava/lang/String;") : NULL;
    if (!mapping || (*e)->ExceptionCheck(e)) return 0;
    *source = (*e)->GetObjectField(e, projection, original);
    *display = (*e)->GetObjectField(e, projection, painted);
    *coordinates = (*e)->GetObjectField(e, projection, mapping);
    return *source && *display && *coordinates && !(*e)->ExceptionCheck(e);
}

JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_ReaderBridge_presentText(JNIEnv *e, jclass ignored, jobject activity,
                                                 jstring displayed, jstring source, jstring coordinates) {
    (void)ignored;
    if ((*e)->PushLocalFrame(e, 32) != 0) return -1;
    jobject view = reader_body(e, activity);
    jclass type = view ? (*e)->FindClass(e, "org/isomorphisms/ib/NativeTextProjection") : NULL;
    jmethodID constructor = type ? (*e)->GetMethodID(e, type, "<init>", "()V") : NULL;
    jobject projection = constructor && displayed && source && coordinates ? (*e)->NewObject(e, type, constructor) : NULL;
    jfieldID original = projection ? (*e)->GetFieldID(e, type, "sourceText", "Ljava/lang/String;") : NULL;
    jfieldID painted = original ? (*e)->GetFieldID(e, type, "displayText", "Ljava/lang/String;") : NULL;
    jfieldID mapping = painted ? (*e)->GetFieldID(e, type, "coordinatesText", "Ljava/lang/String;") : NULL;
    jfieldID storage = mapping ? view_field(e, view, "projection", "Lorg/isomorphisms/ib/NativeTextProjection;") : NULL;
    jclass span_type = storage ? (*e)->FindClass(e, "android/text/SpannableString") : NULL;
    jmethodID span_constructor = span_type ? (*e)->GetMethodID(e, span_type, "<init>", "(Ljava/lang/CharSequence;)V") : NULL;
    jobject styled = span_constructor ? (*e)->NewObject(e, span_type, span_constructor, displayed) : NULL;
    jmethodID set = styled ? sdk_method(e, "android/widget/TextView", "setText", "(Ljava/lang/CharSequence;)V") : NULL;
    int ok = set && !(*e)->ExceptionCheck(e);
    if (ok) {
        (*e)->SetObjectField(e, projection, original, source);
        (*e)->SetObjectField(e, projection, painted, displayed);
        (*e)->SetObjectField(e, projection, mapping, coordinates);
        /* UI-thread invocation, after all source/projection preparation. The
         * previous projection remains intact if the SDK rejects setText. */
        (*e)->CallVoidMethod(e, view, set, styled);
        ok = !(*e)->ExceptionCheck(e);
        if (ok) (*e)->SetObjectField(e, view, storage, projection);
        ok = ok && !(*e)->ExceptionCheck(e);
    }
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    (*e)->PopLocalFrame(e, NULL);
    return ok ? 0 : -1;
}

/* The listener binds the exact immutable projection, not a mutable screen
 * pathname or a later document. Pre-draw runs after TextView layout, when the
 * SDK can scroll the source-backed UTF-16 point into its parent viewport. */
JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_ReaderBridge_scrollAnchor(JNIEnv *e, jclass ignored,
                                                  jobject activity, jint point) {
    (void)ignored;
    if (point < 0 || (*e)->PushLocalFrame(e, 32) != 0) return -1;
    jobject view = reader_body(e, activity);
    jfieldID stored = view ? view_field(e, view, "projection", "Lorg/isomorphisms/ib/NativeTextProjection;") : NULL;
    jobject projection = stored ? (*e)->GetObjectField(e, view, stored) : NULL;
    jstring source = NULL, display = NULL, coordinates = NULL;
    int bounded = projection && projection_texts(e, view, &source, &display, &coordinates) &&
        point <= (*e)->GetStringLength(e, display);
    jclass type = bounded ? (*e)->FindClass(e, "org/isomorphisms/ib/ReaderAnchorListener") : NULL;
    jmethodID construct = type ? (*e)->GetMethodID(e, type, "<init>", "()V") : NULL;
    jobject listener = construct ? (*e)->NewObject(e, type, construct) : NULL;
    jfieldID target = listener ? (*e)->GetFieldID(e, type, "targetView", "Lorg/isomorphisms/ib/NativeTextView;") : NULL;
    jfieldID owner = target ? (*e)->GetFieldID(e, type, "projection", "Lorg/isomorphisms/ib/NativeTextProjection;") : NULL;
    jfieldID offset = owner ? (*e)->GetFieldID(e, type, "offsetValue", "Ljava/lang/Integer;") : NULL;
    jclass integer_type = offset ? (*e)->FindClass(e, "java/lang/Integer") : NULL;
    jmethodID box = integer_type ? (*e)->GetMethodID(e, integer_type, "<init>", "(I)V") : NULL;
    jobject number = box ? (*e)->NewObject(e, integer_type, box, point) : NULL;
    jmethodID tree = number ? sdk_method(e, "android/view/View", "getViewTreeObserver", "()Landroid/view/ViewTreeObserver;") : NULL;
    jobject observer = tree ? (*e)->CallObjectMethod(e, view, tree) : NULL;
    jmethodID add = observer ? sdk_method(e, "android/view/ViewTreeObserver", "addOnPreDrawListener",
        "(Landroid/view/ViewTreeObserver$OnPreDrawListener;)V") : NULL;
    int ok = add && !(*e)->ExceptionCheck(e);
    if (ok) {
        (*e)->SetObjectField(e, listener, target, view);
        (*e)->SetObjectField(e, listener, owner, projection);
        (*e)->SetObjectField(e, listener, offset, number);
        if (!(*e)->ExceptionCheck(e)) (*e)->CallVoidMethod(e, observer, add, listener);
        jmethodID request = sdk_method(e, "android/view/View", "requestLayout", "()V");
        if (request && !(*e)->ExceptionCheck(e)) (*e)->CallVoidMethod(e, view, request);
        ok = request && !(*e)->ExceptionCheck(e);
    }
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    (*e)->PopLocalFrame(e, NULL);
    return ok ? 0 : -1;
}

/* Explicit source navigation replaces the previous insertion point.
 * TextView.onPreDraw otherwise brings its old selection back into view.
 * A direct scrollTo also leaves an older smooth scroll alive; replace that
 * animation with a zero-distance request at the measured final position. */
static int settle_fragment_cursor(JNIEnv *e, jobject view, jobject parent, jint point, jint position) {
    jmethodID get_text = sdk_method(e, "android/widget/TextView", "getText", "()Ljava/lang/CharSequence;");
    jobject text = get_text ? (*e)->CallObjectMethod(e, view, get_text) : NULL;
    jclass spannable = text ? (*e)->FindClass(e, "android/text/Spannable") : NULL;
    jclass selection = spannable ? (*e)->FindClass(e, "android/text/Selection") : NULL;
    jmethodID set = selection ? (*e)->GetStaticMethodID(e, selection, "setSelection",
        "(Landroid/text/Spannable;I)V") : NULL;
    jmethodID stop = set ? sdk_method(e, "android/widget/ScrollView", "smoothScrollTo", "(II)V") : NULL;
    if (!stop || !(*e)->IsInstanceOf(e, text, spannable) || (*e)->ExceptionCheck(e)) return 0;
    (*e)->CallStaticVoidMethod(e, selection, set, text, point);
    /* On API 29/34 a first smoothScrollTo after an idle gap starts an active
     * zero-distance OverScroller. That still intercepts the next ACTION_DOWN.
     * Both calls run in the same pre-draw animation clock: the rapid second
     * public call aborts that scroller and keeps the measured position. */
    for (unsigned cancellation_pass = 0;
         cancellation_pass < 2 && !(*e)->ExceptionCheck(e); ++cancellation_pass)
        (*e)->CallVoidMethod(e, parent, stop, 0, position);
    return !(*e)->ExceptionCheck(e);
}

/* ScrollView.requestChildRectangleOnScreen computes its vertical window
 * without the parent's padding. With clipToPadding, it can put line zero
 * behind that padding. Use the actual direct ScrollView and measured Layout
 * coordinates; neither focus nor an assumed toolbar/display size is needed. */
static int position_fragment(JNIEnv *e, jobject view, jint point) {
    jmethodID get_layout = sdk_method(e, "android/widget/TextView", "getLayout", "()Landroid/text/Layout;");
    jobject layout = get_layout ? (*e)->CallObjectMethod(e, view, get_layout) : NULL;
    jmethodID for_offset = layout ? sdk_method(e, "android/text/Layout", "getLineForOffset", "(I)I") : NULL;
    jmethodID line_top = for_offset ? sdk_method(e, "android/text/Layout", "getLineTop", "(I)I") : NULL;
    jmethodID line_bottom = line_top ? sdk_method(e, "android/text/Layout", "getLineBottom", "(I)I") : NULL;
    jmethodID padding_top = line_bottom ? sdk_method(e, "android/widget/TextView", "getTotalPaddingTop", "()I") : NULL;
    jmethodID get_height = padding_top ? sdk_method(e, "android/view/View", "getHeight", "()I") : NULL;
    jmethodID get_parent = get_height ? sdk_method(e, "android/view/View", "getParent", "()Landroid/view/ViewParent;") : NULL;
    jobject parent = get_parent ? (*e)->CallObjectMethod(e, view, get_parent) : NULL;
    jclass scroll_type = parent ? (*e)->FindClass(e, "android/widget/ScrollView") : NULL;
    if (!scroll_type || !(*e)->IsInstanceOf(e, parent, scroll_type) || (*e)->ExceptionCheck(e)) return 0;
    jmethodID get_top = sdk_method(e, "android/view/View", "getTop", "()I");
    jmethodID parent_top = get_top ? sdk_method(e, "android/view/View", "getPaddingTop", "()I") : NULL;
    jmethodID parent_bottom = parent_top ? sdk_method(e, "android/view/View", "getPaddingBottom", "()I") : NULL;
    jmethodID scroll = parent_bottom ? sdk_method(e, "android/view/View", "scrollTo", "(II)V") : NULL;
    jmethodID get_scroll = scroll ? sdk_method(e, "android/view/View", "getScrollY", "()I") : NULL;
    if (!get_scroll || (*e)->ExceptionCheck(e)) return 0;
    jint line = (*e)->CallIntMethod(e, layout, for_offset, point);
    if (line < 0 || (*e)->ExceptionCheck(e)) return 0;
    jint top = (*e)->CallIntMethod(e, layout, line_top, line);
    jint bottom = (*e)->CallIntMethod(e, layout, line_bottom, line);
    jint above = (*e)->CallIntMethod(e, view, padding_top);
    jint height = (*e)->CallIntMethod(e, view, get_height);
    jint child_top = (*e)->CallIntMethod(e, view, get_top);
    jint viewport = (*e)->CallIntMethod(e, parent, get_height);
    jint before = (*e)->CallIntMethod(e, parent, parent_top);
    jint after_padding = (*e)->CallIntMethod(e, parent, parent_bottom);
    if ((*e)->ExceptionCheck(e) || top < 0 || bottom <= top || above < 0 ||
        height < 1 || child_top < 0 || before < 0 || after_padding < 0 ||
        (int64_t)before + after_padding >= viewport) return 0;
    int64_t first = (int64_t)top + above, after = (int64_t)bottom + above;
    if (after <= first || after > height || after - first > (int64_t)viewport - before - after_padding) return 0;
    int64_t wanted = (int64_t)child_top + first - before;
    if (wanted < 0) wanted = 0;
    if (wanted > INT32_MAX) return 0;
    /* TextView selection can have its own scroll. The checked source point
     * names Layout coordinates, so normalize that local scroll first. */
    (*e)->CallVoidMethod(e, view, scroll, 0, 0);
    if (!(*e)->ExceptionCheck(e)) (*e)->CallVoidMethod(e, parent, scroll, 0, (jint)wanted);
    jint observed = !(*e)->ExceptionCheck(e) ? (*e)->CallIntMethod(e, parent, get_scroll) : -1;
    if ((*e)->ExceptionCheck(e) || observed < 0) return 0;
    if (!settle_fragment_cursor(e, view, parent, point, observed)) return 0;
    int64_t visible_first = (int64_t)child_top + first - observed;
    int64_t visible_after = (int64_t)child_top + after - observed;
    __android_log_print(ANDROID_LOG_INFO, "IBReader",
        "fragment viewport point=%d line=%d parent_scroll=%d padding=%d,%d",
        point, line, observed, before, after_padding);
    /* scrollTo clamps at the real child extent. Require the complete target
     * line inside the padded viewport; the SDK oracle independently observes
     * its rendered screen position and rejects the compiled zero-point APK. */
    return visible_first >= before && visible_after <= (int64_t)viewport - after_padding;
}

JNIEXPORT jboolean JNICALL
Java_org_isomorphisms_ib_ReaderAnchorListener_onPreDraw(JNIEnv *e, jobject self) {
    if ((*e)->PushLocalFrame(e, 64) != 0) return JNI_TRUE;
    jfieldID target = view_field(e, self, "targetView", "Lorg/isomorphisms/ib/NativeTextView;");
    jfieldID owner = target ? view_field(e, self, "projection", "Lorg/isomorphisms/ib/NativeTextProjection;") : NULL;
    jfieldID offset = owner ? view_field(e, self, "offsetValue", "Ljava/lang/Integer;") : NULL;
    jobject view = offset ? (*e)->GetObjectField(e, self, target) : NULL;
    jobject projection = view ? (*e)->GetObjectField(e, self, owner) : NULL;
    jobject number = projection ? (*e)->GetObjectField(e, self, offset) : NULL;
    if (view) {
        jmethodID tree = sdk_method(e, "android/view/View", "getViewTreeObserver", "()Landroid/view/ViewTreeObserver;");
        jobject observer = tree ? (*e)->CallObjectMethod(e, view, tree) : NULL;
        jmethodID remove = observer ? sdk_method(e, "android/view/ViewTreeObserver", "removeOnPreDrawListener",
            "(Landroid/view/ViewTreeObserver$OnPreDrawListener;)V") : NULL;
        if (remove) (*e)->CallVoidMethod(e, observer, remove, self);
        int failed = (*e)->ExceptionCheck(e);
        if (failed) (*e)->ExceptionClear(e);
        /* Break the listener/view cycle even when a newer projection won. */
        (*e)->SetObjectField(e, self, target, NULL);
        (*e)->SetObjectField(e, self, owner, NULL);
        (*e)->SetObjectField(e, self, offset, NULL);
        jfieldID current_field = view_field(e, view, "projection", "Lorg/isomorphisms/ib/NativeTextProjection;");
        jobject current = current_field ? (*e)->GetObjectField(e, view, current_field) : NULL;
        int same = !failed && !(*e)->ExceptionCheck(e) && current && projection && (*e)->IsSameObject(e, current, projection);
        if (failed) report_refusal(e, view, "The native fragment observer failed; the document is retained.");
        if (same && number && !(*e)->ExceptionCheck(e)) {
            jmethodID integer = sdk_method(e, "java/lang/Integer", "intValue", "()I");
            jint point = integer ? (*e)->CallIntMethod(e, number, integer) : -1;
            jstring source = NULL, display = NULL, coordinates = NULL;
            int bounded = point >= 0 && projection_texts(e, view, &source, &display, &coordinates) &&
                point <= (*e)->GetStringLength(e, display);
            int positioned = bounded && !(*e)->ExceptionCheck(e) && position_fragment(e, view, point);
            if (!positioned || (*e)->ExceptionCheck(e))
                report_refusal(e, view, "The source-linked fragment could not be positioned; its document is retained.");
        }
    }
    if ((*e)->ExceptionCheck(e)) {
        if (view) report_refusal(e, view, "The native fragment adapter failed; the document is retained.");
        else (*e)->ExceptionClear(e);
    }
    (*e)->PopLocalFrame(e, NULL);
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_ReaderBridge_markText(JNIEnv *e, jclass ignored, jobject activity,
                                              jint first, jint after, jint style) {
    (void)ignored;
    if ((*e)->PushLocalFrame(e, 24) != 0) return -1;
    jobject view = reader_body(e, activity);
    jmethodID get = view ? sdk_method(e, "android/widget/TextView", "getText", "()Ljava/lang/CharSequence;") : NULL;
    jobject text = get ? (*e)->CallObjectMethod(e, view, get) : NULL;
    jclass span_type = text ? (*e)->FindClass(e, "android/text/Spannable") : NULL;
    jmethodID length = span_type ? sdk_method(e, "java/lang/CharSequence", "length", "()I") : NULL;
    jint count = length && text ? (*e)->CallIntMethod(e, text, length) : 0;
    jmethodID set = span_type ? (*e)->GetMethodID(e, span_type, "setSpan", "(Ljava/lang/Object;III)V") : NULL;
    int ok = span_type && (*e)->IsInstanceOf(e, text, span_type) && set &&
        first >= 0 && first < after && after <= count && style >= 0 && style <= 6 && !(*e)->ExceptionCheck(e);
    if (ok) {
        jclass color_type = (*e)->FindClass(e, "android/text/style/ForegroundColorSpan");
        jmethodID color_constructor = color_type ? (*e)->GetMethodID(e, color_type, "<init>", "(I)V") : NULL;
        jobject color = color_constructor ? (*e)->NewObject(e, color_type, color_constructor,
            style == 0 ? (jint)UINT32_C(0xff90caf9) : (jint)UINT32_C(0xffce93d8)) : NULL;
        jclass underline_type = color ? (*e)->FindClass(e, "android/text/style/UnderlineSpan") : NULL;
        jmethodID underline_constructor = underline_type ? (*e)->GetMethodID(e, underline_type, "<init>", "()V") : NULL;
        jobject underline = underline_constructor ? (*e)->NewObject(e, underline_type, underline_constructor) : NULL;
        ok = color && underline && !(*e)->ExceptionCheck(e);
        if (ok) {
            /* Color/underline preserve the Paint advance used by reflow. */
            (*e)->CallVoidMethod(e, text, set, color, first, after, 33);
            if (!(*e)->ExceptionCheck(e)) (*e)->CallVoidMethod(e, text, set, underline, first, after, 33);
            ok = !(*e)->ExceptionCheck(e);
        }
    }
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    (*e)->PopLocalFrame(e, NULL);
    return ok ? 0 : -1;
}

JNIEXPORT jboolean JNICALL
Java_org_isomorphisms_ib_NativeTextView_onTextContextMenuItem(JNIEnv *e, jobject self, jint action) {
    jclass parent = (*e)->FindClass(e, "android/widget/TextView");
    jmethodID delegate = parent ? (*e)->GetMethodID(e, parent, "onTextContextMenuItem", "(I)Z") : NULL;
    if (action != ANDROID_COPY)
        return delegate ? (*e)->CallNonvirtualBooleanMethod(e, self, parent, delegate, action) : JNI_FALSE;
    if ((*e)->PushLocalFrame(e, 40) != 0) return JNI_FALSE;
    jstring source = NULL, display = NULL, coordinates = NULL;
    jmethodID start = sdk_method(e, "android/widget/TextView", "getSelectionStart", "()I");
    jmethodID finish = start ? sdk_method(e, "android/widget/TextView", "getSelectionEnd", "()I") : NULL;
    jint first = start ? (*e)->CallIntMethod(e, self, start) : -1;
    jint after = finish ? (*e)->CallIntMethod(e, self, finish) : -1;
    if (first > after) { jint temporary = first; first = after; after = temporary; }
    int ok = first >= 0 && after > first && projection_texts(e, self, &source, &display, &coordinates);
    jclass generated = ok ? (*e)->FindClass(e, "Idric/Generated") : NULL;
    jmethodID copy = generated ? (*e)->GetStaticMethodID(e, generated, "copy_reader_selection",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II)Ljava/lang/String;") : NULL;
    jstring copied = copy ? (*e)->CallStaticObjectMethod(e, generated, copy, source, display, coordinates, first, after) : NULL;
    ok = copied && !(*e)->ExceptionCheck(e) && (*e)->GetStringLength(e, copied) > 0;
    jobject context = ok ? activity_context(e, self) : NULL;
    jmethodID service = context ? sdk_method(e, "android/content/Context", "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;") : NULL;
    jstring name = service ? (*e)->NewStringUTF(e, "clipboard") : NULL;
    jobject clipboard = name ? (*e)->CallObjectMethod(e, context, service, name) : NULL;
    jclass clip_type = clipboard ? (*e)->FindClass(e, "android/content/ClipData") : NULL;
    jmethodID plain = clip_type ? (*e)->GetStaticMethodID(e, clip_type, "newPlainText",
        "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Landroid/content/ClipData;") : NULL;
    jstring label = plain ? (*e)->NewStringUTF(e, "IB source selection") : NULL;
    jobject clip = label ? (*e)->CallStaticObjectMethod(e, clip_type, plain, label, copied) : NULL;
    jmethodID assign = clip ? sdk_method(e, "android/content/ClipboardManager", "setPrimaryClip", "(Landroid/content/ClipData;)V") : NULL;
    if (assign && !(*e)->ExceptionCheck(e)) (*e)->CallVoidMethod(e, clipboard, assign, clip);
    ok = assign && !(*e)->ExceptionCheck(e);
    if (!ok) report_refusal(e, self, "This selection has no verified source coordinates; copy was refused.");
    (*e)->PopLocalFrame(e, NULL);
    return ok ? JNI_TRUE : JNI_FALSE;
}

static int has_selection(JNIEnv *e, jobject view) {
    jmethodID first = sdk_method(e, "android/widget/TextView", "getSelectionStart", "()I");
    jmethodID after = first ? sdk_method(e, "android/widget/TextView", "getSelectionEnd", "()I") : NULL;
    jint start = first ? (*e)->CallIntMethod(e, view, first) : -1;
    jint finish = after ? (*e)->CallIntMethod(e, view, after) : -1;
    return !after || (*e)->ExceptionCheck(e) || (start >= 0 && finish >= 0 && start != finish);
}

static int follow_tap(JNIEnv *e, jobject view, jobject event) {
    jmethodID x_method = sdk_method(e, "android/view/MotionEvent", "getX", "()F");
    jmethodID y_method = x_method ? sdk_method(e, "android/view/MotionEvent", "getY", "()F") : NULL;
    jfloat x = x_method ? (*e)->CallFloatMethod(e, event, x_method) : NAN;
    jfloat y = y_method ? (*e)->CallFloatMethod(e, event, y_method) : NAN;
    jmethodID layout_method = sdk_method(e, "android/widget/TextView", "getLayout", "()Landroid/text/Layout;");
    jobject layout = layout_method ? (*e)->CallObjectMethod(e, view, layout_method) : NULL;
    jmethodID padding_x = sdk_method(e, "android/widget/TextView", "getTotalPaddingLeft", "()I");
    jmethodID padding_y = sdk_method(e, "android/widget/TextView", "getTotalPaddingTop", "()I");
    jmethodID scroll_x = sdk_method(e, "android/view/View", "getScrollX", "()I");
    jmethodID scroll_y = sdk_method(e, "android/view/View", "getScrollY", "()I");
    jmethodID height_method = sdk_method(e, "android/text/Layout", "getHeight", "()I");
    if (!layout || !padding_x || !padding_y || !scroll_x || !scroll_y || !height_method ||
        !isfinite(x) || !isfinite(y) || (*e)->ExceptionCheck(e)) return 0;
    jfloat text_x = x - (*e)->CallIntMethod(e, view, padding_x) + (*e)->CallIntMethod(e, view, scroll_x);
    jfloat text_y = y - (*e)->CallIntMethod(e, view, padding_y) + (*e)->CallIntMethod(e, view, scroll_y);
    jint height = (*e)->CallIntMethod(e, layout, height_method);
    if (text_y < 0 || text_y >= height) return 0;
    jmethodID line_method = sdk_method(e, "android/text/Layout", "getLineForVertical", "(I)I");
    jmethodID left_method = sdk_method(e, "android/text/Layout", "getLineLeft", "(I)F");
    jmethodID right_method = sdk_method(e, "android/text/Layout", "getLineRight", "(I)F");
    jmethodID offset_method = sdk_method(e, "android/widget/TextView", "getOffsetForPosition", "(FF)I");
    if (!line_method || !left_method || !right_method || !offset_method || (*e)->ExceptionCheck(e)) return 0;
    jint line = (*e)->CallIntMethod(e, layout, line_method, (jint)text_y);
    jfloat left = (*e)->CallFloatMethod(e, layout, left_method, line);
    jfloat right = (*e)->CallFloatMethod(e, layout, right_method, line);
    if (text_x < left || text_x >= right || (*e)->ExceptionCheck(e)) return 0;
    jint point = (*e)->CallIntMethod(e, view, offset_method, x, y);
    jstring source = NULL, display = NULL, coordinates = NULL;
    if (!projection_texts(e, view, &source, &display, &coordinates)) return 0;
    jclass generated = (*e)->FindClass(e, "Idric/Generated");
    jmethodID link = generated ? (*e)->GetStaticMethodID(e, generated, "reader_link_at",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)Ljava/lang/String;") : NULL;
    jstring url = link ? (*e)->CallStaticObjectMethod(e, generated, link, source, display, coordinates, point) : NULL;
    if (!url || (*e)->ExceptionCheck(e) || (*e)->GetStringLength(e, url) == 0) return 0;
    jobject activity = activity_context(e, view);
    jmethodID follow = activity ? (*e)->GetStaticMethodID(e, generated, "reader_follow_link",
        "(Landroid/app/Activity;Ljava/lang/String;)V") : NULL;
    if (follow) (*e)->CallStaticVoidMethod(e, generated, follow, activity, url);
    return follow && !(*e)->ExceptionCheck(e);
}

JNIEXPORT jboolean JNICALL
Java_org_isomorphisms_ib_NativeTextView_onTouchEvent(JNIEnv *e, jobject self, jobject event) {
    if ((*e)->PushLocalFrame(e, 64) != 0) return JNI_FALSE;
    jclass parent = (*e)->FindClass(e, "android/widget/TextView");
    jmethodID delegate = parent ? (*e)->GetMethodID(e, parent, "onTouchEvent", "(Landroid/view/MotionEvent;)Z") : NULL;
    jmethodID action_method = sdk_method(e, "android/view/MotionEvent", "getActionMasked", "()I");
    jmethodID points_method = sdk_method(e, "android/view/MotionEvent", "getPointerCount", "()I");
    jfieldID storage = view_field(e, self, "gestureStart", "Landroid/view/MotionEvent;");
    jint action = action_method ? (*e)->CallIntMethod(e, event, action_method) : -1;
    jint points = points_method ? (*e)->CallIntMethod(e, event, points_method) : 0;
    jobject initial = storage ? (*e)->GetObjectField(e, self, storage) : NULL;
    jmethodID recycle = sdk_method(e, "android/view/MotionEvent", "recycle", "()V");
    int tap = 0, eligible = 0;
    if (action == 0 && storage && recycle && !(*e)->ExceptionCheck(e)) {
        if (initial) (*e)->CallVoidMethod(e, initial, recycle);
        (*e)->SetObjectField(e, self, storage, NULL);
        if (points == 1 && !has_selection(e, self)) {
            jclass motion = (*e)->FindClass(e, "android/view/MotionEvent");
            jmethodID obtain = motion ? (*e)->GetStaticMethodID(e, motion, "obtain", "(Landroid/view/MotionEvent;)Landroid/view/MotionEvent;") : NULL;
            jobject snapshot = obtain ? (*e)->CallStaticObjectMethod(e, motion, obtain, event) : NULL;
            if (snapshot) (*e)->SetObjectField(e, self, storage, snapshot);
        }
    } else if ((action == 1 || action == 2) && initial && points == 1 && !has_selection(e, self)) {
        jmethodID x_method = sdk_method(e, "android/view/MotionEvent", "getX", "()F");
        jmethodID y_method = sdk_method(e, "android/view/MotionEvent", "getY", "()F");
        jmethodID time_method = sdk_method(e, "android/view/MotionEvent", "getEventTime", "()J");
        jclass configuration = (*e)->FindClass(e, "android/view/ViewConfiguration");
        jmethodID timeout_method = configuration ? (*e)->GetStaticMethodID(e, configuration, "getLongPressTimeout", "()I") : NULL;
        jmethodID get_configuration = configuration ? (*e)->GetStaticMethodID(e, configuration, "get", "(Landroid/content/Context;)Landroid/view/ViewConfiguration;") : NULL;
        jobject context = activity_context(e, self);
        jobject settings = get_configuration && context ? (*e)->CallStaticObjectMethod(e, configuration, get_configuration, context) : NULL;
        jmethodID slop_method = settings ? sdk_method(e, "android/view/ViewConfiguration", "getScaledTouchSlop", "()I") : NULL;
        if (x_method && y_method && time_method && timeout_method && slop_method && !(*e)->ExceptionCheck(e)) {
            jfloat dx = (*e)->CallFloatMethod(e, event, x_method) - (*e)->CallFloatMethod(e, initial, x_method);
            jfloat dy = (*e)->CallFloatMethod(e, event, y_method) - (*e)->CallFloatMethod(e, initial, y_method);
            jlong duration = (*e)->CallLongMethod(e, event, time_method) - (*e)->CallLongMethod(e, initial, time_method);
            jint limit = (*e)->CallStaticIntMethod(e, configuration, timeout_method);
            jint slop = (*e)->CallIntMethod(e, settings, slop_method);
            eligible = !(*e)->ExceptionCheck(e) && isfinite(dx) && isfinite(dy) && duration >= 0 &&
                duration < limit && slop > 0 && dx * dx + dy * dy <= (jfloat)slop * slop;
            tap = action == 1 && eligible;
        }
    }
    /* Once a drag/selection/long press leaves the tap contract it cannot
     * become a link click by returning to its initial position. */
    if (action == 2 && initial && !eligible && storage && recycle && !(*e)->ExceptionCheck(e)) {
        (*e)->CallVoidMethod(e, initial, recycle);
        (*e)->SetObjectField(e, self, storage, NULL);
        initial = NULL;
    }
    if ((action == 1 || action == 3 || points != 1) && storage && recycle && !(*e)->ExceptionCheck(e)) {
        if (initial) (*e)->CallVoidMethod(e, initial, recycle);
        (*e)->SetObjectField(e, self, storage, NULL);
    }
    jboolean handled = delegate && !(*e)->ExceptionCheck(e) ?
        (*e)->CallNonvirtualBooleanMethod(e, self, parent, delegate, event) : JNI_FALSE;
    if (tap && !(*e)->ExceptionCheck(e) && follow_tap(e, self, event)) handled = JNI_TRUE;
    if ((*e)->ExceptionCheck(e)) report_refusal(e, self, "The source-linked native action failed; the document is retained.");
    (*e)->PopLocalFrame(e, NULL);
    return handled;
}

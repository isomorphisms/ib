module ib_native;

/*
 * IB's Android shell.  It is intentionally -betterC: no druntime, Phobos,
 * garbage collector, exceptions, classes, or Java-owned application state.
 */

alias JObject = void*;
alias JClass = void*;
alias JMethod = void*;
alias JString = void*;

union JValue {
    ubyte z;
    byte b;
    ushort c;
    short s;
    int i;
    long j;
    float f;
    double d;
    JObject l;
}

struct ANativeActivity {
    void* callbacks;
    void* vm;
    void* env;
    JObject clazz;
    const(char)* internalDataPath;
    const(char)* externalDataPath;
    int sdkVersion;
    void* instance;
    void* assetManager;
    const(char)* obbPath;
}

struct HttpResult {
    int curlCode;
    int httpStatus;
    uint length;
    int truncated;
    char[192] error;
}

alias ReadyCallback = extern(C) void function() nothrow @nogc;
alias ThreadEntry = extern(C) void* function(void*) nothrow @nogc;

extern(C) nothrow @nogc {
    void* ib_jni_get_env(void* machine);
    JClass ib_jni_find_class(void* env, const(char)* name);
    JClass ib_jni_get_object_class(void* env, JObject object);
    JMethod ib_jni_get_method_id(void* env, JClass type,
                                 const(char)* name, const(char)* signature);
    JObject ib_jni_new_object_a(void* env, JClass type, JMethod constructor,
                                const(JValue)* arguments);
    void ib_jni_call_void_method_a(void* env, JObject object, JMethod method,
                                   const(JValue)* arguments);
    JObject ib_jni_call_object_method_a(void* env, JObject object,
                                        JMethod method,
                                        const(JValue)* arguments);
    int ib_jni_call_int_method_a(void* env, JObject object, JMethod method,
                                 const(JValue)* arguments);
    ubyte ib_jni_call_boolean_method_a(void* env, JObject object,
                                       JMethod method,
                                       const(JValue)* arguments);
    JString ib_jni_new_string(void* env, const(ushort)* characters, int length);
    int ib_jni_get_string_length(void* env, JString string);
    const(ushort)* ib_jni_get_string_chars(void* env, JString string);
    void ib_jni_release_string_chars(void* env, JString string,
                                     const(ushort)* characters);
    JObject ib_jni_new_global_ref(void* env, JObject object);
    void ib_jni_delete_global_ref(void* env, JObject object);
    void ib_jni_delete_local_ref(void* env, JObject object);
    ubyte ib_jni_exception_check(void* env);
    void ib_jni_exception_describe_clear(void* env);

    int ib_http_get(const(char)* url, const(void)* caData, uint caLength,
                    ubyte* body, uint bodyCapacity, HttpResult* result);
    int ib_network_prepare(ReadyCallback callback);
    void ib_network_signal_main();
    int ib_network_try_begin();
    void ib_network_end();
    int ib_start_detached_thread(ThreadEntry entry, void* argument);

    void* AAssetManager_open(void* manager, const(char)* filename, int mode);
    int AAsset_read(void* asset, void* buffer, uint count);
    void AAsset_close(void* asset);

    int read(int descriptor, void* buffer, uint count);
    int close(int descriptor);
    int __android_log_write(int priority, const(char)* tag,
                            const(char)* message);
}

private enum int OPEN_DOCUMENT = 1;
private enum int READ_CLIPBOARD = 2;
private enum int FETCH_URL = 3;
private enum int OPEN_DOCUMENT_REQUEST = 7;
private enum int RESULT_OK = -1;
private enum uint MAX_DOCUMENTS = 4;
private enum uint MAX_FILE_BYTES = 24 * 1024;
private enum uint DOCUMENT_CAPACITY = 48 * 1024;
private enum uint CA_CAPACITY = 320 * 1024;
private enum uint URL_CAPACITY = 2 * 1024;
private enum uint RESPONSE_CAPACITY = 64 * 1024;

private __gshared void* javaVm;
private __gshared void* nativeAssetManager;
private __gshared JObject activityReference;
private __gshared JObject statusReference;
private __gshared JObject contentReference;
private __gshared ushort[DOCUMENT_CAPACITY] documentText;
private __gshared uint documentLength;
private __gshared ubyte[CA_CAPACITY] authorityBundle;
private __gshared uint authorityLength;
private __gshared char[URL_CAPACITY] requestUrl;
private __gshared ubyte[RESPONSE_CAPACITY] networkBody;
private __gshared HttpResult networkResult;

private void logInfo(const(char)[] message) nothrow @nogc {
    __android_log_write(4, "IB-D".ptr, message.ptr);
}

private JMethod method(void* env, const(char)[] className,
                       const(char)[] name, const(char)[] signature)
                       nothrow @nogc {
    JClass type = ib_jni_find_class(env, className.ptr);
    if (type is null) {
        return null;
    }
    JMethod result = ib_jni_get_method_id(env, type, name.ptr, signature.ptr);
    ib_jni_delete_local_ref(env, type);
    return result;
}

private JObject callObject(void* env, JObject object,
                           const(char)[] className, const(char)[] name,
                           const(char)[] signature,
                           const(JValue)* arguments = null)
                           nothrow @nogc {
    JMethod id = method(env, className, name, signature);
    return id is null ? null
                      : ib_jni_call_object_method_a(env, object, id, arguments);
}

private int callInt(void* env, JObject object, const(char)[] className,
                    const(char)[] name, const(char)[] signature,
                    const(JValue)* arguments = null) nothrow @nogc {
    JMethod id = method(env, className, name, signature);
    return id is null ? 0
                      : ib_jni_call_int_method_a(env, object, id, arguments);
}

private bool callBoolean(void* env, JObject object, const(char)[] className,
                         const(char)[] name, const(char)[] signature,
                         const(JValue)* arguments = null) nothrow @nogc {
    JMethod id = method(env, className, name, signature);
    return id !is null &&
           ib_jni_call_boolean_method_a(env, object, id, arguments) != 0;
}

private void callVoid(void* env, JObject object, const(char)[] className,
                      const(char)[] name, const(char)[] signature,
                      const(JValue)* arguments = null) nothrow @nogc {
    JMethod id = method(env, className, name, signature);
    if (id !is null) {
        ib_jni_call_void_method_a(env, object, id, arguments);
    }
}

private JObject newObject(void* env, const(char)[] className,
                          const(char)[] constructorSignature,
                          const(JValue)* arguments = null) nothrow @nogc {
    JClass type = ib_jni_find_class(env, className.ptr);
    if (type is null) {
        return null;
    }
    JMethod constructor = ib_jni_get_method_id(
        env, type, "<init>".ptr, constructorSignature.ptr);
    JObject result = constructor is null ? null
        : ib_jni_new_object_a(env, type, constructor, arguments);
    ib_jni_delete_local_ref(env, type);
    return result;
}

private JString newAsciiString(void* env, const(char)[] text) nothrow @nogc {
    ushort[512] encoded;
    uint length = cast(uint) text.length;
    if (length > encoded.length) {
        length = cast(uint) encoded.length;
    }
    for (uint index = 0; index < length; ++index) {
        encoded[index] = cast(ubyte) text[index];
    }
    return ib_jni_new_string(env, encoded.ptr, cast(int) length);
}

private void setTextObject(void* env, JObject view, JObject text)
                           nothrow @nogc {
    JValue[1] arguments;
    arguments[0].l = text;
    callVoid(env, view, "android/widget/TextView", "setText",
             "(Ljava/lang/CharSequence;)V", arguments.ptr);
}

private void setTextAscii(void* env, JObject view, const(char)[] text)
                          nothrow @nogc {
    JString value = newAsciiString(env, text);
    if (value !is null) {
        setTextObject(env, view, value);
        ib_jni_delete_local_ref(env, value);
    }
}

private void setTextUtf16(void* env, JObject view, const(ushort)* text,
                          uint length) nothrow @nogc {
    JString value = ib_jni_new_string(env, text, cast(int) length);
    if (value !is null) {
        setTextObject(env, view, value);
        ib_jni_delete_local_ref(env, value);
    }
}

private void setStatus(void* env, const(char)[] text) nothrow @nogc {
    if (statusReference !is null) {
        setTextAscii(env, statusReference, text);
    }
}

private void addView(void* env, JObject parent, const(char)[] parentClass,
                     JObject child) nothrow @nogc {
    JValue[1] arguments;
    arguments[0].l = child;
    callVoid(env, parent, parentClass, "addView",
             "(Landroid/view/View;)V", arguments.ptr);
}

private JObject makeTextView(void* env, JObject activity,
                             const(char)[] className,
                             const(char)[] text) nothrow @nogc {
    JValue[1] arguments;
    arguments[0].l = activity;
    JObject view = newObject(env, className,
                             "(Landroid/content/Context;)V", arguments.ptr);
    if (view !is null) {
        setTextAscii(env, view, text);
    }
    return view;
}

private void setInt(void* env, JObject object, const(char)[] className,
                    const(char)[] name, int value) nothrow @nogc {
    JValue[1] arguments;
    arguments[0].i = value;
    callVoid(env, object, className, name, "(I)V", arguments.ptr);
}

private void setBoolean(void* env, JObject object, const(char)[] className,
                        const(char)[] name, bool value) nothrow @nogc {
    JValue[1] arguments;
    arguments[0].z = value ? 1 : 0;
    callVoid(env, object, className, name, "(Z)V", arguments.ptr);
}

private bool buildScreen(void* env, JObject activity) nothrow @nogc {
    JValue[1] context;
    context[0].l = activity;
    JObject root = newObject(env, "android/widget/LinearLayout",
                             "(Landroid/content/Context;)V", context.ptr);
    if (root is null) {
        return false;
    }

    setInt(env, root, "android/widget/LinearLayout", "setOrientation", 1);
    setInt(env, root, "android/view/View", "setBackgroundColor",
           cast(int) 0xff121518);
    JValue[4] padding;
    padding[0].i = 28;
    padding[1].i = 24;
    padding[2].i = 28;
    padding[3].i = 28;
    callVoid(env, root, "android/view/View", "setPadding", "(IIII)V",
             padding.ptr);

    JObject title = makeTextView(env, activity, "android/widget/TextView",
                                 "IB / D NATIVE");
    JObject status = makeTextView(env, activity, "android/widget/TextView",
                                  "Ready - D owns this screen");
    JObject open = makeTextView(env, activity, "android/widget/Button",
                                "Open documents");
    JObject clipboard = makeTextView(env, activity, "android/widget/Button",
                                     "Paste clipboard");
    JObject fetch = makeTextView(env, activity, "android/widget/Button",
                                 "Fetch current HTTPS URL");
    JObject scroll = newObject(env, "android/widget/ScrollView",
                               "(Landroid/content/Context;)V", context.ptr);
    JObject content = makeTextView(
        env, activity, "android/widget/TextView",
        "Choose up to four text files. Android returns URI grants; D detaches " ~
        "and reads their file descriptors. You can also share text or a URL " ~
        "to IB, paste the clipboard explicitly, then fetch an HTTPS URL with " ~
        "native curl and mbedTLS.");

    if (title is null || status is null || open is null || clipboard is null ||
        fetch is null || scroll is null || content is null) {
        ib_jni_exception_describe_clear(env);
        return false;
    }

    setInt(env, title, "android/widget/TextView", "setTextColor",
           cast(int) 0xff8bc4ff);
    setInt(env, status, "android/widget/TextView", "setTextColor",
           cast(int) 0xffb9c1c8);
    setInt(env, content, "android/widget/TextView", "setTextColor",
           cast(int) 0xfff5f7f8);
    setBoolean(env, content, "android/widget/TextView", "setTextIsSelectable",
               true);
    setBoolean(env, scroll, "android/widget/ScrollView", "setFillViewport",
               true);

    setInt(env, open, "android/view/View", "setId", OPEN_DOCUMENT);
    setInt(env, clipboard, "android/view/View", "setId", READ_CLIPBOARD);
    setInt(env, fetch, "android/view/View", "setId", FETCH_URL);
    JValue[1] listener;
    listener[0].l = activity;
    callVoid(env, open, "android/view/View", "setOnClickListener",
             "(Landroid/view/View$OnClickListener;)V", listener.ptr);
    callVoid(env, clipboard, "android/view/View", "setOnClickListener",
             "(Landroid/view/View$OnClickListener;)V", listener.ptr);
    callVoid(env, fetch, "android/view/View", "setOnClickListener",
             "(Landroid/view/View$OnClickListener;)V", listener.ptr);

    addView(env, scroll, "android/widget/ScrollView", content);
    addView(env, root, "android/widget/LinearLayout", title);
    addView(env, root, "android/widget/LinearLayout", status);
    addView(env, root, "android/widget/LinearLayout", open);
    addView(env, root, "android/widget/LinearLayout", clipboard);
    addView(env, root, "android/widget/LinearLayout", fetch);
    addView(env, root, "android/widget/LinearLayout", scroll);

    JValue[1] screen;
    screen[0].l = root;
    callVoid(env, activity, "android/app/Activity", "setContentView",
             "(Landroid/view/View;)V", screen.ptr);

    if (ib_jni_exception_check(env) != 0) {
        ib_jni_exception_describe_clear(env);
        return false;
    }

    statusReference = ib_jni_new_global_ref(env, status);
    contentReference = ib_jni_new_global_ref(env, content);

    ib_jni_delete_local_ref(env, content);
    ib_jni_delete_local_ref(env, scroll);
    ib_jni_delete_local_ref(env, fetch);
    ib_jni_delete_local_ref(env, clipboard);
    ib_jni_delete_local_ref(env, open);
    ib_jni_delete_local_ref(env, status);
    ib_jni_delete_local_ref(env, title);
    ib_jni_delete_local_ref(env, root);
    return statusReference !is null && contentReference !is null;
}

private void openDocumentPicker(void* env) nothrow @nogc {
    JString action = newAsciiString(env, "android.intent.action.OPEN_DOCUMENT");
    JValue[1] constructorArguments;
    constructorArguments[0].l = action;
    JObject intent = newObject(env, "android/content/Intent",
                               "(Ljava/lang/String;)V",
                               constructorArguments.ptr);
    if (intent is null) {
        ib_jni_exception_describe_clear(env);
        setStatus(env, "Could not construct the document picker request.");
        return;
    }

    JString category = newAsciiString(env, "android.intent.category.OPENABLE");
    JValue[1] one;
    one[0].l = category;
    JObject chained = callObject(env, intent, "android/content/Intent",
                                 "addCategory",
                                 "(Ljava/lang/String;)Landroid/content/Intent;",
                                 one.ptr);
    if (chained !is null) ib_jni_delete_local_ref(env, chained);

    JString type = newAsciiString(env, "text/*");
    one[0].l = type;
    chained = callObject(env, intent, "android/content/Intent", "setType",
                         "(Ljava/lang/String;)Landroid/content/Intent;",
                         one.ptr);
    if (chained !is null) ib_jni_delete_local_ref(env, chained);

    JString multiple = newAsciiString(
        env, "android.intent.extra.ALLOW_MULTIPLE");
    JValue[2] extra;
    extra[0].l = multiple;
    extra[1].z = 1;
    chained = callObject(env, intent, "android/content/Intent", "putExtra",
                         "(Ljava/lang/String;Z)Landroid/content/Intent;",
                         extra.ptr);
    if (chained !is null) ib_jni_delete_local_ref(env, chained);

    JValue[1] flags;
    flags[0].i = 1; // Intent.FLAG_GRANT_READ_URI_PERMISSION
    chained = callObject(env, intent, "android/content/Intent", "addFlags",
                         "(I)Landroid/content/Intent;", flags.ptr);
    if (chained !is null) ib_jni_delete_local_ref(env, chained);

    JValue[2] start;
    start[0].l = intent;
    start[1].i = OPEN_DOCUMENT_REQUEST;
    callVoid(env, activityReference, "android/app/Activity",
             "startActivityForResult", "(Landroid/content/Intent;I)V",
             start.ptr);
    if (ib_jni_exception_check(env) != 0) {
        ib_jni_exception_describe_clear(env);
        setStatus(env, "No system document picker is available.");
    } else {
        setStatus(env, "Waiting for document selection...");
    }

    ib_jni_delete_local_ref(env, multiple);
    ib_jni_delete_local_ref(env, type);
    ib_jni_delete_local_ref(env, category);
    ib_jni_delete_local_ref(env, intent);
    ib_jni_delete_local_ref(env, action);
}

private void appendCodePoint(uint point) nothrow @nogc {
    if (point <= 0xffff) {
        if (point >= 0xd800 && point <= 0xdfff) point = 0xfffd;
        if (documentLength < documentText.length) {
            documentText[documentLength++] = cast(ushort) point;
        }
        return;
    }
    if (point > 0x10ffff || documentLength + 1 >= documentText.length) {
        if (documentLength < documentText.length) {
            documentText[documentLength++] = 0xfffd;
        }
        return;
    }
    point -= 0x10000;
    documentText[documentLength++] = cast(ushort) (0xd800 + (point >> 10));
    documentText[documentLength++] = cast(ushort) (0xdc00 + (point & 0x3ff));
}

private void appendAscii(const(char)[] text) nothrow @nogc {
    for (uint index = 0;
         index < text.length && documentLength < documentText.length;
         ++index) {
        documentText[documentLength++] = cast(ubyte) text[index];
    }
}

private void appendUnsigned(uint value) nothrow @nogc {
    ushort[10] digits;
    uint count;
    do {
        digits[count++] = cast(ushort) ('0' + value % 10);
        value /= 10;
    } while (value != 0 && count < digits.length);
    while (count != 0 && documentLength < documentText.length) {
        documentText[documentLength++] = digits[--count];
    }
}

private void appendUtf8(const(ubyte)* bytes, uint length) nothrow @nogc {
    uint index;
    while (index < length && documentLength < documentText.length) {
        uint first = bytes[index];
        if (first < 0x80) {
            appendCodePoint(first);
            ++index;
            continue;
        }

        uint point;
        uint needed;
        uint minimum;
        if ((first & 0xe0) == 0xc0) {
            point = first & 0x1f;
            needed = 1;
            minimum = 0x80;
        } else if ((first & 0xf0) == 0xe0) {
            point = first & 0x0f;
            needed = 2;
            minimum = 0x800;
        } else if ((first & 0xf8) == 0xf0) {
            point = first & 0x07;
            needed = 3;
            minimum = 0x10000;
        } else {
            appendCodePoint(0xfffd);
            ++index;
            continue;
        }

        if (index + needed >= length) {
            appendCodePoint(0xfffd);
            break;
        }
        bool valid = true;
        for (uint offset = 1; offset <= needed; ++offset) {
            uint next = bytes[index + offset];
            if ((next & 0xc0) != 0x80) {
                valid = false;
                break;
            }
            point = (point << 6) | (next & 0x3f);
        }
        if (!valid || point < minimum || point > 0x10ffff ||
            (point >= 0xd800 && point <= 0xdfff)) {
            appendCodePoint(0xfffd);
            ++index;
            continue;
        }
        appendCodePoint(point);
        index += needed + 1;
    }
}

private int readFileDescriptor(int descriptor, ubyte* destination,
                               uint capacity, bool* truncated)
                               nothrow @nogc {
    uint length;
    *truncated = false;
    while (length < capacity) {
        int amount = read(descriptor, destination + length, capacity - length);
        if (amount < 0) return -1;
        if (amount == 0) return cast(int) length;
        length += cast(uint) amount;
    }
    ubyte extra;
    int amount = read(descriptor, &extra, 1);
    if (amount > 0) *truncated = true;
    return cast(int) length;
}

private int importUri(void* env, JObject resolver, JObject uri,
                      uint ordinal, bool* truncated) nothrow @nogc {
    JString mode = newAsciiString(env, "r");
    JValue[2] openArguments;
    openArguments[0].l = uri;
    openArguments[1].l = mode;
    JObject descriptor = callObject(
        env, resolver, "android/content/ContentResolver", "openFileDescriptor",
        "(Landroid/net/Uri;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;",
        openArguments.ptr);
    ib_jni_delete_local_ref(env, mode);
    if (descriptor is null) {
        ib_jni_exception_describe_clear(env);
        return -1;
    }

    int fd = callInt(env, descriptor, "android/os/ParcelFileDescriptor",
                     "detachFd", "()I");
    if (fd < 0 || ib_jni_exception_check(env) != 0) {
        ib_jni_exception_describe_clear(env);
        ib_jni_delete_local_ref(env, descriptor);
        return -1;
    }

    ubyte[MAX_FILE_BYTES] bytes;
    int length = readFileDescriptor(fd, bytes.ptr, bytes.length, truncated);
    close(fd);
    ib_jni_delete_local_ref(env, descriptor);
    if (length < 0) return -1;

    if (documentLength != 0) appendAscii("\n\n");
    appendAscii("[document ");
    appendUnsigned(ordinal);
    appendAscii("]\n");
    appendUtf8(bytes.ptr, cast(uint) length);
    if (*truncated) appendAscii("\n[truncated at 24 KiB]");
    return length;
}

private void showImportStatus(void* env, uint count, uint bytes,
                              bool truncated) nothrow @nogc {
    ushort[96] status;
    uint length;
    const(char)[] prefix = "Imported ";
    foreach (character; prefix) status[length++] = cast(ubyte) character;

    ushort[10] digits;
    uint digitCount;
    uint remaining = count;
    do {
        digits[digitCount++] = cast(ushort) ('0' + remaining % 10);
        remaining /= 10;
    } while (remaining != 0);
    while (digitCount != 0) status[length++] = digits[--digitCount];

    const(char)[] middle = count == 1 ? " document / " : " documents / ";
    foreach (character; middle) status[length++] = cast(ubyte) character;
    remaining = bytes;
    digitCount = 0;
    do {
        digits[digitCount++] = cast(ushort) ('0' + remaining % 10);
        remaining /= 10;
    } while (remaining != 0);
    while (digitCount != 0) status[length++] = digits[--digitCount];
    foreach (character; " bytes") status[length++] = cast(ubyte) character;
    if (truncated) {
        foreach (character; " / bounded preview")
            status[length++] = cast(ubyte) character;
    }
    setTextUtf16(env, statusReference, status.ptr, length);
}

private void receiveDocuments(void* env, JObject intent) nothrow @nogc {
    JObject resolver = callObject(env, activityReference, "android/app/Activity",
                                  "getContentResolver",
                                  "()Landroid/content/ContentResolver;");
    if (resolver is null) {
        ib_jni_exception_describe_clear(env);
        setStatus(env, "Android did not provide a content resolver.");
        return;
    }

    documentLength = 0;
    uint imported;
    uint bytesRead;
    bool anyTruncated;
    JObject clip = callObject(env, intent, "android/content/Intent",
                              "getClipData", "()Landroid/content/ClipData;");
    if (clip !is null) {
        int available = callInt(env, clip, "android/content/ClipData",
                                "getItemCount", "()I");
        uint limit = available > cast(int) MAX_DOCUMENTS
            ? MAX_DOCUMENTS : cast(uint) available;
        for (uint index = 0; index < limit; ++index) {
            JValue[1] itemArgument;
            itemArgument[0].i = cast(int) index;
            JObject item = callObject(
                env, clip, "android/content/ClipData", "getItemAt",
                "(I)Landroid/content/ClipData$Item;", itemArgument.ptr);
            JObject uri = item is null ? null : callObject(
                env, item, "android/content/ClipData$Item", "getUri",
                "()Landroid/net/Uri;");
            if (uri !is null) {
                bool truncated;
                int amount = importUri(env, resolver, uri, imported + 1,
                                       &truncated);
                if (amount >= 0) {
                    ++imported;
                    bytesRead += cast(uint) amount;
                    anyTruncated = anyTruncated || truncated;
                }
                ib_jni_delete_local_ref(env, uri);
            }
            if (item !is null) ib_jni_delete_local_ref(env, item);
        }
        ib_jni_delete_local_ref(env, clip);
    } else {
        JObject uri = callObject(env, intent, "android/content/Intent",
                                 "getData", "()Landroid/net/Uri;");
        if (uri !is null) {
            bool truncated;
            int amount = importUri(env, resolver, uri, 1, &truncated);
            if (amount >= 0) {
                imported = 1;
                bytesRead = cast(uint) amount;
                anyTruncated = truncated;
            }
            ib_jni_delete_local_ref(env, uri);
        }
    }
    ib_jni_delete_local_ref(env, resolver);

    if (imported == 0) {
        setStatus(env, "No readable document was returned.");
        return;
    }
    setTextUtf16(env, contentReference, documentText.ptr, documentLength);
    showImportStatus(env, imported, bytesRead, anyTruncated);
}

private bool copyJavaString(void* env, JString string) nothrow @nogc {
    if (string is null) return false;
    int available = ib_jni_get_string_length(env, string);
    const(ushort)* characters = ib_jni_get_string_chars(env, string);
    if (available < 0 || characters is null) return false;
    uint count = available > cast(int) documentText.length
        ? cast(uint) documentText.length : cast(uint) available;
    for (uint index = 0; index < count; ++index) {
        documentText[index] = characters[index];
    }
    documentLength = count;
    ib_jni_release_string_chars(env, string, characters);
    return true;
}

private bool receiveSharedText(void* env, JObject intent) nothrow @nogc {
    if (intent is null) return false;
    JString key = newAsciiString(env, "android.intent.extra.TEXT");
    JValue[1] extra;
    extra[0].l = key;
    JObject sequence = callObject(
        env, intent, "android/content/Intent", "getCharSequenceExtra",
        "(Ljava/lang/String;)Ljava/lang/CharSequence;", extra.ptr);
    ib_jni_delete_local_ref(env, key);
    if (sequence is null) {
        if (ib_jni_exception_check(env) != 0) {
            ib_jni_exception_describe_clear(env);
        }
        return false;
    }

    JString string = callObject(env, sequence, "java/lang/Object", "toString",
                                "()Ljava/lang/String;");
    bool copied = copyJavaString(env, string);
    if (copied) {
        setTextUtf16(env, contentReference, documentText.ptr, documentLength);
        setStatus(env, "Received shared text or URL.");
    }
    if (string !is null) ib_jni_delete_local_ref(env, string);
    ib_jni_delete_local_ref(env, sequence);
    return copied;
}

private void readClipboard(void* env) nothrow @nogc {
    JString serviceName = newAsciiString(env, "clipboard");
    JValue[1] serviceArgument;
    serviceArgument[0].l = serviceName;
    JObject clipboard = callObject(
        env, activityReference, "android/app/Activity", "getSystemService",
        "(Ljava/lang/String;)Ljava/lang/Object;", serviceArgument.ptr);
    ib_jni_delete_local_ref(env, serviceName);
    if (clipboard is null || !callBoolean(
            env, clipboard, "android/content/ClipboardManager",
            "hasPrimaryClip", "()Z")) {
        if (clipboard !is null) ib_jni_delete_local_ref(env, clipboard);
        setStatus(env, "The clipboard has no readable item.");
        return;
    }

    JObject clip = callObject(env, clipboard,
                              "android/content/ClipboardManager",
                              "getPrimaryClip", "()Landroid/content/ClipData;");
    int count = clip is null ? 0 : callInt(
        env, clip, "android/content/ClipData", "getItemCount", "()I");
    if (count <= 0) {
        if (clip !is null) ib_jni_delete_local_ref(env, clip);
        ib_jni_delete_local_ref(env, clipboard);
        setStatus(env, "The clipboard has no readable item.");
        return;
    }

    JValue[1] itemArgument;
    itemArgument[0].i = 0;
    JObject item = callObject(
        env, clip, "android/content/ClipData", "getItemAt",
        "(I)Landroid/content/ClipData$Item;", itemArgument.ptr);
    JValue[1] context;
    context[0].l = activityReference;
    JObject sequence = item is null ? null : callObject(
        env, item, "android/content/ClipData$Item", "coerceToText",
        "(Landroid/content/Context;)Ljava/lang/CharSequence;", context.ptr);
    JString string = sequence is null ? null : callObject(
        env, sequence, "java/lang/Object", "toString", "()Ljava/lang/String;");
    if (copyJavaString(env, string)) {
        setTextUtf16(env, contentReference, documentText.ptr, documentLength);
        setStatus(env, "Clipboard copied into D-owned state.");
    } else {
        ib_jni_exception_describe_clear(env);
        setStatus(env, "The clipboard item could not be converted to text.");
    }

    if (string !is null) ib_jni_delete_local_ref(env, string);
    if (sequence !is null) ib_jni_delete_local_ref(env, sequence);
    if (item !is null) ib_jni_delete_local_ref(env, item);
    ib_jni_delete_local_ref(env, clip);
    ib_jni_delete_local_ref(env, clipboard);
}

private uint loadAuthorityBundle() nothrow @nogc {
    if (nativeAssetManager is null) return 0;
    void* asset = AAssetManager_open(nativeAssetManager, "cacert.pem".ptr, 2);
    if (asset is null) return 0;
    uint length;
    while (length < authorityBundle.length) {
        int amount = AAsset_read(asset, authorityBundle.ptr + length,
                                 cast(uint) authorityBundle.length - length);
        if (amount <= 0) break;
        length += cast(uint) amount;
    }
    AAsset_close(asset);
    return length;
}

private bool prepareHttpsUrl() nothrow @nogc {
    uint first;
    uint last = documentLength;
    while (first < last && documentText[first] <= 0x20) ++first;
    while (last > first && documentText[last - 1] <= 0x20) --last;
    uint length = last - first;
    if (length < 9 || length >= requestUrl.length) return false;

    immutable char[] scheme = "https://";
    for (uint index = 0; index < scheme.length; ++index) {
        if (documentText[first + index] != cast(ushort) scheme[index]) {
            return false;
        }
    }
    for (uint index = 0; index < length; ++index) {
        ushort character = documentText[first + index];
        if (character < 0x21 || character > 0x7e) return false;
        requestUrl[index] = cast(char) character;
    }
    requestUrl[length] = '\0';
    return true;
}

private uint statusAscii(ushort* target, uint capacity, uint offset,
                         const(char)[] text) nothrow @nogc {
    for (uint index = 0; index < text.length && offset < capacity; ++index) {
        target[offset++] = cast(ubyte) text[index];
    }
    return offset;
}

private uint statusUnsigned(ushort* target, uint capacity, uint offset,
                            uint value) nothrow @nogc {
    ushort[10] digits;
    uint count;
    do {
        digits[count++] = cast(ushort) ('0' + value % 10);
        value /= 10;
    } while (value != 0 && count < digits.length);
    while (count != 0 && offset < capacity) {
        target[offset++] = digits[--count];
    }
    return offset;
}

private void showNetworkStatus(void* env) nothrow @nogc {
    ushort[320] status;
    uint length;
    bool received = networkResult.curlCode == 0 ||
                    (networkResult.truncated != 0 && networkResult.length != 0);
    if (received) {
        length = statusAscii(status.ptr, status.length, length, "HTTPS ");
        length = statusUnsigned(status.ptr, status.length, length,
                                cast(uint) networkResult.httpStatus);
        length = statusAscii(status.ptr, status.length, length, " / ");
        length = statusUnsigned(status.ptr, status.length, length,
                                networkResult.length);
        length = statusAscii(status.ptr, status.length, length, " bytes");
        if (networkResult.truncated != 0) {
            length = statusAscii(status.ptr, status.length, length,
                                 " / bounded preview");
        }
    } else {
        length = statusAscii(status.ptr, status.length, length,
                             "HTTPS failed / curl ");
        length = statusUnsigned(status.ptr, status.length, length,
                                cast(uint) networkResult.curlCode);
        length = statusAscii(status.ptr, status.length, length, " / ");
        for (uint index = 0;
             index < networkResult.error.length &&
             networkResult.error[index] != '\0' && length < status.length;
             ++index) {
            status[length++] = cast(ubyte) networkResult.error[index];
        }
    }
    setTextUtf16(env, statusReference, status.ptr, length);
}

extern(C) private void* fetchWorker(void* argument) nothrow @nogc {
    cast(void) argument;
    ib_http_get(requestUrl.ptr, authorityBundle.ptr, authorityLength,
                networkBody.ptr, cast(uint) networkBody.length,
                &networkResult);
    ib_network_signal_main();
    return null;
}

extern(C) private void networkReady() nothrow @nogc {
    void* env = ib_jni_get_env(javaVm);
    if (env !is null) {
        bool received = networkResult.curlCode == 0 ||
                        (networkResult.truncated != 0 &&
                         networkResult.length != 0);
        if (received && networkResult.length != 0) {
            documentLength = 0;
            appendUtf8(networkBody.ptr, networkResult.length);
            setTextUtf16(env, contentReference, documentText.ptr,
                         documentLength);
        }
        showNetworkStatus(env);
    }
    ib_network_end();
}

private void fetchCurrentUrl(void* env) nothrow @nogc {
    if (authorityLength == 0) {
        setStatus(env, "HTTPS CA bundle is unavailable.");
        return;
    }
    if (!prepareHttpsUrl()) {
        setStatus(env, "Share or paste one ASCII https:// URL first.");
        return;
    }
    if (ib_network_try_begin() == 0) {
        setStatus(env, "An HTTPS request is already running.");
        return;
    }
    setStatus(env, "Fetching with ARMv7 curl + mbedTLS...");
    if (ib_start_detached_thread(&fetchWorker, null) != 0) {
        ib_network_end();
        setStatus(env, "Could not start the native fetch worker.");
    }
}

extern(C) export void ANativeActivity_onCreate(ANativeActivity* activity,
                                               void* savedState,
                                               uint savedStateSize)
                                               nothrow @nogc {
    cast(void) savedState;
    cast(void) savedStateSize;
    if (activity is null || activity.env is null || activity.clazz is null) {
        return;
    }
    void* env = activity.env;
    javaVm = activity.vm;
    nativeAssetManager = activity.assetManager;
    activityReference = ib_jni_new_global_ref(env, activity.clazz);
    if (activityReference is null || !buildScreen(env, activity.clazz)) {
        ib_jni_exception_describe_clear(env);
        logInfo("Native screen construction failed");
        return;
    }

    authorityLength = loadAuthorityBundle();
    int networkPreparation = ib_network_prepare(&networkReady);
    if (networkPreparation != 0) {
        setStatus(env, "Local interface ready / main-thread network wakeup failed.");
    } else if (authorityLength == 0) {
        setStatus(env, "Local interface ready / HTTPS CA bundle missing.");
    }

    logInfo("D native activity created");
    JObject launchIntent = callObject(
        env, activityReference, "android/app/Activity", "getIntent",
        "()Landroid/content/Intent;");
    if (launchIntent !is null) {
        receiveSharedText(env, launchIntent);
        ib_jni_delete_local_ref(env, launchIntent);
    }
}

extern(C) export void ib_native_activity_result(void* env, JClass type,
                                                int requestCode,
                                                int resultCode, JObject data)
                                                nothrow @nogc {
    cast(void) type;
    if (requestCode != OPEN_DOCUMENT_REQUEST) return;
    if (resultCode != RESULT_OK) {
        setStatus(env, "Document selection cancelled.");
        return;
    }
    if (data is null) {
        setStatus(env, "Picker returned no document data.");
        return;
    }
    receiveDocuments(env, data);
}

extern(C) export void ib_native_new_intent(void* env, JClass type,
                                          JObject intent) nothrow @nogc {
    cast(void) type;
    if (!receiveSharedText(env, intent)) {
        setStatus(env, "Received an intent without shared text.");
    }
}

extern(C) export void ib_native_action(void* env, JClass type, int action)
                                      nothrow @nogc {
    cast(void) type;
    if (action == OPEN_DOCUMENT) {
        openDocumentPicker(env);
    } else if (action == READ_CLIPBOARD) {
        readClipboard(env);
    } else if (action == FETCH_URL) {
        fetchCurrentUrl(env);
    }
}

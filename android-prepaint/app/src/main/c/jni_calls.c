#include <jni.h>
#include <stdint.h>

/* Implemented in ib_native.d.  These are the complete Java -> D surface. */
extern void ib_native_activity_result(JNIEnv *, jclass, jint, jint, jobject);
extern void ib_native_new_intent(JNIEnv *, jclass, jobject);
extern void ib_native_action(JNIEnv *, jclass, jint);

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    jclass activity;
    (void) reserved;

    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    activity = (*env)->FindClass(env, "org/isomorphisms/ib/prepaint/IbActivity");
    if (activity == NULL) {
        return JNI_ERR;
    }

    const JNINativeMethod methods[] = {
        {"nativeActivityResult", "(IILandroid/content/Intent;)V",
         (void *) ib_native_activity_result},
        {"nativeNewIntent", "(Landroid/content/Intent;)V",
         (void *) ib_native_new_intent},
        {"nativeAction", "(I)V", (void *) ib_native_action},
    };
    const jint count = (jint) (sizeof(methods) / sizeof(methods[0]));
    const jint result = (*env)->RegisterNatives(env, activity, methods, count);
    (*env)->DeleteLocalRef(env, activity);
    return result == JNI_OK ? JNI_VERSION_1_6 : JNI_ERR;
}

/*
 * D deliberately does not mirror the implementation-defined JNIEnv table.
 * These one-for-one calls are the only C in the application and contain no
 * application policy, state, parsing, storage, or UI composition.
 */

jclass ib_jni_find_class(void *environment, const char *name) {
    JNIEnv *env = (JNIEnv *) environment;
    return (*env)->FindClass(env, name);
}

void *ib_jni_get_env(void *machine) {
    JavaVM *vm = (JavaVM *) machine;
    JNIEnv *env = NULL;
    if (vm == NULL ||
        (*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return NULL;
    }
    return env;
}

jclass ib_jni_get_object_class(void *environment, jobject object) {
    JNIEnv *env = (JNIEnv *) environment;
    return (*env)->GetObjectClass(env, object);
}

jmethodID ib_jni_get_method_id(void *environment, jclass type,
                               const char *name, const char *signature) {
    JNIEnv *env = (JNIEnv *) environment;
    return (*env)->GetMethodID(env, type, name, signature);
}

jobject ib_jni_new_object_a(void *environment, jclass type,
                            jmethodID constructor, const jvalue *arguments) {
    JNIEnv *env = (JNIEnv *) environment;
    return (*env)->NewObjectA(env, type, constructor, arguments);
}

void ib_jni_call_void_method_a(void *environment, jobject object,
                               jmethodID method, const jvalue *arguments) {
    JNIEnv *env = (JNIEnv *) environment;
    (*env)->CallVoidMethodA(env, object, method, arguments);
}

jobject ib_jni_call_object_method_a(void *environment, jobject object,
                                    jmethodID method, const jvalue *arguments) {
    JNIEnv *env = (JNIEnv *) environment;
    return (*env)->CallObjectMethodA(env, object, method, arguments);
}

jint ib_jni_call_int_method_a(void *environment, jobject object,
                              jmethodID method, const jvalue *arguments) {
    JNIEnv *env = (JNIEnv *) environment;
    return (*env)->CallIntMethodA(env, object, method, arguments);
}

jboolean ib_jni_call_boolean_method_a(void *environment, jobject object,
                                      jmethodID method, const jvalue *arguments) {
    JNIEnv *env = (JNIEnv *) environment;
    return (*env)->CallBooleanMethodA(env, object, method, arguments);
}

jstring ib_jni_new_string(void *environment, const uint16_t *characters,
                          jint length) {
    JNIEnv *env = (JNIEnv *) environment;
    return (*env)->NewString(env, (const jchar *) characters, length);
}

jsize ib_jni_get_string_length(void *environment, jstring string) {
    JNIEnv *env = (JNIEnv *) environment;
    return (*env)->GetStringLength(env, string);
}

const uint16_t *ib_jni_get_string_chars(void *environment, jstring string) {
    JNIEnv *env = (JNIEnv *) environment;
    return (const uint16_t *) (*env)->GetStringChars(env, string, NULL);
}

void ib_jni_release_string_chars(void *environment, jstring string,
                                 const uint16_t *characters) {
    JNIEnv *env = (JNIEnv *) environment;
    (*env)->ReleaseStringChars(env, string, (const jchar *) characters);
}

jobject ib_jni_new_global_ref(void *environment, jobject object) {
    JNIEnv *env = (JNIEnv *) environment;
    return (*env)->NewGlobalRef(env, object);
}

void ib_jni_delete_global_ref(void *environment, jobject object) {
    JNIEnv *env = (JNIEnv *) environment;
    (*env)->DeleteGlobalRef(env, object);
}

void ib_jni_delete_local_ref(void *environment, jobject object) {
    JNIEnv *env = (JNIEnv *) environment;
    (*env)->DeleteLocalRef(env, object);
}

jboolean ib_jni_exception_check(void *environment) {
    JNIEnv *env = (JNIEnv *) environment;
    return (*env)->ExceptionCheck(env);
}

void ib_jni_exception_describe_clear(void *environment) {
    JNIEnv *env = (JNIEnv *) environment;
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
    }
}

#include "clipboard.h"

#include "../scan/scan_result.h"
#include <stdlib.h>

static JNIEnv *attach(ANativeActivity *activity, bool *attached) {
    JNIEnv *env = 0; *attached = false;
    if ((*activity->vm)->GetEnv(activity->vm, (void **)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if ((*activity->vm)->AttachCurrentThread(activity->vm, &env, 0) != JNI_OK) return 0;
        *attached = true;
    }
    return env;
}

static void detach(ANativeActivity *activity, bool attached) { if (attached) (*activity->vm)->DetachCurrentThread(activity->vm); }

static jstring utf8_to_string(JNIEnv *env, const uint8_t *data, int length) {
    jchar *utf16 = calloc((size_t)length * 2 + 1, sizeof(*utf16));
    if (!utf16) return 0;
    int output = 0;
    for (int i = 0; i < length;) {
        uint32_t codepoint = data[i++];
        if (codepoint >= 0x80) {
            int extra = codepoint >= 0xf0 ? 3 : codepoint >= 0xe0 ? 2 : 1;
            codepoint &= (1u << (7 - extra)) - 1u;
            for (int j = 0; j < extra; ++j) codepoint = (codepoint << 6) | (data[i++] & 0x3f);
        }
        if (codepoint <= 0xffff) utf16[output++] = (jchar)codepoint;
        else { codepoint -= 0x10000; utf16[output++] = (jchar)(0xd800 | (codepoint >> 10)); utf16[output++] = (jchar)(0xdc00 | (codepoint & 0x3ff)); }
    }
    jstring result = (*env)->NewString(env, utf16, output);
    free(utf16);
    return result;
}

bool tinyqr_copy_to_clipboard(ANativeActivity *activity, const uint8_t *data, int length) {
    if (!activity || !tinyqr_result_is_utf8_text(data, length)) return false;
    bool attached; JNIEnv *env = attach(activity, &attached); if (!env) return false;
    bool copied = false;
    jclass activity_class = (*env)->GetObjectClass(env, activity->clazz);
    jmethodID service = activity_class ? (*env)->GetMethodID(env, activity_class, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;") : 0;
    jstring name = service ? (*env)->NewStringUTF(env, "clipboard") : 0;
    jobject manager = name ? (*env)->CallObjectMethod(env, activity->clazz, service, name) : 0;
    jclass clip_class = manager ? (*env)->FindClass(env, "android/content/ClipData") : 0;
    jmethodID plain_text = clip_class ? (*env)->GetStaticMethodID(env, clip_class, "newPlainText", "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Landroid/content/ClipData;") : 0;
    jstring label = plain_text ? (*env)->NewStringUTF(env, "QR") : 0;
    jstring text = label ? utf8_to_string(env, data, length) : 0;
    jobject clip = text ? (*env)->CallStaticObjectMethod(env, clip_class, plain_text, label, text) : 0;
    jclass manager_class = clip ? (*env)->GetObjectClass(env, manager) : 0;
    jmethodID set_clip = manager_class ? (*env)->GetMethodID(env, manager_class, "setPrimaryClip", "(Landroid/content/ClipData;)V") : 0;
    if (set_clip) { (*env)->CallVoidMethod(env, manager, set_clip, clip); copied = !(*env)->ExceptionCheck(env); }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    if (manager_class) (*env)->DeleteLocalRef(env, manager_class);
    if (clip) (*env)->DeleteLocalRef(env, clip);
    if (text) (*env)->DeleteLocalRef(env, text);
    if (label) (*env)->DeleteLocalRef(env, label);
    if (clip_class) (*env)->DeleteLocalRef(env, clip_class);
    if (manager) (*env)->DeleteLocalRef(env, manager);
    if (name) (*env)->DeleteLocalRef(env, name);
    if (activity_class) (*env)->DeleteLocalRef(env, activity_class);
    detach(activity, attached);
    return copied;
}

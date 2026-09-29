#include "permission.h"

static JNIEnv *attach(ANativeActivity *activity, bool *attached) {
    JNIEnv *env = 0; *attached = false;
    if ((*activity->vm)->GetEnv(activity->vm, (void **)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if ((*activity->vm)->AttachCurrentThread(activity->vm, &env, 0) != JNI_OK) return 0;
        *attached = true;
    }
    return env;
}
static void detach(ANativeActivity *activity, bool attached) { if (attached) (*activity->vm)->DetachCurrentThread(activity->vm); }
static jstring permission_name(JNIEnv *env) { return (*env)->NewStringUTF(env, "android.permission.CAMERA"); }

bool tinyqr_camera_permission_granted(ANativeActivity *activity) {
    bool attached; JNIEnv *env = attach(activity, &attached); if (!env) return false;
    jclass activity_class = (*env)->GetObjectClass(env, activity->clazz);
    jmethodID check = (*env)->GetMethodID(env, activity_class, "checkSelfPermission", "(Ljava/lang/String;)I");
    jstring permission = permission_name(env);
    jint result = (*env)->CallIntMethod(env, activity->clazz, check, permission);
    (*env)->DeleteLocalRef(env, permission);
    (*env)->DeleteLocalRef(env, activity_class);
    detach(activity, attached); return result == 0;
}

void tinyqr_request_camera_permission(ANativeActivity *activity) {
    bool attached; JNIEnv *env = attach(activity, &attached); if (!env) return;
    jclass activity_class = (*env)->GetObjectClass(env, activity->clazz);
    jmethodID request = (*env)->GetMethodID(env, activity_class, "requestPermissions", "([Ljava/lang/String;I)V");
    jclass string_class = (*env)->FindClass(env, "java/lang/String");
    jobjectArray permissions = (*env)->NewObjectArray(env, 1, string_class, 0);
    jstring permission = permission_name(env);
    (*env)->SetObjectArrayElement(env, permissions, 0, permission);
    (*env)->CallVoidMethod(env, activity->clazz, request, permissions, 1);
    (*env)->DeleteLocalRef(env, permission);
    (*env)->DeleteLocalRef(env, permissions);
    (*env)->DeleteLocalRef(env, string_class);
    (*env)->DeleteLocalRef(env, activity_class);
    detach(activity, attached);
}

void tinyqr_open_app_settings(ANativeActivity *activity) {
    bool attached; JNIEnv *env = attach(activity, &attached); if (!env) return;
    jclass activity_class = (*env)->GetObjectClass(env, activity->clazz);
    jmethodID get_package = (*env)->GetMethodID(env, activity_class, "getPackageName", "()Ljava/lang/String;");
    jstring package_name = (*env)->CallObjectMethod(env, activity->clazz, get_package);
    jstring prefix = (*env)->NewStringUTF(env, "package:");
    jclass string_class = (*env)->FindClass(env, "java/lang/String");
    jmethodID concat = (*env)->GetMethodID(env, string_class, "concat", "(Ljava/lang/String;)Ljava/lang/String;");
    jstring package_uri = (*env)->CallObjectMethod(env, prefix, concat, package_name);
    jclass uri_class = (*env)->FindClass(env, "android/net/Uri");
    jmethodID parse = (*env)->GetStaticMethodID(env, uri_class, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
    jobject uri = (*env)->CallStaticObjectMethod(env, uri_class, parse, package_uri);
    jclass intent_class = (*env)->FindClass(env, "android/content/Intent");
    jmethodID init = (*env)->GetMethodID(env, intent_class, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");
    jstring action = (*env)->NewStringUTF(env, "android.settings.APPLICATION_DETAILS_SETTINGS");
    jobject intent = (*env)->NewObject(env, intent_class, init, action, uri);
    jmethodID start = (*env)->GetMethodID(env, activity_class, "startActivity", "(Landroid/content/Intent;)V");
    (*env)->CallVoidMethod(env, activity->clazz, start, intent);
    (*env)->DeleteLocalRef(env, intent);
    (*env)->DeleteLocalRef(env, action);
    (*env)->DeleteLocalRef(env, intent_class);
    (*env)->DeleteLocalRef(env, uri);
    (*env)->DeleteLocalRef(env, uri_class);
    (*env)->DeleteLocalRef(env, package_uri);
    (*env)->DeleteLocalRef(env, string_class);
    (*env)->DeleteLocalRef(env, prefix);
    (*env)->DeleteLocalRef(env, package_name);
    (*env)->DeleteLocalRef(env, activity_class);
    detach(activity, attached);
}

#include <jni.h>
#include <android/log.h>
#include <string>
#include <atomic>
#include <pthread.h>
#include <EGL/egl.h>

#include "mpv/client.h"
#include "mpv/render.h"
#include "mpv/render_gl.h"

// Mpv player and openGL handles
static mpv_handle *mpv;
static mpv_render_context *mpv_gl;

// Mpv queue events thread
static std::atomic<bool> queue_events_thread_exit = false;
static pthread_t queue_events_thread_id;

// Current surface dimensions
static int surface_width;
static int surface_height;

// Mapping to integrate mpv logging in android logcat
static std::unordered_map<int, int> mpv_log_mapping = {
        {MPV_LOG_LEVEL_FATAL, ANDROID_LOG_FATAL},
        {MPV_LOG_LEVEL_ERROR, ANDROID_LOG_ERROR},
        {MPV_LOG_LEVEL_WARN, ANDROID_LOG_WARN},
        {MPV_LOG_LEVEL_INFO, ANDROID_LOG_INFO},
        {MPV_LOG_LEVEL_V, ANDROID_LOG_VERBOSE},
        {MPV_LOG_LEVEL_DEBUG, ANDROID_LOG_DEBUG},
};

// JavaVM
static JavaVM *javaVM;
static jclass mpvPlayerReference_class;
static jmethodID mpvPlayerReference_onIntPropertyChange;
static jmethodID mpvPlayerReference_onStringPropertyChange;
static jmethodID mpvPlayerReference_onBoolPropertyChange;
static jmethodID mpvPlayerReference_onDoublePropertyChange;

// Callback for MPV to retrieve OpenGL function addresses
void *get_proc_address(void *ctx, const char *name)
{
    return reinterpret_cast<void *>(eglGetProcAddress(name));
}

bool acquire_jni_env(JavaVM *vm, JNIEnv **env)
{
    int ret = vm->GetEnv((void**) env, JNI_VERSION_1_6);
    if (ret == JNI_EDETACHED)
        return vm->AttachCurrentThread(env, nullptr) == 0;
    else
        return ret == JNI_OK;
}

static void sendPropertyUpdateToJava(JNIEnv *env, mpv_event_property *prop) {
    jstring jprop = env->NewStringUTF(prop->name);
    jstring jvalue = nullptr;

    switch (prop->format) {
        case MPV_FORMAT_NONE:
            // TODO
            break;
        case MPV_FORMAT_FLAG:
            env->CallStaticVoidMethod(mpvPlayerReference_class, mpvPlayerReference_onBoolPropertyChange, jprop, (jboolean) (*(int*)prop->data != 0));
            break;
        case MPV_FORMAT_INT64:
            env->CallStaticVoidMethod(mpvPlayerReference_class, mpvPlayerReference_onIntPropertyChange, jprop, (jlong) *(int64_t*)prop->data);
            break;
        case MPV_FORMAT_DOUBLE:
            env->CallStaticVoidMethod(mpvPlayerReference_class, mpvPlayerReference_onDoublePropertyChange, jprop, (jdouble) *(double*)prop->data);
            break;
        case MPV_FORMAT_STRING:
            jvalue = env->NewStringUTF(*(const char**)prop->data);
            env->CallStaticVoidMethod(mpvPlayerReference_class, mpvPlayerReference_onStringPropertyChange, jprop, jvalue);
            break;
        default:
//            ALOGV("sendPropertyUpdateToJava: Unknown property update format received in callback: %d!", prop->format);
            break;
    }

    if (jprop) {
        env->DeleteLocalRef(jprop);
    }
    if (jvalue) {
        env->DeleteLocalRef(jvalue);
    }
}

void *mpv_queue_handler(void *arg)
{
    JNIEnv *env = nullptr;
    acquire_jni_env(javaVM, &env);

    while (true) {
        mpv_event *mp_event;
        mpv_event_property *mp_property = nullptr;
        mpv_event_log_message *msg = nullptr;

        mp_event = mpv_wait_event(mpv, -1.0);

        if (queue_events_thread_exit)
            break;

        if (mp_event->event_id == MPV_EVENT_NONE)
            continue;

        switch (mp_event->event_id) {
            case MPV_EVENT_LOG_MESSAGE:
                msg = (mpv_event_log_message*)mp_event->data;
                __android_log_print(mpv_log_mapping[msg->log_level], "MPV", "%s", msg->text);
                break;
            case MPV_EVENT_PROPERTY_CHANGE:
                mp_property = (mpv_event_property*)mp_event->data;
                sendPropertyUpdateToJava(env, mp_property);
                break;
            default:
//                ALOGV("event: %s\n", mpv_event_name(mp_event->event_id));
//                sendEventToJava(env, mp_event->event_id);
                break;
        }
    }

    return nullptr;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_diegof_mpv_1android_1lib_MpvView_00024MpvRenderer_surfaceCreated(JNIEnv *env, jobject thiz)
{
    // MPV creation and initialization with some basic options
    mpv = mpv_create();
    if (!mpv) {
        return false;
    }

    mpv_request_log_messages(mpv, "v");
    mpv_set_option_string(mpv, "vo", "libmpv");
    mpv_set_option_string(mpv, "profile", "fast");
    mpv_set_option_string(mpv, "gpu-context", "android");
    mpv_set_option_string(mpv, "opengl-es", "yes");
    mpv_set_option_string(mpv, "sub-fonts-dir", "/data/data/com.example.mpv_android_player/files/fonts"); // TODO

    if (mpv_initialize(mpv) < 0) {
        return false;
    }

    // Binding of the openGL context created by GLSurfaceView
    mpv_opengl_init_params gl_init_params[1] = {get_proc_address, nullptr};
    mpv_render_param render_params[] = {
            {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
            {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
            {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    if (mpv_render_context_create(&mpv_gl, mpv, render_params) < 0) {
        return false;
    }

    // Save reference to the kotlin class and methods
    env->GetJavaVM(&javaVM);
    mpvPlayerReference_class = reinterpret_cast<jclass>(env->NewGlobalRef(env->FindClass("com/diegof/mpv_android_lib/MpvPlayer")));
    mpvPlayerReference_onIntPropertyChange = env->GetStaticMethodID(mpvPlayerReference_class, "onIntPropertyChange", "(Ljava/lang/String;J)V");
    mpvPlayerReference_onStringPropertyChange = env->GetStaticMethodID(mpvPlayerReference_class, "onStringPropertyChange", "(Ljava/lang/String;Ljava/lang/String;)V");
    mpvPlayerReference_onBoolPropertyChange = env->GetStaticMethodID(mpvPlayerReference_class, "onBoolPropertyChange", "(Ljava/lang/String;Z)V");
    mpvPlayerReference_onDoublePropertyChange = env->GetStaticMethodID(mpvPlayerReference_class, "onDoublePropertyChange", "(Ljava/lang/String;D)V");

    // Start the thread that handle mpv events queue
    pthread_create(&queue_events_thread_id, nullptr, mpv_queue_handler, nullptr);

    return true;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_diegof_mpv_1android_1lib_MpvView_00024MpvRenderer_drawFrame(JNIEnv *env, jobject thiz)
{
    mpv_opengl_fbo gl_fbo{0, surface_width, surface_height};
    int flip_y{1};

    mpv_render_param render_params[] = {
            {MPV_RENDER_PARAM_OPENGL_FBO, &gl_fbo},
            {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
            {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    mpv_render_context_render(mpv_gl, render_params);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_diegof_mpv_1android_1lib_MpvView_00024MpvRenderer_surfaceChanged(JNIEnv *env, jobject thiz, jint new_surface_width, jint new_surface_height)
{
    surface_width = new_surface_width;
    surface_height = new_surface_height;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_diegof_mpv_1android_1lib_MpvPlayer_00024Companion_destroy(JNIEnv *env, jobject thiz)
{
    // Tell and wait for events queue thread to exit
    queue_events_thread_exit = true;
    mpv_wakeup(mpv);
    pthread_join(queue_events_thread_id, NULL);

    if (mpv_gl) {
        mpv_render_context_free(mpv_gl);
    }

    if (mpv) {
        mpv_terminate_destroy(mpv);
    }

    mpv_gl = nullptr;
    mpv = nullptr;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_diegof_mpv_1android_1lib_MpvPlayer_00024Companion_command(JNIEnv *env, jobject thiz, jobjectArray arguments)
{
    jsize args_count = env->GetArrayLength(arguments);
    std::vector<const char*> args;

    for (jsize i = 0; i < args_count; i++) {
        auto string_arg = (jstring) env->GetObjectArrayElement(arguments, i);
        const char *arg = env->GetStringUTFChars(string_arg, nullptr);

        args.push_back(arg);

        env->ReleaseStringUTFChars(string_arg, arg);
        env->DeleteLocalRef(string_arg);
    }
    args.push_back(nullptr);

    return mpv_command(mpv, args.data());
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_diegof_mpv_1android_1lib_MpvPlayer_00024Companion_option(JNIEnv *env, jobject thiz, jstring option, jstring value) {
    const char *option_name = env->GetStringUTFChars(option, nullptr);
    const char *option_value = env->GetStringUTFChars(value, nullptr);

    env->ReleaseStringUTFChars(option, option_name);
    env->ReleaseStringUTFChars(value, option_value);

    return mpv_set_option_string(mpv, option_name, option_value);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_diegof_mpv_1android_1lib_MpvPlayer_00024Companion_observeProperty(JNIEnv *env, jobject thiz, jstring property, jint type) {
    const char *property_name = env->GetStringUTFChars(property, nullptr);
    env->ReleaseStringUTFChars(property, property_name);

    return mpv_observe_property(mpv, 0, property_name, (mpv_format) type);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_diegof_mpv_1android_1lib_MpvPlayer_00024Companion_setIntProperty(JNIEnv *env, jobject thiz, jstring property, jint value) {
    const char *property_name = env->GetStringUTFChars(property, nullptr);
    env->ReleaseStringUTFChars(property, property_name);

    return mpv_set_property(mpv, property_name, MPV_FORMAT_INT64, &value);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_diegof_mpv_1android_1lib_MpvPlayer_00024Companion_setStringProperty(JNIEnv *env, jobject thiz, jstring property, jstring value) {
    const char *property_name = env->GetStringUTFChars(property, nullptr);
    const char *property_value = env->GetStringUTFChars(value, nullptr);
    env->ReleaseStringUTFChars(property, property_name);
    env->ReleaseStringUTFChars(value, property_value);

    return mpv_set_property(mpv, property_name, MPV_FORMAT_STRING, &property_value);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_diegof_mpv_1android_1lib_MpvPlayer_00024Companion_setBoolProperty(JNIEnv *env, jobject thiz, jstring property, jboolean value) {
    const char *property_name = env->GetStringUTFChars(property, nullptr);
    env->ReleaseStringUTFChars(property, property_name);

    return mpv_set_property(mpv, property_name, MPV_FORMAT_FLAG, &value);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_diegof_mpv_1android_1lib_MpvPlayer_00024Companion_setDoubleProperty(JNIEnv *env, jobject thiz, jstring property, jdouble value) {
    const char *property_name = env->GetStringUTFChars(property, nullptr);
    env->ReleaseStringUTFChars(property, property_name);

    return mpv_set_property(mpv, property_name, MPV_FORMAT_DOUBLE, &value);
}

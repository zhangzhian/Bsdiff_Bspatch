#include <jni.h>
#include <string>
//#include <android/log.h>
extern "C"
{
#include "bsdiff.h"
#include "bspatch.h"
}



extern "C" JNIEXPORT jstring
JNICALL
Java_com_yodosmart_bsdiffbspatch_BsdiffBspatchActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_yodosmart_bsdiffbspatch_DiffPatchUtil_diff(JNIEnv *env, jclass type, jstring oldPath_,
                                                    jstring newPath_, jstring patchPath_) {

    const char *oldPath = env->GetStringUTFChars(oldPath_, 0);
    const char *newPath = env->GetStringUTFChars(newPath_, 0);
    const char *patchPath = env->GetStringUTFChars(patchPath_, 0);

    int argc = 4;
     char * argv[argc];
    argv[0] = (char *)"bsdiff";
    argv[1] = (char *)oldPath;
    argv[2] =(char *)newPath;
    argv[3] = (char *)patchPath;

//    __android_log_print(ANDROID_LOG_INFO, "zza", "start = %s ", argv[0]);
//    __android_log_print(ANDROID_LOG_INFO, "zza", "old = %s ", argv[1]);
//    __android_log_print(ANDROID_LOG_INFO, "zza", "new = %s ", argv[2]);
//    __android_log_print(ANDROID_LOG_INFO, "zza", "patch = %s ", argv[3]);

    int ret = bsdiff_main(argc,argv);

    env->ReleaseStringUTFChars(oldPath_, oldPath);
    env->ReleaseStringUTFChars(newPath_, newPath);
    env->ReleaseStringUTFChars(patchPath_, patchPath);

    return ret;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_yodosmart_bsdiffbspatch_DiffPatchUtil_patch(JNIEnv *env, jclass type, jstring oldPath_,
                                                     jstring newPath_, jstring patchPath_) {
    const char *oldPath = env->GetStringUTFChars(oldPath_, 0);
    const char *newPath = env->GetStringUTFChars(newPath_, 0);
    const char *patchPath = env->GetStringUTFChars(patchPath_, 0);

    // TODO
    int argc = 4;
    char * argv[argc];
    argv[0] = (char *)"bspatch";
    argv[1] = (char *)oldPath;
    argv[2] = (char *)newPath;
    argv[3] = (char *)patchPath;


    int ret = bspatch_main(argc,argv);

    env->ReleaseStringUTFChars(oldPath_, oldPath);
    env->ReleaseStringUTFChars(newPath_, newPath);
    env->ReleaseStringUTFChars(patchPath_, patchPath);

    return ret;

}

#include <jni.h>
#include <string>
#include <android/log.h>
#define TAG "zza-jni" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__) // 定义LOGF类型
extern "C"
{
#include "bsdiff.h"
#include "bspatch.h"
#include "bsdiff_str.h"    
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
    argv[2] = (char *)newPath;
    argv[3] = (char *)patchPath;

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

extern "C"
JNIEXPORT jobject JNICALL
Java_com_yodosmart_bsdiffbspatch_DiffPatchUtil_diffStr(JNIEnv *env, jclass type, jstring oldStr_,
                                                       jstring newStr_, jint oldSzie,
                                                       jint newSize) {
    const char *oldStr = env->GetStringUTFChars(oldStr_, 0);
    const char *newStr = env->GetStringUTFChars(newStr_, 0);
    struct  bsdiff_result result_new[100] = {};
    struct  bsdiff_result result_old[100] = {};
    int result = bsdiff_str((char *)oldStr,(char *)newStr,oldSzie,newSize, result_new, result_old);


    jclass list = env->FindClass("java/util/ArrayList");
    jmethodID list_costruct = env->GetMethodID(list, "<init>", "()V"); //获得得构造函数Id
    jobject list_obj = env->NewObject(list, list_costruct, ""); //创建一个Arraylist集合对象

    //获得Arraylist类中的 add()方法ID，其方法原型为： boolean add(Object object) ;
    jmethodID list_add = env->GetMethodID(list, "add", "(Ljava/lang/Object;)Z");

    jbyte buff[100] = {};
    jbyte extra[100] = {};

    for (int i = 0; i < result; ++i) {

        jclass jcBsdiffBean = env->FindClass("com/yodosmart/bsdiffbspatch/BsdiffBean");
        jfieldID jfBuff = env->GetFieldID(jcBsdiffBean,"buff","[B");
        jfieldID jfExtra = env->GetFieldID(jcBsdiffBean,"extra","[B");
        jfieldID jftype = env->GetFieldID(jcBsdiffBean,"type","Z");
        jobject  joBsdiffBean = env->AllocObject(jcBsdiffBean);

        jbyteArray byteBuff = env->NewByteArray(result_new[i].buffsize);
        jbyteArray byteExtra = env->NewByteArray(result_new[i].extrasize);

        for (int j = 0; j < result_new[i].buffsize; ++j) {
            buff[j] = result_new[i].buffer[j];
        }
        for (int j = 0; j < result_new[i].extrasize; ++j) {
            extra[j] = result_new[i].extra[j];
        }

        env->SetByteArrayRegion(byteBuff,0,result_new[i].buffsize,buff);
        env->SetByteArrayRegion(byteExtra,0,result_new[i].extrasize,extra);

        env->SetObjectField(joBsdiffBean,jfBuff,byteBuff);
        env->SetObjectField(joBsdiffBean,jfExtra,byteExtra);
        env->SetBooleanField(joBsdiffBean,jftype, true);

        env->CallBooleanMethod(list_obj,list_add,joBsdiffBean);
    }

    for (int i = 0; i < result; ++i) {

        jclass jcBsdiffBean = env->FindClass("com/yodosmart/bsdiffbspatch/BsdiffBean");
        jfieldID jfBuff = env->GetFieldID(jcBsdiffBean,"buff","[B");
        jfieldID jfExtra = env->GetFieldID(jcBsdiffBean,"extra","[B");
        jfieldID jftype = env->GetFieldID(jcBsdiffBean,"type","Z");
        jobject  joBsdiffBean = env->AllocObject(jcBsdiffBean);

        jbyteArray byteBuff = env->NewByteArray(result_old[i].buffsize);
        jbyteArray byteExtra = env->NewByteArray(result_old[i].extrasize);

        for (int j = 0; j < result_old[i].buffsize; ++j) {
            buff[j] = result_old[i].buffer[j];
        }
        for (int j = 0; j < result_old[i].extrasize; ++j) {
            extra[j] = result_old[i].extra[j];
        }

        env->SetByteArrayRegion(byteBuff,0,result_old[i].buffsize,buff);
        env->SetByteArrayRegion(byteExtra,0,result_old[i].extrasize,extra);

        env->SetObjectField(joBsdiffBean,jfBuff,byteBuff);
        env->SetObjectField(joBsdiffBean,jfExtra,byteExtra);
        env->SetBooleanField(joBsdiffBean,jftype, false);

        env->CallBooleanMethod(list_obj,list_add,joBsdiffBean);
    }

    env->ReleaseStringUTFChars(oldStr_, oldStr);
    env->ReleaseStringUTFChars(newStr_, newStr);

    return list_obj;
}

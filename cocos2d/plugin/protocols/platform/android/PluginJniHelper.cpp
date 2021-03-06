/****************************************************************************
Copyright (c) 2012-2013 cocos2d-x.org

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/
#include "PluginJniHelper.h"
#include <android/log.h>
#include <string.h>

#if 1
#define  LOG_TAG    "PluginJniHelper"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#else
#define  LOGD(...) 
#endif

#define JAVAVM    cocos2d::PluginJniHelper::getJavaVM()

using namespace std;

extern "C"
{
    static bool _getEnv(JNIEnv **env)
    {

        bool bRet = false;

        do 
        {
            if(!JAVAVM)
            {
                LOGD("JavaVM is NULL");
                break;
            }

            if (JAVAVM->GetEnv((void**)env, JNI_VERSION_1_4) != JNI_OK)
            {
                LOGD("Failed to get the environment using GetEnv()");
                break;
            }

            if (JAVAVM->AttachCurrentThread(env, 0) < 0)
            {
                LOGD("Failed to get the environment using AttachCurrentThread()");
                break;
            }
            bRet = true;
        } while (0);

        return bRet;
    }

    //////////////////////////////////////////////////////////////////////////
    // java vm helper function
    //////////////////////////////////////////////////////////////////////////

    static jclass _getClassIDByLoader(const char *className) {
        if (NULL == className) {
            return NULL;
        }

        JNIEnv* env = cocos2d::PluginJniHelper::getEnv();

        jstring _jstrClassName = env->NewStringUTF(className);

        jclass _clazz = (jclass) env->CallObjectMethod(cocos2d::PluginJniHelper::classloader,
                                                       cocos2d::PluginJniHelper::loadclassMethod_methodID,
                                                       _jstrClassName);

        if (NULL == _clazz) {
            LOGD("Classloader failed to find class of %s", className);
        }

        env->DeleteLocalRef(_jstrClassName);

        return _clazz;
    }

    static jclass _getClassID(const char *className, JNIEnv *env)
    {
        JNIEnv *pEnv = env;
        jclass ret = 0;

        do 
        {
            if (cocos2d::PluginJniHelper::classloader)
            {
                ret = _getClassIDByLoader(className);
                break;
            }

            if (! pEnv)
            {
                pEnv = cocos2d::PluginJniHelper::getEnv();
                if (! pEnv)
                {
                    break;
                }
            }

            ret = pEnv->FindClass(className);
            if (! ret)
            {
                 LOGD("Failed to find class of %s", className);
                break;
            }
        } while (0);

        return ret;
    }
}

namespace cocos2d {

JavaVM* PluginJniHelper::_psJavaVM = NULL;
jmethodID PluginJniHelper::loadclassMethod_methodID = NULL;
jobject PluginJniHelper::classloader = NULL;

JavaVM* PluginJniHelper::getJavaVM()
{
    return _psJavaVM;
}

void PluginJniHelper::setJavaVM(JavaVM *javaVM)
{
    _psJavaVM = javaVM;
}

JNIEnv* PluginJniHelper::getEnv()
{
    JNIEnv* ret = NULL;
    bool bRet = _getEnv(&ret);

    if (! bRet)
        ret = NULL;

    return ret;
}

bool PluginJniHelper::getStaticMethodInfo(PluginJniMethodInfo &methodinfo, const char *className, const char *methodName, const char *paramCode)
{
    if ((NULL == className) ||
        (NULL == methodName) ||
        (NULL == paramCode)) {
        return false;
    }

    JNIEnv *pEnv = PluginJniHelper::getEnv();
    if (!pEnv) {
        LOGD("Failed to get JNIEnv");
        return false;
    }

    jclass classID = _getClassID(className, pEnv);
    if (! classID) {
        LOGD("Failed to find class %s", className);
        return false;
    }

    jmethodID methodID = pEnv->GetStaticMethodID(classID, methodName, paramCode);
    if (! methodID) {
        if(pEnv->ExceptionCheck())
        {
            pEnv->ExceptionClear();
        }
        LOGD("Failed to find static method id of %s", methodName);
        return false;
    }

    methodinfo.classID = classID;
    methodinfo.env = pEnv;
    methodinfo.methodID = methodID;
    return true;
}

bool PluginJniHelper::getMethodInfo(PluginJniMethodInfo &methodinfo, const char *className, const char *methodName, const char *paramCode)
{
    if ((NULL == className) ||
        (NULL == methodName) ||
        (NULL == paramCode)) {
        return false;
    }

    JNIEnv *pEnv = PluginJniHelper::getEnv();
    if (!pEnv) {
        return false;
    }

    jclass classID = _getClassID(className, pEnv);
    if (! classID) {
        LOGD("Failed to find class %s", className);
        return false;
    }

    jmethodID methodID = pEnv->GetMethodID(classID, methodName, paramCode);
    if (! methodID) {
        if(pEnv->ExceptionCheck())
        {
            pEnv->ExceptionClear();
        }
        LOGD("Failed to find method id of %s", methodName);
        return false;
    }

    methodinfo.classID = classID;
    methodinfo.env = pEnv;
    methodinfo.methodID = methodID;

    return true;
}

string PluginJniHelper::jstring2string(jstring jstr)
{
    if (jstr == NULL) {
        return "";
    }

    JNIEnv *pEnv = PluginJniHelper::getEnv();
    if (! pEnv) {
        return NULL;
    }

    const char* chars = pEnv->GetStringUTFChars(jstr, NULL);
    std::string ret(chars);
    pEnv->ReleaseStringUTFChars(jstr, chars);

    return ret;
}

bool PluginJniHelper::setClassLoaderFrom(jobject nativeactivityinstance) {
    PluginJniMethodInfo _getclassloaderMethod;
    if (!PluginJniHelper::getMethodInfo_DefaultClassLoader(_getclassloaderMethod,
                                                     "android/app/NativeActivity",
                                                     "getClassLoader",
                                                     "()Ljava/lang/ClassLoader;")) {
        return false;
    }

    jobject _c = cocos2d::PluginJniHelper::getEnv()->CallObjectMethod(nativeactivityinstance,
                                                                _getclassloaderMethod.methodID);

    if (NULL == _c) {
        return false;
    }

    PluginJniMethodInfo _m;
    if (!PluginJniHelper::getMethodInfo_DefaultClassLoader(_m,
                                                     "java/lang/ClassLoader",
                                                     "loadClass",
                                                     "(Ljava/lang/String;)Ljava/lang/Class;")) {
        return false;
    }

    PluginJniHelper::classloader = _c;
    PluginJniHelper::loadclassMethod_methodID = _m.methodID;

    return true;
}

bool PluginJniHelper::getMethodInfo_DefaultClassLoader(PluginJniMethodInfo &methodinfo,
                                                     const char *className,
                                                     const char *methodName,
                                                     const char *paramCode)
{
    if ((NULL == className) ||
        (NULL == methodName) ||
        (NULL == paramCode)) {
        return false;
    }

    JNIEnv *pEnv = PluginJniHelper::getEnv();
    if (!pEnv) {
        return false;
    }

    jclass classID = pEnv->FindClass(className);
    if (! classID) {
        LOGD("Failed to find class %s", className);
        return false;
    }

    jmethodID methodID = pEnv->GetMethodID(classID, methodName, paramCode);
    if (! methodID) {
        LOGD("Failed to find method id of %s", methodName);
        return false;
    }

    methodinfo.classID = classID;
    methodinfo.env = pEnv;
    methodinfo.methodID = methodID;

    return true;
}

} // end of namespace cocos2d

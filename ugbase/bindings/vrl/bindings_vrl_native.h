/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class edu_gcsc_vrl_ug_UG */

#ifndef _Included_edu_gcsc_vrl_ug_UG
#define _Included_edu_gcsc_vrl_ug_UG
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _convertRegistryInfo
 * Signature: ()Ledu/gcsc/vrl/ug/NativeAPIInfo;
 */
JNIEXPORT jobject JNICALL Java_edu_gcsc_vrl_ug_UG__1convertRegistryInfo
  (JNIEnv *, jobject);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _invokeMethod
 * Signature: (Ljava/lang/String;JZLjava/lang/String;[Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_edu_gcsc_vrl_ug_UG__1invokeMethod
  (JNIEnv *, jobject, jstring, jlong, jboolean, jstring, jobjectArray);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _newInstance
 * Signature: (J[Ljava/lang/Object;)Ledu/gcsc/vrl/ug/Pointer;
 */
JNIEXPORT jobject JNICALL Java_edu_gcsc_vrl_ug_UG__1newInstance
  (JNIEnv *, jobject, jlong, jobjectArray);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _getExportedClassPtrByName
 * Signature: (Ljava/lang/String;Z)J
 */
JNIEXPORT jlong JNICALL Java_edu_gcsc_vrl_ug_UG__1getExportedClassPtrByName
  (JNIEnv *, jobject, jstring, jboolean);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _getDefaultClassNameFromGroup
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG__1getDefaultClassNameFromGroup
  (JNIEnv *, jobject, jstring);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _invokeFunction
 * Signature: (Ljava/lang/String;Z[Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_edu_gcsc_vrl_ug_UG__1invokeFunction
  (JNIEnv *, jobject, jstring, jboolean, jobjectArray);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _getSvnRevision
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG__1getSvnRevision
  (JNIEnv *, jobject);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _getUGVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG__1getUGVersion
  (JNIEnv *, jobject);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _getDescription
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG__1getDescription
  (JNIEnv *, jobject);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _getAuthors
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG__1getAuthors
  (JNIEnv *, jobject);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _getCompileDate
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG__1getCompileDate
  (JNIEnv *, jobject);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _getBinaryLicense
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG__1getBinaryLicense
  (JNIEnv *, jobject);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _ugInit
 * Signature: ([Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_edu_gcsc_vrl_ug_UG__1ugInit
  (JNIEnv *, jclass, jobjectArray);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _test_debug
 * Signature: (Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_edu_gcsc_vrl_ug_UG__1test_1debug
  (JNIEnv *, jobject, jstring, jobjectArray);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _delete
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_edu_gcsc_vrl_ug_UG__1delete
  (JNIEnv *, jclass, jlong, jlong);

/*
 * Class:     edu_gcsc_vrl_ug_UG
 * Method:    _invalidate
 * Signature: (Ledu/gcsc/vrl/ug/SmartPointer;)V
 */
JNIEXPORT void JNICALL Java_edu_gcsc_vrl_ug_UG__1invalidate
  (JNIEnv *, jclass, jobject);

#ifdef __cplusplus
}
#endif
#endif
/* Header for class edu_gcsc_vrl_ug_UG_MessageThread */

#ifndef _Included_edu_gcsc_vrl_ug_UG_MessageThread
#define _Included_edu_gcsc_vrl_ug_UG_MessageThread
#ifdef __cplusplus
extern "C" {
#endif
#undef edu_gcsc_vrl_ug_UG_MessageThread_MIN_PRIORITY
#define edu_gcsc_vrl_ug_UG_MessageThread_MIN_PRIORITY 1L
#undef edu_gcsc_vrl_ug_UG_MessageThread_NORM_PRIORITY
#define edu_gcsc_vrl_ug_UG_MessageThread_NORM_PRIORITY 5L
#undef edu_gcsc_vrl_ug_UG_MessageThread_MAX_PRIORITY
#define edu_gcsc_vrl_ug_UG_MessageThread_MAX_PRIORITY 10L
#ifdef __cplusplus
}
#endif
#endif

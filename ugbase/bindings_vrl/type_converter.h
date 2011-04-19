/* 
 * File:   type_converter.h
 * Author: Michael Hoffer <info@michaelhoffer.de>
 *
 * Created on 5. Oktober 2010, 14:54
 */

#ifndef TYPE_CONVERTER_H
#define	TYPE_CONVERTER_H

#include<jni.h>
#include<string>
#include<vector>
#include "ug_bridge/class.h"
#include "ug_bridge/registry.h"
#include "messaging.h"

namespace ug {
namespace vrl {

/**
 * Converts a native string to a Java string.
 * @param env JVM environment to operate on
 * @param s string to convert
 * @return a java string
 */
jstring stringC2J(JNIEnv *env, const char* s);

/**
 * <p>
 * Converts a Java string to a native string.
 * </p>
 * <p>
 * <b>Note:</b> this function must not be used to convert large amounts
 * of strings because of inefficient memory handling.
 * </p>
 * @param env JVM environment to operate on
 * @param s string to convert
 * @return a native string
 */
std::string stringJ2C(JNIEnv *env, jstring const& s);

/**
 * Converts a native string array to a Java object array.
 * @param env JVM environment to operate on
 * @param strings array to convert
 * @return a java object array
 */
jobjectArray stringArrayC2J(JNIEnv *env, const std::string* strings);

/**
 * Converts a native string array to a java object array.
 * @param env JVM environment to operate on
 * @param strings array to convert
 * @return a java object array
 */
jobjectArray stringArrayC2J(
		JNIEnv *env,
		std::vector<std::string> const& strings);

/**
 * <p>
 * Converts a Java string array to a native string array.
 * </p>
 * <p>
 * <b>Note:</b> this function must not be used to convert large amounts
 * of strings because of inefficient memory handling.
 * </p>
 * @param env JVM environment to operate on
 * @param array array to convert
 * @return a native string array
 */
std::vector<std::string> stringArrayJ2C(
		JNIEnv *env, jobjectArray const& array);

/**
 * Converts a Java boolean to a native boolean.
 * @param b Java boolean to convert
 * @return native boolean
 */
inline bool boolJ2C(jboolean b) {
	return b == JNI_TRUE;
}

/**
 * Converts a native boolean to a Java boolean.
 * @param b native boolean to convert
 * @return Java boolean
 */
inline jboolean boolC2J(bool b) {
	return b ? JNI_TRUE : JNI_FALSE;
}

/**
 * Converts a jboolean to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param value value to convert
 * @return Java object (jobject)
 */
jobject boolean2JObject(JNIEnv *env, jboolean value);

/**
 * Converts a Java object to a native boolean value.
 * @param env JVM environment to operate on
 * @param obj object to convert
 * @return a native boolean value
 */
jboolean jObject2Boolean(JNIEnv *env, jobject obj);

/**
 * Converts an integer to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param value value to convert
 * @return Java object (jobject)
 */
jobject int2JObject(JNIEnv *env, jint value);

/**
 * Converts a Java object (jobject) to a native int value.
 * @param env JVM environment to operate on
 * @param obj object to convert
 * @return a native int value
 */
jint jObject2Int(JNIEnv *env, jobject obj);

/**
 * Converts a double to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param value value to convert
 * @return a Java object (jobject)
 */
jobject double2JObject(JNIEnv *env, jdouble value);

/**
 * Converts a Java object (jobject) to a native double value.
 * @param env JVM environment to operate on
 * @param obj object to convert
 * @return a native double value
 */
jdouble jObject2Double(JNIEnv *env, jobject obj);

/**
 * Converts a string to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param value value to convert
 * @return a Java object (jobject)
 */
jobject string2JObject(JNIEnv *env, const char* value);

/**
 * Converts a Java object (jobject) to a native string.
 * @param env JVM environment to operate on
 * @param obj object to convert
 * @return a native string
 */
std::string jObject2String(JNIEnv *env, jobject obj);

/**
 * Converts a native pointer to a Java object (jobject).
 * @param env JVM environment to operate on
 * @param value pointer to convert
 * @return a Java object (jobject)
 */
jobject pointer2JObject(JNIEnv *env, void* value);

/**
 * Indicates whether the specified smart pointer is const, i.e., if the
 * specified Java object represents a const smart pointer.
 * @param env JVM environment to operate on
 * @param ptr smart pointer to check
 * @return <code>true</code> if the specified smart pointer is const;
 *         <code>false</code> otherwise
 */
bool isJSmartPointerConst(JNIEnv *env, jobject ptr);

/**
 * Invalidates the native equivalent of the specified Java smart pointer.
 * TODO what about error handling???
 * @param env JVM environment to operate on
 * @param obj smart pointer to invalidate
 */
void invalidateJSmartPointer(JNIEnv *env, jobject obj);

/**
 * Invalidates the native equivalent of the specified Java smart pointer.
 * TODO what about error handling???
 * @param env JVM environment to operate on
 * @param obj smart pointer to invalidate
 */
void invalidateJConstSmartPointer(JNIEnv *env, jobject obj);

/**
 * Converts a Java object (jobject) to a native pointer.
 * @param env JVM environment to operate on
 * @param obj object to convert
 * @return a native pointer
 */
void* jObject2Pointer(JNIEnv *env, jobject obj);


/**
 * Converts an array of Java objects to a parameter stack.
 * @param env JVM environment to operate on
 * @param paramsOut converted parameter stack (return value)
 * @param paramsTemplate template parameter stack used to get correct
 *                       parameter type
 * @param array object array to convert
 */
void jobjectArray2ParamStack(JNIEnv *env,
		ug::bridge::ParameterStack& paramsOut,
		const ug::bridge::ParameterStack& paramsTemplate,
		jobjectArray const& array);

/**
 * Creates an empty Java array using the specified class as element
 * type.
 * @param env JVM environment to operate on
 * @param className name of the element class
 * @return emtpy Java array
 */
jobjectArray createEmptyJavaArray(
		JNIEnv *env, std::string className);

/**
 * Creates an empty Java array using the specified class as element
 * type.
 * @param env JVM environment to operate on
 * @param elementClass element class
 * @return emtpy Java array
 */
jobjectArray createEmptyJavaArray(
		JNIEnv *env, jclass elementClass);

/**
 * Converts a parameter stack entry to a Java object.
 * @param env JVM environment to operate on
 * @param params parameter stack to convert
 * @param index index of the element to convert
 * @return a Java object (jobject)
 */
jobject param2JObject(JNIEnv *env,
		ug::bridge::ParameterStack& params, int index);

/**
 * Returns the class object of the specified Java object.
 * @param env JVM environment to operate on
 * @param obj Java object
 * @return class object of the specified Java object
 */
jobject getClass(JNIEnv *env, jobject obj);

/**
 * Returns the class name of the specified Java object.
 * @param env JVM environment to operate on
 * @param obj Java object
 * @return class name of the specified Java object
 */
std::string getClassName(JNIEnv *env, jobject obj);

/**
 * Returns the class name of the specified param object (class UGObject).
 * @param env JVM environment to operate on
 * @param obj param object
 * @return class name of the specified param object
 */
std::string getParamClassName(JNIEnv *env, jobject obj);

/**
 * Returns the parameter type (ug::bridge::ParameterTypes) of the
 * specified Java object.
 * @param env JVM environment to operate on
 * @param obj Java object
 * @return parameter type (ug::bridge::ParameterTypes) of the
 *         specified Java object
 */
uint paramClass2ParamType(JNIEnv *env, jobject obj);

/**
 * Compares the parameter types of a Java object array and a
 * paramter stack. It ignores differences regarding constness.
 * This is checked on Java via interface types.
 * @param env JVM environment to operate on
 * @param params array of Java objects
 * @param reg ug registry
 * @param paramStack parameter stack
 * @return <code>true</code> if parameter types are equal;
 *         <code>false</code> otherwise
 */
bool compareParamTypes(JNIEnv *env, jobjectArray params,
        ug::bridge::Registry *reg,
		const ug::bridge::ParameterStack& paramStack);

/**
 * Returns parent classes (super classes) of an exported class.
 * @param reg registry to search
 * @param clazz exported class
 * @return a vector containing all parent classes of the given class
 */
const std::vector<const ug::bridge::IExportedClass*> getParentClasses(
		ug::bridge::Registry* reg,
		const ug::bridge::IExportedClass* clazz);


/**
 * Converts registry information to Java objects.
 * @param env JVM environment to operate on
 * @param reg registry to convert
 * @return native api info
 */
jobject registry2NativeAPI(JNIEnv *env, ug::bridge::Registry* reg);

/**
 * Returns the base classes of the given class name node.
 * @return the base classes of the given class name node
 */
std::vector<const char*> getBaseClassNames(const ug::bridge::ClassNameNode* node);

}// end vrl::
}// end ug::

#endif	/* TYPE_CONVERTER_H */


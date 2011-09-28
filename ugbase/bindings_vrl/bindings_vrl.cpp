#include <string>
#include <vector>
#include <string>

#include "ug.h"
#include "ugbase.h"
#include "registry/registry.h"
#include "registry/class.h"
#include "common/util/path_provider.h"

#include "common/common.h"
#include "lib_algebra/operator/convergence_check.h"
#include "common/authors.h"


#include "type_converter.h"
#include "messaging.h"
#include "canvas.h"
#include "bindings_vrl.h"
#include "bindings_vrl_native.h"

#include "lib_grid/lib_grid.h"
#include "compiledate.h"
#include "user_data.h"
#include "lib_disc/spatial_discretization/ip_data/const_user_data.h"

#include "invocation.h"
#include "playground.h"
#include "threading.h"

#include "ug_script/externals/lua/lstate.h"

namespace ug {
namespace vrl {
static ug::bridge::Registry* vrlRegistry = NULL;
static JavaVM* javaVM = NULL;

void SetVRLRegistry(ug::bridge::Registry* pReg) {
	vrlRegistry = pReg;
}

void initJavaVM(JNIEnv* env) {
	if (javaVM == NULL) {
		env->GetJavaVM(&javaVM);
	} else {
		UG_LOG("UG-VRL: JavaVM already initialized!"
				" JavaVM can be initialized only once!");
	}
}

JavaVM* getJavaVM() {
	return javaVM;
}

void Log(std::string s) {
	UG_LOG(s);
}

void Logln(std::string s) {
	UG_LOG(s << std::endl);
}

void ThrowIf(bool b, std::string s) {
	if (!b) {
		throw(ug::UGError(s.c_str()));
	}
}

void ThrowIfNot(bool b, std::string s) {
	if (!b) {
		throw(ug::UGError(s.c_str()));
	}
}

void registerMessaging(ug::bridge::Registry & reg) {
	reg.add_function("print", &Log, "UG4/Messaging");
	reg.add_function("println", &Logln, "UG4/Messaging");
}

void registerThrowUtil(ug::bridge::Registry & reg) {
	reg.add_function("throwIf", &ThrowIf, "UG4/util");
	reg.add_function("throwIfNot", &ThrowIfNot, "UG4/util");
}

class NumberArray {
private :
	std::vector<number> _vec;
public:
	
	NumberArray() {}
	
	NumberArray(std::vector<number> vec) {
		_vec = vec;
	}
	
	void setArray(std::vector<number> vec) {
		_vec = vec;
	}
	
	int size() const {
		return _vec.size();
	}
	
	number get(size_t i) const {
		if (i < 0 || i >= _vec.size()){
			throw UGFatalError("NumberArray: index out of Bounds!");
		}
		
		return _vec[i];
	}
};



SmartPtr<NumberArray> getDefects(const ug::StandardConvCheck* convCheck) {
	
	return SmartPtr<NumberArray>(
			new NumberArray(convCheck->get_defects()));
}

void registerNumberArray(ug::bridge::Registry & reg) {
	reg.add_class_<NumberArray>("NumberArray","UG4/util")
	.add_constructor()
				.add_method("get", &NumberArray::get)
				.add_method("size", &NumberArray::size);
	reg.add_function("GetDefects", &getDefects, "UG4/util");
}

void registerUGFinalize(ug::bridge::Registry & reg) {
	reg.add_function("UGFinalize", &ug::UGFinalize, "UG4/util");
}

//void registryChanged(ug::bridge::Registry* reg) {
//	UG_LOG("VRL: REGISTRY CHANGED\n");
//}

}// end vrl::
}// end ug::


//*********************************************************
//* JNI METHODS
//*********************************************************

JNIEXPORT jint JNICALL Java_edu_gcsc_vrl_ug_UG_ugInit
(JNIEnv *env, jobject obj, jobjectArray args) {

	ug::vrl::initJavaVM(env);

	std::vector<std::string> arguments = ug::vrl::stringArrayJ2C(env, args);

	std::vector<char*> argv(arguments.size());
	for (unsigned int i = 0; i < arguments.size(); i++) {
		argv[i] = (char*) arguments[i].c_str();
	}

	// Choose registry used.
	ug::bridge::Registry& reg = ug::bridge::GetUGRegistry();

//	reg.add_callback(&ug::vrl::registryChanged);
	
	using namespace ug;

	// define paths
	ug::PathProvider::set_path(PLUGIN_PATH,arguments[0]);
	
	int argc = arguments.size();
	char** pargv = &argv[0];
	//\todo: generalize outputproc rank
	// isn't this possible already via SetOuputRank() ?
	int retVal = ug::UGInit(&argc, &pargv, 0);

	// Register Playground if we are in debug mode
	
	//#ifdef UG_DEBUG
	//	registerPlayground(reg);
	//#endif

	ug::vrl::RegisterUserData(reg, "UG4/VRL");
	ug::vrl::registerMessaging(reg);
	ug::vrl::registerThrowUtil(reg);
	ug::vrl::registerNumberArray(reg);
	ug::vrl::registerUGFinalize(reg);

	if (!reg.check_consistency()) {
		UG_LOG("UG-VRL: cannot compile code due to registration error.\n");
		return 1;
	}

	ug::vrl::SetVRLRegistry(&reg);

	ug::vrl::invocation::initClasses(*ug::vrl::vrlRegistry);

	return (jint) retVal;
}

JNIEXPORT jobject JNICALL Java_edu_gcsc_vrl_ug_UG_invokeMethod
(JNIEnv *env, jobject obj,
		jstring exportedClassName, jlong objPtr, jboolean readOnly,
		jstring methodName, jobjectArray params) {

	std::string className = ug::vrl::stringJ2C(env, exportedClassName);

	const ug::bridge::IExportedClass* clazz =
			ug::vrl::invocation::getExportedClassPtrByName(
			ug::vrl::vrlRegistry, className);

	ug::bridge::ParameterStack paramsIn;
	ug::bridge::ParameterStack paramsOut;

	std::string name = ug::vrl::stringJ2C(env, methodName);

	jobject result = NULL;

	try {
		const ug::bridge::ExportedMethod* method =
				ug::vrl::invocation::getMethodBySignature(
				env, ug::vrl::vrlRegistry,
				clazz, ug::vrl::boolJ2C(readOnly), name, params);

		if (method == NULL && readOnly == false) {
			method = ug::vrl::invocation::getMethodBySignature(
					env, ug::vrl::vrlRegistry,
					clazz, ug::vrl::boolJ2C(true), name, params);
		}

		if (method == NULL) {

			std::stringstream ss;

			ss << "Method not found: " <<
					EMPHASIZE_BEGIN << name <<
					"()" << EMPHASIZE_END << ".";

			jclass Exception = env->FindClass("edu/gcsc/vrl/ug/UGException");
			env->ThrowNew(Exception, ss.str().c_str());
			return NULL;
		}

		ug::vrl::jobjectArray2ParamStack(
				env, ug::vrl::vrlRegistry,
				paramsIn, method->params_in(), params);


		const ug::bridge::ClassNameNode* clsNode =
				ug::vrl::invocation::getClassNodePtrByName(
				ug::vrl::vrlRegistry, className);

		void* finalObjPtr = ug::bridge::ClassCastProvider::cast_to_base_class(
				(void*) objPtr,
				clsNode, method->class_name());

		method->execute(finalObjPtr, paramsIn, paramsOut);

		if (paramsOut.size() > 0) {
			result = ug::vrl::param2JObject(env, paramsOut, 0);
		}


	} catch (ug::bridge::ERROR_IncompatibleClasses ex) {

		std::stringstream ss;
		ss << "Incompatible Conversion from " <<
				ex.m_from << " to " << ex.m_to;

		jclass Exception = env->FindClass("edu/gcsc/vrl/ug/UGException");
		env->ThrowNew(Exception, ss.str().c_str());
	} catch (ug::bridge::ERROR_BadConversion ex) {

		std::stringstream ss;

		ss << "Incompatible Conversion from " <<
		ex.m_from << " to " << ex.m_to;

		jclass Exception = env->FindClass("edu/gcsc/vrl/ug/UGException");
		env->ThrowNew(Exception, ss.str().c_str());
	} catch (ug::UGError ex) {
		jclass Exception = env->FindClass("edu/gcsc/vrl/ug/UGException");
		env->ThrowNew(Exception, ex.get_msg().c_str());
	} catch (...) {

		std::stringstream ss;

		ss << "Unknown exception thrown while"
				<< " trying to invoke method: " << name << "().";

		jclass Exception = env->FindClass("edu/gcsc/vrl/ug/UGException");
		env->ThrowNew(Exception, ss.str().c_str());
	}

	return result;
}

JNIEXPORT jlong JNICALL Java_edu_gcsc_vrl_ug_UG_newInstance
(JNIEnv *env, jobject obj, jlong objPtr) {

	long result = 0;
	ug::bridge::IExportedClass* clazz = NULL;

	try {

		ug::bridge::IExportedClass* clazz =
				(ug::bridge::IExportedClass*) objPtr;

		result = (long) clazz->create();

	} catch (ug::UGError ex) {

		jclass Exception = env->FindClass("edu/gcsc/vrl/ug/UGException");
		env->ThrowNew(Exception, ex.get_msg().c_str());
	} catch (...) {
		std::string className = "Unknown class";
		if (clazz != NULL) {
			className = clazz->name();
		}

		std::stringstream ss;

		ss << "Unknown exception thrown while"
				<< " trying to instanciate class " << className << "().";

		jclass Exception = env->FindClass("edu/gcsc/vrl/ug/UGException");
		env->ThrowNew(Exception, ss.str().c_str());
	}

	return result;
}

JNIEXPORT jobject JNICALL Java_edu_gcsc_vrl_ug_UG_invokeFunction
(JNIEnv *env, jobject obj, jstring fName, jboolean readOnly, jobjectArray params) {

	const ug::bridge::ExportedFunction* func =
			ug::vrl::invocation::getFunctionBySignature(
			env, ug::vrl::vrlRegistry, ug::vrl::stringJ2C(env, fName), params);

	ug::bridge::ParameterStack paramsIn;
	ug::bridge::ParameterStack paramsOut;

	jobject result = NULL;

	try {

		if (func == NULL) {
			std::stringstream ss;

			ss << "Function not found: " <<
					EMPHASIZE_BEGIN << ug::vrl::stringJ2C(env, fName) <<
					"()" << EMPHASIZE_END << ".";

			jclass Exception = env->FindClass("edu/gcsc/vrl/ug/UGException");
			env->ThrowNew(Exception, ss.str().c_str());

			return NULL;
		}

		ug::vrl::jobjectArray2ParamStack(
				env, ug::vrl::vrlRegistry, paramsIn, func->params_in(), params);

		func->execute(paramsIn, paramsOut);

		if (paramsOut.size() > 0) {
			result = ug::vrl::param2JObject(env, paramsOut, 0);
		}

	} catch (ug::bridge::ERROR_IncompatibleClasses ex) {
		std::stringstream ss;
		ss << "Incompatible Conversion from " <<
				ex.m_from << " to " << ex.m_to;

		jclass Exception = env->FindClass("edu/gcsc/vrl/ug/UGException");
		env->ThrowNew(Exception, ss.str().c_str());

	} catch (ug::bridge::ERROR_BadConversion ex) {
		std::stringstream ss;
		ss << "Incompatible Conversion from " <<
				ex.m_from << " to " << ex.m_to;

		jclass Exception = env->FindClass("edu/gcsc/vrl/ug/UGException");
		env->ThrowNew(Exception, ss.str().c_str());

	} catch (...) {
		std::stringstream ss;

		ss << "Unknown exception thrown while"
				<< " trying to invoke function: " <<
				ug::vrl::stringJ2C(env, fName) << "().";

		jclass Exception = env->FindClass("edu/gcsc/vrl/ug/UGException");
		env->ThrowNew(Exception, ss.str().c_str());
	}

	return result;
}

JNIEXPORT jlong JNICALL Java_edu_gcsc_vrl_ug_UG_getExportedClassPtrByName
(JNIEnv *env, jobject obj, jstring name, jboolean classGrp) {

	if (ug::vrl::boolJ2C(classGrp)) {

		const ug::bridge::ClassGroupDesc* grpDesc =
				ug::vrl::vrlRegistry->get_class_group(
				ug::vrl::stringJ2C(env, name).c_str());

		if (grpDesc == NULL || grpDesc->get_default_class() == NULL) {
			return (long) NULL;
		}

		return (long) grpDesc->get_default_class();

	} else {
		return (long) ug::vrl::invocation::getExportedClassPtrByName(
				ug::vrl::vrlRegistry, ug::vrl::stringJ2C(env, name));
	}

	return (long) NULL;
}

JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG_getDefaultClassNameFromGroup
(JNIEnv *env, jobject obj, jstring grpName) {
	const ug::bridge::ClassGroupDesc* grpDesc =
			ug::vrl::vrlRegistry->get_class_group(
			ug::vrl::stringJ2C(env, grpName).c_str());

	if (grpDesc == NULL) {
		return ug::vrl::stringC2J(env, "");
	}

	if (grpDesc->get_default_class() == NULL) {
		return ug::vrl::stringC2J(env, "");
	}

	//\todo: @Michi: not using c_str()
	return ug::vrl::stringC2J(env, grpDesc->get_default_class()->name().c_str());
}

JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG_getSvnRevision
(JNIEnv *env, jobject obj) {
	std::string revision = ug::vrl::svnRevision();
	return ug::vrl::stringC2J(env, revision.c_str());
}

JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG_getCompileDate
(JNIEnv *env, jobject obj) {
	return ug::vrl::stringC2J(env, COMPILE_DATE);
}

JNIEXPORT void JNICALL Java_edu_gcsc_vrl_ug_MemoryManager_delete
(JNIEnv * env, jclass cls, jlong objPtr, jlong exportedClsPtr) {

	if (((void*) objPtr) != NULL && ((void*) exportedClsPtr) != NULL) {
		ug::bridge::IExportedClass* clazz =
				(ug::bridge::IExportedClass*) exportedClsPtr;
		clazz->destroy((void*) objPtr);
	}
}

JNIEXPORT void JNICALL Java_edu_gcsc_vrl_ug_MemoryManager_invalidate
(JNIEnv * env, jclass cls, jobject smartPtr) {

	if (ug::vrl::isJSmartPointerConst(env, smartPtr)) {
		ug::vrl::invalidateJConstSmartPointer(env, smartPtr);
	} else {
		ug::vrl::invalidateJSmartPointer(env, smartPtr);
	}
}

JNIEXPORT jobject JNICALL Java_edu_gcsc_vrl_ug_UG_convertRegistryInfo
(JNIEnv * env, jobject obj) {
	return ug::vrl::registry2NativeAPI(env, ug::vrl::vrlRegistry);
}

JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG_getDescription
(JNIEnv *env, jobject obj) {
	std::string desc = 
			"UG is a general platform for the numerical solution<br>"
            " of partial differential equations.";
	
	return ug::vrl::stringC2J(env, desc.c_str());
}

JNIEXPORT jstring JNICALL Java_edu_gcsc_vrl_ug_UG_getAuthors
(JNIEnv *env, jobject obj) {
	return ug::vrl::stringC2J(env, ug::UG_AUTHORS.c_str());
}



//JNIEXPORT void JNICALL Java_edu_gcsc_vrl_ug_UG_attachCanvas
//(JNIEnv *env, jobject obj, jobject canvas) {
//	ug::vrl::Canvas::getInstance()->setJObject(env, canvas);
//
//	//	ug::vrl::Canvas::getInstance()->addObject(ug::vrl::string2JObject(env,"Test_String"));
//}

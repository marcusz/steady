//
//  cpp_extension.cpp
//  steady
//
//  Created by Marcus Zetterquist on 2013-11-22.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#include "quark.h"

#include <iostream>
#include <cmath>
#include <cassert>
#include <sstream>



namespace quark {

#if QUARK__UNIT_TESTS_ON
unit_test_registry* unit_test_rec::_registry_instance = nullptr;
#endif


namespace {
	runtime_i* gRuntimePtr = nullptr;
}


runtime_i* get_runtime(){
	return gRuntimePtr;
}

void set_runtime(runtime_i* iRuntime){
	gRuntimePtr = iRuntime;
}






//	ASSERT SUPPORT
//	====================================================================================================================





#if QUARK__ASSERT_ON

void on_assert_hook(runtime_i* runtime, const source_code_location& location, const char expression[]){
	assert(runtime != nullptr);
	assert(expression != nullptr);

	runtime->runtime_i__on_assert(location, expression);
	exit(-1);
}

#endif




//	TRACE
//	====================================================================================================================



#if QUARK__TRACE_ON

void on_trace_hook(runtime_i* runtime, const char s[]){
	assert(runtime != nullptr);
	assert(s != nullptr);

	runtime->runtime_i__trace(s);
}

void on_trace_hook(runtime_i* runtime, const std::string& s){
	assert(runtime != nullptr);

	runtime->runtime_i__trace(s.c_str());
}

void on_trace_hook(runtime_i* runtime, const std::stringstream& s){
	assert(runtime != nullptr);

	runtime->runtime_i__trace(s.str().c_str());
}

#endif





//	UNIT TEST SUPPORT
//	====================================================================================================================




#if QUARK__UNIT_TESTS_ON

void on_unit_test_failed_hook(runtime_i* runtime, const source_code_location& location, const char expression[]){
	assert(runtime != nullptr);
	assert(expression != nullptr);

	runtime->runtime_i__on_unit_test_failed(location, expression);
}

/*
std::string OnGetPrivateTestDataPath(runtime_i* iRuntime, const char iModuleUnderTest[], const char iSourceFilePath[]){
	ASSERT(iRuntime != nullptr);
	ASSERT(iModuleUnderTest != nullptr);
	ASSERT(iSourceFilePath != nullptr);

	const TAbsolutePath absPath(iSourceFilePath);
	const TAbsolutePath parent = GetParent(absPath);
	return parent.GetStringPath();
}
*/

void run_tests(){
	QUARK_TRACE_FUNCTION();
	QUARK_ASSERT(unit_test_rec::_registry_instance != nullptr);

	std::size_t test_count = unit_test_rec::_registry_instance->_tests.size();
//	std::size_t fail_count = 0;

	QUARK_TRACE_SS("Running " << test_count << " tests...");

	for(std::size_t i = 0 ; i < test_count ; i++){
		const unit_test_def& test = unit_test_rec::_registry_instance->_tests[i];

		std::stringstream testInfo;
		testInfo << "Test #" << i
			<< " " << test._class_under_test
			<< " | " << test._function_under_test
			<< " | " << test._scenario
			<< " | " << test._expected_result;

		try{
			QUARK_SCOPED_TRACE(testInfo.str());
			test._test_f();
		}
		catch(...){
			QUARK_TRACE("FAILURE: " + testInfo.str());
//			fail_count++;
			exit(-1);
		}
	}

//	if(fail_count == 0){
		QUARK_TRACE_SS("Success - " << test_count << " tests!");
//	}
//	else{
//		TRACE_SS("FAILED " << fail_count << " out of " << test_count << " tests!");
//		exit(-1);
//	}
}

#endif





//	Default implementation
//	====================================================================================================================




//////////////////////////////////			TDefaultRuntime





TDefaultRuntime::TDefaultRuntime(const std::string& test_data_root) :
	_test_data_root(test_data_root),
	_indent(0)
{
}

void TDefaultRuntime::runtime_i__trace(const char s[]){
//		for (auto &i: items){
//		}
	for(long i = 0 ; i < _indent ; i++){
		std::cout << "|\t";
	}

	std::cout << std::string(s);
	std::cout << std::endl;
}

void TDefaultRuntime::runtime_i__add_log_indent(long add){
	_indent += add;
}

void TDefaultRuntime::runtime_i__on_assert(const source_code_location& location, const char expression[]){
	QUARK_TRACE_SS(std::string("Assertion failed ") << location._source_file << ", " << location._line_number << " \"" << expression << "\"");
	perror("perror() says");
	throw std::logic_error("assert");
}

void TDefaultRuntime::runtime_i__on_unit_test_failed(const source_code_location& location, const char expression[]){
	QUARK_TRACE_SS("Unit test failed " << location._source_file << ", " << location._line_number << " \"" << expression << "\"");
	perror("perror() says");

	throw std::logic_error("Unit test failed");
}




//	TESTS
//	====================================================================================================================



/*
	This function uses all macros so we know they compile.
*/
void TestMacros(){
	QUARK_ASSERT(true);
	QUARK_ASSERT_UNREACHABLE;

	QUARK_TRACE("hello");
	QUARK_TRACE_SS("hello" << 1234);
	QUARK_SCOPED_TRACE("scoped trace");
	QUARK_SCOPED_INDENT();
	QUARK_TRACE_FUNCTION();
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_UT_VERIFY(true);
	QUARK_TEST_VERIFY(true);
}



}


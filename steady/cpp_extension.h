//
//  cpp_extension.h
//  steady
//
//  Created by Marcus Zetterquist on 2013-11-16.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#ifndef steady_cpp_extension_h
#define steady_cpp_extension_h

#include <cassert>
#include <vector>
#include <string>


typedef void (*unit_test_function)();




#define PP_STRING(a) #a
#define PP_CONCAT2(a,b)  a##b
#define PP_CONCAT3(a, b, c) a##b##c
#define PP_UNIQUE_LABEL_(prefix, suffix) PP_CONCAT2(prefix, suffix)
#define PP_UNIQUE_LABEL(prefix) PP_UNIQUE_LABEL_(prefix, __LINE__)


struct TUnitTest {
	TUnitTest(const std::string& p1, const std::string& p2, const std::string& p3, const std::string& p4, unit_test_function f)
	:
		_class_under_test(p1),
		_function_under_test(p2),
		_scenario(p3),
		_expected_result(p4),
		_test_f(f)
	{
	}


	////////////////		State.
		std::string _class_under_test;

		std::string _function_under_test;
		std::string _scenario;
		std::string _expected_result;

		unit_test_function _test_f;
};

extern std::vector<const TUnitTest> gUnitTests;

struct TUnitTestReg {
	TUnitTestReg(const std::string& p1, const std::string& p2, const std::string& p3, const std::string& p4, unit_test_function f){
		TUnitTest test(p1, p2, p3, p4, f);
		gUnitTests.push_back(test);
	}
};


class icppextension_runtime {
	public: virtual void icppextension_runtime__trace(const char s[]) = 0;
	public: virtual void icppextension_runtime__assert(const char s[]) = 0;
	public: virtual void icppextension_runtime__dbc_precondition_failed(const char s[]) = 0;
	public: virtual void icppextension_runtime__dbc_postcondition_failed(const char s[]) = 0;
	public: virtual void icppextension_runtime__dbc_invariant_failed(const char s[]) = 0;
};

//	### Use runtime as arg.
#define TRACE(s)
#define ASSERT assert


#define UNIT_TEST(class_under_test, function_under_test, scenario, expected_result) \
	static void PP_UNIQUE_LABEL(fun_)(); \
	TUnitTestReg PP_UNIQUE_LABEL(rec)(class_under_test, function_under_test, scenario, expected_result, PP_UNIQUE_LABEL(fun_)); \
	static void PP_UNIQUE_LABEL(fun_)()


#define TEST_VERIFY(exp) if(exp){}else{ throw std::logic_error("TEST_VERIFY failed"); }


void run_tests();


#endif

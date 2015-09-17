
#ifndef quark_h
#define quark_h

/*

QUARK
Why: the low-level bits standard C++ is missing. Tracing, asserts, unit tests. The guiding principle for quark is
minimalism.


TRACING
------------------------------------------------------------------------------------------------------------------------
Quark has primitives for tracing that can be routed and enabled / disabled. Includes support for indenting the log.
Use QUARK__TRACE_ON to enable / disable tracing.

Examples:

	Trace a C string
		QUARK_TRACE("abc");

	Trace using << code
		QUARK_TRACE_SS("my value" << 123);

	Trace a title, then increase all indentation until scope is left.
		{
			QUARK_SCOPED_TRACE("File contents");
			QUARK_TRACE("insides");
		}

	Increase indentation until scope is left.
		{
			QUARK_SCOPED_INDENT;
			QUARK_TRACE("insides");
		}


ASSERT
------------------------------------------------------------------------------------------------------------------------
Assert is used for finding runtime defects. Use QUARK__ASSERT_ON to enable / disable all asserts.


Examples:
	Check function arguments
		char* my_strlen(const char* s){
			QUARK_ASSERT(s != nullptr);
			...
		}

	Detect

	Detect impossible conditions:
		if(a == 0){
		}
		else if (a == 3){
		}
		else {
			QUARK_ASSERT_UNREACHABLE;
		}


UNIT TEST
------------------------------------------------------------------------------------------------------------------------
You can easily add a unit test where ever you can define a function. It's possible to interleave the functions with
the unit tests that excercise the functions. You supply 4 strings to each test:

QUARK_UNIT_TEST(class_under_test, function_under_test, scenario, expected_result)

You check for failure / success using QUARK_TEST_VERIFY(expression). Do not use QUARK_ASSERT to check for test failures.

It is possible to run unit tests even when asserts are disabled.


Examples:

	Registering a unit test

		QUARK_UNIT_TEST("std::list, "list()", "basic construction", "no exceptions"){
			std::list<int> a;
		}

		QUARK_UNIT_TEST("std::list, "find()", "empty list", "returns list.end()"){
			std::list<int> a;
			std::list<int>::const_iterator found_it = a.find(123);
			QUARK_TEST_VERIFY(found_it == a.end());
		}

	Running all unit tests:
		quark::run_tests();
*/


#include <cassert>
#include <vector>
#include <string>
#include <sstream>


#define QUARK__ASSERT_ON true
#define QUARK__TRACE_ON true
#define QUARK__UNIT_TESTS_ON true


namespace quark {

/*

//	TODO
//	====================================================================================================================

	Fix so you can put your unit tests inside unnamed namespaces (to make tests internal).
	internals-namespace

	Add basic exception classes, ala bacteria.

	Text lookup:
		/	Use 7-bit english text as lookup key.
		//	Returns localized utf8 text.
		//	Basic implementations can chose to just return the lookup key.
		//	addKey is additional characters to use for lookup, but that is not part of the actual returned-text if
		//	no localization exists.
		public: virtual std::string runtime_i__lookup_text(const source_code_location& location,
			const int locale,
			const char englishLookupKey[],
			const char addKey[]) = 0;

	Design by contract:
		public: virtual void runtime_i__on_dbc_precondition_failed(const char s[]) = 0;
		public: virtual void runtime_i__on_dbc_postcondition_failed(const char s[]) = 0;
		public: virtual void runtime_i__on_dbc_invariant_failed(const char s[]) = 0;

	Test files for unit tests
		//	gives you native, absolute path to your modules test-directory.
		#define UNIT_TEST_PRIVATE_DATA_PATH(moduleUnderTest) OnGetPrivateTestDataPath(get_runtime(), moduleUnderTest, __FILE__)
		std::string OnGetPrivateTestDataPath(runtime_i* iRuntime, const char module_under_test[], const char source_file_path[]);

		public: virtual std::string icppextension_get_test_data_root(const char iModuleUnderTest[]) = 0;
*/



//	PRIMITIVES
//	====================================================================================================================





////////////////////////////		source_code_location


/*
	Value-object that specifies a specific line of code in a specific source file.
*/



struct source_code_location {
	source_code_location(const char source_file[], long line_number) :
		_line_number(line_number)
	{
		assert(source_file != nullptr);
		assert(std::strlen(source_file) <= 1024);
		strcpy(_source_file, source_file);
	}

	char _source_file[1024 + 1];
	long _line_number;
};


////////////////////////////		runtime_i

/*
	Interface class so client can hook-in behaviors for basics like logging and asserts.
*/

class runtime_i {
	public: virtual ~runtime_i(){};
	public: virtual void runtime_i__trace(const char s[]) = 0;
	public: virtual void runtime_i__add_log_indent(long add) = 0;
	public: virtual void runtime_i__on_assert(const source_code_location& location, const char expression[]) = 0;
	public: virtual void runtime_i__on_unit_test_failed(const source_code_location& location, const char s[]) = 0;
};


////////////////////////////		get_runtime() and set_runtime()

/*
	Global functions for storing the current runtime.
	Notice that only the macros use these! The implementation does NOT.
*/

runtime_i* get_runtime();
void set_runtime(runtime_i* iRuntime);



//	WORKAROUNDS
//	====================================================================================================================


/*
	Macros to generate unique names for unit test functions etc.
*/

#define QUARK_STRING(a) #a
#define QUARK_CONCAT2(a,b)  a##b
#define QUARK_CONCAT3(a, b, c) a##b##c
#define QUARK_UNIQUE_LABEL_INTERNAL(prefix, suffix) QUARK_CONCAT2(prefix, suffix)
#define QUARK_UNIQUE_LABEL(prefix) QUARK_UNIQUE_LABEL_INTERNAL(prefix, __LINE__)



//	ASSERT SUPPORT
//	====================================================================================================================



#if QUARK__ASSERT_ON

	void on_assert_hook(runtime_i* runtime, const source_code_location& location, const char expression[]) __dead2;

	#define QUARK_ASSERT(x) if(x){}else {::quark::on_assert_hook(::quark::get_runtime(), quark::source_code_location(__FILE__, __LINE__), QUARK_STRING(x)); }

	#define QUARK_ASSERT_UNREACHABLE QUARK_ASSERT(false)

#else

	#define QUARK_ASSERT(x)
	#define QUARK_ASSERT_UNREACHABLE throw std::logic_error("")

#endif




//	TRACE
//	====================================================================================================================



////////////////////////////		scoped_trace
/*
	Part of internal mechanism to get stack / scoped-based RAII working for indented tracing.
*/

struct scoped_trace {
	scoped_trace(const char s[]){
#if QUARK__TRACE_ON
		runtime_i* r = get_runtime();
		r->runtime_i__trace(s);
		r->runtime_i__trace("{");
		r->runtime_i__add_log_indent(1);
#endif
	}

	scoped_trace(const std::string& s){
#if QUARK__TRACE_ON
		runtime_i* r = get_runtime();
		r->runtime_i__trace(s.c_str());
		r->runtime_i__trace("{");
		r->runtime_i__add_log_indent(1);
#endif
	}

	scoped_trace(const std::stringstream& s){
#if QUARK__TRACE_ON
		runtime_i* r = get_runtime();
		r->runtime_i__trace(s.str().c_str());
		r->runtime_i__trace("{");
		r->runtime_i__add_log_indent(1);
#endif
	}

	~scoped_trace(){
#if QUARK__TRACE_ON
		runtime_i* r = get_runtime();
		r->runtime_i__add_log_indent(-1);
		r->runtime_i__trace("}");
#endif
	}
};


////////////////////////////		scoped_trace_indent
/*
	Part of internal mechanism to get stack / scoped-based RAII working for indented tracing.
*/

struct scoped_trace_indent {
#if QUARK__TRACE_ON
	scoped_trace_indent(){
		runtime_i* r = get_runtime();
		r->runtime_i__add_log_indent(1);
	}
	~scoped_trace_indent(){
		runtime_i* r = get_runtime();
		r->runtime_i__add_log_indent(-1);
	}
#endif
};


#if QUARK__TRACE_ON

	////////////////////////////		Hook functions.
	/*
		These functions are called by the macros and they in turn call the runtime_i.
	*/
	void on_trace_hook(runtime_i* runtime, const char s[]);
	void on_trace_hook(runtime_i* runtime, const std::string& s);
	void on_trace_hook(runtime_i* runtime, const std::stringstream& s);


	//	### Use runtime as explicit argument instead?
	#define QUARK_TRACE(s) ::quark::on_trace_hook(::quark::get_runtime(), s)
	#define QUARK_TRACE_SS(x) {std::stringstream ss; ss << x; ::quark::on_trace_hook(::quark::get_runtime(), ss);}

	/*
		Works with:
			char[]
			std::string
	*/
	#define QUARK_SCOPED_TRACE(s) ::quark::scoped_trace QUARK_UNIQUE_LABEL(scoped_trace) (s)
	#define QUARK_SCOPED_INDENT() ::quark::scoped_trace_indent QUARK_UNIQUE_LABEL(scoped_indent)

	#define QUARK_TRACE_FUNCTION() ::quark::scoped_trace QUARK_UNIQUE_LABEL(trace_function) (__FUNCTION__)

#else

	#define QUARK_TRACE(s)
	#define QUARK_TRACE_SS(s)

	#define QUARK_SCOPED_TRACE(s)
	#define QUARK_SCOPED_INDENT()
	#define QUARK_TRACE_FUNCTION()

#endif




//	UNIT TEST SUPPORT
//	====================================================================================================================


#if QUARK__UNIT_TESTS_ON


	typedef void (*unit_test_function)();


	////////////////////////////		unit_test_def
	/*
		The defintion of a single unit test, including the function itself.
	*/
	struct unit_test_def {
		unit_test_def(const std::string& p1, const std::string& p2, const std::string& p3, const std::string& p4, unit_test_function f)
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


	////////////////////////////		unit_test_registry
	/*
		Stores all unit tests registered for the entire executable.
	*/
	struct unit_test_registry {
		public: std::vector<const unit_test_def> _tests;
	};


	////////////////////////////		unit_test_rec
	/*
		This is part of an RAII-mechansim to register and unregister unit-tests.
	*/
	struct unit_test_rec {
		unit_test_rec(const std::string& p1, const std::string& p2, const std::string& p3, const std::string& p4, unit_test_function f){
			unit_test_def test(p1, p2, p3, p4, f);
			if(!_registry_instance){
				_registry_instance = new unit_test_registry();
			}
			_registry_instance->_tests.push_back(test);
		}


		////////////////		State.

		//	!!! Singleton. ### lose this.
		static unit_test_registry* _registry_instance;
	};


	////////////////////////////		Hooks


	void on_unit_test_failed_hook(runtime_i* runtime, const source_code_location& location, const char expression[]);


	////////////////////////////		run_tests()
	/*
		Client application calls this function run all unit tests.
		It will handle tracing and exceptions etc.
		On unit-test failure this function exits the executable.
	*/
	void run_tests();


	////////////////////////////		Macros used by client code


	//	The generated function is static and will be stripped in optimized builds (it will not be referenced).
	#define QUARK_UNIT_TEST(class_under_test, function_under_test, scenario, expected_result) \
		static void QUARK_UNIQUE_LABEL(cppext_unit_test_)(); \
		static ::quark::unit_test_rec QUARK_UNIQUE_LABEL(rec)(class_under_test, function_under_test, scenario, expected_result, QUARK_UNIQUE_LABEL(cppext_unit_test_)); \
		static void QUARK_UNIQUE_LABEL(cppext_unit_test_)()

	//### Add argument to unit-test functions that can be used / checked in UT_VERIFY().
	#define QUARK_UT_VERIFY(exp) if(exp){}else{ ::quark::on_unit_test_failed_hook(::quark::get_runtime(), ::quark::source_code_location(__FILE__, __LINE__), QUARK_STRING(exp)); }

	#define QUARK_TEST_VERIFY QUARK_UT_VERIFY

#else

	//	The generated function is static and will be stripped in optimized builds (it will not be referenced).
	#define QUARK_UNIT_TEST(class_under_test, function_under_test, scenario, expected_result) \
		void QUARK_UNIQUE_LABEL(cppext_unit_test_)()

	#define QUARK_UT_VERIFY(exp)
	#define QUARK_TEST_VERIFY QUARK_UT_VERIFY

#endif




//	Default implementation
//	====================================================================================================================



//////////////////////////////////			default_runtime
/*
	This is a default implementation that client can chose to instantiate and plug-in using set_runtime().

	It uses cout.
*/
struct default_runtime : public runtime_i {
	default_runtime(const std::string& test_data_root);

	public: virtual void runtime_i__trace(const char s[]);
	public: virtual void runtime_i__add_log_indent(long add);
	public: virtual void runtime_i__on_assert(const source_code_location& location, const char expression[]);
	public: virtual void runtime_i__on_unit_test_failed(const source_code_location& location, const char expression[]);


	///////////////		State.
		const std::string _test_data_root;
		long _indent;
};


}	//	quark

#endif

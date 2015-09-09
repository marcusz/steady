//
//  cpp_extension.cpp
//  steady
//
//  Created by Marcus Zetterquist on 2013-11-22.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#include "cpp_extension.h"

#include <iostream>
#include <cmath>



std::vector<const TUnitTest> gUnitTests;


void run_tests(){
	std::size_t test_count = gUnitTests.size();
	std::size_t fail_count = 0;
	std::cout << "Running " << test_count << " tests..." << std::endl;
	for(long i = 0 ; i < test_count ; i++){
		const TUnitTest& test = gUnitTests[i];
		try{
			gUnitTests[i]._test_f();
		}
		catch(...){
			std::cout
				<< "FAILURE:\t"
				<< test._class_under_test
				<< " \t|\t" << test._function_under_test
				<< " " << test._scenario
				<< " " << test._expected_result << std::endl;
			fail_count++;
		}
	}

	if(fail_count == 0){
		std::cout << "Success - " << test_count << " tests!" << std::endl;
	}
	else{
		std::cout << "FAILED " << fail_count << " out of " << test_count << " tests!" << std::endl;
	}
}



#if DEBUG
//		???
#else
#endif


#if NDEBUG
#else
#endif




static void MyTest();
TUnitTestReg xxxx("p1", "p2", "p3", "p4", MyTest);
static void MyTest(){
	TEST_VERIFY(true);
}



UNIT_TEST("cpp_extension", "sin()", "0.0", "return 0.0"){
	TEST_VERIFY(std::sin(0.0f) == 0.0f);
}

UNIT_TEST("cpp_extension", "cos()", "0.0", "return 0.0"){
	TEST_VERIFY(std::cos(0.0f) == 1.0f);
}



UNIT_TEST("cpp_extension", "misc", "", ""){
	ASSERT(true);
//	ASSERT(false);
	TRACE("Test trace");
	TEST_VERIFY(true);

	std::cout << "Test" << std::endl;
}


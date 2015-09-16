//
//  main.cpp
//  steady
//
//  Created by Marcus Zetterquist on 2013-11-13.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//


#include "steady_vector.h"

#include <iostream>



void Example(){
	{
		steady::steady_vector<int> a;
		a = a.push_back(3);
		a = a.push_back(8);
		a = a.push_back(11);

		QUARK_ASSERT(a.size() == 3);
	}

	{
	}
}

int main(int argc, const char * argv[]){
#if QUARK__UNIT_TESTS_ON
	quark::run_tests();
#endif

	try {
		QUARK_TRACE("Hello, World 3!");
		Example();
	}
	catch(...){
		QUARK_TRACE("Error");
		return -1;
	}

    return 0;
}

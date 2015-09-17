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
		steady::vector<int> a;
		a = a.push_back(3);
		a = a.push_back(8);
		a = a.push_back(11);

		QUARK_ASSERT(a.size() == 3);
	}

	{
		const steady::vector<int> a{ 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 };
		const auto b = a + a + a + a + a + a + a + a + a + a;
		QUARK_ASSERT(b.size() == 100);

		const auto c = b + b + b + b + b + b + b + b + b + b;
		QUARK_ASSERT(c.size() == 1000);

		const auto d = c + c + c + c + c + c + c + c + c + c;
		QUARK_ASSERT(d.size() == 10000);

		const auto e = d + d + d + d + d + d + d + d + d + d;
		QUARK_ASSERT(e.size() == 100000);
	}


#if false
	{
		steady::vector<int> a;
		for(int i = 0 ; i < 10000000 ; i++){
			 a = a.push_back(i);
		}
		QUARK_ASSERT(a.size() == 1000000);
	}
#endif
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

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
	/*
		Example 1
		Make a vector of ints. Add a few numbers.
		Notice that push_back() returns a new vector each time - you need to save the return value.
		There are no side effects. This makes code very simple and solid.
		It also makes it simple to design pure functions.
	*/
	{
		steady::vector<int> a;
		a.push_back(3);
		a.push_back(8);
		a.push_back(11);

		//	Notice! a is still the empty vector! It has not changed!
		assert(a.size() == 0);

		//	Reuse variable b to keep the latest generation of the vector.
		steady::vector<int> b;
		b = b.push_back(3);
		b = b.push_back(8);
		b = b.push_back(11);

		assert(b.size() == 3);
		assert(b[2] == 11);
	}

	/*
		Example 2
		Demonstrate that "modifying" a vector leaves the original unchanged too.
		Also: make the vector using initializer list (c++11)
	*/
	{
		const steady::vector<int> a{ 10, 20, 30 };
		const auto b = a.push_back(40);
		const auto c = b.push_back(50);

		assert(a == (steady::vector<int>{ 10, 20, 30 }));
		assert(b == (steady::vector<int>{ 10, 20, 30, 40 }));
		assert(c == (steady::vector<int>{ 10, 20, 30, 40, 50 }));
		assert(c.size() == 5);
	}

	/*
		Example 3
		Replace values in the vector. This also leaves original vector unchanged.
	*/
	{
		const steady::vector<int> a{ 10, 20, 30 };
		const auto b = a.store(0, 2010);
		const auto c = b.store(2, 2030);

		assert(a == (steady::vector<int>{ 10, 20, 30 }));
		assert(b == (steady::vector<int>{ 2010, 20, 30 }));
		assert(c == (steady::vector<int>{ 2010, 20, 2030 }));
	}

	/*
		Example 4
		Append two vectors.
	*/
	{
		const steady::vector<int> a{ 1, 2, 3 };
		const steady::vector<int> b{ 4, 5 };
		const auto c = a + b;

		assert(a == (steady::vector<int>{ 1, 2, 3 }));
		assert(b == (steady::vector<int>{ 4, 5 }));
		assert(c == (steady::vector<int>{ 1, 2, 3, 4, 5 }));
	}

	/*
		Example 5
		Converting to and from std::vector<>.
	*/
	{
		const std::vector<int> a{ 1, 2, 3 };

		//	Make a steady::vector<> from a std::vector<>:
		const steady::vector<int> b(a);

		//	Make a std::vector<> from a steady::vector<>
		const std::vector<int> c = b.to_vec();

		assert(a == (std::vector<int>{ 1, 2, 3 }));
		assert(b == (steady::vector<int>{ 1, 2, 3 }));
		assert(c == (std::vector<int>{ 1, 2, 3 }));
	}

	/*
		Example 6
		Try vectors of strings instead of ints.
	*/
	{
//		using std::string;

		const steady::vector<std::string> a{ "one", "two", "three" };

		const steady::vector<std::string> b{ "four", "five" };
		const auto c = a + b;

		assert(a == (steady::vector<std::string>{ "one", "two", "three" }));
		assert(b == (steady::vector<std::string>{ "four", "five" }));
		assert(c == (steady::vector<std::string>{ "one", "two", "three", "four", "five" }));
	}



	{
		const steady::vector<int> a{ 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 };
		const auto b = a + a + a + a + a + a + a + a + a + a;
		assert(b.size() == 100);

		const auto c = b + b + b + b + b + b + b + b + b + b;
		assert(c.size() == 1000);

		const auto d = c + c + c + c + c + c + c + c + c + c;
		assert(d.size() == 10000);

		const auto e = d + d + d + d + d + d + d + d + d + d;
		assert(e.size() == 100000);
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

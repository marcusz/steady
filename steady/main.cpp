/*
	Copyright 2015 Marcus Zetterquist

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.

	steady::vector<> is a persistent vector class for C++
*/

#include "steady_vector.h"

#include <iostream>


/*
	Make a vector of ints. Add a few numbers.
	Notice that push_back() returns a new vector each time - you need to save the return value.
	There are no side effects. This makes code very simple and solid.
	It also makes it simple to design pure functions.
*/
void example1(){
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
	Demonstrate that "modifying" a vector leaves the original unchanged too.
	Also: make the vector using initializer list (c++11)
*/
void example2(){
	const steady::vector<int> a{ 10, 20, 30 };
	const auto b = a.push_back(40);
	const auto c = b.push_back(50);

	assert(a == (steady::vector<int>{ 10, 20, 30 }));
	assert(b == (steady::vector<int>{ 10, 20, 30, 40 }));
	assert(c == (steady::vector<int>{ 10, 20, 30, 40, 50 }));
	assert(c.size() == 5);
}


/*
	Replace values in the vector. This also leaves original vector unchanged.
*/
void example3(){
	const steady::vector<int> a{ 10, 20, 30 };
	const auto b = a.store(0, 2010);
	const auto c = b.store(2, 2030);

	assert(a == (steady::vector<int>{ 10, 20, 30 }));
	assert(b == (steady::vector<int>{ 2010, 20, 30 }));
	assert(c == (steady::vector<int>{ 2010, 20, 2030 }));
}


/*
	Append two vectors.
*/
void example4(){
	const steady::vector<int> a{ 1, 2, 3 };
	const steady::vector<int> b{ 4, 5 };
	const auto c = a + b;

	assert(a == (steady::vector<int>{ 1, 2, 3 }));
	assert(b == (steady::vector<int>{ 4, 5 }));
	assert(c == (steady::vector<int>{ 1, 2, 3, 4, 5 }));
}


/*
	Converting to and from std::vector<>.
*/
void example5(){
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
	Try vectors of strings instead of ints.
*/
void example6(){
	using std::string;

	const steady::vector<string> a{ "one", "two", "three" };
	const steady::vector<string> b{ "four", "five" };
	const auto c = a + b;

	assert(a == (steady::vector<string>{ "one", "two", "three" }));
	assert(b == (steady::vector<string>{ "four", "five" }));
	assert(c == (steady::vector<string>{ "one", "two", "three", "four", "five" }));
}


/*
	Build huge vector by appending repeatedly.
*/
void example7(){
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



void examples(){
	example1();
	example2();
	example3();
	example4();
	example5();
	example6();
	example7();
}

int main(int argc, const char * argv[]){
#if QUARK_UNIT_TESTS_ON
	quark::run_tests();
#endif

	try {
		QUARK_TRACE("Hello, World 3!");
		examples();
	}
	catch(...){
		QUARK_TRACE("Error");
		return -1;
	}

    return 0;
}

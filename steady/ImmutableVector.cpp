//
//  ImmutableVector.cpp
//  Permafrost
//
//  Created by Marcus Zetterquist on 2014-01-21.
//  Copyright (c) 2014 Marcus Zetterquist. All rights reserved.
//

#include "ImmutableVector.h"

#include "cpp_extension.h"



/*
UNIT_TEST("ImmutableVector.h", "std::vector<int>operator[]", "Try getting pointer to first element of empty vector", "!= nullptr"){
	std::vector<int> a;
	int* ptr = &a[0];
	UT_VERIFY(ptr != nullptr);
}
*/



////////////////////////		Constructor #1


UNIT_TEST("ImmutableVector.h", "immutable_vector::immutable_vector()", "Most basic construction", "empty() == true"){
	const immutable_vector<int> a;
	UT_VERIFY(a.empty());
}


////////////////////////		Constructor #2


UNIT_TEST("ImmutableVector.h", "immutable_vector::immutable_vector()", "count 0", "empty() == true"){
	const immutable_vector<int> a(0, 42);
	UT_VERIFY(a.empty());
}

UNIT_TEST("ImmutableVector.h", "immutable_vector::immutable_vector()", "count 3", "operator[] can give contents"){
	const immutable_vector<int> a(3, 42);
	UT_VERIFY(a[0] == 42);
	UT_VERIFY(a[1] == 42);
	UT_VERIFY(a[2] == 42);
}


////////////////////////		Constructor #3


UNIT_TEST("ImmutableVector.h", "immutable_vector::immutable_vector()", "empty", "empty() == true"){
	const std::vector<int> test{};
	const immutable_vector<int> a(test);
	UT_VERIFY(a.empty());
}

UNIT_TEST("ImmutableVector.h", "immutable_vector::immutable_vector()", "3 items", "operator[] can give contents"){
	const std::vector<int> test = { 91, 92, 93 };
	const immutable_vector<int> a(test);
	UT_VERIFY(a[0] == 91);
	UT_VERIFY(a[1] == 92);
	UT_VERIFY(a[2] == 93);
}


////////////////////////		Constructor #4


UNIT_TEST("ImmutableVector.h", "immutable_vector::immutable_vector()", "empty source", "empty() == true"){
	const immutable_vector<int> test;
	const immutable_vector<int> a(test);
	UT_VERIFY(a.empty());
}

UNIT_TEST("ImmutableVector.h", "immutable_vector::immutable_vector()", "3 items", "operator[] can give contents"){
	const std::vector<int> test = { 91, 92, 93 };
	const immutable_vector<int> a(test);
	UT_VERIFY(a[0] == 91);
	UT_VERIFY(a[1] == 92);
	UT_VERIFY(a[2] == 93);
}



////////////////////////		Constructor #5


UNIT_TEST("ImmutableVector.h", "immutable_vector::immutable_vector()", "empty source", "empty() == true"){
	const immutable_vector<int> a = {};
	UT_VERIFY(a.empty());
}

UNIT_TEST("ImmutableVector.h", "immutable_vector::immutable_vector()", "3 items", "operator[] can give contents"){
	const immutable_vector<int> a = { 91, 92, 93 };
	UT_VERIFY(a[0] == 91);
	UT_VERIFY(a[1] == 92);
	UT_VERIFY(a[2] == 93);
}




////////////////////////		size()



UNIT_TEST("ImmutableVector.h", "size()", "empty", "0"){
	UT_VERIFY(immutable_vector<int>().empty());
}

UNIT_TEST("ImmutableVector.h", "size()", "3 items", "3"){
	const immutable_vector<int> a = { 4, 5, 6 };
	UT_VERIFY(a.size() == 3);
}


////////////////////////		data()



UNIT_TEST("ImmutableVector.h", "data()", "3 items", "3"){
	const immutable_vector<int> a = { 4, 5, 6 };
	UT_VERIFY(a.data() != nullptr);
	UT_VERIFY(a.data()[0] == 4);
	UT_VERIFY(a.data()[1] == 5);
	UT_VERIFY(a.data()[2] == 6);
}

UNIT_TEST("ImmutableVector.h", "data()", "empty", "nullptr"){
	const immutable_vector<int> a;
	UT_VERIFY(a.data() == nullptr);
}



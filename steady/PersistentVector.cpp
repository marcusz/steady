//
//  PersistentVector.cpp
//  Perma App
//
//  Created by Marcus Zetterquist on 2014-03-09.
//  Copyright (c) 2014 Marcus Zetterquist. All rights reserved.
//

#include "PersistentVector.h"


#include <vector>
#include <initializer_list>
#include "cpp_extension.h"


int calc_depth(std::size_t max_index){
	if(max_index == 0){
		return 0;
	}
	else{
		return 1 + calc_depth(max_index >> 5);
	}
}

UNIT_TEST("ImmutableVector.h", "calc_depth()", "", ""){
	UT_VERIFY(calc_depth(0) == 0);

	UT_VERIFY(calc_depth(1) == 1);
	UT_VERIFY(calc_depth(31) == 1);

	UT_VERIFY(calc_depth(32) == 2);
	UT_VERIFY(calc_depth(33) == 2);
	UT_VERIFY(calc_depth(1023) == 2);

	UT_VERIFY(calc_depth(1024) == 3);
	UT_VERIFY(calc_depth(1025) == 3);
	UT_VERIFY(calc_depth(32767) == 3);

	UT_VERIFY(calc_depth(32768) == 4);
}




UNIT_TEST("ImmutableVector.h", "pvector::pvector()", "Most basic construction", "empty() == true"){
	const pvector<int> a;
	UT_VERIFY(a.empty());
}




UNIT_TEST("ImmutableVector.h", "pvector::pvector()", "empty", "empty() == true"){
	const pvector<int> a({});
	UT_VERIFY(a.empty());
}

#if 0
UNIT_TEST("ImmutableVector.h", "immutable_vector::immutable_vector()", "3 items", "operator[] can give contents"){
	const pvector<int> a({ 91, 92, 93 });
	UT_VERIFY(a[0] == 91);
	UT_VERIFY(a[1] == 92);
	UT_VERIFY(a[2] == 93);
}



UNIT_TEST("ImmutableVector.h", "immutable_vector::push_back()", "Add 98 to empty vector", "operator[] can read-back"){
	const pvector<int> a;
	pvector<int> b = a.push_back(98);

	UT_VERIFY(b[0] == 98);
}


UNIT_TEST("ImmutableVector.h", "immutable_vector::push_back()", "Add 98, 99, 100 to empty vector", "operator[] can read-back"){
	const pvector<int> a;
	pvector<int> b = a.push_back(98);
	b = b.push_back(99);
	b = b.push_back(100);

	UT_VERIFY(b[0] == 98);
	UT_VERIFY(b[1] == 99);
	UT_VERIFY(b[2] == 100);
}
#endif


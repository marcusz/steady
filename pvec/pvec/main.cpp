//
//  main.cpp
//  pvec
//
//  Created by Marcus Zetterquist on 2014-10-08.
//  Copyright (c) 2014 Marcus Zetterquist AB. All rights reserved.
//

#include <iostream>

#include "cpp_extension.h"

#include <array>



UNIT_TEST("std::array", "array::array()", "", "no assert"){
    std::array<int, 3> a = { 1, 2, 3};
}

UNIT_TEST("std::array", "array::size()", "3", "3"){
    std::array<int, 3> a = { 1, 2, 3};
	UT_VERIFY(a.size() == 3);
}

UNIT_TEST("std::array", "array::size()", "3", "3"){
    std::array<int, 3> a = { 1, 2, 3};
	UT_VERIFY(a[0] == 1);
	UT_VERIFY(a[1] == 2);
	UT_VERIFY(a[2] == 3);
}




/*
/////////////////////////////////////		pvec_array


template<typename T> class pvec_array {
	public: pvec_array();
	public: pvec_array(const T* start, const T* end);
	public: bool check_invariant() const;
	public: size_t size() const;


	////////////////////		State
		private: size_t _size;
		public: T* _array;
};

template<typename T> pvec_array<T>::pvec_array() :
	_size(0),
	_array(nullptr)
{

	ASSERT(check_invariant());
}

template<typename T> pvec_array<T>::pvec_array(const T* start, const T* end) :
	_size(end - start),
	_array(nullptr)
{
	ASSERT(start != nullptr);
	ASSERT(end != nullptr);
	ASSERT(end >= start);
	ASSERT(end - start <= 32);

	void* mem = std::calloc(_size, sizeof(T));
	_array = reinterpret_cast<T*>(mem);
	for(auto i = 0 ; i < _size ; i++){
		_array[i] = start[i];
	}

	ASSERT(check_invariant());
}


template<typename T> bool pvec_array<T>::check_invariant() const{
	ASSERT(_array == nullptr ? _size == 0 : _size >= 0 && _size <=32);

	return true;
}

template<typename T> size_t  pvec_array<T>::size() const{
	ASSERT(check_invariant());

	return _size;
}



UNIT_TEST("pvec_array", "pvec_array::pvec_array()", "", "no assert"){
	pvec_array<int> a;
}

UNIT_TEST("pvec_array", "pvec_array::pvec_array(start, end)", "3 integers", "no assert"){
	const int kTest[] { 1, 2, 3 };
	pvec_array<int> a(&kTest[0], &kTest[3]);
}

UNIT_TEST("pvec_array", "pvec_array::size()", "3 integers", "3"){
	const int kTest[] { 1, 2, 3 };
	pvec_array<int> a(&kTest[0], &kTest[3]);
	UT_VERIFY(a.size() == 3);
}

UNIT_TEST("pvec_array", "pvec_array::size()", "3 integers", "3"){
	const int kTest[] { 1, 2, 3 };
	pvec_array<int> a(&kTest[0], &kTest[3]);
	UT_VERIFY(a._array[0] == 1);
	UT_VERIFY(a._array[1] == 2);
	UT_VERIFY(a._array[2] == 3);
}
*/






/////////////////////////////////////		pvec



template<typename T> class pvec {
	public: pvec();
	public: bool check_invariant() const;

	////////////////////		State
		public: std::array<T, 32> _root;
};

template<typename T> pvec<T>::pvec(){

	ASSERT(check_invariant());
}

template<typename T> bool pvec<T>::check_invariant() const{
	return true;
}



UNIT_TEST("pvec", "pvec::pvec()", "", "no assert"){
	pvec<int> a;
}




/////////////////////////////////////		main()



int main(int argc, const char * argv[]){
	TDefaultRuntime runtime("");
	SetRuntime(&runtime);
	run_tests();

	try {
		TRACE("Hello, World 3!");
	}
	catch(...){
		TRACE("Error");
		return -1;
	}
}




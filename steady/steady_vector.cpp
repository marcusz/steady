//
//  steady_vector.cpp
//  steady
//
//  Created by Marcus Zetterquist on 2013-11-13.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#include "steady_vector.h"

#include "cpp_extension.h"

//#include <iostream>


//??? Immutable = don't deep copy!
//??? Exception safety pls!



template <class T>
steady_vector<T>::steady_vector() :
	_allocation(nullptr),
	_vector_count(0)
{
	ASSERT(check_invariant());
}


template <class T>
steady_vector<T>::steady_vector(const T entries[], size_t count) :
	_allocation(nullptr),
	_vector_count(0)
{
	ASSERT(entries != nullptr);

	steady_vector temp;
	for(auto i = 0 ; i < count ; i++){
		temp = temp.push_back(entries[i]);
	}
	temp.swap(*this);

	ASSERT(check_invariant());
}


template <class T>
steady_vector<T>::steady_vector(std::initializer_list<T> args) :
	_allocation(nullptr),
	_vector_count(0)
{
	steady_vector temp;
	for(auto i: args){
		temp = temp.push_back(i);
	}
	temp.swap(*this);

	ASSERT(check_invariant());
}


template <class T>
steady_vector<T>::~steady_vector(){
	ASSERT(check_invariant());

	delete[] _allocation;
	_allocation = NULL;
	_vector_count = 0;
}


template <class T>
bool steady_vector<T>::check_invariant() const{
	if(_allocation == NULL){
		ASSERT(_vector_count == 0);
	}
	else{
		ASSERT(_vector_count >= 0);
	}

	return true;
}


template <class T>
steady_vector<T>::steady_vector(const steady_vector& rhs)
:
	_allocation(NULL),
	_vector_count(0)
{
	ASSERT(rhs.check_invariant());

	if(rhs._vector_count > 0){
		_allocation = new T[rhs._vector_count];
		_vector_count = rhs._vector_count;

		for(std::size_t i = 0 ; i < _vector_count ; i++){
			_allocation[i] = rhs._allocation[i];
		}
	}

	ASSERT(check_invariant());
}


template <class T>
steady_vector<T>& steady_vector<T>::operator=(const steady_vector& rhs){
	ASSERT(check_invariant());
	ASSERT(rhs.check_invariant());

	steady_vector<T> temp(rhs);
	temp.swap(*this);

	ASSERT(check_invariant());
	return *this;
}

template <class T>
void steady_vector<T>::swap(steady_vector& rhs){
	ASSERT(check_invariant());
	ASSERT(rhs.check_invariant());

	std::swap(_allocation, rhs._allocation);
	std::swap(_vector_count, rhs._vector_count);

	ASSERT(check_invariant());
	ASSERT(rhs.check_invariant());
}



template <class T>
steady_vector<T> steady_vector<T>::push_back(const T& entry) const{
	ASSERT(check_invariant());

	std::size_t new_count = _vector_count + 1;

	steady_vector<T> result;
	result._allocation = new T[new_count];
	result._vector_count = new_count;

	for(std::size_t i = 0 ; i < _vector_count ; i++){
		result._allocation[i] = _allocation[i];
	}
	result._allocation[_vector_count] = entry;

	ASSERT(result.check_invariant());

	return result;
}


template <class T>
steady_vector<T> steady_vector<T>::update(size_t index, const T& entry) const{
	ASSERT(check_invariant());
	ASSERT(index < _vector_count);
	
	std::size_t new_count = _vector_count + 1;

	steady_vector<T> result;
	result._allocation = new T[new_count];
	result._vector_count = new_count;

	for(std::size_t i = 0 ; i < _vector_count ; i++){
		result._allocation[i] = _allocation[i];
	}
	result._allocation[_vector_count] = entry;

	ASSERT(check_invariant());

	return result;
}


template <class T>
std::size_t steady_vector<T>::size() const{
	ASSERT(check_invariant());

	return _vector_count;
}




template <class T>
const T& steady_vector<T>::get_at(const std::size_t index) const{
	ASSERT(check_invariant());
	ASSERT(index < _vector_count);

	return _allocation[index];
}





template <class T>
std::vector<T> steady_vector<T>::to_vec() const{
	ASSERT(check_invariant());

	std::vector<T> a;
	for(size_t i = 0 ; i < size() ; i++){
		a.push_back(get_at(i));
	}

	return a;
}








////////////////////////////////////////////			Unit tests








struct T5ItemFixture {
	T5ItemFixture(){
//		_v = steady_vector<int>({	500, 501, 502, 504, 505 });
	}


	/////////////////////		State
		const steady_vector<int> _v = {	500, 501, 502, 504, 505 };
};



struct T34ItemFixture {
	T34ItemFixture(){
		for(int i = 0 ; i < 34 ; i++){
			_v = _v.push_back(i * 3);
		}
	}


	/////////////////////		State
		steady_vector<int> _v;
};



UNIT_TEST("steady_vector", "steady_vector()", "", "no_assert"){
	steady_vector<int> v;
}

UNIT_TEST("steady_vector", "push_back()", "", "read back"){
	steady_vector<int> v;
	v = v.push_back(4);
	TEST_VERIFY(v[0] == 4);
}

UNIT_TEST("steady_vector", "push_back()", "vector already has 1 item", "read back both"){
	steady_vector<int> v;
	v = v.push_back(4);
	v = v.push_back(9);

	TEST_VERIFY(v[0] == 4);
	TEST_VERIFY(v[1] == 9);
}

UNIT_TEST("steady_vector", "push_back()", "add 1000 items", "read back all items"){
	steady_vector<int> v;
	for(int i = 0 ; i < 1000 ; i++){
		v = v.push_back(i * 3);
	}

	for(int i = 0 ; i < 1000 ; i++){
		TEST_VERIFY(v[i] == i * 3);
	}
}

UNIT_TEST("steady_vector", "size()", "empty vector", "0"){
	steady_vector<int> v;
	TEST_VERIFY(v.size() == 0);
}

UNIT_TEST("steady_vector", "size()", "34 item vector", "34"){
	T34ItemFixture f;
	TEST_VERIFY(f._v.size() == 34);
}

UNIT_TEST("steady_vector", "swap()", "34 vs empty", "empty vs 34"){
	const T34ItemFixture f;
	steady_vector<int> v1 = f._v;
	steady_vector<int> v2;
	v1.swap(v2);
	TEST_VERIFY(v1.size() == 0);
	TEST_VERIFY(v2.size() == 34);
}



UNIT_TEST("steady_vector", "steady_vector(const T entries[], size_t count)", "0 items", "empty"){
	const int a[] = {};
	steady_vector<int> v(&a[0], 0);
	TEST_VERIFY(v.size() == 0);
}


UNIT_TEST("steady_vector", "steady_vector(const T entries[], size_t count)", "7 items", "read back all"){
	const int a[] = {	3, 4, 5, 6, 7, 8, 9	};
	steady_vector<int> v(&a[0], 7);
	TEST_VERIFY(v.size() == 7);
	TEST_VERIFY(v[0] == 3);
	TEST_VERIFY(v[1] == 4);
	TEST_VERIFY(v[2] == 5);
	TEST_VERIFY(v[3] == 6);
	TEST_VERIFY(v[4] == 7);
	TEST_VERIFY(v[5] == 8);
	TEST_VERIFY(v[6] == 9);
}





UNIT_TEST("steady_vector", "steady_vector(std::initializer_list<T> args)", "0 items", "empty"){
	steady_vector<int> v = {};

	TEST_VERIFY(v.size() == 0);
}


UNIT_TEST("steady_vector", "steady_vector(std::initializer_list<T> args)", "7 items", "read back all"){
	steady_vector<int> v = {	3, 4, 5, 6, 7, 8, 9	};

	TEST_VERIFY(v.size() == 7);
	TEST_VERIFY(v[0] == 3);
	TEST_VERIFY(v[1] == 4);
	TEST_VERIFY(v[2] == 5);
	TEST_VERIFY(v[3] == 6);
	TEST_VERIFY(v[4] == 7);
	TEST_VERIFY(v[5] == 8);
	TEST_VERIFY(v[6] == 9);
}





#if 0

UNIT_TEST("steady_vector", "update()", "Update [0]", "Read back"){
	T5ItemFixture f;

	const auto b = f._v.update(0, 127);
	TEST_VERIFY(b.size() == 5);
	TEST_VERIFY(b[0] == 500);
	TEST_VERIFY(b[1] == 501);
	TEST_VERIFY(b[2] == 502);
	TEST_VERIFY(b[3] == 503);
	TEST_VERIFY(b[4] == 504);
}
#endif



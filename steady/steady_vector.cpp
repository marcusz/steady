//
//  steady_vector.cpp
//  steady
//
//  Created by Marcus Zetterquist on 2013-11-13.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#include "steady_vector.h"

#include "cpp_extension.h"

#include <iostream>



template <class T>
steady_vector<T>::steady_vector() :
	_allocation(NULL),
	_vector_count(0)
{
	check_invariant();
}

template <class T>
steady_vector<T>::~steady_vector(){
	check_invariant();

	delete[] _allocation;
	_allocation = NULL;
	_vector_count = 0;
}


template <class T>
void steady_vector<T>::check_invariant() const{
	if(_allocation == NULL){
		ASSERT(_vector_count == 0);
	}
	else{
		ASSERT(_vector_count >= 0);
	}
}

template <class T>
const T& steady_vector<T>::operator[](const std::size_t index) const{
	check_invariant();
	ASSERT(index < _vector_count);

	return _allocation[index];
}




//??? Immutable = don't deep copy!
//??? Exception safety pls!

template <class T>
steady_vector<T>::steady_vector(const steady_vector& rhs)
:
	_allocation(NULL),
	_vector_count(0)
{
	rhs.check_invariant();

	if(rhs._vector_count > 0){
		_allocation = new T[rhs._vector_count];
		_vector_count = rhs._vector_count;

		for(std::size_t i = 0 ; i < _vector_count ; i++){
			_allocation[i] = rhs._allocation[i];
		}
	}

	check_invariant();
}


template <class T>
steady_vector<T>& steady_vector<T>::operator=(const steady_vector& rhs){
	check_invariant();
	rhs.check_invariant();

	steady_vector<T> temp(rhs);
	temp.swap(*this);

	check_invariant();
	return *this;
}

template <class T>
void steady_vector<T>::swap(steady_vector& rhs){
	check_invariant();
	rhs.check_invariant();

	std::swap(_allocation, rhs._allocation);
	std::swap(_vector_count, rhs._vector_count);

	check_invariant();
	rhs.check_invariant();
}



template <class T>
steady_vector<T> steady_vector<T>::push_back(const T& entry) const{
	check_invariant();

	std::size_t new_count = _vector_count + 1;

	steady_vector<T> result;
	result._allocation = new T[new_count];
	result._vector_count = new_count;

	for(std::size_t i = 0 ; i < _vector_count ; i++){
		result._allocation[i] = _allocation[i];
	}
	result._allocation[_vector_count] = entry;

	return result;
}

template <class T>
std::size_t steady_vector<T>::size() const{
	check_invariant();
	return _vector_count;
}




/////////////////////////////////////			Unit tests





struct T300ItemFixture {
	T300ItemFixture(){
		for(int i = 0 ; i < 300 ; i++){
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

UNIT_TEST("steady_vector", "size()", "300 item vector", "300"){
	T300ItemFixture f;
	TEST_VERIFY(f._v.size() == 300);
}



UNIT_TEST("steady_vector", "swap()", "300 vs empty", "empty vs 300"){
	const T300ItemFixture f;
	steady_vector<int> v1 = f._v;
	steady_vector<int> v2;
	v1.swap(v2);
	TEST_VERIFY(v1.size() == 0);
	TEST_VERIFY(v2.size() == 300);
}


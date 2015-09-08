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
	_root(nullptr),
	_size(0)
{
	ASSERT(check_invariant());
}

template <class T>
steady_vector<T>::steady_vector(const std::vector<T>& vec) :
	_root(nullptr),
	_size(0)
{
	//	!!! Illegal to take adress of first element of vec if it's empty.
	if(!vec.empty()){
		steady_vector<T> temp2(&vec[0], vec.size());
		temp2.swap(*this);
	}

	ASSERT(check_invariant());
}


template <class T>
steady_vector<T>::steady_vector(const T entries[], size_t count) :
	_root(nullptr),
	_size(0)
{
	ASSERT(entries != nullptr);

	T* root = new T[count];
	for(size_t i = 0 ; i < count ; i++){
		root[i] = entries[i];
	}

	steady_vector temp;
	temp._root = root;
	temp._size = count;

	temp.swap(*this);

	ASSERT(check_invariant());
}


template <class T>
steady_vector<T>::steady_vector(std::initializer_list<T> args) :
	_root(nullptr),
	_size(0)
{
	std::vector<T> temp;
	for(auto i: args){
		temp.push_back(i);
	}

	steady_vector<T> temp2 = temp;
	temp2.swap(*this);

	ASSERT(check_invariant());
}


template <class T>
steady_vector<T>::~steady_vector(){
	ASSERT(check_invariant());

	delete[] _root;
	_root = NULL;
	_size = 0;
}


template <class T>
bool steady_vector<T>::check_invariant() const{
	if(_root == NULL){
		ASSERT(_size == 0);
	}
	else{
		ASSERT(_size >= 0);
	}

	return true;
}


template <class T>
steady_vector<T>::steady_vector(const steady_vector& rhs)
:
	_root(NULL),
	_size(0)
{
	ASSERT(rhs.check_invariant());

	if(!rhs.empty()){
		steady_vector<T> temp = rhs.to_vec();
		temp.swap(*this);
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

	std::swap(_root, rhs._root);
	std::swap(_size, rhs._size);

	ASSERT(check_invariant());
	ASSERT(rhs.check_invariant());
}



template <class T>
steady_vector<T> steady_vector<T>::push_back(const T& v) const{
	ASSERT(check_invariant());

	T* root = new T[_size + 1];
	for(size_t i = 0 ; i < _size ; i++){
		root[i] = _root[i];
	}
	root[_size] = v;

	steady_vector<T> result;
	result._root = root;
	result._size = _size + 1;
	ASSERT(result.check_invariant());

	return result;
}

#if 0
template <class T>
steady_vector<T> steady_vector<T>::update(size_t index, const T& entry) const{
	ASSERT(check_invariant());
	ASSERT(index < _size);
	
	std::size_t new_count = _size + 1;

	steady_vector<T> result;
	result._allocation = new T[new_count];
	result._size = new_count;

	for(std::size_t i = 0 ; i < _size ; i++){
		result._allocation[i] = _allocation[i];
	}
	result._allocation[_size] = entry;

	ASSERT(check_invariant());

	return result;
}
#endif


template <class T>
std::size_t steady_vector<T>::size() const{
	ASSERT(check_invariant());

	return _size;
}




template <class T>
const T& steady_vector<T>::get_at(const std::size_t index) const{
	ASSERT(check_invariant());
	ASSERT(index < _size);

	return _root[index];
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





UNIT_TEST("steady_vector", "steady_vector()", "", "no_assert"){
	steady_vector<int> v;
}

UNIT_TEST("steady_vector", "push_back()", "one item", "read back"){
	const steady_vector<int> a;
	const auto b = a.push_back(4);
	TEST_VERIFY(a.size() == 0);
	TEST_VERIFY(b.size() == 1);
	TEST_VERIFY(b[0] == 4);
}

UNIT_TEST("steady_vector", "push_back()", "two items", "read back both"){
	const steady_vector<int> a;
	const auto b = a.push_back(4);
	const auto c = b.push_back(9);

	TEST_VERIFY(a.size() == 0);

	TEST_VERIFY(b.size() == 1);
	TEST_VERIFY(b[0] == 4);

	TEST_VERIFY(c.size() == 2);
	TEST_VERIFY(c[0] == 4);
	TEST_VERIFY(c[1] == 9);
}

UNIT_TEST("steady_vector", "push_back()", "two items", "read back both"){
	const steady_vector<int> a;
	const auto b = a.push_back(4);
	const auto c = b.push_back(9);

	TEST_VERIFY(a.size() == 0);

	TEST_VERIFY(b.size() == 1);
	TEST_VERIFY(b[0] == 4);

	TEST_VERIFY(c.size() == 2);
	TEST_VERIFY(c[0] == 4);
	TEST_VERIFY(c[1] == 9);
}


UNIT_TEST("steady_vector", "size()", "empty vector", "0"){
	steady_vector<int> v;
	TEST_VERIFY(v.size() == 0);
}






UNIT_TEST("steady_vector", "steady_vector(const std::vector<T>& vec)", "0 items", "empty"){
	const std::vector<int> a = {};
	steady_vector<int> v(a);
	TEST_VERIFY(v.size() == 0);
}


UNIT_TEST("steady_vector", "steady_vector(const std::vector<T>& vec)", "7 items", "read back all"){
	const std::vector<int> a = {	3, 4, 5, 6, 7, 8, 9	};
	steady_vector<int> v(a);
	TEST_VERIFY(v.size() == 7);
	TEST_VERIFY(v[0] == 3);
	TEST_VERIFY(v[1] == 4);
	TEST_VERIFY(v[2] == 5);
	TEST_VERIFY(v[3] == 6);
	TEST_VERIFY(v[4] == 7);
	TEST_VERIFY(v[5] == 8);
	TEST_VERIFY(v[6] == 9);
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



UNIT_TEST("steady_vector", "push_back()", "add 1000 items", "read back all items"){
	steady_vector<int> v;
	for(int i = 0 ; i < 1000 ; i++){
		v = v.push_back(i * 3);
	}

	for(int i = 0 ; i < 1000 ; i++){
		TEST_VERIFY(v[i] == i * 3);
	}
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



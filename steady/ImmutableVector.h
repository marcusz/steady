//
//  ImmutableVector.h
//  Permafrost
//
//  Created by Marcus Zetterquist on 2014-01-21.
//  Copyright (c) 2014 Marcus Zetterquist. All rights reserved.
//

#ifndef __Permafrost__ImmutableVector__
#define __Permafrost__ImmutableVector__

#include <vector>
#include <initializer_list>
#include "cpp_extension.h"

///////////////////////////		immutable_vector





template <typename T>
class immutable_vector {
	//	Constructor #1
	public: immutable_vector(){
		ASSERT(check_invariant());
	}

	//	Constructor #2
	public: immutable_vector(std::size_t count, const T& v) :
		_vector(count, v)
	{
		ASSERT(check_invariant());
	}

	//	Constructor #3
	public: immutable_vector(const std::vector<T>& i) :
		_vector(i)
	{
		ASSERT(check_invariant());
	}

	//	Constructor #4
	public: immutable_vector(const immutable_vector& iOther) :
		_vector(iOther._vector)
	{
		ASSERT(check_invariant());
	}
		
	//	Constructor #5
	public: immutable_vector(std::initializer_list<T> args) :
		_vector(args)
	{
		ASSERT(check_invariant());
	}

	//	Constructor #6
	public: immutable_vector(const T* iStart, const T* iEnd) :
		_vector(iStart, iEnd)
	{
		ASSERT(check_invariant());
	}
		

	public: ~immutable_vector(){
		ASSERT(check_invariant());
	}

	public: bool check_invariant() const{
		ASSERT(this != nullptr);
		return true;
	}

	public: const T& operator[](size_t i) const{
		ASSERT(check_invariant());
		ASSERT(i < _vector.size());

		return _vector[i];
	}

	public: std::size_t size() const {
		ASSERT(check_invariant());

		return _vector.size();
	}

	public: const T* data() const {
		ASSERT(check_invariant());

		return _vector.data();
	}

	public: bool empty() const {
		ASSERT(check_invariant());

		return _vector.empty();
	}

	public: bool operator==(const immutable_vector& iOther) const {
		ASSERT(check_invariant());

		return _vector == iOther._vector;
	}

	public: bool operator!=(const immutable_vector& iOther) const {
		return !(*this == iOther);
	}

	public: typename std::vector<T>::const_iterator begin() const{
		ASSERT(check_invariant());

		return _vector.begin();
	}
	public: typename std::vector<T>::const_iterator end() const{
		ASSERT(check_invariant());

		return _vector.end();
	}

	//////////////////		State
		private: std::vector<T> _vector;
};


/*
template <typename T>
class immutable_slice {
	immutable_vector<T>* _vector;
	typename immutable_vector<T>::size_type _start;
	typename immutable_vector<T>::size_type _end;
};
*/


#endif /* defined(__Permafrost__ImmutableVector__) */

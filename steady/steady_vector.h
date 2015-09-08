//
//  steady_vector.h
//  steady
//
//  Created by Marcus Zetterquist on 2013-11-13.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#ifndef __steady__steady_vector__
#define __steady__steady_vector__

#include <cstddef>
#include <initializer_list>
#include <vector>

template <class T>
struct array {
};



/**
Persistent
Templetized
UT
Ranges, not iterators
Strong exception safety
Thread safe

Hash-consing for global deduplication
*/


template <class T>
class steady_vector {
	public: steady_vector();
	public: steady_vector(const T entries[], size_t count);
	public: steady_vector(std::initializer_list<T> args);

	//public: steady_vector(T entries[], std::size_t count);
	public: ~steady_vector();
	public: bool check_invariant() const;

	public: steady_vector(const steady_vector& rhs);
	public: steady_vector& operator=(const steady_vector& rhs);
	public: void swap(steady_vector& other);

	// ###	operator== and !=

	public: steady_vector push_back(const T& entry) const;
	public: steady_vector update(size_t index, const T& entry) const;
	public: std::size_t size() const;

	public: const T& operator[](const std::size_t index) const{
		return get_at(index);
	}

	public: const T& get_at(const std::size_t index) const;

	public: std::vector<T> to_vec() const;


	/////////////////		State
		private: T* _allocation;
		private: std::size_t _vector_count;
};

	
#endif /* defined(__steady__steady_vector__) */

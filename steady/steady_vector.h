//
//  steady_vector.h
//  steady
//
//  Created by Marcus Zetterquist on 2013-11-13.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#ifndef __steady__steady_vector__
#define __steady__steady_vector__

#include "cstddef"

template <class T>
class steady_vector {
	public: steady_vector();
	//public: steady_vector(T entries[], std::size_t count);
	public: ~steady_vector();
	public: void check_invariant() const;

	public: steady_vector(const steady_vector& rhs);
	public: steady_vector& operator=(const steady_vector& rhs);
	public: void swap(steady_vector& other);

	public: steady_vector push_back(const T& entry) const;
	public: std::size_t size() const;

	public: const T& operator[](const std::size_t index) const;


	/////////////////		State
		private: T* _allocation;
		private: std::size_t _vector_count;
};

	
#endif /* defined(__steady__steady_vector__) */

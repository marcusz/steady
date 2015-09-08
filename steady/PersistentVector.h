//
//  PersistentVector.h
//  Perma App
//
//  Created by Marcus Zetterquist on 2014-03-09.
//  Copyright (c) 2014 Marcus Zetterquist. All rights reserved.
//

#ifndef __Perma_App__PersistentVector__
#define __Perma_App__PersistentVector__


#include "cpp_extension.h"



/**
https://github.com/chaelim/HAMT
*/

const int kBranchingFactor = 32;





///////////////////////////////////			pvector_leaf_node


/*
	Child arrays are uncompressed - change to use std::vector<>.
??? in which scenarios is pvector better than immutable_vector?
	A) push_back() needs to copy entire vector everytime. push_back()-loop = slow.


Insight: a vector is nevern sparse (HAMT is sparse) so has less / no need child-bits!
*/


/*
### Other concept: do a content-addressable design 

*/



template <typename T>
struct node {
	public: enum class EType {
		kINode = 4,
		kLeafNode
	};

	public: node() :
		_type(EType::kLeafNode),
		_rc(0),
		_child_bits(0x00000000)
	{
	}

	public: void swap(node& other){
		std::swap(_type, other._type);
		std::swap(_rc, other._rc);
		std::swap(_child_bits, other._child_bits);

		std::swap(_children, other._children);
	}

	////////////////////		State.
		EType _type;
		long _rc;
		uint32_t _child_bits;

		//	One entry for each used child.
		union EChildren {
			T _values[kBranchingFactor];
			T _subnodes[kBranchingFactor];
		};
		EChildren _children;
};



///////////////////////////////////			pvector


/**
	32-bit addressindex, split into 5-bit chunks - one chunk for each level in the tree.
	If indexes go from 0-31, only a single level (5 bits address) is needed etc.

	level:		11    10    9     8     7     6     5     4     3     2     1
	bits:		---00 00000 00000 00000 00000 00000 00000 00000 00000 00000 00000

	??? Need to handle level 11 too, even though it has only 2 bits address.
*/

/**
	Returns the required depth of the tree from the max-index of the array. Use (size - 1) for max_index.
*/
int calc_depth(std::size_t max_index);

template <typename T>
class pvector {
	public: pvector() :
		_size(0)
	{
	}

	public: pvector(std::initializer_list<T> args) :
		_size(0)
	{
		pvector temp;
		for(auto i: args){
			temp.push_back(i);
		}

		temp.swap(*this);

		ASSERT(check_invariant());
	}

	public: ~pvector(){
	}

	public: void swap(pvector& other){
		std::swap(_size, other._size);
		_root.swap(other._root);
	}

	public: bool check_invariant() const{
		return true;
	}

	public: const T& operator[](size_t i) const{
		ASSERT(check_invariant());
		ASSERT(i < _size);

#if 0
		const int depth = calc_depth(_size - 1);
		const node<T>* n = &_root;

		long level = depth;
		size_t levelMask = 31 << (depth - 1);
		const int skipLevels = depth - 1;
		for(int a = 0 ; a < skipLevels ; a++){
			n = n->_children[i & ];
			levelMask = levelMask >> 5;
			level++;
		}
		return n->_children._values[top5];
#endif
		return T();
	}

	public: std::size_t size() const {
		ASSERT(check_invariant());

		return _size;
	}

	public: bool empty() const {
		ASSERT(check_invariant());

		return _size == 0;
	}

	public: pvector push_back(const T& iValue) const {
		ASSERT(check_invariant());

		pvector temp = *this;
		temp.mutable_push_back(iValue);
		return temp;
	}

	public: void mutable_push_back(const T& iValue){
		ASSERT(check_invariant());

//		mutable_push_back(iValue);
	}


	//////////////////////////		State.

	//	private: std::vector<T> _vector;
		private: node<T> _root;
		private: std::size_t _size;
};



#endif /* defined(__Perma_App__PersistentVector__) */

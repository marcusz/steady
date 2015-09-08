//
//  CAPersistentVector.h
//  Perma App
//
//  Created by Marcus Zetterquist on 2014-03-10.
//  Copyright (c) 2014 Marcus Zetterquist. All rights reserved.
//

#ifndef __Perma_App__CAPersistentVector__
#define __Perma_App__CAPersistentVector__


#include "cpp_extension.h"

#include <cmath>
namespace NSAPersistentVector {

/*
### Other concept: do a content-addressable design 

Address all nodes using 32-bit hashes.
Keep heap of read-only leaf-nodes containing arrays of T.
Heap also contains inodes referencing other nodes.
*/

typedef uint32_t ref_count;


/*
typedef uint32_t hash32;

template <typename T>
hash32 CalcHash(const T& v);

template <typename T>
hash32 CalcHash(const T values[], long count);

template <typename T>
hash32 CalcHash(const node<T>* subnodes[], long count);
*/


/**
_type == 0x88 = leaf node
_type == 0x99 = inode
*/


#if 000

int size_to_depth(std::size_t size);



/////////////////////////////////////////		leaf_node



template <typename T>
struct node {
	node(uint32_t type) :
		_type(type)
	{
	}
	bool check_invariant() const{
		ASSERT(this != nullptr);
		return true;
	}

	////////////////////		State
		const uint32_t _type;
};


/////////////////////////////////////////		leaf_node


template <typename T>
struct leaf_node : public node<T> {
	leaf_node(const T values[32], const uint8_t count) :
		node<T>(0x88),
		_count(count)
	{
		std::copy_n(values, count, _values);
		std::fill(_values + count, _values + 32, 0);
	}


	/////////////////		State.
		T _values[32];
		const uint8_t _count;
};


/////////////////////////////////////////		inode


template <typename T>
struct inode : public node<T> {
	inode(const node<T>* subnodes[32], const uint8_t count) :
		node<T>(0x99),
		_count(0)
	{
		std::copy_n(subnodes, count, _subnodes);
		std::fill(_subnodes + count, _subnodes + 32, nullptr);
	}


	/////////////////		State.
		const node<T>* _subnodes[32];
		const uint8_t _count;
};


/////////////////////////////////////////		pool


template <typename T>
class pool {
	public: static bool check_invariant(){
		for(auto i: _leaf_nodes){
			ASSERT(i.second > 0);
		}
		for(auto i: _inodes){
			ASSERT(i.second > 0);
		}
		return true;
	}

	public: static const leaf_node<T>* allocate_leaf_node(const T values[], long count){
		ASSERT(check_invariant());
		ASSERT(values != nullptr);
		ASSERT(count >=0 && count <= 32);

		leaf_node<T>* temp = new leaf_node<T>(values, count);
		return temp;
//		temp->_refcount = 1;

		ASSERT(check_invariant());
	}

	public: static const inode<T>* allocate_inode(const node<T>* subnodes[], long count){
		ASSERT(check_invariant());
		ASSERT(subnodes != nullptr);
		ASSERT(count >=0 && count < 32);

		inode<T>* temp = new inode<T>(subnodes, count);
//		temp->_refcount = 1;
		return temp;

		ASSERT(check_invariant());
	}

	public: static void add_ref(const node<T>& node){
		ASSERT(check_invariant());

		ASSERT(check_invariant());
	}

	public: static void release_ref(const node<T>& node){
		ASSERT(check_invariant());

		ASSERT(check_invariant());
	}


	///////////////////		State.
		static std::vector<std::pair<leaf_node<T>, ref_count> > _leaf_nodes;
		static std::vector<std::pair<inode<T>, ref_count> > _inodes;
};


/////////////////////////////////////////		pvector


template <typename T>
class pvector {

	/**
		Will increment reference counter on the root node.
	*/
	public: pvector(const node<T>* root_node, std::size_t size) :
		_size(size),
		_root_node(root_node),
		_inodeLevels(size_to_depth(_size) - 1)
	{
		if(_root_node != nullptr){
			pool<T>::add_ref(*_root_node);
		}

		ASSERT(check_invariant());
	}

	public: pvector() :
		_size(0),
		_root_node(nullptr),
		_inodeLevels(-1)
	{
		ASSERT(check_invariant());
	}

	public: pvector(const pvector<T>& other) :
		_size(other._size),
		_root_node(other._root_node),
		_inodeLevels(other._inodeLevels)
	{
		ASSERT(other.check_invariant());

		if(_root_node != nullptr){
			pool<T>::add_ref(*_root_node);
		}

		ASSERT(check_invariant());
	}

	public: pvector(std::initializer_list<T> args) :
		_size(0),
		_root_node(nullptr),
		_inodeLevels(-1)
	{
		ASSERT(check_invariant());

		pvector<T> result;
		for(auto i: args){
			result = result.push_back(i);
		}
		result.swap(*this);

		ASSERT(size() == args.size());
		ASSERT(check_invariant());
	}

	public: pvector<T>& operator=(const pvector<T>& other){
		ASSERT(check_invariant());
		ASSERT(other.check_invariant());

		pvector<T> temp = other;
		temp.swap(*this);
		return *this;
	}

	public: ~pvector(){
		ASSERT(check_invariant());

		if(_root_node != nullptr){
			pool<T>::release_ref(*_root_node);
//			_root_node = nullptr;
		}
	}

	public: bool check_invariant() const{
		if(_size == 0){
			ASSERT(_root_node == nullptr);
			ASSERT(_inodeLevels == -1);
			return true;
		}
		else{
			ASSERT(_root_node != nullptr);
			ASSERT(_inodeLevels >= 0);
			return true;
		}
	}

	public: void swap(pvector<T>& other){
		std::swap(_size, other._size);
		std::swap(_root_node, other._root_node);
		std::swap(_inodeLevels, other._inodeLevels);
	}

	public: bool empty() const{
		ASSERT(check_invariant());

		return _root_node ? false : true;
	}

	public: size_t size() const{
		ASSERT(check_invariant());

		return _size;
	}

//### Important its fast to copy partially full node. Variable lengt arrays with a single allocation per node?

	public: const T& operator[](size_t i) const{
		ASSERT(check_invariant());
		ASSERT(i < _size);

		const node<T>* n = _root_node;
		for(int a = 0 ; a < _inodeLevels ; a++){
			const inode<T>& inodeRef = as_inode(*n);
			const size_t shift = 5 * (1 + a);
			n = inodeRef._subnodes[(i >> shift) & 31];
		}

		const leaf_node<T>& leaf = as_leaf_node(*n);
		return leaf._values[i & 31];
	}

#if 1
	//	depth_count == 0 when a at topmost inode.
	private: static const node<T>* append_to_node(const node<T>& node, int depth_count, size_t pos, const T& iValue){
		ASSERT(node.check_invariant());
		ASSERT(depth_count >= 0);

		const size_t shift = 5 * (1 + depth_count);
		if(is_inode(node)){
			const inode<T>& inodeRef = as_inode(*n);
			const node<T>* subnodes[32];

			const inode<T>* n = inodeRef._subnodes[(pos >> shift) & 31];
			if(n != nullptr){
				std::copy_n(inodeRef._subnodes, pos, subnodes);
			}
			subnodes[pos] = iValue;
			const inode<T>* new_inode = pool<T>::allocate_inode(subnodes, pos);
			return append_to_node(*new_inode, depth_count + 1, pos, iValue);
		}
		else{
			const leaf_node<T>& leaf = as_leaf_node(*node);
			if(pos > 32){
			}
			else{
				const inode<T>* n = leaf._values[(pos >> shift) & 31];
				if(n == nullptr){
					new_node =
					return new_node;
				}
				else{
					new_node =
					return new_node;
				}
			}
		}

		if(depth == 1){
			ASSERT(is_leaf_node(node));
			const inode<T>& inodeRef = as_inode(*n);
			const size_t shift = 5 * (1 + a);
			n = inodeRef._subnodes[(i >> shift) & 31];
		}

		const leaf_node<T>& leaf = as_leaf_node(*n);
		return leaf._values[i & 31];


		if(depth == 1){
			ASSERT(node != nullptr);

			const leaf_node<T>& leaf = as_leaf_node(*node);
			T newValues[32];
			std::copy_n(leaf._values, size, newValues);
			newValues[size] = iValue;
			const leaf_node<T>* newNode = pool<T>::allocate_leaf_node(newValues, size + 1);
			return newNode;
		}
		else{
			ASSERT(node != nullptr);
/*
			const inode<T>& inodeRef = as_inode(*n);
			const size_t shift = 5 * (1 + a);
			n = inodeRef._subnodes[(i >> shift) & 31];
*/
			ASSERT(false);
		}
		return nullptr;
	}

	public: pvector<T> push_back(const T& iValue) const{
		if(_root_node == nullptr){
			const leaf_node<T>* new_leaf = pool<T>::allocate_leaf_node(&iValue, 1);
			return pvector<T>(new_leaf, 1);
		}
		else{
			const int depth = size_to_depth(_size);
			const node<T>* new_root = append_to_node(_root_node, depth, _size, iValue);
			return pvector<T>(new_root, _size + 1);
		}
	}
#endif

#if 0
	public: pvector<T> push_back(const T& iValue) const{
		ASSERT(check_invariant());

		const node<T>* n = _root_node;
		if(n == nullptr){
			const leaf_node<T>* node = pool<T>::allocate_leaf_node(&iValue, 1);
			return pvector(node, _size + 1);
		}
		else{
			if(_size < 32){
				const leaf_node<T>& prevNodeRef = as_leaf_node(*_root_node);

				T newValues[32];
				std::copy_n(prevNodeRef._values, _size, newValues);
				newValues[_size] = iValue;
				const leaf_node<T>* newNode = pool<T>::allocate_leaf_node(newValues, _size + 1);

				pool<T>::release_ref(*_root_node);
				return pvector(newNode, _size + 1);
			}
			else{
				ASSERT(false);
			}
		}

		ASSERT(check_invariant());
	}
#endif


	public: bool operator==(const pvector<T>& b) const{
		ASSERT(check_invariant());
		ASSERT(b.check_invariant());

		if(_size != b._size){
			return false;
		}

		if(_size == 0 && b._size == 0){
			return true;
		}

		if(_size <=32){
			const leaf_node<T>& leafA = as_leaf_node(*_root_node);
			const leaf_node<T>& leafB = as_leaf_node(*b._root_node);
			if(std::equal(leafA._values, leafA._values + _size, leafB._values)){
				return true;
			}
			else{
				return false;
			}
		}
		else{
			ASSERT(false);
		}
	}


	///////////////////		State.
		private: std::size_t _size;
		private: const node<T>* _root_node;
		private: int _inodeLevels;
};




template <typename T>
bool is_inode(const node<T>& node){
	return node._type == 0x99;
}

template <typename T>
bool is_leaf_node(const node<T>& node){
	return node._type == 0x88;
}


template <typename T>
const inode<T>& as_inode(const node<T>& node){
	ASSERT(is_inode(node));
	return static_cast<const inode<T>&>(node);
}

template <typename T>
const leaf_node<T>& as_leaf_node(const node<T>& node){
	ASSERT(is_leaf_node(node));
	return static_cast<const leaf_node<T>&>(node);
}





/**
http://clojure.org/data_structures

subvec
function
Usage: (subvec v start)
       (subvec v start end)
Returns a persistent vector of the items in vector from
start (inclusive) to end (exclusive).  If end is not supplied,
defaults to (count vector). This operation is O(1) and very fast, as
the resulting vector shares structure with the original and no
trimming is done.


assoc
function
Usage: (assoc map key val)
       (assoc map key val & kvs)
assoc[iate]. When applied to a map, returns a new map of the
same (hashed/sorted) type, that contains the mapping of key(s) to
val(s). When applied to a vector, returns a new vector that
contains val at index. Note - index must be <= (count vector).
Added in Clojure version 1.0
Source
*/

/**
	Usage: (conj coll x)
		   (conj coll x & xs)
	conj[oin]. Returns a new collection with the xs
	'added'. (conj nil item) returns (item).  The 'addition' may
	happen at different 'places' depending on the concrete type.
*/
template <typename T>
pvector<T> conj(const pvector<T>& v, const T& iValue){
	ASSERT(v.check_invariant());
	return v;
}


/**
	Returns a new seq where x is the first element and seq is
	the rest.
*/
template <typename T>
pvector<T> cons(const pvector<T>& v, const T& iValue){
	ASSERT(v.check_invariant());
	return v;
}


#endif


}	//	NSAPersistentVector


#endif /* defined(__Perma_App__CAPersistentVector__) */



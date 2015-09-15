//
//  steady_vector.h
//  steady
//
//  Created by Marcus Zetterquist on 2013-11-13.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#ifndef __steady__steady_vector__
#define __steady__steady_vector__

#include <initializer_list>
#include <atomic>
#include <vector>
#include "cpp_extension.h"


//	Change kBranchingFactorShift to get different branching factors. Number of bits per inode.
static const int kBranchingFactorShift = 3;

static const int kBranchingFactor = 1 << kBranchingFactorShift;
static const size_t kBranchingFactorMask = (kBranchingFactor - 1);

template <typename T> struct NodeRef;
template <typename T> struct INode;
template <typename T> struct LeafNode;

enum NodeType {
	kNullNode = 4,
	kInode,
	kLeafNode
};


////////////////////////////////////////////		NodeRef<T>


/*
	Safe, reference counted handle that wraps either an INode, a LeadNode or null.
*/


template <typename T>
struct NodeRef {
	public: NodeRef();

	//	Will assume ownership of the input node - caller must not delete it after call returns.
	//	Adds ref.
	//	node == nullptr => kNullNode
	public: NodeRef(INode<T>* node);

	//	Will assume ownership of the input node - caller must not delete it after call returns.
	//	Adds ref.
	//	node == nullptr => kNullNode
	public: NodeRef(LeafNode<T>* node);

	//	Uses reference counting to share all state.
	public: NodeRef(const NodeRef<T>& ref);

	public: ~NodeRef();

	public: bool check_invariant() const;
	public: void swap(NodeRef<T>& other);
	public: NodeRef<T>& operator=(const NodeRef<T>& other);
	

	///////////////////////////////////////		Internals
	public: NodeType GetType() const;


	///////////////////////////////////////		State

	public: INode<T>* _inode;
	public: LeafNode<T>* _leaf;
};



////////////////////////////////////////////		steady_vector



template <class T>
class steady_vector {
	public: steady_vector();
	public: steady_vector(const std::vector<T>& vec);
	public: steady_vector(const T entries[], size_t count);
	public: steady_vector(std::initializer_list<T> args);

	public: ~steady_vector();
	public: bool check_invariant() const;

	public: steady_vector(const steady_vector& rhs);
	public: steady_vector& operator=(const steady_vector& rhs);
	public: void swap(steady_vector& other);

	public: steady_vector assoc(size_t index, const T& value) const;
	public: steady_vector push_back(const T& value) const;

	public: steady_vector pop_back() const;

	public: bool operator==(const steady_vector& rhs) const;
	public: bool operator!=(const steady_vector& rhs) const{
		return !(*this == rhs);
	}

	public: std::size_t size() const;
	public: bool empty() const{
		return size() == 0;
	}

	public: T operator[](const std::size_t index) const;

	public: std::vector<T> to_vec() const;



	///////////////////////////////////////		Internals

	public: void trace_internals() const;

	public: const NodeRef<T>& GetRoot() const{
		return _root;
	}
	public: steady_vector(NodeRef<T> root, std::size_t size);



	///////////////////////////////////////		State

	private: NodeRef<T> _root;
	private: std::size_t _size;
};

	
#endif /* defined(__steady__steady_vector__) */

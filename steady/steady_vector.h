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
#include "cpp_extension.h"

template <class T>
struct array {
};

static const bool kCheckInvariant = false;


/**
Persistent
Templetized
UT
Ranges, not iterators
Strong exception safety
Thread safe

Hash-consing for global deduplication
*/


static const int kBranchingFactorShift = 3;

static const int kBranchingFactor = 1 << kBranchingFactorShift;
static const size_t kBranchingFactorMask = (kBranchingFactor - 1);



template <typename T>
struct NodeRef;


enum NodeType {
	kNullNode = 4,
	kInode,
	kLeafNode
};




////////////////////////////////////////////		LeafNode



/*
//	1 -> kBranchingFactor values.
	Sets its internal RC to 0 and never changes it.
*/


template <class T>
struct LeafNode {
	public: LeafNode() :
		_rc(0),
		_values(kBranchingFactor, T{})
	{

		_debug_count++;
		ASSERT(check_invariant());
	}

	//	values: 0 -> kBranchingFactor items.
	public: LeafNode(const std::vector<T>& values) :
		_rc(0),
		_values(values)
	{
		//	Expand to fixed number of values.
		_values.resize(kBranchingFactor, T{});

		_debug_count++;
		ASSERT(check_invariant());
	}

	public: ~LeafNode(){
		ASSERT(check_invariant());
		ASSERT(_rc == 0);

		_debug_count--;
	}

	public: bool check_invariant() const {
		ASSERT(_rc >= 0);
		ASSERT(_rc < 1000);
		ASSERT(_values.size() == kBranchingFactor);
		return true;
	}

	private: LeafNode<T>& operator=(const LeafNode& other);
	private: LeafNode(const LeafNode& other);



	//////////////////////////////	State

	public: int32_t _rc;
	public: std::vector<T> _values;
	public: static int _debug_count;
};



////////////////////////////////////////////		INode



namespace {

	template <class T>
	bool validate_inode_children(const std::vector<T>& vec){
		ASSERT(vec.size() >= 0);
		ASSERT(vec.size() <= kBranchingFactor);

		for(auto i: vec){
			i.check_invariant();
		}

		if(vec.size() > 0){
			const auto type = vec[0].GetType();
			if(type == kNullNode){
				for(auto i: vec){
					ASSERT(i.GetType() == kNullNode);
				}
			}
			else if(type == kInode){
				int i = 0;
				while(i < vec.size() && vec[i].GetType() == kInode){
					i++;
				}
				while(i < vec.size()){
					ASSERT(vec[i].GetType() == kNullNode);
					i++;
				}
			}
			else if(type == kLeafNode){
				int i = 0;
				while(i < vec.size() && vec[i].GetType() == kLeafNode){
					i++;
				}
				while(i < vec.size()){
					ASSERT(vec[i].GetType() == kNullNode);
					i++;
				}
			}
			else{
				ASSERT(false);
			}
		}
		return true;
	}

}


/*
	An INode has these states:
		32 iNode pointers or
		32 leaf node pointers or
		empty vector.

	You cannot mix inode and leaf nodes in the same INode.
	INode pointers and leaf node pointers can be null, but the nulls are always at the end of the arrays.

	Sets its internal RC to 0 and never changes it.
*/

template <class T>
struct INode {
	public: INode() :
		_rc(0)
	{
		_debug_count++;
		ASSERT(check_invariant());
	}

	//	children: 0-32 children, all of the same type. kNullNodes can only appear at end of vector.
	public: INode(const std::vector<NodeRef<T>>& children2) :
		_rc(0)
	{
		ASSERT(children2.size() >= 0);
		ASSERT(children2.size() <= kBranchingFactor);
#if DEBUG
		for(auto i: children2){
			i.check_invariant();
		}
#endif

		std::vector<NodeRef<T>> children = children2;
		children.resize(kBranchingFactor, NodeRef<T>());
		ASSERT(validate_inode_children(children));
		_children = children;

		_debug_count++;
		ASSERT(check_invariant());
	}

	public: ~INode(){
		ASSERT(check_invariant());
		ASSERT(_rc == 0);

		_debug_count--;
	}

	private: INode<T>& operator=(const INode& other);
	private: INode(const INode& other);

	public: bool check_invariant() const {
		ASSERT(_rc >= 0);
		ASSERT(_rc < 10000);
		ASSERT(validate_inode_children(_children));

		return true;
	}

	//	Counts
	public: size_t GetChildCountSkipNulls() const{
		ASSERT(check_invariant());

		size_t index = 0;
		while(index < _children.size() && _children[index].GetType() != kNullNode){
			index++;
		}
		return index;
	}

	public: std::vector<NodeRef<T>> GetChildrenWithNulls(){
		ASSERT(check_invariant());

		return _children;
	}

	public: NodeRef<T> GetChild(size_t index) const{
		ASSERT(check_invariant());
		ASSERT(index < kBranchingFactor);
		ASSERT(index < _children.size());
		return _children[index];
	}

	public: LeafNode<T>* GetChildLeafNode(size_t index){
		ASSERT(check_invariant());
		ASSERT(index < _children.size());

		ASSERT(_children[0].GetType() == kLeafNode);
		return _children[index]._leaf;
	}



	//////////////////////////////	State


	public: int32_t _rc;
	private: std::vector<NodeRef<T>> _children;

	public: static int _debug_count;
};





////////////////////////////////////////////		NodeRef<T>






template <typename T>
struct NodeRef {
	public: NodeRef() :
		_inode(nullptr),
		_leaf(nullptr)
	{
		ASSERT(check_invariant());
	}

	//	Will assume ownership of the input node - caller must not delete it after call returns.
	//	Adds ref.
	//	node == nullptr => kNullNode
	public: NodeRef(INode<T>* node) :
		_inode(nullptr),
		_leaf(nullptr)
	{
		if(node != nullptr){
			ASSERT(node->check_invariant());
			ASSERT(node->_rc >= 0);

			_inode = node;
			_inode->_rc++;
		}

		ASSERT(check_invariant());
	}

	//	Will assume ownership of the input node - caller must not delete it after call returns.
	//	Adds ref.
	//	node == nullptr => kNullNode
	public: NodeRef(LeafNode<T>* node) :
		_inode(nullptr),
		_leaf(nullptr)
	{
		if(node != nullptr){
			ASSERT(node->check_invariant());
			ASSERT(node->_rc >= 0);

			_leaf = node;
			_leaf->_rc++;
		}

		ASSERT(check_invariant());
	}

	//	Uses reference counting to share all state.
	public: NodeRef(const NodeRef<T>& ref) :
		_inode(nullptr),
		_leaf(nullptr)
	{
		ASSERT(ref.check_invariant());

		if(ref.GetType() == kNullNode){
		}
		else if(ref.GetType() == kInode){
			_inode = ref._inode;
			_inode->_rc++;
		}
		else if(ref.GetType() == kLeafNode){
			_leaf = ref._leaf;
			_leaf->_rc++;
		}
		else{
			ASSERT(false);
		}

		ASSERT(check_invariant());
	}

	public: ~NodeRef(){
		ASSERT(check_invariant());

		if(GetType() == kNullNode){
		}
		else if(GetType() == kInode){
			_inode->_rc--;
			if(_inode->_rc == 0){
				delete _inode;
				_inode = nullptr;
			}
		}
		else if(GetType() == kLeafNode){
			_leaf->_rc--;
			if(_leaf->_rc == 0){
				delete _leaf;
				_leaf = nullptr;
			}
		}
		else{
			ASSERT(false);
		}
	}

	public: bool check_invariant() const {
		ASSERT(_inode == nullptr || _leaf == nullptr);

		if(_inode != nullptr){
			ASSERT(_inode->check_invariant());
			ASSERT(_inode->_rc > 0);
		}
		else if(_leaf != nullptr){
			ASSERT(_leaf->check_invariant());
			ASSERT(_leaf->_rc > 0);
		}
		return true;
	}

	public: void swap(NodeRef<T>& other){
		ASSERT(check_invariant());
		ASSERT(other.check_invariant());

		std::swap(_inode, other._inode);
		std::swap(_leaf, other._leaf);

		ASSERT(check_invariant());
		ASSERT(other.check_invariant());
	}

	public: NodeRef<T>& operator=(const NodeRef<T>& other){
		ASSERT(check_invariant());
		ASSERT(other.check_invariant());

		NodeRef<T> temp(other);

		temp.swap(*this);

		ASSERT(check_invariant());
		ASSERT(other.check_invariant());
		return *this;
	}
	


	///////////////////////////////////////		Internals


	public: NodeType GetType() const {
		ASSERT(_inode == nullptr || _leaf == nullptr);

		if(_inode == nullptr && _leaf == nullptr){
			return kNullNode;
		}
		else if(_inode != nullptr){
			return kInode;
		}
		else if(_leaf != nullptr){
			return kLeafNode;
		}
		else{
			ASSERT_UNREACHABLE;
		}
	}



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

	// ###	operator== and !=

	public: steady_vector assoc(size_t index, const T& value) const;
	public: steady_vector push_back(const T& value) const;
	public: std::size_t size() const;
	public: bool empty() const{
		return size() == 0;
	}

	public: T operator[](const std::size_t index) const;

	public: std::vector<T> to_vec() const;

	public: void trace_internals() const;


	///////////////////////////////////////		Internals

	public: steady_vector(NodeRef<T> root, std::size_t size);



	///////////////////////////////////////		State
		public: NodeRef<T> _root;
		public: std::size_t _size;
};

	
#endif /* defined(__steady__steady_vector__) */

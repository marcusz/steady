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



/**
Persistent
Templetized
UT
Ranges, not iterators
Strong exception safety
Thread safe

Hash-consing for global deduplication
*/

static const size_t kBranchingFactor = 4;
static const size_t kBranchingFactorMask = 0x3;
static const size_t kBranchingFactorShift = 2;
static const size_t kBranchingFactorAntiMask = 0xffffffff - kBranchingFactorMask;


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
		_rc(0)
	{

		_debug_count++;
		ASSERT(check_invariant());
	}

	public: LeafNode(const std::vector<T>& values) :
		_rc(0),
		_values(values)
	{
		ASSERT(values.size() <= kBranchingFactor);

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
		ASSERT(_rc < 10000);
		ASSERT(_values.size() <= kBranchingFactor);
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



/*
	An INode points to up to 32 other iNodes alternatively 32 leaf nodes.
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


	public: INode(const std::vector<NodeRef<T>>& children) :
		_rc(0)
	{
		ASSERT(children.size() > 0);
		ASSERT(children.size() <= kBranchingFactor);

		const auto childrenType = children[0].GetType();
	#if DEBUG
		//	Make sure children are of the same type!
		{
			for(auto i: children){
				ASSERT(i.GetType() == childrenType);
			}
		}
	#endif

		if(childrenType == kInode){
			for(auto i: children){
				i._inode->_rc++;
				_child_inodes.push_back(i._inode);
			}
		}
		else if(childrenType == kLeafNode){
			for(auto i: children){
				i._leaf->_rc++;
				_child_leaf_nodes.push_back(i._leaf);
			}
		}
		else{
			ASSERT(false);
		}

		_debug_count++;
		ASSERT(check_invariant());
	}

	public: ~INode(){
		ASSERT(check_invariant());
		ASSERT(_rc == 0);

		_debug_count--;

		//	Decrement RC of all children.
		{
			//	Use NodeRef<> to delete them if RC becomes 0.
			auto childrenRefs = GetChildren();

			for(auto i: _child_inodes){
				i->_rc--;
			}
			_child_inodes.clear();

			for(auto i: _child_leaf_nodes){
				i->_rc--;
			}
			_child_leaf_nodes.clear();
		}
	}

	private: INode<T>& operator=(const INode& other);
	private: INode(const INode& other);

	public: bool check_invariant() const {
		ASSERT(_rc >= 0);
		ASSERT(_rc < 10000);

		//	_inodes OR _child_leaf_nodes is used.
		ASSERT(_child_inodes.empty() || _child_leaf_nodes.empty());
		ASSERT(_child_inodes.size() <= kBranchingFactor);
		ASSERT(_child_leaf_nodes.size() <= kBranchingFactor);
//		ASSERT(_children.size() <= kBranchingFactor);

		if(!_child_inodes.empty()){
			for(auto i: _child_inodes){
				ASSERT(i->check_invariant());
			}
		}
		else{
			for(auto i: _child_leaf_nodes){
				ASSERT(i->check_invariant());
			}
		}

		return true;
	}

	public: size_t GetChildCount() const{
		ASSERT(check_invariant());

		if(!_child_inodes.empty()){
			return _child_inodes.size();
		}
		else{
			return _child_leaf_nodes.size();
		}
	}

	public: std::vector<NodeRef<T>> GetChildren(){
		ASSERT(check_invariant());

		if(!_child_inodes.empty()){
			std::vector<NodeRef<T>> result;
			for(auto i: _child_inodes){
				result.push_back(NodeRef<T>(i));
			}
			return result;
		}
		else{
			std::vector<NodeRef<T>> result;
			for(auto i: _child_leaf_nodes){
				result.push_back(NodeRef<T>(i));
			}
			return result;
		}
	}

	public: NodeRef<T> GetChild(size_t index){
		ASSERT(check_invariant());

		if(!_child_inodes.empty()){
			return _child_inodes[index];
		}
		else{
			return _child_leaf_nodes[index];
		}
	}

	public: INode<T>* GetChildINode(size_t index){
		ASSERT(check_invariant());
		ASSERT(index < _child_inodes.size());

		return _child_inodes[index];
	}

	public: LeafNode<T>* GetChildLeafNode(size_t index){
		ASSERT(check_invariant());
		ASSERT(index < _child_leaf_nodes.size());

		return _child_leaf_nodes[index];
	}



	//////////////////////////////	State


	public: int32_t _rc;
	private: std::vector<INode<T>*> _child_inodes;
	private: std::vector<LeafNode<T>*> _child_leaf_nodes;

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
	public: NodeRef(INode<T>* node) :
		_inode(nullptr),
		_leaf(nullptr)
	{
		ASSERT(node != nullptr);
		ASSERT(node->_rc >= 0);
		ASSERT(node->check_invariant());

		_inode = node;
		_inode->_rc++;

		ASSERT(check_invariant());
	}

	//	Will assume ownership of the input node - caller must not delete it after call returns.
	//	Adds ref.
	public: NodeRef(LeafNode<T>* node) :
		_inode(nullptr),
		_leaf(nullptr)
	{
		ASSERT(node != nullptr);
		ASSERT(node->_rc >= 0);
		ASSERT(node->check_invariant());

		_leaf = node;
		_leaf->_rc++;

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
			ASSERT(_inode->_rc > 0);
			ASSERT(_inode->check_invariant());
		}
		else if(_leaf != nullptr){
			ASSERT(_leaf->_rc > 0);
			ASSERT(_leaf->check_invariant());
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
			ASSERT(false);
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

	//public: steady_vector(T entries[], std::size_t count);
	public: ~steady_vector();
	public: bool check_invariant() const;

	public: steady_vector(const steady_vector& rhs);
	public: steady_vector& operator=(const steady_vector& rhs);
	public: void swap(steady_vector& other);

	// ###	operator== and !=

	public: steady_vector push_back(const T& value) const;
	public: steady_vector assoc(size_t index, const T& value) const;
	public: std::size_t size() const;
	public: bool empty() const{
		return size() == 0;
	}

	public: T operator[](const std::size_t index) const{
		return get_at(index);
	}

	public: T get_at(const std::size_t index) const;

	public: std::vector<T> to_vec() const;

	public: void trace_internals() const;


	///////////////////////////////////////		Internals

	public: steady_vector(NodeRef<T> root, std::size_t size);

	private: NodeRef<T> insert(NodeRef<T> node, size_t size, size_t pos, const T& v) const;


	///////////////////////////////////////		State
		public: NodeRef<T> _root;
		public: std::size_t _size;
};

	
#endif /* defined(__steady__steady_vector__) */

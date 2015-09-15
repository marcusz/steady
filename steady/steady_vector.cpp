//
//  steady_vector.cpp
//  steady
//
//  Created by Marcus Zetterquist on 2013-11-13.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#include "steady_vector.h"

#include <algorithm>


//??? Exception safety pls!

//### Removing values or nodes from a node doesn not need path-copying, only disposing entire nodes: we already store the count in 
//??? Path copying requires 31 * 6 RC-bumps!
//??? Test break for branch factor != 4!!! Tests must find problems with 32.
//	### Support different branch factors per instance of steady_vector<T>? BF 2 is great for modification, bad for lookup.
//	### Support holes = allow using for ideal hash.



template <class T>
int INode<T>::_debug_count = 0;

template <class T>
int LeafNode<T>::_debug_count = 0;



/////////////////////////////////////////////			Building blocks


namespace {

	size_t round_up(size_t value, size_t align){
		auto r = value / align;
		return r * align < value ? r + 1 : r;
	}


	/*
		Returns how deep node hiearchy is for a tree with *count* values. Counts both leaf-nodes and inodes.
		0: empty
		1: one leaf node.
		2: one inode with 1-4 leaf nodes.
		3: two levels of inodes plus leaf nodes.
	*/
	int CountToDepth(size_t count){
		const auto leafNodeCount = round_up(count, kBranchingFactor);

		if(leafNodeCount == 0){
			return 0;
		}
		else if(leafNodeCount == 1){
			return 1;
		}
		else {
			return 1 + CountToDepth(leafNodeCount);
		}
	}

	/*
		Return how many steps to shift vector-index to get its *top-level* bits.

		0 = leaf-node level, 1 = inode1 (inode that points to leafnodes), 2 >= inode that points to inodes.
	*/
	size_t VectorSizeToShift(size_t size){
		size_t shift = (CountToDepth(size) - 1) * kBranchingFactorShift;
		return shift;
	}

	template <class T>
	NodeRef<T> MakeLeaf(const std::vector<T>& values){
		ASSERT(values.size() <= kBranchingFactor);

		return NodeRef<T>(new LeafNode<T>(values));
	}



	template <class T>
	NodeRef<T> MakeINode(const std::vector<NodeRef<T>>& children){
		ASSERT(children.size() > 0);
		ASSERT(children.size() <= kBranchingFactor);

		return NodeRef<T>(new INode<T>(children));
	}


	/*
		Verifies the tree is valid.
	*/

	template <class T>
	bool tree_check_invariant(const NodeRef<T>& tree, size_t size){
		ASSERT(tree.check_invariant());
	#if DEBUG
		if(size == 0){
			ASSERT(tree.GetType() == kNullNode);
		}
		else{
			ASSERT(tree.GetType() != kNullNode);
		}
	#endif
		return true;
	}





	template <class T>
	NodeRef<T> find_leaf(const NodeRef<T>& tree, size_t size, size_t index){
		ASSERT(tree_check_invariant(tree, size));
		ASSERT(index < size);

		auto shift = VectorSizeToShift(size);

		NodeRef<T> node = tree;

		//	Traverse all inodes.
		while(shift > 0){
			size_t slot = (index >> shift) & kBranchingFactorMask;
			node = node._inode->GetChild(slot);
			shift -= kBranchingFactorShift;
		}

		ASSERT(shift == 0);
		ASSERT(node.GetType() == kLeafNode);
		return node;
	}


	template <class T>
	NodeRef<T> make_1_tree(const T& value){
		std::vector<T> temp;
		temp.push_back(value);
		auto leafNodeRef = MakeLeaf(temp);
		ASSERT(tree_check_invariant(leafNodeRef, 1));
		return leafNodeRef;
	}


	template <class T>
	NodeRef<T> copy_node_shallow(const NodeRef<T>& node){
		if(node.GetType() == kNullNode){
			return NodeRef<T>();
		}
		else if(node.GetType() == kInode){
			return NodeRef<T>(new INode<T>(node._inode->GetChildren()));
		}
		if(node.GetType() == kLeafNode){
			return NodeRef<T>(new LeafNode<T>(node._leaf->_values));
		}
		else{
			ASSERT(false);
		}
	}




	/*
		node: original tree. Not changed by function. Cannot be null node, only inode or leaf node.

		shift:
		index: entry to store "value" to.
		value: value to store.
		result: copy of "tree" that has "value" stored. Same size as original.
			result-tree and original tree shares internal state
	*/
	template <class T>
	NodeRef<T> modify_value(const NodeRef<T>& node, size_t shift, size_t index, const T& value){
		ASSERT(node.GetType() == kInode || node.GetType() == kLeafNode);

		size_t slotIndex = (index >> shift) & kBranchingFactorMask;
		if(shift == 0){
			ASSERT(node.GetType() == kLeafNode);

			NodeRef<T> copy = copy_node_shallow(node);

			ASSERT(slotIndex < copy._leaf->_values.size());
			copy._leaf->_values[slotIndex] = value;
			return copy;
		}
		else{
			ASSERT(node.GetType() == kInode);

			auto child = node._inode->GetChild(slotIndex);
			auto childCopy = modify_value(child, shift - kBranchingFactorShift, index, value);

			std::vector<NodeRef<T>> children = node._inode->GetChildren();
			children[slotIndex] = childCopy;
			NodeRef<T> copy = NodeRef<T>(new INode<T>(children));
			return copy;
		}
	}


}






/////////////////////////////////////////////			steady_vector





template <class T>
steady_vector<T>::steady_vector() :
	_size(0)
{
	ASSERT(check_invariant());
}


template <class T>
steady_vector<T>::steady_vector(const std::vector<T>& vec) :
	_size(0)
{
	//	!!! Illegal to take adress of first element of vec if it's empty.
	if(!vec.empty()){
		steady_vector<T> temp(&vec[0], vec.size());
		temp.swap(*this);
	}

	ASSERT(check_invariant());
}


template <class T>
steady_vector<T>::steady_vector(const T entries[], size_t count) :
	_size(0)
{
	ASSERT(entries != nullptr);

	steady_vector<T> temp;
	for(size_t i = 0 ; i < count ; i++){
		temp = temp.push_back(entries[i]);
	}

	steady_vector<T> temp2 = temp;
	temp2.swap(*this);

	ASSERT(check_invariant());
}


template <class T>
steady_vector<T>::steady_vector(std::initializer_list<T> args) :
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

	_size = 0;
}


template <class T>
bool steady_vector<T>::check_invariant() const{
	if(_root.GetType() == kNullNode){
		ASSERT(_size == 0);
	}
	else{
		ASSERT(_size >= 0);
	}
	ASSERT(tree_check_invariant(_root, _size));
	return true;
}


template <class T>
steady_vector<T>::steady_vector(const steady_vector& rhs)
:
	_size(0)
{
	ASSERT(rhs.check_invariant());

	NodeRef<T> newRef(rhs._root);
	_root.swap(newRef);
	_size = rhs._size;

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

	_root.swap(rhs._root);
	std::swap(_size, rhs._size);

	ASSERT(check_invariant());
	ASSERT(rhs.check_invariant());
}


template <class T>
steady_vector<T>::steady_vector(NodeRef<T> root, std::size_t size) :
	_root(root),
	_size(size)
{
	ASSERT(check_invariant());
}








namespace  {

	/*
		original: original tree. Not changed by function. Cannot be null node, only inode or leaf node.
		size: current number of values in tree.
		value: value to store.
		result: copy of "tree" that has "value" stored. Same size as original.
			result-tree and original tree shares internal state

		New tree may be same depth or +1 deep.
	*/
	template <class T>
	NodeRef<T> append_leaf_node(const NodeRef<T>& original, size_t shift, size_t size, const NodeRef<T> append){
		ASSERT(tree_check_invariant(original, size));
		ASSERT(original.GetType() == kInode || original.GetType() == kLeafNode);

		NodeRef<T> newRoot;

		//	Is this a leaf node? Then replace it with a new inode that holds it and its new sibling.
		if(shift == 0){
			NodeRef<T> newParent = MakeINode<T>({ original, append });
			return newParent;
		}

		//	Inode holding leafs?
		else if(shift == kBranchingFactorShift){
			std::vector<NodeRef<T>> children = original._inode->GetChildren();

			//	Can we fit new branch in this node?
			if(children.size() < kBranchingFactor){
				children.push_back(append);
				NodeRef<T> newParent = MakeINode(children);
				return newParent;
			}
			else{
				NodeRef<T> newSiblingINode = MakeINode<T>({ append });
				NodeRef<T> newParent = MakeINode<T>({ original, newSiblingINode });
				return newParent;
			}
		}
		else {
			ASSERT(false);
		}
	}

}


template <class T>
steady_vector<T> steady_vector<T>::push_back(const T& value) const{
	ASSERT(check_invariant());

	if(_size == 0){
		const auto root = make_1_tree(value);
		return steady_vector<T>(root, 1);
	}
	else{
		//	Does last leaf node have space left? Then we can use modify_value()...
		if((_size & kBranchingFactorMask) != 0){
			auto shift = VectorSizeToShift(_size);
			const auto root = modify_value(_root, shift, _size, value);
			return steady_vector<T>(root, _size + 1);
		}

		//	Need to make new leaf node.
		else{
			const auto leaf = MakeLeaf<T>({ value });

			//	0 = leaf-node level, 1 = inode1 (inode that points to leafnodes), 2 >= inode that points to inodes.
			auto shift = VectorSizeToShift(_size);

			const auto root = append_leaf_node(_root, shift, _size, leaf);
			return steady_vector<T>(root, _size + 1);

//			return steady_vector<T>();
		}
	}
}


template <class T>
steady_vector<T> steady_vector<T>::assoc(size_t index, const T& value) const{
	ASSERT(check_invariant());
	ASSERT(index < _size);

	auto shift = VectorSizeToShift(_size);
	const auto root = modify_value(_root, shift, index, value);
	return steady_vector<T>(root, _size);
}


template <class T>
std::size_t steady_vector<T>::size() const{
	ASSERT(check_invariant());

	return _size;
}


template <class T>
T steady_vector<T>::get_at(const std::size_t index) const{
	ASSERT(check_invariant());
	ASSERT(index < _size);

	const auto leaf = find_leaf(_root, _size, index);
	const auto slotIndex = index & kBranchingFactorMask;

	ASSERT(slotIndex < leaf._leaf->_values.size());
	const T result = leaf._leaf->_values[slotIndex];
	return result;
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


namespace {
	template <class T>
	void trace_node(const NodeRef<T>& node){
		if(node.GetType() == kNullNode){
			TRACE_SS("<null>");
		}
		else if(node.GetType() == kInode){
			TRACE_SS("<inode> RC: " << node._inode->_rc);
			SCOPED_INDENT();
			for(auto i: node._inode->GetChildren()){
				trace_node(i);
			}
		}
		else if(node.GetType() == kLeafNode){
			TRACE_SS("<leaf> RC: " << node._leaf->_rc);
			SCOPED_INDENT();
			for(auto i: node._leaf->_values){
				TRACE_SS(i);
			}
		}
		else{
			ASSERT(false);
		}
	}
}


template <class T>
void steady_vector<T>::trace_internals() const{
	ASSERT(check_invariant());

	TRACE_SS("Vector (size: " << _size << ") "
		"total inodes: " << INode<T>::_debug_count << ", "
		"total leaf nodes: " << LeafNode<T>::_debug_count);

	trace_node(_root);
}






////////////////////////////////////////////			Unit tests


void vector_test(const std::vector<int>& v){
}

UNIT_TEST("std::vector<>", "auto convertion from initializer list", "", ""){


	std::vector<int> vi {1,2,3,4,5,6};

	vector_test(vi);

	vector_test(std::vector<int>{ 8, 9, 10});
	vector_test({ 8, 9, 10});
}




template <class T>
struct TestFixture {
	TestFixture() :
		_scopedTracer("TestFixture"),
		_inode_count(INode<T>::_debug_count),
		_leaf_count(LeafNode<T>::_debug_count)
	{
		TRACE_SS("INode count: " << _inode_count << " " << "LeafNode count: " << _leaf_count);
	}

	TestFixture(int inode_expected_count, int leaf_expected_count) :
		_scopedTracer("TestFixture"),
		_inode_count(INode<T>::_debug_count),
		_leaf_count(LeafNode<T>::_debug_count),

		_inode_expected_count(inode_expected_count),
		_leaf_expected_count(leaf_expected_count)
	{
		TRACE_SS("INode count: " << _inode_count << " " << "LeafNode count: " << _leaf_count);
	}

	~TestFixture(){
		int inode_count = INode<T>::_debug_count;
		int leaf_count = LeafNode<T>::_debug_count;

		TRACE_SS("INode count: " << inode_count << " " << "LeafNode count: " << leaf_count);

		int inode_diff_count = inode_count - _inode_count;
		int leaf_expected_diff = leaf_count - _leaf_count;


		TEST_VERIFY(inode_diff_count == _inode_expected_count);
		TEST_VERIFY(leaf_expected_diff == _leaf_expected_count);
	}


	CScopedTrace _scopedTracer;
	int _inode_count = 0;
	int _leaf_count = 0;

	int _inode_expected_count = 0;
	int _leaf_expected_count = 0;
};






steady_vector<int> MakeVectorWith1(){
	TestFixture<int> f(0, 1);

	std::vector<int> values = {	7	};
	NodeRef<int> leaf = MakeLeaf(values);
	return steady_vector<int>(leaf, values.size());
}


UNIT_TEST("steady_vector", "MakeVectorWith1()", "", "correct nodes"){
	TestFixture<int> f;

	const auto a = MakeVectorWith1();
	TEST_VERIFY(a.size() == 1);
	TEST_VERIFY(a._root.GetType() == kLeafNode);
	TEST_VERIFY(a._root._leaf->_rc == 1);
	TEST_VERIFY(a._root._leaf->_values.size() == 1);
	TEST_VERIFY(a._root._leaf->_values[0] == 7);
}


steady_vector<int> MakeVectorWith2(){
	TestFixture<int> f(0, 1);

	std::vector<int> values = {	7, 8	};
	NodeRef<int> leaf = MakeLeaf(values);
	return steady_vector<int>(leaf, values.size());
}

UNIT_TEST("steady_vector", "MakeVectorWith2()", "", "correct nodes"){
	TestFixture<int> f;
	const auto a = MakeVectorWith2();
	TEST_VERIFY(a.size() == 2);
	TEST_VERIFY(a._root.GetType() == kLeafNode);
	TEST_VERIFY(a._root._leaf->_rc == 1);
	TEST_VERIFY(a._root._leaf->_values.size() == 2);
	TEST_VERIFY(a._root._leaf->_values[0] == 7);
	TEST_VERIFY(a._root._leaf->_values[1] == 8);
}




steady_vector<int> MakeVectorWith5(){
	TestFixture<int> f(1, 2);
	NodeRef<int> leaf0 = MakeLeaf(std::vector<int>({ 7, 8, 9, 10 }));
	NodeRef<int> leaf1 = MakeLeaf(std::vector<int>({ 11 }));

	std::vector<NodeRef<int>> leafs = { leaf0, leaf1 };
	NodeRef<int> inode = MakeINode(leafs);
	return steady_vector<int>(inode, 5);
}

UNIT_TEST("steady_vector", "MakeVectorWith5()", "", "correct nodes"){
	TestFixture<int> f;

	const auto a = MakeVectorWith5();
	TEST_VERIFY(a.size() == 5);

	TEST_VERIFY(a._root.GetType() == kInode);
	TEST_VERIFY(a._root._inode->_rc == 1);
	TEST_VERIFY(a._root._inode->GetChildCount() == 2);
	TEST_VERIFY(a._root._inode->GetChild(0).GetType() == kLeafNode);
	TEST_VERIFY(a._root._inode->GetChild(1).GetType() == kLeafNode);

	LeafNode<int>* leaf0 = a._root._inode->GetChildLeafNode(0);
	TEST_VERIFY(leaf0->_rc == 1);
	TEST_VERIFY(leaf0->_values.size() == 4);
	TEST_VERIFY(leaf0->_values[0] == 7);
	TEST_VERIFY(leaf0->_values[1] == 8);
	TEST_VERIFY(leaf0->_values[2] == 9);
	TEST_VERIFY(leaf0->_values[3] == 10);

	LeafNode<int>* leaf1 = a._root._inode->GetChildLeafNode(1);
	TEST_VERIFY(leaf1->_rc == 1);
	TEST_VERIFY(leaf1->_values.size() == 1);
	TEST_VERIFY(leaf1->_values[0] == 11);
}




steady_vector<int> MakeVectorWith17(){
	TestFixture<int> f(3, 5);

	NodeRef<int> leaf0 = MakeLeaf(std::vector<int>({ 1000, 1001, 1002, 1003 }));
	NodeRef<int> leaf1 = MakeLeaf(std::vector<int>({ 1004, 1005, 1006, 1007 }));
	NodeRef<int> leaf2 = MakeLeaf(std::vector<int>({ 1008, 1009, 1010, 1011 }));
	NodeRef<int> leaf3 = MakeLeaf(std::vector<int>({ 1012, 1013, 1014, 1015 }));
	NodeRef<int> leaf4 = MakeLeaf(std::vector<int>({ 1016 }));

	NodeRef<int> inodeA = MakeINode(std::vector<NodeRef<int>>{ leaf0, leaf1, leaf2, leaf3 });
	NodeRef<int> inodeB = MakeINode(std::vector<NodeRef<int>>{ leaf4 });
	NodeRef<int> rootInode = MakeINode(std::vector<NodeRef<int>>{ inodeA, inodeB });
	return steady_vector<int>(rootInode, 17);
}

UNIT_TEST("steady_vector", "MakeVectorWith17()", "", "correct nodes"){
	TestFixture<int> f;

	const auto a = MakeVectorWith17();
	TEST_VERIFY(a.size() == 17);

	NodeRef<int> rootINode = a._root;
	TEST_VERIFY(rootINode.GetType() == kInode);
	TEST_VERIFY(rootINode._inode->_rc == 2);
	TEST_VERIFY(rootINode._inode->GetChildCount() == 2);
	TEST_VERIFY(rootINode._inode->GetChild(0).GetType() == kInode);
	TEST_VERIFY(rootINode._inode->GetChild(1).GetType() == kInode);


	NodeRef<int> inodeA = rootINode._inode->GetChild(0);
	TEST_VERIFY(inodeA.GetType() == kInode);
	TEST_VERIFY(inodeA._inode->_rc == 2);
	TEST_VERIFY(inodeA._inode->GetChildCount() == 4);
	TEST_VERIFY(inodeA._inode->GetChild(0).GetType() == kLeafNode);
	TEST_VERIFY(inodeA._inode->GetChild(1).GetType() == kLeafNode);
	TEST_VERIFY(inodeA._inode->GetChild(2).GetType() == kLeafNode);
	TEST_VERIFY(inodeA._inode->GetChild(3).GetType() == kLeafNode);

	LeafNode<int>* leaf0 = inodeA._inode->GetChildLeafNode(0);
	TEST_VERIFY(leaf0->_rc == 1);
	TEST_VERIFY(leaf0->_values.size() == 4);
	TEST_VERIFY(leaf0->_values[0] == 1000);
	TEST_VERIFY(leaf0->_values[1] == 1001);
	TEST_VERIFY(leaf0->_values[2] == 1002);
	TEST_VERIFY(leaf0->_values[3] == 1003);

	LeafNode<int>* leaf1 = inodeA._inode->GetChildLeafNode((1));
	TEST_VERIFY(leaf1->_rc == 1);
	TEST_VERIFY(leaf1->_values.size() == 4);
	TEST_VERIFY(leaf1->_values[0] == 1004);
	TEST_VERIFY(leaf1->_values[1] == 1005);
	TEST_VERIFY(leaf1->_values[2] == 1006);
	TEST_VERIFY(leaf1->_values[3] == 1007);

	LeafNode<int>* leaf2 = inodeA._inode->GetChildLeafNode(2);
	TEST_VERIFY(leaf2->_rc == 1);
	TEST_VERIFY(leaf2->_values.size() == 4);
	TEST_VERIFY(leaf2->_values[0] == 1008);
	TEST_VERIFY(leaf2->_values[1] == 1009);
	TEST_VERIFY(leaf2->_values[2] == 1010);
	TEST_VERIFY(leaf2->_values[3] == 1011);

	LeafNode<int>* leaf3 = inodeA._inode->GetChildLeafNode(3);
	TEST_VERIFY(leaf3->_rc == 1);
	TEST_VERIFY(leaf3->_values.size() == 4);
	TEST_VERIFY(leaf3->_values[0] == 1012);
	TEST_VERIFY(leaf3->_values[1] == 1013);
	TEST_VERIFY(leaf3->_values[2] == 1014);
	TEST_VERIFY(leaf3->_values[3] == 1015);


	NodeRef<int> inodeB = rootINode._inode->GetChild(1);
	TEST_VERIFY(inodeB.GetType() == kInode);
	TEST_VERIFY(inodeB._inode->_rc == 2);
	TEST_VERIFY(inodeB._inode->GetChildCount() == 1);
	TEST_VERIFY(inodeB._inode->GetChild(0).GetType() == kLeafNode);


	LeafNode<int>* leaf4 = inodeB._inode->GetChildLeafNode(0);
	TEST_VERIFY(leaf4->_rc == 1);
	TEST_VERIFY(leaf4->_values.size() == 1);
	TEST_VERIFY(leaf4->_values[0] == 1016);

}





UNIT_TEST("steady_vector", "CountToDepth()", "0", "-1"){
	TEST_VERIFY(CountToDepth(0) == 0);

	TEST_VERIFY(CountToDepth(1) == 1);
	TEST_VERIFY(CountToDepth(2) == 1);
	TEST_VERIFY(CountToDepth(3) == 1);

	TEST_VERIFY(CountToDepth(5) == 2);
	TEST_VERIFY(CountToDepth(16) == 2);

	TEST_VERIFY(CountToDepth(17) == 3);
	TEST_VERIFY(CountToDepth(64) == 3);
}







UNIT_TEST("steady_vector", "steady_vector()", "", "no_assert"){
	TestFixture<int> f;

	steady_vector<int> v;
	v.trace_internals();
}





UNIT_TEST("steady_vector", "operator[]", "1 item", "read back"){
	TestFixture<int> f;

	const auto a = MakeVectorWith1();
	TEST_VERIFY(a[0] == 7);
	a.trace_internals();
}



UNIT_TEST("steady_vector", "operator[]", "5 items", "read back"){
	TestFixture<int> f;
	const auto a = MakeVectorWith5();
	TEST_VERIFY(a[0] == 7);
	TEST_VERIFY(a[1] == 8);
	TEST_VERIFY(a[2] == 9);
	TEST_VERIFY(a[3] == 10);
	TEST_VERIFY(a[4] == 11);
	a.trace_internals();
}


UNIT_TEST("steady_vector", "operator[]", "17 items", "read back"){
	TestFixture<int> f;
	const auto a = MakeVectorWith17();
	TEST_VERIFY(a[0] == 1000);
	TEST_VERIFY(a[1] == 1001);
	TEST_VERIFY(a[2] == 1002);
	TEST_VERIFY(a[3] == 1003);
	TEST_VERIFY(a[4] == 1004);
	TEST_VERIFY(a[5] == 1005);
	TEST_VERIFY(a[6] == 1006);
	TEST_VERIFY(a[7] == 1007);
	TEST_VERIFY(a[8] == 1008);
	TEST_VERIFY(a[9] == 1009);
	TEST_VERIFY(a[10] == 1010);
	TEST_VERIFY(a[11] == 1011);
	TEST_VERIFY(a[12] == 1012);
	TEST_VERIFY(a[13] == 1013);
	TEST_VERIFY(a[14] == 1014);
	TEST_VERIFY(a[15] == 1015);
	TEST_VERIFY(a[16] == 1016);
	a.trace_internals();
}



UNIT_TEST("steady_vector", "assoc()", "1 item", "read back"){
	TestFixture<int> f;
	const auto a = MakeVectorWith1();
	const auto b = a.assoc(0, 1000);
	TEST_VERIFY(a[0] == 7);
	TEST_VERIFY(b[0] == 1000);
	a.trace_internals();
}


UNIT_TEST("steady_vector", "assoc()", "5 item vector, replace #0", "read back"){
	TestFixture<int> f;
	const auto a = MakeVectorWith5();
	const auto b = a.assoc(0, 1000);
	TEST_VERIFY(a[0] == 7);
	TEST_VERIFY(b[0] == 1000);
}

UNIT_TEST("steady_vector", "assoc()", "5 item vector, replace #4", "read back"){
	TestFixture<int> f;
	const auto a = MakeVectorWith5();
	const auto b = a.assoc(4, 1000);
	TEST_VERIFY(a[0] == 7);
	TEST_VERIFY(b[4] == 1000);
}

UNIT_TEST("steady_vector", "assoc()", "17 item vector, replace bunch", "read back"){
	TestFixture<int> f;
	auto a = MakeVectorWith17();
	a = a.assoc(4, 1004);
	a = a.assoc(5, 1005);
	a = a.assoc(0, 1000);
	a = a.assoc(16, 1016);
	a = a.assoc(10, 1010);

	TEST_VERIFY(a[0] == 1000);
	TEST_VERIFY(a[4] == 1004);
	TEST_VERIFY(a[5] == 1005);
	TEST_VERIFY(a[16] == 1016);
	TEST_VERIFY(a[10] == 1010);

	a.trace_internals();
}

UNIT_TEST("steady_vector", "assoc()", "5 item vector, replace value 10000 times", "read back"){
	TestFixture<int> f;
	auto a = MakeVectorWith5();

	for(int i = 0 ; i < 1000 ; i++){
		a = a.assoc(4, i);
	}
	TEST_VERIFY(a[4] == 999);

	a.trace_internals();
}






UNIT_TEST("steady_vector", "push_back()", "one item", "read back"){
	TestFixture<int> f;
	const steady_vector<int> a;
	const auto b = a.push_back(4);
	TEST_VERIFY(a.size() == 0);
	TEST_VERIFY(b.size() == 1);
	TEST_VERIFY(b[0] == 4);
}

#if 0
UNIT_TEST("steady_vector", "push_back()", "two items", "read back both"){
	TestFixture<int> f;
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

UNIT_TEST("steady_vector", "push_back()", "force 1 inode", "read back"){
	TestFixture<int> f;
	steady_vector<int> a;
	const int count = kBranchingFactor + 1;
	for(int i = 0 ; i < count ; i++){
		a = a.push_back(1000 + i);
	}

	TEST_VERIFY(a.size() == count);
	a.trace_internals();

	for(int i = 0 ; i < count ; i++){
		const auto v = a[i];
		TEST_VERIFY(v == (1000 + i));
	}
}
#endif


UNIT_TEST("steady_vector", "size()", "empty vector", "0"){
	TestFixture<int> f;
	steady_vector<int> v;
	TEST_VERIFY(v.size() == 0);
}






UNIT_TEST("steady_vector", "steady_vector(const std::vector<T>& vec)", "0 items", "empty"){
	TestFixture<int> f;
	const std::vector<int> a = {};
	steady_vector<int> v(a);
	TEST_VERIFY(v.size() == 0);
}

#if 0
UNIT_TEST("steady_vector", "steady_vector(const std::vector<T>& vec)", "7 items", "read back all"){
	TestFixture<int> f;
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
#endif


#if 0

UNIT_TEST("steady_vector", "steady_vector(const T entries[], size_t count)", "0 items", "empty"){
	TestFixture<int> f;
	const int a[] = {};
	steady_vector<int> v(&a[0], 0);
	TEST_VERIFY(v.size() == 0);
}


UNIT_TEST("steady_vector", "steady_vector(const T entries[], size_t count)", "7 items", "read back all"){
	TestFixture<int> f;
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
	TestFixture<int> f;
	steady_vector<int> v = {};

	TEST_VERIFY(v.size() == 0);
}


UNIT_TEST("steady_vector", "steady_vector(std::initializer_list<T> args)", "7 items", "read back all"){
	TestFixture<int> f;
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

#endif








#if 0

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


#endif



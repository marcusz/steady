//
//  steady_vector.cpp
//  steady
//
//  Created by Marcus Zetterquist on 2013-11-13.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#include "steady_vector.h"

#include <algorithm>


//??? Immutable = don't deep copy!
//??? Exception safety pls!

//### Removing values or nodes from a node doesn not need path-copying, only disposing entire nodes: we already store the count in 

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
		steady_vector<T> temp2(&vec[0], vec.size());
		temp2.swap(*this);
	}

	ASSERT(check_invariant());
}



size_t round_up(size_t value, size_t align){
	auto r = value / align;
	return r * align < value ? r + 1 : r;
}


#if 0
//	Returns a new node owning all the child nodes + a count telling how many values it stores.
template <class T>
std::pair<NodeRef<T>, int> fill(const T values[], size_t count){
	ASSERT(values != nullptr);

	//	Round upwards.
	const auto leafNodesNeeded = (count >> kBranchingFactorShift) + ((count & kBranchingFactorMask) != 0 ? 1 : 0);

	//	Empty?
	if(leafNodesNeeded == 0){
		return std::pair<NodeRef<T>, int>(NodeRef<T>(), 0);
	}

	//	Make one leaf node?
	if(leafNodesNeeded == 1){
		auto leaf = new Leaf<T>();
		leaf->_rc = 1;
		leaf->_values = std::vector<T>(&values[0], &values[count]);
		return std::pair<NodeRef<T>, int>(NodeRef<T>(leaf), count);
	}

	//	Make an inode that refers to leaf nodes?
	else if(leafNodesNeeded <= kBranchingFactor){
		auto iNodeRef = NodeRef<T>(new INode<T>());

		for(int i = 0 ; i < leafNodesNeeded ; i++){
			const auto offset = i * kBranchingFactor;
			auto a = fill(&values[offset], std::min(leafNodesNeeded - offset, kBranchingFactor));
			ASSERT(a.first._type == NodeRef<T>::kLeaf);
			a.first._ptr._leaf->_rc++;
			iNodeRef._ptr._inode->_leafs.push_back(a.first._ptr._leaf);
		}
		return std::pair<NodeRef<T>, int>(NodeRef<T>(iNodeRef), count);
	}

	//	Make an inode that refers to other inodes nodes?
	else{
//		auto inode = new INode<T>();

/*
		const auto subNodeCount = (count >> (kBranchingFactorShift * 2)) + ((count & kBranchingFactorMask) != 0 ? 1 : 0);

		for(int i = 0 ; i < subNodeCount ; i++){
			const auto offset = i * kBranchingFactor;
			auto leafNodeRef = fill(&values[offset], std::min(leafNodesNeeded - offset, kBranchingFactor));
			ASSERT(leafNodeRef._type == NodeRef<T>::kLeaf);
			leafNodeRef._ptr._leaf->_rc++;
			inode->_leafs.push_back(leafNodeRef._ptr._leaf);
		}
		return NodeRef<T>(inode);
*/
	}
}


#endif



#if 0

//	Returns a new node owning all the child nodes + a count telling how many values it stores.
template <class T>
std::pair<NodeRef<T>, int> fill2(const T values[], size_t count, size_t totalValueCount){
	ASSERT(values != nullptr);

	const auto totalLeafNodesNeeded = round_up(totalValueCount, kBranchingFactor);
	const auto leafNodesNeeded = round_up(totalValueCount, kBranchingFactor);

	//	Empty?
	if(leafNodesNeeded == 0){
		return std::pair<NodeRef<T>, int>(NodeRef<T>(), 0);
	}

	//	Make one leaf node?
	if(leafNodesNeeded == 1){
		auto leaf = new Leaf<T>();
		leaf->_rc = 1;
		leaf->_values = std::vector<T>(&values[0], &values[count]);
		return std::pair<NodeRef<T>, int>(NodeRef<T>(leaf), count);
	}

	//	Make an inode that refers to leaf nodes?
	else if(leafNodesNeeded <= kBranchingFactor){
		auto iNodeRef = NodeRef<T>(new INode<T>());

		for(int i = 0 ; i < leafNodesNeeded ; i++){
			const auto offset = i * kBranchingFactor;
			auto a = fill(&values[offset], std::min(leafNodesNeeded - offset, kBranchingFactor));
			ASSERT(a.first._type == NodeRef<T>::kLeaf);
			a.first._ptr._leaf->_rc++;
			iNodeRef._ptr._inode->_leafs.push_back(a.first._ptr._leaf);
		}
		return std::pair<NodeRef<T>, int>(NodeRef<T>(iNodeRef), count);
	}

	//	Make an inode that refers to other inodes nodes?
	else{
		auto inode = new INode<T>();

/*
		const auto subNodeCount = (count >> (kBranchingFactorShift * 2)) + ((count & kBranchingFactorMask) != 0 ? 1 : 0);

		for(int i = 0 ; i < subNodeCount ; i++){
			const auto offset = i * kBranchingFactor;
			auto leafNodeRef = fill(&values[offset], std::min(leafNodesNeeded - offset, kBranchingFactor));
			ASSERT(leafNodeRef._type == NodeRef<T>::kLeaf);
			leafNodeRef._ptr._leaf->_rc++;
			inode->_leafs.push_back(leafNodeRef._ptr._leaf);
		}
		return NodeRef<T>(inode);
*/
	}
}

#endif







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
	if(_root._type == NodeRef<T>::kNull){
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
	_size(0)
{
	ASSERT(rhs.check_invariant());

	NodeRef<T> newRef(rhs._root);
	_root.swap(newRef);

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






template <class T>
NodeRef<T> append_item(const NodeRef<T>& nodeRef, const T& value){
	if(nodeRef._type == NodeRef<T>::kNull){
		auto leaf = new Leaf<T>();
		leaf._rc = 1;
		leaf._values.push_back(value);
		return NodeRef<T>(leaf);
	}
	else if(nodeRef._type == NodeRef<T>::kInode){
	}
	else if(nodeRef._type == NodeRef<T>::kLeaf){
	}
	else{
		ASSERT(false);
	}
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


template <class T>
NodeRef<T> MakeLeaf(std::vector<T> values){
	ASSERT(values.size() <= kBranchingFactor);

	NodeRef<T> ref(new Leaf<T>());
	ref._ptr._leaf->_rc = 1;
	ref._ptr._leaf->_values = values;
	return ref;
}



template <class T>
NodeRef<T> MakeINode(const std::vector<NodeRef<T>>& children){
	ASSERT(children.size() > 0);
	ASSERT(children.size() <= kBranchingFactor);

	const auto childrenType = children[0]._type;
#if DEBUG
	//	Make sure children are of the same type!
	{
		for(auto i: children){
			ASSERT(i._type == childrenType);
		}
	}
#endif


	NodeRef<T> inodeRef(new INode<T>());
	inodeRef._ptr._inode->_rc = 1;

	if(childrenType == NodeRef<T>::kInode){
		for(auto i: children){
			inodeRef._ptr._inode->_inodes.push_back(i._ptr._inode);
			i._ptr._inode->_rc++;
		}
	}
	else if(childrenType == NodeRef<T>::kLeaf){
		for(auto i: children){
			inodeRef._ptr._inode->_leafs.push_back(i._ptr._leaf);
			i._ptr._leaf->_rc++;
		}
	}
	else{
		ASSERT(false);
	}

	return inodeRef;
}




template <class T>
steady_vector<T> steady_vector<T>::push_back(const T& v) const{
	ASSERT(check_invariant());

	if(_size == 0){
		std::vector<T> temp;
		temp.push_back(v);
		auto leafNodeRef = MakeLeaf(temp);
		return steady_vector<T>(leafNodeRef, 1);
	}
	else{
		return steady_vector<T>();
/*
		auto newRoot = CopyPath(_size);



		int inodeDepth = CountToDepth(_size);
		const auto leafItems = _size & kBranchingFactorMask;
		if(leafItems < kBranchingFactor){
			NodeRef<T> newRoot;
		}

		auto a = append_item(_root, v);
		steady_vector<T> result;
		result._root = a;
		result._size = _size + 1;
		ASSERT(result.check_invariant());

		return result;
*/
	}
}



#if 0
template <class T>
steady_vector<T> steady_vector<T>::update(size_t index, const T& v) const{
	ASSERT(check_invariant());
	ASSERT(index < _size);
	
	std::size_t new_count = _size + 1;

	steady_vector<T> result;
	result._allocation = new T[new_count];
	result._size = new_count;

	for(std::size_t i = 0 ; i < _size ; i++){
		result._allocation[i] = _allocation[i];
	}
	result._allocation[_size] = v;

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
T steady_vector<T>::get_at(const std::size_t index) const{
	ASSERT(check_invariant());
	ASSERT(index < _size);

	auto depth = CountToDepth(_size);
	ASSERT(depth > 0);

	//	0 = leaf-node level, 1 = inode1 (inode that points to leafnodes), 2 >= inode that points to inodes.
	size_t shift = (depth - 1) * kBranchingFactorShift;

	NodeRef<T> node = _root;

	//	Traverse all inodes that points to other inodes.
	while(shift > kBranchingFactorShift){
		size_t slot = (index >> shift) & kBranchingFactorMask;
		INode<T>* childPtr = node._ptr._inode->_inodes[slot];
		node = NodeRef<T>(childPtr);
		shift -= kBranchingFactorShift;
	}

	//	Inode that points to leaf nodes?
	if(shift == kBranchingFactorShift){
		size_t slot = (index >> shift) & kBranchingFactorMask;
		Leaf<T>* childPtr = node._ptr._inode->_leafs[slot];
		node = NodeRef<T>(childPtr);
		shift -= kBranchingFactorShift;
	}

	ASSERT(shift == 0);

	size_t slot = (index >> shift) & kBranchingFactorMask;
	const T result = node._ptr._leaf->_values[slot];
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








////////////////////////////////////////////			Unit tests






steady_vector<int> MakeVectorWith1(){
	std::vector<int> values = {	7	};
	NodeRef<int> leaf = MakeLeaf(values);
	return steady_vector<int>(leaf, values.size());
}


UNIT_TEST("steady_vector", "MakeVectorWith1()", "", "correct nodes"){
	const auto a = MakeVectorWith1();
	TEST_VERIFY(a.size() == 1);
	TEST_VERIFY(a._root._type == NodeRef<int>::kLeaf);
	TEST_VERIFY(a._root._ptr._leaf->_rc == 1);
	TEST_VERIFY(a._root._ptr._leaf->_values.size() == 1);
	TEST_VERIFY(a._root._ptr._leaf->_values[0] == 7);
}


steady_vector<int> MakeVectorWith2(){
	std::vector<int> values = {	7, 8	};
	NodeRef<int> leaf = MakeLeaf(values);
	return steady_vector<int>(leaf, values.size());
}

UNIT_TEST("steady_vector", "MakeVectorWith2()", "", "correct nodes"){
	const auto a = MakeVectorWith2();
	TEST_VERIFY(a.size() == 2);
	TEST_VERIFY(a._root._type == NodeRef<int>::kLeaf);
	TEST_VERIFY(a._root._ptr._leaf->_rc == 1);
	TEST_VERIFY(a._root._ptr._leaf->_values.size() == 2);
	TEST_VERIFY(a._root._ptr._leaf->_values[0] == 7);
	TEST_VERIFY(a._root._ptr._leaf->_values[1] == 8);
}


steady_vector<int> MakeVectorWith5(){
	NodeRef<int> leaf0 = MakeLeaf(std::vector<int>({ 7, 8, 9, 10 }));
	NodeRef<int> leaf1 = MakeLeaf(std::vector<int>({ 11 }));

	std::vector<NodeRef<int>> leafs = { leaf0, leaf1 };
	NodeRef<int> inode = MakeINode(leafs);
	return steady_vector<int>(inode, 5);
}

UNIT_TEST("steady_vector", "MakeVectorWith5()", "", "correct nodes"){
	const auto a = MakeVectorWith5();
	TEST_VERIFY(a.size() == 5);

	TEST_VERIFY(a._root._type == NodeRef<int>::kInode);
	TEST_VERIFY(a._root._ptr._inode->_rc == 1);
	TEST_VERIFY(a._root._ptr._inode->_leafs.size() == 2);
	TEST_VERIFY(a._root._ptr._inode->_leafs[0] != nullptr);
	TEST_VERIFY(a._root._ptr._inode->_leafs[1] != nullptr);

	Leaf<int>* leaf0 = a._root._ptr._inode->_leafs[0];
	TEST_VERIFY(leaf0->_rc == 1);
	TEST_VERIFY(leaf0->_values.size() == 4);
	TEST_VERIFY(leaf0->_values[0] == 7);
	TEST_VERIFY(leaf0->_values[1] == 8);
	TEST_VERIFY(leaf0->_values[2] == 9);
	TEST_VERIFY(leaf0->_values[3] == 10);

	Leaf<int>* leaf1 = a._root._ptr._inode->_leafs[1];
	TEST_VERIFY(leaf1->_rc == 1);
	TEST_VERIFY(leaf1->_values.size() == 1);
	TEST_VERIFY(leaf1->_values[0] == 11);
}








steady_vector<int> MakeVectorWith17(){
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
	const auto a = MakeVectorWith17();
	TEST_VERIFY(a.size() == 17);

	NodeRef<int> rootINode = a._root;
	TEST_VERIFY(rootINode._type == NodeRef<int>::kInode);
	TEST_VERIFY(rootINode._ptr._inode->_rc == 2);
	TEST_VERIFY(rootINode._ptr._inode->_inodes.size() == 2);
	TEST_VERIFY(rootINode._ptr._inode->_inodes[0] != nullptr);
	TEST_VERIFY(rootINode._ptr._inode->_inodes[1] != nullptr);


	NodeRef<int> inodeA = rootINode._ptr._inode->_inodes[0];
	TEST_VERIFY(inodeA._type == NodeRef<int>::kInode);
	TEST_VERIFY(inodeA._ptr._inode->_rc == 2);
	TEST_VERIFY(inodeA._ptr._inode->_leafs.size() == 4);
	TEST_VERIFY(inodeA._ptr._inode->_leafs[0] != nullptr);
	TEST_VERIFY(inodeA._ptr._inode->_leafs[1] != nullptr);
	TEST_VERIFY(inodeA._ptr._inode->_leafs[2] != nullptr);
	TEST_VERIFY(inodeA._ptr._inode->_leafs[3] != nullptr);

	Leaf<int>* leaf0 = inodeA._ptr._inode->_leafs[0];
	TEST_VERIFY(leaf0->_rc == 1);
	TEST_VERIFY(leaf0->_values.size() == 4);
	TEST_VERIFY(leaf0->_values[0] == 1000);
	TEST_VERIFY(leaf0->_values[1] == 1001);
	TEST_VERIFY(leaf0->_values[2] == 1002);
	TEST_VERIFY(leaf0->_values[3] == 1003);

	Leaf<int>* leaf1 = inodeA._ptr._inode->_leafs[1];
	TEST_VERIFY(leaf1->_rc == 1);
	TEST_VERIFY(leaf1->_values.size() == 4);
	TEST_VERIFY(leaf1->_values[0] == 1004);
	TEST_VERIFY(leaf1->_values[1] == 1005);
	TEST_VERIFY(leaf1->_values[2] == 1006);
	TEST_VERIFY(leaf1->_values[3] == 1007);

	Leaf<int>* leaf2 = inodeA._ptr._inode->_leafs[2];
	TEST_VERIFY(leaf2->_rc == 1);
	TEST_VERIFY(leaf2->_values.size() == 4);
	TEST_VERIFY(leaf2->_values[0] == 1008);
	TEST_VERIFY(leaf2->_values[1] == 1009);
	TEST_VERIFY(leaf2->_values[2] == 1010);
	TEST_VERIFY(leaf2->_values[3] == 1011);

	Leaf<int>* leaf3 = inodeA._ptr._inode->_leafs[3];
	TEST_VERIFY(leaf3->_rc == 1);
	TEST_VERIFY(leaf3->_values.size() == 4);
	TEST_VERIFY(leaf3->_values[0] == 1012);
	TEST_VERIFY(leaf3->_values[1] == 1013);
	TEST_VERIFY(leaf3->_values[2] == 1014);
	TEST_VERIFY(leaf3->_values[3] == 1015);




	NodeRef<int> inodeB = rootINode._ptr._inode->_inodes[1];
	TEST_VERIFY(inodeB._type == NodeRef<int>::kInode);
	TEST_VERIFY(inodeB._ptr._inode->_rc == 2);
	TEST_VERIFY(inodeB._ptr._inode->_leafs.size() == 1);
	TEST_VERIFY(inodeB._ptr._inode->_leafs[0] != nullptr);



	Leaf<int>* leaf4 = inodeB._ptr._inode->_leafs[0];
	TEST_VERIFY(leaf4->_rc == 1);
	TEST_VERIFY(leaf4->_values.size() == 1);
	TEST_VERIFY(leaf4->_values[0] == 1016);

}

//??? Test break for branch factor != 4!!!











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
	steady_vector<int> v;
}





UNIT_TEST("steady_vector", "operator[]", "1 item", "read back"){
	const auto a = MakeVectorWith1();
	TEST_VERIFY(a[0] == 7);
}



UNIT_TEST("steady_vector", "operator[]", "5 items", "read back"){
	const auto a = MakeVectorWith5();
	TEST_VERIFY(a[0] == 7);
	TEST_VERIFY(a[1] == 8);
	TEST_VERIFY(a[2] == 9);
	TEST_VERIFY(a[3] == 10);
	TEST_VERIFY(a[4] == 11);
}


UNIT_TEST("steady_vector", "operator[]", "17 items", "read back"){
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
}










UNIT_TEST("steady_vector", "push_back()", "one item", "read back"){
	const steady_vector<int> a;
	const auto b = a.push_back(4);
	TEST_VERIFY(a.size() == 0);
	TEST_VERIFY(b.size() == 1);
	TEST_VERIFY(b[0] == 4);
}

#if 0
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
#endif


UNIT_TEST("steady_vector", "size()", "empty vector", "0"){
	steady_vector<int> v;
	TEST_VERIFY(v.size() == 0);
}






UNIT_TEST("steady_vector", "steady_vector(const std::vector<T>& vec)", "0 items", "empty"){
	const std::vector<int> a = {};
	steady_vector<int> v(a);
	TEST_VERIFY(v.size() == 0);
}

#if 0
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
#endif


#if 0




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



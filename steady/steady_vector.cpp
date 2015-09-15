//
//  steady_vector.cpp
//  steady
//
//  Created by Marcus Zetterquist on 2013-11-13.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#include "steady_vector.h"

#include <algorithm>
#include <array>

/*
NOW
====================================================================================================================
### optimize pop_back()


NEXT
====================================================================================================================
### optimize operator==()

Improve tree validation.

first()
rest()


SOMEDAY
====================================================================================================================
Store shift in steady_vector to avoid recomputing it all the time.

subvec - no trimming = very fast.

assoc at end => append

pool nodes?

Over-alloc / reserve nodes?


??? Exception safety pls!

??? Path copying requires 31 * 6 RC-bumps!

### Use placement-now in leaf nodes to avoid default-constructing all leaf node items.

### Add tail-node optimization, or even random-access modification cache (one leaf-node that slides across vector, not just at the end).

### Removing values or nodes from a node doesn not need path-copying, only disposing entire nodes: we already store the count in

### Support different branch factors per instance of steady_vector<T>? BF 2 is great for modification, bad for lookup.

### Support holes = allow using for ideal hash.
*/







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

	public: LeafNode(const std::array<T, kBranchingFactor>& values) :
		_rc(0),
		_values(values)
	{
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

	public: std::atomic<int32_t> _rc;
	public: std::array<T, kBranchingFactor> _values{};
	public: static int _debug_count;
};



////////////////////////////////////////////		INode



namespace {


	template <class T>
	bool validate_inode_children(const std::array<NodeRef<T>, kBranchingFactor>& vec){
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
		full set of iNode pointers or
		full set of leaf node pointers or
		empty vector.

	You cannot mix inode and leaf nodes in the same INode.
	INode pointers and leaf node pointers can be null, but the nulls are always at the end of the arrays.
	Sets its internal RC to 0 and never changes it.
*/

template <class T>
struct INode {
	public: typedef std::array<NodeRef<T>, kBranchingFactor> TChildren;


	public: INode() :
		_rc(0)
	{
		_debug_count++;
		ASSERT(check_invariant());
	}

	//	children: 0-32 children, all of the same type. kNullNodes can only appear at end of vector.
	public: INode(const TChildren& children2) :
		_rc(0),
		_children(children2)
	{
		ASSERT(children2.size() >= 0);
		ASSERT(children2.size() <= kBranchingFactor);
#if DEBUG
		for(auto i: children2){
			i.check_invariant();
		}
#endif

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

	public: TChildren GetChildrenWithNulls(){
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


	public: std::atomic<int32_t> _rc;
	private: TChildren _children;
	public: static int _debug_count;
};




////////////////////////////////////////////		NodeRef<T>





template <typename T>
NodeRef<T>::NodeRef() :
	_inode(nullptr),
	_leaf(nullptr)
{
	ASSERT(check_invariant());
}

template <typename T>
NodeRef<T>::NodeRef(INode<T>* node) :
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

template <typename T>
NodeRef<T>::NodeRef(LeafNode<T>* node) :
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

template <typename T>
NodeRef<T>::NodeRef(const NodeRef<T>& ref) :
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

template <typename T>
NodeRef<T>::~NodeRef(){
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

template <typename T>
bool NodeRef<T>::check_invariant() const {
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

template <typename T>
void NodeRef<T>::swap(NodeRef<T>& other){
	ASSERT(check_invariant());
	ASSERT(other.check_invariant());

	std::swap(_inode, other._inode);
	std::swap(_leaf, other._leaf);

	ASSERT(check_invariant());
	ASSERT(other.check_invariant());
}

template <typename T>
NodeRef<T>& NodeRef<T>::operator=(const NodeRef<T>& other){
	ASSERT(check_invariant());
	ASSERT(other.check_invariant());

	NodeRef<T> temp(other);

	temp.swap(*this);

	ASSERT(check_invariant());
	ASSERT(other.check_invariant());
	return *this;
}

template <typename T>
NodeType NodeRef<T>::GetType() const {
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

		-kBranchingFactorShift: empty tree
		0: leaf-node level
		kBranchingFactorShift: inode1 (inode that points to leafnodes)
		>=kBranchingFactorShift: inode that points to inodes.
	*/
	int VectorSizeToShift(size_t size){
		int shift = (CountToDepth(size) - 1) * kBranchingFactorShift;
		return shift;
	}

	template <class T>
	NodeRef<T> MakeLeafNode(const std::array<T, kBranchingFactor>& values){
		return NodeRef<T>(new LeafNode<T>(values));
	}



	template <class T>
	NodeRef<T> MakeINodeFromVector(const std::vector<NodeRef<T>>& children){
		ASSERT(children.size() >= 0);
		ASSERT(children.size() <= kBranchingFactor);

		std::array<NodeRef<T>, kBranchingFactor> temp{};
		std::copy(children.begin(), children.end(), temp.begin());
		return NodeRef<T>(new INode<T>(temp));
	}

	template <class T>
	NodeRef<T> MakeINodeFromArray(const std::array<NodeRef<T>, kBranchingFactor>& children){
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
	NodeRef<T> find_leaf_node(const NodeRef<T>& tree, size_t size, size_t index){
		ASSERT(tree_check_invariant(tree, size));
		ASSERT(index < size);

		auto shift = VectorSizeToShift(size);

		NodeRef<T> nodeIt = tree;

		//	Traverse all inodes.
		while(shift > 0){
			size_t slotIndex = (index >> shift) & kBranchingFactorMask;
			nodeIt = nodeIt._inode->GetChild(slotIndex);
			shift -= kBranchingFactorShift;
		}

		ASSERT(shift == 0);
		ASSERT(nodeIt.GetType() == kLeafNode);
		return nodeIt;
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
	NodeRef<T> modify_existing_value(const NodeRef<T>& node, int shift, size_t index, const T& value){
		ASSERT(node.GetType() == kInode || node.GetType() == kLeafNode);

		const size_t slotIndex = (index >> shift) & kBranchingFactorMask;
		if(shift == 0){
			ASSERT(node.GetType() == kLeafNode);

			auto copy = NodeRef<T>(new LeafNode<T>(node._leaf->_values));
			ASSERT(slotIndex < copy._leaf->_values.size());
			copy._leaf->_values[slotIndex] = value;
			return copy;
		}
		else{
			ASSERT(node.GetType() == kInode);

			const auto child = node._inode->GetChild(slotIndex);
			auto childCopy = modify_existing_value(child, shift - kBranchingFactorShift, index, value);

			auto children = node._inode->GetChildrenWithNulls();
			children[slotIndex] = childCopy;
			auto copy = MakeINodeFromArray(children);
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
	std::vector<T> temp(args.begin(), args.end());
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

	template <class T>
	NodeRef<T> make_new_path(int shift, const NodeRef<T>& append){
		if(shift == 0){
			return append;
		}
		else{
			auto a = make_new_path(shift - kBranchingFactorShift, append);
			NodeRef<T> b = MakeINodeFromArray<T>({ a });
			return b;
		}
	}

	/*
		original: original tree. Not changed by function. Cannot be null node, only inode or leaf node.
		size: current number of values in tree.
		value: value to store.
		result: copy of "tree" that has "value" stored. Same size as original.
			result-tree and original tree shares internal state

		New tree may be same depth or +1 deep.
	*/
	template <class T>
	NodeRef<T> append_leaf_node(const NodeRef<T>& original, int shift, size_t index, const NodeRef<T>& append){
		ASSERT(original.check_invariant());
		ASSERT(original.GetType() == kInode);
		ASSERT(append.check_invariant());
		ASSERT(append.GetType() == kLeafNode);

		size_t slotIndex = (index >> shift) & kBranchingFactorMask;
		auto children = original._inode->GetChildrenWithNulls();

		//	Lowest level inode, pointing to leaf nodes.
		if(shift == kBranchingFactorShift){
			children[slotIndex] = append;
			return MakeINodeFromArray<T>(children);
		}
		else {
			const auto child = children[slotIndex];
			if(child.GetType() == kNullNode){
				NodeRef<T> child2 = make_new_path(shift - kBranchingFactorShift, append);
				children[slotIndex] = child2;
				return MakeINodeFromArray<T>(children);
			}
			else{
				NodeRef<T> child2 = append_leaf_node(child, shift - kBranchingFactorShift, index, append);
				children[slotIndex] = child2;
				return MakeINodeFromArray<T>(children);
			}
		}
	}

}


template <class T>
steady_vector<T> steady_vector<T>::push_back(const T& value) const{
	ASSERT(check_invariant());

	if(_size == 0){
		return steady_vector<T>(MakeLeafNode<T>({value}), 1);
	}
	else{

		//	Does last leaf node have space left? Then we can use modify_existing_value()...
		if((_size & kBranchingFactorMask) != 0){
			auto shift = VectorSizeToShift(_size);
			const auto root = modify_existing_value(_root, shift, _size, value);
			return steady_vector<T>(root, _size + 1);
		}

		//	Allocate new *leaf-node*, adding it to tree.
		else{
			const auto leaf = MakeLeafNode<T>({ value });

			auto shift = VectorSizeToShift(_size);
			auto shift2 = VectorSizeToShift(_size + 1);

			//	Space left in root?
			if(shift2 == shift){
				const auto root = append_leaf_node(_root, shift, _size, leaf);
				return steady_vector<T>(root, _size + 1);
			}
			else{
				auto newPath = make_new_path(shift, leaf);
				auto newRoot = MakeINodeFromArray<T>({ _root, newPath });
				return steady_vector<T>(newRoot, _size + 1);
			}
		}
	}
}


/*
	Correct but inefficient.
*/
template <class T>
steady_vector<T> steady_vector<T>::pop_back() const{
	ASSERT(check_invariant());
	ASSERT(_size > 0);

	const auto temp = to_vec();
	const auto result = steady_vector<T>(&temp[0], _size - 1);
	return result;
}


/*
	Correct but inefficient.
*/
template <class T>
bool steady_vector<T>::operator==(const steady_vector& rhs) const{
	ASSERT(check_invariant());

	const auto a = to_vec();
	const auto b = rhs.to_vec();
	return a == b;
}



template <class T>
steady_vector<T> steady_vector<T>::assoc(size_t index, const T& value) const{
	ASSERT(check_invariant());
	ASSERT(index < _size);

	auto shift = VectorSizeToShift(_size);
	const auto root = modify_existing_value(_root, shift, index, value);
	return steady_vector<T>(root, _size);
}


template <class T>
std::size_t steady_vector<T>::size() const{
	ASSERT(check_invariant());

	return _size;
}


template <class T>
T steady_vector<T>::operator[](const std::size_t index) const{
	ASSERT(check_invariant());
	ASSERT(index < _size);

	const auto leaf = find_leaf_node(_root, _size, index);
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
		const auto value = operator[](i);
		a.push_back(value);
	}

	return a;
}


namespace {
	template <class T>
	void trace_node(const std::string& prefix, const NodeRef<T>& node){
		if(node.GetType() == kNullNode){
			TRACE_SS(prefix << "<null>");
		}
		else if(node.GetType() == kInode){
			TRACE_SS(prefix << "<inode> RC: " << node._inode->_rc);
			SCOPED_INDENT();
			int index = 0;
			for(auto i: node._inode->GetChildrenWithNulls()){
				trace_node("#" + std::to_string(index) + "\t", i);
				index++;
			}
		}
		else if(node.GetType() == kLeafNode){
			TRACE_SS(prefix << "<leaf> RC: " << node._leaf->_rc);
			SCOPED_INDENT();
			int index = 0;
			for(auto i: node._leaf->_values){
				TRACE_SS("#" << std::to_string(index) << "\t" << i);
				(void)i;
				index++;
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

	trace_node("", _root);
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

UNIT_TEST("", "TestFixture()", "", "no assert"){
	TestFixture<int> test;
}




UNIT_TEST("", "CountToDepth()", "0", "-1"){
	TEST_VERIFY(CountToDepth(0) == 0);

	TEST_VERIFY(CountToDepth(1) == 1);
	TEST_VERIFY(CountToDepth(2) == 1);
	TEST_VERIFY(CountToDepth(3) == 1);

	TEST_VERIFY(CountToDepth(kBranchingFactor + 1) == 2);
	TEST_VERIFY(CountToDepth(kBranchingFactor * kBranchingFactor) == 2);

	TEST_VERIFY(CountToDepth(kBranchingFactor * kBranchingFactor + 1) == 3);
	TEST_VERIFY(CountToDepth(kBranchingFactor * kBranchingFactor *kBranchingFactor) == 3);
}




std::vector<int> GenerateNumbers(int start, int count, int totalCount){
	ASSERT(count >= 0);
	ASSERT(totalCount >= count);

	std::vector<int> a;
	int i = 0;
	while(i < count){
		a.push_back(start + i);
		i++;
	}
	while(i < totalCount){
		a.push_back(0);
		i++;
	}
	return a;
}

UNIT_TEST("", "GenerateNumbers()", "5 numbers", "correct vector"){
	const auto a = GenerateNumbers(8, 4, 7);
	TEST_VERIFY(a == (std::vector<int>{ 8, 9, 10, 11, 0, 0, 0 }));
}

std::array<int, kBranchingFactor> GenerateLeaves(int start, int count){
	const auto a = GenerateNumbers(start, count, kBranchingFactor);

	std::array<int, kBranchingFactor> result;
	for(int i = 0 ; i < kBranchingFactor ; i++){
		result[i] = a[i];
	}
	return result;
}



steady_vector<int> MakeManualVectorWith1(){
	TestFixture<int> f(0, 1);

	NodeRef<int> leaf = MakeLeafNode<int>({ 7 });
	return steady_vector<int>(leaf, 1);
}

UNIT_TEST("", "MakeManualVectorWith1()", "", "correct nodes"){
	TestFixture<int> f;

	const auto a = MakeManualVectorWith1();
	TEST_VERIFY(a.size() == 1);
	TEST_VERIFY(a.GetRoot().GetType() == kLeafNode);
	TEST_VERIFY(a.GetRoot()._leaf->_rc == 1);
	TEST_VERIFY(a.GetRoot()._leaf->_values[0] == 7);
	for(int i = 1 ; i < kBranchingFactor ; i++){
		TEST_VERIFY(a.GetRoot()._leaf->_values[i] == 0);
	}
}


steady_vector<int> MakeManualVectorWith2(){
	TestFixture<int> f(0, 1);

	NodeRef<int> leaf = MakeLeafNode<int>({	7, 8	});
	return steady_vector<int>(leaf, 2);
}

UNIT_TEST("", "MakeManualVectorWith2()", "", "correct nodes"){
	TestFixture<int> f;
	const auto a = MakeManualVectorWith2();
	TEST_VERIFY(a.size() == 2);
	TEST_VERIFY(a.GetRoot().GetType() == kLeafNode);
	TEST_VERIFY(a.GetRoot()._leaf->_rc == 1);
	TEST_VERIFY(a.GetRoot()._leaf->_values[0] == 7);
	TEST_VERIFY(a.GetRoot()._leaf->_values[1] == 8);
	TEST_VERIFY(a.GetRoot()._leaf->_values[2] == 0);
	TEST_VERIFY(a.GetRoot()._leaf->_values[3] == 0);
}




steady_vector<int> MakeManualVectorWithBranchFactorPlus1(){
	TestFixture<int> f(1, 2);
	NodeRef<int> leaf0 = MakeLeafNode(GenerateLeaves(7, kBranchingFactor));
	NodeRef<int> leaf1 = MakeLeafNode(GenerateLeaves(7 + kBranchingFactor, 1));
	std::vector<NodeRef<int>> leafs = { leaf0, leaf1 };
	NodeRef<int> inode = MakeINodeFromVector(leafs);
	return steady_vector<int>(inode, kBranchingFactor + 1);
}

UNIT_TEST("", "MakeManualVectorWithBranchFactorPlus1()", "", "correct nodes"){
	TestFixture<int> f;

	const auto a = MakeManualVectorWithBranchFactorPlus1();
	TEST_VERIFY(a.size() == kBranchingFactor + 1);

	TEST_VERIFY(a.GetRoot().GetType() == kInode);
	TEST_VERIFY(a.GetRoot()._inode->_rc == 1);
	TEST_VERIFY(a.GetRoot()._inode->GetChildCountSkipNulls() == 2);
	TEST_VERIFY(a.GetRoot()._inode->GetChild(0).GetType() == kLeafNode);
	TEST_VERIFY(a.GetRoot()._inode->GetChild(1).GetType() == kLeafNode);

	const auto leaf0 = a.GetRoot()._inode->GetChildLeafNode(0);
	TEST_VERIFY(leaf0->_rc == 1);
	TEST_VERIFY(leaf0->_values == GenerateLeaves(7 + kBranchingFactor * 0, kBranchingFactor));

	const auto leaf1 = a.GetRoot()._inode->GetChildLeafNode(1);
	TEST_VERIFY(leaf1->_rc == 1);
	TEST_VERIFY(leaf1->_values == GenerateLeaves(7 + kBranchingFactor * 1, 1));
}


//17
steady_vector<int> MakeManualVectorWithBranchFactorSquarePlus1(){
	TestFixture<int> f(3, kBranchingFactor + 1);

	std::vector<NodeRef<int>> leaves;
	for(int i = 0 ; i < kBranchingFactor ; i++){
		NodeRef<int> leaf = MakeLeafNode(GenerateLeaves(1000 + kBranchingFactor * i, kBranchingFactor));
		leaves.push_back(leaf);
	}

	NodeRef<int> extraLeaf = MakeLeafNode(GenerateLeaves(1000 + kBranchingFactor * kBranchingFactor + 0, 1));

	NodeRef<int> inodeA = MakeINodeFromVector<int>(leaves);
	NodeRef<int> inodeB = MakeINodeFromVector<int>({ extraLeaf });
	NodeRef<int> rootInode = MakeINodeFromVector<int>({ inodeA, inodeB });
	return steady_vector<int>(rootInode, kBranchingFactor * kBranchingFactor + 1);
}

UNIT_TEST("", "MakeManualVectorWithBranchFactorSquarePlus1()", "", "correct nodes"){
	TestFixture<int> f;

	const auto a = MakeManualVectorWithBranchFactorSquarePlus1();
	TEST_VERIFY(a.size() == kBranchingFactor * kBranchingFactor + 1);

	NodeRef<int> rootINode = a.GetRoot();
	TEST_VERIFY(rootINode.GetType() == kInode);
	TEST_VERIFY(rootINode._inode->_rc == 2);
	TEST_VERIFY(rootINode._inode->GetChildCountSkipNulls() == 2);
	TEST_VERIFY(rootINode._inode->GetChild(0).GetType() == kInode);
	TEST_VERIFY(rootINode._inode->GetChild(1).GetType() == kInode);

	NodeRef<int> inodeA = rootINode._inode->GetChild(0);
		TEST_VERIFY(inodeA.GetType() == kInode);
		TEST_VERIFY(inodeA._inode->_rc == 2);
		TEST_VERIFY(inodeA._inode->GetChildCountSkipNulls() == kBranchingFactor);
		for(int i = 0 ; i < kBranchingFactor ; i++){
			const auto leafNode = inodeA._inode->GetChildLeafNode(i);
			TEST_VERIFY(leafNode->_rc == 1);
			TEST_VERIFY(leafNode->_values == GenerateLeaves(1000 + kBranchingFactor * i, kBranchingFactor));
		}

	NodeRef<int> inodeB = rootINode._inode->GetChild(1);
		TEST_VERIFY(inodeB.GetType() == kInode);
		TEST_VERIFY(inodeB._inode->_rc == 2);
		TEST_VERIFY(inodeB._inode->GetChildCountSkipNulls() == 1);
		TEST_VERIFY(inodeB._inode->GetChild(0).GetType() == kLeafNode);

		const auto leaf4 = inodeB._inode->GetChildLeafNode(0);
		TEST_VERIFY(leaf4->_rc == 1);
		TEST_VERIFY(leaf4->_values == GenerateLeaves(1000 + kBranchingFactor * kBranchingFactor + 0, 1));
}




////////////////////////////////////////////		steady_vector::steady_vector()



UNIT_TEST("steady_vector", "steady_vector()", "", "no_assert"){
	TestFixture<int> f;

	steady_vector<int> v;
	v.trace_internals();
}


////////////////////////////////////////////		steady_vector::operator[]


UNIT_TEST("steady_vector", "operator[]", "1 item", "read back"){
	TestFixture<int> f;

	const auto a = MakeManualVectorWith1();
	TEST_VERIFY(a[0] == 7);
	a.trace_internals();
}

UNIT_TEST("steady_vector", "operator[]", "Branchfactor + 1 items", "read back"){
	TestFixture<int> f;
	const auto a = MakeManualVectorWithBranchFactorPlus1();
	TEST_VERIFY(a[0] == 7);
	TEST_VERIFY(a[1] == 8);
	TEST_VERIFY(a[2] == 9);
	TEST_VERIFY(a[3] == 10);
	TEST_VERIFY(a[4] == 11);
	a.trace_internals();
}

UNIT_TEST("steady_vector", "operator[]", "Branchfactor^2 + 1 items", "read back"){
	TestFixture<int> f;
	const auto a = MakeManualVectorWithBranchFactorSquarePlus1();
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


////////////////////////////////////////////		steady_vector::assoc()



UNIT_TEST("steady_vector", "assoc()", "1 item", "read back"){
	TestFixture<int> f;
	const auto a = MakeManualVectorWith1();
	const auto b = a.assoc(0, 1000);
	TEST_VERIFY(a[0] == 7);
	TEST_VERIFY(b[0] == 1000);
	a.trace_internals();
}


UNIT_TEST("steady_vector", "assoc()", "5 item vector, replace #0", "read back"){
	TestFixture<int> f;
	const auto a = MakeManualVectorWithBranchFactorPlus1();
	const auto b = a.assoc(0, 1000);
	TEST_VERIFY(a[0] == 7);
	TEST_VERIFY(b[0] == 1000);
}

UNIT_TEST("steady_vector", "assoc()", "5 item vector, replace #4", "read back"){
	TestFixture<int> f;
	const auto a = MakeManualVectorWithBranchFactorPlus1();
	const auto b = a.assoc(4, 1000);
	TEST_VERIFY(a[0] == 7);
	TEST_VERIFY(b[4] == 1000);
}

UNIT_TEST("steady_vector", "assoc()", "17 item vector, replace bunch", "read back"){
	TestFixture<int> f;
	auto a = MakeManualVectorWithBranchFactorSquarePlus1();
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
	auto a = MakeManualVectorWithBranchFactorPlus1();

	for(int i = 0 ; i < 1000 ; i++){
		a = a.assoc(4, i);
	}
	TEST_VERIFY(a[4] == 999);

	a.trace_internals();
}




////////////////////////////////////////////		steady_vector::push_back()




namespace {
	steady_vector<int> push_back_n(int count, int value0){
	//	TestFixture<int> f;
		steady_vector<int> a;
		for(int i = 0 ; i < count ; i++){
			a = a.push_back(value0 + i);
		}
		return a;
	}

	void test_values(const steady_vector<int>& vec, int value0){
		TestFixture<int> f;

		for(int i = 0 ; i < vec.size() ; i++){
			const auto value = vec[i];
			const auto expected = value0 + i;
			TEST_VERIFY(value == expected);
		}
	}
}

UNIT_TEST("steady_vector", "push_back()", "one item => 1 leaf node", "read back"){
	TestFixture<int> f;
	const steady_vector<int> a;
	const auto b = a.push_back(4);
	TEST_VERIFY(a.size() == 0);
	TEST_VERIFY(b.size() == 1);
	TEST_VERIFY(b[0] == 4);
}

UNIT_TEST("steady_vector", "push_back()", "two items => 1 leaf node", "read back"){
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

UNIT_TEST("steady_vector", "push_back()", "1 inode", "read back"){
	TestFixture<int> f;
	const auto count = kBranchingFactor + 1;
	steady_vector<int> a = push_back_n(count, 1000);
	TEST_VERIFY(a.size() == count);
	test_values(a, 1000);
	a.trace_internals();
}

UNIT_TEST("steady_vector", "push_back()", "1 inode + add leaf to last node", "read back all items"){
	TestFixture<int> f;
	const auto count = kBranchingFactor + 2;
	steady_vector<int> a = push_back_n(count, 1000);
	TEST_VERIFY(a.size() == count);
	test_values(a, 1000);
	a.trace_internals();
}

UNIT_TEST("steady_vector", "push_back()", "2-levels of inodes", "read back all items"){
	TestFixture<int> f;
	const auto count = kBranchingFactor * kBranchingFactor + 1;
	steady_vector<int> a = push_back_n(count, 1000);
	TEST_VERIFY(a.size() == count);
	test_values(a, 1000);
	a.trace_internals();
}

UNIT_TEST("steady_vector", "push_back()", "2-levels of inodes + add leaf-node to last node", "read back all items"){
	TestFixture<int> f;
	const auto count = kBranchingFactor * kBranchingFactor * 2;
	steady_vector<int> a = push_back_n(count, 1000);
	TEST_VERIFY(a.size() == count);
	test_values(a, 1000);
	a.trace_internals();
}

UNIT_TEST("steady_vector", "push_back()", "3-levels of inodes + add leaf-node to last node", "read back all items"){
	TestFixture<int> f;
	const auto count = kBranchingFactor * kBranchingFactor * kBranchingFactor * 2;
	steady_vector<int> a = push_back_n(count, 1000);
	TEST_VERIFY(a.size() == count);
	test_values(a, 1000);
	a.trace_internals();
}


////////////////////////////////////////////		steady_vector::pop_back()


UNIT_TEST("steady_vector", "pop_back()", "basic", "correct result vector"){
	TestFixture<int> f;
	const auto data = GenerateNumbers(4, 50, 50);
	const auto data2 = std::vector<int>(&data[0], &data[data.size() - 1]);
	const auto a = steady_vector<int>(data);
	const auto b = a.pop_back();
	TEST_VERIFY(b.to_vec() == data2);
}



////////////////////////////////////////////		steady_vector::operator==()


UNIT_TEST("steady_vector", "operator==()", "empty vs empty", "true"){
	TestFixture<int> f;

	const std::vector<int> a;
	const std::vector<int> b;
	TEST_VERIFY(a == b);
}

UNIT_TEST("steady_vector", "operator==()", "empty vs 1", "false"){
	TestFixture<int> f;

	const std::vector<int> a;
	const std::vector<int> b{ 33 };
	TEST_VERIFY(!(a == b));
}

UNIT_TEST("steady_vector", "operator==()", "1000 vs 1000", "true"){
	TestFixture<int> f;
	const auto data = GenerateNumbers(4, 50, 50);
	const std::vector<int> a(data);
	const std::vector<int> b(data);
	TEST_VERIFY(a == b);
}

UNIT_TEST("steady_vector", "operator==()", "1000 vs 1000", "false"){
	TestFixture<int> f;
	const auto data = GenerateNumbers(4, 50, 50);
	auto data2 = data;
	data2[47] = 0;

	const std::vector<int> a(data);
	const std::vector<int> b(data2);
	TEST_VERIFY(!(a == b));
}



////////////////////////////////////////////		steady_vector::size()



UNIT_TEST("steady_vector", "size()", "empty vector", "0"){
	TestFixture<int> f;
	steady_vector<int> v;
	TEST_VERIFY(v.size() == 0);
}

UNIT_TEST("steady_vector", "size()", "BranchFactorSquarePlus1", "BranchFactorSquarePlus1"){
	TestFixture<int> f;
	const auto a = MakeManualVectorWithBranchFactorSquarePlus1();
	TEST_VERIFY(a.size() == kBranchingFactor * kBranchingFactor + 1);
}



////////////////////////////////////////////		steady_vector::steady_vector(const std::vector<T>& vec)



UNIT_TEST("steady_vector", "steady_vector(const std::vector<T>& vec)", "0 items", "empty"){
	TestFixture<int> f;
	const std::vector<int> a = {};
	steady_vector<int> v(a);
	TEST_VERIFY(v.size() == 0);
}

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


////////////////////////////////////////////		steady_vector::steady_vector(const T entries[], size_t count)


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


////////////////////////////////////////////		steady_vector::steady_vector(std::initializer_list<T> args)


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


////////////////////////////////////////////		steady_vector::to_vec()


UNIT_TEST("steady_vector", "to_vec()", "0", "empty"){
	TestFixture<int> f;
	const auto a = steady_vector<int>();
	TEST_VERIFY(a.to_vec() == std::vector<int>());
}

UNIT_TEST("steady_vector", "to_vec()", "50", "correct data"){
	TestFixture<int> f;
	const auto data = GenerateNumbers(4, 50, 50);
	const auto a = steady_vector<int>(data);
	TEST_VERIFY(a.to_vec() == data);
}


////////////////////////////////////////////		steady_vector::steady_vector(const steady_vector& rhs)


UNIT_TEST("steady_vector", "steady_vector(const steady_vector& rhs)", "empty", "empty"){
	TestFixture<int> f;
	const auto a = steady_vector<int>();
	const auto b(a);
	TEST_VERIFY(a.empty());
	TEST_VERIFY(b.empty());
}

UNIT_TEST("steady_vector", "steady_vector(const steady_vector& rhs)", "7 items", "identical, sharing root"){
	TestFixture<int> f;
	const auto data = std::vector<int>{	3, 4, 5, 6, 7, 8, 9	};
	const steady_vector<int> a = data;
	const auto b(a);

	TEST_VERIFY(a.to_vec() == data);
	TEST_VERIFY(b.to_vec() == data);
	TEST_VERIFY(a.GetRoot()._leaf == b.GetRoot()._leaf);
}



////////////////////////////////////////////		steady_vector::operator=()



UNIT_TEST("steady_vector", "operator=()", "empty", "empty"){
	TestFixture<int> f;
	const auto a = steady_vector<int>();
	auto b = steady_vector<int>();

	b = a;

	TEST_VERIFY(a.empty());
	TEST_VERIFY(b.empty());
}

UNIT_TEST("steady_vector", "operator=()", "7 items", "identical, sharing root"){
	TestFixture<int> f;
	const auto data = std::vector<int>{	3, 4, 5, 6, 7, 8, 9	};
	const steady_vector<int> a = data;
	auto b = steady_vector<int>();

	b = a;

	TEST_VERIFY(a.to_vec() == data);
	TEST_VERIFY(b.to_vec() == data);
	TEST_VERIFY(a.GetRoot()._leaf == b.GetRoot()._leaf);
}

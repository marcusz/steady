
#include "steady_vector.h"

#include <algorithm>
#include <array>

/*

NOW
====================================================================================================================
[optimization] optimize pop_back()
[optimization] Append leaf by leaf when copying data etc.
[feature] Name library


NEXT
====================================================================================================================
[defect] Exception safety pls!

[defect] Use placement-now in leaf nodes to avoid default-constructing all leaf node values.

[internal quality] Improve tree validation.


SOMEDAY
====================================================================================================================
[feature] first(),rest(). Add seq?

[optimization] optimize operator==()

[optimization] Make inode use one pointer - not two - for each child.

[feature] Make memory allocation hookable.

[feature] Add subvec() - trimming and no trimming (= very fast).

[feature] Allow assoc() at end of vector => append

[optimization] Pool nodes? Notice that inodes are the same size, independently of sizeof(T). This allows inod pooling across Ts.

[optimization] Over-alloc / reserve nodes like std::vector<>?

[optimization] Path copying requires 31 * 6 RC-bumps!

[optimization] Add tail-node optimization, or even random-access modification cache (one leaf-node that slides across vector, not just at the end).

[optimization] Removing values or nodes from a node doesn not need path-copying, only disposing entire nodes: we already store the count in

[feature] Support different branch factors per instance of vector<T>? BF 2 is great for modification, bad for lookup.

[feature] Support holes = allow using for ideal hash.

*/


//	Give QUARK macros shorter names
#define ASSERT(x) QUARK_ASSERT(x)
#define TRACE(x) QUARK_TRACE(x)
#define TRACE_SS(x) QUARK_TRACE_SS(x)
#define VERIFY(x) QUARK_TEST_VERIFY(x)


namespace steady {


namespace internals {

	static const size_t BRANCHING_FACTOR_MASK = (BRANCHING_FACTOR - 1);


////////////////////////////////////////////		leaf_node



/*
//	1 -> BRANCHING_FACTOR values.
	Sets its internal RC to 0 and never changes it.
*/


template <class T>
struct leaf_node {
	public: leaf_node(const std::array<T, BRANCHING_FACTOR>& values) :
		_rc(0),
		_values(values)
	{
		_debug_count++;
		ASSERT(check_invariant());
	}

	public: ~leaf_node(){
		ASSERT(check_invariant());
		ASSERT(_rc == 0);

		_debug_count--;
	}

	public: bool check_invariant() const {
		ASSERT(_rc >= 0);
		ASSERT(_rc < 1000);
		ASSERT(_values.size() == BRANCHING_FACTOR);
		return true;
	}

	private: leaf_node<T>& operator=(const leaf_node& rhs);
	private: leaf_node(const leaf_node& rhs);



	//////////////////////////////	State

	public: std::atomic<int32_t> _rc;
	public: std::array<T, BRANCHING_FACTOR> _values{};
	public: static int _debug_count;
};



////////////////////////////////////////////		inode



namespace {

	/*
		Validates the list, not that the children är valid.
	*/
	template <class T>
	bool validate_inode_children(const std::array<node_ref<T>, BRANCHING_FACTOR>& vec){
		ASSERT(vec.size() >= 0);
		ASSERT(vec.size() <= BRANCHING_FACTOR);

/*
		for(auto i: vec){
			i.check_invariant();
		}
*/

		if(vec.size() > 0){
			const auto type = vec[0].get_type();
			if(type == node_type::null_node){
				for(auto i: vec){
					ASSERT(i.get_type() == node_type::null_node);
				}
			}
			else if(type == node_type::inode){
				int i = 0;
				while(i < vec.size() && vec[i].get_type() == node_type::inode){
					i++;
				}
				while(i < vec.size()){
					ASSERT(vec[i].get_type() == node_type::null_node);
					i++;
				}
			}
			else if(type == node_type::leaf_node){
				int i = 0;
				while(i < vec.size() && vec[i].get_type() == node_type::leaf_node){
					i++;
				}
				while(i < vec.size()){
					ASSERT(vec[i].get_type() == node_type::null_node);
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
	An inode has these states:
		full set of iNode pointers or
		full set of leaf node pointers or
		empty vector.

	You cannot mix sub-inode and sub-leaf nodes in the same inode.
	inode pointers and leaf node pointers can be null, but the nulls are always at the end of the arrays.
	Sets its internal RC to 0 and never changes it.
*/

template <class T>
struct inode {
	public: typedef std::array<node_ref<T>, BRANCHING_FACTOR> children_t;

	//	children: 0-32 children, all of the same type. kNullNodes can only appear at end of vector.
	public: inode(const children_t& children2) :
		_rc(0),
		_children(children2)
	{
		ASSERT(children2.size() >= 0);
		ASSERT(children2.size() <= BRANCHING_FACTOR);
#if DEBUG
		for(auto i: children2){
			i.check_invariant();
		}
#endif

		_debug_count++;
		ASSERT(check_invariant());
	}


	public: ~inode(){
		ASSERT(check_invariant());
		ASSERT(_rc == 0);

		_debug_count--;
	}

	private: inode<T>& operator=(const inode& rhs);
	private: inode(const inode& rhs);

	public: bool check_invariant() const {
		ASSERT(_rc >= 0);
		ASSERT(_rc < 10000);
		ASSERT(validate_inode_children(_children));

		return true;
	}

	//	Counts the children actually used = skips trailing any null children.
	public: size_t count_children() const{
		ASSERT(check_invariant());

		size_t index = 0;
		while(index < _children.size() && _children[index].get_type() != node_type::null_node){
			index++;
		}
		return index;
	}

	//	Returns entire array, even if not all items are used.
	public: children_t get_child_array() const{
		ASSERT(check_invariant());

		return _children;
	}

	public: node_ref<T> get_child(size_t index) const{
		ASSERT(check_invariant());
		ASSERT(index < BRANCHING_FACTOR);
		ASSERT(index < _children.size());

		return _children[index];
	}

	//	You can only call this for leaf nodes.
	public: const leaf_node<T>* get_child_as_leaf_node(size_t index) const{
		ASSERT(check_invariant());
		ASSERT(index < _children.size());

		ASSERT(_children[0].get_type() == node_type::leaf_node);
		return _children[index]._leaf_node;
	}



	//////////////////////////////	State


	public: std::atomic<int32_t> _rc;
	private: children_t _children;
	public: static int _debug_count;
};




////////////////////////////////////////////		node_ref<T>


/*
	Safe, reference counted handle that wraps either an inode, a LeadNode or null.
*/

template <typename T>
node_ref<T>::node_ref() :
	_inode(nullptr),
	_leaf_node(nullptr)
{
	ASSERT(check_invariant());
}

//	Will assume ownership of the input node - caller must not delete it after call returns.
//	Adds ref.
//	node == nullptr => null_node
template <typename T>
node_ref<T>::node_ref(inode<T>* node) :
	_inode(nullptr),
	_leaf_node(nullptr)
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
//	node == nullptr => null_node
template <typename T>
node_ref<T>::node_ref(leaf_node<T>* node) :
	_inode(nullptr),
	_leaf_node(nullptr)
{
	if(node != nullptr){
		ASSERT(node->check_invariant());
		ASSERT(node->_rc >= 0);

		_leaf_node = node;
		_leaf_node->_rc++;
	}

	ASSERT(check_invariant());
}

//	Uses reference counting to share all state.
template <typename T>
node_ref<T>::node_ref(const node_ref<T>& ref) :
	_inode(nullptr),
	_leaf_node(nullptr)
{
	ASSERT(ref.check_invariant());

	if(ref.get_type() == node_type::null_node){
	}
	else if(ref.get_type() == node_type::inode){
		_inode = ref._inode;
		_inode->_rc++;
	}
	else if(ref.get_type() == node_type::leaf_node){
		_leaf_node = ref._leaf_node;
		_leaf_node->_rc++;
	}
	else{
		ASSERT(false);
	}

	ASSERT(check_invariant());
}

template <typename T>
node_ref<T>::~node_ref(){
	ASSERT(check_invariant());

	if(get_type() == node_type::null_node){
	}
	else if(get_type() == node_type::inode){
		_inode->_rc--;
		if(_inode->_rc == 0){
			delete _inode;
			_inode = nullptr;
		}
	}
	else if(get_type() == node_type::leaf_node){
		_leaf_node->_rc--;
		if(_leaf_node->_rc == 0){
			delete _leaf_node;
			_leaf_node = nullptr;
		}
	}
	else{
		ASSERT(false);
	}
}

template <typename T>
bool node_ref<T>::check_invariant() const {
	ASSERT(_inode == nullptr || _leaf_node == nullptr);

	if(_inode != nullptr){
		ASSERT(_inode->check_invariant());
		ASSERT(_inode->_rc > 0);
	}
	else if(_leaf_node != nullptr){
		ASSERT(_leaf_node->check_invariant());
		ASSERT(_leaf_node->_rc > 0);
	}
	return true;
}

template <typename T>
void node_ref<T>::swap(node_ref<T>& rhs){
	ASSERT(check_invariant());
	ASSERT(rhs.check_invariant());

	std::swap(_inode, rhs._inode);
	std::swap(_leaf_node, rhs._leaf_node);

	ASSERT(check_invariant());
	ASSERT(rhs.check_invariant());
}

template <typename T>
node_ref<T>& node_ref<T>::operator=(const node_ref<T>& rhs){
	ASSERT(check_invariant());
	ASSERT(rhs.check_invariant());

	node_ref<T> temp(rhs);

	temp.swap(*this);

	ASSERT(check_invariant());
	ASSERT(rhs.check_invariant());
	return *this;
}

template <typename T>
node_type node_ref<T>::get_type() const {
	ASSERT(_inode == nullptr || _leaf_node == nullptr);

	if(_inode == nullptr && _leaf_node == nullptr){
		return node_type::null_node;
	}
	else if(_inode != nullptr){
		return node_type::inode;
	}
	else if(_leaf_node != nullptr){
		return node_type::leaf_node;
	}
	else{
		QUARK_ASSERT_UNREACHABLE;
	}
}

template <typename T>
const inode<T>* node_ref<T>::get_inode() const {
	ASSERT(check_invariant());
	ASSERT(get_type() == node_type::inode);

	return _inode;
}

template <typename T>
const leaf_node<T>* node_ref<T>::get_leaf_node() const {
	ASSERT(check_invariant());
	ASSERT(get_type() == node_type::leaf_node);

	return _leaf_node;
}

template <typename T>
leaf_node<T>* node_ref<T>::get_leaf_node() {
	ASSERT(check_invariant());
	ASSERT(get_type() == node_type::leaf_node);

	return _leaf_node;
}



template <class T>
int inode<T>::_debug_count = 0;

template <class T>
int leaf_node<T>::_debug_count = 0;

}	//	internals



using namespace internals;



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
	int count_to_depth(size_t count){
		const auto leaf_count = round_up(count, BRANCHING_FACTOR);

		if(leaf_count == 0){
			return 0;
		}
		else if(leaf_count == 1){
			return 1;
		}
		else {
			return 1 + count_to_depth(leaf_count);
		}
	}
}


QUARK_UNIT_TEST("", "count_to_depth()", "0", "-1"){
	VERIFY(count_to_depth(0) == 0);

	VERIFY(count_to_depth(1) == 1);
	VERIFY(count_to_depth(2) == 1);
	VERIFY(count_to_depth(3) == 1);

	VERIFY(count_to_depth(BRANCHING_FACTOR + 1) == 2);
	VERIFY(count_to_depth(BRANCHING_FACTOR * BRANCHING_FACTOR) == 2);

	VERIFY(count_to_depth(BRANCHING_FACTOR * BRANCHING_FACTOR + 1) == 3);
	VERIFY(count_to_depth(BRANCHING_FACTOR * BRANCHING_FACTOR * BRANCHING_FACTOR) == 3);
}



namespace {

	/*
		Return how many steps to shift vector-index to get its *top-level* bits.

		-BRANCHING_FACTOR_SHIFT: empty tree
		0: leaf-node level
		BRANCHING_FACTOR_SHIFT: inode1 (inode that points to leafnodes)
		>=BRANCHING_FACTOR_SHIFT: inode that points to inodes.
	*/
	int vector_size_to_shift(size_t size){
		int shift = (count_to_depth(size) - 1) * BRANCHING_FACTOR_SHIFT;
		return shift;
	}

	template <class T>
	node_ref<T> make_leaf_node(const std::array<T, BRANCHING_FACTOR>& values){
		return node_ref<T>(new leaf_node<T>(values));
	}



	template <class T>
	node_ref<T> make_inode_from_vector(const std::vector<node_ref<T>>& children){
		ASSERT(children.size() >= 0);
		ASSERT(children.size() <= BRANCHING_FACTOR);

		std::array<node_ref<T>, BRANCHING_FACTOR> temp{};
		std::copy(children.begin(), children.end(), temp.begin());
		return node_ref<T>(new inode<T>(temp));
	}

	template <class T>
	node_ref<T> make_inode_from_array(const std::array<node_ref<T>, BRANCHING_FACTOR>& children){
		return node_ref<T>(new inode<T>(children));
	}


	/*
		Verifies the tree is valid.
		??? improve
	*/
	template <class T>
	bool tree_check_invariant(const node_ref<T>& tree, size_t size){
		ASSERT(tree.check_invariant());
	#if DEBUG
		if(size == 0){
			ASSERT(tree.get_type() == node_type::null_node);
		}
		else{
			ASSERT(tree.get_type() != node_type::null_node);
		}
	#endif
		return true;
	}



	//	### supply shift.
	template <class T>
	node_ref<T> find_leaf_node(const node_ref<T>& root_node, size_t size, int shift, size_t index){
		ASSERT(tree_check_invariant(root_node, size));
		ASSERT(index < size);

		node_ref<T> node_it = root_node;

		//	Traverse all inodes.
		while(shift > 0){
			size_t slot_index = (index >> shift) & BRANCHING_FACTOR_MASK;
			node_it = node_it.get_inode()->get_child(slot_index);
			shift -= BRANCHING_FACTOR_SHIFT;
		}

		ASSERT(shift == LEAF_NODE_SHIFT);
		ASSERT(node_it.get_type() == node_type::leaf_node);
		return node_it;
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
	node_ref<T> replace_value(const node_ref<T>& node, int shift, size_t index, const T& value){
		ASSERT(node.get_type() == node_type::inode || node.get_type() == node_type::leaf_node);

		const size_t slot_index = (index >> shift) & BRANCHING_FACTOR_MASK;
		if(shift == LEAF_NODE_SHIFT){
			ASSERT(node.get_type() == node_type::leaf_node);

			auto copy = node_ref<T>(new leaf_node<T>(node.get_leaf_node()->_values));
			ASSERT(slot_index < copy.get_leaf_node()->_values.size());
			copy.get_leaf_node()->_values[slot_index] = value;
			return copy;
		}
		else{
			ASSERT(node.get_type() == node_type::inode);

			const auto child = node.get_inode()->get_child(slot_index);
			auto child2 = replace_value(child, shift - BRANCHING_FACTOR_SHIFT, index, value);

			auto children = node.get_inode()->get_child_array();
			children[slot_index] = child2;
			auto copy = make_inode_from_array(children);
			return copy;
		}
	}


	template <class T>
	node_ref<T> make_new_path(int shift, const node_ref<T>& append){
		if(shift == LEAF_NODE_SHIFT){
			return append;
		}
		else{
			auto a = make_new_path(shift - BRANCHING_FACTOR_SHIFT, append);
			node_ref<T> b = make_inode_from_array<T>({ a });
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
	node_ref<T> append_leaf_node(const node_ref<T>& original, int shift, size_t index, const node_ref<T>& append){
		ASSERT(original.check_invariant());
		ASSERT(original.get_type() == node_type::inode);
		ASSERT(append.check_invariant());
		ASSERT(append.get_type() == node_type::leaf_node);

		size_t slot_index = (index >> shift) & BRANCHING_FACTOR_MASK;
		auto children = original.get_inode()->get_child_array();

		//	Lowest level inode, pointing to leaf nodes.
		if(shift == LOWEST_LEVEL_INODE_SHIFT){
			children[slot_index] = append;
			return make_inode_from_array<T>(children);
		}
		else {
			const auto child = children[slot_index];
			if(child.get_type() == node_type::null_node){
				node_ref<T> child2 = make_new_path(shift - BRANCHING_FACTOR_SHIFT, append);
				children[slot_index] = child2;
				return make_inode_from_array<T>(children);
			}
			else{
				node_ref<T> child2 = append_leaf_node(child, shift - BRANCHING_FACTOR_SHIFT, index, append);
				children[slot_index] = child2;
				return make_inode_from_array<T>(children);
			}
		}
	}


	template <class T>
	vector<T> push_back_1(const vector<T>& original, const T& value){
		ASSERT(original.check_invariant());

		const auto original_size = original.size();
		const auto original_shift = original.get_shift();

		if(original_size == 0){
			return vector<T>(make_leaf_node<T>({value}), 1, LEAF_NODE_SHIFT);
		}
		else{

			//	Does last leaf node have space left? Then we can use replace_value()...
			if((original_size & BRANCHING_FACTOR_MASK) != 0){
				const auto root = replace_value(original.get_root(), original_shift, original_size, value);
				return vector<T>(root, original_size + 1, original_shift);
			}

			//	Allocate new *leaf-node*, adding it to tree.
			else{
				const auto leaf = make_leaf_node<T>({ value });

				//	### no need to calculate shift2 from scratch, just compare (1 << original_shift) and (size + 1).
				auto shift2 = vector_size_to_shift(original_size + 1);

				//	Space left in root?
				if(shift2 == original_shift){
					const auto root = append_leaf_node(original.get_root(), original_shift, original_size, leaf);
					return vector<T>(root, original_size + 1, original_shift);
				}
				else{
					auto new_path = make_new_path(original_shift, leaf);
					auto new_root = make_inode_from_array<T>({ original.get_root(), new_path });
					return vector<T>(new_root, original_size + 1, shift2);
				}
			}
		}
	}


	/*
		Very fast if input vector is a multiple of BRANCHING_FACTOR big.
	*/
	template <class T>
	vector<T> push_back_leaf(const vector<T>& original, const std::array<T, BRANCHING_FACTOR>& values){
		ASSERT(original.check_invariant());

		const auto original_size = original.get_size();
		const auto original_shift = original.get_shift();

		if(original_size == 0){
			return vector<T>(make_leaf_node<T>(values), BRANCHING_FACTOR);
		}
		else{
			//	Slow path - if vector isn't aligned on BRANCHING_FACTOR.
			if((original_size & BRANCHING_FACTOR_MASK) != 0){
				vector<T> result = original;
				for(size_t i = 0 ; i < BRANCHING_FACTOR_MASK ; i++){
					result = result.push_back(values[i]);
				}
			}

			//	Fast path - make a new full leaf node and append it in one go.
			else{
				const auto leaf = make_leaf_node<T>(values);

				//	### no need to calculate shift2 from scratch, just compare (1 << original_shift) and (size + 1).
				auto shift2 = vector_size_to_shift(original_size + 1);

				//	Space left in root?
				if(shift2 == original_shift){
					const auto root = append_leaf_node(original.get_root(), original_shift, original_size, leaf);
					return vector<T>(root, original_size + BRANCHING_FACTOR);
				}
				else{
					auto new_path = make_new_path(original_shift, leaf);
					auto new_root = make_inode_from_array<T>({ original.get_root(), new_path });
					return vector<T>(new_root, original_size + BRANCHING_FACTOR);
				}
			}
		}
	}




	/*
		This is the central building block: adds many values to a vector (or a create a new vector) fast.
	*/
	template <class T>
	vector<T> push_back_batch(const vector<T>& original, const T values[], size_t count){
		ASSERT(original.check_invariant());

		vector<T> result = original;
		for(size_t i = 0 ; i < count ; i++){
			result = result.push_back(values[i]);
		}
		return result;
	}
#if false
	template <class T>
	vector<T> push_back_batch(const vector<T>& original, const T values[], size_t count){
		ASSERT(original.check_invariant());
		ASSERT(values != nullptr);

		vector<T> result = original;

		size_t source_pos = 0;

		/*
			1) If the last leaf node is partially filled, pad it out.
		*/
		{
			size_t last_leaf_size = original.size() & BRANCHING_FACTOR_MASK;
			size_t dest_pos = last_leaf_size;
			while(dest_pos < BRANCHING_FACTOR && source_pos < count){
			//	### Make a new replace_leaf_node() function and use it! Code is inside replace_value().
		//		node_ref<T> partial_leaf_node = find_leaf_node(const node_ref<T>& tree, size_t size, size_t index){

				result = result.push_back(values[source_pos]);
				dest_pos++;
				source_pos++;
			}
		}

		/*
			Append entire leaf nodes while there are enough source values.
		*/
		while((source_pos + BRANCHING_FACTOR) <= count){
		}

		/*
			3) If there are values left for a partial leaf node, make that leaf node now.
		*/
		{
		}


		if(_size == 0){
			return vector<T>(make_leaf_node<T>({value}), 1);
		}
		else{

			//	Does last leaf node have space left? Then we can use replace_value()...
			if((_size & BRANCHING_FACTOR_MASK) != 0){
				auto shift = vector_size_to_shift(_size);
				const auto root = replace_value(_root, shift, _size, value);
				return vector<T>(root, _size + 1);
			}

			//	Allocate new *leaf-node*, adding it to tree.
			else{
				const auto leaf = make_leaf_node<T>({ value });

				auto shift = vector_size_to_shift(_size);
				auto shift2 = vector_size_to_shift(_size + 1);

				//	Space left in root?
				if(shift2 == shift){
					const auto root = append_leaf_node(_root, shift, _size, leaf);
					return vector<T>(root, _size + 1);
				}
				else{
					auto new_path = make_new_path(shift, leaf);
					auto new_root = make_inode_from_array<T>({ _root, new_path });
					return vector<T>(new_root, _size + 1);
				}
			}
		}
	}

#endif



}






/////////////////////////////////////////////			vector






template <class T>
vector<T>::vector(){
	ASSERT(check_invariant());
}


template <class T>
vector<T>::vector(const std::vector<T>& values){
	//	!!! Illegal to take adress of first element of vec if it's empty.
	if(!values.empty()){
		auto temp = push_back_batch(vector<T>(), &values[0], values.size());
		temp.swap(*this);
	}

	ASSERT(size() == values.size());
	ASSERT(check_invariant());
}

template <class T>
vector<T>::vector(const T values[], size_t count){
	ASSERT(values != nullptr);

	auto temp = push_back_batch(vector<T>(), values, count);
	temp.swap(*this);

	ASSERT(size() == count);
	ASSERT(check_invariant());
}


template <class T>
vector<T>::vector(std::initializer_list<T> args){
	auto temp = push_back_batch(vector<T>(), args.begin(), args.end() - args.begin());
	temp.swap(*this);

	ASSERT(size() == args.size());
	ASSERT(check_invariant());
}


template <class T>
vector<T>::~vector(){
	ASSERT(check_invariant());

	_size = -1;
}


template <class T>
bool vector<T>::check_invariant() const{
	if(_root.get_type() == node_type::null_node){
		ASSERT(_size == 0);
	}
	else{
		ASSERT(_size >= 0);
	}
	ASSERT(tree_check_invariant(_root, _size));

	ASSERT(_shift >= EMPTY_TREE_SHIFT && _shift < 32);
	ASSERT(_shift == vector_size_to_shift(_size));

	return true;
}


template <class T>
vector<T>::vector(const vector& rhs){
	ASSERT(rhs.check_invariant());

	node_ref<T> newRef(rhs._root);

	_root = newRef;
	_size = rhs._size;
	_shift = rhs._shift;

	ASSERT(check_invariant());
}


template <class T>
vector<T>& vector<T>::operator=(const vector& rhs){
	ASSERT(check_invariant());
	ASSERT(rhs.check_invariant());

	vector<T> temp(rhs);
	temp.swap(*this);

	ASSERT(check_invariant());
	return *this;
}


template <class T>
void vector<T>::swap(vector& rhs){
	ASSERT(check_invariant());
	ASSERT(rhs.check_invariant());

	_root.swap(rhs._root);
	std::swap(_size, rhs._size);
	std::swap(_shift, rhs._shift);

	ASSERT(check_invariant());
	ASSERT(rhs.check_invariant());
}


template <class T>
vector<T>::vector(node_ref<T> root, std::size_t size, int shift) :
	_root(root),
	_size(size),
	_shift(shift)
{
	ASSERT(shift >= EMPTY_TREE_SHIFT);
	ASSERT(vector_size_to_shift(size) == shift);
	ASSERT(check_invariant());
}


template <class T>
int vector<T>::get_shift() const{
	ASSERT(check_invariant());

	return _shift;
}


template <class T>
vector<T> vector<T>::push_back(const T& value) const{
	ASSERT(check_invariant());

	return push_back_1(*this, value);
}


/*
	### Correct but inefficient.
*/
template <class T>
vector<T> vector<T>::pop_back() const{
	ASSERT(check_invariant());
	ASSERT(_size > 0);

	const auto temp = to_vec();
	const auto result = vector<T>(&temp[0], _size - 1);
	return result;
}


/*
	### Correct but inefficient.
*/
template <class T>
bool vector<T>::operator==(const vector& rhs) const{
	ASSERT(check_invariant());

	if(_size == rhs._size && _root._leaf == rhs._leaf && _root._inode == rhs._root._inode){
		return true;
	}
	else{
		//	### optimize by comparing leaf node by leaf node.
		//	First check leaf-node to see if they are the same pointer. If not, compary their values.
		const auto a = to_vec();
		const auto b = rhs.to_vec();
		return a == b;
	}
}



template <class T>
vector<T> vector<T>::assoc(size_t index, const T& value) const{
	ASSERT(check_invariant());
	ASSERT(index < _size);

	const auto root = replace_value(_root, _shift, index, value);
	return vector<T>(root, _size, _shift);
}


template <class T>
std::size_t vector<T>::size() const{
	ASSERT(check_invariant());

	return _size;
}


template <class T>
T vector<T>::operator[](const std::size_t index) const{
	ASSERT(check_invariant());
	ASSERT(index < _size);

	const auto leaf = find_leaf_node(_root, _size, _shift, index);
	const auto slot_index = index & BRANCHING_FACTOR_MASK;

	ASSERT(slot_index < leaf.get_leaf_node()->_values.size());
	const T result = leaf.get_leaf_node()->_values[slot_index];
	return result;
}

//	### Optimization potential here.
template <class T>
std::vector<T> vector<T>::to_vec() const{
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
	void trace_node(const std::string& prefix, const node_ref<T>& node){
		if(node.get_type() == node_type::null_node){
			TRACE_SS(prefix << "<null>");
		}
		else if(node.get_type() == node_type::inode){
			TRACE_SS(prefix << "<inode> RC: " << node.get_inode()->_rc);
			QUARK_SCOPED_INDENT();

			int index = 0;
			for(auto i: node.get_inode()->get_child_array()){
				trace_node("#" + std::to_string(index) + "\t", i);
				index++;
			}
		}
		else if(node.get_type() == node_type::leaf_node){
			TRACE_SS(prefix << "<leaf> RC: " << node.get_leaf_node()->_rc);
			QUARK_SCOPED_INDENT();

			int index = 0;
			for(auto i: node.get_leaf_node()->_values){
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
void vector<T>::trace_internals() const{
	ASSERT(check_invariant());

	TRACE_SS("Vector (size: " << _size << ") "
		"total inodes: " << inode<T>::_debug_count << ", "
		"total leaf nodes: " << leaf_node<T>::_debug_count);

	trace_node("", _root);
}


//	### Optimization potential here.
template <class T>
vector<T> operator+(const vector<T>& a, const vector<T>& b){
	vector<T> result;

#if 1
	if(b.empty()){
	}
	else{
		const auto v = b.to_vec();
		result = push_back_batch(a, &v[0], v.size());
	}
#else
	for(size_t i = 0 ; i < b.size() ; i++){
		result = result.push_back(b[i]);
	}
#endif

	ASSERT(result.size() == a.size() + b.size());
	return result;
}





////////////////////////////////////////////			Unit tests






void vector_test(const std::vector<int>& v){
}

QUARK_UNIT_TEST("std::vector<>", "auto convertion from initializer list", "", ""){
	std::vector<int> vi {1,2,3,4,5,6};

	vector_test(vi);

	vector_test(std::vector<int>{ 8, 9, 10});
	vector_test({ 8, 9, 10});
}

////////////////////////////////////////////			test_fixture<T>

/*
	Fixture class that you put on the stack in your unit tests.
	It makes sure the total count of leaf_node<T> and inode<T> are as expected - no leakage.
*/
template <class T>
struct test_fixture {
	test_fixture() :
		_scoped_tracer("test_fixture"),
		_inode_count(inode<T>::_debug_count),
		_leaf_count(leaf_node<T>::_debug_count)
	{
		TRACE_SS("inode count: " << _inode_count << " " << "Leaf node count: " << _leaf_count);
	}

	/*
		Use this constructor when you *expect* the number of nodes to have grown when test_fixture destructs.
	*/
	test_fixture(int inode_expected_count, int leaf_expected_count) :
		_scoped_tracer("test_fixture"),
		_inode_count(inode<T>::_debug_count),
		_leaf_count(leaf_node<T>::_debug_count),

		_inode_expected_count(inode_expected_count),
		_leaf_expected_count(leaf_expected_count)
	{
		TRACE_SS("inode count: " << _inode_count << " " << "Leaf node count: " << _leaf_count);
	}

	~test_fixture(){
		int inode_count = inode<T>::_debug_count;
		int leaf_count = leaf_node<T>::_debug_count;

		TRACE_SS("inode count: " << inode_count << " " << "Leaf node count: " << leaf_count);

		int inode_diff_count = inode_count - _inode_count;
		int leaf_expected_diff = leaf_count - _leaf_count;


		ASSERT(inode_diff_count == _inode_expected_count);
		ASSERT(leaf_expected_diff == _leaf_expected_count);
	}

	quark::scoped_trace _scoped_tracer;
	int _inode_count = 0;
	int _leaf_count = 0;

	int _inode_expected_count = 0;
	int _leaf_expected_count = 0;
};

QUARK_UNIT_TEST("", "test_fixture()", "no nodes", "no assert"){
	test_fixture<int> test;
	VERIFY(test._inode_count == 0);
	VERIFY(test._leaf_count == 0);
	VERIFY(test._inode_expected_count == 0);
	VERIFY(test._leaf_expected_count == 0);
}

QUARK_UNIT_TEST("", "test_fixture()", "1 inode, 2 leaf nodes", "correct state (and no assert!)"){
	test_fixture<int> test;

	std::unique_ptr<inode<int>> a;
	std::unique_ptr<leaf_node<int>> b;
	std::unique_ptr<leaf_node<int>> c;
	{
		test_fixture<int> test(1, 2);

		a.reset(new inode<int>({}));
		b.reset(new leaf_node<int>({}));
		c.reset(new leaf_node<int>({}));

		VERIFY(test._inode_count == 0);
		VERIFY(test._leaf_count == 0);
		VERIFY(test._inode_expected_count == 1);
		VERIFY(test._leaf_expected_count == 2);
	}
}




/*
	Generates a ramp from _start_ incrementing by +1. _count_ items.
	Resulting vector is always _total_count_ big, padded with 0s.

	total_count: total_count >= count
*/
std::vector<int> generate_numbers(int start, int count, int total_count){
	ASSERT(count >= 0);
	ASSERT(total_count >= count);

	std::vector<int> a;
	int i = 0;
	while(i < count){
		a.push_back(start + i);
		i++;
	}
	while(i < total_count){
		a.push_back(0);
		i++;
	}
	return a;
}

QUARK_UNIT_TEST("", "generate_numbers()", "5 numbers", "correct vector"){
	const auto a = generate_numbers(8, 4, 7);
	VERIFY(a == (std::vector<int>{ 8, 9, 10, 11, 0, 0, 0 }));
}

std::array<int, BRANCHING_FACTOR> generate_leaves(int start, int count){
	const auto a = generate_numbers(start, count, BRANCHING_FACTOR);

	std::array<int, BRANCHING_FACTOR> result;
	for(int i = 0 ; i < BRANCHING_FACTOR ; i++){
		result[i] = a[i];
	}
	return result;
}


/*
	Construct a vector that uses 1 leaf node.

	leaf_node
		value 0
*/
vector<int> make_manual_vector1(){
	test_fixture<int> f(0, 1);

	node_ref<int> leaf = make_leaf_node<int>({ 7 });
	return vector<int>(leaf, 1, LEAF_NODE_SHIFT);
}

QUARK_UNIT_TEST("", "make_manual_vector1()", "", "correct nodes"){
	test_fixture<int> f;

	const auto a = make_manual_vector1();
	VERIFY(a.size() == 1);
	VERIFY(a.get_root().get_type() == node_type::leaf_node);
	VERIFY(a.get_root().get_leaf_node()->_rc == 1);
	VERIFY(a.get_root().get_leaf_node()->_values[0] == 7);
	for(int i = 1 ; i < BRANCHING_FACTOR ; i++){
		VERIFY(a.get_root().get_leaf_node()->_values[i] == 0);
	}
}


/*
	Construct a vector that uses 1 leaf node and two values.

	leaf_node
		value 0
		value 1
*/
vector<int> make_manual_vector2(){
	test_fixture<int> f(0, 1);

	node_ref<int> leaf = make_leaf_node<int>({	7, 8	});
	return vector<int>(leaf, 2, LEAF_NODE_SHIFT);
}

QUARK_UNIT_TEST("", "make_manual_vector2()", "", "correct nodes"){
	test_fixture<int> f;
	const auto a = make_manual_vector2();
	VERIFY(a.size() == 2);
	VERIFY(a.get_root().get_type() == node_type::leaf_node);
	VERIFY(a.get_root().get_leaf_node()->_rc == 1);
	VERIFY(a.get_root().get_leaf_node()->_values[0] == 7);
	VERIFY(a.get_root().get_leaf_node()->_values[1] == 8);
	VERIFY(a.get_root().get_leaf_node()->_values[2] == 0);
	VERIFY(a.get_root().get_leaf_node()->_values[3] == 0);
}


/*
	Construct a vector that uses 1 inode and 2 leaf nodes.

	inode
		leaf_node
			value 0
			value 1
			... full
		leaf_node
			value x
*/
vector<int> make_manual_vector_branchfactor_plus_1(){
	test_fixture<int> f(1, 2);
	node_ref<int> leaf0 = make_leaf_node(generate_leaves(7, BRANCHING_FACTOR));
	node_ref<int> leaf1 = make_leaf_node(generate_leaves(7 + BRANCHING_FACTOR, 1));
	std::vector<node_ref<int>> leafs = { leaf0, leaf1 };
	node_ref<int> inode = make_inode_from_vector(leafs);
	return vector<int>(inode, BRANCHING_FACTOR + 1, vector_size_to_shift(BRANCHING_FACTOR + 1));
}

QUARK_UNIT_TEST("", "make_manual_vector_branchfactor_plus_1()", "", "correct nodes"){
	test_fixture<int> f;

	const auto a = make_manual_vector_branchfactor_plus_1();
	VERIFY(a.size() == BRANCHING_FACTOR + 1);

	VERIFY(a.get_root().get_type() == node_type::inode);
	VERIFY(a.get_root().get_inode()->_rc == 1);
	VERIFY(a.get_root().get_inode()->count_children() == 2);
	VERIFY(a.get_root().get_inode()->get_child(0).get_type() == node_type::leaf_node);
	VERIFY(a.get_root().get_inode()->get_child(1).get_type() == node_type::leaf_node);

	const auto leaf0 = a.get_root().get_inode()->get_child_as_leaf_node(0);
	VERIFY(leaf0->_rc == 1);
	VERIFY(leaf0->_values == generate_leaves(7 + BRANCHING_FACTOR * 0, BRANCHING_FACTOR));

	const auto leaf1 = a.get_root().get_inode()->get_child_as_leaf_node(1);
	VERIFY(leaf1->_rc == 1);
	VERIFY(leaf1->_values == generate_leaves(7 + BRANCHING_FACTOR * 1, 1));
}

/*
	Construct a vector using 2 levels of inodes.

	inode
		inode
			leaf_node 0
				value 0
				value 1
				... full
			leaf node 1
				value 0
				value 1
				... full
			leaf node 2
				value 0
				value 1
				... full
			... full
		inode
			leaf_node
*/
vector<int> make_manual_vector_branchfactor_square_plus_1(){
	test_fixture<int> f(3, BRANCHING_FACTOR + 1);

	std::vector<node_ref<int>> leaves;
	for(int i = 0 ; i < BRANCHING_FACTOR ; i++){
		node_ref<int> leaf = make_leaf_node(generate_leaves(1000 + BRANCHING_FACTOR * i, BRANCHING_FACTOR));
		leaves.push_back(leaf);
	}

	node_ref<int> extraLeaf = make_leaf_node(generate_leaves(1000 + BRANCHING_FACTOR * BRANCHING_FACTOR + 0, 1));

	node_ref<int> inodeA = make_inode_from_vector<int>(leaves);
	node_ref<int> inodeB = make_inode_from_vector<int>({ extraLeaf });
	node_ref<int> rootInode = make_inode_from_vector<int>({ inodeA, inodeB });
	const size_t size = BRANCHING_FACTOR * BRANCHING_FACTOR + 1;
	return vector<int>(rootInode, size, vector_size_to_shift(size));
}

QUARK_UNIT_TEST("", "make_manual_vector_branchfactor_square_plus_1()", "", "correct nodes"){
	test_fixture<int> f;

	const auto a = make_manual_vector_branchfactor_square_plus_1();
	VERIFY(a.size() == BRANCHING_FACTOR * BRANCHING_FACTOR + 1);

	node_ref<int> rootINode = a.get_root();
	VERIFY(rootINode.get_type() == node_type::inode);
	VERIFY(rootINode.get_inode()->_rc == 2);
	VERIFY(rootINode.get_inode()->count_children() == 2);
	VERIFY(rootINode.get_inode()->get_child(0).get_type() == node_type::inode);
	VERIFY(rootINode.get_inode()->get_child(1).get_type() == node_type::inode);

	node_ref<int> inodeA = rootINode.get_inode()->get_child(0);
		VERIFY(inodeA.get_type() == node_type::inode);
		VERIFY(inodeA.get_inode()->_rc == 2);
		VERIFY(inodeA.get_inode()->count_children() == BRANCHING_FACTOR);
		for(int i = 0 ; i < BRANCHING_FACTOR ; i++){
			const auto leafNode = inodeA.get_inode()->get_child_as_leaf_node(i);
			VERIFY(leafNode->_rc == 1);
			VERIFY(leafNode->_values == generate_leaves(1000 + BRANCHING_FACTOR * i, BRANCHING_FACTOR));
		}

	node_ref<int> inodeB = rootINode.get_inode()->get_child(1);
		VERIFY(inodeB.get_type() == node_type::inode);
		VERIFY(inodeB.get_inode()->_rc == 2);
		VERIFY(inodeB.get_inode()->count_children() == 1);
		VERIFY(inodeB.get_inode()->get_child(0).get_type() == node_type::leaf_node);

		const auto leaf4 = inodeB.get_inode()->get_child_as_leaf_node(0);
		VERIFY(leaf4->_rc == 1);
		VERIFY(leaf4->_values == generate_leaves(1000 + BRANCHING_FACTOR * BRANCHING_FACTOR + 0, 1));
}


////////////////////////////////////////////		vector::vector()


QUARK_UNIT_TEST("vector", "vector()", "", "no_assert"){
	test_fixture<int> f;

	vector<int> v;
	v.trace_internals();
}


////////////////////////////////////////////		vector::operator[]


QUARK_UNIT_TEST("vector", "operator[]", "1 value", "read back"){
	test_fixture<int> f;

	const auto a = make_manual_vector1();
	VERIFY(a[0] == 7);
	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "operator[]", "Branchfactor + 1 values", "read back"){
	test_fixture<int> f;
	const auto a = make_manual_vector_branchfactor_plus_1();
	VERIFY(a[0] == 7);
	VERIFY(a[1] == 8);
	VERIFY(a[2] == 9);
	VERIFY(a[3] == 10);
	VERIFY(a[4] == 11);
	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "operator[]", "Branchfactor^2 + 1 values", "read back"){
	test_fixture<int> f;
	const auto a = make_manual_vector_branchfactor_square_plus_1();
	VERIFY(a[0] == 1000);
	VERIFY(a[1] == 1001);
	VERIFY(a[2] == 1002);
	VERIFY(a[3] == 1003);
	VERIFY(a[4] == 1004);
	VERIFY(a[5] == 1005);
	VERIFY(a[6] == 1006);
	VERIFY(a[7] == 1007);
	VERIFY(a[8] == 1008);
	VERIFY(a[9] == 1009);
	VERIFY(a[10] == 1010);
	VERIFY(a[11] == 1011);
	VERIFY(a[12] == 1012);
	VERIFY(a[13] == 1013);
	VERIFY(a[14] == 1014);
	VERIFY(a[15] == 1015);
	VERIFY(a[16] == 1016);
	a.trace_internals();
}


////////////////////////////////////////////		vector::assoc()


QUARK_UNIT_TEST("vector", "assoc()", "1 value", "read back"){
	test_fixture<int> f;
	const auto a = make_manual_vector1();
	const auto b = a.assoc(0, 1000);
	VERIFY(a[0] == 7);
	VERIFY(b[0] == 1000);
	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "assoc()", "5 value vector, replace #0", "read back"){
	test_fixture<int> f;
	const auto a = make_manual_vector_branchfactor_plus_1();
	const auto b = a.assoc(0, 1000);
	VERIFY(a[0] == 7);
	VERIFY(b[0] == 1000);
}

QUARK_UNIT_TEST("vector", "assoc()", "5 value vector, replace #4", "read back"){
	test_fixture<int> f;
	const auto a = make_manual_vector_branchfactor_plus_1();
	const auto b = a.assoc(4, 1000);
	VERIFY(a[0] == 7);
	VERIFY(b[4] == 1000);
}

QUARK_UNIT_TEST("vector", "assoc()", "17 value vector, replace bunch", "read back"){
	test_fixture<int> f;
	auto a = make_manual_vector_branchfactor_square_plus_1();
	a = a.assoc(4, 1004);
	a = a.assoc(5, 1005);
	a = a.assoc(0, 1000);
	a = a.assoc(16, 1016);
	a = a.assoc(10, 1010);

	VERIFY(a[0] == 1000);
	VERIFY(a[4] == 1004);
	VERIFY(a[5] == 1005);
	VERIFY(a[16] == 1016);
	VERIFY(a[10] == 1010);

	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "assoc()", "5 value vector, replace value 10000 times", "read back"){
	test_fixture<int> f;
	auto a = make_manual_vector_branchfactor_plus_1();

	for(int i = 0 ; i < 1000 ; i++){
		a = a.assoc(4, i);
	}
	VERIFY(a[4] == 999);

	a.trace_internals();
}


////////////////////////////////////////////		vector::push_back()


vector<int> push_back_n(int count, int value0){
//	test_fixture<int> f;
	vector<int> a;
	for(int i = 0 ; i < count ; i++){
		a = a.push_back(value0 + i);
	}
	return a;
}

void test_values(const vector<int>& vec, int value0){
	test_fixture<int> f;

	for(int i = 0 ; i < vec.size() ; i++){
		const auto value = vec[i];
		const auto expected = value0 + i;
		VERIFY(value == expected);
	}
}

QUARK_UNIT_TEST("vector", "push_back()", "one value => 1 leaf node", "read back"){
	test_fixture<int> f;
	const vector<int> a;
	const auto b = a.push_back(4);
	VERIFY(a.size() == 0);
	VERIFY(b.size() == 1);
	VERIFY(b[0] == 4);
}

QUARK_UNIT_TEST("vector", "push_back()", "two values => 1 leaf node", "read back"){
	test_fixture<int> f;
	const vector<int> a;
	const auto b = a.push_back(4);
	const auto c = b.push_back(9);

	VERIFY(a.size() == 0);

	VERIFY(b.size() == 1);
	VERIFY(b[0] == 4);

	VERIFY(c.size() == 2);
	VERIFY(c[0] == 4);
	VERIFY(c[1] == 9);
}

QUARK_UNIT_TEST("vector", "push_back()", "1 inode", "read back"){
	test_fixture<int> f;
	const auto count = BRANCHING_FACTOR + 1;
	vector<int> a = push_back_n(count, 1000);
	VERIFY(a.size() == count);
	test_values(a, 1000);
	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "push_back()", "1 inode + add leaf to last node", "read back all values"){
	test_fixture<int> f;
	const auto count = BRANCHING_FACTOR + 2;
	vector<int> a = push_back_n(count, 1000);
	VERIFY(a.size() == count);
	test_values(a, 1000);
	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "push_back()", "2-levels of inodes", "read back all values"){
	test_fixture<int> f;
	const auto count = BRANCHING_FACTOR * BRANCHING_FACTOR + 1;
	vector<int> a = push_back_n(count, 1000);
	VERIFY(a.size() == count);
	test_values(a, 1000);
	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "push_back()", "2-levels of inodes + add leaf-node to last node", "read back all values"){
	test_fixture<int> f;
	const auto count = BRANCHING_FACTOR * BRANCHING_FACTOR * 2;
	vector<int> a = push_back_n(count, 1000);
	VERIFY(a.size() == count);
	test_values(a, 1000);
	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "push_back()", "3-levels of inodes + add leaf-node to last node", "read back all values"){
	test_fixture<int> f;
	const auto count = BRANCHING_FACTOR * BRANCHING_FACTOR * BRANCHING_FACTOR * 2;
	vector<int> a = push_back_n(count, 1000);
	VERIFY(a.size() == count);
	test_values(a, 1000);
	a.trace_internals();
}


////////////////////////////////////////////		vector::pop_back()


QUARK_UNIT_TEST("vector", "pop_back()", "basic", "correct result vector"){
	test_fixture<int> f;
	const auto data = generate_numbers(4, 50, 50);
	const auto data2 = std::vector<int>(&data[0], &data[data.size() - 1]);
	const auto a = vector<int>(data);
	const auto b = a.pop_back();
	VERIFY(b.to_vec() == data2);
}


////////////////////////////////////////////		vector::operator==()


QUARK_UNIT_TEST("vector", "operator==()", "empty vs empty", "true"){
	test_fixture<int> f;

	const std::vector<int> a;
	const std::vector<int> b;
	VERIFY(a == b);
}

QUARK_UNIT_TEST("vector", "operator==()", "empty vs 1", "false"){
	test_fixture<int> f;

	const std::vector<int> a;
	const std::vector<int> b{ 33 };
	VERIFY(!(a == b));
}

QUARK_UNIT_TEST("vector", "operator==()", "1000 vs 1000", "true"){
	test_fixture<int> f;
	const auto data = generate_numbers(4, 50, 50);
	const std::vector<int> a(data);
	const std::vector<int> b(data);
	VERIFY(a == b);
}

QUARK_UNIT_TEST("vector", "operator==()", "1000 vs 1000", "false"){
	test_fixture<int> f;
	const auto data = generate_numbers(4, 50, 50);
	auto data2 = data;
	data2[47] = 0;

	const std::vector<int> a(data);
	const std::vector<int> b(data2);
	VERIFY(!(a == b));
}


////////////////////////////////////////////		vector::size()


QUARK_UNIT_TEST("vector", "size()", "empty vector", "0"){
	test_fixture<int> f;
	vector<int> v;
	VERIFY(v.size() == 0);
}

QUARK_UNIT_TEST("vector", "size()", "BranchFactorSquarePlus1", "BranchFactorSquarePlus1"){
	test_fixture<int> f;
	const auto a = make_manual_vector_branchfactor_square_plus_1();
	VERIFY(a.size() == BRANCHING_FACTOR * BRANCHING_FACTOR + 1);
}


////////////////////////////////////////////		vector::vector(const std::vector<T>& vec)


QUARK_UNIT_TEST("vector", "vector(const std::vector<T>& vec)", "0 values", "empty"){
	test_fixture<int> f;
	const std::vector<int> a = {};
	vector<int> v(a);
	VERIFY(v.size() == 0);
}

QUARK_UNIT_TEST("vector", "vector(const std::vector<T>& vec)", "7 values", "read back all"){
	test_fixture<int> f;
	const std::vector<int> a = {	3, 4, 5, 6, 7, 8, 9	};
	vector<int> v(a);
	VERIFY(v.size() == 7);
	VERIFY(v[0] == 3);
	VERIFY(v[1] == 4);
	VERIFY(v[2] == 5);
	VERIFY(v[3] == 6);
	VERIFY(v[4] == 7);
	VERIFY(v[5] == 8);
	VERIFY(v[6] == 9);
}


////////////////////////////////////////////		vector::vector(const T values[], size_t count)


QUARK_UNIT_TEST("vector", "vector(const T values[], size_t count)", "0 values", "empty"){
	test_fixture<int> f;
	const int a[] = {};
	vector<int> v(&a[0], 0);
	VERIFY(v.size() == 0);
}

QUARK_UNIT_TEST("vector", "vector(const T values[], size_t count)", "7 values", "read back all"){
	test_fixture<int> f;
	const int a[] = {	3, 4, 5, 6, 7, 8, 9	};
	vector<int> v(&a[0], 7);
	VERIFY(v.size() == 7);
	VERIFY(v[0] == 3);
	VERIFY(v[1] == 4);
	VERIFY(v[2] == 5);
	VERIFY(v[3] == 6);
	VERIFY(v[4] == 7);
	VERIFY(v[5] == 8);
	VERIFY(v[6] == 9);
}


////////////////////////////////////////////		vector::vector(std::initializer_list<T> args)


QUARK_UNIT_TEST("vector", "vector(std::initializer_list<T> args)", "0 values", "empty"){
	test_fixture<int> f;
	vector<int> v = {};
	VERIFY(v.size() == 0);
}

QUARK_UNIT_TEST("vector", "vector(std::initializer_list<T> args)", "7 values", "read back all"){
	test_fixture<int> f;
	vector<int> v = {	3, 4, 5, 6, 7, 8, 9	};
	VERIFY(v.size() == 7);
	VERIFY(v[0] == 3);
	VERIFY(v[1] == 4);
	VERIFY(v[2] == 5);
	VERIFY(v[3] == 6);
	VERIFY(v[4] == 7);
	VERIFY(v[5] == 8);
	VERIFY(v[6] == 9);
}


////////////////////////////////////////////		vector::to_vec()


QUARK_UNIT_TEST("vector", "to_vec()", "0", "empty"){
	test_fixture<int> f;
	const auto a = vector<int>();
	VERIFY(a.to_vec() == std::vector<int>());
}

QUARK_UNIT_TEST("vector", "to_vec()", "50", "correct data"){
	test_fixture<int> f;
	const auto data = generate_numbers(4, 50, 50);
	const auto a = vector<int>(data);
	VERIFY(a.to_vec() == data);
}


////////////////////////////////////////////		vector::vector(const vector& rhs)


QUARK_UNIT_TEST("vector", "vector(const vector& rhs)", "empty", "empty"){
	test_fixture<int> f;
	const auto a = vector<int>();
	const auto b(a);
	VERIFY(a.empty());
	VERIFY(b.empty());
}

template <class T>
bool same_root(const vector<T>& a, const vector<T>& b){
	if(a.get_root().get_type() == node_type::inode){
		return a.get_root().get_inode() == b.get_root().get_inode();
	}
	else{
		return a.get_root().get_leaf_node() == b.get_root().get_leaf_node();
	}
}

QUARK_UNIT_TEST("vector", "vector(const vector& rhs)", "7 values", "identical, sharing root"){
	test_fixture<int> f;
	const auto data = std::vector<int>{	3, 4, 5, 6, 7, 8, 9	};
	const vector<int> a = data;
	const auto b(a);

	VERIFY(a.to_vec() == data);
	VERIFY(b.to_vec() == data);
	VERIFY(same_root(a, b));
}



////////////////////////////////////////////		vector::operator=()


QUARK_UNIT_TEST("vector", "operator=()", "empty", "empty"){
	test_fixture<int> f;
	const auto a = vector<int>();
	auto b = vector<int>();

	b = a;

	VERIFY(a.empty());
	VERIFY(b.empty());
}

QUARK_UNIT_TEST("vector", "operator=()", "7 values", "identical, sharing root"){
	test_fixture<int> f;
	const auto data = std::vector<int>{	3, 4, 5, 6, 7, 8, 9	};
	const vector<int> a = data;
	auto b = vector<int>();

	b = a;

	VERIFY(a.to_vec() == data);
	VERIFY(b.to_vec() == data);
	VERIFY(same_root(a, b));
}


////////////////////////////////////////////		operator+()


QUARK_UNIT_TEST("vector", "operator+()", "3 + 4 values", "7 values"){
	test_fixture<int> f;
	const vector<int> a{ 2, 3, 4 };
	const vector<int> b{ 5, 6, 7, 8 };

	const auto c = a + b;

	VERIFY(c.to_vec() == (std::vector<int>{ 2, 3, 4, 5, 6, 7, 8 }));
}

}	//	steady

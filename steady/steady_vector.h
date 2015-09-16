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

namespace steady {

	//	Change kBranchingFactorShift to get different branching factors. Number of bits per inode.
	static const int kBranchingFactorShift = 3;

	namespace internals {
		static const int kBranchingFactor = 1 << kBranchingFactorShift;
		static const size_t kBranchingFactorMask = (kBranchingFactor - 1);

		template <typename T> struct NodeRef;
		template <typename T> struct INode;
		template <typename T> struct LeafNode;

		enum class NodeType {
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

	}


////////////////////////////////////////////		steady_vector

/*
	Persistent vector class.

	Vector object is immutable = can never be changed. This makes for robust code and thread safety etc.

	When you modify the vector you always get a copy of the vector with yours changes integrated.

	Internally the new and old vectors shares most state so this is fast and uses little memory.


	Based on Clojure's vector.
*/


template <class T>
class steady_vector {
	/*
		Makes empty vector.
		No memory allocation.
		O(1)
	*/
	public: steady_vector();

	/*
		Makes vector containing values from a std::vector<>.
		Allocates memory.

		values: zero -> many values.
		this: on exit this holds the new vector
	*/
	public: steady_vector(const std::vector<T>& values);

	/*
		Makes vector containing _count_ values from _values_.
		Values are copied into the vector.
		Allocates memory.

		values: must not be nullptr, not even when count == 0
		count: >= 0
		this: on exit this holds the new vector
	*/
	public: steady_vector(const T values[], size_t count);

	/*
		C++11 initializer-list constructor. Allows you to write
			const steady_vector<int>({ 1, 2, 3 });

		Allocates memory.

		args: a C++11 initializer list object that specifies the values for the new vector
		this: on exit this holds the new vector
	*/
	public: steady_vector(std::initializer_list<T> args);

	/*
		no-throw
	*/
	public: ~steady_vector();

	/*
		Development feature: validates the internal state of the vector and calls ASSERT on defects.
		use like:
			ASSERT(myVector.check_invariant());

		this: on exit, this will hold a copy of _rhs_
		return: true = internal state is correct, false if not.
	*/
	public: bool check_invariant() const;

	/*
		Copies a vector object.
		Extremely fast since it shares the entire state with rhs, just updated the reference counter.
		No memory allocation.
		O(1)

		this: on exit, this will hold a copy of _rhs_
		rhs: vector to copy.
	*/
	public: steady_vector(const steady_vector& rhs);

	/*
		Same as copy-constructor.
		Your existing variable holding the vector will be changd to hold vector rhs instead.
		Notice: this function may appear to mutate the instance, but it doesn't.
		No memory allocation.
		O(1)

		this: on entry this is the destination vector, on exit, it will hold _rhs_.
		rhs: source vector
	*/
	public: steady_vector& operator=(const steady_vector& rhs);

	/*
		The variable holding your vector will be changed to hold the vector specified by _rhs_ and vice versa.
		The vector objects are not mutated, they just switch place.
		O(1)
		No-throw guarantee.

		this: on entry this holds vector A, on exit it holds vector B
		rhs: on entry this holds vector B, on exit it holds vector A
	*/
	public: void swap(steady_vector& rhs);

	/*
		Store value into the vector
		Old vector will not be changed, instead a new vector with the store will be returned.
		The new and old vector share most internal state.
		O(1) ... almost

		this: input vector
		index: 0 -> (size - 1)
		value: new value to store
		return: new copy of the vector with _value_ stored at the _index_.
	*/
	public: steady_vector assoc(size_t index, const T& value) const;

	/*
		Append value to the end of the vector, returning a vector with size + 1.
		Old vector will not be changed, instead a new vector with the store will be returned.
		The new and old vector share most internal state.
		O(1) ... almost

		value: value to append.
		return: new copy of the vector, with _value_ tacked to the end. It will be 1 bigger than the input vector.
	*/
	public: steady_vector push_back(const T& value) const;

	/*
		Remove last value in the vector, returning a vector with size - 1.

		this: must have size() > 0
		return: new copy of the vector, with the last values removed. It will be 1 smaller than input vector.
	*/
	public: steady_vector pop_back() const;

	/*
		Returns true if vectors are equivalent.

		Worst case is O(n) but performance is better when sharing is detected between vectors.
		Best case: O(1)

		### Warning: current implementation copies memory, allocates memory and can throw exceptions.

		return: true if vectors have the same size and operator==() on every value are true
	*/
	public: bool operator==(const steady_vector& rhs) const;
	public: bool operator!=(const steady_vector& rhs) const{
		return !(*this == rhs);
	}

	/*
		How many values does the vector hold?

		this: vector
		return: the number of values in the vector. 0 ->
	*/
	public: std::size_t size() const;

	public: bool empty() const{
		return size() == 0;
	}

	/*
		Get value at index
		O(1) ... almost

		this: input vector
		index: 0 -> (size - 1)
		return: value at index _index_.
	*/
	public: T operator[](const std::size_t index) const;

	public: std::vector<T> to_vec() const;



	///////////////////////////////////////		Internals

	/*
		Traces a diagram of the structure of the vector using TRACE.
	*/
	public: void trace_internals() const;

	public: const internals::NodeRef<T>& GetRoot() const{
		return _root;
	}
	public: steady_vector(internals::NodeRef<T> root, std::size_t size);



	///////////////////////////////////////		State

	private: internals::NodeRef<T> _root;
	private: std::size_t _size;
};

}	//	steady
	
#endif /* defined(__steady__steady_vector__) */

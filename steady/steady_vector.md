# steady::vector<T>



## vector()
Makes empty vector.

- No memory allocation.
- Never throws exceptions
- O(1)



## vector(const std::vector<T>& values)
Makes a vector containing the values from a std::vector<>.

- Allocates memory.
- O(n)
- Throws exceptions.

**Arguments**

- values: input values to copy. The size is [0 <= count < UINT32_MAX] values.
- this: on exit this holds the new vector




## vector(const T values[], size_t count)

Makes vector containing _count_ values copied from _values_-array.

- Allocates memory.
- O(n)
- Throws exceptions

**Arguments**

- values: must not be nullptr, not even when count == 0
- count: [0 <= count < UINT32_MAX]
- this: on exit this holds the new vector




## vector(std::initializer_list<T> args)

C++11 initializer-list constructor. Allows you to write:

```
const vector<int>({ 1, 2, 3 });
```

- Allocates memory.
- O(n)
- Throws exceptions

**Arguments**

-   args: a C++11 initializer list object that specifies the values for the new vector
-    this: on exit this holds the new vector




## ~vector()
Destructs the vector.

- Never throws exceptions




## bool check_invariant() const
Development feature: validates the internal state of the vector and calls STEADY_ASSERT on defects. Use like:

```
    void my_function(steady::vector<string>& my_vector){
        STEADY_ASSERT(my_vector.check_invariant());
        ...
    }
```




## vector(const vector& rhs)

Copies a vector object. Extremely fast since it shares the entire state with rhs, it just updates a reference counter.

- No memory allocation.
- O(1)
- Never throws exceptions

**Arguments**

- this: on exit, this will hold a copy of _rhs_
- rhs: vector to copy.




## vector& operator=(const vector& rhs)

Same as copy-constructor. Your existing variable holding the vector will be changed to hold vector rhs instead.

- No memory allocation.
- O(1)
- Never throws exceptions

**Arguments**

- this: on entry this is the destination vector, on exit, it will hold _rhs_.
- rhs: source vector
- return: the new vector




## void swap(vector& rhs)
The variable holding your vector will be changed to hold the vector specified by _rhs_ and vice versa. The vector objects are not mutated, they just switch place.

- No memory allocation.
- O(1)
- No-throw guarantee.

**Arguments**

- this: on entry this holds vector A, on exit it holds vector B
- rhs: on entry this holds vector B, on exit it holds vector A




## vector store(size_t index, const T& value) const
Store value into the vector at the specified index. Old vector will not be changed, instead a new vector with the modification will be returned.
The new and old vector share most internal state.

- Allocates memory
- O(1) ... almost
- Throws exceptions

**Arguments**

- this: input vector
- index: [0 <= index < size())
- value: new value to store
- return: new copy of the vector with _value_ stored at the _index_.




## vector push_back(const T& value) const
Append value to the end of the vector, returning a vector with size + 1. Old vector will not be changed, instead a new, updated vector will be returned.
The new and old vector share most internal state.

- Allocates memory
- O(1) ... almost. It never copies the entire vector.
- Throws exceptions

**Arguments**

- value: value to append.
- return: new copy of the vector, with _value_ tacked to the end. It will be 1 bigger than the input vector.




## vector push_back(const std::vector<T>& values) const
Appends all values in _values_ to the vector. This is faster than adding one item at a time.

- Allocates memory
- Time complexity depends only on the size of _values_.
- Throws exceptions

**Arguments**

- values: std::vector of values to append.
- return: new copy of the vector, with _values_ tacked to the end. It will be 1 bigger than the input vector.


template <class T>
## vector push_back(const T values[], size_t count) const
Appends the values values in _values_ to the vector. This is faster than adding one item at a time.

- Allocates memory
- Time complexity depends only on the size specified by _count_.
- Throws exceptions

**Arguments**

- values: pointer to a number of values to append. Cannot be nullptr, even when count == 0.
- count: [0 <= count < UMAX32_t] number of values to copy from _values_.
- return: new copy of the vector, with _values_ tacked to the end. It will be 1 bigger than the input vector.










## vector pop_back() const
Remove last value in the vector, returning a vector with size - 1.

**WARNING: The current implementation of this function works fine but is naive and inefficient.**

- Allocates memory
- O(n)
- Throws exceptions

**Arguments**

- this: must have size() > 0
- return: new copy of the vector, with the last values removed. It will be 1 smaller than input vector.




## bool operator==(const vector& rhs) const
Returns true if vectors are equivalent.

- No memory allocation.
- Worst case is O(n) but performance is better when sharing is detected between vectors. Best case: O(1)
- Never throws exceptions

**Arguments**

- this: vector A
- rhs: vector B
- return: true if A and B have the same size and operator==() on each value then contain are true.



## bool operator!=(const vector& rhs) const
Same as operator== but inverted result.




## std::size_t size() const
How many values does the vector contain?

- No memory allocation.
- O(1)
- Never throws exceptions

**Arguments**

- this: vector
- return: the number of values in the vector. [0 <= return < UINT32_MAX]




## bool empty() const
Shortcut for (size() == 0).




## T operator\[\](std::size_t index) const
Get value at index.

- No memory allocation
- O(1) ... almost
- Throws exceptions

**Arguments**

- this: input vector
- index: [0 <= index < size]
- return: value at _index_.




## std::vector<T> to_vec() const
Copies all values into a std::vector and returns it.

- Allocates memory
- O(n)
- Throws exceptions



## size_t get_block_count() const
Returns how many "blocks" the vector is stored in, rounded up to multiples of BRANCHING_FACTOR.

- No memory allocation
- O(1)
- Never throws exceptions



## const T* get_block(size_t block_index) const
Returns a constant pointer directly into the vector's internal storage. This isn't as scary as it first may seem, since the vector will never change. Make sure you do not keep this pointer after vector is destructed.

This is a way to very quickly read large amounts of data from a vector, without using operator[] for each value, or use to_vec() which copies all values.

All blocks except the last one are guaranteed to be full with values. Last block may be partial if vector isn't multiple of block size.
You can only call this function when get_block_count() returns > 0.

- No memory allocation
- O(1)
- Never throws exceptions

**Arguments**

- this: input vector
- index: [0 <= index < get_block_count()]
- return: pointer to a continous block of up to BRANCHING_FACTOR values.





## vector<T> operator+(const vector<T\>& a, const vector<T\>& b)
Appends two vectors and returns a new one.

- Allocates memory
- O(n)
- Throws exceptions

**Arguments**

- a: input vector a
- b: input vector b
- return: new vector with values from a followed by values from b.






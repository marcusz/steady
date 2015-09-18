# STEADY::VECTOR<T>
This is a fast and reliable persistent vector class for C++. It is also thread safe.

"Persistent" means that all objects are immutable and modifications return new objects. Internal state is shared
between generations of vectors, using atomic reference counting. Since vectors never change, there is no need
for thread synchronization.


When you "modify" the vector you always get a copy of the vector with your changes integrated.
Internally, the new and old vectors shares most state so this is very fast and uses little memory.

	//	Make a vector of ints. Add a few numbers.
	//	Notice that push_back() returns a new vector each time - you need to save the return value.
	//	There are no side effects. This makes code very simple and solid.
	//	It also makes it simple to design pure functions.
	void example1(){
		steady::vector<int> a;
		a.push_back(3);
		a.push_back(8);
		a.push_back(11);

		//	Notice! a is still the empty vector! It has not changed!
		assert(a.size() == 0);

		//	Reuse variable b to keep the latest generation of the vector.
		steady::vector<int> b;
		b = b.push_back(3);
		b = b.push_back(8);
		b = b.push_back(11);

		assert(b.size() == 3);
		assert(b[2] == 11);
	}


Apache License, Version 2.0
Based on Clojure's magical persistent vector class. Does not yet use the tail-optimization.
Strong exception-safety guarantee, just like C++ standad library and boost.


# COMPARISON TO C++ VECTOR (std::vector<>)

PROs

1) More robust, easy-to-understand and side-effect free code since vectors never change.

2) Easier to implement pure function - functions that have no side-effects and still have good performance.
	Pure functions are central to making reliable, testable and multithreaded code.

3) Since the vector never changes, it is safe to use it from many threads = thread safe.

4) Faster than std::vector<> when growing big vectors.


CONs

1) Slower reading and writing. (There are techniques - like batching - to avoid some of the overhead.

2) Allocates several memory blocks for bigger vectors where std::vector<> only has maximum of 1.

3) Not complete set of std C++ features, like iterators.

4) Not 100% swap-in replacement for std::vector<>.

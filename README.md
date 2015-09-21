# steady::vector<T>
This is a fast and reliable persistent (immutable) vector class for C++. You can safely share these vectors between threads in multi-threaded programs.

"Persistent" means that all vectors are immutable and when you "modify" the vector you get a copy of it with your changes integrated. Internally, the different generations of the vector object shares most state so this is very fast and uses little memory. This is done using atomic reference counting.

Since vectors never change, there is no need for thread synchronization like mutexes!

Basic example:

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

Writing to a vector:

	//	Replace values in the vector. This also leaves original vector unchanged.
	void example3(){
		const steady::vector<int> a{ 10, 20, 30 };
		const auto b = a.store(0, 2010);
		const auto c = b.store(2, 2030);

		assert(a == (steady::vector<int>{ 10, 20, 30 }));
		assert(b == (steady::vector<int>{ 2010, 20, 30 }));
		assert(c == (steady::vector<int>{ 2010, 20, 2030 }));
	}


- Apache License, Version 2.0

- Based on Clojure's magical persistent vector class and Phil Bagwells work. Does not yet use Clojure's tail-optimization.

- Strong exception-safety guarantee, just like C++ standard library and boost.

- Robust and heavily unit-tested implementation.


# Comparison to C++ std::vector<>

PRO:s

1) More robust, easy-to-understand and side-effect free code since you *know* vectors never change.

2) Since the vector never changes, it is safe to use it from many threads = thread safe.

3) Easier to implement pure function - functions that have no side-effects - and still have good performance.
	Pure functions are central to making reliable, testable and multithreaded code.

4) Faster than std::vector<> when growing big vectors.


CON:s

1) Somewhat slower reading and writing. (There are techniques - like batching - to avoid some of this overhead.)

2) Not complete set of std C++ features, like iterators.

3) Not a 100% drop-in replacement for std::vector<>.
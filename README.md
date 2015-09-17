# steady

This is a fast, persistent and thread-safe vector class for C++. "Persistent" means that all modifications returns new objects, original object is unchanged.

Internal state is shared between generations of vectors. Uses reference counting internally using std::atomic>.

Based on Clojure's magical vector.

//
//  Range.cpp
//  Permafrost
//
//  Created by Marcus Zetterquist on 2014-02-02.
//  Copyright (c) 2014 Marcus Zetterquist. All rights reserved.
//

#include "Range.h"
#include "cpp_extension.h"


namespace {

	struct CMyContainer : public irange_support<int> {
		public: virtual const int& irange_support_get_at_pos(size_type iIndex) const{
			ASSERT(iIndex >= 0 && iIndex < fInts.size());
			return fInts[iIndex];
		}


		////////////////		State
			std::vector<int> fInts;
	};

}



template <typename T>
struct TRange {
	const T& front() const{
		ASSERT(check_invariant());

		return fContainer->irange_support_get_at_pos(fStart);
	}
	void pop_front(){
		ASSERT(check_invariant());

		ASSERT(fStart < fEnd);
		fStart++;
	}
	bool is_empty() const{
		ASSERT(check_invariant());

		return fStart == fEnd;
	}


	long get_front_linear_range_size() const {
		ASSERT(check_invariant());

		return fEnd - fStart;
	}

	const T* front_linear_range_ptr() const {
		ASSERT(check_invariant());

		return &fContainer->irange_support_get_at_pos(fStart);
	}

	void pop_front_linear_range(long iCount){
		ASSERT(check_invariant());
		ASSERT((fStart + iCount) <= fEnd);

		fStart += iCount;
	}

	bool check_invariant() const {
		ASSERT(this != nullptr);
		ASSERT(fContainer != nullptr);
		ASSERT(fStart <= fEnd);
		return true;
	}

/*
	TRange(const CMyContainer& iContainer, long iStart, long iEnd) :
		fContainer(iContainer),
		fStart(iStart),
		fEnd(iEnd)
	{
		ASSERT(iStart >=0);
		ASSERT(iEnd >= iStart);

		ASSERT(check_invariant());
	}
*/

	friend TRange<int> MakeRange(const CMyContainer& iContainer, long iStart, long iEnd);


	//////////////		State
		private: const irange_support<T>* fContainer;
		private: long fStart;
		private: long fEnd;
};

	TRange<int> MakeRange(const CMyContainer& iContainer, long iStart, long iEnd);


	TRange<int> MakeRange(const CMyContainer& iContainer, long iStart, long iEnd){
		TRange<int> r;
		r.fStart = iStart;
		r.fEnd = iEnd;
		r.fContainer = &iContainer;
		return r;
	}



UNIT_TEST("Range", "", "", ""){
	CMyContainer container;
	container.fInts = {	20, 21, 22 };
	UT_VERIFY(container.fInts[0] == 20);
	UT_VERIFY(container.fInts[1] == 21);
	UT_VERIFY(container.fInts[2] == 22);
}


UNIT_TEST("Range", "", "", ""){
	CMyContainer container;
	container.fInts = {	20, 21, 22, 23, 24 };

	TRange<int> r = MakeRange(container, 1, 4);
	{
		UT_VERIFY(!r.is_empty());
		UT_VERIFY(r.front() == 21);

		r.pop_front();
		UT_VERIFY(!r.is_empty());
		UT_VERIFY(r.front() == 22);

		r.pop_front();
		UT_VERIFY(!r.is_empty());
		UT_VERIFY(r.front() == 23);

		r.pop_front();
		UT_VERIFY(r.is_empty());
	}
}


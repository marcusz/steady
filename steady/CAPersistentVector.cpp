//
//  CAPersistentVector.cpp
//  Perma App
//
//  Created by Marcus Zetterquist on 2014-03-10.
//  Copyright (c) 2014 Marcus Zetterquist. All rights reserved.
//

#include "CAPersistentVector.h"

#include <vector>

namespace NSAPersistentVector {



template <typename T>
std::vector<std::pair<leaf_node<T>, ref_count> > pool<T>::_leaf_nodes;

template <typename T>
std::vector<std::pair<inode<T>, ref_count> > pool<T>::_inodes;


/**
	32-bit addressindex, split into 5-bit chunks - one chunk for each level in the tree.
	If indexes go from 0-31, only a single level (5 bits address) is needed etc.

	level:	11    10    9     8     7     6     5     4     3     2     1
	bits:	---00 00000 00000 00000 00000 00000 00000 00000 00000 00000 00000
			                                                      index index
			                                                      in    in
			                                                      bottm leaf
			                                                      inode
	max size
	0		-----------------------------------------------------------------	empty vector
	32		------------------------------------------------------------LLLLL	only leaf node
	1024	------------------------------------------------------11111 LLLLL	1 level of inodes = 1 inode + 1-32 leafs
			------------------------------------------------22222 11111 LLLLL	2 levels of inodes = 2-33 inodes + 1-32 leafs

	??? Need to handle level 11 too, even though it has only 2 bits address.
*/


int maxindex_to_depth(std::size_t maxindex){
	if(maxindex < 32){
		return 1;
	}
	else{
		return 1 + maxindex_to_depth(maxindex >> 5);
	}
}

int size_to_depth(std::size_t size){
	if(size == 0){
		return 0;
	}
	else{
		return maxindex_to_depth(size - 1);
	}
}



}	//	NSAPersistentVector


using namespace NSAPersistentVector;


UNIT_TEST("CAPersistentVector", "calc_depth()", "", ""){
	UT_VERIFY(size_to_depth(0) == 0);

	UT_VERIFY(size_to_depth(1) == 1);
	UT_VERIFY(size_to_depth(32) == 1);

	UT_VERIFY(size_to_depth(33) == 2);
	UT_VERIFY(size_to_depth(1024) == 2);

	UT_VERIFY(size_to_depth(1025) == 3);
	UT_VERIFY(size_to_depth(32768) == 3);

	UT_VERIFY(size_to_depth(32769) == 4);
}


UNIT_TEST("CAPersistentVector", "pvector::pvector()", "Most basic construction", "empty() == true"){
	const pvector<int> a;
	UT_VERIFY(a.empty());
}




UNIT_TEST("CAPersistentVector", "pvector::pvector()", "empty", "empty() == true"){
	const pvector<int> a({});
	UT_VERIFY(a.empty());
}

UNIT_TEST("CAPersistentVector", "pvector::pvector()", "3 items", "operator[] can give contents"){
	const pvector<int> a({ 91, 92, 93 });
	UT_VERIFY(a[0] == 91);
	UT_VERIFY(a[1] == 92);
	UT_VERIFY(a[2] == 93);
}



UNIT_TEST("CAPersistentVector", "pvector::push_back()", "Add 98 to empty vector", "operator[] can read-back"){
	const pvector<int> a;
	pvector<int> b = a.push_back(98);

	UT_VERIFY(b[0] == 98);
}


UNIT_TEST("CAPersistentVector", "pvector::push_back()", "Add 98, 99, 100 to empty vector", "operator[] can read-back"){
	const pvector<int> a;
	pvector<int> b = a.push_back(98);
	b = b.push_back(99);
	b = b.push_back(100);

	UT_VERIFY(b[0] == 98);
	UT_VERIFY(b[1] == 99);
	UT_VERIFY(b[2] == 100);
}

UNIT_TEST("CAPersistentVector", "pvector::push_back()", "Modify vector and keep original", "original vector is unchanged"){
	const pvector<int> a({ 4, 5, 6 });
	pvector<int> b = a.push_back(98);
	UT_VERIFY(a == pvector<int>({ 4, 5, 6 }));
}

template <typename T>
bool check_contents_using_array_subscription(const pvector<T>& v, std::vector<T> entries){
	if(v.size() != entries.size()){
		return false;
	}

	auto entriesIt = entries.begin();
	for(long i = 0 ; i < v.size() ; i++){
		T a = v[i];
		T b = *entriesIt;
		if(a != b){
			return false;
		}

		entriesIt++;
	}
	return true;
}


	std::vector<int> kTestNumbers = {
			500, 501, 502, 503, 504, 505, 506, 507, 508, 509,
			510, 511, 512, 513, 514, 515, 516, 517, 518, 519,
			520, 521, 522, 523, 524, 525, 526, 527, 528, 529,
			530, 531,

			601
		};

pvector<int> make_test1(){
	const leaf_node<int>* leaf = pool<int>::allocate_leaf_node(kTestNumbers.data(), 32);
	return pvector<int>(leaf, 32);
}

pvector<int> make_test2(){
	const leaf_node<int>* leaf0 = pool<int>::allocate_leaf_node(kTestNumbers.data(), 32);
	const leaf_node<int>* leaf1 = pool<int>::allocate_leaf_node(kTestNumbers.data() + 32, 1);

	std::vector<const node<int>* > subnodes = { leaf0, leaf1 };
	const inode<int>* inode = pool<int>::allocate_inode(&subnodes[0], 2);
	return pvector<int>(inode, 33);
}

UNIT_TEST("CAPersistentVector", "pvector::operator[]", "can read all items in level 0 vector", ""){
	pvector<int> a = make_test1();

	ASSERT(check_contents_using_array_subscription<int>(a, std::vector<int>(kTestNumbers.begin(), kTestNumbers.begin() + 32)));
}

UNIT_TEST("CAPersistentVector", "pvector::operator[]", "can 33 items in level 1 vector", ""){
	pvector<int> a = make_test2();

	ASSERT(check_contents_using_array_subscription<int>(a, std::vector<int>(kTestNumbers.begin(), kTestNumbers.begin() + 33)));
}


#if 1
UNIT_TEST("CAPersistentVector", "pvector::push_back()", "push_back() 33 items - inode needed", "proper read back"){
	pvector<int> a;
	for(int i = 0 ; i < 33 ; i++){
		a = a.push_back((i + 3) * 10);
	}

	ASSERT(check_contents_using_array_subscription(a,
		{
			30, 40, 50, 60, 70, 80, 90, 100, 110, 120,
			130, 140, 150, 160, 170, 180, 190, 200, 210, 220,
			230, 240, 250, 260, 270, 280, 290, 300, 310, 320,
			330, 340, 350
		}
	));
}
#endif


UNIT_TEST("", "", "", ""){
	exit(0);
}





//
//  ImmutableString.cpp
//  Permafrost
//
//  Created by Marcus Zetterquist on 2014-02-13.
//  Copyright (c) 2014 Marcus Zetterquist. All rights reserved.
//

#include "ImmutableString.h"

#include <utility>



bool IsValidUTF8(const std::string& /*iString*/){
	return true;
}


///////////////////////////		immutable_string::TContainer



immutable_string::TContainer::TContainer() :
	_size(0)
{
	ASSERT(check_invariant());
}

immutable_string::TContainer::TContainer(const std::string& iData) :
	_size(iData.size())
{
	if(_size > kInlineSize){
		_data._ptr = new std::uint8_t[_size];
		memcpy(_data._ptr, iData.c_str(), _size);
	}
	else{
		memcpy(_data._inline, iData.c_str(), _size);
	}

	ASSERT(check_invariant());
	ASSERT(iData == std::string(data(), data() + _size));
}

immutable_string::TContainer::TContainer(const TContainer& iOther) :
	_size(iOther._size)
{
	ASSERT(iOther.check_invariant());

	if(_size > kInlineSize){
		_data._ptr = new std::uint8_t[_size];
		memcpy(_data._ptr, iOther._data._ptr, _size);
	}
	else{
		memcpy(_data._inline, iOther._data._inline, _size);
	}

	ASSERT(check_invariant());
}

immutable_string::TContainer::~TContainer(){
	ASSERT(check_invariant());

	if(_size > kInlineSize){
		delete[] _data._ptr;
		_data._ptr = nullptr;
	}
	else{
	}
}

bool immutable_string::TContainer::check_invariant() const{
	ASSERT(this != nullptr);

	if(_size > kInlineSize){
		ASSERT(_data._ptr != nullptr);
	}
	else{
	}

	return true;
}

const std::uint8_t* immutable_string::TContainer::data() const{
	ASSERT(check_invariant());

	if(_size > kInlineSize){
		return _data._ptr;
	}
	else{
		return &_data._inline[0];
	}
}

std::size_t immutable_string::TContainer::size() const{
	ASSERT(check_invariant());

	return _size;
}

bool immutable_string::TContainer::operator==(const TContainer& iOther) const {
	ASSERT(check_invariant());
	ASSERT(iOther.check_invariant());

	if(_size == iOther._size){
		return memcmp(data(), iOther.data(), _size) == 0;
	}
	else{
		return false;
	}
}


immutable_string::TContainer& immutable_string::TContainer::operator=(const TContainer& iOther){
	immutable_string::TContainer temp = iOther;
	temp.swap(*this);
	return *this;
}

void immutable_string::TContainer::swap(TContainer& iOther) throw(){
	ASSERT(check_invariant());
	ASSERT(iOther.check_invariant());

	std::swap(_data, iOther._data);
	std::swap(_size, iOther._size);

	ASSERT(check_invariant());
	ASSERT(iOther.check_invariant());
}




///////////////////////////		immutable_string



//	Constructor #1
immutable_string::immutable_string(){
	ASSERT(check_invariant());
}

/*
//	Constructor #2
immutable_string(std::size_t count, const immutable_string& v){
	ASSERT(v.check_invariant());

	std::string temp;
	for(std::size_t i = 0 ; i < count ; i++){
		temp += v._utf8;
	}
	temp.swap(_utf8);
	immutable_string t(temp);

	ASSERT(check_invariant());
}
*/

//	Constructor #3
/**
	iUTF8 must be well-formed UTF8 Unicode using Normalization D. Else defect.
*/
immutable_string::immutable_string(const std::string& iUTF8){
	ASSERT(!iUTF8.empty() || IsValidUTF8(iUTF8));

	_container = TContainer(iUTF8);

	ASSERT(check_invariant());
}

//	Constructor #4
immutable_string::immutable_string(const immutable_string& iOther) :
	_container(iOther._container)
{
	ASSERT(check_invariant());
}
	
/*
//	Constructor #5
immutable_string(std::initializer_list<T> args) :
	_utf8(args)
{
	ASSERT(check_invariant());
}
*/

immutable_string::~immutable_string(){
	ASSERT(check_invariant());
}

bool immutable_string::check_invariant() const{
	ASSERT(this != nullptr);

	ASSERT(_container.check_invariant());

	return true;
}

std::size_t immutable_string::size() const {
	ASSERT(check_invariant());

	return _container.size();
}

bool immutable_string::empty() const {
	ASSERT(check_invariant());

	return _container.size() == 0;
}

bool immutable_string::operator==(const immutable_string& iOther) const {
	ASSERT(check_invariant());

	return _container == iOther._container;
}


std::string immutable_string::get_utf8() const {
	ASSERT(check_invariant());

	const char* p = reinterpret_cast<const char*>(_container.data());
	return std::string(p, p + _container.size());
}



///////////////////////////		UNIT TESTS




///////////////////////////		immutable_string()


UNIT_TEST("ImmutableString", "immutable_string()", "Basic construction", "empty() == true"){
	const immutable_string a;
	UT_VERIFY(a.empty());
}

UNIT_TEST("ImmutableString", "immutable_string()", "Inline string: abc", "get_utf8() == 'abcd'"){
	UT_VERIFY(immutable_string("abcd").get_utf8() == "abcd");
}


///////////////////////////		immutable_string(std::string)


UNIT_TEST("ImmutableString", "immutable_string(std::string)", "Out-of-line string: 123456789", "get_utf8() == '123456789'"){
	UT_VERIFY(immutable_string::TContainer::kInlineSize == 8);
	UT_VERIFY(immutable_string("123456789").get_utf8() == "123456789");
}

UNIT_TEST("ImmutableString", "immutable_string(std::string)", "String with embedded nulls", "get_utf8() == correct"){
	UT_VERIFY(immutable_string("12345\06789").get_utf8() == "12345\06789");
}


///////////////////////////		immutable_string(const immutable_string)


UNIT_TEST("ImmutableString", "immutable_string(const immutable_string)", "Inline string: abc", "get_utf8() == 'abcd'"){
	const immutable_string temp("abcd");
	const immutable_string copy(temp);
	UT_VERIFY(copy.get_utf8() == "abcd");
}

UNIT_TEST("ImmutableString", "immutable_string(const immutable_string)", "Out-of-line string: 123456789", "get_utf8() == '123456789'"){
	UT_VERIFY(immutable_string::TContainer::kInlineSize == 8);
	const immutable_string temp("123456789");
	const immutable_string copy(temp);
	UT_VERIFY(copy.get_utf8() == "123456789");
}


///////////////////////////		immutable_string::size()


UNIT_TEST("ImmutableString", "immutable_string::size()", "Inline string: abc", "size() == 4"){
	UT_VERIFY(immutable_string("abcd").size() == 4);
}

UNIT_TEST("ImmutableString", "immutable_string::size()", "Out-of-line string: 123456789", "size() == 9"){
	UT_VERIFY(immutable_string::TContainer::kInlineSize == 8);
	UT_VERIFY(immutable_string("123456789").size() == 9);
}


///////////////////////////		immutable_string::operator==()


UNIT_TEST("ImmutableString", "immutable_string::operator==()", "two empty strings", "equal"){
	UT_VERIFY(immutable_string("") == immutable_string(""));
}

UNIT_TEST("ImmutableString", "immutable_string::operator==()", "One empty strings", "not equal"){
	UT_VERIFY(!(immutable_string("a") == immutable_string("")));
}

UNIT_TEST("ImmutableString", "immutable_string::operator==()", "One empty strings 2", "not equal"){
	UT_VERIFY(!(immutable_string("") == immutable_string("a")));
}

UNIT_TEST("ImmutableString", "immutable_string::operator==()", "Two non-empty strings", "equal"){
	UT_VERIFY(immutable_string("xyz") == immutable_string("xyz"));
}

UNIT_TEST("ImmutableString", "immutable_string::operator==()", "Two non-empty strings", "not equal"){
	UT_VERIFY(!(immutable_string("abc") == immutable_string("xyz")));
}


///////////////////////////		immutable_string::get_utf8()


UNIT_TEST("ImmutableString", "immutable_string::get_utf8()", "empty", "empty"){
	UT_VERIFY(immutable_string("").get_utf8() == "");
}

UNIT_TEST("ImmutableString", "immutable_string::get_utf8()", "Inline string: abc", "abc"){
	UT_VERIFY(immutable_string("abc").get_utf8() == "abc");
}

UNIT_TEST("ImmutableString", "immutable_string::get_utf8()", "Out-of-line string: 123456789", "123456789"){
	UT_VERIFY(immutable_string("123456789").get_utf8() == "123456789");
}


///////////////////////////		immutable_string - scenarios


UNIT_TEST("ImmutableString", "", "", ""){
//	immutable_string b = "abc";

	immutable_string a = immutable_string("abc");
}





//
//  ImmutableString.h
//  Permafrost
//
//  Created by Marcus Zetterquist on 2014-02-13.
//  Copyright (c) 2014 Marcus Zetterquist. All rights reserved.
//

#ifndef __Permafrost__ImmutableString__
#define __Permafrost__ImmutableString__

#include <string>
#include <initializer_list>
#include "cpp_extension.h"


bool IsValidUTF8(const std::string& iString);



///////////////////////////		immutable_string


/**
	Cannot be modified.
	Guaranteed to support any valid Unicode.
	Clients cannot access individual Unicode-parts, like code-units or code-points - only valid texts.

	All comparisons are done using Unicode Normalization D.
	Pure - can contain binary-zeros.
*/

class immutable_string {
	//	Constructor #1
	public: immutable_string();

	//	Constructor #3
	/**
		iUTF8 must be well-formed UTF8 Unicode using Normalization D. Else defect.
	*/
	public: explicit immutable_string(const std::string& iUTF8);

	//	Constructor #4
	public: immutable_string(const immutable_string& iOther);

	public: ~immutable_string();

	public: bool check_invariant() const;

	public: std::size_t size() const;

	public: bool empty() const;

	public: bool operator==(const immutable_string& iOther) const;

	public: bool operator!=(const immutable_string& iOther) const {
		return !(*this == iOther);
	}

	public: std::string get_utf8() const;


	//////////////////		State
		public: class TContainer {
			public: static const std::size_t kInlineSize = 8;

			public: TContainer();
			public: TContainer(const std::string& iData);
			public: TContainer(const TContainer& iOther);
			public: ~TContainer();
			public: bool check_invariant() const;
			public: const std::uint8_t* data() const;
			public: std::size_t size() const;
			public: bool operator==(const TContainer& iOther) const;

			public: TContainer& operator=(const TContainer& iOther);
			public: void swap(TContainer& iOther) throw();

			private: union UData {
				UData() : _ptr(nullptr){};

				std::uint8_t _inline[kInlineSize];
				std::uint8_t* _ptr;
			};

			private: UData _data;
			private: std::size_t _size;
		};

		private: TContainer _container;
};





/**
	Use to mark-up strings that are swapped at runtime.
*/
#define LOCALIZABLE(x);


int Sort(const immutable_string& iA, const immutable_string& iB);

immutable_string ExpandUsingTemplate(const immutable_string& iTemplate, const immutable_string& i1);
immutable_string ExpandUsingTemplate(const immutable_string& iTemplate, const immutable_string& i1, const immutable_string& i2);
immutable_string ExpandUsingTemplate(const immutable_string& iTemplate, const immutable_string& i1, const immutable_string& i2, const immutable_string& i3);



#endif /* defined(__Permafrost__ImmutableString__) */

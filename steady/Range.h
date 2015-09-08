//
//  Range.h
//  Permafrost
//
//  Created by Marcus Zetterquist on 2014-02-02.
//  Copyright (c) 2014 Marcus Zetterquist. All rights reserved.
//

#ifndef __Permafrost__Range__
#define __Permafrost__Range__


#include <vector>
#include <initializer_list>
#include "cpp_extension.h"

///////////////////////////		immutable_vector


/**
*/
template <typename T>
struct irange_support {
	typedef std::size_t size_type;
	public: virtual ~irange_support(){};
	public: virtual const T& irange_support_get_at_pos(size_type iIndex) const = 0;
};


#endif /* defined(__Permafrost__Range__) */

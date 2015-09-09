//
//  main.cpp
//  steady
//
//  Created by Marcus Zetterquist on 2013-11-13.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//


#include "cpp_extension.h"
#include "steady_vector.h"

#include <iostream>

/*
int main(int argc, const char * argv[]){
	// insert code here...
	std::cout << "Hello, World!\n";

	run_tests();

    return 0;
}
*/




int main(int argc, const char * argv[])
{
	TDefaultRuntime runtime("");
	SetRuntime(&runtime);
	run_tests();

	try {
		TRACE("Hello, World 3!");
	}
	catch(...){
		TRACE("Error");
		return -1;
	}

    return 0;
}

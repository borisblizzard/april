/// @file
/// @version 4.3
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Defines main for WinUWP.

#if defined(_WIN32) && defined(_WINUWP) && !defined(_OPENKODE)
#ifndef APRIL_WINUWP_MAIN_H
#define APRIL_WINUWP_MAIN_H

#include "aprilExport.h"

#include <hltypes/harray.h>
#include <hltypes/hltypesUtil.h>
#include <hltypes/hstring.h>

#include "main_base.h"
#include "aprilExport.h"

/// @brief Defines the main function in WinUWP.
/// @param[in] args Arguments.
/// @return Application result code.
/// @note This is used internally only.
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^ args)
{
	// extract arguments
	int argc = 0;
	char** argv = new char*[argc];
	hstr arg;
	for_iter (i, 0, argc)
	{
		arg = hstr::fromUnicode(args[i]->Data());
		argv[i] = new char[arg.size() + 1];
		memcpy(argv[i], arg.cStr(), sizeof(char) * (arg.size() + 1));
	}
	// call the user specified main function
	int result = __april_main(april_init, april_destroy, argc, argv);
	// free allocated memory for arguments
	for_iter (i, 0, argc)
	{
		delete [] argv[i];
	}
	delete [] argv;
	return result;
}
#endif
#endif
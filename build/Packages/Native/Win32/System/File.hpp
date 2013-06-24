// -----------------------------------------------------------------------------
// 	file.hpp
// 	Copyright (C) 2012-2013 TwinDrills, All Rights Reserved
// -----------------------------------------------------------------------------
//	This package contains the declarations of the class used to perform
//	common file operations.
// -----------------------------------------------------------------------------

#ifndef __LS_PACKAGES_NATIVE_WIN32_SYSTEM_FILE__
#define __LS_PACKAGES_NATIVE_WIN32_SYSTEM_FILE__

#include "Packages/Native/Win32/Compiler/Support/Types.hpp"

// -----------------------------------------------------------------------------
//	This class is used to perform several common file operations.
// -----------------------------------------------------------------------------
class lsFile
{
public:
	static bool Create	(lsString path, bool recursive);
	static bool Delete	(lsString path);
	static bool Copy	(lsString path, lsString from, bool overwrite);
	static bool Rename	(lsString from, lsString to);
	static bool Exists	(lsString path);

};

#endif // __LS_PACKAGES_NATIVE_WIN32_SYSTEM_FILE__


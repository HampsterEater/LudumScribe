// -----------------------------------------------------------------------------
// 	gc.ls
// 	Copyright (C) 2012-2013 TwinDrills, All Rights Reserved
// -----------------------------------------------------------------------------
//	This package contains the declarations of the class used to interact with
//	the target languages garbage collector.
// -----------------------------------------------------------------------------
using native {NATIVE_PACKAGE_NAMESPACE}.System.GC;

// Include the Boehm Garbage Collector folder/library
// if our target language is C++.
#if TRANSLATOR_SHORT_NAME=="C++"
	using copy    {NATIVE_PACKAGE_NAMESPACE}.System.GC.include.*;
	using copy    {NATIVE_PACKAGE_NAMESPACE}.System.GC.include.extra.*;
	using copy    {NATIVE_PACKAGE_NAMESPACE}.System.GC.include.private.*;	
	#if CONFIG=="Debug"
		using library {NATIVE_PACKAGE_NAMESPACE}.System.GC.lib.{PLATFORM}.libgcd;
	#else
		using library {NATIVE_PACKAGE_NAMESPACE}.System.GC.lib.{PLATFORM}.libgc;	
	#endif
#endif

// -----------------------------------------------------------------------------
//	This class is used to interact with the target languages garbage collector.
//	Some target languages will not allow interaction with the garbage collector,
//	if this is the case then the method of this class will act as if they
//	are empty.
// -----------------------------------------------------------------------------
public static native("lsGC") class GC
{
	public static native("Collect") 		  void Collect				(bool full = false);
	public static native("GetBytesAllocated") int  GetBytesAllocated	();
}

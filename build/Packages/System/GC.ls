// -----------------------------------------------------------------------------
// 	gc.ls
// 	Copyright (C) 2012-2013 TwinDrills, All Rights Reserved
// -----------------------------------------------------------------------------
//	This package contains the declarations of the class used to interact with
//	the target languages garbage collector.
// -----------------------------------------------------------------------------
using native Native.{PLATFORM}.System.GC;

// -----------------------------------------------------------------------------
//	This class is used to interact with the target languages garbage collector.
//	Some target languages will not allow interaction with the garbage collector,
//	if this is the case then the method of this class will act as if they
//	are empty.
// -----------------------------------------------------------------------------
public static native("lsGC") class GC
{
	public static native("Collect") void Collect(bool full = false);
}

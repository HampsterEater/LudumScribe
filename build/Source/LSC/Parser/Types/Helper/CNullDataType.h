/* *****************************************************************

		CNullDataType.h

		Copyright (C) 2012 Tim Leonard - All Rights Reserved

   ***************************************************************** */
#pragma once
#ifndef _CNULLDATATYPE_H_
#define _CNULLDATATYPE_H_

#include <string>
#include <vector>

#include "CToken.h"
#include "CDataType.h"

class CSemanter;
class CTranslationUnit;
class CArrayDataType;
class CDataType;

// =================================================================
//	Base class for all data types.
// =================================================================
class CNullDataType : public CDataType
{
protected:

public:
	CNullDataType(CToken& token);
	
	virtual bool			IsEqualTo	(CSemanter* semanter, CDataType* type);
	virtual bool			CanCastTo	(CSemanter* semanter, CDataType* type);
	virtual std::string		ToString	();

};

#endif
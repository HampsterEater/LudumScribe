/* *****************************************************************

		CFloatDataType.h

		Copyright (C) 2012 Tim Leonard - All Rights Reserved

   ***************************************************************** */
#pragma once
#ifndef _CFLOATDATATYPE_H_
#define _CFLOATDATATYPE_H_

#include <string>
#include <vector>

#include "CToken.h"
#include "CDataType.h"
#include "CNumericDataType.h"

class CSemanter;
class CTranslationUnit;
class CArrayDataType;
class CDataType;

// =================================================================
//	Base class for all data types.
// =================================================================
class CFloatDataType : public CNumericDataType
{
protected:

public:
	CFloatDataType(CToken& token);
	
	virtual CClassASTNode*	GetClass	(CSemanter* semanter);
	virtual bool			IsEqualTo	(CSemanter* semanter, CDataType* type);
	virtual bool			CanCastTo	(CSemanter* semanter, CDataType* type);
	virtual std::string		ToString	();

};

#endif
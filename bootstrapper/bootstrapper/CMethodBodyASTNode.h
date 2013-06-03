/* *****************************************************************

		CMethodBodyASTNode.h

		Copyright (C) 2012 Tim Leonard - All Rights Reserved

   ***************************************************************** */
#pragma once
#ifndef _CMETHODBODYASTNODE_H_
#define _CMETHODBODYASTNODE_H_

#include <string>
#include <vector>

#include "CToken.h"
#include "CASTNode.h"

// =================================================================
//	Stores information on a method body.
// =================================================================
class CMethodBodyASTNode : public CASTNode
{
protected:	

public:
	CMethodBodyASTNode(CASTNode* parent, CToken token);
		
	virtual CASTNode* Clone	(CSemanter* semanter);
	virtual CASTNode* Semant(CSemanter* semanter);
};

#endif
/* *****************************************************************

		CPackageASTNode.h

		Copyright (C) 2012 Tim Leonard - All Rights Reserved

   ***************************************************************** */
#pragma once
#ifndef _CPACKAGEASTNODE_H_
#define _CPACKAGEASTNODE_H_

#include <string>
#include <vector>

#include "CToken.h"
#include "CASTNode.h"

// =================================================================
//	Stores information on a package declaration.
// =================================================================
class CPackageASTNode : public CASTNode
{
protected:	

public:
	CPackageASTNode(CASTNode* parent, CToken token);
	
	// Semantic analysis.
	virtual CASTNode* Semant(CSemanter* semanter);
	virtual CASTNode* Clone	(CSemanter* semanter);

};

#endif
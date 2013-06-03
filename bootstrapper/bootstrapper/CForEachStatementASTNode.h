/* *****************************************************************

		CForEachStatementASTNode.h

		Copyright (C) 2012 Tim Leonard - All Rights Reserved

   ***************************************************************** */
#pragma once
#ifndef _CFOREACHSTATEMENTASTNODE_H_
#define _CFOREACHSTATEMENTASTNODE_H_

#include <string>
#include <vector>

#include "CToken.h"
#include "CASTNode.h"

class CExpressionASTNode;
class CExpressionBaseASTNode;

// =================================================================
//	Stores information on an block statement.
// =================================================================
class CForEachStatementASTNode : public CASTNode
{
protected:	

public:
	CASTNode* VariableStatement;
	CExpressionBaseASTNode*	ExpressionStatement;
	CASTNode* BodyStatement;

	CForEachStatementASTNode(CASTNode* parent, CToken token);
	
	virtual CASTNode* Clone(CSemanter* semanter);
	virtual CASTNode* Semant(CSemanter* semanter);

	virtual CASTNode* FindLoopScope(CSemanter* semanter);
	
	virtual bool AcceptBreakStatement();
	virtual bool AcceptContinueStatement();

	virtual void Translate(CTranslator* translator);
};

#endif
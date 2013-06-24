/* *****************************************************************

		CWhileStatementASTNode.h

		Copyright (C) 2012 Tim Leonard - All Rights Reserved

   ***************************************************************** */
#pragma once
#ifndef _CWHILESTATEMENTASTNODE_H_
#define _CWHILESTATEMENTASTNODE_H_

#include <string>
#include <vector>

#include "CToken.h"
#include "CASTNode.h"

class CExpressionBaseASTNode;
class CTranslator;

// =================================================================
//	Stores information on an block statement.
// =================================================================
class CWhileStatementASTNode : public CASTNode
{
protected:	

public:
	CExpressionBaseASTNode* ExpressionStatement;
	CASTNode* BodyStatement;

	CWhileStatementASTNode(CASTNode* parent, CToken token);	

	virtual CASTNode* Clone	(CSemanter* semanter);
	virtual CASTNode* Semant(CSemanter* semanter);

	virtual CASTNode* FindLoopScope(CSemanter* semanter);
	
	virtual bool AcceptBreakStatement();
	virtual bool AcceptContinueStatement();

	virtual void Translate(CTranslator* translator);

};

#endif
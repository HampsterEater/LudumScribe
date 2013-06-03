/* *****************************************************************

		CSwitchStatementASTNode.cpp

		Copyright (C) 2012 Tim Leonard - All Rights Reserved

   ***************************************************************** */

#include "CSwitchStatementASTNode.h"
#include "CExpressionASTNode.h"
#include "CExpressionBaseASTNode.h"

#include "CTranslator.h"

// =================================================================
//	Constructs a new instance of this class.
// =================================================================
CSwitchStatementASTNode::CSwitchStatementASTNode(CASTNode* parent, CToken token) :
	CASTNode(parent, token),
	ExpressionStatement(NULL)
{
}

// =================================================================
// Performs semantic analysis of this node.
// =================================================================
CASTNode* CSwitchStatementASTNode::Semant(CSemanter* semanter)
{	
	// Only semant once.
	if (Semanted == true)
	{
		return this;
	}
	Semanted = true;

	// Semant the expression.
	ExpressionStatement = dynamic_cast<CExpressionBaseASTNode*>(ReplaceChild(ExpressionStatement, ExpressionStatement->Semant(semanter)));

	// Semant all case/default statements.
	SemantChildren(semanter);

	return this;
}

// =================================================================
//	Creates a clone of this node.
// =================================================================
CASTNode* CSwitchStatementASTNode::Clone(CSemanter* semanter)
{
	CSwitchStatementASTNode* clone = new CSwitchStatementASTNode(NULL, Token);

	if (ExpressionStatement != NULL)
	{
		clone->ExpressionStatement = dynamic_cast<CExpressionASTNode*>(ExpressionStatement->Clone(semanter));
		clone->AddChild(clone->ExpressionStatement);
	}

	return clone;
}

// =================================================================
//	Causes this node to be translated.
// =================================================================
void CSwitchStatementASTNode::Translate(CTranslator* translator)
{
	translator->TranslateSwitchStatement(this);
}
/* *****************************************************************

		CDoStatementASTNode.cpp

		Copyright (C) 2012 Tim Leonard - All Rights Reserved

   ***************************************************************** */

#include "CDoStatementASTNode.h"

#include "CExpressionASTNode.h"
#include "CExpressionBaseASTNode.h"

#include "CBoolDataType.h"

#include "CTranslator.h"

#include "CSemanter.h"

// =================================================================
//	Constructs a new instance of this class.
// =================================================================
CDoStatementASTNode::CDoStatementASTNode(CASTNode* parent, CToken token) :
	CASTNode(parent, token),
	BodyStatement(NULL),
	ExpressionStatement(NULL)
{
}

// =================================================================
//	Returns true if this node can accept break statements inside
//	of it.
// =================================================================
bool CDoStatementASTNode::AcceptBreakStatement()
{
	return true;
}

// =================================================================
//	Returns true if this node can accept continue statements inside
//	of it.
// =================================================================
bool CDoStatementASTNode::AcceptContinueStatement()
{
	return true;
}

// =================================================================
// Performs semantic analysis of this node.
// =================================================================
CASTNode* CDoStatementASTNode::Semant(CSemanter* semanter)
{	
	SEMANT_TRACE("CDoStatementASTNode");

	// Only semant once.
	if (Semanted == true)
	{
		return this;
	}
	Semanted = true;

	// Semant the expression.
	ExpressionStatement = dynamic_cast<CExpressionBaseASTNode*>(ReplaceChild(ExpressionStatement, ExpressionStatement->Semant(semanter)));
	ExpressionStatement = dynamic_cast<CExpressionBaseASTNode*>(ReplaceChild(ExpressionStatement, ExpressionStatement->CastTo(semanter, new CBoolDataType(Token), Token)));

	// Semant Body statement.
	if (BodyStatement != NULL)
	{
		BodyStatement = ReplaceChild(BodyStatement, BodyStatement->Semant(semanter));
	}

	return this;
}

// =================================================================
//	Creates a clone of this node.
// =================================================================
CASTNode* CDoStatementASTNode::Clone(CSemanter* semanter)
{
	CDoStatementASTNode* clone = new CDoStatementASTNode(NULL, Token);
	
	if (ExpressionStatement != NULL)
	{
		clone->ExpressionStatement = dynamic_cast<CExpressionASTNode*>(ExpressionStatement->Clone(semanter));
		clone->AddChild(clone->ExpressionStatement);
	}
	if (BodyStatement != NULL)
	{
		clone->BodyStatement = dynamic_cast<CASTNode*>(BodyStatement->Clone(semanter));
		clone->AddChild(clone->BodyStatement);
	}

	return clone;
}

// =================================================================
//	Finds the scope the looping statement this node is contained by.
// =================================================================
CASTNode* CDoStatementASTNode::FindLoopScope(CSemanter* semanter)
{
	return this;
}

// =================================================================
//	Causes this node to be translated.
// =================================================================
void CDoStatementASTNode::Translate(CTranslator* translator)
{
	translator->TranslateDoStatement(this);
}
/* *****************************************************************

		CExpressionASTNode.cpp

		Copyright (C) 2012 Tim Leonard - All Rights Reserved

   ***************************************************************** */

#include "CExpressionASTNode.h"

#include "CSemanter.h"
#include "CTranslationUnit.h"

#include "CTranslator.h"

// =================================================================
//	Constructs a new instance of this class.
// =================================================================
CExpressionASTNode::CExpressionASTNode(CASTNode* parent, CToken token) :
	CExpressionBaseASTNode(parent, token),
	LeftValue(NULL),
	IsConstant(false)
{
}

// =================================================================
//	Creates a clone of this node.
// =================================================================
CASTNode* CExpressionASTNode::Clone(CSemanter* semanter)
{
	CExpressionASTNode* clone = new CExpressionASTNode(NULL, Token);

	if (LeftValue != NULL)
	{
		clone->LeftValue = dynamic_cast<CASTNode*>(LeftValue->Clone(semanter));
		clone->AddChild(clone->LeftValue);
	}

	return clone;
}

// =================================================================
//	Performs semantic analysis on this node.
// =================================================================
CASTNode* CExpressionASTNode::Semant(CSemanter* semanter)
{ 
	SEMANT_TRACE("CExpressionASTNode");

	// Only semant once.
	if (Semanted == true)
	{
		return this;
	}
	Semanted = true;

	// Semant the expression.
	LeftValue = ReplaceChild(LeftValue, LeftValue->Semant(semanter));
	ExpressionResultType = dynamic_cast<CExpressionBaseASTNode*>(LeftValue)->ExpressionResultType;

	// Check it evaluates correctly.
	if (IsConstant == true)
	{
	//	try
	//	{
			EvaluationResult result = Evaluate(semanter->GetContext());
	//	}
	//	catch (std::runtime_error ex)
	//	{
	//		semanter->GetContext()->FatalError("Illegal constant expression.", Token);
	//	}
	}

	return this;
}

// =================================================================
//	Evalulates the constant value of this node.
// =================================================================
EvaluationResult CExpressionASTNode::Evaluate(CTranslationUnit* unit)
{
	EvaluationResult leftResult = LeftValue->Evaluate(unit);
	return leftResult;
}

// =================================================================
//	Causes this node to be translated.
// =================================================================
void CExpressionASTNode::Translate(CTranslator* translator)
{
	translator->TranslateExpressionStatement(this);
}

// =================================================================
//	Causes this node to be translated.
// =================================================================
std::string CExpressionASTNode::TranslateExpr(CTranslator* translator)
{
	return translator->TranslateExpression(this);
}
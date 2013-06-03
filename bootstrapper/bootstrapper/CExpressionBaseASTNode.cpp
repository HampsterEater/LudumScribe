/* *****************************************************************

		CExpressionBaseASTNode.cpp

		Copyright (C) 2012 Tim Leonard - All Rights Reserved

   ***************************************************************** */

#include "CExpressionBaseASTNode.h"
#include "CCastExpressionASTNode.h"

#include "CToken.h"
#include "CDataType.h"

#include "CTranslator.h"

#include "CTranslationUnit.h"

// =================================================================
//	Constructs a new instance of this class.
// =================================================================
CExpressionBaseASTNode::CExpressionBaseASTNode(CASTNode* parent, CToken token) :
	CASTNode(parent, token),
	ExpressionResultType(NULL)
{
}

// =================================================================
//	Returns a node that casts this expression's result to the correct
//  data type. If already of the correct type this node is returned.
// =================================================================
CASTNode* CExpressionBaseASTNode::CastTo(CSemanter* semanter, CDataType* type, CToken& castToken, bool explicit_cast, bool exception_on_fail)
{
	// If we are already of this type, just return this.
	if (ExpressionResultType->IsEqualTo(semanter, type))
	{
		return this;
	}

	// Create a cast.
	CCastExpressionASTNode* node = new CCastExpressionASTNode(NULL, castToken, false);
	node->Parent = Parent;
	node->Type = type;
	node->RightValue = this;
	node->AddChild(this);
	node->Explicit = explicit_cast;
	node->ExceptionOnFail = exception_on_fail;

	CASTNode* ret = node->Semant(semanter);
	node->Parent = NULL;

	return ret;
}

// =================================================================
//	Translates this expression.
// =================================================================
void CExpressionBaseASTNode::Translate(CTranslator* translator)
{
	translator->GetContext()->FatalError("Internal error. Attempt to directly translate expression base node.", Token);
}
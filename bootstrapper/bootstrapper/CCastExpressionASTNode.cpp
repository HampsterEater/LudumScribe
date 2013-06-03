/* *****************************************************************

		CCastExpressionASTNode.cpp

		Copyright (C) 2012 Tim Leonard - All Rights Reserved

   ***************************************************************** */

#include "CCastExpressionASTNode.h"
#include "CDataType.h"
#include "CBoolDataType.h"
#include "CObjectDataType.h"
#include "CVoidDataType.h"
#include "CStringDataType.h"
#include "CIntDataType.h"
#include "CFloatDataType.h"
#include "CArrayDataType.h"

#include "CClassASTNode.h"

#include "CStringHelper.h"

#include "CSemanter.h"
#include "CTranslationUnit.h"

#include "CNewExpressionASTNode.h"

#include "CMethodCallExpressionASTNode.h"
#include "CFieldAccessExpressionASTNode.h"
#include "CIdentifierExpressionASTNode.h"

#include "CTranslator.h"

// =================================================================
//	Constructs a new instance of this class.
// =================================================================
CCastExpressionASTNode::CCastExpressionASTNode(CASTNode* parent, CToken token, bool explicitCast) :
	CExpressionBaseASTNode(parent, token), 
	Explicit(explicitCast),
	ExceptionOnFail(true),
	Type(NULL),
	RightValue(NULL)
{
}

// =================================================================
//	Creates a clone of this node.
// =================================================================
CASTNode* CCastExpressionASTNode::Clone(CSemanter* semanter)
{
	CCastExpressionASTNode* clone = new CCastExpressionASTNode(NULL, Token, Explicit);
	clone->Type = Type;

	if (RightValue != NULL)
	{
		clone->RightValue = dynamic_cast<CASTNode*>(RightValue->Clone(semanter));
		clone->AddChild(clone->RightValue);
	}

	return clone;
}

// =================================================================
//	Performs semantic analysis on this node.
// =================================================================
CASTNode* CCastExpressionASTNode::Semant(CSemanter* semanter)
{ 
	// Only semant once.
	if (Semanted == true)
	{
		return this;
	}
	Semanted = true;
	
	// Semant everything!
	Type		= Type->Semant(semanter, this);
	RightValue  = ReplaceChild(RightValue, RightValue->Semant(semanter));
	
	// Get expression references.
	CExpressionBaseASTNode* rightValueBase = dynamic_cast<CExpressionBaseASTNode*>(RightValue);
	CDataType* rightValueDataType = rightValueBase->ExpressionResultType;

	ExpressionResultType = NULL;

	// Already correct type?
	if (rightValueDataType->IsEqualTo(semanter, Type))
	{
		return RightValue;
	}

	// Can cast?
	else if (rightValueDataType->CanCastTo(semanter, Type))
	{
		// Box
		if (dynamic_cast<CObjectDataType*>(Type) != NULL &&
			dynamic_cast<CObjectDataType*>(rightValueDataType) == NULL)
		{
			// Create new boxed object.
			CNewExpressionASTNode* astNode	= new CNewExpressionASTNode(NULL, Token);
			astNode->DataType				= rightValueDataType->GetBoxClass(semanter)->ObjectDataType;
			astNode->ArgumentExpressions.push_back(RightValue);
			astNode->AddChild(RightValue);
			astNode->Semant(semanter);

			ExpressionResultType = Type;

			return astNode; // TODO: Memory leak.
		}

		// Unbox
		else if (dynamic_cast<CObjectDataType*>(rightValueDataType) != NULL &&
				 dynamic_cast<CObjectDataType*>(Type) == NULL &&
				 dynamic_cast<CStringDataType*>(Type) == NULL) // String is an exception as we use ToString for that
		{
			if (Explicit == true)
			{
				// Get the boxed value and return it.
				CMethodCallExpressionASTNode* astNode = new CMethodCallExpressionASTNode(Parent, Token);
				astNode->LeftValue					   = this;
				astNode->RightValue					   = new CIdentifierExpressionASTNode(NULL, CToken());
				astNode->RightValue->Token.Literal	   = "GetValue";
				astNode->AddChild(astNode->LeftValue);
				astNode->AddChild(astNode->RightValue);

				//ExceptionOnFail = true;
				Type = Type->GetBoxClass(semanter)->ObjectDataType;
				ExpressionResultType = Type;

				astNode->Semant(semanter);
				
				return astNode; // TODO: Memory leak.

				// Unbox the object.
			//	CNewExpressionASTNode* astNode = new CNewExpressionASTNode(NULL, Token);
			//	astNode->DataType = Type;		
			//	astNode->ArgumentExpressions.push_back(RightValue);
			//	astNode->AddChild(RightValue);
			//	RightValue = ReplaceChild(RightValue, astNode);
			//	astNode->Semant(semanter);
			}
		}

		// Normal cast?
		else
		{
			ExpressionResultType = Type;
		}		
	}
	
	if (ExpressionResultType == NULL)
	{

		// Implicitly cast to bool.
		if (dynamic_cast<CBoolDataType*>(Type) != NULL)
		{
			if (dynamic_cast<CVoidDataType*>(Type) == NULL)// &&
//				Explicit == true)
			{
				ExpressionResultType = Type;
			}
		}

		// Implicitly cast to string.
		else if (dynamic_cast<CStringDataType*>(Type) != NULL)
		{
			if (dynamic_cast<CNumericDataType*>(rightValueDataType) != NULL || Explicit == true)
			{
				ExpressionResultType = Type;
			}
		}
	
		// Explicitly cast objects.
		else if (Type->CanCastTo(semanter, rightValueDataType))
		{
			if (Explicit == true)
			{
				if (dynamic_cast<CObjectDataType*>(Type) != NULL &&
					dynamic_cast<CObjectDataType*>(rightValueDataType) != NULL)
				{
					ExpressionResultType = Type;
				}
			}
		}

	}

	// Invalid cast :S
	if (ExpressionResultType == NULL)
	{
		if (Explicit == false)
		{
			semanter->GetContext()->FatalError(CStringHelper::FormatString("Cannot implicitly cast value from '%s' to '%s'.", rightValueDataType->ToString().c_str(), Type->ToString().c_str()), Token);
		}
		else
		{
			semanter->GetContext()->FatalError(CStringHelper::FormatString("Cannot cast value from '%s' to '%s'.", rightValueDataType->ToString().c_str(), Type->ToString().c_str()), Token);
		}
	}

	return this;
}

// =================================================================
//	Returns true if the conversion between two data types is a valid
//  cast.
// =================================================================
bool CCastExpressionASTNode::IsValidCast(CSemanter* semanter, CDataType* from, CDataType* to, bool explicit_cast)
{
	// Already correct type?
	if (from->IsEqualTo(semanter, to))
	{
		return true;
	}

	// Can cast?
	else if (from->CanCastTo(semanter, to))
	{
		// Box
		if (dynamic_cast<CObjectDataType*>(to) != NULL &&
			dynamic_cast<CObjectDataType*>(from) == NULL)
		{
			return true;
		}

		// Unbox
		else if (dynamic_cast<CObjectDataType*>(from) != NULL &&
				 dynamic_cast<CObjectDataType*>(to) == NULL)
		{
			if (explicit_cast == true)
			{
				return true;
			}
		}

		// Normal cast?
		else
		{
			return true;
		}
	}
	
	// Implicitly cast to bool.
	if (dynamic_cast<CBoolDataType*>(to) != NULL)
	{
		if (dynamic_cast<CVoidDataType*>(to) == NULL)
		{			
			return true;
		}
	}

	// Implicitly cast to string.
	else if (dynamic_cast<CStringDataType*>(to) != NULL)
	{
		if (dynamic_cast<CNumericDataType*>(from) != NULL || explicit_cast == true)
		{
			return true;
		}
	}
	
	// Explicitly cast objects.
	else if (to->CanCastTo(semanter, from))
	{
		if (explicit_cast == true)
		{
			if (dynamic_cast<CObjectDataType*>(to) != NULL &&
				dynamic_cast<CObjectDataType*>(from) != NULL)
			{
				return true;
			}
		}
	}

	return false;
}

// =================================================================
//	Evalulates the constant value of this node.
// =================================================================
EvaluationResult CCastExpressionASTNode::Evaluate(CTranslationUnit* unit)
{
	EvaluationResult leftResult  = RightValue->Evaluate(unit);

	if (dynamic_cast<CBoolDataType*>(ExpressionResultType) != NULL)
	{
		leftResult.SetType(EvaluationResult::BOOL);
	}
	else if (dynamic_cast<CIntDataType*>(ExpressionResultType) != NULL)
	{
		leftResult.SetType(EvaluationResult::INT);
	}
	else if (dynamic_cast<CFloatDataType*>(ExpressionResultType) != NULL)
	{
		leftResult.SetType(EvaluationResult::FLOAT);
	}
	else if (dynamic_cast<CStringDataType*>(ExpressionResultType) != NULL)
	{
		leftResult.SetType(EvaluationResult::STRING);
	}

	return leftResult;
}

// =================================================================
//	Causes this node to be translated.
// =================================================================
std::string CCastExpressionASTNode::TranslateExpr(CTranslator* translator)
{
	return translator->TranslateCastExpression(this);
}
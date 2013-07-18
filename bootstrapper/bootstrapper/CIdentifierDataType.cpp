/* *****************************************************************

		CIdentifierDataType.cpp

		Copyright (C) 2012 Tim Leonard - All Rights Reserved

   ***************************************************************** */

#include "CIdentifierDataType.h"
#include "CArrayDataType.h"
#include "CObjectDataType.h"
#include "CTranslationUnit.h"

#include "CClassASTNode.h"
#include "CClassBodyASTNode.h"

#include "CASTNode.h"

#include "CStringHelper.h"

// =================================================================
//	Constructs a new instance of this class.
// =================================================================
CIdentifierDataType::CIdentifierDataType(CToken& token, std::string identifier, std::vector<CDataType*> genericTypes) :
	CDataType(token)
{
	Identifier = identifier;
	GenericTypes = genericTypes;
	m_do_not_semant_dt = false;
}

// =================================================================
//	Checks if this data type is equal to another.
// =================================================================
bool CIdentifierDataType::IsEqualTo(CSemanter* semanter, CDataType* type)
{
	semanter->GetContext()->FatalError("CIdentifierDataType::IsEqualTo invoked! This should have already been resolved!", Token);
	return false;
}

// =================================================================
//	Checks if this data type extends another.
// =================================================================
bool CIdentifierDataType::CanCastTo(CSemanter* semanter, CDataType* type)
{
	return IsEqualTo(semanter, type);
}

// =================================================================
//	Converts data type to string.
// =================================================================
std::string	CIdentifierDataType::ToString()
{
	if (GenericTypes.size() > 0)
	{
		std::string args = "";
		for (auto iter = GenericTypes.begin(); iter != GenericTypes.end(); iter++)
		{
			if (args != "")
			{
				args += ",";
			}
			args += (*iter)->ToString();
		}
		return Identifier + "<" + args + ">";
	}
	else
	{
		return Identifier;
	}
}

// =================================================================
//	Gets the class this data type is based on.
// =================================================================
CClassASTNode* CIdentifierDataType::GetClass(CSemanter* semanter)
{
	return NULL;
}

// =================================================================
//	Semants this data type and returns its output type.
// =================================================================
CDataType* CIdentifierDataType::Semant(CSemanter* semanter, CASTNode* node)
{
	SEMANT_TRACE("CIdentifierDataType");

	std::vector<CDataType*> generic_arguments;

	for (auto iter = GenericTypes.begin(); iter != GenericTypes.end(); iter++)
	{
		generic_arguments.push_back((*iter)->Semant(semanter, node));
	}

	CDataType* type = node->FindDataType(semanter, Identifier, generic_arguments, false, m_do_not_semant_dt);
	if (type == NULL)
	{
		semanter->GetContext()->FatalError(CStringHelper::FormatString("Unknown data type '%s'.", ToString().c_str()), Token);
	}

	return type->Semant(semanter, node);//type;
}

// =================================================================
//	Semants this data type and returns a class reference.
// =================================================================
CClassASTNode* CIdentifierDataType::SemantAsClass(CSemanter* semanter, CASTNode* node, bool do_not_semant)
{
	m_do_not_semant_dt = do_not_semant;
	CObjectDataType* type = dynamic_cast<CObjectDataType*>(Semant(semanter, node));
	m_do_not_semant_dt = false;

	if (type != NULL)
	{
		return type->GetClass(semanter);
	}
	else
	{
		semanter->GetContext()->FatalError("Identifier does not reference a class or interface.", Token);
	}
	return NULL;
}
/* *****************************************************************

		CClassReferenceDataType.cpp

		Copyright (C) 2012 Tim Leonard - All Rights Reserved

   ***************************************************************** */

#include "CClassReferenceDataType.h"
#include "CArrayDataType.h"

#include "CObjectDataType.h"
#include "CClassMemberASTNode.h"

#include "CStringHelper.h"

#include "CClassASTNode.h"

// =================================================================
//	Constructs a new instance of this class.
// =================================================================
CClassReferenceDataType::CClassReferenceDataType(CToken& token, CClassASTNode* node) :
	CDataType(token),
	m_class(node)
{
}

// =================================================================
//	Checks if this data type is equal to another.
// =================================================================
bool CClassReferenceDataType::IsEqualTo(CSemanter* semanter, CDataType* type)
{
	CClassReferenceDataType* classRef = dynamic_cast<CClassReferenceDataType*>(type) ;
	return (classRef != NULL && classRef->m_class == m_class);
}

// =================================================================
//	Checks if this data type extends another.
// =================================================================
bool CClassReferenceDataType::CanCastTo(CSemanter* semanter, CDataType* type)
{
	return IsEqualTo(semanter, type);
}

// =================================================================
//	Converts data type to string.
// =================================================================
std::string	CClassReferenceDataType::ToString()
{
	return m_class->ToString();
}

// =================================================================
//	Gets the class this data type is based on.
// =================================================================
CClassASTNode* CClassReferenceDataType::GetClass(CSemanter* semanter)
{
	return m_class;
}

// -----------------------------------------------------------------------------
// 	CStringDataType.ls
// 	Copyright (C) 2012-2013 TwinDrills, All Rights Reserved
// -----------------------------------------------------------------------------
using System.*;
using System.Collections.*;

// =================================================================
//	Base class for all data types.
// =================================================================
public class CStringDataType : CDataType
{
	public CStringDataType(CToken token)
	{
		Token = token;
	}
	
	public virtual override CClassASTNode GetClass(CSemanter semanter)
	{	
		return semanter.GetContext().GetASTRoot().FindDeclaration(semanter, "string").Semant(semanter) as CClassASTNode;
	}
	
	public virtual override bool IsEqualTo(CSemanter semanter, CDataType type)
	{
		return (type as CStringDataType) != null;
	}
	
	public virtual override bool CanCastTo(CSemanter semanter, CDataType type)
	{
		CObjectDataType obj = type as CObjectDataType;

		if (obj != null)
		{
			// Can be upcast to anything that its boxed class allows.
			if (type.GetClass(semanter).Identifier == "object" &&
				GetBoxClass(semanter) != null)
			{
				// Look to see if our box-class contains an argument that accepts us.
				CClassASTNode			node	= GetBoxClass(semanter);
				CClassMemberASTNode		field 	= node == null ? null : node.FindClassField(semanter, "Value", null, null); 
				
				return field != null && field.ReturnType.IsEqualTo(semanter, this);
			}
		}
		else
		{
			return IsEqualTo(semanter, type);
		}

		return false;	
	}
	
	public virtual override string ToString()
	{
		return "string";
	}
}


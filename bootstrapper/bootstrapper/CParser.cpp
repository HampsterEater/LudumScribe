/* *****************************************************************

		CParser.cpp

		Copyright (C) 2012 Tim Leonard - All Rights Reserved

   ***************************************************************** */

#include <string>
#include <stdio.h>
#include <assert.h>

#include "CCompiler.h"
#include "CParser.h"
#include "CStringHelper.h"
#include "CPathHelper.h"
#include "CTranslationUnit.h"

#include "CASTNode.h"
#include "CPackageASTNode.h"
#include "CClassASTNode.h"
#include "CClassBodyASTNode.h"
#include "CClassMemberASTNode.h"
#include "CMethodBodyASTNode.h"

#include "CExpressionASTNode.h"
#include "CAssignmentExpressionASTNode.h"
#include "CBinaryMathExpressionASTNode.h"
#include "CComparisonExpressionASTNode.h"
#include "CIdentifierExpressionASTNode.h"
#include "CLiteralExpressionASTNode.h"
#include "CLogicalExpressionASTNode.h"
#include "CTernaryExpressionASTNode.h"
#include "CThisExpressionASTNode.h"
#include "CTypeExpressionASTNode.h"
#include "CPreFixExpressionASTNode.h"
#include "CPostFixExpressionASTNode.h"
#include "CIndexExpressionASTNode.h"
#include "CSliceExpressionASTNode.h"
#include "CMethodCallExpressionASTNode.h"
#include "CFieldAccessExpressionASTNode.h"
#include "CCastExpressionASTNode.h"
#include "CCommaExpressionASTNode.h"
#include "CClassRefExpressionASTNode.h"

#include "CIfStatementASTNode.h"
#include "CBlockStatementASTNode.h"
#include "CWhileStatementASTNode.h"
#include "CBreakStatementASTNode.h"
#include "CContinueStatementASTNode.h"
#include "CDoStatementASTNode.h"
#include "CSwitchStatementASTNode.h"
#include "CForStatementASTNode.h"
#include "CForEachStatementASTNode.h"
#include "CReturnStatementASTNode.h"
#include "CCaseStatementASTNode.h"
#include "CDefaultStatementASTNode.h"
#include "CVariableStatementASTNode.h"
#include "CBaseExpressionASTNode.h"
#include "CNewExpressionASTNode.h"
#include "CTryStatementASTNode.h"
#include "CCatchStatementASTNode.h"
#include "CThrowStatementASTNode.h"

#include "CDataType.h"
#include "CBoolDataType.h"
#include "CArrayDataType.h"
#include "CFloatDataType.h"
#include "CIdentifierDataType.h"
#include "CIntDataType.h"
#include "CObjectDataType.h"
#include "CStringDataType.h"
#include "CVoidDataType.h"
#include "CNullDataType.h"

// =================================================================
//	Constructs the parser.
// =================================================================
CParser::CParser() :
	m_scope(NULL),
	m_root(NULL)
{

}

// =================================================================
//	Returns true if at the offset is beyond the end of tokens.
// =================================================================
bool CParser::EndOfTokens(int offset)
{
	return m_token_offset + offset >= (int)m_context->GetTokenList().size();
}

// =================================================================
//	Advances the token stream and returns the next token.
// =================================================================
CToken& CParser::NextToken()
{
	if (EndOfTokens())
	{
		return m_eof_token;
	}

	CToken& token = m_context->GetTokenList().at(m_token_offset);
	m_token_offset++;

	return token;
}

// =================================================================
//	Returns a token at an offset ahead in the stream.
// =================================================================
CToken& CParser::LookAheadToken(int offset)
{	
	offset--;

	if (EndOfTokens(offset))
	{
		return m_eof_token;
	}

	return m_context->GetTokenList().at(m_token_offset + offset);
}

// =================================================================
//	Returns current token in the stream.
// =================================================================
CToken& CParser::CurrentToken()
{
	return m_context->GetTokenList().at(m_token_offset - 1);
}

// =================================================================
//	Returns previous token in the stream.
// =================================================================
CToken& CParser::PreviousToken()
{
	if (m_token_offset < 2)
	{
		return m_sof_token;
	}

	return m_context->GetTokenList().at(m_token_offset - 2);
}

// =================================================================
//	Advances the token stream and throws an error if the next
//	token is not what was expected.
// =================================================================
CToken& CParser::ExpectToken(TokenIdentifier::Type type)
{
	CToken& token = NextToken();
	if (token.Type != type)
	{
		m_context->FatalError(CStringHelper::FormatString("Unexpected token '%s' (0x%x).", token.Literal.c_str(), (int)token.Type), token);
	}
	return token;
}

// =================================================================
//	Rewinds the token stream by the given amount.
// =================================================================
void CParser::RewindStream(int offset)
{
	m_token_offset -= offset;
}

// =================================================================
//	Pushs a scope onto the stack.
// =================================================================
void CParser::PushScope(CASTNode* node)
{
	m_scope_stack.push_back(m_scope);
	m_scope = node;
}

// =================================================================
//	Pops a scope off the scope stack.
// =================================================================
void CParser::PopScope()
{
	m_scope = m_scope_stack.at(m_scope_stack.size() - 1);
	m_scope_stack.pop_back();
}

// =================================================================
//	Retrieves the current scope.
// =================================================================
CASTNode* CParser::CurrentScope()
{
	return m_scope;
}

// =================================================================
//	Retrieves the current class scope.
// =================================================================
CClassASTNode* CParser::CurrentClassScope()
{
	CASTNode* scope = m_scope;

	while (true)
	{
		CClassASTNode* class_scope = dynamic_cast<CClassASTNode*>(scope);
		if (class_scope != NULL)
		{
			return class_scope;
		}
		scope = scope->Parent;
	}

	m_context->FatalError("Expecting to be inside class scope.", CurrentToken());
}

// =================================================================
//	Retrieves the current class member scope.
// =================================================================
CClassMemberASTNode* CParser::CurrentClassMemberScope()
{
	CASTNode* scope = m_scope;

	while (true)
	{
		CClassMemberASTNode* class_scope = dynamic_cast<CClassMemberASTNode*>(scope);
		if (class_scope != NULL)
		{
			return class_scope;
		}
		scope = scope->Parent;
	}

	m_context->FatalError("Expecting to be inside class member scope.", CurrentToken());
}

// =================================================================
//	Get the root node of the AST.
// =================================================================
CASTNode* CParser::GetASTRoot() 
{ 
	return m_root;
}

// =================================================================
//	Processes input and performs the actions requested.
// =================================================================
bool CParser::Process(CTranslationUnit* context)
{	
	std::vector<CToken>& tokens = context->GetTokenList();

	m_context		= context;
	m_token_offset	= 0;

	// Create an end of file token for use in errors.
	m_eof_token.Row		= 1;
	m_eof_token.Column	= 1;
	if (tokens.size() > 1)
	{
		m_eof_token			= tokens.at(m_context->GetTokenList().size() - 1);
		m_eof_token.Column	= m_eof_token.Column + m_eof_token.Literal.size();
	}
	m_eof_token.Type		= TokenIdentifier::EndOfFile;
	m_eof_token.Literal		= "<end-of-file>";
	m_sof_token.SourceFile	= m_context->GetFilePath();

	// Create a start of file token for use in errors.
	m_sof_token.Type		= TokenIdentifier::StartOfFile;
	m_sof_token.Literal		= "<start-of-file>";
	m_sof_token.SourceFile	= m_context->GetFilePath();
	m_sof_token.Row			= 1;
	m_sof_token.Column		= 1;

	// Create the root AST node.
	m_root  = new CPackageASTNode(NULL, m_sof_token);
	PushScope(m_root);

	// Keep parsing top-level statements until we 
	// run out of tokens.
	while (!EndOfTokens())
	{
		ParseTopLevelStatement();
	}

	PopScope();

	return true;
}

// =================================================================
//	Evalulates an expression.
// =================================================================
bool CParser::Evaluate(CTranslationUnit* context)
{	
	std::vector<CToken>& tokens = context->GetTokenList();

	m_context		= context;
	m_token_offset	= 0;

	// Create an end of file token for use in errors.
	m_eof_token.Row		= 1;
	m_eof_token.Column	= 1;
	if (tokens.size() > 1)
	{
		m_eof_token			= tokens.at(m_context->GetTokenList().size() - 1);
		m_eof_token.Column	= m_eof_token.Column + m_eof_token.Literal.size();
	}
	m_eof_token.Type		= TokenIdentifier::EndOfFile;
	m_eof_token.Literal		= "<end-of-file>";
	m_sof_token.SourceFile	= m_context->GetFilePath();

	// Create a start of file token for use in errors.
	m_sof_token.Type		= TokenIdentifier::StartOfFile;
	m_sof_token.Literal		= "<start-of-file>";
	m_sof_token.SourceFile	= m_context->GetFilePath();
	m_sof_token.Row			= 1;
	m_sof_token.Column		= 1;

	// Create the root AST node.
	m_root = ParseExpr();

	return EndOfTokens();
}

// =================================================================
//	Parses a top-level statement. Using/Class/Etc
// =================================================================
void CParser::ParseTopLevelStatement()
{
	if (EndOfTokens())
	{
		return;
	}

	CToken token = NextToken();
	switch (token.Type)
	{
		// Using statement.
		case TokenIdentifier::KEYWORD_USING:
			{
				ParseUsingStatement();
				return;
			}

		// Class statement.
		case TokenIdentifier::KEYWORD_PUBLIC:
		case TokenIdentifier::KEYWORD_PRIVATE:
		case TokenIdentifier::KEYWORD_PROTECTED:
		case TokenIdentifier::KEYWORD_STATIC:
		case TokenIdentifier::KEYWORD_ABSTRACT:
		case TokenIdentifier::KEYWORD_INTERFACE:
		case TokenIdentifier::KEYWORD_CLASS:
			{
				ParseClassStatement();
				return;
			}

		// Dafuq?
		default:
			{
				m_context->FatalError(CStringHelper::FormatString("Unexpected token '%s' (0x%x).", token.Literal.c_str(), (int)token.Type), token);
				return;
			}
	}
}

// =================================================================
//	Parses a using statement: using x.y.z;
// =================================================================
void CParser::ParseUsingStatement()
{
	bool path_is_dir = false;
	std::vector<std::string> path_segments;
	CToken start_token = CurrentToken();
	int counter = 0;

	bool isNative = false;

	// We looking for native files?
	if (LookAheadToken().Type == TokenIdentifier::KEYWORD_NATIVE)
	{
		ExpectToken(TokenIdentifier::KEYWORD_NATIVE);
		isNative = true;
	}

	while (true)
	{
		if (counter > 0 && LookAheadToken().Type == TokenIdentifier::OP_MUL)
		{
			CToken token = ExpectToken(TokenIdentifier::OP_MUL);
			path_is_dir = true;

			break;
		}
		else
		{
			CToken token = ExpectToken(TokenIdentifier::IDENTIFIER);
			path_segments.push_back(token.Literal);
		}

		counter++;

		if (LookAheadToken().Type != TokenIdentifier::SEMICOLON)
		{
			ExpectToken(TokenIdentifier::PERIOD);
		}
		else
		{
			break;
		}
	}

	ExpectToken(TokenIdentifier::SEMICOLON);

	// Compile into a relative file path.
	std::string path = "";
	std::string using_statement = "";
	for (unsigned int i = 0; i < path_segments.size(); i++)
	{
		if (using_statement != "")
		{
			using_statement += ".";
		}
		using_statement += path_segments.at(i);
		path += "/" + path_segments.at(i);
	}

	// Try and file this file.
	std::string file_ext	 = m_context->GetCompiler()->GetFileExtension();
	std::string package_path = m_context->GetCompiler()->GetPackageDirectory();
	std::string local_path	 = CPathHelper::GetAbsolutePath(CPathHelper::StripFilename(m_context->GetFilePath()) + path);
	std::string remote_path	 = CPathHelper::GetAbsolutePath(package_path + path);
	std::string final_path	 = "";

	if (isNative == true)
	{
		file_ext = m_context->GetCompiler()->GetProjectConfig().GetString("TRANSLATOR_NATIVE_FILE_EXTENSION");
	}

	// Referenced as a wildcard directory.
	if (path_is_dir == true)
	{
		// Local path?
		if (CPathHelper::IsDirectory(local_path))
		{
			final_path = local_path;
		}

		// Remote path?
		else if (CPathHelper::IsDirectory(remote_path))
		{
			final_path = remote_path;
		}

		// Dosen't exist?
		else
		{
			m_context->FatalError(CStringHelper::FormatString("Unable to find referenced directory '%s'.", using_statement.c_str()), start_token);
			return;
		}

		// List all files in directory.
		std::vector<std::string> files = CPathHelper::ListFiles(final_path);
		for (auto iter = files.begin(); iter != files.end(); iter++)
		{
			std::string file_path = final_path + "/" + (*iter);		
			
			// Check extension.
			if (CPathHelper::ExtractExtension(file_path) != file_ext)
			{
				continue;
			}

			if (!m_context->AddUsingFile(file_path, isNative))
			{
				m_context->Warning(CStringHelper::FormatString("Using statement imports duplicate file '%s'.", file_path.c_str()), start_token);
			}
		}
	}

	// Referenced as a file.
	else
	{
		local_path += "." + file_ext;
		remote_path += "." + file_ext;

		// Local path?
		if (CPathHelper::IsFile(local_path))
		{
			final_path = local_path;
		}

		// Remote path?
		else if (CPathHelper::IsFile(remote_path))
		{
			final_path = remote_path;
		}

		// Dosen't exist?
		else
		{
			m_context->FatalError(CStringHelper::FormatString("Unable to find referenced file '%s'.", using_statement.c_str()), start_token);
			return;
		}

		// Store the file.
		if (!m_context->AddUsingFile(final_path, isNative))
		{
			m_context->Warning(CStringHelper::FormatString("Using statement imports duplicate file '%s'.", final_path.c_str()), start_token);
		}
	}
}

// =================================================================
//	Parses a class statement: public class XYZ { }
// =================================================================
CClassASTNode* CParser::ParseClassStatement()
{
	CToken& start_token = CurrentToken();
	CClassASTNode* classNode = new CClassASTNode(CurrentScope(), start_token);

	// Read in all attributes.
	bool readAccessLevel = false;
	bool readingAttributes = true;	
	CToken& token = start_token;

	while (readingAttributes)
	{
		switch (token.Type)
		{
			case TokenIdentifier::KEYWORD_PUBLIC:
				{
					if (readAccessLevel == true)
					{						
						m_context->FatalError("Encountered duplicate access level attribute.", token);
					}
					classNode->AccessLevel = AccessLevel::PUBLIC;
					readAccessLevel = false;
					break;
				}
			
			case TokenIdentifier::KEYWORD_PRIVATE:
				{
					if (readAccessLevel == true)
					{						
						m_context->FatalError("Encountered duplicate access level attribute.", token);
					}
					classNode->AccessLevel = AccessLevel::PRIVATE;
					readAccessLevel = false;
					break;
				}

			case TokenIdentifier::KEYWORD_PROTECTED:
				{
					if (readAccessLevel == true)
					{						
						m_context->FatalError("Encountered duplicate access level attribute.", token);
					}
					classNode->AccessLevel = AccessLevel::PROTECTED;
					readAccessLevel = false;
					break;
				}

			case TokenIdentifier::KEYWORD_STATIC:
				{
					if (classNode->IsAbstract == true)
					{						
						m_context->FatalError("Abstract class cannot be declared as static.", token);
					}
					if (classNode->IsStatic == true)
					{						
						m_context->FatalError("Encountered duplicate static attribute.", token);
					}
				//	if (classNode->IsNative == true)
				//	{						
				//		m_context->FatalError("Native class cannot be declared as static.", token);
				//	}
					classNode->IsStatic = true;
					break;
				}

			case TokenIdentifier::KEYWORD_ABSTRACT:
				{
					if (classNode->IsStatic == true)
					{						
						m_context->FatalError("Static class cannot be declared as abstract.", token);
					}
					if (classNode->IsAbstract == true)
					{						
						m_context->FatalError("Encountered duplicate abstract attribute.", token);
					}
					if (classNode->IsNative == true)
					{						
						m_context->FatalError("Native class cannot be declared as abstract.", token);
					}
					classNode->IsAbstract = true;
					break;
				}

			case TokenIdentifier::KEYWORD_SEALED:
				{
					if (classNode->IsStatic == true)
					{						
						m_context->FatalError("Static class cannot be declared as sealed.", token);
					}
					if (classNode->IsAbstract == true)
					{						
						m_context->FatalError("Abstract class cannot be declared as sealed.", token);
					}
					if (classNode->IsAbstract == true)
					{						
						m_context->FatalError("Abstract class cannot be declared as sealed.", token);
					}
					classNode->IsSealed = true;
					break;
				}

			case TokenIdentifier::KEYWORD_NATIVE:
				{
				//	if (classNode->IsStatic == true)
				//	{						
				//		m_context->FatalError("Static class cannot be declared as native.", token);
				//	}
					if (classNode->IsAbstract == true)
					{						
						m_context->FatalError("Abstract class cannot be declared as native.", token);
					}
					if (classNode->IsNative == true)
					{						
						m_context->FatalError("Encountered duplicate native attribute.", token);
					}
					classNode->IsNative = true;

					ExpectToken(TokenIdentifier::OPEN_PARENT);
					classNode->MangledIdentifier = ExpectToken(TokenIdentifier::STRING_LITERAL).Literal;
					ExpectToken(TokenIdentifier::CLOSE_PARENT);

					break;
				}
				

			case TokenIdentifier::KEYWORD_BOX:
				{
					if (classNode->HasBoxClass == true)
					{						
						m_context->FatalError("Encountered duplicate box class attribute.", token);
					}

					ExpectToken(TokenIdentifier::OPEN_PARENT);
					classNode->HasBoxClass = true;
					classNode->BoxClassIdentifier = ExpectToken(TokenIdentifier::STRING_LITERAL).Literal;
					ExpectToken(TokenIdentifier::CLOSE_PARENT);

					break;
				}

			case TokenIdentifier::KEYWORD_INTERFACE:
				{
					if (classNode->IsStatic == true)
					{						
						m_context->FatalError("Interfaces cannot be declared as static.", token);
					}
					if (classNode->IsAbstract == true)
					{						
						m_context->FatalError("Interfaces cannot be declared as abstract.", token);
					}
					classNode->IsInterface = true;
					readingAttributes = false;
					break;
				}

			case TokenIdentifier::KEYWORD_CLASS:
				{
					readingAttributes = false;
					break;
				}

			default:
				{
					m_context->FatalError(CStringHelper::FormatString("Unexpected token while parsing class attributes '%s' (0x%x).", token.Literal.c_str(), (int)token.Type), token);
					break;
				}
		}

		// Read in next attribute.
		if (readingAttributes == true)
		{
			token = NextToken();
		}
	}
	
	// Read in the identifier.
	CToken& ident_token = ExpectToken(TokenIdentifier::IDENTIFIER);
	classNode->Identifier = ident_token.Literal;
	classNode->Token = ident_token;

	// Read in generic tags.
	if (LookAheadToken().Type == TokenIdentifier::OP_LESS)
	{
		if (classNode->IsInterface == true)
		{
			m_context->FatalError("Interfaces cannot be generic.", token);
		}
	//	if (classNode->IsNative == true)
	//	{
	//		m_context->FatalError("Native classes cannot be generics.", token);
	//	}

		classNode->IsGeneric = true;

		PushScope(classNode);
		ExpectToken(TokenIdentifier::OP_LESS);

		while (true)
		{
			CToken& generic_token = ExpectToken(TokenIdentifier::IDENTIFIER);
			classNode->GenericTypeTokens.push_back(generic_token);

			if (LookAheadToken().Type == TokenIdentifier::OP_GREATER)
			{
				break;
			}
			else
			{
				ExpectToken(TokenIdentifier::COMMA);
			}
		}

		ExpectToken(TokenIdentifier::OP_GREATER);
		PopScope();
	}

	// Read in all inherited classes.
	if (LookAheadToken().Type == TokenIdentifier::COLON)
	{
		NextToken();
		PushScope(classNode);

		bool continueParsing = true;

		if (LookAheadToken().Type == TokenIdentifier::KEYWORD_NULL)
		{
			ExpectToken(TokenIdentifier::KEYWORD_NULL);

			classNode->InheritsNull = true;
			if (classNode->IsNative == false)
			{
				m_context->FatalError("Only native classes can inherit from NULL.", token);
			}
			if (classNode->IsInterface == true)
			{
				m_context->FatalError("Interfaces cannot inherit from NULL.", token);
			}

			continueParsing = false;
			if (LookAheadToken().Type == TokenIdentifier::COMMA)
			{
				continueParsing = true;
				ExpectToken(TokenIdentifier::COMMA);
			}
		}
		
		if (continueParsing == true)
		{
			while (true)
			{
				classNode->InheritedTypes.push_back(ParseIdentifierDataType());
			
				if (LookAheadToken().Type == TokenIdentifier::COMMA)
				{
					NextToken();
				}
				else
				{
					break;
				}
			}
		}

		PopScope();
	}

	// Read in class block.
	ExpectToken(TokenIdentifier::OPEN_BRACE);

	PushScope(classNode);
	classNode->Body = ParseClassBody();
	PopScope();

	ExpectToken(TokenIdentifier::CLOSE_BRACE);

	return classNode;
}

// =================================================================
//	Parses statements contained in a class body.
//	Does not deal with reading the { and } from the token stream.
// =================================================================
CClassBodyASTNode* CParser::ParseClassBody()
{
	CClassBodyASTNode* body = new CClassBodyASTNode(CurrentScope(), CurrentToken());
	PushScope(body);

	while (true)
	{
		CToken token = LookAheadToken();
		if (token.Type == TokenIdentifier::CLOSE_BRACE)
		{
			break;
		}
		else if (token.Type == TokenIdentifier::EndOfFile)
		{
			m_context->FatalError("Unexpected end-of-file, expecting closing brace.", token);
		}
		ParseClassBodyStatement();
	}

	PopScope();

	return body;
}

// =================================================================
//	Parses a statement contained in a class body.
// =================================================================
CASTNode* CParser::ParseClassBodyStatement()
{
	CToken token = NextToken();
	switch (token.Type)
	{
		// Empty Statement.
		case TokenIdentifier::SEMICOLON:
			{
				return NULL;
			}

		// Variable/Function statement.
		case TokenIdentifier::KEYWORD_PUBLIC:
		case TokenIdentifier::KEYWORD_PRIVATE:
		case TokenIdentifier::KEYWORD_PROTECTED:
		case TokenIdentifier::KEYWORD_STATIC:
		case TokenIdentifier::KEYWORD_ABSTRACT:
		case TokenIdentifier::KEYWORD_VIRTUAL:
		case TokenIdentifier::KEYWORD_CONST:

//		case TokenIdentifier::KEYWORD_OBJECT:
		case TokenIdentifier::KEYWORD_BOOL:
		case TokenIdentifier::KEYWORD_VOID:
		//case TokenIdentifier::KEYWORD_BYTE:
		//case TokenIdentifier::KEYWORD_SHORT:
		case TokenIdentifier::KEYWORD_INT:
		//case TokenIdentifier::KEYWORD_LONG:
		case TokenIdentifier::KEYWORD_FLOAT:
		//case TokenIdentifier::KEYWORD_DOUBLE:
		case TokenIdentifier::KEYWORD_STRING:
		case TokenIdentifier::IDENTIFIER:
		case TokenIdentifier::OP_NOT:
			{
				return ParseClassMemberStatement();
			}

		// Dafuq?
		default:
			{
				m_context->FatalError(CStringHelper::FormatString("Unexpected token '%s' (0x%x).", token.Literal.c_str(), (int)token.Type), token);
			}
	}

	return NULL;
}

// =================================================================
//	Parses a statement that can either be a member or a method.
// =================================================================
CClassMemberASTNode* CParser::ParseClassMemberStatement()
{
	CToken& start_token = CurrentToken();
	CClassASTNode* classScope = CurrentClassScope();
	CClassMemberASTNode* classMemberNode = new CClassMemberASTNode(CurrentScope(), start_token);

	// Read in all attributes.
	bool readAccessLevel = false;
	bool readingAttributes = true;	
	CToken token = start_token;

	while (readingAttributes)
	{
		switch (token.Type)
		{
			case TokenIdentifier::KEYWORD_PUBLIC:
				{
					if (readAccessLevel == true)
					{						
						m_context->FatalError("Encountered duplicate access level attribute.", token);
					}
					classMemberNode->AccessLevel = AccessLevel::PUBLIC;
					readAccessLevel = false;
					break;
				}
			
			case TokenIdentifier::KEYWORD_PRIVATE:
				{
					if (readAccessLevel == true)
					{						
						m_context->FatalError("Encountered duplicate access level attribute.", token);
					}
					classMemberNode->AccessLevel = AccessLevel::PRIVATE;
					readAccessLevel = false;
					break;
				}
				
			case TokenIdentifier::KEYWORD_PROTECTED:
				{
					if (readAccessLevel == true)
					{						
						m_context->FatalError("Encountered duplicate access level attribute.", token);
					}
					classMemberNode->AccessLevel = AccessLevel::PROTECTED;
					readAccessLevel = false;
					break;
				}

			case TokenIdentifier::KEYWORD_STATIC:
				{
					if (classMemberNode->IsAbstract == true)
					{						
						m_context->FatalError("Abstract class member cannot be declared as static.", token);
					}
					if (classMemberNode->IsVirtual == true)
					{						
						m_context->FatalError("Virtual class member cannot be declared as static.", token);
					}
					if (classMemberNode->IsConst == true)
					{						
						m_context->Warning("Constant values are implicitly static, unneccessary static keyword.", token);
					}
					else if (classMemberNode->IsStatic == true)
					{						
						m_context->FatalError("Encountered duplicate static attribute.", token);
					}
					if (classMemberNode->IsOverride == true)
					{						
						m_context->FatalError("Overriden class member cannot be declared as static.", token);
					}
				//	if (classMemberNode->IsNative == true)
				//	{						
				//		m_context->FatalError("Native class member cannot be declared static.", token);
				//	}
					classMemberNode->IsStatic = true;
					break;
				}

			case TokenIdentifier::KEYWORD_ABSTRACT:
				{
					if (classMemberNode->IsStatic == true)
					{						
						m_context->FatalError("Static class member cannot be declared as abstract.", token);
					}
					if (classMemberNode->IsVirtual == true)
					{						
						m_context->FatalError("Virtual class member cannot be declared as abstract.", token);
					}
					if (classMemberNode->IsConst == true)
					{						
						m_context->FatalError("Const class member cannot be declared abstract.", token);
					}
					if (classMemberNode->IsNative == true)
					{						
						m_context->FatalError("Native class member cannot be declared abstract.", token);
					}
					if (classMemberNode->IsOverride == true)
					{						
						m_context->FatalError("Overriden class member cannot be declared as abstract.", token);
					}
					if (classMemberNode->IsAbstract == true)
					{						
						m_context->FatalError("Encountered duplicate abstract attribute.", token);
					}
					classMemberNode->IsAbstract = true;
					classMemberNode->IsVirtual  = true;
					break;
				}

			case TokenIdentifier::KEYWORD_VIRTUAL:
				{
					if (classMemberNode->IsStatic == true)
					{						
						m_context->FatalError("Static class member cannot be declared as abstract.", token);
					}
					if (classMemberNode->IsAbstract == true)
					{						
						m_context->FatalError("Abstract class member cannot be declared as virtual.", token);
					}
					if (classMemberNode->IsConst == true)
					{						
						m_context->FatalError("Const class member cannot be declared virtual.", token);
					}
					if (classMemberNode->IsOverride == true)
					{						
						m_context->Warning("Overriden class members are implicitly virtual, unneccessary virtual keyword.", token);
					}
					else if (classMemberNode->IsVirtual == true)
					{						
						m_context->FatalError("Encountered duplicate virtual attribute.", token);
					}
					classMemberNode->IsVirtual = true;
					break;
				}
				
			case TokenIdentifier::KEYWORD_CONST:
				{
					if (classMemberNode->IsStatic == true)
					{						
						m_context->Warning("Constant values are implicitly static, unneccessary static keyword.", token);
					}
					if (classMemberNode->IsAbstract == true)
					{						
						m_context->FatalError("Abstract class member cannot be declared as const.", token);
					}
					if (classMemberNode->IsVirtual == true)
					{						
						m_context->FatalError("Virtual class member cannot be declared as const.", token);
					}
					if (classMemberNode->IsNative == true)
					{						
						m_context->FatalError("Native class member cannot be declared as const.", token);
					}
					if (classMemberNode->IsOverride == true)
					{						
						m_context->FatalError("Overriden class member cannot be declared as const.", token);
					}
					if (classMemberNode->IsConst == true)
					{						
						m_context->FatalError("Encountered duplicate const attribute.", token);
					}
					classMemberNode->IsStatic = true;
					classMemberNode->IsConst = true;
					break;
				}

			case TokenIdentifier::KEYWORD_NATIVE:
				{
				//	if (classMemberNode->IsStatic == true)
				//	{						
				//		m_context->FatalError("Static class member cannot be declared as native.", token);
				//	}
					if (classMemberNode->IsAbstract == true)
					{						
						m_context->FatalError("Abstract class member cannot be declared as native.", token);
					}
					if (classMemberNode->IsConst == true)
					{						
						m_context->FatalError("Const class member cannot be declared as native.", token);
					}
				//	if (classMemberNode->IsOverride == true)
				//	{						
				//		m_context->FatalError("Overriden class member cannot be declared as native.", token);
				//	}
					if (classMemberNode->IsNative == true)
					{						
						m_context->FatalError("Encountered duplicate native attribute.", token);
					}

					ExpectToken(TokenIdentifier::OPEN_PARENT);
					classMemberNode->MangledIdentifier = ExpectToken(TokenIdentifier::STRING_LITERAL).Literal;
					ExpectToken(TokenIdentifier::CLOSE_PARENT);

					classMemberNode->IsNative = true;
					break;
				}
				
			case TokenIdentifier::KEYWORD_OVERRIDE:
				{
					if (classMemberNode->IsStatic == true)
					{						
						m_context->FatalError("Static class member cannot be declared as overidden.", token);
					}
					if (classMemberNode->IsAbstract == true)
					{						
						m_context->FatalError("Abstract class member cannot be declared as overidden.", token);
					}
					if (classMemberNode->IsConst == true)
					{						
						m_context->FatalError("Const class member cannot be declared as overidden.", token);
					}
				//	if (classMemberNode->IsNative == true)
				//	{						
				//		m_context->FatalError("Native class member cannot be declared as overidden.", token);
				//	}
					if (classMemberNode->IsOverride == true)
					{						
						m_context->FatalError("Encountered duplicate override attribute.", token);
					}

					classMemberNode->IsOverride = true;
					classMemberNode->IsVirtual = true;
					break;
				}

			case TokenIdentifier::KEYWORD_BOOL:
			case TokenIdentifier::KEYWORD_VOID:
			case TokenIdentifier::KEYWORD_INT:
			case TokenIdentifier::KEYWORD_FLOAT:
			case TokenIdentifier::KEYWORD_STRING:
			case TokenIdentifier::IDENTIFIER:
			case TokenIdentifier::OP_NOT:
				{
					RewindStream();
					readingAttributes = false;
					break;
				}

			default:
				{
					m_context->FatalError(CStringHelper::FormatString("Unexpected token while parsing class attributes '%s' (0x%x).", token.Literal.c_str(), (int)token.Type), token);
					break;
				}
		}

		// Read in next attribute.
		if (readingAttributes == true)
		{
			token = NextToken();
		}
	}
	
	// Read in the data type.
	CASTNode* scope = CurrentScope();
	PushScope(classMemberNode);

	// Are we a constructor?
	CToken& lookAhead = LookAheadToken();
	CToken& lookAhead2 = LookAheadToken(2);

	if (lookAhead.Type == TokenIdentifier::IDENTIFIER &&
			 lookAhead.Literal == classScope->Identifier &&
			 lookAhead2.Type == TokenIdentifier::OPEN_PARENT)
	{
		if (classMemberNode->IsStatic == true)
		{
			m_context->FatalError("Constructors cannot be static.", CurrentToken());	
		}
		if (classMemberNode->IsAbstract == true)
		{
			m_context->FatalError("Constructors cannot be abstract.", CurrentToken());	
		}
		if (classMemberNode->IsVirtual == true)
		{
			m_context->FatalError("Constructors cannot be virtual.", CurrentToken());	
		}
		if (classMemberNode->IsOverride == true)
		{
			m_context->FatalError("Constructors cannot be overriden.", CurrentToken());	
		}

		ExpectToken(TokenIdentifier::IDENTIFIER);

		classMemberNode->Identifier = CurrentToken().Literal;
		classMemberNode->ReturnType = new CVoidDataType(CurrentToken());
		classMemberNode->IsConstructor = true;
	}
	else
	{
		classMemberNode->ReturnType = ParseDataType();

		ExpectToken(TokenIdentifier::IDENTIFIER);
		classMemberNode->Identifier = CurrentToken().Literal;
	}

	// Read in identifier.
	classMemberNode->MemberType = MemberType::Field;

	// Read in arguments (if available).
	if (LookAheadToken().Type == TokenIdentifier::OPEN_PARENT)
	{
		ExpectToken(TokenIdentifier::OPEN_PARENT);
		ParseMethodArguments(classMemberNode);
		ExpectToken(TokenIdentifier::CLOSE_PARENT);

		classMemberNode->MemberType = MemberType::Method;
	}
	else
	{		
		if (classMemberNode->IsAbstract == true)
		{
			m_context->FatalError("Class fields cannot be declared abstract.", CurrentToken());
		}
		if (classMemberNode->IsVirtual == true)
		{
			m_context->FatalError("Class fields cannot be declared virtual.", CurrentToken());
		}
		if (classMemberNode->IsOverride == true)
		{
			m_context->FatalError("Class fields cannot be declared as overridden.", CurrentToken());
		}
		if (CurrentClassScope()->IsInterface == true)
		{
			m_context->FatalError("Interfaces can only contain method declarations.", CurrentToken());
		}
	}

	// Can't add non-native variables to native-classes.
	if (classMemberNode->MemberType == MemberType::Field && classScope->IsNative == true && classMemberNode->IsNative == false && classMemberNode->IsStatic == false)
	{
		m_context->FatalError(CStringHelper::FormatString("Cannot insert non-native instance variable '%s' into native class '%s'.", classMemberNode->Identifier.c_str(), classScope->Identifier.c_str()), CurrentToken());
	}
	classMemberNode->IsExtension = (classScope->IsNative == true && classMemberNode->IsNative == false);

	// Read in equal value.
	if (classMemberNode->MemberType == MemberType::Field &&
		LookAheadToken().Type == TokenIdentifier::OP_ASSIGN)
	{
		if (classMemberNode->IsNative == true)
		{
			m_context->FatalError("Native members cannot be assigned values.", CurrentToken());
		}

		ExpectToken(TokenIdentifier::OP_ASSIGN);

		if (classMemberNode->IsConst == true)
		{
			classMemberNode->Assignment = ParseConstExpr();
		}
		else
		{
			classMemberNode->Assignment = ParseExpr();
		}
	}
	else if (classMemberNode->MemberType == MemberType::Field)
	{
		if (classMemberNode->IsConst == true)
		{
			m_context->FatalError("Constant member expects initialization expression.", CurrentToken());
		}
	}

	// Read in block.
	if (classMemberNode->MemberType == MemberType::Method &&
		LookAheadToken().Type == TokenIdentifier::OPEN_BRACE)
	{
		if (CurrentClassScope()->IsInterface == true)
		{
			m_context->FatalError("Interface methods cannot have bodies.", CurrentToken());
		}
		if (classMemberNode->IsNative == true)
		{
			m_context->FatalError("Native methods cannot have bodies.", CurrentToken());
		}

		ExpectToken(TokenIdentifier::OPEN_BRACE);

		classMemberNode->Body = ParseMethodBody();

		ExpectToken(TokenIdentifier::CLOSE_BRACE);
	}
	else
	{
		// Check if body etc is correct.
		if (classMemberNode->MemberType == MemberType::Method &&
			classMemberNode->IsAbstract == false &&
			CurrentClassScope()->IsInterface == false &&
			CurrentClassScope()->IsNative == false)
		{
			m_context->FatalError("Expecting method body declaration.", CurrentToken());
		}

		ExpectToken(TokenIdentifier::SEMICOLON);
	}

	PopScope();

	return classMemberNode;
}

// =================================================================
//	Parses a data type value.
// =================================================================
CDataType* CParser::ParseDataType()
{
	CToken& token = NextToken();

	CDataType* baseDataTypeNode = NULL;
	
	// Read in main data type.
	switch (token.Type)
	{
		case TokenIdentifier::KEYWORD_BOOL:
			{
				baseDataTypeNode = new CBoolDataType(token);
				break;
			}
		case TokenIdentifier::KEYWORD_VOID:
			{
				baseDataTypeNode = new CVoidDataType(token);
				break;
			}
		case TokenIdentifier::KEYWORD_INT:
			{
				baseDataTypeNode = new CIntDataType(token);
				break;
			}
		case TokenIdentifier::KEYWORD_FLOAT:
			{
				baseDataTypeNode = new CFloatDataType(token);
				break;
			}
		case TokenIdentifier::KEYWORD_STRING:
			{
				baseDataTypeNode = new CStringDataType(token);
				break;
			}
	//	case TokenIdentifier::KEYWORD_OBJECT:
		//	{
			//	baseDataTypeNode = new CIdentifierDataType(token, "object", std::vector<CDataType*>());
				//break;
			//}
		case TokenIdentifier::IDENTIFIER:
			{	
				RewindStream();
				baseDataTypeNode = ParseIdentifierDataType();
				break;
			}
		default:
			{		
				m_context->FatalError(CStringHelper::FormatString("Unexpected token while parsing data type '%s' (0x%x).", token.Literal.c_str(), (int)token.Type), token);
				return NULL;
			}
	}
	
	// Read in array type.
	if (LookAheadToken(1).Type == TokenIdentifier::OPEN_BRACKET &&
		LookAheadToken(2).Type == TokenIdentifier::CLOSE_BRACKET)
	{
		ExpectToken(TokenIdentifier::OPEN_BRACKET);
		ExpectToken(TokenIdentifier::CLOSE_BRACKET);

		baseDataTypeNode = baseDataTypeNode->ArrayOf();

		while (LookAheadToken(1).Type == TokenIdentifier::OPEN_BRACKET &&
			   LookAheadToken(2).Type == TokenIdentifier::CLOSE_BRACKET)
		{
			baseDataTypeNode = baseDataTypeNode->ArrayOf();
	
			ExpectToken(TokenIdentifier::OPEN_BRACKET);
			ExpectToken(TokenIdentifier::CLOSE_BRACKET);
		}
	}

	return baseDataTypeNode;
}

// =================================================================
//	Parses an identifier data type.
// =================================================================
CIdentifierDataType* CParser::ParseIdentifierDataType()
{
	std::vector<CDataType*> generic_args;

	CToken& token = NextToken();

	if (token.Type != TokenIdentifier::IDENTIFIER)
	{
		m_context->FatalError(CStringHelper::FormatString("Unexpected token while parsing data type '%s' (0x%x).", token.Literal.c_str(), (int)token.Type), token);
	}

	// Read in generic type.
	if (LookAheadToken().Type == TokenIdentifier::OP_LESS)
	{
		ExpectToken(TokenIdentifier::OP_LESS);

		if (LookAheadToken().Type == TokenIdentifier::OP_GREATER ||
			LookAheadToken().Type == TokenIdentifier::OP_SHR)
		{
			m_context->FatalError("Generic classes must be declared one or more arguments.", token);
		}
		
		while (true)
		{
			generic_args.push_back(ParseDataType());
			
			// We crack shift-right's: deals with cases like;
			// <float, test<int, float>>
			if (LookAheadToken().Type == TokenIdentifier::OP_SHR)
			{
				CToken& token = LookAheadToken() ;
				token.Type = TokenIdentifier::OP_GREATER;
				token.Literal = ">";

				CToken newtoken = token;
				token.Column++;

				m_context->GetTokenList().insert(m_context->GetTokenList().begin() + m_token_offset, newtoken);

				break;
			}
			else if (LookAheadToken().Type == TokenIdentifier::OP_GREATER)
			{
				break;
			}
			else
			{
				ExpectToken(TokenIdentifier::COMMA);
			}
		}

		ExpectToken(TokenIdentifier::OP_GREATER);
	}

	// Create and return identifier type. 
	// FIXME: Memory leak!
	return new CIdentifierDataType(token, token.Literal, generic_args);
}

// =================================================================
//	Parses an argument declaration list: (int x, float y, bool z)
//	Opening and closing parenthesis are not read.
// =================================================================
void CParser::ParseMethodArguments(CClassMemberASTNode* method)
{	
	while (LookAheadToken().Type != TokenIdentifier::CLOSE_PARENT)
	{
		CVariableStatementASTNode* argumentNode = new CVariableStatementASTNode(CurrentScope(), CurrentToken());
		method->Arguments.push_back(argumentNode);

		PushScope(argumentNode);

		// Parse datatype.
		argumentNode->Type = ParseDataType();

		// Parse identifier.
		argumentNode->Identifier = ExpectToken(TokenIdentifier::IDENTIFIER).Literal;
		argumentNode->Token = CurrentToken();
		
		// Read in equal value.
		if (LookAheadToken().Type == TokenIdentifier::OP_ASSIGN)
		{
			ExpectToken(TokenIdentifier::OP_ASSIGN);
			argumentNode->AssignmentExpression = ParseConstExpr(true);
		}

		PopScope();

		if (LookAheadToken().Type == TokenIdentifier::CLOSE_PARENT)
		{
			break;
		}
		else
		{
			ExpectToken(TokenIdentifier::COMMA);
		}
	}
}

// =================================================================
//	Parses statements in a method block.
// =================================================================
CMethodBodyASTNode* CParser::ParseMethodBody()
{
	CMethodBodyASTNode* body = new CMethodBodyASTNode(CurrentScope(), CurrentToken());
	PushScope(body);

	while (true)
	{
		CToken token = LookAheadToken();
		if (token.Type == TokenIdentifier::CLOSE_BRACE)
		{
			break;
		}
		else if (token.Type == TokenIdentifier::EndOfFile)
		{
			m_context->FatalError("Unexpected end-of-file, expecting closing brace.", token);
		}
		ParseMethodBodyStatement();
	}

	PopScope();

	return body;
}

// =================================================================
//	Parses a single statement from a method body.
// =================================================================
CASTNode* CParser::ParseMethodBodyStatement()
{
	CToken token = NextToken();
	switch (token.Type)
	{
		// Empty Statement.
		case TokenIdentifier::SEMICOLON:
			{
				return NULL;
			}

		// Block Statement.
		case TokenIdentifier::OPEN_BRACE:
			{
				RewindStream();
				return ParseBlockStatement();
			}

		// Flow of control statements.
		case TokenIdentifier::KEYWORD_IF:
			{
				return ParseIfStatement();
			}
		case TokenIdentifier::KEYWORD_WHILE:
			{
				return ParseWhileStatement();
			}
		case TokenIdentifier::KEYWORD_BREAK:
			{
				return ParseBreakStatement();
			}
		case TokenIdentifier::KEYWORD_CONTINUE:
			{
				return ParseContinueStatement();
			}
		case TokenIdentifier::KEYWORD_RETURN:
			{
				return ParseReturnStatement();
			}
		case TokenIdentifier::KEYWORD_DO:
			{
				return ParseDoStatement();
			}
		case TokenIdentifier::KEYWORD_SWITCH:
			{
				return ParseSwitchStatement();
			}
		case TokenIdentifier::KEYWORD_FOR:
			{
				return ParseForStatement();
			}
		case TokenIdentifier::KEYWORD_FOREACH:
			{
				return ParseForEachStatement();
			}
		case TokenIdentifier::KEYWORD_TRY:
			{
				return ParseTryStatement();
			}
		case TokenIdentifier::KEYWORD_THROW:
			{
				return ParseThrowStatement();
			}

		// No const or static declarations in method!
		case TokenIdentifier::KEYWORD_STATIC:
			{
				m_context->FatalError("Static declarations are not permitted in a methods body. Please put static declarations in the class body instead.", token);
			}
		case TokenIdentifier::KEYWORD_CONST:
			{
				m_context->FatalError("Constant declarations are not permitted in a methods body. Please put constant declarations in the class body instead.", token);
			}

		// Local variable declaration.				
//		case TokenIdentifier::KEYWORD_OBJECT:
		case TokenIdentifier::KEYWORD_BOOL:
		case TokenIdentifier::KEYWORD_VOID:
		//case TokenIdentifier::KEYWORD_BYTE:
		//case TokenIdentifier::KEYWORD_SHORT:
		case TokenIdentifier::KEYWORD_INT:
		//case TokenIdentifier::KEYWORD_LONG:
		case TokenIdentifier::KEYWORD_FLOAT:
		//case TokenIdentifier::KEYWORD_DOUBLE:
		case TokenIdentifier::KEYWORD_STRING:
		case TokenIdentifier::IDENTIFIER:
			{
				// If what follows is another identifier or a template specification
				// then we are dealign with a variable declaration.
				if (LookAheadToken().Type == TokenIdentifier::OPEN_BRACKET ||
					LookAheadToken().Type == TokenIdentifier::IDENTIFIER ||
					LookAheadToken().Type == TokenIdentifier::OP_LESS)
				{
					bool isVarDeclaration = true;

					// Check this is not a generic class reference, in which case, its an expression.
					if (LookAheadToken().Type == TokenIdentifier::OP_LESS)
					{		
						int final_token_offset = 0;
						if (IsGenericTypeListFollowing(final_token_offset) &&
							LookAheadToken(final_token_offset + 1).Type == TokenIdentifier::PERIOD)
						{
							isVarDeclaration = false;
						}
					}
					else
					{
						int la = 1;

						// Try and read array references.
						while (LookAheadToken(la).Type == TokenIdentifier::OPEN_BRACKET)
						{
							la++;
							if (LookAheadToken(la).Type != TokenIdentifier::CLOSE_BRACKET)
							{
								isVarDeclaration = false;
							}
							else
							{
								la++;
							}
						}

						// Is there an identifier ahead.
						if (isVarDeclaration == true)
						{
							if (LookAheadToken(la).Type != TokenIdentifier::IDENTIFIER)
							{
								isVarDeclaration = false;
							}
						}
					}

					if (isVarDeclaration == true)
					{
						RewindStream();
						CASTNode* node = ParseLocalVariableStatement(true, true, true);
						ExpectToken(TokenIdentifier::SEMICOLON);
						return node;
					}
				}

				// Otherwise its a general expression.
				RewindStream();
				CASTNode* node = ParseExpr();
				ExpectToken(TokenIdentifier::SEMICOLON);
				return node;
			}

		// If its nothing else its an expression.
		default:
			{				
				RewindStream();
				CASTNode* node = ParseExpr();
				ExpectToken(TokenIdentifier::SEMICOLON);
				return node;
			}
	}

	return NULL;
}

// =================================================================
//	Parses a block statement.
// =================================================================
CBlockStatementASTNode* CParser::ParseBlockStatement()
{
	CBlockStatementASTNode* node = new CBlockStatementASTNode(CurrentScope(), CurrentToken());
	PushScope(node);

	ExpectToken(TokenIdentifier::OPEN_BRACE);	

	while (true)
	{
		CToken token = LookAheadToken();
		if (token.Type == TokenIdentifier::CLOSE_BRACE)
		{
			break;
		}
		else if (token.Type == TokenIdentifier::EndOfFile)
		{
			m_context->FatalError("Unexpected end-of-file, expecting closing brace.", token);
		}
		ParseMethodBodyStatement();
	}

	ExpectToken(TokenIdentifier::CLOSE_BRACE);

	PopScope();
	return node;
}

// =================================================================
//	Parses an if statement; if (x > y) { } else { }
// =================================================================
CIfStatementASTNode* CParser::ParseIfStatement()
{
	CIfStatementASTNode* node = new CIfStatementASTNode(CurrentScope(), CurrentToken());
	PushScope(node);

	ExpectToken(TokenIdentifier::OPEN_PARENT);	
	node->ExpressionStatement = ParseExpr();
	ExpectToken(TokenIdentifier::CLOSE_PARENT);

	node->BodyStatement = ParseMethodBodyStatement();

	if (LookAheadToken().Type == TokenIdentifier::KEYWORD_ELSE)
	{
		NextToken();
		node->ElseStatement = ParseMethodBodyStatement();
	}

	PopScope();

	return node;
}

// =================================================================
//	Parses a while statement; while (x > y) { } else 
// =================================================================
CWhileStatementASTNode* CParser::ParseWhileStatement()
{
	CWhileStatementASTNode* node = new CWhileStatementASTNode(CurrentScope(), CurrentToken());
	PushScope(node);

	ExpectToken(TokenIdentifier::OPEN_PARENT);	
	node->ExpressionStatement = ParseExpr();
	ExpectToken(TokenIdentifier::CLOSE_PARENT);

	node->BodyStatement = ParseMethodBodyStatement();

	PopScope();

	return node;
}

// =================================================================
//	Parses a break statement; break;
// =================================================================
CBreakStatementASTNode* CParser::ParseBreakStatement()
{
	CBreakStatementASTNode* node = new CBreakStatementASTNode(CurrentScope(), CurrentToken());
	ExpectToken(TokenIdentifier::SEMICOLON);
	return node;
}

// =================================================================
//	Parses a continue statement; continue;
// =================================================================
CContinueStatementASTNode* CParser::ParseContinueStatement()
{
	CContinueStatementASTNode* node = new CContinueStatementASTNode(CurrentScope(), CurrentToken());
	ExpectToken(TokenIdentifier::SEMICOLON);
	return node;
}

// =================================================================
//	Parses a return statement; return;
// =================================================================
CReturnStatementASTNode* CParser::ParseReturnStatement()
{
	CReturnStatementASTNode* node = new CReturnStatementASTNode(CurrentScope(), CurrentToken());
	PushScope(node);

	if (LookAheadToken().Type != TokenIdentifier::SEMICOLON)
	{
		node->ReturnExpression = ParseExpr();
	}
	
	ExpectToken(TokenIdentifier::SEMICOLON);	

	PopScope();
	return node;
}

// =================================================================
//	Parses a do statement; do { } while (x < y) 
// =================================================================
CDoStatementASTNode* CParser::ParseDoStatement()
{
	CDoStatementASTNode* node = new CDoStatementASTNode(CurrentScope(), CurrentToken());
	PushScope(node);
	
	node->BodyStatement = ParseMethodBodyStatement();

	if (LookAheadToken().Type == TokenIdentifier::KEYWORD_WHILE)
	{
		ExpectToken(TokenIdentifier::KEYWORD_WHILE);	
		ExpectToken(TokenIdentifier::OPEN_PARENT);	
		node->ExpressionStatement = ParseExpr();
		ExpectToken(TokenIdentifier::CLOSE_PARENT);
	}

	PopScope();

	return node;
}

// =================================================================
//	Parses a switch statement.
// =================================================================
CSwitchStatementASTNode* CParser::ParseSwitchStatement()
{
	CSwitchStatementASTNode* node = new CSwitchStatementASTNode(CurrentScope(), CurrentToken());
	PushScope(node);
	
	ExpectToken(TokenIdentifier::OPEN_PARENT);	
	node->ExpressionStatement = ParseExpr(true);
	ExpectToken(TokenIdentifier::CLOSE_PARENT);

	ExpectToken(TokenIdentifier::OPEN_BRACE);

	bool parsing = true;
	bool parsedCase = false;
	bool parsedDefault = false;
	while (parsing)
	{
		CToken& token = LookAheadToken();
		switch (token.Type)
		{
			case TokenIdentifier::KEYWORD_CASE:
				{
					ExpectToken(TokenIdentifier::KEYWORD_CASE);

					parsedCase = true;
					if (parsedDefault == true)
					{
						m_context->FatalError("Encounted case block after default block. Default block must be last.", token);
					}

					CCaseStatementASTNode* caseNode = new CCaseStatementASTNode(CurrentScope(), CurrentToken());
					PushScope(caseNode);
					
					while (true)
					{
						CExpressionASTNode* node = ParseExpr(true);
						if (node != NULL)
						{
							caseNode->Expressions.push_back(node);
						}
						
						if (LookAheadToken().Type == TokenIdentifier::COLON)
						{
							break;
						}
						else
						{
							ExpectToken(TokenIdentifier::COMMA);
						}
					}
					ExpectToken(TokenIdentifier::COLON);

					if (LookAheadToken().Type == TokenIdentifier::KEYWORD_CASE)
					{
						m_context->FatalError("Case blocks cannot fall-through. Use commas to seperate expressions in a single case statement instead.", token);
					}

					caseNode->BodyStatement = ParseMethodBodyStatement();

					PopScope();
					break;
				}
			case TokenIdentifier::KEYWORD_DEFAULT:
				{
					ExpectToken(TokenIdentifier::KEYWORD_DEFAULT);

					if (parsedCase == false)
					{
						m_context->Warning("Encounted default block without any case blocks. Empty switch statement?", token);
					}
					if (parsedDefault == true)
					{
						m_context->FatalError("Encounted duplicate default block in switch statement.", token);
					}
					parsedDefault = true;

					CDefaultStatementASTNode* defaultNode = new CDefaultStatementASTNode(CurrentScope(), CurrentToken());
					PushScope(defaultNode);
					
					ExpectToken(TokenIdentifier::COLON);
					
					if (LookAheadToken().Type == TokenIdentifier::CLOSE_BRACE)
					{
						m_context->FatalError("Default blocks cannot be empty.", token);
					}

					defaultNode->BodyStatement = ParseMethodBodyStatement();

					PopScope();
					break;
				}
			case TokenIdentifier::CLOSE_BRACE:
				{
					parsing = false;
					break;
				}
			case TokenIdentifier::EndOfFile:
				{
					m_context->FatalError("Unexpected end-of-file, expecting closing brace.", token);
					break;
				}
			default:
				{	
					m_context->FatalError(CStringHelper::FormatString("Unexpected token '%s' (0x%x).", token.Literal.c_str(), (int)token.Type), token);
					break;
				}
		}
	}
	ExpectToken(TokenIdentifier::CLOSE_BRACE);

	PopScope();
	return node;
}

// =================================================================
//	Parses a for statement.
// =================================================================
CForStatementASTNode* CParser::ParseForStatement()
{	
	CBlockStatementASTNode* outer_block = new CBlockStatementASTNode(CurrentScope(), CurrentToken());
	PushScope(outer_block);

	CForStatementASTNode* node = new CForStatementASTNode(CurrentScope(), CurrentToken());
	PushScope(node);

	ExpectToken(TokenIdentifier::OPEN_PARENT);	

	if (LookAheadToken().Type != TokenIdentifier::SEMICOLON)
	{
		node->InitialStatement = ParseMethodBodyStatement();
	}
	else
	{
		ExpectToken(TokenIdentifier::SEMICOLON);	
	}

	if (LookAheadToken().Type != TokenIdentifier::SEMICOLON)
	{
		node->ConditionExpression = ParseExpr();
	}
	ExpectToken(TokenIdentifier::SEMICOLON);	

	if (LookAheadToken().Type != TokenIdentifier::CLOSE_PARENT)
	{
		node->IncrementExpression = ParseExpr();
	}

	ExpectToken(TokenIdentifier::CLOSE_PARENT);	

	node->BodyStatement = ParseMethodBodyStatement();

	PopScope();
	PopScope();

	return node;
}

// =================================================================
//	Parses a foreach statement.
// =================================================================
CForEachStatementASTNode* CParser::ParseForEachStatement()
{
	CBlockStatementASTNode* outer_block = new CBlockStatementASTNode(CurrentScope(), CurrentToken());
	PushScope(outer_block);

	CForEachStatementASTNode* node = new CForEachStatementASTNode(CurrentScope(), CurrentToken());
	PushScope(node);
	
	ExpectToken(TokenIdentifier::OPEN_PARENT);		

	CToken& token = NextToken();
	if (/*token.Type == TokenIdentifier::KEYWORD_OBJECT ||*/
		token.Type == TokenIdentifier::KEYWORD_BOOL ||
		token.Type == TokenIdentifier::KEYWORD_VOID ||
		//token.Type == TokenIdentifier::KEYWORD_BYTE ||
		//token.Type == TokenIdentifier::KEYWORD_SHORT ||
		token.Type == TokenIdentifier::KEYWORD_INT ||
		//token.Type == TokenIdentifier::KEYWORD_LONG ||
		token.Type == TokenIdentifier::KEYWORD_FLOAT ||
		//token.Type == TokenIdentifier::KEYWORD_DOUBLE ||
		token.Type == TokenIdentifier::KEYWORD_STRING ||
		token.Type == TokenIdentifier::IDENTIFIER)
	{
		bool isVarDeclaration = false;

		// If what follows is another identifier or a template specification
		// then we are dealign with a variable declaration.
		if (LookAheadToken().Type == TokenIdentifier::OPEN_BRACKET ||
			LookAheadToken().Type == TokenIdentifier::IDENTIFIER ||
			LookAheadToken().Type == TokenIdentifier::OP_LESS)
		{
			isVarDeclaration = true;

			// Check this is not a generic class reference, in which case, its an expression.
			if (LookAheadToken().Type == TokenIdentifier::OP_LESS)
			{		
				int final_token_offset = 0;
				if (IsGenericTypeListFollowing(final_token_offset) &&
					LookAheadToken(final_token_offset + 1).Type == TokenIdentifier::PERIOD)
				{
					isVarDeclaration = false;
				}
			}
			else
			{
				int la = 1;

				// Try and read array references.
				while (LookAheadToken(la).Type == TokenIdentifier::OPEN_BRACKET)
				{
					la++;
					if (LookAheadToken(la).Type != TokenIdentifier::CLOSE_BRACKET)
					{
						isVarDeclaration = false;
					}
					else
					{
						la++;
					}
				}

				// Is there an identifier ahead.
				if (isVarDeclaration == true)
				{
					if (LookAheadToken(la).Type != TokenIdentifier::IDENTIFIER)
					{
						isVarDeclaration = false;
					}
				}
			}

			if (isVarDeclaration == true)
			{
				RewindStream();
				node->VariableStatement = ParseLocalVariableStatement(true, true, true);
			}
		}

		// Otherwise its a general expression.
		if (isVarDeclaration == false)
		{
			RewindStream();
			node->VariableStatement = ParseExpr();
		}
	}
	else
	{
		m_context->FatalError(CStringHelper::FormatString("Unexpected token '%s' (0x%x), expecting expression or variable declaration.", token.Literal.c_str(), (int)token.Type), token);
	}

	ExpectToken(TokenIdentifier::KEYWORD_IN);	
	node->ExpressionStatement = ParseExpr();
	ExpectToken(TokenIdentifier::CLOSE_PARENT);
	
	node->BodyStatement = ParseMethodBodyStatement();

	PopScope();
	PopScope();

	return node;
}

// =================================================================
//	Parses a try statement.
// =================================================================
CTryStatementASTNode* CParser::ParseTryStatement()
{
	CTryStatementASTNode* node = new CTryStatementASTNode(CurrentScope(), CurrentToken());
	PushScope(node);
	
	node->BodyStatement = ParseBlockStatement();
	
	bool parsing = true;
	bool parsedCatch = false;
	bool parsedFinally = false;
	while (parsing)
	{
		CToken& token = LookAheadToken();
		switch (token.Type)
		{
			case TokenIdentifier::KEYWORD_CATCH:
				{
					ExpectToken(TokenIdentifier::KEYWORD_CATCH);

					parsedCatch = true;
					if (parsedFinally == true)
					{
						m_context->FatalError("Encounted catch block after finally block. Finally block must be last.", token);
					}

					CCatchStatementASTNode* catchNode = new CCatchStatementASTNode(CurrentScope(), CurrentToken());
					PushScope(catchNode);
					
					ExpectToken(TokenIdentifier::OPEN_PARENT);
					catchNode->VariableStatement = ParseLocalVariableStatement(false, false, false);
					ExpectToken(TokenIdentifier::CLOSE_PARENT);

					catchNode->BodyStatement = ParseBlockStatement();

					PopScope();
					break;
				}
			default:
				{
					parsing = false;
				}
		}
	}

	PopScope();
	return node;
}

// =================================================================
//	Parses a throw statement.
// =================================================================
CThrowStatementASTNode* CParser::ParseThrowStatement()
{
	CThrowStatementASTNode* node = new CThrowStatementASTNode(CurrentScope(), CurrentToken());
	PushScope(node);
	
	node->Expression = ParseExpr();

	ExpectToken(TokenIdentifier::SEMICOLON);

	PopScope();
	return node;
}

// =================================================================
//	Parses a local variable statement..
// =================================================================
CVariableStatementASTNode* CParser::ParseLocalVariableStatement(bool acceptMultiple, bool acceptAssignment, bool acceptNonConstAssignment)
{
	CToken& start_token = CurrentToken();

	// Read in the data type.
	CDataType* type = ParseDataType();
	
	CVariableStatementASTNode* start_node = NULL;

	while (true)
	{
		CVariableStatementASTNode* node = new CVariableStatementASTNode(CurrentScope(), start_token);
		node->Type = type;

		if (start_node == NULL)
		{
			start_node = node;
		}

		PushScope(node);

		// Read in identifier.
		node->Identifier = ExpectToken(TokenIdentifier::IDENTIFIER).Literal;
		node->Token = CurrentToken();

		// Read in equal value.
		if (acceptAssignment == true)
		{
			if (LookAheadToken().Type == TokenIdentifier::OP_ASSIGN)
			{
				ExpectToken(TokenIdentifier::OP_ASSIGN);

				if (acceptNonConstAssignment == true)
				{
					node->AssignmentExpression = ParseExpr();
				}
				else
				{
					node->AssignmentExpression = ParseConstExpr();
				}
			}
		}

		PopScope();

		if (LookAheadToken().Type == TokenIdentifier::COMMA && acceptMultiple == true)
		{
			ExpectToken(TokenIdentifier::COMMA);
		}
		else
		{
			break;
		}
	}

	return start_node;
}

// =================================================================
//	Parses an expression.
// =================================================================	
CExpressionASTNode* CParser::ParseExpr(bool noSequencePoints)
{
	CASTNode* lvalue = 
		noSequencePoints == true ?
		ParseExprTernary() :
		ParseExprComma();

	if (lvalue != NULL)
	{
		CExpressionASTNode* node = new CExpressionASTNode(CurrentScope(), CurrentToken());
		node->LeftValue = lvalue;
		node->AddChild(lvalue);
		return node;
	}
	else
	{
		m_context->FatalError("Expected an expression.", CurrentToken());
	}

	return NULL;
}

// =================================================================
//	Parses a constant expression.
// =================================================================	
CExpressionASTNode* CParser::ParseConstExpr(bool noSequencePoints)
{
	CASTNode* lvalue = 
		noSequencePoints == true ?
		ParseExprTernary() :
		ParseExprComma();

	if (lvalue != NULL)
	{
		CExpressionASTNode* node = new CExpressionASTNode(CurrentScope(), CurrentToken());
		node->IsConstant = true;
		node->LeftValue = lvalue;
		node->AddChild(lvalue);
		return node;
	}
	else
	{
		m_context->FatalError("Expected an expression.", CurrentToken());
	}

	return NULL;
}

// =================================================================
//	Parses a comma expression.
//		x += 3, z += 5
// =================================================================	
CASTNode* CParser::ParseExprComma()
{
	CASTNode* lvalue = ParseExprTernary();

	while (true)
	{
		// Operator next?
		CToken& op = LookAheadToken();
		if (op.Type != TokenIdentifier::COMMA)
		{
			return lvalue;
		}
		NextToken();

		// Create and parse operator
		CASTNode* rvalue = ParseExprTernary();

		// Create operator node.
		CCommaExpressionASTNode* node = new CCommaExpressionASTNode(NULL, op);
		node->LeftValue = lvalue;
		node->RightValue = rvalue;
		node->AddChild(lvalue);
		node->AddChild(rvalue);

		lvalue = node;
	}

	return lvalue;
}

// =================================================================
//	Parses a ternary expression.
//		expr ? expr : expr
// =================================================================	
CASTNode* CParser::ParseExprTernary()
{	
	CASTNode* lvalue = ParseExprAssignment();

	// Operator next?
	CToken& op = LookAheadToken();
	if (op.Type != TokenIdentifier::OP_TERNARY)
	{
		return lvalue;
	}
	NextToken();

	// Create and parse operator
	CASTNode* rvalue = ParseExprAssignment();
	ExpectToken(TokenIdentifier::COLON);
	CASTNode* rrvalue = ParseExprAssignment();

	// Create operator node.
	CTernaryExpressionASTNode* node = new CTernaryExpressionASTNode(NULL, op);
	node->Expression	= lvalue;
	node->LeftValue		= rvalue;
	node->RightValue	= rrvalue;

	node->AddChild(lvalue);
	node->AddChild(rvalue);
	node->AddChild(rrvalue);

	return node;
}

// =================================================================
//	Parses an assignment expression.
//		<<=
//		>>=
//		~=
//		^=
//		|=
//		&=
//		%=
//		/=
//		*=
//		-=
//		+=
//		=
// =================================================================	
CASTNode* CParser::ParseExprAssignment()
{
	CASTNode* lvalue = ParseExprIsAs();

	// Operator next?
	CToken& op = LookAheadToken();
	if (op.Type != TokenIdentifier::OP_ASSIGN &&
		op.Type != TokenIdentifier::OP_ASSIGN_SUB &&
		op.Type != TokenIdentifier::OP_ASSIGN_ADD &&
		op.Type != TokenIdentifier::OP_ASSIGN_MUL &&
		op.Type != TokenIdentifier::OP_ASSIGN_DIV &&
		op.Type != TokenIdentifier::OP_ASSIGN_MOD &&
		op.Type != TokenIdentifier::OP_ASSIGN_AND &&
		op.Type != TokenIdentifier::OP_ASSIGN_OR  &&
		op.Type != TokenIdentifier::OP_ASSIGN_XOR &&
		op.Type != TokenIdentifier::OP_ASSIGN_SHL &&
		op.Type != TokenIdentifier::OP_ASSIGN_SHR)
	{
		return lvalue;
	}
	NextToken();

	// Create and parse operator
	CASTNode* rvalue = ParseExprIsAs();

	// Create operator node.
	CAssignmentExpressionASTNode* node = new CAssignmentExpressionASTNode(NULL, op);
	node->LeftValue = lvalue;
	node->RightValue = rvalue;
	node->AddChild(lvalue);
	node->AddChild(rvalue);

	return node;
}

// =================================================================
//	Parses an is/as expression.
//		x is y
//		y as x
// =================================================================	
CASTNode* CParser::ParseExprIsAs()
{
	CASTNode* lvalue = ParseExprLogical();

	// Operator next?
	CToken& op = LookAheadToken();
	if (op.Type != TokenIdentifier::KEYWORD_IS &&
		op.Type != TokenIdentifier::KEYWORD_AS)
	{
		return lvalue;
	}
	NextToken();

	// Create and parse operator
	CDataType* rvalue = ParseDataType();

	// Create operator node.
	CTypeExpressionASTNode* node = new CTypeExpressionASTNode(NULL, op);
	node->Type = rvalue;
	node->LeftValue = lvalue;
	node->AddChild(lvalue);

	return node;
}

// =================================================================
//	Parses a logical expression.
//		&&
//		||
// =================================================================	
CASTNode* CParser::ParseExprLogical()
{
	CASTNode* lvalue = ParseExprBitwise();

	while (true)
	{
		// Operator next?
		CToken& op = LookAheadToken();
		if (op.Type != TokenIdentifier::OP_LOGICAL_AND &&
			op.Type != TokenIdentifier::OP_LOGICAL_OR)
		{
			return lvalue;
		}
		NextToken();

		// Create and parse operator
		CASTNode* rvalue = ParseExprBitwise();

		// Create operator node.
		CLogicalExpressionASTNode* node = new CLogicalExpressionASTNode(NULL, op);
		node->LeftValue = lvalue;
		node->RightValue = rvalue;
		node->AddChild(lvalue);
		node->AddChild(rvalue);

		lvalue = node;
	}

	return lvalue;
}

// =================================================================
//	Parses a bitwise expression.
//		&
//		^
//		|
// =================================================================	
CASTNode* CParser::ParseExprBitwise()
{
	CASTNode* lvalue = ParseExprCompare();

	while (true)
	{
		// Operator next?
		CToken& op = LookAheadToken();
		if (op.Type != TokenIdentifier::OP_AND &&
			op.Type != TokenIdentifier::OP_OR &&
			op.Type != TokenIdentifier::OP_XOR &&
			op.Type != TokenIdentifier::OP_SHL &&
			op.Type != TokenIdentifier::OP_SHR)
		{
			return lvalue;
		}
		NextToken();

		// Create and parse operator
		CASTNode* rvalue = ParseExprCompare();

		// Create operator node.
		CBinaryMathExpressionASTNode* node = new CBinaryMathExpressionASTNode(NULL, op);
		node->LeftValue = lvalue;
		node->RightValue = rvalue;
		node->AddChild(lvalue);
		node->AddChild(rvalue);

		lvalue = node;
	}
	
	return lvalue;
}

// =================================================================
//	Parses a comparison expression.
//		==
//		!=
//		<=
//		>=
//		<
//		>
// =================================================================	
CASTNode* CParser::ParseExprCompare()
{
	CASTNode* lvalue = ParseExprAddSub();

	while (true)
	{
		// Operator next?
		CToken& op = LookAheadToken();
		if (op.Type != TokenIdentifier::OP_LESS &&
			op.Type != TokenIdentifier::OP_LESS_EQUAL &&
			op.Type != TokenIdentifier::OP_GREATER &&
			op.Type != TokenIdentifier::OP_GREATER_EQUAL &&
			op.Type != TokenIdentifier::OP_EQUAL &&
			op.Type != TokenIdentifier::OP_NOT_EQUAL)
		{
			return lvalue;
		}
		NextToken();

		// Create and parse operator
		CASTNode* rvalue = ParseExprAddSub();

		// Create operator node.
		CComparisonExpressionASTNode* node = new CComparisonExpressionASTNode(NULL, op);
		node->LeftValue = lvalue;
		node->RightValue = rvalue;
		node->AddChild(lvalue);
		node->AddChild(rvalue);
	
		lvalue = node;
	}

	return lvalue;
}

// =================================================================
//	Parses a add/sub expression.
//		+
//		-
// =================================================================	
CASTNode* CParser::ParseExprAddSub()
{
	CASTNode* lvalue = ParseExprMulDiv();

	while (true)
	{
		// Operator next?
		CToken& op = LookAheadToken();
		if (op.Type != TokenIdentifier::OP_ADD &&
			op.Type != TokenIdentifier::OP_SUB)
		{
			return lvalue;
		}
		NextToken();

		// Create and parse operator
		CASTNode* rvalue = ParseExprMulDiv();

		// Create operator node.
		CBinaryMathExpressionASTNode* node = new CBinaryMathExpressionASTNode(NULL, op);
		node->LeftValue = lvalue;
		node->RightValue = rvalue;
		node->AddChild(lvalue);
		node->AddChild(rvalue);
	
		lvalue = node;
	}

	return lvalue;
}

// =================================================================
//	Parses a mul/div/mod expression.
//		*
//		/
//		%
// =================================================================	
CASTNode* CParser::ParseExprMulDiv()
{
	CASTNode* lvalue = ParseExprPrefix();

	while (true)
	{
		// Operator next?
		CToken& op = LookAheadToken();
		if (op.Type != TokenIdentifier::OP_MUL &&
			op.Type != TokenIdentifier::OP_MOD &&
			op.Type != TokenIdentifier::OP_DIV)
		{
			return lvalue;
		}
		NextToken();

		// Create and parse operator
		CASTNode* rvalue = ParseExprPrefix();

		// Create operator node.
		CBinaryMathExpressionASTNode* node = new CBinaryMathExpressionASTNode(NULL, op);
		node->LeftValue = lvalue;
		node->RightValue = rvalue;
		node->AddChild(lvalue);
		node->AddChild(rvalue);

		lvalue = node;
	}

	return lvalue;
}

// =================================================================
//	Parses a prefix expression.
//		++
//		--
//		+
//		-
//		~
//		!
// =================================================================	
CASTNode* CParser::ParseExprPrefix()
{
	bool hasUnary = false;
	
	// Operator next?
	CToken& op = LookAheadToken();
	if (op.Type == TokenIdentifier::OP_INCREMENT ||
		op.Type == TokenIdentifier::OP_DECREMENT ||
		op.Type == TokenIdentifier::OP_ADD ||
		op.Type == TokenIdentifier::OP_SUB ||
		op.Type == TokenIdentifier::OP_NOT ||
		op.Type == TokenIdentifier::OP_LOGICAL_NOT)
	{
		NextToken();
		hasUnary = true;
	}

	CASTNode* lvalue = ParseExprTypeCast();
	
	// Perform unary operation.
	if (hasUnary == true)
	{
		CPreFixExpressionASTNode* node = new CPreFixExpressionASTNode(NULL, op);
		((CPreFixExpressionASTNode*)node)->LeftValue = lvalue;
		node->AddChild(lvalue);

		lvalue = node;
	}

	return lvalue;
}

// =================================================================
//	Parses a cast expression.
//		<type-cast>
// =================================================================	
CASTNode* CParser::ParseExprTypeCast()
{
	CASTNode* lvalue = NULL;

	while (true)
	{
		// Operator next?
		CToken& op = LookAheadToken();
		if (op.Type != TokenIdentifier::OP_LESS)
		{
			if (lvalue != NULL)
			{
				return lvalue;
			}
			else
			{
				return ParseExprPostfix();
			}
		}
		NextToken();

		// Create and parse operator
		CDataType* dt = ParseDataType();

		ExpectToken(TokenIdentifier::OP_GREATER);

		// Read rvalue.	
		CASTNode* rvalue = ParseExprComma();

		// Create operator node.
		CCastExpressionASTNode* node = new CCastExpressionASTNode(NULL, op, true);
		node->Type = dt;
		node->RightValue = rvalue; 
		node->AddChild(rvalue);

		lvalue = node;
	}

	return lvalue;
}

// =================================================================
//	Parses a postfix expression.
//		++
//		--
//		x.y
//		x.y(arg1, arg2, arg3)
//		[x..y]
//		[..y]
//		[x..]
//		[123]
// =================================================================	
CASTNode* CParser::ParseExprPostfix()
{
	CASTNode* lvalue = ParseExprFactor();

	while (true)
	{
		// Operator next?
		CToken& op = LookAheadToken();
		if (op.Type != TokenIdentifier::OP_INCREMENT &&
			op.Type != TokenIdentifier::OP_DECREMENT &&
			op.Type != TokenIdentifier::PERIOD &&
			op.Type != TokenIdentifier::OPEN_BRACKET)
		{
			return lvalue;
		}
		NextToken();

		// Create operator node.
		CASTNode* node = NULL;

		// PostFix ++ and --
		if (op.Type == TokenIdentifier::OP_DECREMENT ||
			op.Type == TokenIdentifier::OP_INCREMENT)
		{
			node = new CPostFixExpressionASTNode(NULL, op);
			((CPostFixExpressionASTNode*)node)->LeftValue = lvalue;
			node->AddChild(lvalue);
		}

		// Sub-script / slice
		else if (op.Type == TokenIdentifier::OPEN_BRACKET)
		{
			// [:]
			if (LookAheadToken().Type == TokenIdentifier::COLON)
			{
				ExpectToken(TokenIdentifier::COLON);

				// [:]
				if (LookAheadToken().Type == TokenIdentifier::CLOSE_BRACKET)
				{
					node = new CSliceExpressionASTNode(NULL, op);
					((CSliceExpressionASTNode*)node)->LeftValue = lvalue;
					node->AddChild(lvalue);
				}

				// [:(end)]
				else
				{
					CASTNode* slice_end = ParseExprComma();

					node = new CSliceExpressionASTNode(NULL, op);
					((CSliceExpressionASTNode*)node)->LeftValue = lvalue;
					((CSliceExpressionASTNode*)node)->EndExpression = slice_end;
					node->AddChild(slice_end);
					node->AddChild(lvalue);
				}
			}

			// [(start)]
			else
			{
				CASTNode* slice_start = ParseExprComma();
				
				// [(start):] 
				if (LookAheadToken().Type == TokenIdentifier::COLON)
				{					
					ExpectToken(TokenIdentifier::COLON);

					// [(start):] 
					if (LookAheadToken().Type == TokenIdentifier::CLOSE_BRACKET)
					{
						node = new CSliceExpressionASTNode(NULL, op);
						((CSliceExpressionASTNode*)node)->LeftValue = lvalue;
						((CSliceExpressionASTNode*)node)->StartExpression = slice_start;
						node->AddChild(slice_start);
						node->AddChild(lvalue);
					}

					// [(start):(end)]
					else
					{
						CASTNode* slice_end = ParseExprComma();

						node = new CSliceExpressionASTNode(NULL, op);
						((CSliceExpressionASTNode*)node)->LeftValue = lvalue;
						((CSliceExpressionASTNode*)node)->StartExpression = slice_start;
						((CSliceExpressionASTNode*)node)->EndExpression = slice_end;
						node->AddChild(slice_start);
						node->AddChild(slice_end);
						node->AddChild(lvalue);
					}
				}

				// [(start)]
				else
				{
					node = new CIndexExpressionASTNode(NULL, op);
					((CSliceExpressionASTNode*)node)->LeftValue = lvalue;
					((CIndexExpressionASTNode*)node)->IndexExpression = slice_start;
					node->AddChild(slice_start);
					node->AddChild(lvalue);
				}
			}

			ExpectToken(TokenIdentifier::CLOSE_BRACKET);
		}

		// Read in member access.
		else if (op.Type == TokenIdentifier::PERIOD)
		{
			CToken& identToken = ExpectToken(TokenIdentifier::IDENTIFIER);
			
			CASTNode* rvalue = new CIdentifierExpressionASTNode(NULL, identToken);

			// Method access?
			if (LookAheadToken().Type == TokenIdentifier::OPEN_PARENT)
			{				
				node = new CMethodCallExpressionASTNode(NULL, op);
				((CMethodCallExpressionASTNode*)node)->LeftValue = lvalue;
				((CMethodCallExpressionASTNode*)node)->RightValue = rvalue;
				node->AddChild(lvalue);
				node->AddChild(rvalue);

				ExpectToken(TokenIdentifier::OPEN_PARENT);
				
				// Read in arguments.
				while (LookAheadToken().Type != TokenIdentifier::CLOSE_PARENT)
				{
					CASTNode* expr = ParseExprComma();
					((CMethodCallExpressionASTNode*)node)->ArgumentExpressions.push_back(expr);
					node->AddChild(expr);

					if (LookAheadToken().Type == TokenIdentifier::COMMA)
					{
						ExpectToken(TokenIdentifier::COMMA);
					}
					else
					{
						break;
					}
				}

				ExpectToken(TokenIdentifier::CLOSE_PARENT);
			}

			// Field access?
			else
			{
				node = new CFieldAccessExpressionASTNode(NULL, op);
				((CFieldAccessExpressionASTNode*)node)->LeftValue = lvalue;
				((CFieldAccessExpressionASTNode*)node)->RightValue = rvalue;
				node->AddChild(lvalue);
				node->AddChild(rvalue);
			}
		}

		lvalue = node;
	}

	return lvalue;
}

// =================================================================
//	Parses a factor expression:
//		Literals
//		Identifiers
//		Sub Expressions
// =================================================================	
CASTNode* CParser::ParseExprFactor()
{
	CToken& token = NextToken();

	switch (token.Type)
	{
		case TokenIdentifier::KEYWORD_INT:
		case TokenIdentifier::KEYWORD_FLOAT:
		case TokenIdentifier::KEYWORD_STRING:
		case TokenIdentifier::KEYWORD_BOOL:
		case TokenIdentifier::IDENTIFIER:
			{
				// Method call?
				if (LookAheadToken().Type == TokenIdentifier::OPEN_PARENT)
				{
					CASTNode* rvalue = new CIdentifierExpressionASTNode(NULL, token);

					CMethodCallExpressionASTNode* node = new CMethodCallExpressionASTNode(NULL, token);
					if (CurrentClassMemberScope()->IsStatic == true)
					{
						node->LeftValue = new CClassRefExpressionASTNode(NULL, token);
						node->LeftValue->Token.Literal = CurrentClassScope()->Identifier;
					}
					else
					{
						node->LeftValue = new CThisExpressionASTNode(NULL, token);					
					}
					node->RightValue = rvalue;
					node->AddChild(node->LeftValue);
					node->AddChild(rvalue);

					ExpectToken(TokenIdentifier::OPEN_PARENT);
				
					// Read in arguments.
					while (LookAheadToken().Type != TokenIdentifier::CLOSE_PARENT)
					{
						CASTNode* expr = ParseExpr(true);
						((CMethodCallExpressionASTNode*)node)->ArgumentExpressions.push_back(expr);
						node->AddChild(expr);

						if (LookAheadToken().Type == TokenIdentifier::COMMA)
						{
							ExpectToken(TokenIdentifier::COMMA);
						}
						else
						{
							break;
						}
					}

					ExpectToken(TokenIdentifier::CLOSE_PARENT);

					return node;
				}
				else
				{
					CIdentifierExpressionASTNode* node = new CIdentifierExpressionASTNode(NULL, token);

					// Arrrrrrrrgh, we now have to work out if this identifier is a reference to a generic
					// class, and if it is, read in its generic types. This is a total bitch, as its perfectly
					// possible to have ambiguous lookaheads, eg.
					//	
					//		myClass<int>.Derp
					//		myClass<3
					//		myClass<3,5,6
					//		myClass<3,5,6>6
					//
					// To solve this what we basically do is keep reading nested <>'s until we get to the closing
					// < and check if a period follows it. We limit the scan-ahead range to a small number of tokens.
					// This is not ideal at all, but I can't think of a better more full-proof method at the moment.
					//
					int final_token_offset = 0;
					if (IsGenericTypeListFollowing(final_token_offset) &&
						LookAheadToken(final_token_offset + 1).Type == TokenIdentifier::PERIOD)
					{
						ExpectToken(TokenIdentifier::OP_LESS);
						while (true)
						{
							node->GenericTypes.push_back(ParseDataType());

							if (LookAheadToken().Type == TokenIdentifier::COMMA)
							{
								ExpectToken(TokenIdentifier::COMMA);
							}
							else
							{
								break;
							}
						}
						ExpectToken(TokenIdentifier::OP_GREATER);
					}

					return node;
				}
			}		
		case TokenIdentifier::KEYWORD_BASE:
			{
				return new CBaseExpressionASTNode(NULL, token);
			}	
		case TokenIdentifier::KEYWORD_NEW:
			{
				CNewExpressionASTNode* node = new CNewExpressionASTNode(NULL, token);				
				PushScope(node);

				node->DataType = ParseDataType();

				if (LookAheadToken().Type == TokenIdentifier::OPEN_BRACKET)
				{				
					node->IsArray  = true;
					node->DataType = node->DataType->ArrayOf();

					ExpectToken(TokenIdentifier::OPEN_BRACKET);
					node->ArgumentExpressions.push_back(ParseExpr(true));
					ExpectToken(TokenIdentifier::CLOSE_BRACKET);

					while (LookAheadToken().Type == TokenIdentifier::OPEN_BRACKET)
					{
						ExpectToken(TokenIdentifier::OPEN_BRACKET);
						if (LookAheadToken().Type != TokenIdentifier::CLOSE_BRACKET)
						{
							m_context->FatalError("Attempt to initialize array in a multidimensional syntax - only jagged arrays are supported!", CurrentToken());					
							break;
						}
						ExpectToken(TokenIdentifier::CLOSE_BRACKET);

						node->DataType = node->DataType->ArrayOf();
					}
				}
				else
				{
					ExpectToken(TokenIdentifier::OPEN_PARENT);
					while (LookAheadToken().Type != TokenIdentifier::CLOSE_PARENT)
					{
						node->ArgumentExpressions.push_back(ParseExpr(true));

						if (LookAheadToken().Type == TokenIdentifier::CLOSE_PARENT)
						{
							break;
						}
						else
						{
							ExpectToken(TokenIdentifier::COMMA);
						}
					}
					ExpectToken(TokenIdentifier::CLOSE_PARENT);
				}

				PopScope();

				return node;
			}			
		case TokenIdentifier::KEYWORD_THIS:
			{
				return new CThisExpressionASTNode(NULL, token);
			}	
		case TokenIdentifier::STRING_LITERAL:
			{
				return new CLiteralExpressionASTNode(NULL, token, new CStringDataType(token), token.Literal);
			}
		case TokenIdentifier::FLOAT_LITERAL:
			{
				return new CLiteralExpressionASTNode(NULL, token, new CFloatDataType(token), token.Literal);
			}
		case TokenIdentifier::INT_LITERAL:
			{
				return new CLiteralExpressionASTNode(NULL, token, new CIntDataType(token), token.Literal);
			}
		case TokenIdentifier::KEYWORD_TRUE:
			{
				return new CLiteralExpressionASTNode(NULL, token, new CBoolDataType(token), "1");
			}

		case TokenIdentifier::KEYWORD_FALSE:
			{
				return new CLiteralExpressionASTNode(NULL, token, new CBoolDataType(token), "0");
			}

		case TokenIdentifier::KEYWORD_NULL:
			{
				return new CLiteralExpressionASTNode(NULL, token, new CNullDataType(token), token.Literal);
			}
		case TokenIdentifier::OPEN_PARENT:
			{
				CASTNode* node = ParseExprComma();
				ExpectToken(TokenIdentifier::CLOSE_PARENT);
				return node;
			}
		default:
			{
				m_context->FatalError(CStringHelper::FormatString("Unexpected token while parsing expression '%s' (0x%x).", token.Literal.c_str(), (int)token.Type), token);					
				break;
			}
	}

	return NULL;
}

// =================================================================
//	Looks ahead in the token stream to see if we have a 
//	generic list following.
// =================================================================
bool CParser::IsGenericTypeListFollowing(int& final_token_offset)
{
	if (LookAheadToken().Type != TokenIdentifier::OP_LESS)
	{
		return false;
	}

	int  lookAheadIndex = 2;
	int  depth = 1;
	bool isGeneric = false;

	// Keep reading till we get to the end of our potential "generic type list"
	while (EndOfTokens(lookAheadIndex) == false && 
			lookAheadIndex < 64)
	{
		CToken& lat = LookAheadToken(lookAheadIndex);
		if (lat.Type == TokenIdentifier::OP_LESS)
		{
			depth++;
		}
		else if (lat.Type == TokenIdentifier::OP_GREATER)
		{
			depth--;
			if (depth <= 0)
			{
				break;
			}
		}
		else if (lat.Type == TokenIdentifier::OP_SHR)
		{
			depth -= 2;
			if (depth <= 0)
			{
				break;
			}
		}
		else if (lat.Type != TokenIdentifier::IDENTIFIER &&
					lat.Type != TokenIdentifier::KEYWORD_INT &&
					lat.Type != TokenIdentifier::KEYWORD_FLOAT &&
					lat.Type != TokenIdentifier::KEYWORD_STRING &&
					lat.Type != TokenIdentifier::KEYWORD_VOID)
		{
			break;
		}

		lookAheadIndex++;
	}

	if (depth <= 0)
	{
		final_token_offset = lookAheadIndex;
		return true;
	}

	return false;
}
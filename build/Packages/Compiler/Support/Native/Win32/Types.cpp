// -----------------------------------------------------------------------------
// 	types.ls
// 	Copyright (C) 2012-2013 TwinDrills, All Rights Reserved
// -----------------------------------------------------------------------------
//	This package contains the declarations of all default types used by the
//	language. This should never be modified as the compiler relies on the 
//	correct content and ordering of this file.
// -----------------------------------------------------------------------------

#include "Packages/Compiler/Support/Native/Win32/Types.hpp"
#include "Packages/Compiler/Support/Native/Win32/Exceptions.hpp"

// =========================================================================
//	lsStringBuffer.
// =========================================================================
lsStringBuffer::lsStringBuffer() :
	Buffer(NULL),
	Length(0),
	m_ref_count(1)
{
}

// -------------------------------------------------------------------------
//	Reduces reference count for buffer. One step closer to death.
// -------------------------------------------------------------------------
void lsStringBuffer::Release()
{
	--m_ref_count;
		
#ifdef _DEBUG
	if (m_ref_count < 0)
	{
		throw lsInternalGCException();		
	}
#endif

	if (m_ref_count == 0)
	{
		delete[] Buffer;
		delete this;
	}
}

// -------------------------------------------------------------------------
//	Increases reference count for buffer. One step further from death.
// -------------------------------------------------------------------------
void lsStringBuffer::Retain()
{
	m_ref_count++;
}

// -------------------------------------------------------------------------
//	Allocates a new string buffer than can store the given number of
//	characters. Sets the initial reference count to 1.
// -------------------------------------------------------------------------
lsStringBuffer* lsStringBuffer::Allocate(int size)
{
	static lsStringBuffer* g_empty_string = NULL;		
	if (size == 0)
	{
		if (g_empty_string == NULL)
		{
			g_empty_string 				= new lsStringBuffer();
			g_empty_string->Buffer		= new char[1];
			g_empty_string->Length		= 0;
			g_empty_string->m_ref_count	= 1;
			g_empty_string->Buffer[0] 	= '\0';
		}
		g_empty_string->Retain();
		return g_empty_string;
	}		

	lsStringBuffer* buffer = new lsStringBuffer();
	buffer->Buffer		= new char[size + 1];
	buffer->Length		= size;
	buffer->m_ref_count	= 1;
	buffer->Buffer[size] = '\0';
	
	return buffer;
}

// =========================================================================
//	lsString.
// =========================================================================

lsString::~lsString()
{
	m_buffer->Release();
}

lsString::lsString()
{
	m_buffer = lsStringBuffer::Allocate(0);
}

lsString::lsString(const lsString& string)
{
	m_buffer = string.m_buffer;
	m_buffer->Retain();
}

lsString::lsString(lsStringBuffer* buffer)
{
	m_buffer = buffer;
}

lsString::lsString(const char* buffer)
{
	int length = strlen(buffer);
	m_buffer = lsStringBuffer::Allocate(length);
	memcpy(m_buffer->Buffer, buffer, length); 
}

lsString::lsString(const char* buffer, int length)
{
	m_buffer = lsStringBuffer::Allocate(length);
	memcpy(m_buffer->Buffer, buffer, length); 
}

lsString::lsString(int value)
{
	char buffer[128];
	sprintf(buffer, "%i", value);
	
	int length = strlen(buffer);
	m_buffer = lsStringBuffer::Allocate(length);
	memcpy(m_buffer->Buffer, buffer, length);
}

lsString::lsString(float value)
{
	char buffer[128];
	sprintf(buffer, "%f", value);
	
	int length = strlen(buffer);
	m_buffer = lsStringBuffer::Allocate(length);
	memcpy(m_buffer->Buffer, buffer, length);	
}

int lsString::Length() const
{
	return m_buffer->Length;
}

char lsString::GetIndex(int index) const
{
#ifdef _DEBUG
	if (index < 0 || index >= m_buffer->Length)
	{
		throw lsOutOfBoundsException();		
	}
#endif
	return m_buffer->Buffer[index];
}

lsString lsString::GetSlice(int start_pos) const
{
	return GetSlice(start_pos, m_buffer->Length);
}

lsString lsString::GetSlice(int start_pos, int end_pos) const
{
	if (start_pos < 0)
	{
		start_pos += m_buffer->Length;
		if (start_pos < 0)
		{
			start_pos = 0;
		}		
	}
	else if (start_pos > m_buffer->Length)
	{
		start_pos = m_buffer->Length;
	}
	
	if (end_pos < 0)
	{
		end_pos += m_buffer->Length;
	}
	else if (end_pos > m_buffer->Length)
	{
		end_pos = m_buffer->Length;
	}

	if (start_pos >= end_pos)
	{
		return lsString();
	}
	else if (start_pos == 0 && end_pos == m_buffer->Length)
	{
		return lsString(*this);
	}
	else
	{
		return lsString(m_buffer->Buffer + start_pos, end_pos - start_pos);
	}
}

const char* lsString::ToCString() const
{
	return m_buffer->Buffer;
}

float lsString::ToFloat() const
{
	return (float)atof(ToCString());
}

int lsString::ToInt() const
{
	return atoi(ToCString());
}

int lsString::Compare(const lsString& other) const 
{
	int min_size = (other.m_buffer->Length < m_buffer->Length ? 
						other.m_buffer->Length : 
						m_buffer->Length);

	for (int i = 0; i < min_size; i++)
	{
		int diff = (other.m_buffer->Buffer[i] - m_buffer->Buffer[i]);
		if (diff != 0)
		{
			return diff;
		}
	}

	return (other.m_buffer->Length - m_buffer->Length);
}

lsString& lsString::operator =(const lsString& other)
{
	other.m_buffer->Retain();
	m_buffer->Release();
	m_buffer = other.m_buffer;
	return *this;
}

lsString lsString::operator +(const lsString& other) const
{
	if (m_buffer->Length == 0)
	{
		return other;
	}
	if (other.m_buffer->Length == 0)
	{
		return *this;
	}

	lsStringBuffer* newBuffer = lsStringBuffer::Allocate(m_buffer->Length + other.m_buffer->Length);

	memcpy(newBuffer->Buffer, 
			m_buffer->Buffer, 
			m_buffer->Length);

	memcpy(newBuffer->Buffer + m_buffer->Length, 
			other.m_buffer->Buffer, 
			other.m_buffer->Length);

	return lsString(newBuffer);
}

lsString& lsString::operator +=(const lsString& other) 
{
	return operator =(*this + other);
}

char lsString::operator [](int index) const
{
	return GetIndex(index);
}

bool lsString::operator ==(const lsString& other) const
{
	if (m_buffer->Length != other.m_buffer->Length)
	{
		return false;
	}
	return (memcmp(m_buffer->Buffer, other.m_buffer->Buffer, m_buffer->Length) == 0);
}

bool lsString::operator !=(const lsString& other) const
{
	return !(operator ==(other));
}

bool lsString::operator <(const lsString& other) const
{
	return Compare(other) < 0;
}

bool lsString::operator >(const lsString& other) const
{
	return Compare(other) > 0;
}

bool lsString::operator <=(const lsString& other) const
{
	return Compare(other) <= 0;
}

bool lsString::operator >=(const lsString& other) const
{
	return Compare(other) >= 0;
}

// =========================================================================
//	lsGCObject.
// =========================================================================
lsGCObject::~lsGCObject()
{

}	
/*
void lsGCObject::Mark()
{
}

void* lsGCObject::operator new(size_t size)
{

}

void lsGCObject::operator delete(void *p)
{

}
	
int lsGCObject::Collect()
{

}*/

// =========================================================================
//	lsObject.
// =========================================================================
lsString lsObject::ToString()
{
	return "object";
}


#pragma once
#include <string>
#include <cassert>
#include <wtypes.h>
#include <comdef.h>

template<typename T>
struct Dimensions
{
	T width, height;
	bool operator==(const Dimensions<T>& rhs) { return (this->width == rhs.width) && (this->height == rhs.height); }
	bool operator!=(const Dimensions<T>& rhs) { return !((*this) == rhs); }
};

//from Frank Luna's book : Introduction to 3D Game Programming with DirectX 12
class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& fileName, int lineNumber)
		:
		m_ErrorCode(hr),
		m_FunctionName(functionName),
		m_FileName(fileName),
		m_LineNumber(lineNumber)
	{}
	std::wstring ToString() const
	{
		// Get the string description of the error code.
		_com_error err(m_ErrorCode);
		std::wstring msg = err.ErrorMessage();

		return m_FunctionName + L" failed in " + m_FileName + L"; line " + std::to_wstring(m_LineNumber) + L"; error: " + msg;
	}
	HRESULT m_ErrorCode = S_OK;
	std::wstring m_FunctionName;
	std::wstring m_FileName;
	int m_LineNumber = - 1; 
};

inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

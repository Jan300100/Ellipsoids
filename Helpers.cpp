#include "Helpers.h"

void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        _com_error err(hr);

        std::wstring msg = err.ErrorMessage();
        throw msg;
    }
}

#ifndef __HELPER_H__
#define __HELPER_H__

#define WIN32_MEAN_AND_LEAN
#include <Windows.h>

// From DXSampleHelper.h 
// Source: https://github.com/Microsoft/DirectX-Graphics-Samples
inline
void
ThrowIfFailed(
	HRESULT hr
)
{
	if( FAILED( hr ) )
	{
		throw std::exception( );
	}
}

/*
* @brief formats a wide (unicode) string with va args dynamically
* @return WCHAR* - formatted unicode string
*/
inline
WCHAR* 
__wprintf__(WCHAR const* fmt, ...) 
{
    va_list args;
    va_start(args, fmt);

    // Determine the size of the buffer needed
    INT size = _vscwprintf(fmt, args);
    if (size < 0) {
        va_end(args);
        return nullptr;  // Error in format string
    }

    // Allocate the buffer
    size_t buffersize = size + 1;  // +1 for null terminator
    WCHAR* buffer =  (WCHAR*)(malloc(buffersize * sizeof(WCHAR)));
    if ( !buffer ) 
    {
        va_end( args );
        return nullptr;  // memory allocation failed
    }

    INT ret = vswprintf_s( buffer, buffersize, fmt, args );
    va_end( args );

    if (ret < 0) 
    {
        free( buffer );
        return nullptr;  // formatting failed
    }

    return buffer;
}

// extended assert took me way too fucking long to figure out
#define assertf(expression, fmt, ... ) \
    if (!(expression)) { \
        WCHAR* str = __wprintf__(L"%s\nNotes: " fmt, _CRT_WIDE(#expression), ##__VA_ARGS__); \
        if (str) { \
            _wassert(str, _CRT_WIDE(__FILE__), (unsigned)(__LINE__)); \
        } \
        else \
        { \
            assert( _CRT_WIDE(#expression) && " additonal string failed to format"); \
        } \
        free(str); \
    } \

#endif // !__HELPER_H__

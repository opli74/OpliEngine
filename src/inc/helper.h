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
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

inline
void 
assertf(
	bool condition,
	const char* message,
	...
)
{
	do {
		if (!condition)
		{
			va_list args;
			va_start(args, message);
			vfprintf(stderr, message, args);
			va_end(args);
		}
		assert(condition);
	} while (false);
}

//#define assertf(condition, const char* message, ...) do { \
//if (!(condition)) { va_list args; va_start( args, message ); vfprintf( stderr, message, args ); va_end( args ); } \
//assert ((condition)); } while(false)


#endif // !__HELPER_H__

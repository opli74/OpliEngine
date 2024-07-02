#ifndef __WIN_H__
#define __WIN_H__

#define NOMINMAX
#define WIN32_MEAN_AND_LEAN // faster compiling!
#include <Windows.h>
#include <shellapi.h>	// for CommandLineToArgvW

#ifdef CreateWindow
#undef CreateWindow
#endif // CreateWindow

// Windows run time library for MicroSoft::WRL::ComPtr<>
#include <wrl.h>  

#endif // !__WIN_H__

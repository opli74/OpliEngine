#ifndef __WIN_H__
#define __WIN_H__

#define WIN32_MEAN_AND_LEAN // faster compiling!
#include <Windows.h>
#include <shellapi.h>	// for CommandLineToArgvW

#ifdef min
#undef min
#endif // min
#ifdef max 
#undef max
#endif // max
#ifdef CreateWindow
#undef CreateWindow
#endif // CreateWindow

// Windows run time library for MicroSoft::WRL::ComPtr<>
#include <wrl.h>  

#endif // !__WIN_H__

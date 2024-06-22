#ifndef __LIB_H__
#define __LIB_H__

#include "inc.h"

/*
	```` |/|``````````````````|\| ````
	---- |\| GLOBAL VARIABLES |/| ----
	.... |/|..................|\| ....
*/

//number of swap chain back buffers
extern CONST UINT8 g_num_frames;
//wrap adaptor
extern BOOL g_use_warp;
//for create window
extern CONST WCHAR* g_class_name;

//screen width
extern UINT32 g_client_width;

//screen height
extern UINT32 g_client_height;

// check if all d3d12 devices initalised
extern BOOL g_is_initalised;

// window handle
extern HWND g_hwnd;
// window dimensions & offer stuff
extern RECT g_win_rect;

// D3D12 objects
extern Microsoft::WRL::ComPtr<ID3D12Device2>	         g_device;		//actual d3d12 device
extern Microsoft::WRL::ComPtr<ID3D12CommandQueue>        g_command_queue;	// command queue??
extern Microsoft::WRL::ComPtr<IDXGISwapChain4>	         g_swap_chain;		// presentes rendered frame to window
extern Microsoft::WRL::ComPtr<ID3D12Resource>	         g_back_buffers[];	// tracks backbuffer resource states
extern Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_command_list;	// tracks GPU commands in a !!SINGLE THREAD!!
extern Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    g_command_allocator[];	// backing memory for GPU commands for command list
extern Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	     g_rtv_descriptor_heap;	// where rtv textures are stored ( rtv used to clear back buffer of render and render geometry )
extern UINT											     g_rtv_descriptor_size;	// size of descriptor depends on manufacturor ( AMD, Intel, NVidea ) so needs to be offset in correct context
extern UINT												 g_current_back_buffer_index; // stores index of current back buffer ?simple?

// GPU synchronisation objects
extern Microsoft::WRL::ComPtr<ID3D12Fence> g_fence;	// stores fence object
extern UINT64							   g_fence_value;	// stores next fence value signal
extern UINT64							   g_frame_fence_value[];	// used to keep track of fence values that where used to signal command queue
extern HANDLE							   g_fence_event;	// used to check if fence has not reached specified value where cpu will then stall

// v-sync shit
extern BOOL g_vsync;	// wether present should wait for next vertical refresh
extern BOOL g_tearing_support;		//g-sync shit
extern BOOL g_fullscreen;	// full-screen track!!

/*
	```` |/|``````````````````````|\| ````
	---- |\| FUNCTION DECLRATIONS |/| ----
	.... |/|......................|\| ....
*/

/*
@brief Grabs command line args such as -height to modify global varibles
@return void
*/
void
ParseCommandLineArguments(

);

/*
@brief Enable debugging for DX12
@return void
*/
void
EnableDebugLayer(

);

/*
@brief Registers a window for creation
@param[in] _In_ HINSTANCE
@param[in] _In_ CONST WCHAR*
@return void
*/
void
RegisterWindowClass(
	_In_ HINSTANCE		hinst,
	_In_ CONST WCHAR*   window_name
);

/*
@brief Creates the window after it has been registered
@param[in] _In_ CONST WCHAR*
@param[in] _In_ HINSTANCE
@param[in] _In_ CONST WCHAR*
@param[in] _In_ UINT32
@param[in] _In_ UINT32
@return HWND
*/
HWND
CreateWindow(
	_In_ CONST WCHAR* window_name,
	_In_ HINSTANCE    hinst,
	_In_ CONST WCHAR* window_title,
	_In_ UINT32		  width,
	_In_ UINT32		  height
);

/*
@brief Retrieves most comaptible GPU adapter
@param[in] _In_ BOOL
@return ComPtr< IDXGIAdapter4 >
*/
Microsoft::WRL::ComPtr<IDXGIAdapter4>
GetAdapter(
	_In_ BOOL warp
);

/*
@brief Creates a D3D12Device2 device
@param _In_ ComPtr< IDXGIAdapter4 >
@return ComPtr< ID3D12Device2 >
*/
Microsoft::WRL::ComPtr< ID3D12Device2 >
CreateDevice(
	_In_ Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter
);

/*
@brief Creates Command Queue
@param[in] ComPtr< ID3D12Device2 > 
@param[in] D3D12_COMMAND_LIST_TYPE type
@return ComPtr< ID3D12CommandQueue > - the d3d12 command queue
*/
Microsoft::WRL::ComPtr< ID3D12CommandQueue >
CreateCommandQueue(
	_In_ Microsoft::WRL::ComPtr< ID3D12Device2 > device,
	_In_ D3D12_COMMAND_LIST_TYPE type
);

/*
@brief check if tearing support is availble on computer
@return BOOL - FALSE if tearing supported, FALSE if not supported
*/
BOOL
CheckTearingSupport(

);

#endif // !__LIB_H__

#ifndef __LIB_H__
#define __LIB_H__

#include "inc.h"

#if defined(DX12TL_EXPORTS)
#    define DX12TL_DLL __declspec(dllexport)
#    define DX12TL_EXTERN
#elif defined(DX12TL_IMPORTS)
#    define DX12TL_DLL __declspec(dllimport)
#    define DX12TL_EXTERN extern
#else
#    define DX12TL_DLL
#    define DX12TL_EXTERN
#endif

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
extern BOOL g_is_initialised;

// window handle
extern HWND g_hwnd;
// window dimensions & offer stuff
extern RECT g_win_rect;

// D3D12 objects
extern Microsoft::WRL::ComPtr<ID3D12Device2>	         g_device;		//actual d3d12 device
extern Microsoft::WRL::ComPtr<ID3D12CommandQueue>        g_commandqueue;	// command queue??
extern Microsoft::WRL::ComPtr<IDXGISwapChain4>	         g_swapchain;		// presentes rendered frame to window
extern Microsoft::WRL::ComPtr<ID3D12Resource>	         g_backbuffers[];	// tracks backbuffer resource states
extern Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_commandlist;	// tracks GPU commands in a !!SINGLE THREAD!!
extern Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    g_command_allocator[];	// backing memory for GPU commands for command list
extern Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	     g_rtv_descriptor_heap;	// where rtv textures are stored ( rtv used to clear back buffer of render and render geometry )
extern UINT											     g_rtv_descriptor_size;	// size of descriptor depends on manufacturor ( AMD, Intel, NVidea ) so needs to be offset in correct context
extern UINT												 g_current_back_buffer_index; // stores index of current back buffer ?simple?

// GPU synchronisation objects
extern Microsoft::WRL::ComPtr<ID3D12Fence> g_fence;	// stores fence object
extern UINT64							   g_fence_value;	// stores next fence value signal
extern UINT64							   g_frame_fence_values[];	// used to keep track of fence values that where used to signal command queue
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
@param[in] _In_ CONST WCHAR* - window name
@param[in] _In_ HINSTANCE	 - program instance
@param[in] _In_ CONST WCHAR* - window class name
@param[in] _In_ UINT32		 - client width
@param[in] _In_ UINT32		 - client height
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
@param[in] _In_ ComPtr<IDXGIAdapter4>
@return ComPtr< ID3D12Device2 >
*/
Microsoft::WRL::ComPtr< ID3D12Device2 >
CreateDevice(
	_In_ Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter
);

/*
@brief Creates Command Queue
@param[in] _In_ ComPtr<ID3D12Device2> 
@param[in] _In_ D3D12_COMMAND_LIST_TYPE type
@return ComPtr<ID3D12CommandQueue> - the d3d12 command queue
*/
Microsoft::WRL::ComPtr< ID3D12CommandQueue >
CreateCommandQueue(
	_In_ Microsoft::WRL::ComPtr< ID3D12Device2 > device,
	_In_ D3D12_COMMAND_LIST_TYPE type
);

/*
@brief check if tearing support is availble on computer
@return BOOL - TRUE if tearing supported, FALSE if not supported
*/
BOOL
CheckTearingSupport(

);

/*
@brief creates the swap buffer device
@param[in] _In_ HWND
@param[in] _In_ ComPtr<ID3D12CommandQueue>
@param[in] _In_ UINT32 - client width
@param[in] _In_ UINT32 - client height
@param[in] _In_ UINT32 - buffer count
@return ComPtr<IDXGISwapChain4> - the dx12 swap chain
*/
Microsoft::WRL::ComPtr<IDXGISwapChain4>
CreateSwapChain(
	_In_ HWND hwnd,
	_In_ Microsoft::WRL::ComPtr< ID3D12CommandQueue > command_queue,
	_In_ UINT32 width,
	_In_ UINT32 height,
	_In_ UINT32 buffer_count
);

/*
@brief CreateS descriptor heap, used to store resources for swap chain
@param[in] _In_ ComPtr<ID3D12Device2>
@param[in] _In_ D3D12_DESCRIPTOR_HEAP_TYPE
@param[in] _In_ UINT32 - number of descriptors
@return ComPtr<ID3D12DescriptorHeap> - the dx12 descriptor heap
*/
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
CreateDescriptorHeap(
	_In_ Microsoft::WRL::ComPtr<ID3D12Device2> device,
	_In_ D3D12_DESCRIPTOR_HEAP_TYPE type,
	_In_ UINT32 num_descriptors
);

/*
@brief Updates RenderTargetView (RTV), the rtv is the resource that gets final colour computed by pixel shader
@param[in] _In_ ComPtr<ID3D12Device2> 
@param[in] _In_ ComPtr<IDXGISwapChain4>
@param[in] _In_ ComPtr<ID3D12DescriptorHeap>
@return VOID
*/
VOID
UpdateRenderTargetView(
	_In_ Microsoft::WRL::ComPtr<ID3D12Device2> device,
	_In_ Microsoft::WRL::ComPtr<IDXGISwapChain4> swapchain,
	_In_ Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorheap
);

/*
@brief Creates command allocator which is used to issue rendering commands to GPU
@param[in] ComPtr<ID3D12Device2>
@param[in] D3D12_COMMAND_LIST_TYPE
@return ComPtr<ID3D12CommandAllocator>
*/
Microsoft::WRL::ComPtr<ID3D12CommandAllocator>
CreateCommandAllocator(
	_In_ Microsoft::WRL::ComPtr<ID3D12Device2> device,
	_In_ D3D12_COMMAND_LIST_TYPE type
);

/*
@brief Creates command list, used for recording commands that the GPU executes
@param[in] ComPtr<ID3D12Device2>
@param[in] ComPtr<ID3D12CommandAllocator>
@param[in] D3D12_COMMAND_LIST_TYPE
@return ComPtr<ID3D12GraphicsCommandList>
*/
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>
CreateCommandList(
	_In_ Microsoft::WRL::ComPtr<ID3D12Device2> device,
	_In_ Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator,
	_In_ D3D12_COMMAND_LIST_TYPE type
);

/*
@brief Creates fence object for synchronosiation between GPU and CPU
@param[in] ComPtr<ID3D12Device2> device> 
@return ComPtr<ID3D12Fence>
*/
Microsoft::WRL::ComPtr<ID3D12Fence>
CreateFence(
	_In_ Microsoft::WRL::ComPtr<ID3D12Device2> device
);

/*
@brief Create event handle to block the CPU thread until fence has been signaled 
@return HANDLE - handle to event
*/
HANDLE
CreateEventHandle(

);

/*
@brief used to signal the fence from the GPU
@param[in] ComPtr<ID3D12CommandQueue>
@param[in] ComPtr<ID3D12Fence>
@param[in] UINT64 - the fence value
@return UINT64 - new fence value
*/
UINT64
Signal(
	_In_ Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandqueue,
	_In_ Microsoft::WRL::ComPtr<ID3D12Fence> fence,
	_In_ UINT64& fencevalue
);

/*
@brief stalls the CPU to wait for GPU queue to finish
@param[in] ComPtr<ID3D12Fence> 
@param[in] UINT64 - fence value
@param[in] HANDLE - fence event handle
@param[opt] std::chrono::milliseconds - time to wait
@return void
*/
VOID
WaitForFenceValue(
	_In_ Microsoft::WRL::ComPtr<ID3D12Fence> fence,
	_In_ UINT64 fence_value,
	_In_ HANDLE fence_event,
	_In_opt_ std::chrono::milliseconds duration = std::chrono::milliseconds::max()
);

/*
@brief ensures any commands previously executed on GPU are cleared before CPU thread continues
@param[in] ComPtr<ID3D12CommandQueue>
@param[in] ComPtr<ID3D12Fence>
@param[in] UINT64 - fence value
@param[in] HANDLE - handle to fence event
@return VOID
*/
VOID
Flush(
	_In_ Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandqueue,
	_In_ Microsoft::WRL::ComPtr<ID3D12Fence> fence,
	_In_ UINT64& fence_value,
	_In_ HANDLE fence_event
);

/*
@brief just calculates frame rate for now
@return VOID
*/
VOID
Update(

);

/*
@brief renders to window, basically clears back buffer then "presents" rendered frame
@return VOID
*/
VOID
Render(

);

/*
@brief sets fullscreen (not exclusive but full screen borderless window)
@return VOID
*/
VOID
SetFullScreen(
	BOOL fullscreen
);


/*
@brief functionality to be able to change window to new size
@return VOID
*/
VOID
Resize(
	UINT32 width,
	UINT32 height
);



#endif // !__LIB_H__

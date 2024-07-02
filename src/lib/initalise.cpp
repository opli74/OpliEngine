#include <lib.h>
#include <errhandlingapi.h> // GetLastError()

// Number of swap chain back buffers
CONST UINT8 g_num_frames = 3;
// Wrap adapter
BOOL g_use_warp = FALSE;

CONST WCHAR* g_class_name = L"OpliGameEngine";

UINT32 g_client_width = 1280;
UINT32 g_client_height = 720;

// Check if all D3D12 devices initialized
BOOL g_is_initialised = FALSE;

// Window handle
HWND g_hwnd = NULL;
// Window dimensions & other stuff
RECT g_win_rect = { };

// D3D12 objects
Microsoft::WRL::ComPtr<ID3D12Device2> g_device;
Microsoft::WRL::ComPtr<ID3D12CommandQueue> g_commandqueue;
Microsoft::WRL::ComPtr<IDXGISwapChain4> g_swapchain;
Microsoft::WRL::ComPtr<ID3D12Resource> g_backbuffers[g_num_frames];
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_commandlist;
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_command_allocator[g_num_frames];
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_rtv_descriptor_heap;
UINT g_rtv_descriptor_size = 0;
UINT g_current_back_buffer_index = 0;

// GPU synchronization objects
Microsoft::WRL::ComPtr<ID3D12Fence> g_fence;
UINT64 g_fence_value = 0;
UINT64 g_frame_fence_values[g_num_frames] = { };
HANDLE g_fence_event = nullptr;

// V-sync settings
BOOL g_vsync = TRUE;
BOOL g_tearing_support = FALSE;
BOOL g_fullscreen = FALSE;

 LRESULT
CALLBACK
WndProc(
	HWND   hwnd,
	UINT   message,
	WPARAM wparam,
	LPARAM lparam
)
{
	switch (message)
	{
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			bool alt = (::GetAsyncKeyState( VK_MENU ) & 0x8000 ) != 0;

			switch( wparam )
			{
				case 'V':
					g_vsync = !g_vsync;
					break;
				case VK_ESCAPE:
					::PostQuitMessage( 0 );
					break;
				case VK_RETURN:
					if( alt )
					{
				case VK_F11:
						SetFullScreen( !g_fullscreen );
					}
					break;
			}
			break;
		}
		case WM_SYSCHAR:
			break;
		case WM_PAINT:
			Update( );
			Render( );
			break;
		case WM_SIZE:
		{
			RECT client_rect = {};
			::GetClientRect( g_hwnd, &client_rect );

			INT w = client_rect.right - client_rect.left;
			INT h = client_rect.bottom - client_rect.top;

			Resize( w, h );
			break;
		}
		case WM_DESTROY:
			::PostQuitMessage( 0 );
			break;
		default:
			return ::DefWindowProc(hwnd, message, wparam, lparam);
	}

	return 0;
}

void
ParseCommandLineArguments(

)
//allows global variables to be defined by commandline arugments 
{
	INT argc;	// arguments count
	WCHAR** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc); // get arguments

	//iterate and check for arugments
	for (size_t i = 0; i < argc; ++i)
	{
		if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"-width") == 0)
			g_client_width = ::wcstol(argv[++i], nullptr, 10);
		if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"-height") == 0)
			g_client_height = ::wcstol(argv[++i], nullptr, 10);
		if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
			g_use_warp = TRUE;
	};
	//free memory allocated by CommandLineToArgvW
	::LocalFree(argv);
}

void 
EnableDebugLayer(

)
//debug layer must always be enable before DX12 devices so all errors can be caught
{
#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug> debug_interface;
	ThrowIfFailed(D3D12GetDebugInterface(
		IID_PPV_ARGS(&debug_interface) //retrieves interface ptr automatically!!
	));
	debug_interface->EnableDebugLayer();
#endif // _DEBUG
}

void
RegisterWindowClass(
	_In_ HINSTANCE    hinst,
	_In_ CONST WCHAR* window_name
)
{
	//typedef struct tagWNDCLASSEXW {
	//	UINT        cbSize;
	//	UINT        style;
	//	WNDPROC     lpfnWndProc;
	//	INT         cbClsExtra;
	//	INT         cbWndExtra;
	//	HINSTANCE   hInstance;
	//	HICON       hIcon;
	//	HCURSOR     hCursor;
	//	HBRUSH      hbrBackground;
	//	LPCWSTR     lpszMenuName;
	//	LPCWSTR     lpszClassName;
	//	HICON       hIconSm;
	//} WNDCLASSEXW, * PWNDCLASSEXW;

	WNDCLASSEXW window = { 0 };
	
	window.cbSize		 = sizeof(WNDCLASSEXW);	// size of struct
	window.style		 = CS_HREDRAW | CS_VREDRAW; // specifies entire window to be redrawn if moved or size change
	window.lpfnWndProc	 = &WndProc;	// handles messages to window
	window.cbClsExtra    = 0;	// number of extra bytes to allocate after structure
	window.cbWndExtra    = 0;	// number of extra bytes to allocate after window instance
	window.hInstance     = hinst;	// handle to window
	//window.hIcon       = ::LoadIcon(hinst, NULL); // icon name has to be valid otherwise RegisterClassExW cries
	window.hCursor       = ::LoadCursor(NULL, IDC_ARROW);	// load default arrow cursor
	window.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	// background brush colour
	window.lpszMenuName  = NULL;	// specifies resource name, null means no deault menu
	window.lpszClassName = window_name;	// name of window
	// window.hIconSm    = ::LoadIcon( hinst, NULL ); // icon name has to be valid otherwise RegisterClassExW cries 

	static HRESULT hr = ::RegisterClassExW(&window);
#ifdef _DEBUG
	assertf( hr > 0, L"RegisterClassExW failed[ %d ]\n", (INT)GetLastError());
#endif // _DEBUG
};

HWND
CreateWindow(
	_In_ CONST WCHAR* window_name,
	_In_ HINSTANCE	  hinst,
	_In_ CONST WCHAR* window_title,
	_In_ UINT32		  width,
	_In_ UINT32		  height
)
{
	// gets height and width of display monitor
	INT screen_width  = ::GetSystemMetrics(SM_CXSCREEN);
	INT screen_height = ::GetSystemMetrics(SM_CYSCREEN);

	RECT window = {
		0,
		0,
		(LONG)width,
		(LONG)height,
	};
	// tells windows window can be minimised, maximised has thic-thrame
	::AdjustWindowRect(&window, WS_OVERLAPPEDWINDOW, FALSE);

	INT window_width  = window.right - window.left;
	INT window_height = window.bottom - window.top;

	INT windowx = std::max<INT>(0, (screen_width - window_width) / 2);
	INT windowy = std::max<INT>(0, (screen_height - window_height) / 2);

	//HWND WINAPI CreateWindowExW(
	HWND hwnd = ::CreateWindowExW(
		NULL,				  //	_In_ DWORD dwExStyle,	
		window_title,		  //	_In_opt_ LPCWSTR lpClassName,
		window_name,		  //	_In_opt_ LPCWSTR lpWindowName,
		WS_OVERLAPPEDWINDOW,  //	_In_ DWORD dwStyle,
		windowx,			  //	_In_ INT X,
		windowy,			  //	_In_ INT Y,
		window_width,		  //	_In_ INT nWidth,
		window_height,		  //	_In_ INT nHeight,
		NULL,				  //	_In_opt_ HWND hWndParent,
		NULL,				  //	_In_opt_ HMENU hMenu,
		GetModuleHandle(NULL),//	_In_opt_ HINSTANCE hInstance,
		NULL				  //	_In_opt_ LPVOID lpParam
	);
	// );

#ifdef _DEBUG
	assertf( hwnd, "CreateWindowExW failed [ Error: %d ]", GetLastError());
#endif // _DEBUG

	return hwnd;
}

Microsoft::WRL::ComPtr<IDXGIAdapter4>
GetAdapter(
	_In_ BOOL warp
)
//qeury computer for compatible GPU adapters
{
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory;	// creates adapters!!
	UINT								  create_factory_flags = 0;
#ifdef _DEBUG
	create_factory_flags = DXGI_CREATE_FACTORY_DEBUG;	// catch errors querying for adapters
#endif // _DEBUG
	ThrowIfFailed(CreateDXGIFactory2(create_factory_flags, IID_PPV_ARGS(&dxgi_factory)));

	Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgi_adapter1;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgi_adapter4;

	if (warp)
	{
		ThrowIfFailed(dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(&dxgi_adapter1)));	// creates warp adapter
		ThrowIfFailed(dxgi_adapter1.As(&dxgi_adapter4));
	}
	// use DXGIFactory to query hardware adapters
	else
	{
		size_t max_dedicated_video_memory = 0;
		// iterate and check which adapter can create a d3d12 device and favor one with largest video memory
		// EnumAdapters1( ) for iterating available GPU adapters, returns DXGI_ERROR_NOT_FOUND when index is >= count of adapters
		for (UINT i = 0; dxgi_factory->EnumAdapters1(i, &dxgi_adapter1) != DXGI_ERROR_NOT_FOUND; i++)
		{
			DXGI_ADAPTER_DESC1 dxgi_adapter_desc1;
			dxgi_adapter1->GetDesc1(&dxgi_adapter_desc1);

			if ((dxgi_adapter_desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 && // check if adapter is not a warp adapter with
				// DXGI_ADAPTER_FLAG_SOFTWARE since we only want hardware adapters
				D3D12CreateDevice(
					dxgi_adapter1.Get(),
					D3D_FEATURE_LEVEL_11_0,
					__uuidof(ID3D12Device),
					nullptr
				) == S_OK && // if D3D12CreateDevice returns S_OK then this is a compatible adapter
				dxgi_adapter_desc1.DedicatedVideoMemory > max_dedicated_video_memory)	// find GPU adapter with greatest memory
			{
				max_dedicated_video_memory = dxgi_adapter_desc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgi_adapter1.As(&dxgi_adapter4));
			}
		}
	}
	return dxgi_adapter4;
}

Microsoft::WRL::ComPtr< ID3D12Device2 >
CreateDevice(
	_In_ Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter
)
{
	Microsoft::WRL::ComPtr<ID3D12Device2> d3d12device2; // used to create resources ( textures and buffers )

	//HRESULT WINAPI D3D12CreateDevice(
	ThrowIfFailed(D3D12CreateDevice(
		adapter.Get(),			    // _In_opt_  IUnknown * pAdapter,   ptr to adapter ( use nullptr to pass default adapter by first device from IDXGI::EnumAdapters()  )
		D3D_FEATURE_LEVEL_11_0,     // D3D_FEATURE_LEVEL MinimumFeatureLevel, minimum feature level required for device creation
		IID_PPV_ARGS(&d3d12device2) // _In_ REFIID riid, GUID for the device interface this macro combines this paramter and void** ppDevice
									// _Out_opt_ void** ppDevice, not needed when IID_PPV_ARGS( ) macro is used
	)
		//);
	);

#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> info_queue;
	if (d3d12device2.As(&info_queue) == S_OK)
	{
		// for DX12 debugging!
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
	}

	// fitler out any debug messages that would just be annoying

	// Suppress whole categories of messages
	//D3D12_MESSAGE_CATEGORY categories[] = {};

	// Suppress messages based on their severity level
	D3D12_MESSAGE_SEVERITY severities[] =
	{
		//suppress information messages
		D3D12_MESSAGE_SEVERITY_INFO
	};

	// Suppress invididual messages based on their ids
	D3D12_MESSAGE_ID deny_ids[] =
	{
		D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE, //suppress message of unoptimised clear colour
		D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,	// supress when a frame is captured by a debugger
		D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE // supress when a frame is captured by a debugger
	};

	D3D12_INFO_QUEUE_FILTER filter = {};
	//filter.DenyList.NumCategories = _countof( categories );
	//filter.DenyList.pCategoryList = categories;
	filter.DenyList.NumSeverities   = _countof( severities );
	filter.DenyList.pSeverityList   = severities;
	filter.DenyList.NumIDs			= _countof( deny_ids );
	filter.DenyList.pIDList			= deny_ids;

	ThrowIfFailed( info_queue->PushStorageFilter( &filter ) );
#endif // _DEBUG

	return d3d12device2;
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> 
CreateCommandQueue(
	_In_ Microsoft::WRL::ComPtr<ID3D12Device2> device,
	_In_ D3D12_COMMAND_LIST_TYPE type
)
{
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;

	D3D12_COMMAND_QUEUE_DESC desc = {};

//typedef struct D3D12_COMMAND_QUEUE_DESC {
	desc.Type     = type; 								// D3D12_COMMAND_LIST_TYPE Type;  type can be all a general type (execute,draw,compute,copy) , a type to only execute and computer or only copy
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;// INT Priority;  priority can either be normal, high, or global real time
	desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE; 		// D3D12_COMMAND_QUEUE_FLAGS Flags; Additional flags for command queue
	desc.NodeMask = 0;									// UINT NodeMask; For single GPU, 0, multibyte set correct bit in mask
//} D3D12_COMMAND_QUEUE_DESC;

	ThrowIfFailed( device->CreateCommandQueue( &desc, IID_PPV_ARGS( &command_queue ) ) );

	return command_queue;
}

BOOL 
CheckTearingSupport(

)
{
	BOOL tearing_allowed = FALSE;
	Microsoft::WRL::ComPtr<IDXGIFactory4> factory4;
	if (CreateDXGIFactory1(IID_PPV_ARGS(&factory4)) >= S_OK)
	{
		Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
		if( factory4.As( &factory5 ) >= S_OK )
		{			
//HRESULT CheckFeatureSupport(
			if( factory5->CheckFeatureSupport (       
				DXGI_FEATURE_PRESENT_ALLOW_TEARING,// DXGI_FEATURE Feature, which feature to query, right now only one
				&tearing_allowed,				   // [in, out] void* pFeatureSupportData, buffer to be filled with description of feature support
				sizeof( tearing_allowed )	       // UINT FeatureSupportDataSize, size of buffer
//);
				< S_OK ) )
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

Microsoft::WRL::ComPtr<IDXGISwapChain4> 
CreateSwapChain(
	_In_ HWND hwnd,
	_In_ Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue,
	_In_ UINT32 width,
	_In_ UINT32 height,
	_In_ UINT32 buffer_count
)
{
	Microsoft::WRL::ComPtr< IDXGISwapChain4 > swapchain4;
	Microsoft::WRL::ComPtr< IDXGIFactory4 >   factory4;
	UINT create_factory_flags = 0;

#ifdef _DEBUG
	create_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG

	ThrowIfFailed( CreateDXGIFactory2( create_factory_flags, IID_PPV_ARGS( &factory4 ) ) );

	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};

//typedef struct _DXGI_SWAP_CHAIN_DESC1 {
	swap_chain_desc.Width = width;								  // UINT Width; resolution width
	swap_chain_desc.Height = height;							  // UINT Height; resolution height
	swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;		  // DXGI_FORMAT Format; defines display format
	swap_chain_desc.Stereo = FALSE;								  // BOOL Stereo; specifies if stereo to be used, flip-model swap chain always requires stereo!
	swap_chain_desc.SampleDesc = { 1, 0 };						  // DXGI_SAMPLE_DESC SampleDesc; for multi-sample, not support on flip-model so must be {1, 0}
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;// DXGI_USAGE BufferUsage; surface usage and CPU access to backbuffer
	swap_chain_desc.BufferCount = buffer_count;					  // UINT BufferCount; amount of buffers in swap chain
	swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;				  // DXGI_SCALING Scaling; defines the resize behaviour if backbuffer is different to target
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	  // DXGI_SWAP_EFFECT SwapEffect; swap chain presentation model to use, bitbit is outdated!!
	swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;	  // DXGI_ALPHA_MODE AlphaMode; defines transparency behaviour for backbuffers
	// !!!ALWAYS CHECK FOR TEARING SUPPORT!!!
	swap_chain_desc.Flags = CheckTearingSupport( ) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0; // UINT Flags; just for tearing support
//} DXGI_SWAP_CHAIN_DESC1;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapchain1;

	ThrowIfFailed(factory4
//HRESULT CreateSwapChainForHwnd(
		->CreateSwapChainForHwnd(
			command_queue.Get(), //	[in] IUnknown * pDevice, ptr to command queue
			hwnd,			     //	[in] HWND hWnd, handle to window
			&swap_chain_desc,	 //	[in] const DXGI_SWAP_CHAIN_DESC1 * pDesc, pointer to swap chain desc struc
			nullptr,			 //	[in, optional] const DXGI_SWAP_CHAIN_FULLSCREEN_DESC * pFullscreenDesc, NULL for windowed swap chain, or swap chain fullscreen struct for fullscreen
			nullptr,			 //	[in, optional] IDXGIOutput * pRestrictToOutput, idk??
			&swapchain1			 //	[out] IDXGISwapChain1 * *ppSwapChain, just out ptr!
	));
//);
	//Disable alt+enter for fullscreen!!
	ThrowIfFailed( factory4->MakeWindowAssociation( hwnd, DXGI_MWA_NO_ALT_ENTER ) );
	ThrowIfFailed( swapchain1.As( &swapchain4 ) );

	return swapchain4;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> 
CreateDescriptorHeap(
	_In_ Microsoft::WRL::ComPtr<ID3D12Device2> device,
	_In_ D3D12_DESCRIPTOR_HEAP_TYPE type,
	_In_ UINT32 num_descriptors
)
{
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};

	//typedef struct D3D12_DESCRIPTOR_HEAP_DESC {
	desc.Type = type; 					   // D3D12_DESCRIPTOR_HEAP_TYPE  Type; types of descriptors such as for render, sampling, depth-stencil etc.
	desc.NumDescriptors = num_descriptors; // UINT NumDescriptors; number of descriptors
										   // D3D12_DESCRIPTOR_HEAP_FLAGS Flags; 
										   // UINT NodeMask;
	//} D3D12_DESCRIPTOR_HEAP_DESC;

	ThrowIfFailed( device->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &descriptor_heap ) ) );

	return descriptor_heap;
}

VOID 
UpdateRenderTargetView(
	_In_ Microsoft::WRL::ComPtr<ID3D12Device2> device, 
	_In_ Microsoft::WRL::ComPtr<IDXGISwapChain4> swapchain, 
	_In_ Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorheap
)
{
	//descriptor size varies so needs to be queried
	UINT rtv_descriptor_size = device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
	//to iterate descriptors start is needed, structure allocates pointer to start of descriptor heap
	//CD3DX12 is an extended version of D3DX12 in d3dx12.h
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle( descriptorheap->GetCPUDescriptorHandleForHeapStart( ) );

	for( INT i = 0; i < g_num_frames; i++ )
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> backbuffer;
		//query swap chain back buffer to create RTV
		ThrowIfFailed( swapchain->GetBuffer( i, IID_PPV_ARGS( &backbuffer ) ) );
		//actually creates RTV 
		device->CreateRenderTargetView( backbuffer.Get( ), nullptr, rtv_handle );
		g_backbuffers[ i ] = backbuffer;
		//increment offset to next descriptor
		rtv_handle.Offset( rtv_descriptor_size );
	}
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> 
CreateCommandAllocator(
	_In_ Microsoft::WRL::ComPtr<ID3D12Device2> device, 
	_In_ D3D12_COMMAND_LIST_TYPE type
)
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
	ThrowIfFailed( device->CreateCommandAllocator( type, IID_PPV_ARGS( &command_allocator ) ) );
	return command_allocator;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> 
CreateCommandList(
	_In_ Microsoft::WRL::ComPtr<ID3D12Device2> device,
	_In_ Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator, 
	_In_ D3D12_COMMAND_LIST_TYPE type
)
{
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandlist;

	//HRESULT CreateCommandList(
	ThrowIfFailed( device->CreateCommandList( 
		NULL,					    // [in] UINT nodeMask, single GPU use zero, if multiple set each bit to identify node
		type,						// [in] D3D12_COMMAND_LIST_TYPE type, specify which command list to create, copying, computing, executing etc
		command_allocator.Get( ),   // [in] ID3D12CommandAllocator * pCommandAllocator, ptr to allocator to create command list
		nullptr,					// [in, optional] ID3D12PipelineState * pInitialState, pipeline for command list when null sets a dummy pipeline and can be initalised after
		IID_PPV_ARGS( &commandlist )// REFIID riid, to get GUID of commandlist and to get created command list
		//	[out]          void** ppCommandList
	) );
	//);

	ThrowIfFailed( commandlist->Close( ) );
	return commandlist;
}

Microsoft::WRL::ComPtr<ID3D12Fence> 
CreateFence(
	Microsoft::WRL::ComPtr<ID3D12Device2> device
)
{
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;

	//HRESULT CreateFence(
	ThrowIfFailed( device->CreateFence(
		NULL,				   // UINT64 InitialValue, most times initial value for fence should be 0
		D3D12_FENCE_FLAG_NONE, // D3D12_FENCE_FLAGS Flags, for fence flags such as shared fence, fence for multiple GPUs etc
		IID_PPV_ARGS( &fence ) // REFIID riid, for fence object GUID and out pointer!?
		//	[out] void** ppFence
	) );
	//);	

	return fence;
}

HANDLE 
CreateEventHandle(

)
{
//HANDLE WINAPI CreateEvent(
	HANDLE fence_event = ::CreateEvent( 
		NULL, // _In_opt_ LPSECURITY_ATTRIBUTES lpEventAttributes, ptr to security attributes, null means cannot be inhereited by children
		FALSE,// _In_ BOOL bManualReset, false will automically reset event objects after thread is done waiting
		FALSE,// _In_ BOOL bInitialState, state of event signal
		NULL  // _In_opt_ LPCTSTR lpName null means no event name
	);
//);
	assertf( fence_event, "Failed to create fence event [ Error: %d ]", GetLastError( ) );

	return fence_event;
}

UINT64 
Signal(
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandqueue, 
	Microsoft::WRL::ComPtr<ID3D12Fence> fence, 
	UINT64& fence_value
)
{
	UINT64 new_fence_value = fence_value++;

	//HRESULT Signal(
	ThrowIfFailed( commandqueue->Signal( 
		fence.Get( ),	// ID3D12Fence * pFence, pointer to fence object
		new_fence_value // UINT64 Value	, value to signal fence with when GPU finished with any commands
	) );
	//);

	return new_fence_value;
}

VOID 
WaitForFenceValue(
	Microsoft::WRL::ComPtr<ID3D12Fence> fence, 
	UINT64 fence_value, HANDLE fence_event, 
	std::chrono::milliseconds duration
)
{
	if( fence->GetCompletedValue( ) < fence_value )
	{
		ThrowIfFailed( fence->SetEventOnCompletion( fence_value, fence_event ) );
		::WaitForSingleObject( fence_event, (DWORD)duration.count( ) );	
	}
}

VOID 
Flush(
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandqueue, 
	Microsoft::WRL::ComPtr<ID3D12Fence> fence, 
	UINT64& fence_value, 
	HANDLE fence_event
)
{
	UINT64 new_fence_value = Signal( commandqueue, fence, fence_value );
	WaitForFenceValue( fence, new_fence_value, fence_event );
}

VOID 
Update(

)
{
	static UINT64 frame_count = 0;
	static DOUBLE elapsed_time = 0.0;
	static std::chrono::high_resolution_clock clock;
	static std::chrono::steady_clock::time_point t0 = clock.now( );

	frame_count++;
	std::chrono::steady_clock::time_point t1 = clock.now( );
	auto deltatime = t1 - t0;
	elapsed_time += deltatime.count() * 1e-9;
	if (elapsed_time > 1.0)
	{
		WCHAR buffer[500];
		auto fps = frame_count / elapsed_time;
		swprintf_s<500>( buffer, L"FPS: %f\n", fps );
		OutputDebugString( buffer );
		frame_count = 0;
		elapsed_time = 0.0;
	}
}

VOID 
Render(

)
{
	auto command_allocator = g_command_allocator[ g_current_back_buffer_index ];
	auto backbuffer	       = g_backbuffers[ g_current_back_buffer_index ];

	command_allocator->Reset( );
	g_commandlist->Reset( command_allocator.Get( ), nullptr );

	{ // clear render target
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backbuffer.Get( ),
			D3D12_RESOURCE_STATE_PRESENT, 
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);

		g_commandlist->ResourceBarrier( 1, &barrier );
		FLOAT clearcolor[ ] = { .4f, .6f, .9f, 1.f };

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv( 
			g_rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart( ) ,
			g_current_back_buffer_index,
			g_rtv_descriptor_size
		);

		g_commandlist->ClearRenderTargetView( rtv, clearcolor, NULL, nullptr );
	}

	{ // Present
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backbuffer.Get( ),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);
		g_commandlist->ResourceBarrier( 1, &barrier );

		ThrowIfFailed( g_commandlist->Close( ) );

		ID3D12CommandList* CONST commandlists[ ] = {
			g_commandlist.Get()
		};

		g_commandqueue->ExecuteCommandLists( _countof( commandlists ), commandlists );

		UINT sync_interval = g_vsync ? 1 : 0;
		UINT present_flags = g_tearing_support && !g_vsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
		ThrowIfFailed(g_swapchain->Present(sync_interval, present_flags));

		g_frame_fence_values[ g_current_back_buffer_index ] = Signal( g_commandqueue, g_fence, g_fence_value );
		g_current_back_buffer_index = g_swapchain->GetCurrentBackBufferIndex( );
		WaitForFenceValue( g_fence, g_frame_fence_values[ g_current_back_buffer_index ], g_fence_event );
	}
}

VOID 
Resize(
	UINT32 width, 
	UINT32 height 
)
{
	if (g_client_width != width || g_client_height != height)
	{
		g_client_width = std::max( 1u, width );
		g_client_height = std::max( 1u, height );

		Flush( g_commandqueue, g_fence, g_fence_value, g_fence_event );
		for (INT i = 0; i < g_num_frames; i++)
		{
			g_backbuffers[ i ].Reset( );
			g_frame_fence_values[ i ] = g_frame_fence_values[ g_current_back_buffer_index ];
		}
		DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
		ThrowIfFailed( g_swapchain->GetDesc( &swapchaindesc ) );
		ThrowIfFailed( g_swapchain->ResizeBuffers( g_num_frames, g_client_width, g_client_height, swapchaindesc.BufferDesc.Format, swapchaindesc.Flags ) );

		g_current_back_buffer_index = g_swapchain->GetCurrentBackBufferIndex( );
		UpdateRenderTargetView( g_device, g_swapchain, g_rtv_descriptor_heap );
	}
}

VOID 
SetFullScreen(
	BOOL fullscreen
)
{
	if (g_fullscreen != fullscreen)
	{
		g_fullscreen = fullscreen;
		if (g_fullscreen)
		{
			::GetWindowRect( g_hwnd, &g_win_rect );
			UINT windowstyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
			::SetWindowLongW( g_hwnd, GWL_STYLE, windowstyle );
			HMONITOR hmonitor = ::MonitorFromWindow( g_hwnd, MONITOR_DEFAULTTONEAREST );
			MONITORINFOEX monitor_info = {};
			monitor_info.cbSize = sizeof( MONITORINFOEX );
			::GetMonitorInfo( hmonitor, &monitor_info );
			::SetWindowPos( 
				g_hwnd, 
				HWND_TOP,
				monitor_info.rcMonitor.left,
				monitor_info.rcMonitor.top,
				monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
				monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE 
			);
			::ShowWindow( g_hwnd, SW_MAXIMIZE );
		}
		else
		{
			::SetWindowLong( g_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW );
			::SetWindowPos(
				g_hwnd,
				HWND_NOTOPMOST,
				g_win_rect.left,
				g_win_rect.top,
				g_win_rect.right - g_win_rect.left,
				g_win_rect.bottom - g_win_rect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE
			);
			::ShowWindow( g_hwnd, SW_NORMAL );
		}
	}
}

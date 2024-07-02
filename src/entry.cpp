#include <inc.h>
#include <lib.h>

#ifndef _DEBUG 
#define NDEBUG
#endif

#define ALLOC_CONSOLE AllocConsole(); freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
#define FREE_CONSOLE FreeConsole( );

INT
WINAPI
wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	PWSTR lpCmdLine,
	INT nCmdShow
)
{
	ALLOC_CONSOLE;

	// Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
	// Using this awareness context allows the client area of the window 
	// to achieve 100% scaling while still allowing non-client window content to 
	// be rendered in a DPI sensitive fashion.
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	ParseCommandLineArguments( );
	EnableDebugLayer( );

	g_tearing_support = CheckTearingSupport( );

	RegisterWindowClass( hInstance, g_class_name );
	g_hwnd = CreateWindow( L"GameEngine", hInstance, g_class_name, g_client_width, g_client_height );

	::GetWindowRect( g_hwnd, &g_win_rect );

	//DX12 objects creation!!!!!
	Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter4 = GetAdapter( g_use_warp );
	g_device									   = CreateDevice( adapter4 );
	g_commandqueue								   = CreateCommandQueue( g_device, D3D12_COMMAND_LIST_TYPE_DIRECT );
	g_swapchain									   = CreateSwapChain( g_hwnd, g_commandqueue, g_client_width, g_client_height, g_num_frames );
	g_current_back_buffer_index					   = g_swapchain->GetCurrentBackBufferIndex( );
	g_rtv_descriptor_heap						   = CreateDescriptorHeap( g_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3 );
	g_rtv_descriptor_size						   = g_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

	UpdateRenderTargetView( g_device, g_swapchain, g_rtv_descriptor_heap );

	for ( INT i = 0; i < g_num_frames; i++ )
	{
		g_command_allocator[ i ] = CreateCommandAllocator( g_device, D3D12_COMMAND_LIST_TYPE_DIRECT );
	}
	
	g_commandlist = CreateCommandList( g_device, g_command_allocator[ g_current_back_buffer_index ], D3D12_COMMAND_LIST_TYPE_DIRECT );
	g_fence       = CreateFence( g_device );
	g_fence_event = CreateEventHandle( );

	g_is_initialised = TRUE;

	::ShowWindow( g_hwnd, SW_SHOW );

	MSG message = { };
	while ( message.message != WM_QUIT )
	{ 
		if ( ::PeekMessage( &message, NULL, NULL, NULL, PM_REMOVE ) )
		{
			::TranslateMessage( &message );
			::DispatchMessage( &message );
		}
	};

	Flush( g_commandqueue, g_fence, g_fence_value, g_fence_event );
	::CloseHandle( g_fence_event );

	FREE_CONSOLE;
	return 0;
}
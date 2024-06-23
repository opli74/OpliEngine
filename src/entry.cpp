#include <inc.h>
#include <lib.h>

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
	ParseCommandLineArguments( );
	EnableDebugLayer( );

	g_tearing_support = CheckTearingSupport( );

	RegisterWindowClass( hInstance, g_class_name );
	g_hwnd = CreateWindow( L"GameEngine", hInstance, g_class_name, g_client_width, g_client_height );

	::GetWindowRect( g_hwnd, &g_win_rect );

	//DX12 objects creation!!!!!
	Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter4 = GetAdapter( g_use_warp );
	g_device									   = CreateDevice( adapter4 );
	g_command_queue								   = CreateCommandQueue( g_device, D3D12_COMMAND_LIST_TYPE_DIRECT );
	g_swap_chain								   = CreateSwapChain( g_hwnd, g_command_queue, g_client_width, g_client_height, g_num_frames );

	//g_is_initalised = TRUE; // LINK ERROR WHY!?!?!

	::ShowWindow( g_hwnd, SW_SHOW );

	MSG message = { };
	while ( message.message != WM_QUIT )
	{ 
		if ( ::PeekMessage( &message, NULL, 0, 0, PM_REMOVE ) )
		{
			::TranslateMessage( &message );
			::DispatchMessage( &message );
		}
	};

	FREE_CONSOLE;
	return 0;
}
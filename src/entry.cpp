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
	EnableDebugLayer();
	RegisterWindowClass(hInstance, g_class_name);
	g_hwnd = CreateWindow(L"GameEngine", hInstance, g_class_name, g_client_width, g_client_height);
	::ShowWindow(g_hwnd, nCmdShow);

	while (1)
	{
		if (::GetAsyncKeyState(VK_END) & 1) break;

		MSG message;
		BOOL message_result = ::GetMessage(&message, g_hwnd, 0, 0);
		if (message_result != 0)
			::DispatchMessage(&message);
		else if (message_result == 0)
			break;
	};

	FREE_CONSOLE;
	return 0;
}
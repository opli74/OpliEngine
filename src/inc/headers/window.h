#include "lib.h"

/*
@brief Where messages are dispatched to
@param[in] HWND
@param[in] UINT
@param[in] WPARAM
@param[in] LPARAM
@return LRESULT
*/
LRESULT
CALLBACK
WndProc(
	_In_ HWND   hwnd,
	_In_ UINT   message,
	_In_ WPARAM wparam,
	_In_ LPARAM lparam
);


class DX12TL_DLL Window
{
private:
	//window name
	WCHAR* m_name;
	WCHAR* m_class_name;

	HWND m_hwindow;
	RECT m_window_rect;

	//for variable refresh display
	BOOL m_allow_tearing;

	UINT32 m_width;
	UINT32 m_height;
	BOOL m_fullscreen;

	BOOL m_vsync;

public:
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

};
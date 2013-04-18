/*
	PoE Pulse
	By Aaron Opfer, Copyright 2013
	me@aaronopfer.com
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShellAPI.h>
#include <Windowsx.h>

static const char* className = "pulseInvisibleClass";
HMODULE theDll;
FARPROC hookProc;
HHOOK hook;
UINT windowMessage;
HMENU hMenu;
NOTIFYICONDATA nid = {};

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND hPoE = 0, hOldPoE = 0;

	switch (message) {
		// our shell icon
		case 0xBEEF:
			switch (LOWORD(lParam)) {
				case WM_LBUTTONUP:
				case WM_RBUTTONUP:
				case WM_CONTEXTMENU:
					TrackPopupMenu(hMenu,
				                    0,
				                    GET_X_LPARAM(wParam),
				                    GET_Y_LPARAM(wParam),
					               0,
					               hwnd,
					               NULL);
					break;

			}
			break;
		case WM_COMMAND:
			Shell_NotifyIcon(NIM_DELETE,&nid);
			DestroyWindow(hwnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_HOTKEY:
			hPoE = FindWindow("Direct3DWindowClass","Path of Exile");
			if (hPoE != NULL || hPoE != hOldPoE) {
				// Create the hook
				hook = SetWindowsHookEx(WH_CALLWNDPROC,
								    (HOOKPROC)hookProc,
								    theDll,
								    GetWindowThreadProcessId(hPoE,NULL));

				if (!hook) {
					MessageBox(NULL,"Failed to create window hook!", NULL,NULL);
					return 1;
				}
			}
			

			SendMessage(hPoE,windowMessage,0,0);

			hOldPoE = hPoE;
			break;
	}
	return DefWindowProc(hwnd,message,wParam,lParam);
}


int CALLBACK WinMain (HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine,
                      int nCmdShow) {
	MSG msg = {0};
	HWND mainWindow;
	NOTIFYICONDATA nid = {};
	WNDCLASSEX wx = {};
	MENUITEMINFO mi = {};

	mi.cbSize = sizeof(MENUITEMINFO);
	mi.fMask = MIIM_STRING;
	mi.wID = 1;
	mi.dwTypeData = "E&xit";
	 
	hMenu = CreatePopupMenu();
	if (InsertMenuItem(hMenu,0,TRUE,&mi) == 0) {
		MessageBox(NULL,"Failed to create menu!", NULL, NULL);
		return 1;
	}
	
	wx.cbSize = sizeof(WNDCLASSEX);
	wx.lpfnWndProc = WndProc;
	wx.hInstance = hInstance;
	wx.lpszClassName = className;

	

	if (RegisterClassEx(&wx) == NULL) {
		MessageBox(NULL,"Failed to register class!", NULL, NULL);
		return 1;
	}

	mainWindow = CreateWindowEx(0,
	                            className,
	                            "poepulse",
	                            0,0,0,0,0, 
	                            HWND_MESSAGE,
	                            NULL,
	                            hInstance,
	                            NULL);

	if (mainWindow == 0) {
		MessageBox(NULL,"Failed to create window!", NULL, NULL);
		return 1;
	}

	windowMessage = RegisterWindowMessage("poepulse_PulseCursor");
	if (!windowMessage) {
		MessageBox(NULL,"Failed to register custom message!", NULL, NULL);
		return 1;
	}

	theDll = LoadLibrary("poepmod.dll");
	if (!theDll) {
		MessageBox(NULL,"Failed to load poepmod.dll!", NULL, NULL);
		return 1;
	}

	hookProc = GetProcAddress(theDll,"_PoeWndProc@12");
	if (!hookProc) {
		MessageBox(NULL,"Failed to find window hook procedure!", NULL,NULL);
		return 1;
	}


	if (RegisterHotKey(mainWindow,
	                   1,
	                   MOD_ALT,
	                   VK_SPACE) == FALSE) {
		MessageBox(NULL,"Failed to Create Hotkey!", NULL, NULL);
		return 1;
	}

	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = mainWindow;
	nid.uID = 222;
	nid.uVersion = NOTIFYICON_VERSION_4;
	nid.uFlags = NIF_MESSAGE|NIF_TIP|NIF_ICON;
	nid.uCallbackMessage = 0xBEEF;
	strcpy(nid.szTip, "PoE Pulse");
	nid.hIcon = LoadIcon(NULL,IDI_APPLICATION);
	

	if (Shell_NotifyIcon(NIM_ADD,&nid) == FALSE || Shell_NotifyIcon(NIM_SETVERSION,&nid) == FALSE) {
		MessageBox(NULL,"Failed to Create notification bar icon!!", NULL, NULL);
		return 1;
	}
	
	while (GetMessage(&msg,NULL,0,0) != 0) {
		DispatchMessage(&msg);
	}
	ExitProcess(0);
	return 0;
}
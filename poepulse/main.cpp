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
// makes us use COMCTL32 6.0 for pretty visuals. See:
// http://msdn.microsoft.com/en-us/library/windows/desktop/bb773175(v=vs.85).aspx#using_manifests
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShellAPI.h>
#include <Windowsx.h>
#include "resource.h"

static const char* className = "pulseInvisibleClass";
HMODULE theDll;
FARPROC hookProc;
HHOOK hook;
UINT windowMessage;
HMENU hMenu;
HINSTANCE hInstance;
NOTIFYICONDATA nid = {};
UINT fsModifiers, vKey;
HKEY regKey;
DWORD sizeofUINT = sizeof(UINT);
HWND mainWindow;

INT_PTR CALLBACK HotkeyDialogProc (HWND hwnd, UINT uMsg,
                                   WPARAM wParam, LPARAM lParam) {
	WORD keyControl = 0;
	UINT nFsModifiers=0,nVKey=0;
	switch (uMsg) {
		case WM_INITDIALOG:
			if (fsModifiers & MOD_ALT) {
				SendMessage(GetDlgItem(hwnd,IDC_ALT),BM_SETCHECK,BST_CHECKED,0);
			}
			if (fsModifiers & MOD_CONTROL) {
				SendMessage(GetDlgItem(hwnd,IDC_CTRL),BM_SETCHECK,BST_CHECKED,0);
			}
			if (fsModifiers & MOD_SHIFT) {
				SendMessage(GetDlgItem(hwnd,IDC_SHIFT),BM_SETCHECK,BST_CHECKED,0);
			}
			switch (vKey) {
				case VK_SPACE:
					keyControl = IDC_SPACE;
					break;
				case 'F':
					keyControl = IDC_F;
					break;
				case VK_OEM_3:
					keyControl = IDC_TILDE;
					break;
			}
			if (keyControl != 0) {
				SendMessage(GetDlgItem(hwnd,keyControl),BM_SETCHECK,BST_CHECKED,0);
			}
			return TRUE;
		case WM_COMMAND:
			switch (wParam) {
				case IDOK:
					nFsModifiers |= (MOD_SHIFT   * Button_GetCheck(GetDlgItem(hwnd,IDC_SHIFT)));
					nFsModifiers |= (MOD_ALT     * Button_GetCheck(GetDlgItem(hwnd,IDC_ALT)));
					nFsModifiers |= (MOD_CONTROL * Button_GetCheck(GetDlgItem(hwnd,IDC_CTRL)));

					if (Button_GetCheck(GetDlgItem(hwnd,IDC_F))) {
						nVKey = 'F';
					} else if( (Button_GetCheck(GetDlgItem(hwnd,IDC_SPACE)))) {
						nVKey = VK_SPACE;
					} else if( (Button_GetCheck(GetDlgItem(hwnd,IDC_TILDE)))) {
						nVKey = VK_OEM_3;
					} else {
						nVKey = vKey;
					}

					if (nVKey != vKey || nFsModifiers != fsModifiers) {
						// remove old hotkey
						UnregisterHotKey(mainWindow,1);
						vKey = nVKey;
						fsModifiers = nFsModifiers;

						// place new hotkey
						if (RegisterHotKey(mainWindow,
									    1,
									    fsModifiers,
									    vKey) == FALSE) {
							MessageBox(NULL,"Failed to Create Hotkey!", NULL, NULL);
							return 1;
						}

						// save registry
						if (RegSetValueEx(regKey,
										"fsModifiers",
										NULL,
										REG_DWORD,
										(BYTE*)&fsModifiers,
										sizeofUINT) != ERROR_SUCCESS ||
							RegSetValueEx(regKey,
										"vKey",
										NULL,
										REG_DWORD,
										(BYTE*)&vKey,
										sizeofUINT) != ERROR_SUCCESS) {
							MessageBox(NULL,"Failed to write registry!", NULL,NULL);
							return 1;
						}
					}
				case IDCANCEL:
					EndDialog(hwnd,0);
					return TRUE;
			}
			break;
	}
	return FALSE;
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND hPoE = 0, hOldPoE = 0;
	switch (message) {
		// our shell icon
		case 0xBEEF:
			switch (LOWORD(lParam)) {
				case WM_LBUTTONDBLCLK:
				case WM_CONTEXTMENU:
					SetForegroundWindow(hwnd);
					TrackPopupMenu(hMenu,
				                    0,
				                    GET_X_LPARAM(wParam),
				                    GET_Y_LPARAM(wParam),
					               0,
					               hwnd,
					               NULL);
					PostMessage(hwnd,WM_NULL,0,0);
					break;

			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case 1:
					Shell_NotifyIcon(NIM_DELETE,&nid);
					DestroyWindow(hwnd);
					break;
				case 2:
					DialogBox(hInstance,MAKEINTRESOURCE(IDD_DIALOG1),NULL,HotkeyDialogProc);
					break;
			}
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

	NOTIFYICONDATA nid = {};
	WNDCLASSEX wx = {};
	MENUITEMINFO mi = {};


	::hInstance = hInstance;

	mi.cbSize = sizeof(MENUITEMINFO);
	mi.fMask = MIIM_STRING|MIIM_ID;
	mi.wID = 1;
	mi.dwTypeData = "E&xit";
	 
	hMenu = CreatePopupMenu();
	if (InsertMenuItem(hMenu,0,TRUE,&mi) == 0) {
		MessageBox(NULL,"Failed to create menu!", NULL, NULL);
		return 1;
	}

	mi.wID = 2;
	mi.dwTypeData = "&Hotkey...";
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

	// query/create the registry for our hotkey information
	if (RegCreateKeyEx(HKEY_CURRENT_USER,
	                   "Software\\Aaron Opfer\\PoE Pulse",
	                   NULL,
	                   NULL,
	                   NULL,
	                   KEY_ALL_ACCESS,
	                   NULL,
	                   &regKey,
	                   NULL) != ERROR_SUCCESS) {
		MessageBox(NULL,"Failed to read/write registry!", NULL,NULL);
		return 1;
	}   

	// retrieve the hotkey values from the registry
	if (RegGetValue(regKey,
	                NULL,
	                "fsModifiers",
	                RRF_RT_REG_DWORD,
	                NULL,
	                &fsModifiers,
	                &sizeofUINT) != ERROR_SUCCESS ||
	    RegGetValue(regKey,
	                NULL,
	                "vKey",
	                RRF_RT_REG_DWORD,
	                NULL,
	                &vKey,
	                &sizeofUINT) != ERROR_SUCCESS) {
		// Couldn't read these registry keys, better set some defaults
		// instead
		fsModifiers = MOD_ALT;
		vKey = VK_SPACE;
		
		if (RegSetValueEx(regKey,
		                  "fsModifiers",
		                  NULL,
		                  REG_DWORD,
		                  (BYTE*)&fsModifiers,
		                  sizeofUINT) != ERROR_SUCCESS ||
		    RegSetValueEx(regKey,
		                  "vKey",
		                  NULL,
		                  REG_DWORD,
		                  (BYTE*)&vKey,
		                  sizeofUINT) != ERROR_SUCCESS) {
			MessageBox(NULL,"Failed to write registry!", NULL,NULL);
			return 1;
		}
	}

	if (RegisterHotKey(mainWindow,
	                   1,
	                   fsModifiers,
	                   vKey) == FALSE) {
		MessageBox(NULL,"Failed to Create Hotkey!", NULL, NULL);
		return 1;
	}

	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = mainWindow;
	nid.uID = 222;
	nid.uVersion = NOTIFYICON_VERSION_4;
	nid.uFlags = NIF_MESSAGE|NIF_TIP|NIF_ICON|NIF_SHOWTIP;
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
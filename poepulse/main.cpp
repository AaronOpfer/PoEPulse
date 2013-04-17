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

int CALLBACK WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     int nCmdShow) {
	MSG msg = {0};
	HWND hPoE = 0, hOldPoE = 0;
	HMODULE theDll;
	FARPROC hookProc;
	HHOOK hook;
	UINT windowMessage;

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


	if (RegisterHotKey(NULL,
	                   1,
	                   MOD_ALT | MOD_NOREPEAT,
	                   VK_SPACE) == FALSE) {
		MessageBox(NULL,"Failed to Create Hotkey!", NULL, NULL);
		return 1;
	}

	
	while (GetMessage(&msg,NULL,0,0) != 0) {
		if (msg.message == WM_HOTKEY) {
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
		}
	}

	return 0;
}
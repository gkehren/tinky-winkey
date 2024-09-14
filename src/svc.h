#ifndef SVC_H
#define SVC_H

#define SVCNAME TEXT("tinky")
#define WINKEY_PATH TEXT("C:\\Users\\User\\Desktop\\tinky-winkey\\winkey.exe")

VOID SvcInstall(void);
VOID SvcDelete(void);
VOID SvcStart(void);
VOID SvcStop(void);

VOID WINAPI SvcCtrlHandler(DWORD CtrlCode);
VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR* lpszArgv);
VOID SvcInit(DWORD dwArgc, LPTSTR* lpszArgv);
VOID SvcReportEvent(LPTSTR szFunction);
VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);

#endif
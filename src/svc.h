#ifndef SVC_H
#define SVC_H

#define SVCNAME TEXT("tinky")

VOID SvcInstall();
VOID SvcDelete();
VOID SvcStart();
VOID SvcStop();

VOID WINAPI SvcCtrlHandler(DWORD CtrlCode);
VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR* lpszArgv);
VOID SvcInit(DWORD dwArgc, LPTSTR* lpszArgv);
VOID SvcReportEvent(LPTSTR szFunction);
VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);

#endif
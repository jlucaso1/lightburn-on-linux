#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <stdio.h>
#include <wchar.h>

static void CALLBACK service_changed(void *parameter)
{
    SERVICE_NOTIFYW *notification = parameter;
    *(BOOL *)notification->pContext = TRUE;
}

int main(void)
{
    SC_HANDLE manager = NULL, service = NULL;
    SERVICE_STATUS status;
    DWORD error = ERROR_SUCCESS, exit_code;
    const WCHAR *tail = GetCommandLineW();
    const WCHAR application[] = L"C:\\LightBurn\\LightBurn.exe";
    const WCHAR command_prefix[] = L"\"C:\\LightBurn\\LightBurn.exe\"";
    WCHAR *command = NULL;
    STARTUPINFOW startup = { .cb = sizeof(startup) };
    PROCESS_INFORMATION process;
    HANDLE job;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {0};

    job = CreateJobObjectW(NULL, NULL);
    if (!job) { error = GetLastError(); goto failed; }
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits)) ||
        !AssignProcessToJobObject(job, GetCurrentProcess()))
    { error = GetLastError(); goto failed; }
    /* Children inherit this job atomically. Only this process owns its handle,
       kept until ExitProcess so killing the helper also kills every descendant. */

    manager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!manager) { error = GetLastError(); goto failed; }
    service = OpenServiceW(manager, L"Winmgmt", SERVICE_START | SERVICE_QUERY_STATUS);
    if (!service) { error = GetLastError(); goto failed; }
    if (!QueryServiceStatus(service, &status)) { error = GetLastError(); goto failed; }
    if (status.dwCurrentState == SERVICE_STOPPED)
    {
        if (!StartServiceW(service, 0, NULL))
        {
            error = GetLastError();
            if (error != ERROR_SERVICE_ALREADY_RUNNING) goto failed;
        }
        if (!QueryServiceStatus(service, &status)) { error = GetLastError(); goto failed; }
    }
    while (status.dwCurrentState == SERVICE_START_PENDING)
    {
        BOOL notified = FALSE;
        SERVICE_NOTIFYW notification = {
            .dwVersion = SERVICE_NOTIFY_STATUS_CHANGE,
            .pfnNotifyCallback = service_changed,
            .pContext = &notified
        };
        /* Registration also reports a state reached between query and subscribe. */
        error = NotifyServiceStatusChangeW(service,
            SERVICE_NOTIFY_RUNNING | SERVICE_NOTIFY_STOPPED | SERVICE_NOTIFY_STOP_PENDING |
            SERVICE_NOTIFY_PAUSED | SERVICE_NOTIFY_PAUSE_PENDING | SERVICE_NOTIFY_CONTINUE_PENDING,
            &notification);
        if (error) goto failed;
        while (!notified) SleepEx(INFINITE, TRUE);
        error = notification.dwNotificationStatus;
        if (error) goto failed;
        if (!QueryServiceStatus(service, &status)) { error = GetLastError(); goto failed; }
    }
    if (status.dwCurrentState != SERVICE_RUNNING)
    {
        error = status.dwWin32ExitCode ? status.dwWin32ExitCode : ERROR_SERVICE_NOT_ACTIVE;
        goto failed;
    }
    CloseServiceHandle(service);
    service = NULL;
    CloseServiceHandle(manager);
    manager = NULL;

    /* Replace only argv[0]; preserve Wine's quoting of the caller's arguments. */
    if (*tail == L'"')
    {
        ++tail;
        while (*tail && *tail != L'"') ++tail;
        if (*tail) ++tail;
    }
    else
        while (*tail && *tail != L' ' && *tail != L'\t') ++tail;
    command = HeapAlloc(GetProcessHeap(), 0, sizeof(command_prefix) + wcslen(tail) * sizeof(WCHAR));
    if (!command) { error = ERROR_NOT_ENOUGH_MEMORY; goto failed; }
    wcscpy(command, command_prefix);
    wcscat(command, tail);
    if (!CreateProcessW(application, command, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &process))
    { error = GetLastError(); goto failed; }
    HeapFree(GetProcessHeap(), 0, command);
    command = NULL;
    CloseHandle(process.hThread);
    if (WaitForSingleObject(process.hProcess, INFINITE) != WAIT_OBJECT_0 ||
        !GetExitCodeProcess(process.hProcess, &exit_code))
    {
        error = GetLastError();
        CloseHandle(process.hProcess);
        goto failed;
    }
    CloseHandle(process.hProcess);
    ExitProcess(exit_code);

failed:
    fprintf(stderr, "LightBurn native startup failed, Win32 error %lu\n", (unsigned long)error);
    if (command) HeapFree(GetProcessHeap(), 0, command);
    if (service) CloseServiceHandle(service);
    if (manager) CloseServiceHandle(manager);
    ExitProcess(1);
}

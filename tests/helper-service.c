#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    SC_HANDLE manager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    assert(manager);
    SC_HANDLE service = OpenServiceW(manager, L"Winmgmt", SERVICE_QUERY_STATUS);
    assert(service);
    SERVICE_STATUS status;
    assert(QueryServiceStatus(service, &status));
    if (argc == 2 && !strcmp(argv[1], "--fixture"))
    {
        assert(status.dwCurrentState == SERVICE_RUNNING);
        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        return 37;
    }
    assert(argc == 1);
    printf("Winmgmt initial state=%lu\n", (unsigned long)status.dwCurrentState);
    fflush(stdout);
    WCHAR module[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, module, MAX_PATH);
    assert(length && length < MAX_PATH);
    assert(CreateDirectoryW(L"C:\\LightBurn", NULL) || GetLastError() == ERROR_ALREADY_EXISTS);
    assert(CopyFileW(module, L"C:\\LightBurn\\LightBurn.exe", TRUE));
    for (unsigned attempt = 0; attempt < 2; ++attempt)
    {
        WCHAR command[] = L"\"Z:\\validation\\start-lightburn.exe\" --fixture";
        STARTUPINFOW startup = { .cb = sizeof(startup) };
        PROCESS_INFORMATION process;
        assert(CreateProcessW(L"Z:\\validation\\start-lightburn.exe", command, NULL, NULL,
                              TRUE, 0, NULL, NULL, &startup, &process));
        CloseHandle(process.hThread);
        assert(WaitForSingleObject(process.hProcess, 30000) == WAIT_OBJECT_0);
        DWORD exit_code;
        assert(GetExitCodeProcess(process.hProcess, &exit_code) && exit_code == 37);
        CloseHandle(process.hProcess);
        assert(QueryServiceStatus(service, &status) && status.dwCurrentState == SERVICE_RUNNING);
        puts(attempt ? "PASS production helper repeated startup, fixture exit 37" :
                       "PASS production helper initial startup, fixture exit 37");
    }
    assert(DeleteFileW(L"C:\\LightBurn\\LightBurn.exe"));
    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    return 0;
}

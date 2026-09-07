#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <wchar.h>
#include <tlhelp32.h>
#include <string.h>

static DWORD state, start_error, query_error, notify_error, completion_error, next_state;
static unsigned starts, queries, waits, launches, handles;
static SERVICE_NOTIFYW *subscription;
static const WCHAR *input, *expected;
static jmp_buf finished;
enum process_failure { NONE, CREATE_JOB, SET_JOB, ASSIGN_JOB, CREATE_CHILD, WAIT_CHILD, CHILD_EXIT_CODE };
static enum process_failure failure;
static BOOL job_open, job_assigned, child_alive;
static DWORD expected_exit;
static unsigned killed_children;
static BOOL real_processes;

static HANDLE mock_job(SECURITY_ATTRIBUTES *attributes, const WCHAR *name)
{
    assert(!attributes && !name);
    if (real_processes) { job_open = TRUE; return CreateJobObjectW(attributes, name); }
    if (failure == CREATE_JOB) { SetLastError(ERROR_ACCESS_DENIED); return NULL; }
    job_open = TRUE;
    return (HANDLE)5;
}

static BOOL mock_limits(HANDLE job, JOBOBJECTINFOCLASS type, void *information, DWORD size)
{
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION *limits = information;
    if (real_processes) return SetInformationJobObject(job, type, information, size);
    assert(job == (HANDLE)5 && job_open && type == JobObjectExtendedLimitInformation && size == sizeof(*limits));
    assert(limits->BasicLimitInformation.LimitFlags == JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE);
    if (failure == SET_JOB) { SetLastError(ERROR_ACCESS_DENIED); return FALSE; }
    return TRUE;
}

static BOOL mock_assign(HANDLE job, HANDLE process)
{
    if (real_processes) { job_assigned = AssignProcessToJobObject(job, process); return job_assigned; }
    assert(job == (HANDLE)5 && job_open && process == GetCurrentProcess() && !launches);
    if (failure == ASSIGN_JOB) { SetLastError(ERROR_ACCESS_DENIED); return FALSE; }
    job_assigned = TRUE;
    return TRUE;
}

static SC_HANDLE mock_manager(const WCHAR *machine, const WCHAR *database, DWORD access)
{
    assert(!machine && !database && access == SC_MANAGER_CONNECT);
    assert(job_assigned);
    ++handles;
    return (SC_HANDLE)1;
}

static SC_HANDLE mock_service(SC_HANDLE manager, const WCHAR *name, DWORD access)
{
    assert(manager == (SC_HANDLE)1 && !wcscmp(name, L"Winmgmt"));
    assert(access == (SERVICE_START | SERVICE_QUERY_STATUS));
    ++handles;
    return (SC_HANDLE)2;
}

static BOOL mock_close(SC_HANDLE handle)
{
    assert(handle == (SC_HANDLE)1 || handle == (SC_HANDLE)2);
    assert(handles);
    --handles;
    return TRUE;
}

static BOOL mock_query(SC_HANDLE handle, SERVICE_STATUS *status)
{
    assert(handle == (SC_HANDLE)2);
    ++queries;
    if (query_error) { SetLastError(query_error); return FALSE; }
    ZeroMemory(status, sizeof(*status));
    status->dwCurrentState = state;
    return TRUE;
}

static BOOL mock_start(SC_HANDLE handle, DWORD argc, const WCHAR **argv)
{
    assert(handle == (SC_HANDLE)2 && !argc && !argv && state == SERVICE_STOPPED);
    ++starts;
    if (!start_error || start_error == ERROR_SERVICE_ALREADY_RUNNING) state = SERVICE_START_PENDING;
    SetLastError(start_error);
    return !start_error;
}

static DWORD mock_notify(SC_HANDLE handle, DWORD mask, SERVICE_NOTIFYW *notification)
{
    assert(handle == (SC_HANDLE)2);
    assert((mask & (SERVICE_NOTIFY_RUNNING | SERVICE_NOTIFY_STOPPED)) ==
           (SERVICE_NOTIFY_RUNNING | SERVICE_NOTIFY_STOPPED));
    assert(!(mask & SERVICE_NOTIFY_START_PENDING));
    assert(notification->dwVersion == SERVICE_NOTIFY_STATUS_CHANGE);
    subscription = notification;
    return notify_error;
}

static DWORD mock_sleep(DWORD milliseconds, BOOL alertable)
{
    assert(milliseconds == INFINITE && alertable && subscription && !launches);
    ++waits;
    state = next_state;
    subscription->dwNotificationStatus = completion_error;
    subscription->pfnNotifyCallback(subscription);
    return WAIT_IO_COMPLETION;
}

static WCHAR *mock_command(void) { return (WCHAR *)input; }

static BOOL mock_create(const WCHAR *application, WCHAR *command,
                        SECURITY_ATTRIBUTES *process_attributes, SECURITY_ATTRIBUTES *thread_attributes,
                        BOOL inherit, DWORD flags, void *environment, const WCHAR *directory,
                        STARTUPINFOW *startup, PROCESS_INFORMATION *process)
{
    assert(state == SERVICE_RUNNING && handles == 0);
    assert(job_assigned && job_open);
    assert(!wcscmp(application, L"C:\\LightBurn\\LightBurn.exe"));
    assert(!wcscmp(command, expected));
    assert(!process_attributes && !thread_attributes && inherit && !flags && !environment && !directory);
    assert(startup->cb == sizeof(*startup));
    if (real_processes)
    {
        WCHAR module[MAX_PATH], child_command[MAX_PATH + 40];
        assert(GetModuleFileNameW(NULL, module, MAX_PATH));
        swprintf(child_command, MAX_PATH + 40, L"\"%ls\" --child", module);
        return CreateProcessW(module, child_command, NULL, NULL, TRUE, 0, NULL, NULL, startup, process);
    }
    if (failure == CREATE_CHILD) { SetLastError(ERROR_ACCESS_DENIED); return FALSE; }
    process->hProcess = (HANDLE)3;
    process->hThread = (HANDLE)4;
    ++launches;
    child_alive = TRUE;
    return TRUE;
}

static BOOL mock_close_process(HANDLE handle)
{
    if (real_processes) return CloseHandle(handle);
    assert(handle == (HANDLE)3 || handle == (HANDLE)4);
    return TRUE;
}
static DWORD mock_wait(HANDLE handle, DWORD milliseconds)
{
    if (real_processes) return WaitForSingleObject(handle, milliseconds);
    assert(handle == (HANDLE)3 && milliseconds == INFINITE);
    if (failure == WAIT_CHILD) { SetLastError(ERROR_INVALID_HANDLE); return WAIT_FAILED; }
    child_alive = FALSE;
    return WAIT_OBJECT_0;
}
static BOOL mock_exit_code(HANDLE handle, DWORD *code)
{
    if (real_processes) return GetExitCodeProcess(handle, code);
    assert(handle == (HANDLE)3);
    if (failure == CHILD_EXIT_CODE) { SetLastError(ERROR_ACCESS_DENIED); return FALSE; }
    *code = 37;
    return TRUE;
}
_Noreturn static void mock_exit(DWORD code)
{
    if (real_processes) ExitProcess(code);
    assert(code == expected_exit);
    if (child_alive) { assert(job_open && job_assigned); ++killed_children; }
    job_open = job_assigned = child_alive = FALSE;
    longjmp(finished, 1);
}

#define OpenSCManagerW mock_manager
#define OpenServiceW mock_service
#define CloseServiceHandle mock_close
#define QueryServiceStatus mock_query
#define StartServiceW mock_start
#define NotifyServiceStatusChangeW mock_notify
#define SleepEx mock_sleep
#define GetCommandLineW mock_command
#define CreateProcessW mock_create
#define CloseHandle mock_close_process
#define WaitForSingleObject mock_wait
#define GetExitCodeProcess mock_exit_code
#define ExitProcess mock_exit
#define CreateJobObjectW mock_job
#define SetInformationJobObject mock_limits
#define AssignProcessToJobObject mock_assign
#define main launcher_main
#include "../scripts/start-lightburn.c"
#undef main
#undef CreateProcessW
#undef CloseHandle
#undef WaitForSingleObject
#undef GetExitCodeProcess
#undef ExitProcess

static void run_launcher(void)
{
    if (!setjmp(finished)) { launcher_main(); assert(0); }
}

int main(int argc, char **argv)
{
    WCHAR module[MAX_PATH], child_command[MAX_PATH + 40], event_name[80];
    STARTUPINFOW startup = { .cb = sizeof(startup) };
    PROCESS_INFORMATION process;
    assert(GetModuleFileNameW(NULL, module, MAX_PATH));
    if (argc == 2 && !strcmp(argv[1], "--parent"))
    {
        real_processes = TRUE;
        state = SERVICE_RUNNING;
        input = L"helper.exe";
        expected = L"\"C:\\LightBurn\\LightBurn.exe\"";
        launcher_main();
        assert(0);
    }
    if (argc == 2 && !strcmp(argv[1], "--child"))
    {
        WCHAR value[2];
        if (GetEnvironmentVariableW(L"LIGHTBURN_JOB_TEST_EXIT", value, 2)) return 37;
        swprintf(child_command, MAX_PATH + 40, L"\"%ls\" --grandchild", module);
        assert(CreateProcessW(module, child_command, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &process));
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        Sleep(INFINITE);
    }
    if (argc == 2 && !strcmp(argv[1], "--grandchild"))
    {
        assert(GetEnvironmentVariableW(L"LIGHTBURN_JOB_TEST_EVENT", event_name, 80));
        HANDLE ready = OpenEventW(EVENT_MODIFY_STATE, FALSE, event_name);
        assert(ready && SetEvent(ready));
        CloseHandle(ready);
        Sleep(INFINITE);
    }
    assert(argc == 1);
    const WCHAR *tails[] = {
        L"", L" \"file with spaces.lbrn2\" \"--example=a b\"", L" \"\"",
        L" \"trailing\\\\\" \"embedded\\\"quote\"", L" & | < > %PATH% ! ^ ( )",
        L" \"\x03bb\x6587.lbrn2\""
    };
    WCHAR command[512], forwarded[512];
    for (unsigned scenario = 0; scenario < 9; ++scenario)
    {
        state = scenario == 1 ? SERVICE_RUNNING : scenario == 2 ? SERVICE_START_PENDING : SERVICE_STOPPED;
        start_error = scenario == 3 ? ERROR_SERVICE_ALREADY_RUNNING : scenario == 4 ? ERROR_ACCESS_DENIED : 0;
        query_error = scenario == 5 ? ERROR_ACCESS_DENIED : 0;
        notify_error = scenario == 6 ? ERROR_INVALID_HANDLE : 0;
        completion_error = scenario == 7 ? ERROR_SERVICE_MARKED_FOR_DELETE : 0;
        next_state = scenario == 8 ? SERVICE_STOPPED : SERVICE_RUNNING;
        starts = queries = waits = launches = handles = 0;
        expected_exit = scenario < 4 ? 37 : 1;
        input = L"C:\\LightBurn\\helper.exe";
        expected = L"\"C:\\LightBurn\\LightBurn.exe\"";
        run_launcher();
        assert(handles == 0);
        assert(launches == (scenario < 4));
        if (scenario == 1) assert(!starts && !waits);
        if (scenario == 2) assert(!starts && waits == 1);
        if (scenario == 0 || scenario == 3) assert(starts == 1 && waits == 1);
    }
    for (failure = CREATE_JOB; failure <= CHILD_EXIT_CODE; ++failure)
    {
        state = SERVICE_RUNNING;
        query_error = 0;
        starts = queries = waits = launches = handles = killed_children = 0;
        expected_exit = 1;
        run_launcher();
        assert(!handles && !job_open && !child_alive);
        assert(launches == (failure >= WAIT_CHILD));
        assert(killed_children == (failure == WAIT_CHILD));
        if (failure <= ASSIGN_JOB) assert(!queries);
    }
    failure = NONE;
    expected_exit = 37;
    for (unsigned i = 0; i < sizeof(tails) / sizeof(*tails); ++i)
    {
        state = SERVICE_RUNNING;
        query_error = 0;
        handles = 0;
        launches = 0;
        wcscpy(command, L"\"C:\\path with spaces\\helper.exe\"");
        wcscat(command, tails[i]);
        wcscpy(forwarded, L"\"C:\\LightBurn\\LightBurn.exe\"");
        wcscat(forwarded, tails[i]);
        input = command;
        expected = forwarded;
        run_launcher();
    }
    puts("PASS 9 native service scenarios, 6 process failures and 6 argument tails");
    swprintf(event_name, 80, L"Local\\lightburn-job-test-%lu", (unsigned long)GetCurrentProcessId());
    HANDLE ready = CreateEventW(NULL, TRUE, FALSE, event_name);
    assert(ready && GetLastError() != ERROR_ALREADY_EXISTS);
    assert(SetEnvironmentVariableW(L"LIGHTBURN_JOB_TEST_EVENT", event_name));
    assert(SetEnvironmentVariableW(L"LIGHTBURN_JOB_TEST_EXIT", NULL));
    swprintf(child_command, MAX_PATH + 40, L"\"%ls\" --parent", module);
    assert(CreateProcessW(module, child_command, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &process));
    assert(WaitForSingleObject(ready, 10000) == WAIT_OBJECT_0);
    DWORD ids[3] = { process.dwProcessId, 0, 0 };
    HANDLE children[2];
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    assert(snapshot != INVALID_HANDLE_VALUE);
    for (unsigned i = 0; i < 2; ++i)
    {
        PROCESSENTRY32W entry = { .dwSize = sizeof(entry) };
        assert(Process32FirstW(snapshot, &entry));
        do
        {
            if (entry.th32ParentProcessID == ids[i])
            {
                assert(!ids[i + 1]);
                ids[i + 1] = entry.th32ProcessID;
            }
        } while (Process32NextW(snapshot, &entry));
        assert(ids[i + 1]);
        children[i] = OpenProcess(SYNCHRONIZE, FALSE, ids[i + 1]);
        assert(children[i]);
    }
    CloseHandle(snapshot);
    assert(TerminateProcess(process.hProcess, 99));
    assert(WaitForSingleObject(process.hProcess, 5000) == WAIT_OBJECT_0);
    for (unsigned i = 0; i < 2; ++i)
    {
        assert(WaitForSingleObject(children[i], 5000) == WAIT_OBJECT_0);
        CloseHandle(children[i]);
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    CloseHandle(ready);
    puts("PASS real helper, child and grandchild terminate together");
    assert(SetEnvironmentVariableW(L"LIGHTBURN_JOB_TEST_EXIT", L"1"));
    swprintf(child_command, MAX_PATH + 40, L"\"%ls\" --parent", module);
    assert(CreateProcessW(module, child_command, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &process));
    assert(WaitForSingleObject(process.hProcess, 10000) == WAIT_OBJECT_0);
    DWORD exit_code;
    assert(GetExitCodeProcess(process.hProcess, &exit_code) && exit_code == 37);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    puts("PASS real helper preserves child exit 37");
    return 0;
}

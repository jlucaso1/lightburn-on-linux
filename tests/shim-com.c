#include <stdio.h>
#include <stddef.h>
#define AsyncStatus ShimAsyncStatus
#include "../shim/winrtcamstub.c"
#undef AsyncStatus
#include <asyncinfo.h>
#include <winstring.h>

// Public ABI references are listed in winrtcamstub.c. Use the SDK interfaces at
// the two call sites whose incorrect vtable layouts caused the original repro.
_Static_assert(offsetof(CompletedAsyncVtbl, GetResults) == 8 * sizeof(void*), "async ABI");
_Static_assert(sizeof(CompletedAsyncVtbl) == 9 * sizeof(void*), "no IAsyncInfo tail");
_Static_assert(offsetof(DIFactoryVtbl, CreateWatcherAqsFilter) == 14 * sizeof(void*), "statics ABI");
_Static_assert(offsetof(DIWatcherVtbl, Start) == 17 * sizeof(void*), "watcher ABI");

static unsigned passed, failed;
static void check(const char *name, int ok) {
    printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    if (ok) passed++; else failed++;
}
#define REQUIRE(name, expression) do { \
    int ok = !!(expression); check(name, ok); if (!ok) return 1; \
} while (0)

static void release(void *p) {
    if (p) { IUnknown *u = p; u->lpVtbl->Release(u); }
}
static void identity(void *object, REFIID iid) {
    IUnknown *u = object, *own = NULL, *a = NULL, *b = NULL;
    HRESULT hr = u->lpVtbl->QueryInterface(u, iid, (void**)&own);
    check("QI accepts the object's public IID", hr == S_OK && own != NULL);
    if (!own) return;
    u->lpVtbl->QueryInterface(u, &IID_IUnknown_, (void**)&a);
    own->lpVtbl->QueryInterface(own, &IID_IUnknown_, (void**)&b);
    check("QI shares canonical IUnknown identity", a && a == b);
    void *again = NULL;
    own->lpVtbl->QueryInterface(own, iid, &again);
    check("QI is reflexive", again == own);
    release(again); release(a); release(b); release(own);
    void *unknown = (void*)1;
    const GUID unsupported = {0xdeadbeef,0,0,{0}};
    check("unsupported QI clears output", u->lpVtbl->QueryInterface(u, &unsupported, &unknown) == E_NOINTERFACE && !unknown);
}
static void discovery(void *object, const GUID *expected, ULONG expectedCount) {
    IInspectable *p = object;
    ULONG count = 0, count2 = 0;
    IID *ids = NULL, *ids2 = NULL;
    HRESULT hr = p->lpVtbl->GetIids(p, &count, &ids);
    check("GetIids lists implemented WinRT interfaces", hr == S_OK && count == expectedCount && ids);
    if (ids && count == expectedCount) {
        for (ULONG i = 0; i < count; i++) {
            ULONG matches = 0;
            for (ULONG j = 0; j < count; j++) matches += guid_eq(&expected[i], &ids[j]);
            check("GetIids contains each expected IID exactly once", matches == 1);
            void *queried = NULL;
            check("each discovered IID supports QI", p->lpVtbl->QueryInterface(p, &ids[i], &queried) == S_OK && queried);
            release(queried);
        }
    }
    hr = p->lpVtbl->GetIids(p, &count2, &ids2);
    check("GetIids returns independent caller-owned arrays", hr == S_OK && count2 == expectedCount && ids2 && ids2 != ids);
    CoTaskMemFree(ids); CoTaskMemFree(ids2);
}

typedef struct {
    CompletedHandler iface;
    LONG refs, calls;
    BOOL sawCompleted, getterOwned, releaseOperation;
    CompletedAsync *addReenter;
    HRESULT reenteredAssignment;
} Completion;
static HRESULT STDMETHODCALLTYPE completion_qi(CompletedHandler *p, REFGUID id, void **o) {
    (void)id; *o = p; p->lpVtbl->AddRef(p); return S_OK;
}
static ULONG STDMETHODCALLTYPE completion_add(CompletedHandler *p) {
    Completion *h = (Completion*)p;
    ULONG refs = InterlockedIncrement(&h->refs);
    CompletedAsync *op = h->addReenter;
    h->addReenter = NULL;
    if (op) h->reenteredAssignment = op->lpVtbl->put_Completed(op, p);
    return refs;
}
static ULONG STDMETHODCALLTYPE completion_rel(CompletedHandler *p) { return InterlockedDecrement(&((Completion*)p)->refs); }
static HRESULT STDMETHODCALLTYPE completion_invoke(CompletedHandler *p, CompletedAsync *op, ShimAsyncStatus st) {
    Completion *h = (Completion*)p;
    InterlockedIncrement(&h->calls);
    h->sawCompleted = st == AsyncStatus_Completed;
    CompletedHandler *got = NULL;
    op->lpVtbl->get_Completed(op, &got);
    h->getterOwned = got == p && h->refs == 3;
    release(got);
    if (h->releaseOperation) op->lpVtbl->Release(op);
    return S_OK;
}
static const CompletedHandlerVtbl completion_vtbl = {completion_qi, completion_add, completion_rel, completion_invoke};

typedef struct {
    WatchHandler iface;
    LONG refs, calls;
    DIWatcher *sender;
    int status;
    HANDLE entered, resume;
    BOOL removeSelf, stopSelf;
    INT64 token;
    DIWatcher *addReenter;
    HRESULT reenteredStatus;
} Event;
static HRESULT STDMETHODCALLTYPE event_qi(WatchHandler *p, REFGUID id, void **o) {
    (void)id; *o = p; p->lpVtbl->AddRef(p); return S_OK;
}
static ULONG STDMETHODCALLTYPE event_add(WatchHandler *p) {
    Event *h = (Event*)p;
    ULONG refs = InterlockedIncrement(&h->refs);
    DIWatcher *w = h->addReenter;
    h->addReenter = NULL;
    if (w) {
        h->reenteredStatus = w->lpVtbl->get_Status(w, &h->status);
        w->lpVtbl->remove_EnumerationCompleted(w, h->token);
    }
    return refs;
}
static ULONG STDMETHODCALLTYPE event_rel(WatchHandler *p) { return InterlockedDecrement(&((Event*)p)->refs); }
static HRESULT STDMETHODCALLTYPE event_invoke(WatchHandler *p, DIWatcher *w, void *args) {
    Event *h = (Event*)p;
    (void)args;
    InterlockedIncrement(&h->calls);
    h->sender = w;
    w->lpVtbl->get_Status(w, &h->status);
    if (h->removeSelf) w->lpVtbl->remove_EnumerationCompleted(w, h->token);
    if (h->stopSelf) w->lpVtbl->Stop(w);
    if (h->entered) {
        SetEvent(h->entered);
        if (WaitForSingleObject(h->resume, 10000) != WAIT_OBJECT_0) ExitProcess(2);
    }
    return S_OK;
}
static const WatchHandlerVtbl event_vtbl = {event_qi, event_add, event_rel, event_invoke};
static DWORD WINAPI start_thread(void *p) {
    DIWatcher *w = p;
    HRESULT hr = w->lpVtbl->Start(w);
    release(w);
    return (DWORD)hr;
}
static DWORD WINAPI completion_thread(void *p) {
    CompletedAsync *op = p;
    CompletedHandler *h = NULL;
    HRESULT hr = op->lpVtbl->get_Completed(op, &h);
    release(h);
    release(op);
    return (DWORD)hr;
}

static int reentrant_refs(CamFactory *cam, DIFactory *di) {
    CompletedAsync *op = NULL;
    DIWatcher *w = NULL;
    REQUIRE("create reentrant AddRef operation", cam->lpVtbl->FindAllAsync(cam, &op) == S_OK);
    REQUIRE("create reentrant AddRef watcher", di->lpVtbl->CreateWatcherAqsFilter(di, NULL, &w) == S_OK);
    Completion completion = {.iface = {&completion_vtbl}, .refs = 1, .reenteredAssignment = E_FAIL};
    Event event = {.iface = {&event_vtbl}, .refs = 1, .reenteredStatus = E_FAIL};
    REQUIRE("register reentrant completion", op->lpVtbl->put_Completed(op, &completion.iface) == S_OK);
    REQUIRE("register reentrant watcher handler", w->lpVtbl->add_EnumerationCompleted(w, &event, &event.token) == S_OK);
    completion.addReenter = op;
    event.addReenter = w;
    op->lpVtbl->AddRef(op);
    w->lpVtbl->AddRef(w);
    HANDLE threads[] = {CreateThread(NULL, 0, completion_thread, op, 0, NULL),
                        CreateThread(NULL, 0, start_thread, w, 0, NULL)};
    REQUIRE("create reentrant AddRef workers", threads[0] && threads[1]);
    BOOL asyncDone = WaitForSingleObject(threads[0], 3000) == WAIT_OBJECT_0;
    BOOL watcherDone = WaitForSingleObject(threads[1], 3000) == WAIT_OBJECT_0;
    check("completion AddRef can reenter setter without deadlock", asyncDone);
    check("watcher AddRef can reenter status and removal without deadlock", watcherDone);
    // A failed worker still owns stack-backed delegates. End the process, not its thread.
    if (!asyncDone || !watcherDone) ExitProcess(1);
    check("reentrant setter rejects reassignment", completion.reenteredAssignment == E_ILLEGAL_DELEGATE_ASSIGNMENT);
    check("reentrant watcher removal preserves delivery", event.reenteredStatus == S_OK && event.calls == 1 && event.refs == 1);
    CloseHandle(threads[0]); CloseHandle(threads[1]);
    release(op); release(w);
    check("reentrant AddRef releases all owned references", completion.refs == 1 && event.refs == 1);
    return 0;
}

int main(int argc, char **argv) {
    typedef HRESULT (STDMETHODCALLTYPE *GetFactory)(void*, void**);
    GetFactory getFactory = DllGetActivationFactory;
    HMODULE dll = NULL;
    if (argc == 2) {
        dll = LoadLibraryA(argv[1]);
        REQUIRE("load separately built DLL", dll != NULL);
        FARPROC proc = GetProcAddress(dll, "DllGetActivationFactory");
        REQUIRE("resolve DLL export", proc != NULL);
        _Static_assert(sizeof(proc) == sizeof(getFactory), "Windows function pointer size");
        memcpy(&getFactory, &proc, sizeof(proc));
    }
    const wchar_t *classes[] = {
        L"Windows.Media.Capture.Frames.MediaFrameSourceGroup",
        L"Windows.Devices.Enumeration.DeviceInformation"
    };
    IActivationFactory *factories[2] = {0};
    for (unsigned i = 0; i < 2; i++) {
        HSTRING name = NULL;
        REQUIRE("create class HSTRING", WindowsCreateString(classes[i], (UINT32)wcslen(classes[i]), &name) == S_OK);
        HRESULT hr = getFactory(name, (void**)&factories[i]);
        WindowsDeleteString(name);
        REQUIRE("exact runtime class returns factory", hr == S_OK && factories[i]);
        identity(factories[i], &IID_IActivationFactory_);
        const GUID factoryIids[] = {IID_IActivationFactory_, i ? IID_DIStatics_ : IID_MFSGStatics_};
        discovery(factories[i], factoryIids, 2);
        IInspectable *instance = (void*)1;
        check("ActivateInstance rejects static-only class", factories[i]->lpVtbl->ActivateInstance(factories[i], &instance) == E_NOTIMPL && !instance);
    }
    const wchar_t *unknown[] = {L"", L"Unrelated.Unknown", L"DeviceInformation",
        L"Other.DeviceInformation", L"Other.MediaFrameSourceGroup",
        L"windows.Devices.Enumeration.DeviceInformation",
        L"Windows.Media.Capture.Frames.MediaFrameSourceGroupX"};
    for (unsigned i = 0; i < sizeof(unknown) / sizeof(*unknown); i++) {
        HSTRING name = NULL;
        WindowsCreateString(unknown[i], (UINT32)wcslen(unknown[i]), &name);
        void *out = (void*)1;
        check("unknown or suffix-only class rejected", getFactory(name, &out) == CLASS_E_CLASSNOTAVAILABLE && !out);
        WindowsDeleteString(name);
    }
    wchar_t embedded[] = L"Windows.Devices.Enumeration.DeviceInformation\0extra";
    HSTRING name = NULL;
    WindowsCreateString(embedded, sizeof(embedded) / sizeof(*embedded) - 1, &name);
    void *out = (void*)1;
    check("embedded NUL cannot bypass class match", getFactory(name, &out) == CLASS_E_CLASSNOTAVAILABLE && !out);
    WindowsDeleteString(name);
    check("factory validates output pointer", getFactory(NULL, NULL) == E_POINTER);

    CamFactory *cam = NULL;
    REQUIRE("factory QI returns media statics", factories[0]->lpVtbl->QueryInterface(factories[0], &IID_MFSGStatics_, (void**)&cam) == S_OK && cam);
    check("activation and media statics have separate vtables", (void*)cam->lpVtbl != (void*)factories[0]->lpVtbl);
    identity(cam, &IID_MFSGStatics_);
    identity(cam, &IID_IActivationFactory_);
    const GUID camIids[] = {IID_IActivationFactory_, IID_MFSGStatics_};
    discovery(cam, camIids, 2);
    CompletedAsync *op = NULL, *op2 = NULL;
    REQUIRE("FindAllAsync returns operation", cam->lpVtbl->FindAllAsync(cam, &op) == S_OK && op);
    REQUIRE("second FindAllAsync returns separate operation", cam->lpVtbl->FindAllAsync(cam, &op2) == S_OK && op2 && op2 != op);
    const GUID async_iid = {0xa795889f,0x6d49,0x5687,{0xaa,0xbe,0xf2,0xfc,0x62,0x37,0xfa,0x1a}};
    const GUID vector_iid = {0xd01148ae,0xcccd,0x56eb,{0xb2,0xb4,0xa7,0xd2,0xac,0xce,0x14,0xec}};
    const GUID watcher_iid = {0xc9eab97d,0x8f6b,0x4f96,{0xa9,0xf4,0xab,0xc8,0x14,0xe2,0x22,0x71}};
    identity(op, &async_iid);
    identity(op, &IID_IAsyncInfo_);
    IAsyncInfo *info = NULL;
    REQUIRE("QI IAsyncInfo succeeds", op->lpVtbl->QueryInterface(op, &IID_IAsyncInfo_, (void**)&info) == S_OK && info);
    check("IAsyncInfo is a separate interface", (void*)info != (void*)op);
    const GUID asyncIids[] = {async_iid, IID_IAsyncInfo_};
    discovery(op, asyncIids, 2);
    discovery(info, asyncIids, 2);
    struct { AsyncStatus status; UINT32 guard; } status = {Started, 0x12345678};
    check("IAsyncInfo reports Completed", info->lpVtbl->get_Status(info, &status.status) == S_OK && status.status == Completed);
    check("IAsyncInfo preserves adjacent memory", status.guard == 0x12345678);
    UINT32 id = 0;
    HRESULT error = E_FAIL;
    check("IAsyncInfo get_Id slot", info->lpVtbl->get_Id(info, &id) == S_OK && id != 0);
    check("IAsyncInfo get_ErrorCode slot", info->lpVtbl->get_ErrorCode(info, &error) == S_OK && error == S_OK);
    check("IAsyncInfo Cancel slot", info->lpVtbl->Cancel(info) == S_OK);
    EmptyVector *vec = NULL;
    REQUIRE("GetResults returns collection", op->lpVtbl->GetResults(op, &vec) == S_OK && vec);
    UINT32 size = 99, count = 99, index = 99;
    BOOLEAN found = TRUE;
    check("collection is empty", vec->lpVtbl->get_Size(vec, &size) == S_OK && !size);
    identity(vec, &vector_iid);
    discovery(vec, &vector_iid, 1);
    out = (void*)1;
    check("GetAt returns E_BOUNDS", vec->lpVtbl->GetAt(vec, 0, &out) == (HRESULT)0x8000000BL && !out);
    check("IndexOf reports absent", vec->lpVtbl->IndexOf(vec, NULL, &index, &found) == S_OK && !index && !found);
    check("GetMany accepts empty range", vec->lpVtbl->GetMany(vec, 0, 0, NULL, &count) == S_OK && !count);

    Completion completion = {.iface = {&completion_vtbl}, .refs = 1};
    CompletedHandler *got = (void*)1;
    check("completion getter initially empty", op->lpVtbl->get_Completed(op, &got) == S_OK && !got);
    check("completion delegate invoked at IUnknown slot 3", op->lpVtbl->put_Completed(op, &completion.iface) == S_OK && completion.calls == 1 && completion.sawCompleted);
    check("callback can reenter owned completion getter", completion.getterOwned);
    check("operation retains completion delegate", completion.refs == 2);
    op->lpVtbl->get_Completed(op, &got);
    check("completion getter returns owned delegate", got == &completion.iface && completion.refs == 3);
    release(got);
    check("second assignment rejected without callback or leak", op->lpVtbl->put_Completed(op, &completion.iface) == E_ILLEGAL_DELEGATE_ASSIGNMENT && completion.calls == 1 && completion.refs == 2);
    op2->lpVtbl->get_Completed(op2, &got);
    check("operation handlers are independent", !got);
    Completion releasing = {.iface = {&completion_vtbl}, .refs = 1, .releaseOperation = TRUE};
    check("callback may release last caller-owned operation reference", op2->lpVtbl->put_Completed(op2, &releasing.iface) == S_OK && releasing.calls == 1 && releasing.refs == 1);
    check("IAsyncInfo Close after operation use", info->lpVtbl->Close(info) == S_OK);
    release(info); release(op); release(vec);
    check("last operation release frees owned delegate", completion.refs == 1);

    DIFactory *di = NULL;
    REQUIRE("factory QI returns device statics", factories[1]->lpVtbl->QueryInterface(factories[1], &IID_DIStatics_, (void**)&di) == S_OK && di);
    identity(di, &IID_DIStatics_);
    identity(di, &IID_IActivationFactory_);
    const GUID diIids[] = {IID_IActivationFactory_, IID_DIStatics_};
    discovery(di, diIids, 2);
    check("activation and device statics have separate vtables", (void*)di->lpVtbl != (void*)factories[1]->lpVtbl);
    DIWatcher *w = NULL, *w2 = NULL;
    REQUIRE("CreateWatcherAqsFilter returns watcher", di->lpVtbl->CreateWatcherAqsFilter(di, NULL, &w) == S_OK && w);
    REQUIRE("watchers are per-instance", di->lpVtbl->CreateWatcherAqsFilter(di, NULL, &w2) == S_OK && w2 && w2 != w);
    identity(w, &watcher_iid);
    identity(w, &IID_IAgileObject_);
    discovery(w, &watcher_iid, 1);
    int st = -1;
    check("watcher initially Created", w->lpVtbl->get_Status(w, &st) == S_OK && st == 0);
    Event event = {.iface = {&event_vtbl}, .refs = 1};
    Event second = {.iface = {&event_vtbl}, .refs = 1};
    Event stopped = {.iface = {&event_vtbl}, .refs = 1};
    INT64 token, token2, stopToken;
    INT64 addedToken, updatedToken, removedToken;
    w->lpVtbl->add_Added(w, &event, &addedToken);
    w->lpVtbl->add_Updated(w, &event, &updatedToken);
    w->lpVtbl->add_Removed(w, &event, &removedToken);
    check("all watcher event kinds retain handlers", event.refs == 4);
    w->lpVtbl->remove_Added(w, addedToken);
    w->lpVtbl->remove_Updated(w, updatedToken);
    w->lpVtbl->remove_Removed(w, removedToken);
    check("all watcher event kinds release on removal", event.refs == 1);
    w->lpVtbl->add_EnumerationCompleted(w, &event, &token);
    check("watcher retains registered handler", event.refs == 2);
    w->lpVtbl->remove_EnumerationCompleted(w, token);
    check("removal releases handler", event.refs == 1);
    check("removed handler not invoked", w->lpVtbl->Start(w) == S_OK && !event.calls);
    check("Start leaves Created for EnumerationCompleted", w->lpVtbl->get_Status(w, &st) == S_OK && st == 2);
    check("repeated Start rejected", w->lpVtbl->Start(w) == E_ILLEGAL_METHOD_CALL);
    check("other watcher remains Created", w2->lpVtbl->get_Status(w2, &st) == S_OK && st == 0);
    w->lpVtbl->add_Stopped(w, &stopped, &stopToken);
    check("Stop invokes Stopped with correct state", w->lpVtbl->Stop(w) == S_OK && stopped.calls == 1 && stopped.status == 4);
    check("repeated Stop does not redeliver", w->lpVtbl->Stop(w) == S_OK && stopped.calls == 1);
    w->lpVtbl->add_EnumerationCompleted(w, &event, &token);
    w->lpVtbl->add_EnumerationCompleted(w, &second, &token2);
    check("registration tokens are distinct and not reused", token != token2 && token != stopToken && token > 1);
    w->lpVtbl->remove_Stopped(w, token);
    check("wrong event removal leaves registration intact", event.refs == 2);
    check("restart delivers both subscribers", w->lpVtbl->Start(w) == S_OK && event.calls == 1 && second.calls == 1 && event.sender == w);
    w->lpVtbl->Stop(w);
    event.removeSelf = event.stopSelf = TRUE;
    event.token = token;
    check("callback can remove itself and stop", w->lpVtbl->Start(w) == S_OK && event.calls == 2 && event.refs == 1 && stopped.calls == 3);
    check("reentrant Stop finishes after enumeration", w->lpVtbl->get_Status(w, &st) == S_OK && st == 4);
    release(w);
    check("watcher destruction releases remaining handlers", second.refs == 1 && stopped.refs == 1);

    // A blocked callback makes removal and Stop overlap deterministically.
    Event blocked = {.iface = {&event_vtbl}, .refs = 1};
    blocked.entered = CreateEventA(NULL, TRUE, FALSE, NULL);
    blocked.resume = CreateEventA(NULL, TRUE, FALSE, NULL);
    REQUIRE("create concurrency gates", blocked.entered && blocked.resume);
    w2->lpVtbl->add_EnumerationCompleted(w2, &blocked, &token);
    w2->lpVtbl->add_Stopped(w2, &stopped, &stopToken);
    w2->lpVtbl->AddRef(w2);
    HANDLE thread = CreateThread(NULL, 0, start_thread, w2, 0, NULL);
    REQUIRE("start watcher on worker thread", thread != NULL);
    REQUIRE("worker enters callback", WaitForSingleObject(blocked.entered, 10000) == WAIT_OBJECT_0);
    w2->lpVtbl->remove_EnumerationCompleted(w2, token);
    check("in-flight callback retains removed handler", blocked.refs == 2);
    LONG before = stopped.calls;
    check("concurrent Stop does not deadlock", w2->lpVtbl->Stop(w2) == S_OK);
    check("Stop waits for in-flight enumeration", w2->lpVtbl->get_Status(w2, &st) == S_OK && st == 3 && stopped.calls == before);
    check("Start while Stopping rejected", w2->lpVtbl->Start(w2) == E_ILLEGAL_METHOD_CALL);
    SetEvent(blocked.resume);
    REQUIRE("worker completes", WaitForSingleObject(thread, 10000) == WAIT_OBJECT_0);
    DWORD exitCode;
    GetExitCodeThread(thread, &exitCode);
    check("worker Start succeeds", exitCode == S_OK);
    check("Stopped delivered after callback drains", stopped.calls == before + 1 && stopped.status == 4);
    check("in-flight reference released", blocked.refs == 1);
    CloseHandle(thread); CloseHandle(blocked.entered); CloseHandle(blocked.resume);
    release(w2);
    check("stopped subscription released at destruction", stopped.refs == 1);
    if (reentrant_refs(cam, di)) return 1;
    release(cam); release(di); release(factories[0]); release(factories[1]);
    if (dll) FreeLibrary(dll);
    printf("%u passed, %u failed\n", passed, failed);
    return failed ? 1 : 0;
}

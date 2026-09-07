// WinRT camera stub for Wine: answers MediaFrameSourceGroup::FindAllAsync
// with an empty list so apps that enumerate cameras at startup (as LightBurn
// 2.x does, blocking on the C++/WinRT .get()) keep going instead of dying on
// REGDB_E_CLASSNOTREG. No real camera: Size is always 0.
//
// Register via:
//   HKLM\Software\Microsoft\WindowsRuntime\ActivatableClassId
//     Windows.Media.Capture.Frames.MediaFrameSourceGroup
//     DllPath = <path to this DLL>
//
// Build with: build.sh (llvm-mingw, x86_64)
#define CONST_VTABLE
#include <windows.h>
#include <activation.h>
#include <winstring.h>
#include <wchar.h>

// These public Winerror.h values are absent in some MinGW header releases.
#ifndef E_ILLEGAL_METHOD_CALL
#define E_ILLEGAL_METHOD_CALL ((HRESULT)0x8000000EL)
#endif
#ifndef E_ILLEGAL_DELEGATE_ASSIGNMENT
#define E_ILLEGAL_DELEGATE_ASSIGNMENT ((HRESULT)0x80000018L)
#endif

typedef HSTRING HSTRING_F;

// ABI and IIDs from Microsoft's public WinSDK headers in microsoft/win32metadata,
// generation/WinSDK/RecompiledIdlHeaders/winrt/{asyncinfo,windows.media.capture.frames,
// windows.devices.enumeration}.h. No application-private interfaces are used.
static const GUID IID_IUnknown_ = {0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
static const GUID IID_IInspectable_ = {0xAF86E2E0,0xB12D,0x4C6A,{0x9C,0x5A,0xD7,0xAA,0x65,0x10,0x1E,0x90}};
static const GUID IID_IActivationFactory_ = {0x00000035,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
static const GUID IID_IAsyncInfo_ = {0x00000036,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
// IAgileObject: answer S_OK so C++/WinRT skips cross-thread marshaling
// (Wine has no RoGetAgileReference). No methods beyond IUnknown.
static const GUID IID_IAgileObject_ = {0x94EA2B94,0xE9CC,0x49E0,{0xC0,0xFF,0xEE,0x64,0xCA,0x8F,0x5B,0x90}};
// IMediaFrameSourceGroupStatics
static const GUID IID_MFSGStatics_ = {0x1C48BFC5,0x436F,0x4508,{0x94,0xCF,0xD5,0xD8,0xB7,0x32,0x64,0x45}};
static const GUID IID_MFSGVector_ = {0xd01148ae,0xcccd,0x56eb,{0xb2,0xb4,0xa7,0xd2,0xac,0xce,0x14,0xec}};
static const GUID IID_MFSGAsync_ = {0xa795889f,0x6d49,0x5687,{0xaa,0xbe,0xf2,0xfc,0x62,0x37,0xfa,0x1a}};
static const GUID IID_DIWatcher_ = {0xc9eab97d,0x8f6b,0x4f96,{0xa9,0xf4,0xab,0xc8,0x14,0xe2,0x22,0x71}};

typedef enum { AsyncStatus_Started = 0, AsyncStatus_Completed = 1 } AsyncStatus;

static int guid_eq(REFGUID a, REFGUID b) {
    return !memcmp(a, b, sizeof(GUID));
}
static HRESULT copy_iids(const GUID *ids, ULONG count, ULONG *n, GUID **out) {
    if (n) *n = 0;
    if (out) *out = NULL;
    if (!n || !out) return E_POINTER;
    *out = CoTaskMemAlloc(count * sizeof(GUID));
    if (!*out) return E_OUTOFMEMORY;
    memcpy(*out, ids, count * sizeof(GUID));
    *n = count;
    return S_OK;
}

// ---------- IVectorView (empty) ----------
typedef struct EmptyVector EmptyVector;
typedef struct {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(EmptyVector*, REFGUID, void**);
    ULONG (STDMETHODCALLTYPE *AddRef)(EmptyVector*);
    ULONG (STDMETHODCALLTYPE *Release)(EmptyVector*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(EmptyVector*, ULONG*, GUID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(EmptyVector*, void**);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(EmptyVector*, int*);
    HRESULT (STDMETHODCALLTYPE *GetAt)(EmptyVector*, UINT32, void**);
    HRESULT (STDMETHODCALLTYPE *get_Size)(EmptyVector*, UINT32*);
    HRESULT (STDMETHODCALLTYPE *IndexOf)(EmptyVector*, void*, UINT32*, BOOLEAN*);
    HRESULT (STDMETHODCALLTYPE *GetMany)(EmptyVector*, UINT32, UINT32, void**, UINT32*);
} EmptyVectorVtbl;
struct EmptyVector { const EmptyVectorVtbl *lpVtbl; LONG refs; };

static HRESULT STDMETHODCALLTYPE Vec_QI(EmptyVector *s, REFGUID id, void **o) {
    if (!o) return E_POINTER;
    if (guid_eq(id, &IID_IUnknown_) || guid_eq(id, &IID_IInspectable_) || guid_eq(id, &IID_MFSGVector_)) {
        *o = s; InterlockedIncrement(&s->refs); return S_OK;
    }
    *o = NULL; return E_NOINTERFACE;
}
static ULONG STDMETHODCALLTYPE Vec_Add(EmptyVector *s) { return InterlockedIncrement(&s->refs); }
static ULONG STDMETHODCALLTYPE Vec_Rel(EmptyVector *s) { return InterlockedDecrement(&s->refs); }
static HRESULT STDMETHODCALLTYPE Vec_Iids(EmptyVector *s, ULONG *n, GUID **i) {
    (void)s; return copy_iids(&IID_MFSGVector_, 1, n, i);
}
static HRESULT STDMETHODCALLTYPE Vec_NoName(EmptyVector *s, void **n) { (void)s; *n = NULL; return S_OK; }
static HRESULT STDMETHODCALLTYPE Vec_Trust(EmptyVector *s, int *t) { (void)s; *t = 0; return S_OK; }
static HRESULT STDMETHODCALLTYPE Vec_GetAt(EmptyVector *s, UINT32 i, void **o) { (void)s; (void)i; *o = NULL; return 0x8000000BL; }
static HRESULT STDMETHODCALLTYPE Vec_Size(EmptyVector *s, UINT32 *n) { (void)s; *n = 0; return S_OK; }
static HRESULT STDMETHODCALLTYPE Vec_IndexOf(EmptyVector *s, void *v, UINT32 *ix, BOOLEAN *f) {
    (void)s; (void)v; *ix = 0; *f = FALSE; return S_OK;
}
static HRESULT STDMETHODCALLTYPE Vec_GetMany(EmptyVector *s, UINT32 st, UINT32 cap, void **items, UINT32 *n) {
    (void)s; (void)st; (void)cap; (void)items; *n = 0; return S_OK;
}
static const EmptyVectorVtbl g_vecVtbl = {
    Vec_QI, Vec_Add, Vec_Rel, Vec_Iids, Vec_NoName, Vec_Trust,
    Vec_GetAt, Vec_Size, Vec_IndexOf, Vec_GetMany
};
static EmptyVector g_vector = { &g_vecVtbl, 1 };

// ---------- Already-completed IAsyncOperation ----------
typedef struct CompletedAsync CompletedAsync;
typedef struct CompletedHandler CompletedHandler;
typedef struct {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(CompletedHandler*, REFGUID, void**);
    ULONG (STDMETHODCALLTYPE *AddRef)(CompletedHandler*);
    ULONG (STDMETHODCALLTYPE *Release)(CompletedHandler*);
    HRESULT (STDMETHODCALLTYPE *Invoke)(CompletedHandler*, CompletedAsync*, AsyncStatus);
} CompletedHandlerVtbl;
struct CompletedHandler { const CompletedHandlerVtbl *lpVtbl; };
typedef struct {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(CompletedAsync*, REFGUID, void**);
    ULONG (STDMETHODCALLTYPE *AddRef)(CompletedAsync*);
    ULONG (STDMETHODCALLTYPE *Release)(CompletedAsync*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(CompletedAsync*, ULONG*, GUID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(CompletedAsync*, void**);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(CompletedAsync*, int*);
    HRESULT (STDMETHODCALLTYPE *put_Completed)(CompletedAsync*, CompletedHandler*);
    HRESULT (STDMETHODCALLTYPE *get_Completed)(CompletedAsync*, CompletedHandler**);
    HRESULT (STDMETHODCALLTYPE *GetResults)(CompletedAsync*, EmptyVector**);
} CompletedAsyncVtbl;
typedef struct AsyncInfo AsyncInfo;
typedef struct {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(AsyncInfo*, REFGUID, void**);
    ULONG (STDMETHODCALLTYPE *AddRef)(AsyncInfo*);
    ULONG (STDMETHODCALLTYPE *Release)(AsyncInfo*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(AsyncInfo*, ULONG*, GUID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(AsyncInfo*, void**);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(AsyncInfo*, int*);
    HRESULT (STDMETHODCALLTYPE *get_Id)(AsyncInfo*, UINT32*);
    HRESULT (STDMETHODCALLTYPE *get_Status)(AsyncInfo*, AsyncStatus*);
    HRESULT (STDMETHODCALLTYPE *get_ErrorCode)(AsyncInfo*, HRESULT*);
    HRESULT (STDMETHODCALLTYPE *Cancel)(AsyncInfo*);
    HRESULT (STDMETHODCALLTYPE *Close)(AsyncInfo*);
} AsyncInfoVtbl;
struct AsyncInfo { const AsyncInfoVtbl *lpVtbl; CompletedAsync *owner; };
struct CompletedAsync {
    const CompletedAsyncVtbl *lpVtbl;
    LONG refs;
    AsyncInfo info;
    SRWLOCK lock;
    CompletedHandler *handler;
};

static HRESULT STDMETHODCALLTYPE Async_QI(CompletedAsync *s, REFGUID id, void **o) {
    if (!o) return E_POINTER;
    *o = NULL;
    if (guid_eq(id, &IID_IAsyncInfo_)) *o = &s->info;
    else if (guid_eq(id, &IID_IUnknown_) || guid_eq(id, &IID_IInspectable_) || guid_eq(id, &IID_MFSGAsync_)) *o = s;
    if (!*o) return E_NOINTERFACE;
    InterlockedIncrement(&s->refs); return S_OK;
}
static ULONG STDMETHODCALLTYPE Async_Add(CompletedAsync *s) { return InterlockedIncrement(&s->refs); }
static ULONG STDMETHODCALLTYPE Async_Rel(CompletedAsync *s) {
    ULONG refs = InterlockedDecrement(&s->refs);
    if (!refs) {
        if (s->handler) s->handler->lpVtbl->Release(s->handler);
        HeapFree(GetProcessHeap(), 0, s);
    }
    return refs;
}
static HRESULT STDMETHODCALLTYPE Async_Iids(CompletedAsync *s, ULONG *n, GUID **i) {
    const GUID ids[] = {IID_MFSGAsync_, IID_IAsyncInfo_};
    (void)s; return copy_iids(ids, 2, n, i);
}
static HRESULT STDMETHODCALLTYPE Async_NoName(CompletedAsync *s, void **n) { (void)s; *n = NULL; return S_OK; }
static HRESULT STDMETHODCALLTYPE Async_Trust(CompletedAsync *s, int *t) { (void)s; *t = 0; return S_OK; }
static HRESULT STDMETHODCALLTYPE Async_put_Completed(CompletedAsync *s, CompletedHandler *h) {
    if (!h) return E_POINTER;
    h->lpVtbl->AddRef(h);
    AcquireSRWLockExclusive(&s->lock);
    if (s->handler) {
        ReleaseSRWLockExclusive(&s->lock);
        h->lpVtbl->Release(h);
        return E_ILLEGAL_DELEGATE_ASSIGNMENT;
    }
    s->handler = h;
    ReleaseSRWLockExclusive(&s->lock);
    // Keep the operation alive even if the inline callback releases its caller's reference.
    Async_Add(s);
    h->lpVtbl->Invoke(h, s, AsyncStatus_Completed);
    Async_Rel(s);
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE Async_get_Completed(CompletedAsync *s, CompletedHandler **h) {
    if (!h) return E_POINTER;
    Async_Add(s);
    AcquireSRWLockShared(&s->lock);
    *h = s->handler;
    ReleaseSRWLockShared(&s->lock);
    // The handler is immutable after assignment and owned until the operation dies.
    if (*h) (*h)->lpVtbl->AddRef(*h);
    Async_Rel(s);
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE Async_GetResults(CompletedAsync *s, EmptyVector **o) {
    (void)s; *o = &g_vector; InterlockedIncrement(&g_vector.refs); return S_OK;
}
static HRESULT STDMETHODCALLTYPE Info_QI(AsyncInfo *s, REFGUID id, void **o) { return Async_QI(s->owner, id, o); }
static ULONG STDMETHODCALLTYPE Info_Add(AsyncInfo *s) { return Async_Add(s->owner); }
static ULONG STDMETHODCALLTYPE Info_Rel(AsyncInfo *s) { return Async_Rel(s->owner); }
static HRESULT STDMETHODCALLTYPE Info_Iids(AsyncInfo *s, ULONG *n, GUID **i) { return Async_Iids(s->owner, n, i); }
static HRESULT STDMETHODCALLTYPE Info_NoName(AsyncInfo *s, void **n) { return Async_NoName(s->owner, n); }
static HRESULT STDMETHODCALLTYPE Info_Trust(AsyncInfo *s, int *t) { return Async_Trust(s->owner, t); }
static HRESULT STDMETHODCALLTYPE Async_Id(AsyncInfo *s, UINT32 *i) { (void)s; *i = 1; return S_OK; }
static HRESULT STDMETHODCALLTYPE Async_Status(AsyncInfo *s, AsyncStatus *st) {
    (void)s; *st = AsyncStatus_Completed; return S_OK;
}
static HRESULT STDMETHODCALLTYPE Async_Error(AsyncInfo *s, HRESULT *e) { (void)s; *e = S_OK; return S_OK; }
static HRESULT STDMETHODCALLTYPE Async_Cancel(AsyncInfo *s) { (void)s; return S_OK; }
static HRESULT STDMETHODCALLTYPE Async_Close(AsyncInfo *s) { (void)s; return S_OK; }
static const CompletedAsyncVtbl g_asyncVtbl = {
    Async_QI, Async_Add, Async_Rel, Async_Iids, Async_NoName, Async_Trust,
    Async_put_Completed, Async_get_Completed, Async_GetResults
};
static const AsyncInfoVtbl g_infoVtbl = {
    Info_QI, Info_Add, Info_Rel, Info_Iids, Info_NoName, Info_Trust,
    Async_Id, Async_Status, Async_Error, Async_Cancel, Async_Close
};

// Static-only classes still expose a real IActivationFactory, not their statics vtable.
typedef struct { IActivationFactory iface; IUnknown *statics; } Activation;
static HRESULT STDMETHODCALLTYPE Act_QI(IActivationFactory *s, REFIID id, void **o) {
    IUnknown *p = ((Activation*)s)->statics;
    return p->lpVtbl->QueryInterface(p, id, o);
}
static ULONG STDMETHODCALLTYPE Act_Add(IActivationFactory *s) {
    IUnknown *p = ((Activation*)s)->statics; return p->lpVtbl->AddRef(p);
}
static ULONG STDMETHODCALLTYPE Act_Rel(IActivationFactory *s) {
    IUnknown *p = ((Activation*)s)->statics; return p->lpVtbl->Release(p);
}
static HRESULT STDMETHODCALLTYPE Act_Iids(IActivationFactory *s, ULONG *n, IID **i) {
    IInspectable *p = (IInspectable*)((Activation*)s)->statics;
    return p->lpVtbl->GetIids(p, n, i);
}
static HRESULT STDMETHODCALLTYPE Act_Name(IActivationFactory *s, HSTRING *n) { (void)s; *n = NULL; return S_OK; }
static HRESULT STDMETHODCALLTYPE Act_Trust(IActivationFactory *s, TrustLevel *t) { (void)s; *t = BaseTrust; return S_OK; }
static HRESULT STDMETHODCALLTYPE Act_Create(IActivationFactory *s, IInspectable **o) {
    (void)s; if (!o) return E_POINTER; *o = NULL; return E_NOTIMPL;
}
static const IActivationFactoryVtbl g_actVtbl = { Act_QI, Act_Add, Act_Rel, Act_Iids, Act_Name, Act_Trust, Act_Create };
static Activation g_camActivation, g_diActivation;

// ---------- Activation factory (IMediaFrameSourceGroupStatics) ----------
typedef struct CamFactory CamFactory;
typedef struct {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(CamFactory*, REFGUID, void**);
    ULONG (STDMETHODCALLTYPE *AddRef)(CamFactory*);
    ULONG (STDMETHODCALLTYPE *Release)(CamFactory*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(CamFactory*, ULONG*, GUID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(CamFactory*, void**);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(CamFactory*, int*);
    HRESULT (STDMETHODCALLTYPE *FindAllAsync)(CamFactory*, CompletedAsync**);
    HRESULT (STDMETHODCALLTYPE *FromIdAsync)(CamFactory*, HSTRING_F, void**);
    HRESULT (STDMETHODCALLTYPE *GetDeviceSelector)(CamFactory*, HSTRING_F*);
} CamFactoryVtbl;
struct CamFactory { const CamFactoryVtbl *lpVtbl; LONG refs; };

static void ods(const char *m) {
    OutputDebugStringA(m);
    HANDLE f = CreateFileA("C:\\windows\\temp\\winrtstub.log", FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f != INVALID_HANDLE_VALUE) {
        DWORD w = 0; WriteFile(f, m, (DWORD)strlen(m), &w, NULL);
        WriteFile(f, "\r\n", 2, &w, NULL); CloseHandle(f);
    }
}

static HRESULT STDMETHODCALLTYPE Fac_QI(CamFactory *s, REFGUID id, void **o) {
    if (!o) return E_POINTER;
    if (guid_eq(id, &IID_IActivationFactory_)) {
        *o = &g_camActivation; InterlockedIncrement(&s->refs); return S_OK;
    }
    char b[128];
    wsprintfA(b, "STUB Fac_QI %08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        id->Data1, id->Data2, id->Data3,
        id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
        id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7]);
    ods(b);
    if (guid_eq(id, &IID_IUnknown_) || guid_eq(id, &IID_IInspectable_) ||
        guid_eq(id, &IID_MFSGStatics_) ||
        guid_eq(id, &IID_IAgileObject_)) {
        *o = s; InterlockedIncrement(&s->refs); return S_OK;
    }
    *o = NULL; return E_NOINTERFACE;
}
static ULONG STDMETHODCALLTYPE Fac_Add(CamFactory *s) { return InterlockedIncrement(&s->refs); }
static ULONG STDMETHODCALLTYPE Fac_Rel(CamFactory *s) { return InterlockedDecrement(&s->refs); }
static HRESULT STDMETHODCALLTYPE Fac_Iids(CamFactory *s, ULONG *n, GUID **i) {
    const GUID ids[] = {IID_IActivationFactory_, IID_MFSGStatics_};
    (void)s; return copy_iids(ids, 2, n, i);
}
static HRESULT STDMETHODCALLTYPE Fac_NoName(CamFactory *s, void **n) { (void)s; *n = NULL; return S_OK; }
static HRESULT STDMETHODCALLTYPE Fac_Trust(CamFactory *s, int *t) { (void)s; *t = 0; return S_OK; }
static HRESULT STDMETHODCALLTYPE Fac_FindAllAsync(CamFactory *s, CompletedAsync **o) {
    ods("STUB FindAllAsync -> empty");
    (void)s;
    if (!o) return E_POINTER;
    *o = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(**o));
    if (!*o) return E_OUTOFMEMORY;
    (*o)->lpVtbl = &g_asyncVtbl;
    (*o)->refs = 1;
    (*o)->info.lpVtbl = &g_infoVtbl;
    (*o)->info.owner = *o;
    InitializeSRWLock(&(*o)->lock);
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE Fac_FromIdAsync(CamFactory *s, HSTRING_F id, void **o) {
    (void)s; (void)id; if (!o) return E_POINTER; *o = NULL;
    ods("STUB FromIdAsync -> E_NOTIMPL (no saved camera id, should not happen)");
    return 0x80004001L;
}
static HRESULT STDMETHODCALLTYPE Fac_GetDeviceSelector(CamFactory *s, HSTRING_F *sel) {
    HSTRING_F h = NULL;
    (void)s;
    // Empty selector: no media frame source groups on this machine.
    WindowsCreateString(L"", 0, &h);
    ods("STUB GetDeviceSelector -> empty HSTRING");
    *sel = h; return S_OK;
}
static const CamFactoryVtbl g_facVtbl = {
    Fac_QI, Fac_Add, Fac_Rel, Fac_Iids, Fac_NoName, Fac_Trust,
    Fac_FindAllAsync, Fac_FromIdAsync, Fac_GetDeviceSelector
};
static CamFactory g_factory = { &g_facVtbl, 1 };
static Activation g_camActivation = { { &g_actVtbl }, (IUnknown*)&g_factory };

// ---------- DeviceInformation factory + DeviceWatcher (all empty) ----------
typedef struct DIWatcher DIWatcher;
typedef struct {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(DIWatcher*, REFGUID, void**);
    ULONG (STDMETHODCALLTYPE *AddRef)(DIWatcher*);
    ULONG (STDMETHODCALLTYPE *Release)(DIWatcher*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(DIWatcher*, ULONG*, GUID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(DIWatcher*, void**);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(DIWatcher*, int*);
    HRESULT (STDMETHODCALLTYPE *add_Added)(DIWatcher*, void*, INT64*);
    HRESULT (STDMETHODCALLTYPE *remove_Added)(DIWatcher*, INT64);
    HRESULT (STDMETHODCALLTYPE *add_Updated)(DIWatcher*, void*, INT64*);
    HRESULT (STDMETHODCALLTYPE *remove_Updated)(DIWatcher*, INT64);
    HRESULT (STDMETHODCALLTYPE *add_Removed)(DIWatcher*, void*, INT64*);
    HRESULT (STDMETHODCALLTYPE *remove_Removed)(DIWatcher*, INT64);
    HRESULT (STDMETHODCALLTYPE *add_EnumerationCompleted)(DIWatcher*, void*, INT64*);
    HRESULT (STDMETHODCALLTYPE *remove_EnumerationCompleted)(DIWatcher*, INT64);
    HRESULT (STDMETHODCALLTYPE *add_Stopped)(DIWatcher*, void*, INT64*);
    HRESULT (STDMETHODCALLTYPE *remove_Stopped)(DIWatcher*, INT64);
    HRESULT (STDMETHODCALLTYPE *get_Status)(DIWatcher*, int*);
    HRESULT (STDMETHODCALLTYPE *Start)(DIWatcher*);
    HRESULT (STDMETHODCALLTYPE *Stop)(DIWatcher*);
} DIWatcherVtbl;
typedef struct WatchHandler WatchHandler;
typedef struct {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(WatchHandler*, REFGUID, void**);
    ULONG (STDMETHODCALLTYPE *AddRef)(WatchHandler*);
    ULONG (STDMETHODCALLTYPE *Release)(WatchHandler*);
    HRESULT (STDMETHODCALLTYPE *Invoke)(WatchHandler*, DIWatcher*, void*);
} WatchHandlerVtbl;
struct WatchHandler { const WatchHandlerVtbl *lpVtbl; };
typedef enum { WatchAdded, WatchUpdated, WatchRemoved, WatchCompleted, WatchStopped } WatchEvent;
typedef enum { WatchCreated, WatchStarted, WatchEnumerationCompleted, WatchStopping, WatchIsStopped } WatchStatus;
typedef struct WatchRegistration {
    LONG refs;
    struct WatchRegistration *next;
    WatchHandler *handler;
    WatchEvent event;
    INT64 token;
} WatchRegistration;
typedef struct { WatchRegistration **registrations; size_t count; } WatchDelivery;
struct DIWatcher {
    const DIWatcherVtbl *lpVtbl;
    LONG refs;
    SRWLOCK lock;
    WatchRegistration *handlers;
    INT64 nextToken;
    WatchStatus status;
    BOOL dispatching;
    WatchDelivery stopped;
};

static HRESULT STDMETHODCALLTYPE W_QI(DIWatcher *s, REFGUID id, void **o) {
    if (!o) return E_POINTER;
    char b[128];
    wsprintfA(b, "STUB Watcher_QI %08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        id->Data1, id->Data2, id->Data3,
        id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
        id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7]);
    ods(b);
    if (guid_eq(id, &IID_IUnknown_) || guid_eq(id, &IID_IInspectable_) || guid_eq(id, &IID_IAgileObject_) || guid_eq(id, &IID_DIWatcher_)) {
        *o = s; InterlockedIncrement(&s->refs); return S_OK;
    }
    *o = NULL; return E_NOINTERFACE;
}
static ULONG STDMETHODCALLTYPE W_Add(DIWatcher *s) { return InterlockedIncrement(&s->refs); }
static void W_releaseRegistration(WatchRegistration *r) {
    if (!InterlockedDecrement(&r->refs)) {
        r->handler->lpVtbl->Release(r->handler);
        HeapFree(GetProcessHeap(), 0, r);
    }
}
static ULONG STDMETHODCALLTYPE W_Rel(DIWatcher *s) {
    ULONG refs = InterlockedDecrement(&s->refs);
    if (!refs) {
        WatchRegistration *r = s->handlers;
        while (r) {
            WatchRegistration *next = r->next;
            W_releaseRegistration(r);
            r = next;
        }
        HeapFree(GetProcessHeap(), 0, s);
    }
    return refs;
}
static HRESULT STDMETHODCALLTYPE W_Iids(DIWatcher *s, ULONG *n, GUID **i) {
    (void)s; return copy_iids(&IID_DIWatcher_, 1, n, i);
}
static HRESULT STDMETHODCALLTYPE W_NoName(DIWatcher *s, void **n) { (void)s; *n = NULL; return S_OK; }
static HRESULT STDMETHODCALLTYPE W_Trust(DIWatcher *s, int *t) { (void)s; *t = 0; return S_OK; }
static HRESULT W_register(DIWatcher *s, void *handler, INT64 *tok, WatchEvent event) {
    if (!tok) return E_POINTER;
    *tok = 0;
    if (!handler) return E_POINTER;
    WatchRegistration *r = HeapAlloc(GetProcessHeap(), 0, sizeof(*r));
    if (!r) return E_OUTOFMEMORY;
    r->refs = 1;
    r->handler = handler;
    r->event = event;
    r->handler->lpVtbl->AddRef(r->handler);
    AcquireSRWLockExclusive(&s->lock);
    if (s->nextToken == MAXLONGLONG) {
        ReleaseSRWLockExclusive(&s->lock);
        W_releaseRegistration(r);
        return E_OUTOFMEMORY;
    }
    *tok = r->token = ++s->nextToken;
    r->next = s->handlers;
    s->handlers = r;
    ReleaseSRWLockExclusive(&s->lock);
    return S_OK;
}
static HRESULT W_unregister(DIWatcher *s, INT64 tok, WatchEvent event) {
    AcquireSRWLockExclusive(&s->lock);
    WatchRegistration **p = &s->handlers, *r;
    while (*p && ((*p)->token != tok || (*p)->event != event)) p = &(*p)->next;
    r = *p;
    if (r) *p = r->next;
    ReleaseSRWLockExclusive(&s->lock);
    if (r) W_releaseRegistration(r);
    return S_OK;
}
#define WATCH_EVENT(name, event) \
static HRESULT STDMETHODCALLTYPE W_add##name(DIWatcher *s, void *h, INT64 *t) { return W_register(s, h, t, event); } \
static HRESULT STDMETHODCALLTYPE W_remove##name(DIWatcher *s, INT64 t) { return W_unregister(s, t, event); }
WATCH_EVENT(Added, WatchAdded)
WATCH_EVENT(Updated, WatchUpdated)
WATCH_EVENT(Removed, WatchRemoved)
WATCH_EVENT(Completed, WatchCompleted)
WATCH_EVENT(Stopped, WatchStopped)
#undef WATCH_EVENT

// Pin only internal registrations under the lock. Delegate methods can reenter us.
static HRESULT W_snapshot(DIWatcher *s, WatchEvent event, WatchDelivery *d) {
    *d = (WatchDelivery){0};
    for (WatchRegistration *r = s->handlers; r; r = r->next)
        if (r->event == event) d->count++;
    if (!d->count) return S_OK;
    if (d->count > (size_t)-1 / sizeof(*d->registrations)) return E_OUTOFMEMORY;
    d->registrations = HeapAlloc(GetProcessHeap(), 0, d->count * sizeof(*d->registrations));
    if (!d->registrations) return E_OUTOFMEMORY;
    size_t i = d->count;
    for (WatchRegistration *r = s->handlers; r; r = r->next) {
        if (r->event != event) continue;
        InterlockedIncrement(&r->refs);
        d->registrations[--i] = r;
    }
    return S_OK;
}
static void W_deliver(DIWatcher *s, WatchDelivery d) {
    for (size_t i = 0; i < d.count; i++) {
        WatchHandler *h = d.registrations[i]->handler;
        h->lpVtbl->AddRef(h);
        W_releaseRegistration(d.registrations[i]);
        h->lpVtbl->Invoke(h, s, NULL);
        h->lpVtbl->Release(h);
    }
    if (d.registrations) HeapFree(GetProcessHeap(), 0, d.registrations);
}
static HRESULT STDMETHODCALLTYPE W_Status(DIWatcher *s, int *st) {
    if (!st) return E_POINTER;
    AcquireSRWLockShared(&s->lock);
    *st = s->status;
    ReleaseSRWLockShared(&s->lock);
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE W_Start(DIWatcher *s) {
    ods("STUB Watcher Start -> fire EnumerationCompleted (0 devices)");
    WatchDelivery d;
    AcquireSRWLockExclusive(&s->lock);
    if (s->dispatching || (s->status != WatchCreated && s->status != WatchIsStopped)) {
        ReleaseSRWLockExclusive(&s->lock);
        return E_ILLEGAL_METHOD_CALL;
    }
    HRESULT hr = W_snapshot(s, WatchCompleted, &d);
    if (FAILED(hr)) { ReleaseSRWLockExclusive(&s->lock); return hr; }
    s->status = WatchEnumerationCompleted;
    s->dispatching = TRUE;
    W_Add(s);
    ReleaseSRWLockExclusive(&s->lock);
    W_deliver(s, d);
    AcquireSRWLockExclusive(&s->lock);
    BOOL stopped = s->status == WatchStopping;
    if (stopped) {
        s->status = WatchIsStopped;
        d = s->stopped;
        s->stopped = (WatchDelivery){0};
    }
    s->dispatching = FALSE;
    ReleaseSRWLockExclusive(&s->lock);
    if (stopped) W_deliver(s, d);
    W_Rel(s);
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE W_Stop(DIWatcher *s) {
    WatchDelivery d;
    AcquireSRWLockExclusive(&s->lock);
    if (s->status == WatchStopping || s->status == WatchIsStopped) {
        ReleaseSRWLockExclusive(&s->lock); return S_OK;
    }
    if (s->status != WatchEnumerationCompleted && s->status != WatchStarted) {
        ReleaseSRWLockExclusive(&s->lock); return E_ILLEGAL_METHOD_CALL;
    }
    HRESULT hr = W_snapshot(s, WatchStopped, &d);
    if (FAILED(hr)) { ReleaseSRWLockExclusive(&s->lock); return hr; }
    // Stop during an enumeration callback completes only after that delivery returns.
    if (s->dispatching) {
        s->status = WatchStopping;
        s->stopped = d;
        ReleaseSRWLockExclusive(&s->lock); return S_OK;
    }
    s->status = WatchIsStopped;
    W_Add(s);
    ReleaseSRWLockExclusive(&s->lock);
    W_deliver(s, d);
    W_Rel(s);
    return S_OK;
}
static const DIWatcherVtbl g_watchVtbl = {
    W_QI, W_Add, W_Rel, W_Iids, W_NoName, W_Trust,
    W_addAdded, W_removeAdded, W_addUpdated, W_removeUpdated, W_addRemoved, W_removeRemoved,
    W_addCompleted, W_removeCompleted,
    W_addStopped, W_removeStopped,
    W_Status, W_Start, W_Stop                            // 16,17,18
};

typedef struct DIFactory DIFactory;
typedef struct {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(DIFactory*, REFGUID, void**);
    ULONG (STDMETHODCALLTYPE *AddRef)(DIFactory*);
    ULONG (STDMETHODCALLTYPE *Release)(DIFactory*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(DIFactory*, ULONG*, GUID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(DIFactory*, void**);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(DIFactory*, int*);
    HRESULT (STDMETHODCALLTYPE *CreateFromIdAsync)(DIFactory*, HSTRING_F, void**);
    HRESULT (STDMETHODCALLTYPE *CreateFromIdAsyncAdditionalProperties)(DIFactory*, HSTRING_F, void*, void**);
    HRESULT (STDMETHODCALLTYPE *FindAllAsync)(DIFactory*, void**);
    HRESULT (STDMETHODCALLTYPE *FindAllAsyncDeviceClass)(DIFactory*, int, void**);
    HRESULT (STDMETHODCALLTYPE *FindAllAsyncAqsFilter)(DIFactory*, HSTRING_F, void**);
    HRESULT (STDMETHODCALLTYPE *FindAllAsyncAqsFilterAndAdditionalProperties)(DIFactory*, HSTRING_F, void*, void**);
    HRESULT (STDMETHODCALLTYPE *CreateWatcher)(DIFactory*, DIWatcher**);
    HRESULT (STDMETHODCALLTYPE *CreateWatcherDeviceClass)(DIFactory*, int, DIWatcher**);
    HRESULT (STDMETHODCALLTYPE *CreateWatcherAqsFilter)(DIFactory*, HSTRING_F, DIWatcher**);
    HRESULT (STDMETHODCALLTYPE *CreateWatcherAqsFilterAndAdditionalProperties)(DIFactory*, HSTRING_F, void*, DIWatcher**);
} DIFactoryVtbl;
struct DIFactory { const DIFactoryVtbl *lpVtbl; LONG refs; };
// IDeviceInformationStatics
static const GUID IID_DIStatics_ = {0xC17F100E,0x3A46,0x4A78,{0x80,0x13,0x76,0x9D,0xC9,0xB9,0x73,0x90}};
static HRESULT STDMETHODCALLTYPE DI_QI(DIFactory *s, REFGUID id, void **o) {
    if (!o) return E_POINTER;
    if (guid_eq(id, &IID_IActivationFactory_)) {
        *o = &g_diActivation; InterlockedIncrement(&s->refs); return S_OK;
    }
    char b[128];
    wsprintfA(b, "STUB DI_QI %08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        id->Data1, id->Data2, id->Data3,
        id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
        id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7]);
    ods(b);
    if (guid_eq(id, &IID_IUnknown_) || guid_eq(id, &IID_IInspectable_) ||
        guid_eq(id, &IID_IAgileObject_) ||
        guid_eq(id, &IID_DIStatics_)) {
        *o = s; InterlockedIncrement(&s->refs); return S_OK;
    }
    *o = NULL; return E_NOINTERFACE;
}
static ULONG STDMETHODCALLTYPE DI_Add(DIFactory *s) { return InterlockedIncrement(&s->refs); }
static ULONG STDMETHODCALLTYPE DI_Rel(DIFactory *s) { return InterlockedDecrement(&s->refs); }
static HRESULT STDMETHODCALLTYPE DI_Iids(DIFactory *s, ULONG *n, GUID **i) {
    const GUID ids[] = {IID_IActivationFactory_, IID_DIStatics_};
    (void)s; return copy_iids(ids, 2, n, i);
}
static HRESULT STDMETHODCALLTYPE DI_NoName(DIFactory *s, void **n) { (void)s; *n = NULL; return S_OK; }
static HRESULT STDMETHODCALLTYPE DI_Trust(DIFactory *s, int *t) { (void)s; *t = 0; return S_OK; }
static HRESULT STDMETHODCALLTYPE DI_Unsupported(DIFactory *s, void **o) {
    (void)s; if (!o) return E_POINTER; *o = NULL; return E_NOTIMPL;
}
static HRESULT STDMETHODCALLTYPE DI_UnsupportedString(DIFactory *s, HSTRING_F a, void **o) {
    (void)a; return DI_Unsupported(s, o);
}
static HRESULT STDMETHODCALLTYPE DI_UnsupportedProperties(DIFactory *s, HSTRING_F a, void *b, void **o) {
    (void)a; (void)b; return DI_Unsupported(s, o);
}
static HRESULT STDMETHODCALLTYPE DI_UnsupportedClass(DIFactory *s, int a, void **o) {
    (void)a; return DI_Unsupported(s, o);
}
static HRESULT STDMETHODCALLTYPE DI_UnsupportedWatcher(DIFactory *s, DIWatcher **o) {
    (void)s; if (!o) return E_POINTER; *o = NULL; return E_NOTIMPL;
}
static HRESULT STDMETHODCALLTYPE DI_UnsupportedWatcherClass(DIFactory *s, int a, DIWatcher **o) {
    (void)a; return DI_UnsupportedWatcher(s, o);
}
static HRESULT STDMETHODCALLTYPE DI_UnsupportedWatcherProperties(DIFactory *s, HSTRING_F a, void *b, DIWatcher **o) {
    (void)a; (void)b; return DI_UnsupportedWatcher(s, o);
}
static HRESULT STDMETHODCALLTYPE DI_CreateWatcher(DIFactory *s, HSTRING_F selector, DIWatcher **w) {
    (void)s; (void)selector;
    ods("STUB DI CreateWatcher(String) -> empty watcher");
    if (!w) return E_POINTER;
    *w = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(**w));
    if (!*w) return E_OUTOFMEMORY;
    (*w)->lpVtbl = &g_watchVtbl;
    (*w)->refs = 1;
    InitializeSRWLock(&(*w)->lock);
    return S_OK;
}
static const DIFactoryVtbl g_diVtbl = {
    DI_QI, DI_Add, DI_Rel, DI_Iids, DI_NoName, DI_Trust,
    DI_UnsupportedString, DI_UnsupportedProperties, DI_Unsupported, DI_UnsupportedClass,
    DI_UnsupportedString, DI_UnsupportedProperties, DI_UnsupportedWatcher,
    DI_UnsupportedWatcherClass, DI_CreateWatcher, DI_UnsupportedWatcherProperties
};
static DIFactory g_difactory = { &g_diVtbl, 1 };
static Activation g_diActivation = { { &g_actVtbl }, (IUnknown*)&g_difactory };

static int hstring_equals(void *hs, const wchar_t *name) {
    UINT32 len = 0;
    const wchar_t *buf = WindowsGetStringRawBuffer(hs, &len);
    size_t n = wcslen(name);
    return buf && len == n && !wmemcmp(buf, name, n);
}

__declspec(dllexport) HRESULT STDMETHODCALLTYPE DllGetActivationFactory(void *classId, void **factory) {
    if (!factory) return E_POINTER;
    *factory = NULL;
    if (hstring_equals(classId, L"Windows.Devices.Enumeration.DeviceInformation"))
        *factory = &g_diActivation;
    else if (hstring_equals(classId, L"Windows.Media.Capture.Frames.MediaFrameSourceGroup"))
        *factory = &g_camActivation;
    else return CLASS_E_CLASSNOTAVAILABLE;
    Act_Add(*factory);
    return S_OK;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD r, LPVOID x) { (void)h; (void)r; (void)x; return TRUE; }

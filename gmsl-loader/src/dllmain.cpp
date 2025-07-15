#define WIN32_LEAN_AND_MEAN

#include "windows.h"
#include "MinHook.h"
#include "hostfxr/nethost.h"
#include "hostfxr/hostfxr.h"
#include "hostfxr/coreclr_delegates.h"
#include <filesystem>
#include <print>

#define DLL_PROXY_ORIGINAL(name) original_##name

#define EXPORT(name)                  \
    FARPROC DLL_PROXY_ORIGINAL(name); \
    void _##name()                    \
    {                                 \
        DLL_PROXY_ORIGINAL(name)();   \
    }
#include "exports.h"
#undef EXPORT


std::filesystem::path GetSystemDir()
{
    wchar_t systemDirectory[MAX_PATH] = {0};
    if (!GetSystemDirectoryW(systemDirectory, MAX_PATH))
        std::println("GetSystemDirectoryW failed: {}", GetLastError());
    return systemDirectory;
}

bool LoadProxy()
{
    std::filesystem::path libraryPath = GetSystemDir() / "version.dll";
    const auto library = LoadLibrary(libraryPath.string().c_str());
    if (!library)
        return false;

#define EXPORT(name) DLL_PROXY_ORIGINAL(name) = GetProcAddress(library, #name);
#include "exports.h"
#undef EXPORT

    return true;
}

typedef HANDLE(WINAPI* CreateFileW_t)(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile);

CreateFileW_t originalCreateFileW = nullptr;

hostfxr_initialize_for_runtime_config_fn init_fptr = nullptr;
hostfxr_get_runtime_delegate_fn get_delegate_fptr = nullptr;
hostfxr_close_fn close_fptr = nullptr;

typedef void (CORECLR_DELEGATE_CALLTYPE* PatchDataFn)(const wchar_t*);
PatchDataFn patchData = nullptr;

void* load_library(const wchar_t* path)
{
    HMODULE h = ::LoadLibraryW(path);
    return reinterpret_cast<void*>(h);
}

template <typename T>
T get_export(void* h, const char* name)
{
    return reinterpret_cast<T>(GetProcAddress((HMODULE)h, name));
}

bool load_hostfxr()
{
    wchar_t buffer[MAX_PATH];
    size_t size = sizeof(buffer) / sizeof(wchar_t);
    int rc = get_hostfxr_path(buffer, &size, nullptr);
    if (rc != 0)
        return false;

    void* lib = load_library(buffer);
    init_fptr = get_export<hostfxr_initialize_for_runtime_config_fn>(lib, "hostfxr_initialize_for_runtime_config");
    get_delegate_fptr = get_export<hostfxr_get_runtime_delegate_fn>(lib, "hostfxr_get_runtime_delegate");
    close_fptr = get_export<hostfxr_close_fn>(lib, "hostfxr_close");

    return (init_fptr && get_delegate_fptr && close_fptr);
}

HANDLE WINAPI HookedCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    static bool hasPatched = false;
    static bool patching = false;

    std::filesystem::path path(lpFileName);

    if (path.filename() == "data.win" && !patching)
    {
        std::println("Found data.win at path: {}", path.string());
        if (!hasPatched)
        {
            patching = true;
            hasPatched = true;
            patchData(path.c_str());
            patching = false;
        }
        path = path.remove_filename() / "gmsl" / "patcher" / "cache.win";
    }

    return originalCreateFileW(
        path.c_str(),
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);
}

DWORD WINAPI Loader(LPVOID lpParam)
{
    SuspendThread(lpParam);
    LoadProxy();
    FILE* file;
    AllocConsole();
    freopen_s(&file, "CONOUT$", "w", stdout);

    if (!load_hostfxr())
    {
        std::println("Failed to load hostfxr");
        return 1;
    }

    const wchar_t* configPath = L"gmsl/patcher/gmsl-patcher.runtimeconfig.json";

    hostfxr_handle cxt = nullptr;
    int rc = init_fptr(configPath, nullptr, &cxt);

    load_assembly_and_get_function_pointer_fn get_func = nullptr;
    rc = get_delegate_fptr(
        cxt,
        hdt_load_assembly_and_get_function_pointer,
        (void**)&get_func
    );

    rc = close_fptr(cxt);

    get_func(
        L"gmsl/patcher/gmsl-patcher.dll",
        L"GMSLPatcher.Patcher, gmsl-patcher",
        L"PatchData",
        UNMANAGEDCALLERSONLY_METHOD,
        nullptr,
        (void**)&patchData
    );

    if (rc != 0 || patchData == nullptr) 
    {
        std::println("Failed to get PatchData delegate (rc: {})", rc);
        return 1;
    }
    
    if (MH_Initialize() != MH_OK)
        return 1;

    if (MH_CreateHook(&CreateFileW, &HookedCreateFileW, reinterpret_cast<LPVOID*>(&originalCreateFileW)) != MH_OK)
        return 1;

    if (MH_EnableHook(&CreateFileW) != MH_OK)
        return 1;

    ResumeThread(lpParam);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason != DLL_PROCESS_ATTACH)
        return true;

    DisableThreadLibraryCalls(hModule);
    HANDLE currentThread = OpenThread(THREAD_ALL_ACCESS, false, GetCurrentThreadId());
    HANDLE loaderThread = CreateThread(nullptr, 0, Loader, currentThread, 0, nullptr);
    if (!loaderThread)
        return false;

    return true;
}
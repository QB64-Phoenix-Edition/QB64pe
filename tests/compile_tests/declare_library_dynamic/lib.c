
#ifdef WIN32
# define __export __declspec(dllexport)
/* 
 * QB64-PE emits DECLARE DYNAMIC LIBRARY function pointers as __stdcall on
 * Windows, so we have to match that here (rather than the __cdecl default).
 * When building the 32-bit DLL, use `-Wl,--kill-at` so that the exported
 * symbol stays undecorated (`add_values`, not `add_values@8`).
 */
# define __callconv __stdcall
#else
# define __export __attribute__((visibility("default")))
# define __callconv
#endif

/* Simple code to put in a dll, has no dependencies */
__export int __callconv add_values(int v1, int v2)
{
    return v1 + v2;
}

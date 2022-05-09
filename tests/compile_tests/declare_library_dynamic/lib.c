
#ifdef WIN32
# define __export __declspec(dllexport)
#else
# define __export __attribute__((visibility("default")))
#endif

/* Simple code to put in a dll, has no dependencies */
__export int add_values(int v1, int v2)
{
    return v1 + v2;
}

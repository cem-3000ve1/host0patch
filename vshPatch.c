//build
#include <vitasdkkern.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/dirent.h>
#include <taihen.h>
#include <string.h>
#include <stdio.h>

#define printf ksceDebugPrintf

static tai_hook_ref_t ref_open;
static tai_hook_ref_t ref_dopen;
static tai_hook_ref_t ref_remove;

#define REDIRECT_PREFIX "ux0:/host0/"
#define REDIRECT_TARGET "host0:/"

const char *redirect_path(const char *path, char *out, size_t outlen) {
    if (!path) return path;

    if (strncmp(path, REDIRECT_PREFIX, strlen(REDIRECT_PREFIX)) == 0) {
        snprintf(out, outlen, "%s%s", REDIRECT_TARGET, path + strlen(REDIRECT_PREFIX));
        return out;
    }

    return path;
}


int my_open(const char *path, int flags, SceMode mode) {
    char new_path[256];
    const char *final_path = redirect_path(path, new_path, sizeof(new_path));

    return TAI_CONTINUE(int, ref_open, final_path, flags, mode);
}


SceUID my_dopen(const char *path) {
    char new_path[256];
    const char *final_path = redirect_path(path, new_path, sizeof(new_path));

    return TAI_CONTINUE(SceUID, ref_dopen, final_path);
}


int my_remove(const char *path) {
    char new_path[256];
    const char *final_path = redirect_path(path, new_path, sizeof(new_path));

    return TAI_CONTINUE(int, ref_remove, final_path);
}


void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize argc, const void *args) {
    printf("[host0_mapper] Installing hooks...\n");

    taiHookFunctionImport(&ref_open,
        TAI_MAIN_KERNEL,
        TAI_ANY_LIBRARY,
        0x6C60AC61,
        my_open);

    taiHookFunctionImport(&ref_dopen,
        TAI_MAIN_KERNEL,
        TAI_ANY_LIBRARY,
        0xE6C0F027,
        my_dopen);

    taiHookFunctionImport(&ref_remove,
        TAI_MAIN_KERNEL,
        TAI_ANY_LIBRARY,
        0x1CC3F0BC, 
        my_remove);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    taiHookReleaseForKernel(ref_open,     my_open);
    taiHookReleaseForKernel(ref_dopen,    my_dopen);
    taiHookReleaseForKernel(ref_remove,   my_remove);
    printf("[host0_mapper] Hooks removed.\n");
    return SCE_KERNEL_STOP_SUCCESS;
}

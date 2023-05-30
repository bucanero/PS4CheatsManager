#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <orbis/libkernel.h>
#include <libjbc.h>

#include "orbis_patches.h"
#include "util.h"
#include "notifi.h"

#define sys_proc_read_mem(p, a, d, l)       sys_proc_rw(p, a, d, l, 0)
#define sys_proc_write_mem(p, a, d, l)      sys_proc_rw(p, a, d, l, 1)


asm("cpu_enable_wp:\n"
    "mov %rax, %cr0\n"
    "or %rax, 0x10000\n"
    "mov %cr0, %rax\n"
    "ret");
void cpu_enable_wp();

asm("cpu_disable_wp:\n"
    "mov %rax, %cr0\n"
    "and %rax, ~0x10000\n"
    "mov %cr0, %rax\n"
    "ret");
void cpu_disable_wp();

asm("orbis_syscall:\n"
    "movq $0, %rax\n"
    "movq %rcx, %r10\n"
    "syscall\n"
    "jb err\n"
    "retq\n"
"err:\n"
    "pushq %rax\n"
    "callq __error\n"
    "popq %rcx\n"
    "movl %ecx, 0(%rax)\n"
    "movq $0xFFFFFFFFFFFFFFFF, %rax\n"
    "movq $0xFFFFFFFFFFFFFFFF, %rdx\n"
    "retq\n");
int orbis_syscall(int num, ...);

int sysKernelGetLowerLimitUpdVersion(int* unk);
int sysKernelGetUpdVersion(int* unk);

static int goldhen2 = 0;

int _sceKernelGetModuleInfo(OrbisKernelModule handle, OrbisKernelModuleInfo* info)
{
    if (!info)
        return ORBIS_KERNEL_ERROR_EFAULT;

    memset(info, 0, sizeof(*info));
    info->size = sizeof(*info);

    return orbis_syscall(SYS_dynlib_get_info, handle, info);
}

int sceKernelGetModuleInfoByName(const char* name, OrbisKernelModuleInfo* info)
{
    OrbisKernelModuleInfo tmpInfo;
    OrbisKernelModule handles[256] = {0};
    size_t numModules;
    int ret;

    if (!name || !info)
        return ORBIS_KERNEL_ERROR_EFAULT;

    memset(handles, 0, sizeof(handles));

    ret = sceKernelGetModuleList(handles, countof(handles), &numModules);
    if (ret) {
        LOG("sceKernelGetModuleList (%X)", ret);
        return ret;
    }

    for (size_t i = 0; i < numModules; ++i)
    {
        ret = _sceKernelGetModuleInfo(handles[i], &tmpInfo);
        if (ret) {
            LOG("_sceKernelGetModuleInfo (%X)", ret);
            return ret;
        }

        if (strcmp(tmpInfo.name, name) == 0) {
            memcpy(info, &tmpInfo, sizeof(tmpInfo));
            return 0;
        }
    }

    return ORBIS_KERNEL_ERROR_ENOENT;
}

int get_module_base(const char* name, uint64_t* base, uint64_t* size)
{
    OrbisKernelModuleInfo moduleInfo;
    int ret;

    ret = sceKernelGetModuleInfoByName(name, &moduleInfo);
    if (ret)
    {
        LOG("[!] sceKernelGetModuleInfoByName('%s') failed: 0x%08X", name, ret);
        return 0;
    }

    if (base)
        *base = (uint64_t)moduleInfo.segmentInfo[0].address;

    if (size)
        *size = moduleInfo.segmentInfo[0].size;

    return 1;
}

int patch_module(const char* name, module_patch_cb_t* patch_cb, void* arg)
{
    uint64_t base, size;
    int ret;

    if (!get_module_base(name, &base, &size)) {
        LOG("[!] get_module_base return error");
        return 0;
    }
    LOG("[i] '%s' module base=0x%lX size=%ld", name, base, size);

    ret = sceKernelMprotect((void*)base, size, ORBIS_KERNEL_PROT_CPU_ALL);
    if (ret) {
        LOG("[!] sceKernelMprotect(%lX) failed: 0x%08X", base, ret);
        return 0;
    }

    LOG("[+] patching module '%s'...", name);
    if (patch_cb)
        patch_cb(arg, (uint8_t*)base, size);

    return 1;
}

static int get_firmware_version()
{
    int fw;

    // upd_version >> 16 
    // 0x505  0x672  0x702  0x755  etc
    if (sysKernelGetUpdVersion(&fw) && sysKernelGetLowerLimitUpdVersion(&fw))
    {
        LOG("Error: can't detect firmware version!");
        return (-1);
    }

    LOG("[i] PS4 Firmware %X", fw >> 16);
    return (fw >> 16);
}

// custom syscall 112
int sys_console_cmd(uint64_t cmd, void *data)
{
    return orbis_syscall(goldhen2 ? 200 : 112, cmd, data);
}

// GoldHEN 2+ only
// custom syscall 200
static int check_syscalls()
{
    uint64_t tmp;

    if (orbis_syscall(200, SYS_CONSOLE_CMD_ISLOADED, NULL) == 1)
    {
        LOG("GoldHEN 2.x is loaded!");
        goldhen2 = 90;
        return 1;
    }
    else if (orbis_syscall(200, SYS_CONSOLE_CMD_PRINT, "apollo") == 0)
    {
        LOG("GoldHEN 1.x is loaded!");
        return 1;
    }
    else if (orbis_syscall(107, NULL, &tmp) == 0)
    {
        LOG("ps4debug is loaded!");
        return 1;
    }

    return 0;
}

// GoldHEN 2+ custom syscall 197
// (same as ps4debug syscall 107)
int sys_proc_list(struct proc_list_entry *procs, uint64_t *num)
{
    return orbis_syscall(107 + goldhen2, procs, num);
}

// GoldHEN 2+ custom syscall 198
// (same as ps4debug syscall 108)
int sys_proc_rw(uint64_t pid, uint64_t address, void *data, uint64_t length, uint64_t write)
{
    return orbis_syscall(108 + goldhen2, pid, address, data, length, write);
}

// GoldHEN 2+ custom syscall 199
// (same as ps4debug syscall 109)
int sys_proc_cmd(uint64_t pid, uint64_t cmd, void *data)
{
    return orbis_syscall(109 + goldhen2, pid, cmd, data);
}

/*
int goldhen_jailbreak()
{
    return sys_console_cmd(SYS_CONSOLE_CMD_JAILBREAK, NULL);
}
*/

int find_process_pid(const char* proc_name, int* pid)
{
    struct proc_list_entry *proc_list;
    uint64_t pnum;

    if (sys_proc_list(NULL, &pnum))
        return 0;

    proc_list = (struct proc_list_entry *) malloc(pnum * sizeof(struct proc_list_entry));

    if(!proc_list)
        return 0;

    if (sys_proc_list(proc_list, &pnum))
    {
        free(proc_list);
        return 0;
    }

    for (size_t i = 0; i < pnum; i++)
        if (strncmp(proc_list[i].p_comm, proc_name, 32) == 0)
        {
            LOG("[i] Found '%s' PID %d", proc_name, proc_list[i].pid);
            *pid = proc_list[i].pid;
            free(proc_list);
            return 1;
        }

    LOG("[!] '%s' Not Found", proc_name);
    free(proc_list);
    return 0;
}

int find_map_entry_start(int pid, const char* entry_name, uint64_t* start)
{
    struct sys_proc_vm_map_args proc_vm_map_args = {
        .maps = NULL,
        .num = 0
    };

    if(sys_proc_cmd(pid, SYS_PROC_VM_MAP, &proc_vm_map_args))
        return 0;

    proc_vm_map_args.maps = (struct proc_vm_map_entry *) malloc(proc_vm_map_args.num * sizeof(struct proc_vm_map_entry));

    if(!proc_vm_map_args.maps)
        return 0;

    if(sys_proc_cmd(pid, SYS_PROC_VM_MAP, &proc_vm_map_args))
    {
        free(proc_vm_map_args.maps);
        return 0;
    }

    for (size_t i = 0; i < proc_vm_map_args.num; i++)
        if (strncmp(proc_vm_map_args.maps[i].name, entry_name, 32) == 0)
        {
            LOG("Found '%s' Start addr %lX", entry_name, proc_vm_map_args.maps[i].start);
            *start = proc_vm_map_args.maps[i].start;
            free(proc_vm_map_args.maps);
            return 1;
        }

    LOG("[!] '%s' Not Found", entry_name);
    free(proc_vm_map_args.maps);
    return 0;
}

int patch_save_libraries()
{
    int version = get_firmware_version();

    return 1;
/////////
    switch (version)
    {
    case -1:
        notifi(NULL, "Error: Can't detect firmware version!");
        return 0;

    case 0x505:
        break;
    
    case 0x672:
        break;

    case 0x702:
        break;

    case 0x750:
    case 0x755:
        break;

    case 0x900:
        break;

    default:
        notifi(NULL, "Unsupported firmware version %X.%02X", version >> 8, version & 0xFF);
        return 0;
    }

    if (!check_syscalls())
    {
        notifi(NULL, "Missing %X.%02X GoldHEN or ps4debug payload!", version >> 8, version & 0xFF);
        return 0;
    }

    return 1;
}

// Variables for (un)jailbreaking
jbc_cred g_Cred;
jbc_cred g_RootCreds;

// Verify jailbreak
static int is_jailbroken()
{
    FILE *s_FilePointer = fopen("/user/.jailbreak", "w");

    if (!s_FilePointer)
        return 0;

    fclose(s_FilePointer);
    remove("/user/.jailbreak");
    return 1;
}

// Jailbreaks creds
static int jailbreak()
{
    if (is_jailbroken())
        return 1;

    jbc_get_cred(&g_Cred);
    g_RootCreds = g_Cred;
    jbc_jailbreak_cred(&g_RootCreds);
    jbc_set_cred(&g_RootCreds);

    return (is_jailbroken());
}

// Restores original creds
static void unjailbreak()
{
    if (!is_jailbroken())
        return;

    jbc_set_cred(&g_Cred);
}

// Initialize jailbreak
int initialize_jbc()
{
    // Pop notification depending on jailbreak result
    if (!jailbreak())
    {
        LOG("Jailbreak failed!");
        notifi(NULL, "Jailbreak failed!");
        return 0;
    }

    LOG("Jailbreak succeeded!");
    return 1;
}

// Unload libjbc libraries
void terminate_jbc()
{
	unjailbreak();
    LOG("Jailbreak removed!");
}

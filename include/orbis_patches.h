#include <stdint.h>

#define SYS_dynlib_get_info         593
#define SYS_dynlib_get_info_ex      608

#define SYS_PROC_ALLOC              1
#define SYS_PROC_FREE               2
#define SYS_PROC_PROTECT            3
#define SYS_PROC_VM_MAP             4
#define SYS_PROC_INSTALL            5
#define SYS_PROC_CALL               6
#define SYS_PROC_ELF                7
#define SYS_PROC_INFO               8
#define SYS_PROC_THRINFO            9

#define SYS_CONSOLE_CMD_REBOOT      1
#define SYS_CONSOLE_CMD_PRINT       2
#define SYS_CONSOLE_CMD_JAILBREAK   3
#define SYS_CONSOLE_CMD_ISLOADED    6

typedef void module_patch_cb_t(void* arg, uint8_t* base, uint64_t size);

// custom syscall 107
struct proc_list_entry {
    char p_comm[32];
    int pid;
}  __attribute__((packed));

struct proc_vm_map_entry {
    char name[32];
    uint64_t start;
    uint64_t end;
    uint64_t offset;
    uint16_t prot;
} __attribute__((packed));

// custom syscall 109
// SYS_PROC_VM_MAP
struct sys_proc_vm_map_args {
    struct proc_vm_map_entry *maps;
    uint64_t num;
} __attribute__((packed));

// SYS_PROC_ALLOC
struct sys_proc_alloc_args {
    uint64_t address;
    uint64_t length;
} __attribute__((packed));

// SYS_PROC_FREE
struct sys_proc_free_args {
    uint64_t address;
    uint64_t length;
} __attribute__((packed));

// SYS_PROC_PROTECT
struct sys_proc_protect_args {
    uint64_t address;
    uint64_t length;
    uint64_t prot;
} __attribute__((packed));

// SYS_PROC_INSTALL
struct sys_proc_install_args {
    uint64_t stubentryaddr;
} __attribute__((packed));

// SYS_PROC_CALL
struct sys_proc_call_args {
    uint32_t pid;
    uint64_t rpcstub;
    uint64_t rax;
    uint64_t rip;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t r8;
    uint64_t r9;
} __attribute__((packed));

// SYS_PROC_ELF
struct sys_proc_elf_args {
    void *elf;
} __attribute__((packed));

// SYS_PROC_INFO
struct sys_proc_info_args {
    int pid;
    char name[40];
    char path[64];
    char titleid[16];
    char contentid[64];
} __attribute__((packed));

// SYS_PROC_THRINFO
struct sys_proc_thrinfo_args {
    uint32_t lwpid;
    uint32_t priority;
    char name[32];
} __attribute__((packed));

typedef struct orbis_patch {
    uint32_t offset;
    char* data;
    uint32_t size;
} orbis_patch_t;

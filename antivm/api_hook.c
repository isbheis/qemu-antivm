/*
 * 2017-10-30
 * implement of api hook
 */

#include "osdep.h"
#include "cpu.h"
#include "api_hook.h"
#include "win_mem_struct.h"
#include "exec-all.h"

/* exactly match dst and target */
static bool target_match(uint8_t *dst, const char* target);

static target_ulong gcr3 = 0;
/*
 * filtered APIs virtual addr, fixed in winxp:
 * GlobalMemoryStatus: 0x7c8310e2
 * GlobalMemoryStatusEx: 0x7c81f97a
 */
static target_ulong gms_addr = 0x7c8310e2;
static uint32_t gmsx_addr = 0x7c81f97a;
/*
 * ntdll!NtTerminateProcess va: 0x7c92de50
 * kernel32!ExitProcess va: 0x7c81cafa
 * nt!NtTerminateProcess va: 0x805c9c2a
 * nt!ZwTerminateProcsss va: 0x805001ac
 */
//static target_ulong tp_addr = 0x805c9c2a;
static const char *target = "pafish.vxe";

static void get_target_cr3(CPUState *env){
    if (gcr3 == 0){
        /* get kthread using kpcr(kpcr->kprcb->kthread) */
        uint8_t kthread[4];
        target_ulong kd_offset = 0xffdff000 + 0x120 + 0x4;
        cpu_memory_rw_debug(env, kd_offset, kthread, 4, 0);

        /* get kprocess */
        uint8_t kprocess[4];
        uint32_t ks_offset = *(uint32_t *)kthread + 0x220;
        cpu_memory_rw_debug(env, (target_ulong)ks_offset, kprocess, 4, 0);

        /* get image file name */
        static uint8_t image_name[16];
        uint32_t in_offset = *(uint32_t *)kprocess + 0x174;
        cpu_memory_rw_debug(env, (target_ulong)in_offset, image_name, 16, 0);

        /* filter target image name */
        if (target_match(image_name, target)){
            /*
             * check if env->cr[3] eq kprocss->DirectoryTableBase
             * excluding process switching or remote thread
             */
            uint8_t DTB[8];
            uint32_t DTB_offset = *(uint32_t *)kprocess + 0x18;
            cpu_memory_rw_debug(env, (target_ulong)DTB_offset, DTB, 8, 0);
            if ((uint32_t)env->cr[3] == *(uint32_t *)DTB){
                gcr3 = env->cr[3];
                fprintf(stderr, "target cr3: %x\n", (uint32_t)gcr3);
            }else {
                fprintf(stderr, "hit target, but cr3 inconsistent\n");
                memset(image_name, 0, sizeof(image_name));
                return;
            }

            /* get target pid */
            uint8_t pid[4];
            uint32_t pid_offset = *(uint32_t *)kprocess + 0x84;
            cpu_memory_rw_debug(env, (target_ulong)pid_offset, pid, 4, 0);
            fprintf(stderr, "target image name: %-16s, pid: %-8d\n", \
                    (const char*)image_name, *(uint32_t *)pid);
        }else{
            memset(image_name, 0, sizeof(image_name));
        }
    }
}

static void monitor_api(CPUState *env){
    if  (gcr3 == env->cr[3] && gcr3 != 0){
        target_ulong eip = env->eip;

        /* not consider multi-threads now */
        static uint8_t gms_ret_addr[4];
        static uint8_t gmsx_ret_addr[4];
        static uint8_t gms_buf[4];
        static uint8_t gmsx_buf[4];

        /* debug */
        //if ((uint32_t)eip >= 0x400000 && (uint32_t)eip <= 0x440000)
        //    fprintf(stderr, "%x\n", (uint32_t)eip);

        /* hit GlobalMemoryStatus */
        if (eip == gms_addr){
            /* get parameter and ret addr from stack */
            uint32_t esp = (uint32_t)env->regs[R_ESP];
            cpu_memory_rw_debug(env, (target_ulong)esp, gms_ret_addr, 4, 0);
            cpu_memory_rw_debug(env, (target_ulong)(esp + 4), gms_buf, 4, 0);
            fprintf(stderr, "call GlobalMemoryStatus with para: %x at %x\n", \
                    *(uint32_t *)gms_buf, *(uint32_t *)gms_ret_addr);

        /* hit GlobalMemoryStatusEx */
        }else if (eip == gmsx_addr){
            /* get para and ret addr from stack */
            uint32_t esp = (uint32_t)env->regs[R_ESP];
            cpu_memory_rw_debug(env, (target_ulong)esp, gmsx_ret_addr, 4, 0);
            cpu_memory_rw_debug(env, (target_ulong)(esp + 4), gmsx_buf, 4, 0);
            fprintf(stderr, "call GlobalMemoryStatusEx with para: %x at %x\n",                      *(uint32_t *)gms_buf, *(uint32_t *)gmsx_ret_addr);

        /* hit ret addr of GlobalMemoryStatus */
        }else if ((uint32_t)eip == *(uint32_t *)gms_ret_addr){
            /* get orignal values */
            uint8_t ms[sizeof(MemoryStatus)];
            uint32_t gms_lpbuf = *(uint32_t *)gms_buf;
            cpu_memory_rw_debug(env, (target_ulong)gms_lpbuf, \
                                ms, sizeof(MemoryStatus), 0);
            MemoryStatus *pms = (MemoryStatus *)ms;
            fprintf(stderr, "original val for dwTotalPhys: %dMB\n", \
                    (DWORD)(pms->dwTotalPhys/(1024 * 1024)));

            /* reset MemoryStatus */
            pms->dwTotalPhys = 2 * 1024 * 1024 * 1024;
            cpu_memory_rw_debug(env, (target_ulong)gms_lpbuf, \
                                ms, sizeof(MemoryStatus), 1);
            fprintf(stderr, "reset dwTotalPhys to: %dMB\n", \
                    (DWORD)(pms->dwTotalPhys/(1024 * 1024)));

            /* reset params */
            memset(gms_ret_addr, 0, sizeof(gms_ret_addr));
            memset(gms_buf, 0, sizeof(gms_buf));

        /* hit ret addr of GlobalMemoryStatusEx */
        }else if ((uint32_t)eip == *(uint32_t *)gmsx_ret_addr){
            /* get original values */
            uint8_t msx[sizeof(MemoryStatusEx)];
            uint32_t gmsx_lpbuf = *(uint32_t *)gmsx_buf;
            cpu_memory_rw_debug(env, (target_ulong)gmsx_lpbuf, \
                                msx, sizeof(MemoryStatusEx), 0);
            MemoryStatusEx *pmsx = (MemoryStatus *)msx;
            fprintf(stderr, "original val for dwTotalPhys: %dMB\n", \
                    (DWORD)(pmsx->ullTotalPhys/(1024 * 1024)));

            /* reset MemoryStatusEx */
            pmsx->ullTotalPhys = 2 * 1024 * 1024 * 1024;
            cpu_memory_rw_debug(env, (target_ulong)gmsx_lpbuf, \
                                msx, sizeof(MemoryStatusEx), 1);
            fprintf(stderr, "reset dwTotalPhys to: %dMB\n", \
                    (DWORD)(pmsx->ullTotalPhys/(1024 * 1024)));

            /* reset params */
            memset(gmsx_ret_addr, 0, sizeof(gmsx_ret_addr));
            memset(gmsx_buf, 0, sizeof(gmsx_buf));
        //}else if (eip == tp_addr){
            /* process exit */
        //    fprintf(stderr, "process exit\n");
        //    gcr3 = 0;
        }
    }
}

void monitor_memory_api(CPUState *env, void *opaque){
    if (gcr3 == 0){
        get_target_cr3(env);
    }else{
        monitor_api(env);
    }
}

static bool target_match(uint8_t dst[], const char* target){
    /* dst is fixed length */
    for (int i = 0; i < 16 && dst[i] == (uint8_t)*target; i++, target++)
        if (dst[i] == '\0')
            return true;
    return false;
}

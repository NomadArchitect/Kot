#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <kot/sys.h>
#include <kot/uisd.h>
#include <bits/ensure.h>
#include <frg/vector.hpp>
#include <mlibc/debug.hpp>
#include <mlibc/elf/startup.h>
#include <mlibc/allocator.hpp>
#include <kot/uisd/srvs/time.h>
#include <mlibc/all-sysdeps.hpp>
#include <kot/uisd/srvs/system.h>


extern char **environ;
extern mlibc::exec_stack_data __mlibc_stack_data;

uint64_t KotAnonAllocateLock = 0;

namespace mlibc{
    void sys_libc_log(const char *message){
        kot_Sys_Logs((char*)message, strlen(message));
    }

    [[noreturn]] void sys_libc_panic(){
        sys_libc_log("libc panic!");
        __builtin_trap();
        for(;;);
    }

    int sys_tcb_set(void *pointer){
        return (kot_Sys_SetTCB(kot_Sys_GetThread(), (void*)pointer) != KSUCCESS);
    }

    int sys_futex_tid(){
        return static_cast<int>(kot_Sys_GetTID());
    }

    int sys_futex_wait(int *pointer, int expected, const struct timespec *time){
        // TODO
        __ensure(!"Not implemented");
    }

    int sys_futex_wake(int *pointer){
        // TODO
        __ensure(!"Not implemented");
    }

    int sys_anon_allocate(size_t size, void **pointer){
        // TODO
        if(size % KotSpecificData.MMapPageSize){
            size -= size % KotSpecificData.MMapPageSize;
            size += KotSpecificData.MMapPageSize;
        }
        atomicAcquire(&KotAnonAllocateLock, 0);
        *pointer = (void*)KotSpecificData.HeapLocation;
        int Status = (Syscall_48(KSys_Map, kot_Sys_GetProcess(), (uint64_t)pointer, 0, 0, (uint64_t)&size, false) != KSUCCESS);
        KotSpecificData.HeapLocation += size;
        atomicUnlock(&KotAnonAllocateLock, 0);
        return Status;
    }

    int sys_anon_free(void *pointer, size_t size){
        sys_vm_unmap(pointer, size);
        return 0;
    }

    int sys_vm_map(void *hint, size_t size, int prot, int flags, int fd, off_t offset, void **window){
        // TODO
        __ensure(!"Not implemented");
        return (Syscall_48(KSys_Map, kot_Sys_GetProcess(), (uint64_t)&hint, 0, 0, (uint64_t)&size, false) != KSUCCESS);
    }

    int sys_vm_unmap(void *pointer, size_t size){
        return (kot_Sys_Unmap(kot_Sys_GetProcess(), (void*)pointer, static_cast<size64_t>(size)) != KSUCCESS);
    }

    int sys_vm_protect(void *pointer, size_t size, int prot){
        // TODO
        __ensure(!"Not implemented");
    }

    [[noreturn]] void sys_exit(int status){
        Syscall_8(KSys_Exit, status);
        __builtin_unreachable();
    }

    [[noreturn, gnu::weak]] void sys_thread_exit(){
        sys_exit(KSUCCESS);
        __builtin_unreachable();
    }

    int sys_clock_get(int clock, time_t *secs, long *nanos){
        // TODO
        *secs = 0;
        *nanos = 0;
        return 0;
    }

    int sys_clock_getres(int clock, time_t *secs, long *nanos){
        // TODO
        *secs = 0;
        *nanos = 0;
        return 0;
    }

    int sys_sleep(time_t *secs, long *nanos){
        KResult Status = kot_Sleep((*secs) * 1000000000 + (*nanos));
        *secs = 0;
	    *nanos = 0;
        return 0;
    }

    int sys_sigprocmask(int how, const sigset_t *__restrict set, sigset_t *__restrict retrieve){
        // TODO
        //__ensure(!"Not implemented");
        return 0;
    }

    int sys_sigaction(int, const struct sigaction *__restrict, struct sigaction *__restrict){
        // TODO
        //__ensure(!"Not implemented");
        return 0;
    }

    int sys_fork(pid_t *child){
        kot_process_t ProcessChild;
        KResult Status = kot_Sys_Fork((kot_process_t*)&ProcessChild);

        if(Status != KSUCCESS) return -1;


        if(kot_Sys_GetPPID()){
            // We are child
            // Reset UISD thread to avoid redirection to parent process
            __ensure(kot_ResetUISDThreads() == KSUCCESS);
            *child = NULL;
        }else{
            // We are parent
            *child = ProcessChild;
        }
        return (Status != KSUCCESS);
    }

    int sys_waitpid(pid_t pid, int *status, int flags, struct rusage *ru, pid_t *ret_pid){
        // TODO
        //__ensure(!"Not implemented");
        return 0;
    }

    int sys_execve(const char *path, char *const argv[], char *const envp[]){
        if(!fopen(path, "r")){
           return -1;
        }

        void* MainStackData;
        size64_t SizeMainStackData;
        uint64_t argc = 0;
        for(; argv[argc] != NULL; argc++);
        kot_SetupStack(&MainStackData, &SizeMainStackData, argc, (char**)argv, (char**)envp);

        kot_Srv_System_LoadExecutableToProcess((char*)path, MainStackData, SizeMainStackData);

        __builtin_unreachable();
    }

    pid_t sys_getpid(){
        return static_cast<pid_t>(kot_Sys_GetPID());
    }

    int sys_kill(int, int){
        // TODO
        __ensure(!"Not implemented");
    }

    uint64_t sys_debug_malloc_lock = 0;

    void *sys_debug_malloc(size_t size){
        if(size == 0) return NULL;
        atomicAcquire(&sys_debug_malloc_lock, 0);
        size64_t SizeToAlloc = size + sizeof(size64_t);
        if(SizeToAlloc % KotSpecificData.MMapPageSize){
            SizeToAlloc -= SizeToAlloc % KotSpecificData.MMapPageSize;
            SizeToAlloc += KotSpecificData.MMapPageSize;
        }
        size64_t* CurrentSeg = (size64_t*)((uintptr_t)KotSpecificData.HeapLocation + KotSpecificData.MMapPageSize - ((size + sizeof(size64_t)) % KotSpecificData.MMapPageSize));
        void* VirtualAddress = (void*)CurrentSeg;
        __ensure(kot_Sys_Map(kot_Sys_GetProcess(), (void**)&VirtualAddress, AllocationTypeBasic, NULL, (size64_t*)&SizeToAlloc, false) == KSUCCESS);
        void* Buffer = (void*)((uintptr_t)CurrentSeg + sizeof(size64_t));
        KotSpecificData.HeapLocation = (uint64_t)((uintptr_t)KotSpecificData.HeapLocation + SizeToAlloc + KotSpecificData.MMapPageSize); // Add page size for the guard
        *CurrentSeg = SizeToAlloc;
        atomicUnlock(&sys_debug_malloc_lock, 0);
        return Buffer;
    }

    void *sys_debug_realloc(void *ptr, size_t size){
        void* newBuffer = sys_debug_malloc(size);
        if(newBuffer == NULL){
            return NULL;
        }
        if(ptr != NULL){
            uint64_t oldSize = *(uint64_t*)((uintptr_t)ptr - sizeof(uint64_t));
            if (size < oldSize) {
                oldSize = size;
            }
            memcpy(newBuffer, ptr, oldSize);
            sys_debug_free(ptr);
        }
        return newBuffer;
    }

    void sys_debug_free(void *ptr){
        if(ptr == NULL) return;
        size64_t SizeToFree = *(size64_t*)(void*)((uintptr_t)ptr - sizeof(size64_t));
        uintptr_t StartAdress = (uintptr_t)((uintptr_t)ptr - sizeof(size64_t));
        if(StartAdress % KotSpecificData.MMapPageSize){
            SizeToFree += StartAdress % KotSpecificData.MMapPageSize;
            StartAdress -= StartAdress % KotSpecificData.MMapPageSize;
        }
        kot_Sys_Unmap(kot_Sys_GetProcess(), (void*)StartAdress, SizeToFree);
    }
}
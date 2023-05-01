#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>
#include <kot/thread.h>
#include <bits/ensure.h>
#include <mlibc/tcb.hpp>
#include <mlibc/tid.hpp>
#include <frg/array.hpp>
#include <mlibc/debug.hpp>
#include <mlibc/thread.hpp>
#include <frg/allocation.hpp>
#include <mlibc/allocator.hpp>
#include <mlibc/posix-sysdeps.hpp>

extern "C" Tcb *__rtdl_allocateTcb();

namespace Kot{
    // Warning: This function must be used after each Sys_CreateThread unless another initialization for the thread has been performed
    KResult InitializeThread(kot_thread_t thread){
        auto new_tcb = __rtdl_allocateTcb();
        pid_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);

        // TODO: due to alignment guarantees, the stackaddr and stacksize might change
        // when the stack is allocated. Currently this isn't propagated to the TCB,
        // but it should be.
        void *stack = attr.__mlibc_stackaddr;
        if (!mlibc::sys_prepare_stack) {
            MLIBC_MISSING_SYSDEP();
            return ENOSYS;
        }
        void *entry;
        Sys_Thread_Info_Get(Sys_Getthread(), 3, (uint64_t*)&entry);
        int ret = mlibc::sys_prepare_stack(&stack, reinterpret_cast<void*>(entry),
                NULL, new_tcb, &attr.__mlibc_stacksize, &attr.__mlibc_guardsize);
        if (ret)
            return ret;

        new_tcb->stackAddr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(stack) - attr.__mlibc_stacksize - attr.__mlibc_guardsize);      

        Sys_SetTCB(thread, (uintptr_t)new_tcb);
        return KSUCCESS; 
    }
}

namespace mlibc{
    int sys_prepare_stack(void **stack, void *entry, void *user_arg, void *tcb, size_t *stack_size, size_t *guard_size) {
        // We can use this thread because stack is initialized in the same way for every thread
        Kot::Sys_Thread_Info_Get(Kot::Sys_Getthread(), 1, (uint64_t*)stack);
        Kot::Sys_Thread_Info_Get(Kot::Sys_Getthread(), 2, stack_size);
        *stack = (void*)((uint64_t)*stack - (uint64_t)*stack_size); // mlibc want the first address of the stack
        *guard_size = 0;
        return 0;
    }
}
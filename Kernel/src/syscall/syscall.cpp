#include "syscall.h"

static uint64_t mutexSyscall;

extern "C" void SyscallInt_Handler(InterruptStack* Registers, uint64_t CoreID){
    Atomic::atomicSpinlock(&mutexSyscall, 0);
    Atomic::atomicLock(&mutexSyscall, 0);

    uint64_t syscall = (uint64_t)Registers->rax;
    uint64_t arg0 = (uint64_t)Registers->rdi;
    uint64_t arg1 = (uint64_t)Registers->rsi;
    uint64_t arg2 = (uint64_t)Registers->rdx;
    uint64_t arg3 = (uint64_t)Registers->r10;
    uint64_t arg4 = (uint64_t)Registers->r8;
    uint64_t arg5 = (uint64_t)Registers->r9;

    void* returnValue = 0;
    TaskContext* task = &globalTaskManager->NodeExecutePerCore[CoreID]->Content;

    switch(syscall){
        case Sys_CreatShareMemory:
            //creat share memory
            returnValue = (void*)Memory::CreatSharing(&task->paging, arg0, (uint64_t*)arg1, (uint64_t*)arg2, (bool)arg3, task->Priviledge);
            //this function return the first physciall address of the sharing memory, it's the key to get sharing
            break;
        case Sys_GetShareMemory:
            //get share memory
            returnValue = (void*)Memory::GetSharing(&task->paging, (void*)arg0, (uint64_t*)arg1, task->Priviledge);
            break;
        case Sys_CreatSubtask: 
            //creat subTask 
            globalTaskManager->CreatSubTask(task->NodeParent, (void*)arg0, (DeviceTaskAdressStruct*)arg1);
            break;
        case Sys_ExecuteSubtask: 
            //execute subTask
            globalTaskManager->ExecuteSubTask(Registers, CoreID, (DeviceTaskAdressStruct*)arg0, (Parameters*)arg1);

            Atomic::atomicUnlock(&mutexSyscall, 0);
            return;
        case Sys_Exit:
            //exit
            if(!task->IsTaskInTask){
                if(arg0 != NULL){
                    globalLogs->Error("App %s close with error code : %x", task->Name, arg0);
                }else{
                    globalLogs->Successful("App %s close Successfuly", task->Name);
                }
                task->Exit();
                globalTaskManager->Scheduler(Registers, CoreID);
            }else{
                if(arg0 != NULL){
                    globalLogs->Error("Subtask %s close with error code : %x", task->Name, arg0);
                }else{
                    globalLogs->Successful("Subtask %s close Successfuly", task->Name);
                }
                returnValue = task->ExitTaskInTask(Registers, CoreID, (void*)arg1);
            }

            Registers->rdi = returnValue;
            
            Atomic::atomicUnlock(&mutexSyscall, 0);
            return;
        case Sys_Map:
            //mmap
            if(task->Priviledge <= DevicesRing){
                returnValue = (void*)mmap(&task->paging, (void*)arg0, (void*)arg1);    
            }else{
                returnValue = (void*)0;
            }
            break;
        case Sys_Unmap:
            //munmap
            if(task->Priviledge <= DevicesRing){
                returnValue = (void*)munmap(&task->paging, (void*)arg0);  
            }else{
                returnValue = (void*)0;
            }

            break;
        default:
            globalLogs->Error("Unknown syscall %x", syscall);
            break;
    }

    Registers->rdi = returnValue;

    Atomic::atomicUnlock(&mutexSyscall, 0);  
}

uint64_t KernelRuntime(TaskContext* task, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5){
    uint64_t returnValue = 0;
    switch(arg0){
        case 0:
            returnValue = LogHandler(arg1, (char*)arg2);
            break;
        default:
            returnValue = 0;
            break;
    }

    return returnValue;
}

uint64_t LogHandler(uint64_t type, char* str){
    switch(type){
        case 0:
            globalLogs->Message(str);
            break;
        case 1:
            globalLogs->Error(str);
            break;
        case 2:
            globalLogs->Warning(str);
            break;
        case 3:
            globalLogs->Successful(str);
            break;
    }

    return 1;
}

uint64_t mmap(PageTableManager* pageTable, void* addressPhysical, void* addressVirtual){
    addressVirtual = (void*)(addressVirtual - (uint64_t)addressVirtual % 0x1000);
    if((uint64_t)addressVirtual < HigherHalfAddress){
        pageTable->MapMemory(addressVirtual, addressPhysical);
    }
    return 1;
}

uint64_t munmap(PageTableManager* pageTable, void* addressVirtual){
    addressVirtual = (void*)(addressVirtual - (uint64_t)addressVirtual % 0x1000);
    if((uint64_t)addressVirtual < HigherHalfAddress){
        pageTable->UnmapMemory(addressVirtual);
    }
    return 1;
}

#pragma once

#include <core/main.h>
#include <kot/types.h>

#define Time_Srv_Version 0x1

KResult InitialiseServer();

KResult SetTimePointerKeySrv(kot_thread_t Callback, uint64_t CallbackArg, kot_key_mem_t TimePointerKey);
KResult SetTickPointerKeySrv(kot_thread_t Callback, uint64_t CallbackArg, kot_key_mem_t TickPointerKey, uint64_t TickPeriod);
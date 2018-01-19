#include <wiiu/os/dynload.h>
#include <wiiu/os/debug.h>

/* First declare headers */
#define DECL(res, func, ...)	res (* func)(__VA_ARGS__) __attribute__((section(".data"))) = 0;

#include "coreinit.h"
#include "gx2.h"
#include "vpad.h"

/* Macros magic */
#undef DECL
#define DECL(res, func, ...)			OSDynLoad_FindExport(lib_handle, 0, # func, (u32 *)&func);	\
										if(!(u32)func)												\
											OSFatal("Function " # func " is NULL");					\
										*(u32*)(((u32)&func) + 0) = (u32)func_ptr;

void InitFunctionPointers(void)
{
   OSDynLoadModule handle;
   addr_OSDynLoad_Acquire = *(void**)0x00801500;
   addr_OSDynLoad_FindExport = *(void**)0x00801504;

#include "../imports.h"

}

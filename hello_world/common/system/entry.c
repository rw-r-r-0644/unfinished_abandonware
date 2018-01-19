#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <fat.h>
#include <iosuhax.h>
#include <iosuhax_devoptab.h>
#include <iosuhax_disc_interface.h>

#include <sys/socket.h>
#include <fs/fs_utils.h>
#include <fs/sd_fat_devoptab.h>
#include <system/memory.h>
#include <system/exception_handler.h>
#include <sys/iosupport.h>

#include <wiiu/os.h>
#include <wiiu/procui.h>
#include <wiiu/vpad.h>
#include <wiiu/sysapp.h>
#include <wiiu/mcp.h>
#include <wiiu/ios.h>

#include "wiiu_dbg.h"

int main(int argc, char **argv);

void __eabi()
{

}

__attribute__((weak))
void __init(void)
{
   extern void(*__CTOR_LIST__[])(void);
   void(**ctor)(void) = __CTOR_LIST__;
   while(*ctor)
      (*ctor++)();
}


__attribute__((weak))
void __fini(void)
{
   extern void(*__DTOR_LIST__[])(void);
   void(**ctor)(void) = __DTOR_LIST__;
   while(*ctor)
      (*ctor++)();
}

//just to be able to call async
void someFunc(void *arg)
{
	(void)arg;
}

static int mcp_hook_fd = -1;
int MCPHookOpen()
{
    //take over mcp thread
    mcp_hook_fd = MCP_Open();
    if(mcp_hook_fd < 0)
        return -1;
    IOS_IoctlAsync(mcp_hook_fd, 0x62, (void*)0, 0, (void*)0, 0, someFunc, (void*)0);
    //let wupserver start up
    OSSleepTicks(sec_to_ticks(1));
    if(IOSUHAX_Open("/dev/mcp") < 0)
    {
        MCP_Close(mcp_hook_fd);
        mcp_hook_fd = -1;
        return -1;
    }
    return 0;
}

void MCPHookClose()
{
    if(mcp_hook_fd < 0)
        return;
    //close down wupserver, return control to mcp
    IOSUHAX_Close();
    //wait for mcp to return
    OSSleepTicks(sec_to_ticks(1));
    MCP_Close(mcp_hook_fd);
    mcp_hook_fd = -1;
}


/* HBL elf entry point */
void InitFunctionPointers(void);
int __entry_menu(int argc, char **argv)
{
   InitFunctionPointers();
   memoryInitialize();

   int res = IOSUHAX_Open(NULL);
   if(res < 0) res = MCPHookOpen();
   if(res < 0) mount_sd_fat("sd");
   else fatInitDefault();

   __init();
   int ret = main(argc, argv);
   __fini();

   if(!(res < 0)) {
      fatUnmount("sd");
      fatUnmount("usb");
      IOSUHAX_sdio_disc_interface.shutdown();
      IOSUHAX_usb_disc_interface.shutdown();
      if(mcp_hook_fd >= 0) MCPHookClose();
      else IOSUHAX_Close();
   } else unmount_sd_fat("sd:/");
   
   memoryRelease();
   return ret;
}

/* RPX entry point */
__attribute__((noreturn))
void _start(int argc, char **argv)
{
   memoryInitialize();
   
   int res = IOSUHAX_Open(NULL);
   if(res < 0) res = MCPHookOpen();
   if(res < 0) mount_sd_fat("sd");
   else fatInitDefault();

   __init();
   int ret = main(argc, argv);
   __fini();

   if(!(res < 0)) {
      fatUnmount("sd");
      fatUnmount("usb");
      IOSUHAX_sdio_disc_interface.shutdown();
      IOSUHAX_usb_disc_interface.shutdown();
      if(mcp_hook_fd >= 0) MCPHookClose();
      else IOSUHAX_Close();
   } else unmount_sd_fat("sd:/");
   
   memoryRelease();
   SYSRelaunchTitle(argc, argv);
   exit(ret);
}

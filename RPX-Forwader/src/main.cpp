#include <coreinit/filesystem.h>
#include <coreinit/foreground.h>
#include <coreinit/debug.h>
#include <coreinit/core.h>
#include <proc_ui/procui.h>
#include <nsysnet/socket.h>
#include <sys/dirent.h>
#include <sys/iosupport.h>
#include <sys/param.h>
#include <sysapp/launch.h>

#include <algorithm>
#include <string>
#include "system/memory.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/StringTools.h"
#include "fs/DirList.h"
#include "fs/fs_utils.h"
#include "common/common.h"
#include "kernel/gx2sploit.h"
#include "HomebrewMemory.h"
#include "fs/CFile.hpp"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>


int LoadHomebrew(std::string filepath) {
    if(filepath.empty())
        return -1;

    CFile file(filepath, CFile::ReadOnly);
    if(!file.isOpen())
        return -2;
	
    u32 bytesRead = 0;
    u32 fileSize = file.size();
    unsigned char *buffer = (unsigned char*) memalign(0x40, (fileSize + 0x3F) & ~0x3F);
    if(!buffer)
        return -3;
	
    while(bytesRead < fileSize) {
        u32 blockSize = 0x8000;
        if(blockSize > (fileSize - bytesRead))
            blockSize = fileSize - bytesRead;
        int ret = file.read(buffer + bytesRead, blockSize);
        if(ret <= 0)
            break;
        bytesRead += ret;
    }
	
    if(bytesRead != fileSize) {
        free(buffer);
        return -4;
    }
	
    HomebrewInitMemory();
    int ret = HomebrewCopyMemory(buffer, bytesRead);
	
    free(buffer);
	
    if(ret < 0)
        return -3;
	
    return fileSize;
}

bool AppRunning() {
	if(!OSIsMainCore()) ProcUISubProcessMessages(true);
	else {
		ProcUIStatus status = ProcUIProcessMessages(true);
		if      (status == PROCUI_STATUS_EXITING) { ProcUIShutdown(); return false; } // Being closed, deinit, free, and prepare to exit   
		else if (status == PROCUI_STATUS_RELEASE_FOREGROUND) ProcUIDrawDoneRelease(); // Free up MEM1 to next foreground app, deinit screen, etc.
	}
	return true;
}

void RequestExit() {
	ProcUIInit(OSSavesDone_ReadyToRelease);
	SYSLaunchMenu();
	while(AppRunning());
}

extern "C" int Menu_Main(void) //! Entry point
{
	if (LAUNCHED_ELF == 1) { //! ELF already ran: exit
		LAUNCHED_ELF = 0;		
		RequestExit();
		return 0;
	}
    socket_lib_init();
	
    if(CheckKernelExploit() == 0) //! Perform kernel exploit (if needed)
        return 0;
    
    HomebrewInitMemory();								//! Initialize homebrew memory layout
    if(LoadHomebrew("fs:/vol/content/load.elf") < 0) {	//! Load homebrew
		RequestExit();
		return 0;
	}
	LAUNCHED_ELF = 1;

	log_deinit();	
	SYSRelaunchTitle(0, 0); //! Relaunch into Homebrew elf file
    return 0;
}


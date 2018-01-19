#include <3ds.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <arpa/inet.h>

#include "endian.h"
#include "wiiucon.h"

#define SERVER_PORT 2424
#define PACKET_DATA_MAGIC 0x7A391F27

char IPAddress[16] = {0};

typedef struct __attribute__((packed)) {
	u32 magic;
	u32 kHeldWiiU;
	u32 kDownWiiU;
	u32 kUpWiiU;
	u16 lStickX;
	u16 lStickY;
} packetData;
packetData WiiUPacketData;

void getIpAddress()
{
	static SwkbdState swkbd;
	
	swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 15);
	swkbdSetValidation(&swkbd, SWKBD_ANYTHING, 0, 0);
	swkbdSetFeatures(&swkbd, SWKBD_FIXED_WIDTH);
	swkbdSetNumpadKeys(&swkbd, L'.', 0);
	swkbdSetHintText(&swkbd, "Enter IP Address");
	swkbdInputText(&swkbd, IPAddress, sizeof(IPAddress));
}

int main(int argc, char **argv)
{
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	u32 kDownOld = 0, kHeldOld = 0, kUpOld = 0;
	circlePosition cDataOld = {0, 0};

	getIpAddress();
	
	/* Initilize network */
	printf("Initializing Socket Library...\n");
	gfxFlushBuffers();
	gfxSwapBuffers();
	
	u32* SOC_buffer = memalign(0x1000, 0x100000);
	socInit(SOC_buffer, 0x100000);
	
	/* Open UDP socket */
	printf("Opening UDP Socket to %s:%lu...\n", IPAddress, (u32)SERVER_PORT);
	gfxFlushBuffers();
	gfxSwapBuffers();
	
	s32 udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	struct sockaddr_in connect_addr;	
	connect_addr.sin_family = AF_INET;
	connect_addr.sin_port = htons(SERVER_PORT);
	inet_aton(IPAddress, &connect_addr.sin_addr);
	
	connect(udp_socket, (struct sockaddr*)&connect_addr, sizeof(connect_addr));
		
	printf("Press Start + Select to exit\n");

	while(aptMainLoop())
	{
		hidScanInput();

		u32 kHeld = hidKeysHeld();
		u32 kDown = hidKeysDown();
		u32 kUp = hidKeysUp();
		
		if(kDown & (KEY_START + KEY_SELECT) == (KEY_START + KEY_SELECT))
			break;
				
		circlePosition cData;
		hidCircleRead(&cData);

		// Only send data when it changes
		if(kHeld != kHeldOld || kDown != kDownOld || kUp != kUpOld || cData.dx != cDataOld.dx || cData.dy != cDataOld.dy)
		{
			WiiUPacketData.magic = 		fixEndian32(PACKET_DATA_MAGIC);
			WiiUPacketData.kHeldWiiU =	fixEndian32(getKeysWiiU(kHeld));
			WiiUPacketData.kDownWiiU =	fixEndian32(getKeysWiiU(kDown));
			WiiUPacketData.kUpWiiU =	fixEndian32(getKeysWiiU(kUp));
			WiiUPacketData.lStickX =	fixEndian16(cData.dx);
			WiiUPacketData.lStickY =	fixEndian16(cData.dy);
			
			// Send the packet
			send(udp_socket, &WiiUPacketData, sizeof(packetData), 0);
			
			kHeldOld = kHeld;
			kDownOld = kDown;
			kUpOld = kUp;
			cDataOld = cData;
		}

		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	closesocket(udp_socket);
	gfxExit();
	return 0;
}

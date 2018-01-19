#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <fat.h>
#include <wiiu/vpad.h>
#include <wiiu/os.h>
#include <wiiu/curl.h>
#include <sys/socket.h>
#include "system/exception_handler.h"
#include "system/logger.h"
#include "system/wiiu_dbg.h"
#include "fs/fs_utils.h"
#include "screen.h"

#pragma pack(1)

enum ContentType
{
	CONTENT_REQUIRED=	(1<< 0),	// not sure
	CONTENT_SHARED	=	(1<<15),
	CONTENT_OPTIONAL=	(1<<14),
};

typedef struct
{
	uint16_t IndexOffset;	//	0	 0x204
	uint16_t CommandCount;	//	2	 0x206
	uint8_t	SHA2[32];		//	12 0x208
} ContentInfo;

typedef struct
{
	uint32_t ID;					//	0	 0xB04
	uint16_t Index;			//	4	0xB08
	uint16_t Type;				//	6	 0xB0A
	uint64_t Size;				//	8	 0xB0C
	uint8_t	SHA2[32];		//	16 0xB14
} Content;

typedef struct
{
	uint32_t SignatureType;				// 0x000
	uint8_t	Signature[0x100];			// 0x004

	uint8_t	Padding0[0x3C];				// 0x104
	uint8_t	Issuer[0x40];				// 0x140

	uint8_t	Version;					// 0x180
	uint8_t	CACRLVersion;				// 0x181
	uint8_t	SignerCRLVersion;			// 0x182
	uint8_t	Padding1;					// 0x183

	uint64_t	SystemVersion;			// 0x184
	uint64_t	titleID;				// 0x18C 
	uint32_t	TitleType;				// 0x194 
	uint16_t	GroupID;				// 0x198 
	uint8_t		Reserved[62];			// 0x19A 
	uint32_t	AccessRights;			// 0x1D8
	uint16_t	TitleVersion;			// 0x1DC 
	uint16_t	ContentCount;			// 0x1DE 
	uint16_t 	BootIndex;				// 0x1E0
	uint8_t		Padding3[2];			// 0x1E2 
	uint8_t		SHA2[32];				// 0x1E4
	
	ContentInfo ContentInfos[64];

	Content Contents[];

} TitleMetaData;

int getFilesize(FILE * f)
{
	fseek(f, 0, SEEK_END);
	int s = ftell(f);
	rewind(f);
	return s;
}

char *vaCreateString(const char *fmt, va_list ap)
{
    va_list ap1;
    va_copy(ap1, ap);
    size_t n = vsnprintf(NULL, 0, fmt, ap1) + 1;
    va_end(ap1);
    char * s = calloc(1, n);
	vsnprintf(s, n, fmt, ap);
	return s;
}

char *createString(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char * s = vaCreateString(fmt, ap);
    va_end(ap);
    return s;
}

void readBuff(FILE * f, void * buf, long int offset, size_t size)
{
	int old_offset = ftell(f);
	fseek(f, offset, SEEK_SET);
	fread(buf, 1, size, f);
	fseek(f, old_offset, SEEK_SET);
}

size_t writeData(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

uint64_t getFile(const char *url, const char *filename)
{
	int downloadedSize;
    CURL *curl = curl_easy_init();
    if (curl)
	{
        FILE * fp = fopen(filename, "wb+");
		if (fp == NULL)
		{
			return -1;
		}
		
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
		
		downloadedSize = ftell(fp);
        fclose(fp);
    }
	
	// Prevent creating empty files
	if (downloadedSize == 0)
		remove(filename);

	return downloadedSize;
}

volatile int exitRequest = 0;

// This thread checks for Home Button press on Core 0
int HomeButtonListenerThread(int argc, const char **argv) {
	int vpadReadCounter = 0;
	int vpadError = -1;
	VPADStatus vpad;
	while (1) {
        if(++vpadReadCounter >= 20) {
			vpadReadCounter = 0;
			VPADRead(0, &vpad, 1, &vpadError);
			if(vpadError == 0 && ((vpad.trigger | vpad.hold) & VPAD_BUTTON_HOME)) {
				break;
			}
        }
	}
	exitRequest = 1;
}

#define exitCheck()	if(exitRequest){screenClear();return 0;}
#define exitPrint(x) {screenPrint(x);int e;VPADStatus v;while(1){VPADRead(0,&v,1,&e);if(v.hold&VPAD_BUTTON_HOME)break;}return 1;}

int main(int argc, char **argv) {
	setup_os_exceptions();
	socket_lib_init();
	OSScreenInit();
	screenInit();
	
	// Listen for Home Menu Press on Core 0
	OSThread *threadCore0 = OSGetDefaultThread(0);
	OSRunThread(threadCore0, &HomeButtonListenerThread, 0, NULL);
	
	char *titleID = malloc(1024);
	char *titleName = malloc(1024);
	strcpy(titleID, TitleList[549].titleID);
	strcpy(titleName, TitleList[549].titleName);
	
	exitCheck();
	
	screenClear();
	screenPrint("--------------------- TinyNUS for Wii U ---------------------");
	screenPrint("Title ID: %s", titleID);
	screenPrint("Title Name: %s", titleName);
	screenSetPrintLine(screenGetPrintLine() + 2);
	
	//! Create and enter download folder
	char *downloadDir = createString("sd:/install/%s", titleName);
	createSubfolder(downloadDir);
	chdir(downloadDir);
	free(downloadDir);
	
	
	//! Download title's metadata
	screenPrint("[1/3] Downloading title.tmd...");
	char *tmdUrl = createString("http://ccs.cdn.wup.shop.nintendo.net/ccs/download/%s/tmd", titleID);
	if (getFile(tmdUrl, "title.tmd") > 0) {
		screenPrintAt(59, screenGetPrintLine(), "[OK]");
	} else {
		exitPrint("title.tmd download failed");
	}
	free(tmdUrl);
	
	//! Parse title's metadata
	FILE *tmdFile = fopen("title.tmd", "rb");
	if (tmdFile == NULL) {
		exitPrint("Error trying to open title.tmd");
	}

	int tmdSize = getFilesize(tmdFile);

	uint8_t *tmdData = malloc(tmdSize);
	if(fread(tmdData, 1, tmdSize, tmdFile) != tmdSize) {
		exitPrint("Failed to read title.tmd");
	}
	fclose(tmdFile);
	
	TitleMetaData *tmd = (TitleMetaData *)tmdData;
	
	//! Download tik file when present, else use "the other source"
	screenPrint("[2/3] Downloading title.tik...");
	char *tikUrl = createString("http://ccs.cdn.wup.shop.nintendo.net/ccs/download/%s/cetk", titleID);
	
	if (getFile(tikUrl, "title.tik") > 0) {
		screenPrintAt(59, screenGetPrintLine(), "[OK]");
	} else {
		exitPrint("No ticket found!");
		/*		
		free(tikUrl);
		tikUrl = createString([...], titleID);
		
		if (getFile(tikUrl, "title.tik") > 0) {
			screenPrintAt(59, screenGetPrintLine(), "[OK]");
		} else {
			screenPrint("Failed to download title.tik");
		}
		*/
	}
	free(tikUrl);
	
	//! Generate cert file
	screenPrint("[3/3] Generating title.cert...");
	
	uint8_t * defaultcert = malloc(0x300);
	uint8_t * tmd_cert1   = malloc(0x400);
	uint8_t * tmd_cert2   = malloc(0x300);
	
	FILE * cetk = fopen("title.tik", "rb");
	if (cetk != NULL && (getFilesize(cetk) >= (0x650 + 0x400)))
	{
		readBuff(cetk, defaultcert, 0x350, 0x300);
		fclose(cetk);
	}
	else
	{
		// The tiked didn't include a cert: use OSv10 cert
		getFile("http://ccs.cdn.wup.shop.nintendo.net/ccs/download/000500101000400A/cetk", "OSv10cetk");
		FILE * OSv10cetk = fopen("OSv10cetk", "rb");
		readBuff(OSv10cetk, defaultcert, 0x350, 0x300);
		fclose(OSv10cetk);
		remove("OSv10cetk");
		screenPrint("Got missing cert from OSv10 title");
	}
	
	int tmd_cert_offset = 0xB04 + (sizeof(Content) * tmd->ContentCount);	
	memcpy(tmd_cert2, &tmdData[tmd_cert_offset], 0x300);
	memcpy(tmd_cert1, &tmdData[tmd_cert_offset + 0x300], 0x400);
	
	// Write cert to file
	FILE * title_cert = fopen("title.cert", "wb");
	fwrite(tmd_cert1,   1, 0x400, title_cert);
	fwrite(tmd_cert2,   1, 0x300, title_cert);
	fwrite(defaultcert, 1, 0x300, title_cert);
	fclose(title_cert);
	
	// Free buffers
	free(tmd_cert2);
	free(tmd_cert1);
	free(defaultcert);
		
	screenPrintAt(59, screenGetPrintLine(), "[OK]");
	screenSetPrintLine(screenGetPrintLine() + 2);
	
	exitCheck();
	
	//! Download contents
	for(int i = 0; i < tmd->ContentCount; i++)
	{
		if (tmd->Contents[i].Size > 1048576) // Use Mb
			screenPrint("[%" PRIu16 "/%" PRIu16 "] Downloading content %08x... [size: %" PRIu64 "Mb]", tmd->Contents[i].Index + 1, tmd->ContentCount, tmd->Contents[i].ID, (tmd->Contents[i].Size / 1048576));
		else if (tmd->Contents[i].Size > 1024) // Use Kb
			screenPrint("[%" PRIu16 "/%" PRIu16 "] Downloading content %08x... [size: %" PRIu64 "Kb]", tmd->Contents[i].Index + 1, tmd->ContentCount, tmd->Contents[i].ID, (tmd->Contents[i].Size / 1024));
		else // Use byte
			screenPrint("[%" PRIu16 "/%" PRIu16 "] Downloading content %08x... [size: %" PRIu64 "byte]", tmd->Contents[i].Index + 1, tmd->ContentCount, tmd->Contents[i].ID, tmd->Contents[i].Size); 
		
		char * url = createString("http://ccs.cdn.wup.shop.nintendo.net/ccs/download/%016" PRIx64 "/%08x", tmd->titleID, tmd->Contents[i].ID);
		char * path = createString("%08x.app", tmd->Contents[i].ID);
		
		if(getFile(url, path) != tmd->Contents[i].Size)
			exitPrint("Error downloading content!");
		
		url = realloc(url, strlen(url) + 4);
		strcat(url, ".h3");
		free(path);
		path = createString("%08x.h3", tmd->Contents[i].ID);
		
		getFile(url, path);
		
		free(path);
		free(url);

		screenPrintAt(59, screenGetPrintLine(), "[OK]");
		
		exitCheck();
	}
	screenPrint("-> Download done!!");
	
	free(tmdData);
	
	while(!exitRequest);
	screenClear();
	return 0;
}


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "math.h"

#include <rfb/rfbclient.h>
#include "inkview.h"
#include "utils.h"

#define HOST_MAXLEN 255

char server_host[HOST_MAXLEN];
char encode_type[HOST_MAXLEN];

int client_width, client_height;
bool working = false;

ibitmap *frame=NULL;
ifont *font_title=NULL, *font_label=NULL;
rfbClient* client = NULL;
pthread_t thread_vnc, thread_draw;


struct {
	float scale;
	int w;
	int h;
	int img_offset_x;
	int img_offset_y;
} stretch;

void *draw_thread_routine(void * arg);


void prepareBitmap(int w, int h, int depth)
{
	int scanline = w*(depth/8);

	frame = malloc(sizeof(ibitmap)+scanline*h);

	frame->width = w;
	frame->height = h;
	frame->depth = depth;
	frame->scanline = scanline;
	memset(frame->data, 0xBB, frame->scanline*frame->height);

	/*for (int row=0; row<100; row++)
	{
		for (int col=0; col<100; col++)
		{
			int pix_idx = col*w+row;
			unsigned char *pix = frame->data+(pix_idx*depth/8);
			pix[0] = 0xFF;
			pix[1] = 0x00;
			pix[2] = 0x00;
		}
	}*/

    double apectRatioWidget = (double)ScreenWidth()/ScreenHeight();
    double apectRatioImg = (double)frame->width/frame->height;

	stretch.img_offset_x = 0;
	stretch.img_offset_y = 0;


    if (apectRatioImg>=apectRatioWidget) //image wider than screen, blank up-down
	{
		stretch.scale = (float)ScreenWidth()/(float)frame->width;
		stretch.w = frame->width*stretch.scale;
		stretch.h = frame->height*stretch.scale;
		stretch.img_offset_y = (ScreenHeight()-stretch.h)/2;

	}
	else if (apectRatioImg<apectRatioWidget) //image higher than screen, blank left-right
	{
		stretch.scale = (float)ScreenHeight()/(float)frame->height;
		stretch.w = frame->width*stretch.scale;
		stretch.h = frame->height*stretch.scale;
		stretch.img_offset_x = (ScreenWidth()-stretch.w)/2;
	}

	pthread_create(&thread_draw, NULL, &draw_thread_routine, NULL);
}

void drawVncFramebuffer(rfbClient* client, int x, int y, int w, int h)
{
	rfbPixelFormat* pf=&client->format;
	int bpp_src=pf->bitsPerPixel/8;
	int row_stride_src=client->width*bpp_src;

	static time_t t=0,t1;

	
	/* assert bpp=4 */
	if(bpp_src!=4 && bpp_src!=2) {
		rfbClientLog("bpp = %d (!=4)\n",bpp_src);
		return;
	}

	if (!frame)
		prepareBitmap(client->width, client->height, 24);

	int bpp_dst=24/8;
	int row_stride_dst=frame->width*bpp_dst;


	int i, j;
	for(j=0;j<client->height;j++)
		for(i=0;i<client->width;i++) {
			unsigned char* p=client->frameBuffer+j*row_stride_src+i*bpp_src;
			unsigned int v;
			if(bpp_src==4)
				v=*(unsigned int*)p;
			else if(bpp_src==2)
				v=*(unsigned short*)p;
			else
				v=*(unsigned char*)p;
			
			unsigned char *pix_dst = frame->data+j*row_stride_dst+i*bpp_dst;
			/*double r = (double)(v>>pf->redShift);
			double g = (double)(v>>pf->greenShift);
			double b = (double)(v>>pf->blueShift);
			pix_dst[0] = (unsigned char)r;
			pix_dst[1] = (unsigned char)g;
			pix_dst[2] = (unsigned char)b;*/

			pix_dst[0] = (v>>pf->redShift);
			pix_dst[1] = (v>>pf->greenShift);
			pix_dst[2] = (v>>pf->blueShift);
		}

	t1=time(NULL);
	if(t1-t>1)
		t=t1;
	else
		return;

}


void *vnc_thread_routine(void * arg)
{
	client = rfbGetClient(8,3,4);

	client->GotFrameBufferUpdate = drawVncFramebuffer;

	char str[255];
	sprintf(str, "Connecting to host %s", server_host);
	DrawString(10, 10, str);
	SoftUpdate();


	//data->encodingsString="tight zrle ultra copyrect hextile zlib corre rre raw";

	int argc = 4;
	char* args[] = { "", strdup(server_host), strdup("-encodings"), strdup(encode_type), NULL};
	if (!rfbInitClient(client,&argc,args))
	{
		sprintf(str, "Failed to connect to %s", server_host);
		DrawString(10, 40, str);
		SoftUpdate();
		return NULL;
	}

	DrawString(10, 40, "Connected");
	SoftUpdate();

	while (working) {
		int n = WaitForMessage(client,50);
		if(n < 0)
			break;
		if(n)
		    if(!HandleRFBServerMessage(client))
			break;
	}

	rfbClientCleanup(client);
	return NULL;
}

void stop()
{
	working = false;
	pthread_join(thread_vnc, NULL);
	pthread_join(thread_draw, NULL);
	CloseApp();
}

void *draw_thread_routine(void * arg)
{
	ClearScreen();
	SoftUpdate();
	while (working)
	{
		if (frame)
		{
			ibitmap *bmp_s = BitmapStretchCopy(frame, 0,0, frame->width, frame->height, stretch.w, stretch.h);
			DrawBitmap(stretch.img_offset_x,stretch.img_offset_y, bmp_s);
			SoftUpdate();
			free(bmp_s);
			usleep(1500*1000);
		}
	}
	return NULL;
}


bool readCfg()
{
	FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

	enum
	{
		LINE_HOST=0,
		LINE_ENCODE
	};

	char config_filename[HOST_MAXLEN];

	get_default_config_path("pbvnc.cfg",
					config_filename, HOST_MAXLEN);

    fp = fopen(config_filename, "r");
    if (fp == NULL)
		return false;

	int lineCount = 0;

    while ((read = getline(&line, &len, fp)) != -1) {
        switch (lineCount)
		{
			case LINE_HOST:
				strcpy(server_host, line);
			case LINE_ENCODE:
				strcpy(encode_type, line);
				break;
			default:
				goto read_exit;
			break;
		}
		lineCount++;
    }
read_exit:
	if (fp)
    	fclose(fp);
    if (line)
        free(line);
	return true;
}

int main_handler(int type, int par1, int par2) {
	switch (type) {
		case EVT_INIT:
			SetPanelType(0);
			SetOrientation(1);
			font_title = OpenFont("cour", 30, 1);
			font_label = OpenFont("cour", 20, 1);
			SetFont(font_label, 0x000000);
			break;
		case EVT_SHOW:
			ClearScreen();
			if (readCfg())
			{
				working = true;
				pthread_create(&thread_vnc, NULL, &vnc_thread_routine, NULL);
			}
			else
				DrawString(0, 10, "Failed to read pbvnc.cfg");
			SoftUpdate();
			break;
		case EVT_KEYPRESS:
			switch (par1) {
				case IV_KEY_LEFT:
				case IV_KEY_PREV:
					stop();
					break;
				case IV_KEY_RIGHT:
				case IV_KEY_NEXT:
					if (client)
					{
						SendKeyEvent(client, XK_Return, true);
						usleep(300*1000);
						SendKeyEvent(client, XK_Return, false);
					}
					break;
				}
			break;
		case EVT_POINTERMOVE:
		case EVT_POINTERDOWN:
			if (client)
			{
				int clientX = (par1-stretch.img_offset_x)/(float)stretch.scale;
				int clientY = (par2-stretch.img_offset_y)/(float)stretch.scale;
				SendPointerEvent(client, clientX, clientY, rfbButton1Mask);
			}
			break;
		case EVT_POINTERUP:
			if (client)
			{
				int clientX = (par1-stretch.img_offset_x)/(float)stretch.scale;
				int clientY = (par2-stretch.img_offset_y)/(float)stretch.scale;
				SendPointerEvent(client, clientX, clientY, 0);
			}
		break;
	}
	printf("EVENT!\n");
	return 0;
}

int main() {
	OpenScreen();
	InkViewMain(&main_handler);
	return 0;
}

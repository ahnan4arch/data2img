// data2img.cpp : Defines the entry point for the console application.
//
#ifdef __GNUC__
#include "SDL2/SDL.h"
#else
#include "SDL.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//TODO Input from serial port, more formats support.

typedef enum{
	IF_UNKNOW = 0,
	IF_RGB565,
	IF_BGR565,

	IF_XRGB444,
	IF_RGBX444,
	IF_RXGXB444,

	IF_RGB888,
	IF_RGBA888,
	IF_ARGB888,

	IF_BGR888,
	IF_ABGR888,
	IF_BGRA888,

	IF_YUI422,
} ImageFromat;

typedef enum{
	FF_UNKNOW = 0,
	FF_BIN,
	FF_SPLIT,
}FileFormat;

SDL_Window *mainwindow = NULL;
SDL_Surface *imagesurface = NULL;

char *srcFilePath = NULL;
FileFormat srcFileFormat = FF_UNKNOW;
ImageFromat srcImageFormat = IF_UNKNOW;
int imageDepth = 0;
int imageBpp = 0;
int reverseBytes = 0;
uint32_t rmask, gmask, bmask, amask;
char *splitString = NULL;

char *destFile = NULL;
size_t imgWidth = 0;
size_t imgHeith = 0;

int displayInWindow = 0;

void displaySurface()
{
	SDL_Surface *winsurface = NULL;
	SDL_Event evt;

	mainwindow = SDL_CreateWindow("DATA TO IMAGE", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, imagesurface->w, imagesurface->h, SDL_WINDOW_SHOWN);
	if (mainwindow == NULL)
	{
		fprintf(stderr, "Error: Failed to create window! %s\n", SDL_GetError());
		return;
	}

	winsurface = SDL_GetWindowSurface(mainwindow);
	if (winsurface == NULL)
	{
		fprintf(stderr, "Error: Failed to get surface of window! %s\n", SDL_GetError());
		return;
	}

	SDL_BlitSurface(imagesurface, &imagesurface->clip_rect, winsurface, &winsurface->clip_rect);

	SDL_UpdateWindowSurface(mainwindow);

	while (1)
	{
		SDL_WaitEvent(&evt);
		if (evt.type == SDL_QUIT)
		{
			break;
		}
	}

	SDL_DestroyWindow(mainwindow);
}

int procReverseBytes(uint8_t *buf, size_t len, int bpp)
{
	if ((len % bpp) != 0 || buf == NULL || len == 0 || bpp == 0)
	{
		return 1;
	}

	if (bpp == 1)
	{
		return 0;
	}

	uint8_t *tmpbuf = NULL;
	uint8_t tmpbyte = 0;

	for (size_t i = 0; i < len / bpp; i++)
	{
		tmpbuf = buf + i * bpp;
		for (size_t j = 0; j < (size_t)(bpp/2); j++)
		{
			tmpbyte = tmpbuf[j];
			tmpbuf[j] = tmpbuf[bpp - 1 - j];
			tmpbuf[bpp - 1 - j] = tmpbyte;
		}
	}

	return 0;
}

int loadFile()
{
	uint8_t *lineBuf = NULL;
	uint8_t *imgBuf = NULL;
	FILE *infp = NULL;

	infp = fopen(srcFilePath, "rb");
	if (infp == NULL)
	{
		fprintf(stderr, "Error: Failed to open %s!  %s\n", srcFilePath, strerror(errno));
		return 1;
	}

	imagesurface = SDL_CreateRGBSurface(0, imgWidth, imgHeith, imageDepth, rmask, gmask, bmask, amask);
	if (imagesurface == NULL)
	{
		fprintf(stderr, "Error: Failed to create surface! %s\n", SDL_GetError());
		goto ABORT;
	}

	imgBuf = imagesurface->pixels;
	lineBuf = malloc(imgWidth * imageBpp);
	if (lineBuf == NULL)
	{
		fprintf(stderr, "Error: Failed to alloc mamory for lineBuf!\n");
		goto ABORT;
	}

	for (size_t i = 0; i < imgHeith; i++)
	{
		if (fread(lineBuf, imgWidth * imageBpp, 1, infp) != 1){
			if (feof(infp) != 0)
			{
				fprintf(stderr, "Error: Failed to read file! File is too small! (EOF)\n");
			}
			else{
				fprintf(stderr, "Error: Failed to read file!  %s\n", strerror(errno));
			}
			goto ABORT;
		}
		if (reverseBytes == 1)
		{
			if (procReverseBytes(lineBuf, imageBpp * imgWidth, imageBpp)){
				fprintf(stderr, "Error: Failed to reverse bytes of linebuf!\n");
				goto ABORT;
			}
		}
		memcpy(imgBuf, lineBuf, imgWidth * imageBpp);
		imgBuf = imgBuf + imagesurface->pitch;
	}

	free(lineBuf);
	fclose(infp);
	return 0;

ABORT:	
	if (imagesurface != NULL)
	{
		SDL_FreeSurface(imagesurface);
		imagesurface = NULL;
	}

	if (lineBuf != NULL)
	{
		free(lineBuf);
		lineBuf = NULL;
	}

	if (infp != NULL)
	{
		fclose(infp);
	}
	
	return 1;
}


int procArguments(int argc, char *argv[])
{
	char *temp;
	int argIdx = 1;

	if (argc <= 1)
	{
		return 0;
	}

	while (1)
	{
		switch (argv[argIdx][1])
		{
		case 'i':
			argIdx++;
			srcFilePath = argv[argIdx];
			break;
		case 'o':
			argIdx++;
			destFile = argv[argIdx];
			break;
		case 'w':
			argIdx++;
			temp = argv[argIdx];
			imgWidth = atoi(temp);
			break;
		case 'h':
			argIdx++;
			temp = argv[argIdx];
			imgHeith = atoi(temp);
			break;
		case 'l':
			argIdx++;
			temp = argv[argIdx];
			if (strncmp(temp, "bin", 3) == 0)
			{
				srcFileFormat = FF_BIN;
			}
			else if (strlen(temp) == 1){
				srcFileFormat = FF_SPLIT;
				splitString = temp;
			}
			else{
				srcFileFormat = FF_UNKNOW;
			}
			break;
		case 'f':
			argIdx++;
			temp = argv[argIdx];
			if (strncmp(temp, "rgb565", 6) == 0)
			{
				imageDepth = 16;
				imageBpp = 2;
				rmask = 0xf800;
				gmask = 0x07e0;
				bmask = 0x001f;
				amask = 0x00;
				srcImageFormat = IF_RGB565;
			}
			else if (strncmp(temp, "rgbx444", 6) == 0)
			{
				imageDepth = 16;
				//srcImageFormat = IF_RGBX444;
			}
			else if (strncmp(temp, "rgb888", 6) == 0)
			{
				imageDepth = 24;
				//srcImageFormat = IF_RGB888;
			}
			else if (strncmp(temp, "yui422", 6) == 0)
			{
				imageDepth = 8;
				//srcImageFormat = IF_YUI422;
			}
			break;
		case 'd':
			displayInWindow = 1;
			break;
		case 'r':
			reverseBytes = 1;
			break;
		default:
			return 1;
		}

		argIdx++;

		if (argIdx >= argc)
		{
			break;
		}
	}
	return 0;
}

void usage()
{
	printf("data2img\n");


	printf("\t -i input file\n");
	printf("\t -o output file\n");
	printf("\t -w width\n");
	printf("\t -h heigth\n");
	printf("\t -l file format: bin, split in a char\n");
	printf("\t -f image format: rgb565 rgb444 rgb888 yui411\n");
	printf("\t -r reverse bytes of pixel\n");
	printf("\t -d display image in a window\n");
	printf("\n");
	printf("Example:  data2img -i girl.bin -o girl.bmp -w 480 -h 320 -l bin -f rgb565 -d\n");
	printf("\n");
	printf("Support: rgb565\n");
	printf("More format support in the future.\n");
	printf("\n");
	printf("Version 0.0.1\n");
}
#undef main
int main(int argc, char *argv[])
{
	if (procArguments(argc, argv)){
		usage();
		return 1;
	}

	if (srcFilePath == NULL || strlen(srcFilePath) == 0)
	{
		fprintf(stderr, "Error: Invalid input file path!\n");
		goto ABORT;
	}

	if (srcFileFormat == FF_UNKNOW)
	{
		fprintf(stderr, "Error: Invalid input file format!\n");
		goto ABORT;
	}

	if (srcFileFormat == FF_SPLIT && (splitString == NULL || strlen(splitString) != 1))
	{
		fprintf(stderr, "Error: Invalid split char!\n");
		goto ABORT;
	}

	if (srcImageFormat == IF_UNKNOW)
	{
		fprintf(stderr, "Error: Invalid image format! (May be support in the future)\n");
		goto ABORT;
	}

	if (imgWidth == 0 || imgHeith == 0)
	{
		fprintf(stderr, "Error: Invalid image size!\n");
		goto ABORT;
	}

	if (loadFile())
	{
		fprintf(stderr, "Error: Could not load file! %s\n", srcFilePath);
		goto ABORT;
	}

	if (displayInWindow)
	{
		displaySurface();
	}

	if (destFile != NULL && strlen(destFile) != 0)
	{
		if (SDL_SaveBMP(imagesurface, destFile) != 0){
			fprintf(stderr, "Error: Save to %s failen! %s\n", srcFilePath, SDL_GetError());
			goto ABORT;
		}
	}

	SDL_FreeSurface(imagesurface);
	return 0;
ABORT:
	usage();
	if (imagesurface != NULL)
	{
		SDL_FreeSurface(imagesurface);
		imagesurface = NULL;
	}
	return 1;
}


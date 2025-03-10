#include <stdio.h>
#include <strings.h>

#define VOICE_NUM 68

typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef signed long  s32;
typedef signed short s16;
typedef signed char  s8;


typedef struct {
	u16 size;
	u16 freq;
	s8 data[1];
} VoiceData;

typedef struct {
	u32 riff_mark;			 // 0x00
	u32 chunk_size;
	u32 wave_mark;
	u32 fmt_mark;
	u32 subchunk_size;		// 0x10
	u16 format;
	u16 ch;
	u32 freq;
	u32 data_rate;
	u16 block;				// 0x20
	u16 sample;
	u8  ext[14];
	u8  data_mark[4];
	u16 data_size;
	u16 data_size_h;
	u8  data[1];
} WinWaveHead;

typedef struct {
	u32 magic;
	u32 count;
	u32 total_size;
	u32 rfu;
	u32 index[256 - 4];
	u8  wav[1];
} GbaWaveHeader;
#define WAV_MAGIC 0xAA550001

s8 gBuf [1024 * 1024 * 10];
s8 gBuf2[1024 * 1024 * 10];
VoiceData* gVD = (VoiceData*)gBuf;
GbaWaveHeader* gwh = (GbaWaveHeader*)gBuf2;


int ReadWave(const char* name /*, s8* buf, size_t max*/){
	s8* buf = gBuf;
	s32 max = sizeof(gBuf);
	WinWaveHead* wwh = (WinWaveHead*)buf;
	int len = 0, sum = 0;
	FILE* fp = fopen(name, "rb");
	if(!fp){
		fprintf(stderr, "Can't open %s\n", name);
		return 0;
	}
	while((len = fread (buf + sum,1,max,fp)) > 0){
		sum += len;
		max -= len;
	}
	fclose(fp);
	fprintf(stderr, "%s: %dHz %dbit %dch len:%d", name, wwh->freq, wwh->sample, wwh->ch, wwh->data_size);
	gVD->size = (sum - 0x3a + 8 + 6) >> 4;

	//ZZZ
//	wwh->freq /= 2;
//	gVD->size /= 2;

	gVD->freq = -(16777216 / wwh->freq);
	fprintf(stderr, "->%d\n", gVD->size);
	for(len = 0 ; len < sum ; ++len){
		gVD->data[len] = wwh->data[len] - 128;
//		gVD->data[len] = wwh->data[len];
	}
	return gVD->size;
}

int PutGBAWave(s8* buf, size_t len){
	FILE* fp = fopen("Voice.bin", "wb");
	if(!fp){
		fprintf(stderr, "Can't create %s\n", "Voice.dat");
		return 0;
	}
	s32 ret = fwrite(buf, 1, len, fp);
/*
	s16 freq2 = -(16777216 / freq);
	fprintf(stderr, "GBA freq = %d\n", freq2);
	fwrite(&freq2, 1, 2, fp);
	size  = fwrite(dat, 1, size, fp);
*/
	fclose(fp);
	fprintf(stderr, "write %d/%d\n", ret, len);
}

int main(int argc, char** argv){
	u32 offset = 0;
	gwh->magic = WAV_MAGIC;
	gwh->count = VOICE_NUM;
	gwh->rfu = 0;
	int i;
	for(i = 0 ; i < VOICE_NUM ; ++i){
		char name[99];
		sprintf(name, "split\\w%03d.wav", i);
		if(ReadWave(name)){
			s32 len = gVD->size << 4;
			gwh->index[i] = offset + 1024;
			memcpy(&gwh->wav[offset], gBuf, len);
			offset += len;
		}
	}
	gwh->total_size = offset;
	PutGBAWave(gBuf2, offset + 1024);
	return 0;
}

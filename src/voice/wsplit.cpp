//#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ABS(a) (((a) < 0)? -(a) : (a))

typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef signed long  s32;
typedef signed short s16;
typedef signed char  s8;
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
} WaveHead;

#define WaveHeadSize (sizeof(WaveHead) - 2)

WaveHead gWH;
typedef signed char s8;
int ReadWave(const char* name, s8* buf, size_t max){
	WaveHead* wh = (WaveHead*)buf;
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

	if(wh->freq == 8000) wh->freq = 8192;
	fprintf(stderr, "%s: %dHz %dbit %dch len:%d\n", name, wh->freq, wh->sample, wh->ch, wh->data_size);
	return sum;
}

int PutWav(char* name, s16* dat, int len, s32 scale){
	int i;
	u32 ret = 0;
	FILE* fp = fopen(name, "wb");
	if(!fp){
		fprintf(stderr, "Can't create %s\n", name);
		return 0;
	}
	gWH.data_size = len - 2;
	gWH.data_size_h = 0;
	gWH.chunk_size = len + WaveHeadSize - 8;
	gWH.sample = 8;

	// ScaleçƒåvéZ
/*
	scale = 0;
	for(i = 0 ; i < len ; ++i){
		if(ABS(dat[i]) > scale) scale = ABS(dat[i]);
	}
*/

	fwrite(&gWH, 1, WaveHeadSize, fp);
	scale = scale * 75 / 100; // 2äÑÇËëùÇµ
//	ret = fwrite(dat, 1, len, fp);
	for(i = 0 ; i < len ; ++i){
//		s32 b = dat[i] * 128 / scale;
		s32 b = ((s32)dat[i] * 128 / scale) + 128;
		if(b < 0) b = 0;
		if(b > 255) b = 255;
		ret += fwrite(&b, 1, 1, fp);
	}
	fclose(fp);
	fprintf(stderr, "%s: write %d(scale%d)\n", name, ret, scale);
	return ret;
}

int Split(WaveHead* wh){
	int len = (wh->data_size | (wh->data_size_h << 16)) / 2;
	s16* dat = (s16*)wh->data;
	s32 min = 0, max = 0;
	int i, j, k;
	char name[99];
	fprintf(stderr, "Normalize...");
	for(i = 0 ; i < len ; ++i){
		if(dat[i] < min) min = dat[i];
		if(dat[i] > max) max = dat[i];
	}
	fprintf(stderr, "(%d..%d)", min, max);
	if(max < -min) max = -min;
	fprintf(stderr, "->%d\n", max);

//8KHz
#define BLANK_LEVEL	256
#define SPC_LEN		2000
#define VALID_LEN	1000
//16KHz

/*
#define BLANK_LEVEL	512
#define SPC_LEN		4000
#define VALID_LEN	2000
*/

	int index = 0;
	for(i = 0 ; i < len ; i++){
		// âπê∫ÇÃç≈èâÇíTÇ∑
		if(ABS(dat[i]) < BLANK_LEVEL) continue;

		// âπê∫ÇÃç≈å„ÇíTÇ∑
		k = 0;
		for(j = i ; j < len; ++j){
			if(ABS(dat[j]) < BLANK_LEVEL){
				if(++k > SPC_LEN) break;
			} else {
				k = 0;
			}
		}
		if(j == len) break; // ç≈å„ÇÕèoóÕÇµÇ»Ç¢

		k = j - i - SPC_LEN;
		if(k > VALID_LEN){
			sprintf(name, "split\\w%03d.wav", index++);
			PutWav(name, &dat[i], (k + 7) & ~7, max); // 8ÉoÉCÉgã´äE
		}
		i = j;
	}
/*
	int count = 0, index = 0;
	for(i = 0 ; i < len ; i++){
		int j;
#define ABS(n) (((n) < 0)? -(n) : (n))
		for(j = 0 ; j < 2000 && i + j < len ; ++j) {
			if(ABS(dat[i + j]) > 100) break;
		}
		if(j >= 1500){
			if(count > 1000){
				s32 align = count & 7;
				sprintf(name, "split\\w%03d.wav", index++);
				PutWav(name, &dat[i - count], count - align, max);
			}
			for(; i < len ; ++i) if(ABS(dat[i]) > 100) break;
			count = 0;
		} else {
			++count;
		}
	}
*/
	return index;
}

s8 gBuf[1024 * 1024 * 10];
int main(int argc, char** argv){
	char* name = "voice.wav";
	if(argc > 1) name = argv[1];
	WaveHead* wh = (WaveHead*)gBuf;
	fprintf(stderr, "Voice splitter\n");
	fprintf(stderr, "#1:%d\n", ReadWave(name, gBuf, sizeof(gBuf)));
	memcpy(&gWH, gBuf, sizeof(gWH));
	Split(wh);
	return 0;
}

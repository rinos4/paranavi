#include <stdio.h>

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
	fprintf(stderr, "%s: %dHz %dbit %dch len:%d\n", name, wh->freq, wh->sample, wh->ch, wh->data_size);
	for(len = 0 ; len < sum ; ++len){
		wh->data[len] = wh->data[len] - 128;
	}
	return sum;
}

int PutGBAWave(s32 size, s32 freq, u8* dat){
	FILE* fp = fopen("Voice.dat", "wb");
	if(!fp){
		fprintf(stderr, "Can't create %s\n", "Voice.dat");
		return 0;
	}
	fwrite(&size, 1, 4, fp);
	s16 freq2 = -(16777216 / freq);
	fprintf(stderr, "GBA freq = %d\n", freq2);
	fwrite(&freq2, 1, 2, fp);
	size  = fwrite(dat, 1, size, fp);
	fclose(fp);
	fprintf(stderr, "write %d\n", size);
}

s8 gBuf[1024 * 1024];
int main(int argc, char** argv){
	int i;
	WaveHead* wh = (WaveHead*)gBuf;
	fprintf(stderr, "Voice converter\n");
	fprintf(stderr, "#1:%d\n", ReadWave(argv[1], gBuf, sizeof(gBuf)));
	PutGBAWave(wh->data_size, wh->freq, wh->data);
	return 0;
}

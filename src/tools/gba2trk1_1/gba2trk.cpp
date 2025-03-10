///////////////////////////////////////////////////////////////////////////////
// ���k�g���b�N���O �� .trk�t�@�C�� �R���o�[�^ Version 1.1 
// Copyright(C) 2006-2007 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////
// gcc -O -Wa gba2trk.cpp -o gba2trk.exe    (mingw32/gcc3.4.2)

//�y���k�g���b�N���O�t�H�[�}�b�g Version 0x01�z
//PID Size:  L(Lat) o(Lon) A(Alt) S(Sec) R(Repeat) P(Precision) B(BCC)
// 0  1byte:                            RRRRRR00 (Repeat:R+2 [R:0-58])// ���S����(�ő�1��Rep)
// 1  1byte:                            LLooAA01 (Lat:   2,A: 2,S: 0) // �����ړ����A���Ԋu
// 2  2byte:                   LLLLLLoo ooooAA10 (Lat:  32,A: 2,S: 0) // ����ړ����A���Ԋu
// 3  3byte:          LLLLLLLL Looooooo ooAAA011 (Lat: 256,A: 4,S: 0) // �s����ړ��A���Ԋu
// 4  4byte: LLLLLLLL LLLLoooo oooooooo AAA00111 (Lat:2048,A: 4,S: 0) // ��������A  ���Ԋu
// 5  4byte: LLLLLLLL LLLooooo ooooooAA AAA01111 (Lat:1024,A:16,S: 0) // ���T�[�}���A���Ԋu
// 6  4byte: LLLLLLLL Looooooo ooAAAASS SSS10111 (Lat: 256,A: 8,S:16) // ���O�Ԋu�ύX
// 7  5byte: L*13 o*13 A* 7 S*0          0011111 (Lat:4096,A:64,S: 0) // �A�N���A    ���Ԋu
// 8  5byte: L*11 o*11 A* 5 S*6          0111111 (Lat:1024,A:16,S:32) // �ϊԊu���ODownload
// 9  6byte: L*15 o*15 A*11 S*0          1011111 (Lat: 16K,A:1K,S: 0) // �������A    ���Ԋu
//10  7byte: L*15 o*15 A*11 S*7         01111111 (Lat: 16K,A:1K,S:64) // UnidentifiedFlyingObj
//11 15byte: L*30 o*31 A*16 S*32    PPP 11101100 ��Βl�p�P�b�g<�p��> (P:-4�`3)   [R59]
//12 15byte: L*30 o*31 A*16 S*32    PPP 11110000 ��Βl�p�P�b�g<�V�K> (�Z�p���[�^)[R60]
//13  2byte:                   BBBBBBBB 11110100 �����p�P�b�g(BCC: 0-255)         [R61]
//14  2byte: Option*8                   11111000 1byte �I�v�V����(Total:2byte)    [R62]
//15  ?byte: Option*n*8 Type*8 nnnnnnnn 11111100 nbyte �I�v�V����(Total=n+3 byte) [R63]
//16  1byte:                            11111111 ����������(FlashROM�������)

// �𑜓x(Lat/Lon):   P0: 1ms(��3cm)�AP1: 8ms(24cm) P2: 32ms(1m) P3: 128ms(4m) ��: RFU
// 1byte�I�v�V����:   [RFU*5 ManualSeg*1 CommTimeout*1 LostFlag*1] (�Z�O�����g�����p)
// �ϐ������l:        ��K����: lat/lon/alt:0, sec:+1  (��Βl�p�P�b�g��M���Ƀ��Z�b�g)
//                    BCC(1byte): 0 (�w�b�_�܂��͌����p�P�b�g��M���Ƀ��Z�b�g)
// FlashBlock:        BlockHead = 'Trk?pppb' (Magic[24] + Version[8] + Blk#[8] + Pkt#[24])
//                    Flash�u���b�N(64KB)�̐擪�́A�p�����ł��K��BlockHead��}������(�����p)
//                    �e�u���b�N�̍ŏ��̃g���b�N�f�[�^�͐�Βl�p�P�b�g���g�p
// �����p�P�b�g:      Flash�u���b�N�����A�܂��̓T�X�y���h���Ɍ����p�P�b�g��}��(�I�v�V����)
//                    3600�p�P�b�g��1��͌����p�P�b�g��}�����ׂ���WriteVerify���Ă���̂ŕs�v?


///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

typedef unsigned char u8; // ParaNavi�\���̂Ƃ̋��ʉ��p
typedef   signed long s32;
typedef unsigned long u32;

///////////////////////////////////////////////////////////////////////////////
// .trk�t�H�[�}�b�g�o��
///////////////////////////////////////////////////////////////////////////////
// WGS84��DMS�t�H�[�}�b�g�ŏo��
const char TRACKLOGHEAD[] = // Kashmir�̏o�͂��Q�l�ɂ��Ă݂��c
	"H  SOFTWARE NAME & VERSION\n"
	"I  PCX5 2.09\n\n"
	"H  COORDINATE SYSTEM\n"
	"U  LAT LON DMS\n\n"
	"H  R DATUM                IDX DA            DF            DX            DY            DZ\n"
	"M  G WGS 84               121 +0.000000e+00 +0.000000e+00 +0.000000e+00 +0.000000e+00 +0.000000e+00\n";

const char* MONTH[13] = {
	"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};
const time_t UNIX2GARMIN = 631033200; // UNIX�b(1970)->GARMIN�b(1990)

// �o�̓t�@�C��
static FILE* gOut = 0;

// �f�o�b�O�p
static u32 gSegFlag, gSegCount, gOption = 0xff; // �f�t�H���g�͑S�ẴZ�O�����g�t���O�ŕ�������
static u32 gTrackCount, gPacketCount[20];// ���v�p�J�E���^
const  u32 PACKET_SIZE[] = {1, 1, 2, 3, 4, 4, 4, 5, 5, 6, 7, 15, 15, 2, 1, 1};

// DMS�ϊ�
s32 DivLatLon(s32 v, s32* d, s32* m, s32* s, s32* x){
	*x = (v < 0)? -v : v;
	*s  = *x / 1000;  *m  = *s / 60;  *d  = *m / 60;
	*x %=      1000;  *s %=      60;  *m %=      60;
	return v < 0;
}

// ���O�J�n���Ԃ��t�@�C�����ɂ��ĐV�K�쐬(���ꎞ���̃t�@�C���͏㏑��)
bool OpenTrackLog(time_t sec){
	// UTC�ł͂Ȃ�localtime���t�@�C�����ɂ���(Lat/Lon����n�掞�������߂��ق����ǂ�����)
	struct tm* t = localtime(&sec);
	char fname[30];
	sprintf(fname, "%04d%02d%02d-%02d%02d.trk", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
	gOut = fopen(fname, "w");
	if(!gOut){
		fprintf(stderr, "Can't create '%s'\n", fname);
		return false; // �쐬�G���[
	}

	// ���v�p�f�[�^������
	memset(gPacketCount, 0, sizeof(gPacketCount));
	gTrackCount = 0;
	gSegCount   = 0;
	gSegFlag    = 1;
	printf( "%s: ", fname);

	// ���O�t�@�C���̐擪�Ƀw�b�_������
	return fputs(TRACKLOGHEAD, gOut) >= 0;// �������݃`�F�b�N
}

// lat/lon/alt/sec -> .trk�R�����ϊ�
bool WriteTrackLog(const s32* v, s32 prc){
	if(!gOut) return false; // �O�̂��߃`�F�b�N

	// �����ϊ�
	time_t sec = v[3] + UNIX2GARMIN;
	gTrackCount++; // ���v�\���p�ɃC���N�������g���Ă���

	if(gSegFlag){ // �Z�O�����g�����t���O
		struct tm* t = localtime(&sec); // �Z�O�����g���ɂ̓��[�J���^�C�����g��
		fprintf(gOut, "\nH %04d/%02d/%02d(#%d)\n\n", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, ++gSegCount);
		gSegFlag = 0; // �Z�O�����g�ǉ��ς݃}�[�N
	}

	// �ܓx�ƌo�x�͐��x�l�ŃV�t�g���ďo��
	s32 d, m, s ,x, f;
	f = DivLatLon(v[0] << prc, &d, &m, &s, &x);
	fprintf(gOut, "T  %c%02d%02d%02d.%03d", f? 'S' : 'N', d, m, s, x);	// �ܓx
	f = DivLatLon(v[1] << prc, &d, &m, &s, &x);
	fprintf(gOut,   " %c%03d%02d%02d.%03d", f? 'W' : 'E', d, m, s, x);	// �o�x

	// �g���b�N��UTC�ŏo��
	struct tm* utc = gmtime(&sec);
    return fprintf(gOut, " %02d-%s-%02d %02d:%02d:%02d %d\n", utc->tm_mday, MONTH[utc->tm_mon], utc->tm_year % 100, utc->tm_hour, utc->tm_min, utc->tm_sec, v[2]) > 0; // �������݃`�F�b�N
}

// �t�@�C���փt���b�V��
bool CloseTrackLog(){
	if(!gOut) return true; // ���ɃN���[�Y��

	// �t�@�C���N���[�Y�`�F�b�N
	if(fclose(gOut)){
		fprintf(stderr, "fclose: failed\n");
		return false;
	}
	gOut = 0;

	// ���v����\�����Ă���
	if(gTrackCount){
		s32 sum = 0;
		for(int i = 0 ; i < 16 ; ++i) sum += PACKET_SIZE[i] * gPacketCount[i];
		printf("Track=%d Segment=%d (%.2fbyte/trk)\n", gTrackCount, gSegCount, sum / (double)gTrackCount);
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// ���k�g���b�N�f�[�^�f�R�[�h
///////////////////////////////////////////////////////////////////////////////
typedef struct{
	u32 bit[4];		// [lat/lon/alt/sec]
	u8  flag;		// ���ʃR�[�h
	u8  mask;		// ���ʃ}�X�N
	u8  mask_size;	// ���ʃT�C�Y
	s32 pid;
} PacketType; // �f�R�[�h�ɕK�v�ȃf�[�^�̂�

#define BitFill(n)  ((1 <<  (n)) - 1)
#define BitRange(n) ( 1 << ((n) - 1))
#define DEF_PKT(lat, lon, alt, sec, flag, mask, pid) {lat, lon, alt, sec, flag, BitFill(mask), mask, pid }
const PacketType PACKET_TABLE[] = {
	//     lat lon alt sec   flag ms pid
	DEF_PKT( 2,  2,  2,  0,  0x01, 2, 1),
	DEF_PKT( 6,  6,  2,  0,  0x02, 2, 2),
	DEF_PKT( 9,  9,  3,  0,  0x03, 3, 3),
	DEF_PKT(12, 12,  3,  0,  0x07, 5, 4),
	DEF_PKT(11, 11,  5,  0,  0x0f, 5, 5),
	DEF_PKT( 9,  9,  4,  5,  0x17, 5, 6),
	DEF_PKT(13, 13,  7,  0,  0x1f, 7, 7),
	DEF_PKT(11, 11,  5,  6,  0x3f, 7, 8),
	DEF_PKT(15, 15, 11,  0,  0x5f, 7, 9),
	DEF_PKT(15, 15, 11,  7,  0x7f, 8, 10),
	DEF_PKT(30, 31, 16, 32,  0xec, 8, 11),
	DEF_PKT(30, 31, 16, 32,  0xf0, 8, 12),
	DEF_PKT( 0,  0,  0,  0,  0xf4, 8, 13),
	DEF_PKT( 0,  0,  0,  0,  0xf8, 8, 14),
	DEF_PKT( 0,  0,  0,  0,  0xfc, 8, 15),
	DEF_PKT( 0,  0,  0,  0,  0xff, 8, 16),
	DEF_PKT( 0,  0,  0,  0,  0x00, 2, 0),
	DEF_PKT( 0,  0,  0,  0,  0,    0, 0), // END(�����ɓ��B���邱�Ƃ͂Ȃ����c)
};
const s32 PRECISION_TABLE[8] = { // �ۑ��𑜓x
	0, 3, 5, 7, 0, 0, 0, 0 // 1ms/8ms/32ms/128ms, ���̓f�t�H���g��1ms(3cm)�Ƃ���
};

// �r�b�g���o��
static inline s32 SgnShift(u32 val, s32 shift){
	return (shift < 0)? (val >> -shift) : (val << shift);
}
s32 UnpackBit(u8** buf, s32* cur_bit, s32 bit_size){
	if(!bit_size) return 0; // ���o���s�v
	s32 ret = 0, shift = -*cur_bit, bs = bit_size;
	while(bs){
		s32 next = *cur_bit + bs;
		if(next > 8) next = 8;
		ret |= SgnShift(**buf & BitFill(next) & ~BitFill(*cur_bit), shift);
		bs -= next - *cur_bit;
		*cur_bit = next & 7;
		shift += 8;
		if(next == 8) ++*buf;
	}
	if(ret & BitRange(bit_size)) ret |= ~BitFill(bit_size); // �����g��
	return ret;
}

// �f�R�[�_
s32 DecodeTrackLog(u8* buf, u8* end){
	s32 dif[4], val[4], prc = 0, i, j;
	while(buf < end){
		// PID����(�Ƃ肠�������[�v�ŁB256type�����e�[�u�����ǂ�����)
		const PacketType* pt = PACKET_TABLE;
		while((pt->mask & *buf) != pt->flag) ++pt;
		gPacketCount[pt->pid]++; // ���v�\���p�ɃJ�E���g���Ă���

		// PID�ʏ���
		switch(pt->pid){
		case 0: // ���s�[�g�p�P�b�g
			for(j = (*buf++ >> 2) + 2 ; j-- ;){
				for(i = 4 ; i-- ;) val[i] += dif[i];
				WriteTrackLog(val, prc);
			}
			break;
		case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10: // ��K�����p�P�b�g
			j = pt->mask_size;
			for(i = 4 ; i-- ;) val[i] += dif[i] += UnpackBit(&buf, &j, pt->bit[i]);
			WriteTrackLog(val, prc);
			break;
		case 12: // ��Βl�p�P�b�g<�V�K>
			if(!CloseTrackLog()) return 2; // FileSystemFull�Ƃ�?
			// no break�@(�Z�p���[�g�����ȊO��<�p��>�Ɠ���)
		case 11: // ��Βl�p�P�b�g<�p��>
			j = 0;
			prc = PRECISION_TABLE[UnpackBit(&++buf, &j, 3) & 7]; // �ۑ��𑜓x�擾
			for(i = 4 ; i-- ;) val[i] = UnpackBit(&buf, &j, pt->bit[i]);
			dif[0] = dif[1] = dif[2] = 0;	// �����l��������
			dif[3] = 1;						// sec����1�������l
			if(!gOut && !OpenTrackLog(val[3] + UNIX2GARMIN)) return 1; // �V�K�쐬���s?
			WriteTrackLog(val, prc);
			break;
		case 13: // BCC(�Ƃ肠����BCC�G���[�͖������āA�ł��邾���f�R�[�h���Ă݂�)
			buf += 2;
			break;
		case 14: // 1byte �R�}���h
			gSegFlag |= buf[1] & gOption; // ���p�P�b�g�ŃZ�O�����g����(�I�v�V�����I��)
			buf += 2;
			break;
		case 15: // n byte �R�}���h
			gPacketCount[pt->pid] += buf[1] + 2; // �I�v�V�����͉σT�C�Y
			buf += (u32)buf[1] + 3;
			break;
		default:
			return 0; // �I�[�}�[�N
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// �t���b�V��ROM�u���b�N����
///////////////////////////////////////////////////////////////////////////////
#define FLBL_TRACK_MAGIC	0x016b7254	// �g���b�N�}�W�b�N("Trk?")
#define BLOCK_SIZE			(64 * 1024)
#define BLOCK_HEAD_SIZE		8
#define BLOCK_TRACK_SIZE	(BLOCK_SIZE - BLOCK_HEAD_SIZE)
typedef struct{
	u32 magic; // ���O���ʗp
	s32 index; // �u���b�N�����ŏd�v�Ȃ̂͏�ʂ�8bit(����24bit��PacketCounter�͂ǂ��ł�����)
	u8 val[BLOCK_TRACK_SIZE];
} TrackBlock;

// 64KB�u���b�N�P�ʂő���
int SearchTrackLogBlock(u8* buf, u32 size){
	TrackBlock* tb = (TrackBlock*)buf;
//	size /= BLOCK_SIZE;
	size = (size + BLOCK_SIZE - 1) / BLOCK_SIZE; // FIX1.1

	// �X�^�[�g�u���b�N�̌���
	u32 start = 0, i;
	while(tb[start].magic != FLBL_TRACK_MAGIC){
		if(++start >= size){ // 64KB�����̓���f�[�^���l������#0�u���b�N�̓T�C�Y�����Ȃ�(#1�ȍ~�͂����Ō���)
			fprintf(stderr, "Can't find TrackLog block\n"); // 1���g���b�N���O��������Ȃ�����
			return 1;
		}
	}
	// �㏑�����[�h�̃��[�e�[�V�������l�����ăX�^�[�g���V�t�g����
	for(i = start; ++i < size ;){
		if(tb[i].magic == FLBL_TRACK_MAGIC && tb[i].index - tb[start].index < 0){
			start = i;
			break;
		}
	}
	// �X�^�[�g�u���b�N���珇�Ԃɏ����o��
	i = start;
	do {
		if(tb[i].magic == FLBL_TRACK_MAGIC) DecodeTrackLog(tb[i].val, tb[i].val + BLOCK_TRACK_SIZE);
		if(++i >= size) i = 0;
	} while(i != start);
	return CloseTrackLog()? 0 : 1; // �t�@�C���N���[�Y�G���[�`�F�b�N
}

///////////////////////////////////////////////////////////////////////////////
// ���k�g���b�N�f�[�^�ǂݍ���
///////////////////////////////////////////////////////////////////////////////
static u8 gBuf[1024 * 1024]; // �Ƃ肠����1MB���O���[�o���Ɂc

int main(int argc, char** argv){
	switch(argc){
	case 3:
		gOption = strtoul(argv[2], 0, 16);
		// no break;
	case 2:
		if(argv[1][0] != '-') break;
		// no break
	default:
		fprintf(stderr, "usage: %s file [segment_option]\n", *argv);
		return 1;
	}

	// ���O�ǂݍ���
	FILE* in = fopen(argv[1], "rb");
	if(!in){
		fprintf(stderr, "Can' open %s\n", argv[1]);
		return 2;
	}
	size_t size = fread(gBuf, 1, sizeof(gBuf), in); // ROM(1MB)���ۂ��Ɠǂݍ���
	fclose(in);

	// �f�R�[�h
	memset(gBuf + size, -1, sizeof(gBuf) - size);// 64KB�����̓���f�[�^���l�����Č㔼��PID:16�Ŗ��߂�
	return SearchTrackLogBlock(gBuf, size);
}

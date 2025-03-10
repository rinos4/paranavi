///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2007 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// SuperCard SD/MiniSD�p�̃t�@�C��I/F���܂Ƃ߂��B
// FAT�t�@�C���V�X�e���������Ŏ���(����FAT�f�o�C�X�p�ɕ������ׂ�?)

// SCSD�ł͍ŏ��������ݒP�ʂ��Z�N�^�T�C�Y�ɂȂ邽�߁A�������ݑ��x�Ə��������񐔐�����A�g���b�N���O��
// ���C�g�L���b�V�����g�킴��𓾂Ȃ��B���̂��߁A�d����؂�Ƃ��̓T�X�y���h���g����WriteCache���t���b
// �V��������K�v����B
// �� �L���b�V�������̏ꍇ�A1�b�Ԋu��512byte�Z�N�^�𖈉񏑂������邽�߁A64KB�̍ŏ����O�t�@�C�����g�p��
//�@�@���Flash���������񐔃��~�b�g(10����)�� 100000 * 65536 / 512 [s] �� 148[��]�ŒB���Ă��܂��B
//    (512byte�Z�N�^�̃��C�g�L���b�V�����g���΁A1byte/point���k�����̘A���^�p�Ŗ�200�N����)


// TODO �������݂�500ms���x�v����ٗl�ɒx���Z�N�^������? �J�[�h�s��!?

// �c�Ȃ񂩁ASCSD�g����SD�J�[�h�A�N�Z�X���Ȃ��Ă�JoyCarry�̔����ʂ����d�r��
// �����Ȃ��B ������Đ�����SCSD��SDRAM�̓d��OFF����Ή��P�\�H

///////////////////////////////////////////////////////////////////////////////
// �萔
///////////////////////////////////////////////////////////////////////////////

// "\Paranavi.dat"���f�[�^�t�@�C���Ɏw�肷��
#define DAT_FILE_NAME "paranavidat " // �Ō�̃X�y�[�X(0x20)�̓A�g���r���[�g

#define ENABLE_WRITE_VERIFY 1 // �x���Ȃ邪�x���t�@�C�͏d�v


///////////////////////////////////////////////////////////////////////////////
// FAT�t�@�C���V�X�e��
///////////////////////////////////////////////////////////////////////////////
//�}�X�^�u�[�g���R�[�h
typedef struct {
	u8 x86code[446];
	
	// �p�[�e�B�V�����e�[�u��
	u8 p0_boot		[1];
	u8 p0_first_sect[3];
	u8 p0_fat_desc	[1];
	u8 p0_last_sect	[3];
	u8 p0_log_sect	[4];
	u8 p0_num_sect	[4];

	u8 p1_boot		[1];
	u8 p1_first_sect[3];
	u8 p1_fat_desc	[1];
	u8 p1_last_sect	[3];
	u8 p1_log_sect	[4];
	u8 p1_num_sect	[4];

	u8 p2_boot		[1];
	u8 p2_first_sect[3];
	u8 p2_fat_desc	[1];
	u8 p2_last_sect	[3];
	u8 p2_log_sect	[4];
	u8 p2_num_sect	[4];

	u8 p3_boot		[1];
	u8 p3_first_sect[3];
	u8 p3_fat_desc	[1];
	u8 p3_last_sect	[3];
	u8 p3_log_sect	[4];
	u8 p3_num_sect	[4];

	// �`�F�b�N�R�[�h 55 aa
	u8 check[2];
} MBR;

// FAT�w�b�_
typedef struct {
	// HEAD				//		UMAX MiniSD 1GB
	u8  jmp;			// 0	EB
	u8  ipl;			// 1	(3C)
	u8  nop;			// 2	90
	u8  name[8];		// 0    "MSDOS 5.0"

	// BPB
	u8  sect_size[2];	// B    512byte
	u8  clst_size;		// D    32 (= 16KB) 16KB*65536 = 1GB
	u16 start_sect;		// E    2
	u8  fat_num;		// 10   2 backup
	u8  dir_entry[2];	// 11   512
	u8  log_sect[2];	// 13   0 (->log_sect2)
	u8  media_desc;		// 15   0xF8
	u16 fat_sect;		// 16   0x00EF
	u16 phy_sec;		// 18   0x003F
	u16 head_count;		// 1A   0x00FF
	u32 hide_sec;		// 1C	0x81
	u32 log_sect2;		// 20	0x001dc37f (* 512 = 1GB)
	// ��͂ǂ��ł������c
} FatHeader;

typedef struct {
	u8 name[8];		// 0
	u8 ext[3];		// 8
	u8 attr;		// B
	u8 NT;			// C
	u8 c_time_ms;
	u8 c_time[2];
	u8 c_date[2];
	u8 a_date[2];
	u8 clst_h[2];
	u8 u_time[2];
	u8 u_date[2];
	u16 head;		// �N���X�^�w�b�_
	u32 size;		// �t�@�C���T�C�Y
} DirEntry;


#define DIR_HEAD_OFFSET 0x1a
#define DIR_SIZE_OFFSET 0x1c

// ����N���X�^�v�Z���Ă�����x���Ďg���Ȃ��̂ŁA�t�@�C���I�t�Z�b�g����
// �Z�N�^�ʒu���v�Z���邽�߂̃N���X�^�L���b�V����SC��SDRAM��ɍ쐬����B
u32* const CLUSTER_CACHE = (u32*)(0x08000000 + 0x80000); // +512KB(��Paranavi.bin+Voice.bin+Boot.bin)

#define SCSD_TIMEOUT	(60 * 1) // 1�b

///////////////////////////////////////////////////////////////////////////////
// SCSD����
///////////////////////////////////////////////////////////////////////////////
// SuperCard WEB�T�C�g��arm�R�[�h���x�[�X�BC�ɋN�����ăG���[�^�C���A�E�g����ǉ��B
// �R���p�C����thumb�ɂȂ�B
#define SC_SD_CMD		(*(vu16*)0x09800000)
#define SC_SD_CMD32A	(*(vu32*)0x09800000)
#define SC_SD_CMD32B	(*(vu32*)0x09800004)
#define SC_SD_CMD32C	(*(vu32*)0x09800008)
#define SC_SD_CMD32D	(*(vu32*)0x0980000c)
#define SC_SD_DATA		(*(vu16*)0x09000000)
#define SC_SD_DATA32A	(*(vu32*)0x09000000)
#define SC_SD_DATA32B	(*(vu32*)0x09000004)
#define SC_SD_READ		(*(vu16*)0x09100000)
#define SC_SD_READ32A	(*(vu32*)0x09100000)
#define SC_SD_READ32B	(*(vu32*)0x09100004)
#define SC_SD_RESET		(*(vu16*)0x09440000)
#define SC_UNLOCK		(*(vu16*)0x09FFFFFE)

#define ENABLE_SDRAM  5
#define ENABLE_SDCARD 3

u32 SCSDIsInserted(){
//  return (SC_SD_CMD & 0x0300);
	return (SC_SD_CMD);
}

// 512 byte sector write
static void sd_data_write16(const u16* src, s32 n){
	while(n--){
		u32 v = *src++;
		// �����ʓ|�ȃR���g���[�����ȁc�B�돑�����ݖh�~��?
		SC_SD_DATA32A = (v += v << 20);
		SC_SD_DATA32B = v >> 8;
	}
}

static s32 sd_data_write_s(const u16* src, const u16* crc16){
	// �A�C�h���҂�
	s32 timeout = IW->vb_counter + SCSD_TIMEOUT;
	while(!(SC_SD_DATA & 0x100)){
		if(timeout - (s32)IW->vb_counter < 0) return FLASH_E_STATUS_TIMEOUT;
	}
	SC_SD_DATA; // ��ǂ�

	// 512�Z�N�^�P�ʂŏ�������
	SC_SD_DATA = 0;		// start bit
	sd_data_write16(src, 256);
	if(crc16) sd_data_write16(crc16, 4);
	SC_SD_DATA = 0xFF;	// end bit

	// �����҂�
	timeout = IW->vb_counter + SCSD_TIMEOUT;
	while(SC_SD_DATA & 0x100){
		if(timeout - (s32)IW->vb_counter < 0) return FLASH_E_STATUS_TIMEOUT;
	}
	SC_SD_DATA32A;// ��ǂ�
	SC_SD_DATA32B;// ��ǂ�
	return 0;
}

static s32 sd_data_read_s(u16* dst){
	// �A�C�h���҂�
	s32 timeout = IW->vb_counter + SCSD_TIMEOUT;
	while(SC_SD_READ & 0x100){
		if(timeout - (s32)IW->vb_counter < 0) return FLASH_E_STATUS_TIMEOUT;
	}
//	SC_SD_READ; // ��ǂ� (TODO ������Read����ƃr�b�g���ꂪ�N����!?�Ƃ肠�����R�����g�ɂ��Ă���)

	u32 i = 0;
	for (i = 0; i < 256; i++){
		SC_SD_READ32A; // �ǂݎ̂�
		*dst++ = (u16)(SC_SD_READ32B >> 16);
	}

	// CRC�͓ǂݎ̂�(�{���̓`�F�b�N���ׂ��c)
	SC_SD_READ32A;
	SC_SD_READ32B;
	SC_SD_READ32A;
	SC_SD_READ32B;
	SC_SD_READ32A;
	SC_SD_READ32B;
	SC_SD_READ32A;
	SC_SD_READ32B;

	SC_SD_READ; // ��ǂ�
	return 0;
}

// CRC�v�Z
static void sd_crc16_s(const u8* src, u16 num, u8* crc16){
	u32 r3 = 0, r4 = 0, r5 = 0, r6 = 0;

	u32 r7 = 0x80808080;
	u32 r8 = 0x1021;
	u32 r2 = 0;
	for(num <<= 3 ; num ; num -= 4){
		if (r7 & 0x80) r2 = *src++;

		r3 <<= 1;
		if (r3 & 0x10000)  r3 ^= r8;
		if (r2 & (r7>>24)) r3 ^= r8;

		r4 <<= 1;
		if (r4 & 0x10000)  r4 ^= r8;
		if (r2 & (r7>>25)) r4 ^= r8;

		r5 <<= 1;
		if (r5 & 0x10000)  r5 ^= r8;
		if (r2 & (r7>>26)) r5 ^= r8;

		r6 <<= 1;
		if (r6 & 0x10000)  r6 ^= r8;
		if (r2 & (r7>>27)) r6 ^= r8;

		r7 = (r7 >> 4) | (r7 << 28); // ror
	}

	for(num = 16 ; num ;){
		r7 <<= 4;
		if (r3 & 0x8000) r7 |= 8;
		if (r4 & 0x8000) r7 |= 4;
		if (r5 & 0x8000) r7 |= 2;
		if (r6 & 0x8000) r7 |= 1;

		r3 <<= 1;
		r4 <<= 1;
		r5 <<= 1;
		r6 <<= 1;

		if (num-- & 1) *crc16++ = (u8)r7;
	}
}

static u32 sd_crc7_s(const u8* src, u16 num){
	u32 r3 = 0;
	u32 r4 = 0x80808080;
	u32 r2 = 0;

	for(num <<= 3 ; num ; --num){
		if (r4 & 0x80) r2 = *src++;

		r3 = r3 << 1;
		if (r3 & 0x80)     r3 ^= 9;
		if (r2 & (r4>>24)) r3 ^= 9;
		r4 = (r4 >> 1) | (r4 << 31); // ror
	}

	return (r3 << 1) + 1;
}

static s32 sd_com_read_s(u32 num){
	// �A�C�h���҂�
	s32 timeout = IW->vb_counter + SCSD_TIMEOUT;
	while(SC_SD_CMD & 1){
		if(timeout - (s32)IW->vb_counter < 0) return FLASH_E_STATUS_TIMEOUT;
	}

	// ��ǂ�
	while(num--){
		SC_SD_CMD32A;
		SC_SD_CMD32B;
		SC_SD_CMD32C;
		SC_SD_CMD32D;
	}
	return 0;
}

static inline s32 sd_get_resp(){
	return sd_com_read_s(6);
}

static void sc_mode(u16 data){
	SC_UNLOCK = 0xA55A;
	SC_UNLOCK = 0xA55A;
	SC_UNLOCK = data;
	SC_UNLOCK = data;
}

static void sc_sdcard_reset(){
	SC_SD_RESET = 0;
}

static s32 sd_com_write_s(const u8* src, u32 num){
	// �A�C�h���҂�
	s32 timeout = IW->vb_counter + SCSD_TIMEOUT;
	while(!(SC_SD_CMD & 1)){
		if(timeout - (s32)IW->vb_counter < 0) return FLASH_E_STATUS_TIMEOUT;
	}
	SC_SD_CMD; // ��ǂ�

	while(num--){
		u32 v= *src++;
		v += v << 17;
		SC_SD_CMD32A = v;
		SC_SD_CMD32B = v << 2;
		SC_SD_CMD32C = v << 4;
		SC_SD_CMD32D = v << 6;
	}
	return 0;
}

static void sd_send_clk(u32 count){
	while(count--) SC_SD_CMD; // �N���b�N
}

static s32 sd_cmd(u8 cmd, u32 sector){
	u8 buf[6];
	buf[0] = (u8)(cmd + 0x40);
	buf[1] = (u8)(sector >> 24);
	buf[2] = (u8)(sector >> 16);
	buf[3] = (u8)(sector >>  8);
	buf[4] = (u8)(sector      );
	buf[5] = (u8)sd_crc7_s(buf, 5);
	return sd_com_write_s(buf, 6);
}

///////////////////////////////////////////////////////////////////////////////
// �Z�N�^�ǂݍ���
static s32 SDReadSector(u16* buf, u32 sector){
	sc_mode(ENABLE_SDCARD);
	sc_sdcard_reset();

	s32 ret = 0;
#define CHECK_RET(f) do { if((ret = (f)) != 0) return ret; } while(0)
	CHECK_RET(sd_cmd(18, sector << 9));
	CHECK_RET(sd_data_read_s(buf));
	CHECK_RET(sd_cmd(12, 0));
	CHECK_RET(sd_get_resp());
	sd_send_clk(0x10);
	return 0;
}

// �Z�N�^��������
static s32 SDWriteSector(const u16* buf, u32 sector){
	sc_mode(ENABLE_SDCARD);
	sc_sdcard_reset();

	s32 ret = 0;
	CHECK_RET(sd_cmd(25, sector << 9));
	CHECK_RET(sd_get_resp());
	sd_send_clk(0x10); 

	u16 crc16[5];
	sd_crc16_s((u8*)buf, 512, (u8*)crc16);
	CHECK_RET(sd_data_write_s(buf, crc16));
	sd_send_clk(0x10); 

	CHECK_RET(sd_cmd(12, 0));
	CHECK_RET(sd_get_resp());
	sd_send_clk(0x10);

	// �������݊����҂�
	s32 timeout = IW->vb_counter + SCSD_TIMEOUT;
	while(!(SC_SD_DATA & 0x0100)){
		if(timeout - (s32)IW->vb_counter < 0){
			ret = FLASH_E_STATUS_TIMEOUT;
			break;
		}
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// DAT�t�@�C���A�N�Z�X����
///////////////////////////////////////////////////////////////////////////////
// �Z�N�^�ʒu�𒼓ǂ݂��邽�߂ɁA���������ɃN���X�^�L���b�V�����쐬���Ă���
// �Ƃ肠����FAT16�̂݃T�|�[�g (FAT12�̓C���i�C?)
static s32 CreateClusterCache(u32 fat_desc){
	SCSDWork* ssw = &IW->cw.scsd_work;

	u32 clst_bytes = ssw->clst_size * SECT_SIZE;

	// �܂��A���g�p��VOICE_ADDRESS�̈�ɃN���X�^�L���b�V�����쐬
	u32 offset = 0;
	u16 clst = ssw->file_head; // �ŏ��̃N���X�^���Z�b�g
	u32* dst = (u32*)VOICE_ADDRESS;
	u32 pre_fat = -1;
	ssw->clst_limit = 0;
	ssw->datf_min = -1;
	ssw->datf_max =  0;

	for(offset = 0 ; offset < ssw->file_size ; offset += clst_bytes){
		if(clst < 0x0002 || 0xfff6 < clst) return FLASH_E_CNUM_RANGE; // �L���N���X�^�ԍ��ȊO�̓G���[
		u32 sect = (clst - 2) * ssw->clst_size + ssw->dat_start; // �擪�̃Z�N�^�ԍ�����������
		*dst++ = sect;

		// �������݃`�F�b�N�p�̒l��ݒ�
		if(ssw->datf_min > sect) ssw->datf_min = sect;
		if(ssw->datf_max < sect) ssw->datf_max = sect;

		// �ΏۃN���X�^��FAT�����[�h
		u32 cur_fat = clst >> 8; //TODO FAT12
		if(pre_fat != cur_fat){
			pre_fat = cur_fat;
			s32 ret = SDReadSector(ssw->scache16, ssw->fat_start + cur_fat);
			if(ret) return ret;
		}
		clst = ssw->scache16[clst & 0xff]; // TODO FAT12
#define CLUSTER_CACHE_LIMIT 0x10000	// 256KB
		if(++ssw->clst_limit > CLUSTER_CACHE_LIMIT) return FLASH_E_DAT_SIZE;
	}
	// VOICE_ADDRESS����SCSD��SDRAM�փN���X�^�L���b�V���]��
	sc_mode(ENABLE_SDRAM); // SDRAM�������݋����Ă���]��
	DmaCopy(3, VOICE_ADDRESS, CLUSTER_CACHE, (u32)dst - (u32)VOICE_ADDRESS, 32); // 32bit Write��OK
	sc_mode(ENABLE_SDCARD); // �ʏ��SD�J�[�h�A�N�Z�X���[�h�ɖ߂�
	return 0;
}

static u32 GetShiftPos(u32 val){
	u32 i = 0;
	for(; val ; ++i, val >>= 1){
		if(val & 1){
			if(val > 1) break; // 2�ȏ�"1"����
			return i;
		}
	}
	return -1;
}

// �f�o�b�O�p�Ƀ_���v�G���A�ɃZ�N�^������������
static s32 MBR_Error(const MBR* mbr){
	DmaCopy(3, mbr, IW->mp.last_wpt, SECT_SIZE, 32); // wpt-route�G���A�ɘA����������
	IW->mp.last_wpt_len = SECT_SIZE + DBGDMP_SCSD;
	IW->mp.last_route_len = 0;
	return FLASH_E_MBR_ERROR;
}
static s32 FAT_Error(const FatHeader* fh, s32 err){
	DmaCopy(3, fh, IW->mp.last_pvt, 256, 32); // �w�b�_�O�����_���v(last_pvt�Ɏ��܂�͈͂Łc)
	IW->mp.last_pvt_len = 256 + DBGDMP_SCSD;
	return err;
}

static s32 InitDatFileInfo(){
	SCSDWork* ssw = &IW->cw.scsd_work;

	sc_mode(ENABLE_SDCARD);
	sc_sdcard_reset();

	// �����Z�N�^0�`�F�b�N
	u32 log_sect = 0;
	u32 fat_desc = 0;
	{
		s32 ret = SDReadSector(ssw->scache16, 0);
		if(ret) return ret;
		MBR* mbr = (MBR*)ssw->scache16;
		// FAT12/16�̂݃T�|�[�g
		fat_desc = mbr->p0_fat_desc[0];
		if(fat_desc != 1 && fat_desc != 4 && fat_desc != 6){
//			return MBR_Error(mbr);
			// �Z�N�^0���_���Z�N�^�̃J�[�h������!?�@�Ƃ肠����log_sect=0�Ői�߂�c
		} else {
			// BPB�̃Z�N�^�T�C�Y�`�F�b�N
			log_sect = GetLong(mbr->p0_log_sect);
		}

		//TODO REMOVE �f�o�b�O�p�Ƀ_���v
		MBR_Error(mbr);

		// mbr���g����̂̓R�R�܂�
	}

	{
		// �Z�N�^�T�C�Y�`�F�b�N
		s32 ret = SDReadSector(ssw->scache16, log_sect); // �_���Z�N�^0�����[�h
		if(ret) return ret;
		FatHeader* fh = (FatHeader*)ssw->scache16;
		ssw->sect_size = GetInt(fh->sect_size);
		ssw->sect_shift = GetShiftPos(ssw->sect_size);
		if(ssw->sect_shift == -1) return FAT_Error(fh, FLASH_E_SECT_ERROR); // sect_size �� 2^n  !!

		// �Ƃ肠���� 512 byte�Z�N�^�̂݃T�|�[�g�B1024�ȏ�ɂ���Ȃ�Z�N�^�L���b�V���𑝂₷�K�v����!
		if(ssw->sect_size != SECT_SIZE) return FAT_Error(fh, FLASH_E_SECT_ERROR);

		// �N���X�^�T�C�Y�`�F�b�N
		ssw->clst_size = fh->clst_size;
		ssw->clst_shift = GetShiftPos(ssw->clst_size);
		if(ssw->clst_shift == -1) return FAT_Error(fh, FLASH_E_CLST_ERROR); // clst_size �� 2^n  !!
		ssw->clst_shift += ssw->sect_shift;

		// FAT�G���g�����烋�[�g�f�B���N�g�������擾
		ssw->fat_start = fh->start_sect + log_sect;
		ssw->dir_start = ssw->fat_start + fh->fat_num * fh->fat_sect;
		ssw->dat_start = ssw->dir_start + (GetInt(fh->dir_entry) >> 4); // max / (512/32)

		//TODO REMOVE �f�o�b�O�p�Ƀ_���v
		FAT_Error(fh, 0);

		// fh���g����̂̓R�R�܂�
	}

	// ���[�g�f�B���N�g������dat�t�@�C����T��
	u32 sect;
	for(sect = ssw->dir_start ; sect < ssw->dat_start ; ++sect){
		s32 ret = SDReadSector(ssw->scache16, sect); // �f�B���N�g���G���g�������[�h
		if(ret) return ret;
#define DIRENT_SEC 16 // 512 / 32
		u32 j;
		for(j = 0 ; j < DIRENT_SEC ; ++j){
			u8* p = &ssw->scache8[j << 5];
			if(!*p) return FLASH_E_NO_FILE; // �I�[�B������Ȃ������B
			if(!strnicmp(p, DAT_FILE_NAME, sizeof(DAT_FILE_NAME) - 1)){
				// �t�@�C����������
				ssw->file_head = GetInt (p + DIR_HEAD_OFFSET);
				ssw->file_size = GetLong(p + DIR_SIZE_OFFSET) & ~(BLOCK_SIZE - 1);// �g�p����t�@�C���̈��BLOCK_SIZE�̔{���ɐ��K��
				return CreateClusterCache(fat_desc);
			}
		}
	}
	return FLASH_E_NO_FILE;
}

static u32 GetSector(u32 offset){
	SCSDWork* ssw = &IW->cw.scsd_work;
	u32 clst = offset >> ssw->clst_shift;
	if(clst >= ssw->clst_limit) return -1; // �傫������I�t�Z�b�g�w��

	// ���S�ł͂Ȃ����A�N���X�^�L���b�V���ُ̈�`�F�b�N�����Ă���
	u32 sect_num = CLUSTER_CACHE[clst];
	if(sect_num < ssw->datf_min || ssw->datf_max < sect_num) return -1; // �A���N���X�^�Ȃ�DAT�t�@�C���ɕ�����

	return sect_num + ((offset >> ssw->sect_shift) & (ssw->clst_size - 1)); // ����Z�N�^�ԍ�?
}

// size ��SECT_SIZE�̔{���ł��邱��!
static s32 ReadDatFile(u16* buf, u32 offset, s32 size){
	SCSDWork* ssw = &IW->cw.scsd_work;
	while(size > 0){
		u32 sect = GetSector(offset);
		if(sect == -1) return FLASH_E_OFFSET;

		// �Z�N�^�L���b�V���`�F�b�N
		if(sect == ssw->cache_sect){
			if(buf != ssw->scache16) DmaCopy(3, ssw->scache16, buf, SECT_SIZE, 32);
		} else {
			s32 ret = SDReadSector(buf, sect);
			if(ret){
				if(buf == ssw->scache16) ssw->cache_sect = 0; // �Z�N�^�L���b�V���j��
				return ret;
			}
			if(buf == ssw->scache16) ssw->cache_sect = sect; // �Z�N�^�L���b�V���X�V
		}
		buf    += SECT_SIZE >> 1;
		offset += SECT_SIZE;
		size   -= SECT_SIZE;
	}
	return 0;
}

static s32 WriteDatFile(const u16* buf, u32 offset, s32 size){
	SCSDWork* ssw = &IW->cw.scsd_work;
	while(size > 0){
		// �f�[�^��������
		u32 sect = GetSector(offset);
		if(buf == ssw->scache16) ssw->cache_sect = sect; // �Z�N�^�L���b�V���X�V(-1�Ȃ疳���Ƃ��ċL�^)
		if(sect == -1) return FLASH_E_OFFSET;
		s32 ret = SDWriteSector(buf, sect);
		if(ret) return ret; // �������݂Ɏ��s���Ă��Z�N�^�L���b�V���̓N���A���Ȃ�

#ifdef ENABLE_WRITE_VERIFY
		// �R���y�A
		u16 rdata[SECT_SIZE >> 1]; // �X�^�b�N�M���M��!!
		ret = SDReadSector(rdata, sect);
		if(ret) return ret;
		if(memcmp(rdata, buf, SECT_SIZE)){
			/*
			// �R���y�A�G���[�ӏ����_���v
			*(u32*)IW->mp.last_wpt = sect;
			DmaCopy(3, rdata, IW->mp.last_wpt + 4, 256 - 4, 32);
			IW->mp.last_wpt_len = 256 + DBGDMP_SCSD;
			*(u32*)IW->mp.last_pvt = offset;
			DmaCopy(3, buf, IW->mp.last_pvt + 4, 256 - 4, 32);
			IW->mp.last_pvt_len = 256 + DBGDMP_SCSD;
			*/
			return FLASH_E_COMPARE;
		}
#endif

		// ��������OK
		buf    += SECT_SIZE >> 1;
		offset += SECT_SIZE;
		size   -= SECT_SIZE;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// File I/F SuperCard SD
///////////////////////////////////////////////////////////////////////////////
void SearchRomVoice();
static s32 SCSD_InitCart(){
	CartWork* cw  = &IW->cw;
	SCSDWork* ssw = &cw->scsd_work;
	s32 ret = InitDatFileInfo();
	if(ret) return ret;
	if(ssw->file_size < FI_TRACK_OFFSET) return FLASH_E_SIZE;

	// �����A�h���X��ݒ�
	SearchRomVoice();

	// �g���b�N���O�̗L���u���b�N��ݒ�
	cw->phy_tlog = 0; // TODO ���O�͕����ΏۊO
	cw->tlog_block = ssw->file_size / FI_TRACK_OFFSET - 1;

	// ���[�N���ݒ�
	cw->phy_boot	= ((u32)&__iwram_overlay_lma) + (0x08000000 - 0x02000000);
	cw->phy_navi	= 0x08000000;
	cw->phy_config	= (u32)&IW->tc; // RAM����R�s�[�B���ɃS�~���t�����Ƃ肠�����C�ɂ��Ȃ��c
	cw->phy_flog	= 0; // TODO ���O�͕����ΏۊO
	cw->phy_rtwpt	= (u32)ROUTE; // RAM����R�s�[�B���ɃS�~���t�����Ƃ肠�����C�ɂ��Ȃ��c
	cw->cap			= CARTCAP_WRITEABLE | CARTCAP_DUPLICATE | CARTCAP_SECTWRITE;
	return 0;
}

// 512byte�Z�N�^�̂݃��[�h
const u32* SCSD_ReadDirect(u32 offset){
	SCSDWork* ssw = &IW->cw.scsd_work;
	if(!ReadDatFile(ssw->scache16, offset, SECT_SIZE)) return ssw->scache32;
	return 0;
}

// �w�b�_����v���Ă���ꍇ�̂�read����
static s32 SCSD_ReadData(u32 offset, void* dst, s32 size, u32 magic){
	// �f�o�b�O�p�`�F�b�N
	if(0 < magic && magic < 10){ // �f�o�b�O�p�R�[�h
		if(magic == 1) return SDReadSector((u16*)dst, offset >> 9); // �Z�N�^���ǂ�
//		DmaCopy(3, CLUSTER_CACHE, dst, size, 16); // 16bit Read
		DmaCopy(3, CLUSTER_CACHE, dst, size, 32); // 32bit Read
		return 0;
	}
		
	const u32* p = SCSD_ReadDirect(offset);
	if(!p) return FLASH_E_OFFSET;
	if(magic && *p != magic) return FLASH_E_MAGIC; // 0�}�W�b�N�͎g��Ȃ�
	return ReadDatFile(dst, offset, size);
}

static s32 SCSD_WriteData(u32 offset, const void* src, s32 size){
	return WriteDatFile((u16*)src, offset, size);
}

static s32 SCSD_Flush();
static s32 SCSD_EraseBlock(u32 offset, s32 size, u32 mode){
	SCSDWork* ssw = &IW->cw.scsd_work;

	// �g���b�N���O��64KB�u���b�N��S��������ƒx���Ďg���Ȃ��̂Ő擪�Z�N�^�̂ݏ���������ꏈ��
	if(mode == ERASE_TRACK){
		DmaClear(3, -1, ssw->tlog_cache32, SECT_SIZE, 32); // ���O�L���b�V�����g�p
		ssw->tlog_offset = offset; // Erase��offset�͕K��64KB�u���b�N�ɃA���C�����g����Ă���
		ssw->tlog_dirty = 1; // �����������ݎw��
		return SCSD_Flush();
	}

	// SCSD�ł�magic�������ŏ����̃Z�N�^�P�ʂŏ�������
	if(mode == ERASE_MAGIC) size = SECT_SIZE;

	// ff����������
	DmaClear(3, -1, ssw->scache32, SECT_SIZE, 32);
	ssw->cache_sect = 0; // �L���b�V���j��
	for(; size > 0 ; size -= SECT_SIZE, offset += SECT_SIZE){
		s32 ret = WriteDatFile(ssw->scache16, offset, SECT_SIZE);
		if(ret) return ret;
	}
	return 0;
}

// �g���b�N���O�ǋL()
static s32 SCSD_AppendTLog(u8* buf, u32 size){ // size < 256 !
	// �������݈ʒu�v�Z
	SCSDWork* ssw = &IW->cw.scsd_work;
	TrackLogParam* tlp = &IW->mp.tlog;
	s32 ret = CUR_BLOCK_OFFSET(tlp) + tlp->index;
	u32 offset_hi = ret & ~(SECT_SIZE - 1); // != 0
	u32 offset_lo = ret &  (SECT_SIZE - 1);
	s32 size2 = offset_lo + size - SECT_SIZE;
	s32 size1 = (size2 > 0)? (size - size2) : size;

	// �Z�N�^�X�V�`�F�b�N
	if(ssw->tlog_offset != offset_hi){
		if(ssw->tlog_offset){
			// �V�Z�N�^�͏������(ff)����J�n
			ret = SCSD_Flush();
			if(ret) return ret;
			DmaClear(3, -1, ssw->tlog_cache32, SECT_SIZE, 32);
		} else {
			// ���񂪃A�y���h�̎��̂�SCSD����f�[�^�ǂݍ���
			ret = ReadDatFile(ssw->tlog_cache16, offset_hi, SECT_SIZE); // ����scache�Ƀf�[�^���c���Ă�̂ŃR�s�[�̂�
			if(ret) return ret;
		}
		ssw->tlog_offset = offset_hi; // �L���b�V���I�t�Z�b�g != 0
	}

	// ��1�Z�N�^��������
	memcpy(ssw->tlog_cache8 + offset_lo, buf, size1); // �g���b�N���O�ǉ�
	ssw->tlog_dirty += size1; // Dirty�r�b�g�Z�b�g

	// �L���b�V���������ݔ��f
#define TLOG_CACHE_LIMIT (VBC_TIMEOUT(600)) // 10����1��͋���Flush
	if(size2 >= 0 || IW->vb_counter - ssw->tlog_vbc > TLOG_CACHE_LIMIT){
		ret = SCSD_Flush();
		if(ret) return ret;
	}

	// ��2�Z�N�^��������
	if(size2 >= 0){
		// ���Z�N�^�̏�����(Dirty�͕K�������Ă���)
		DmaClear(3, -1, ssw->tlog_cache32, SECT_SIZE, 32);
		if(size2) memcpy(ssw->tlog_cache8, buf + size1, size2);
		ssw->tlog_offset = offset_hi + SECT_SIZE;
		if(ssw->tlog_offset >= ssw->file_size) ssw->tlog_offset = FI_TRACK_OFFSET; // ���[�e�[�V�������l���Bfile_size�͐��K������Ă���

		// ���ԃZ�N�^�ł́A�d���I�t�ɔ����ďI�[���o�p�Ɏ��Z�N�^��\���C���[�X���Ă���
		if(tlp->index + size < BLOCK_SIZE){
			ssw->tlog_dirty = 1; // �����������ݎw��
			ret = SCSD_Flush();
			if(ret) return ret;
		}
	}

	tlp->index += size; // �������݂ɐ���������index���X�V
	return 0;
}

// �g���b�N���O�̃��C�g�L���b�V�����t���b�V��
static s32 SCSD_Flush(){
	SCSDWork* ssw = &IW->cw.scsd_work;
	if(ssw->tlog_dirty && ssw->tlog_offset){
		// �������ݕK�v
		s32 ret = WriteDatFile(ssw->tlog_cache16, ssw->tlog_offset, SECT_SIZE);
		if(ret) return ret;
		ssw->tlog_dirty = 0; // Dirty����
	}
	ssw->tlog_vbc = IW->vb_counter;
	return 0;
}

const CartIF CIF_SUPERCARD_SD = {
	SCSD_InitCart,
	SCSD_ReadDirect,
	SCSD_ReadData,
	SCSD_WriteData,
	SCSD_EraseBlock,
	SCSD_AppendTLog,
	SCSD_Flush,
	Emul_GetCodeAddr,
};

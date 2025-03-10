///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005-2008 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////
// v0.1  2005/01/03 rinos
//       �E���ō쐬
// v0.2  2005/03/26 rinos
//       �E�E�F�C�|�C���g/���[�g�f�[�^�𑝗�
//       �E90����]��ʑΉ�
//       �E�����i�r�Ή�
//       �E�^�X�N�����S�ăA�v���ŏ�������悤�ύX(����OS�폜)
// v1.0  2005/05/05 rinos
//       �ESP���[�h(�O�Ճ��[�h�A�x���`�}�[�N���[�h�A�����v���[�h)�ǉ�
// v1.1  2005/07/03 rinos
//       �E�t�H���g�ύX
// v1.2  2005/09/17 rinos
//       �ESBAS�p���x�}�[�J�\���Ή�(�Ђ܂��6��MSAS�����^�p�J�n�L�O)
// v1.3  2005/10/02 rinos
//       �E�p�C�������̕\���ݒ�ǉ�
//       �E�O�ՐF�����W�ݒ�ǉ�
//       �E�X�e�[�g�ʂ̉����؂�ւ��Ή�
// v1.4  2005/11/03 rinos
//       �E�p�C�������̃V�����_�T�C�Y�ݒ�Ή�
//       �E���j���[���O�ւ̃v�`���\����ǉ�
// v1.5  2005/11/12 rinos
//       �E1��������̏㏸���̕\����ǉ�
//       �E�J�[�g���b�W�̕����R�s�[�Ή�
// v1.6  2005/12/29 rinos
//       �E180���A270���̉�]��ʒǉ�
//       �E�O�Ճ|�C���g����1800�ɑ���
// v1.7  2006/02/19 rinos
//       �E�S�[������(���C���A180���Z�N�^)�̐؂�ւ������j���[�ɒǉ�
//       �E�g���b�N���O�̋L�^�Ή�
// v1.8  2006/03/18 rinos
//       �E�p�C���������X�g�\�����̕����ʒu�̎w��@�\��ǉ�
//       �E�V���v�������V�[�P���X��ǉ�
// v1.9  2006/05/28 rinos
//       �E�Z���^�����O���a�̕\����ǉ�
// v1.10 2006/09/17 rinos
//       �E�l��񏉊����Ή�
// v2.0  2006/11/25 rinos
//       �ESP���[�h�ɃV�����_�}�b�v�\����ǉ�
//       �E�v���p�C�����̐ݒ��ǉ�
//       �E�t���[�t���C�g���[�h�p�Ƀ����f�B���O�ݒ��ǉ�
//       �EPC�փg���b�N���O���A�b�v���[�h����@�\��ǉ�
//       �E�����v�ڑ��Ή�
// v2.1  2007/01/14 rinos
//       �E�Q�R��p�C�����̗\�z���x�̌v�Z��ύX
//       �E�S�[���܂ł̎c�����A�\�z�S�[�����x���̕\���@�\�ǉ�
//       �E�t���C�g���O�ۑ��@�\��ǉ�
//       �E�����̃C���^�[�o���ݒ�@�\�ǉ�
//       �E90���Z�N�^�S�[���̐ݒ��ǉ�
// v2.2  2007/05/12 rinos
//       �EGPS60��GPS76CS�ɑΉ�
//       �E�S�[���Ǝ��p�C������L/D�\����ǉ�
//       �E�g���b�N���O�̕ۑ��̈�g��
//       �E�X�^�[�g�A���[�����Ή�
// v2.3  2007/11/03 rinos
//       �E�ő�V�����_�T�C�Y�̊g��(65km)
//       �E���p�C�����̃X�L�b�v�ݒ�ǉ�(TO�p�C��������)
//       �EGPS�ʐM�̃|�b�v�A�b�v�x���\���Ή�
//       �ELocus���[�h�̃J���[�^�C�v��ǉ�
//       �EPC�ƃ��[�g/�E�F�C�|�C���g����������@�\��ǉ�
// v2.4  2007/12/09 rinos
//       �E�s�b�`���O���v���Z���^�����O/��Z���^�����O���ɕ����ĕ\������悤�ύX
//       �E�O�Ճ��[�h�ƃV�����_���[�h�̕\���D��x�ݒ��ǉ�
// v2.5  2008/02/23 Mr.Takemura
//       �EGPS MAP 60CSx�Ή� (D109/D110_Wpt_Type�ǉ�)
// v2.6  2008/05/25 rinos
//       �EGPS-52D�p��NMEA-0183�֑Ή� (NMEA�f�R�[�_�ǉ��A�^�X�N����ύX)
//       �EBluetooth�ڑ��Ή��B(BT���W���[����Parani-ESD�ABluetooth GPS��SPP�f�o�C�X�ɑΉ�)
//       �E�ܓx�o�x���g��\������e�L�X�g���[�h��ǉ�(A�{�^��x3�ŕ\��)
//       �E���[�g���j���[�Ƀ��[�g��1�����I�����ē]������@�\��ǉ�
//       �E�V���v����ʂ̃��[�g���̗������ݎ����ɕύX
//       �E�������s���΍��Locus�p�ܓx�o�x�̃f�[�^�����k(-7KB)

#ifndef PARANAVI_H
#define PARANAVI_H

#define NAVI_VERSION "2.6"

#include <time.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "gba.h"
//#include "font/kanafont.h"
#include "font/comp.h" // �����������Ȃ��Ȃ��Ă������߁A���k���Ă݂�


///////////////////////////////////////////////////////////////////////////////
// inline�֐�
///////////////////////////////////////////////////////////////////////////////
static inline s32 MinS32(s32 a, s32 b){
	return (a < b)? a : b;
}
static inline s32 MaxS32(s32 a, s32 b){
	return (a > b)? a : b;
}
static inline s32 Range32(s32 min, s32 max, s32 v){
	if(v < min) return min;
	if(v > max) return max;
	return v;
}
static inline s32 myAbs(s32 v){
	return (v < 0)? -v : v;
}

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define KEY_CURSOR (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN)

#define CHANGE_MAX(max, v) do { if((max) < (v)) (max) = (v); } while(0)
#define CHANGE_MIN(min, v) do { if((min) > (v)) (min) = (v); } while(0)


static inline s32 GetAngLR(s32 a){
	if((a &= 0xffff) > 0x8000) a -= 0x10000; 
	return a;
}
static inline s32 GetAngDiff(s32 a){
	if((a &= 0xffff) > 0x8000) a = 0x10000 - a;
	return a;
}


///////////////////////////////////////////////////////////////////////////////
// BIOS�R�[��
///////////////////////////////////////////////////////////////////////////////
#ifdef __thumb__
#define BiosHalt() __asm("swi 0x2\n")
#define BiosStop() __asm("swi 0x3\n")
#else
#define BiosHalt() __asm("swi 0x20000\n")
#define BiosStop() __asm("swi 0x30000\n")
#endif

// �Z�p
s32 BiosDiv(s32 num, s32 denom, s32* mod);
u32 BiosSqrt(u32 n);
s32 BiosArcTan2(s32 x, s32 y);
s32 RoundDiv(s32 num, s32 denom);
static inline s32 RoundShift(s32 v, s32 sft){
	return (v + (1 << (sft - 1))) >> sft;
}

// BiosFunc���g�p���������֐�
u32 BiosHypot(s32 a, s32 b);			// BiosSqrt���g��������hypot
u32 BiosTripot(s32 a, s32 b, s32 c);	// BiosSqrt���g��������tripot
s32 BiosAtan64K(s32 yK, s32 xK);		// math�Ɠ��������� 64K�ŕԂ�

s32 CountBit(s32 a);
static inline s32 MulBit(s32 a, s32 b) { return CountBit(a) + CountBit(b); }


// ������
enum{
	BIOSRESET_EWRAM		= 1 << 0,
	BIOSRESET_IWRAM		= 1 << 1,
	BIOSRESET_PALETTE	= 1 << 2,
	BIOSRESET_VRAM		= 1 << 3,
	BIOSRESET_OAM		= 1 << 4,
	BIOSRESET_SIO		= 1 << 5,
	BIOSRESET_SOUND		= 1 << 6,
	BIOSRESET_OTHER		= 1 << 7,
};
void BiosRegisterRamReset(u32 n);

typedef struct {
	u8 pad1[0x14];			// 0
	u8 handshake_data;		// 14
	u8 pad2[3];				// 15
	u8 pc;					// 18
	u8 cd[3];				// 19
	u8 pal;					// 1c
	u8 pad3[1];				// 1d
	u8 cb;					// 1e
	u8 pad4[1];				// 1f
	u32 srcp;				// 20
	u32 endp;				// 24
	u8 pad5[0x28];
} MBootParam;
s32 BiosMBoot(MBootParam* p);

// LZ77�W�J
u32 BiosLZ77UnCompVram(const void* src, const void* dst); // word alignment!
u32 BiosLZ77UnCompWram(const void* src, const void* dst); // word alignment!


// ���̑�(BIOS�R�[���ł͂Ȃ����A�����Ő錾)
u32  GetPC();
void JumpR0();
void MySrand(u32 v);
u32 MyRand();

u32 InterlockedExchange(vu32* p, u32 val);
void Suspend();

#define IS_MBOOT() (*(u8*)0x020000c4)
#define ROM_SIZE 0x40000 // RAM�R�s�[�p


///////////////////////////////////////////////////////////////////////////////
// �Z�p�n
///////////////////////////////////////////////////////////////////////////////
// ����sin
s32 Sin64K(s32 angle);
static inline s32 Cos64K(s32 angle){ return Sin64K(angle + 0x4000);}
// ����sin(�����v�Z���x�����߂邽�߂̐��`�⊮�^)
s32 Sin64KX(s32 angle);
static inline s32 Cos64KX(s32 angle){ return Sin64KX(angle + 0x4000);}

void CalcDist1(s32 lat0, s32 lon0, s32 lat1, s32 lon1, u32* len, u32* houi);// �����x
void CalcDist2(s32 lat0, s32 lon0, s32 lat1, s32 lon1, u32* len, u32* houi);// ����
void CalcDist (s32 lat0, s32 lon0, s32 lat1, s32 lon1, u32* len, u32* houi);// ����

#define ANG64K(ang360) (ang360 * 0x10000 / 360) // �萔�쐬�p�Ȃ̂ŁA�S�u���Ȃ�����Z���g�p�c


///////////////////////////////////////////////////////////////////////////////
// �I�u�W�F(�X�v���C�g)
///////////////////////////////////////////////////////////////////////////////
#define BIGNUM_COUNT 11 // (0-9, '.')
#define BIGNUM_SIZE  (2*4)

#define VARIOOBJ_COUNT 8
#define VARIOOBJ_SIZE  (2*1)

#define VARIOOBJ_COUNT 8
#define NESWOBJ_SIZE  (1*2)

#define NEXT_PYLON_SIZE	(2*2)


// OBJ ID (0-127)
#define OBJ_MENU_CURSOR		0
#define OBJ_MENU_KETA		1
#define OBJ_MENU_UP			2
#define OBJ_MENU_DOWN		3

//�ȍ~��NAVI�p(���j���[���ɂ͓���)
#define OBJ_NAVI_START		4
#define OBJ_BIG_NUM_BEGIN	4 // �g�吔�� ���x4��+����4��
#define OBJ_BIG_NUM_END		15
#define OBJ_MY_POS			16	// ���@�ʒu�ڈ�
#define OBJ_STATUS			18	// �` ���ʏ��
#define OBJ_VARIO_CENTER	24
#define OBJ_STAR_BEGIN		25  // �` ���ΐ�
#define OBJ_NEXT_PYLON		37	// ���p�C����(�����p)
#define OBJ_NESW_BEGIN		38  // NESW
#define OBJ_VERIO_BEGIN		52	// �` �o���I���[�^�p
#define OBJ_VERIO_END		71
//RFU                       72-100 RFU
#define OBJ_NW_PYLN			101	// NaviWin �p�C����
#define OBJ_NW_PYLN2		102	// NaviWin ���p�C����(�����p)
#define OBJ_CIRCLE_BEGIN	103 // �`126 �O�g
#define OBJ_NW_BASE			127 // NaviWin �x�[�X(�Z�N�^)

// �����\���p
// BIGNUM                   4-7
#define OBJ_V_P1			8
#define OBJ_V_P1_END		19
#define OBJ_V_P2			20
#define OBJ_V_P2_END		27
#define OBJ_V_BOX			28
#define OBJ_V_BOX_END		94
#define OBJ_V_STATUS		95 // �����p(�D��x��)
// RFU                      96-102 RFU

#define OBJ_CYL_BEGIN		28 // Star�ȍ~�𗘗p
#define OBJ_CYL_END			125
#define OBJ_CYL_NW_PYLN		126

//Locus View
#define OBJ_LOCUS_NESW		4	// �`9 NESW+Self+Pylon
#define OBJ_LOCUS_TARGET	10	// 2�� WPT�ԍ�
#define OBJ_LOCUS			12	// �`127
#define OBJ_LOCUS_END		127	// (118��)


// �^�C���}�b�s���O (0-1023 ��2)
#define OBJ_TN_CURSOR		0 // ���j���[�I��
#define OBJ_TN_KETA			2 // ���l���͗p�J���b�g
#define OBJ_TN_UP			4 // �X�N���[�����
#define OBJ_TN_STAR0		6 // ���ΐ�
#define OBJ_TN_STAR1		8
#define OBJ_TN_STAR2		10
#define OBJ_TN_STATUS_NG1	12 // timeout
#define OBJ_TN_STATUS_NG2	14 // invalid
#define OBJ_TN_STATUS_2D	16 // 2D�ߑ�
#define OBJ_TN_STATUS_3D	18 // 3D�ߑ�
#define OBJ_TN_STATUS_GOOD	20 // SBAS�ߑ�
#define OBJ_TN_NESW			22 // �`35 ������k ����ʗp 16block(1x2*4)
#define OBJ_TN_SELF			38 // ���@
#define OBJ_TN_VARIO_C		42 // �o���I���[�^
#define OBJ_TN_VARIO_L		50
#define OBJ_TN_VARIO_R		52
#define OBJ_TN_SELF_R		50 // OBJ_TN_VARIO_L/R���㏑��

#define OBJ_TN_LEAD			54	// �����ݐ�(2x2)
#define OBJ_TN_GUIDE1		62	// �p�C�����K�C�h
#define OBJ_TN_GUIDE2		64	// �p�C�����K�C�h
#define OBJ_TN_GUIDE3		66	// �p�C�����K�C�h

#define OBJ_TN_CIRCLE		68  // ���΂̊O�g
#define OBJ_TN_CIRCLE_R		88  // ���΂̊O�g �c
#define OBJ_TN_SMLNUM		106 // ����(��)
#define OBJ_TN_SMLNUM_R		158 // ����(��) �c

// �ȍ~�͌v�Z�ŕ`�悷��
#define OBJ_TN_LOCUS		212 // �O��
#define OBJ_TN_UP_R			278
#define OBJ_TN_VARIO_CR		280
#define OBJ_TN_NEXT			288	// ���p�C����        32block(4x4)
#define OBJ_TN_NESW_R		336	// ������k �c��ʗp 16block(2x1*4)
#define OBJ_TN_VARIO		352	// �o���I����ʗp    32block(2x1*8)
#define OBJ_TN_VARIO_R		384	// �o���I�c��ʗp    32block(1x2*8)
#define OBJ_TN_BIGNUM		416	// �c��ʗp         176block(2x4*11)
#define OBJ_TN_BIGNUM_R		592 // �c��ʗp         176block(4x2*11)
#define OBJ_TN_PYLN			768 // �p�C�������      64block(8x8)
#define OBJ_TN_BASE			896 // �Z�N�^            64block(8x8)


// ��]�ݒ�(0-31)
#define OBJ_RN_PYLN		0	// �p�C����
#define OBJ_RN_BASE		1	// �x�[�X(�Z�N�^)
#define OBJ_RN_NEXT		2	// ���p�C����
#define OBJ_RN_CYL1		3	// �O�V�����_�K�C�h
#define OBJ_RN_CYL2		4	// ���V�����_�K�C�h
#define OBJ_RN_CYL3		5	// �Z�N�^�K�C�h1
#define OBJ_RN_CYL4		6	// �Z�N�^�K�C�h2


#define OBJSIZE_8x8		0x00000000
#define OBJSIZE_16x8	0x40000000
#define OBJSIZE_8x16	0x80000000
#define OBJSIZE_16x16	0x00004000
#define OBJSIZE_32x8	0x40004000
#define OBJSIZE_8x32	0x80004000
#define OBJSIZE_32x32	0x00008000
#define OBJSIZE_32x16	0x40008000
#define OBJSIZE_16x32	0x80008000
#define OBJSIZE_64x64	0x0000c000
#define OBJSIZE_64x32	0x4000c000
#define OBJSIZE_32x64	0x8000c000

#define A2_PRI1			(0x1 << 10)
#define A2_PRI2			(0x2 << 10)
#define A3_HFLIP		0x1000
#define A3_VFLIP		0x2000
#define A3_BLEND		(0x04000000)
#define A3_ROTATE_D(n)	(0x7000000 | ((n) << 9))
#define A3_ROTATE_N(n)	(0x5000000 | ((n) << 9))

void SetNormalObj(u32 id, u32 tile, u32 size, s32 x, s32 y);
void MoveObj(u32 id, s32 x, s32 y);
void MoveOut(u32 id);

void NDrawPoint(u32 offset, u32 xmask, u32 x, u32 y, u8 pal);
void NDrawHL2ine(u32 offset, u32 xmask, u32 x, u32 y, u32 w, u8 pal);
void NDrawRect (u32 offset, u32 xmask, u32 x, u32 y, u32 w, u32 h, u8 pal);

void InitPalette(u32 n);


///////////////////////////////////////////////////////////////////////////////
// �}�b�v
///////////////////////////////////////////////////////////////////////////////
// 0x00..0x5a * 2 = 182 ���p�p��
// 0x5b..0xff * 4 = 660 �S�p����
//               �v 842(0x34A)
// #842-#847:     = 6   �{�[�_�[ 
// #848:          = 1   ���j���[�I��1
// #849:          = 1   ���j���[�I��2
// #850-#895:     = 47  RFU
// map0  896-928  = 32
// map1  928-960  = 32
// map2  960-991  = 32
// map3: 992-1023 = 32

#define FONT_WIDE 0x5B
#define BD_INDEX (842)
#define BD_H		(BD_INDEX + 0)
#define BD_V		(BD_INDEX + 1)
#define BD_UL		(BD_INDEX + 2)
#define BD_UR		(BD_INDEX + 3)
#define BD_BL		(BD_INDEX + 4)
#define BD_BR		(BD_INDEX + 5)
#define SELMENU_UDL	(BD_INDEX + 6)
#define SELMENU_1	(BD_INDEX + 7)
#define SELMENU_11	(BD_INDEX + 8)
#define SELMENU_GRP	(BD_INDEX + 9)
#define SELMENU_GR2 (BD_INDEX + 9 + (1 << 10)) // ���E���]

#define SELMENU_0	(BD_INDEX + 10)
#define SELMENU_01	(BD_INDEX + 11)
#define SELMENU_BG	(BD_INDEX + 12)
#define SELMENU_BK1	(BD_INDEX + 13)
#define SELMENU_BK2	(BD_INDEX + 14)
#define SELMENU_BK3	(BD_INDEX + 15)
#define GRAPH_START	(BD_INDEX + 16)

static inline u32 IsHalf(u32 ch){ return 0x20 <= ch && ch < 0x20 + FONT_WIDE; }

void InitVideo();
void SetVideoMode(s32 t);
void UpdateLocusPalette(u32 pal);

void SelectMap(u32);
enum{
	MAP_BG0,
	MAP_BG1,
	MAP_BG2,
//	MAP_BG3,
};

void SetChar_Base(u32 map, u32 x, u32 y, u16 tile);
void FillTile(u32 map, u32 x0, u32 y0, u32 x1, u32 y1, u16 tile);
void DrawBox_Base(u32 x0, u32 y0, u32 x1, u32 y1);
void DrawBox     (u32 x0, u32 y0, u32 x1, u32 y1);
void FillBox     (u32 x0, u32 y0, u32 x1, u32 y1);
void MoveSelBg2  (u32 y, u32 x1, u32 x2, u32 type1, u32 type2);
#define MoveSelBg(y, type) MoveSelBg2((y), 0, 0, (type), 0)


///////////////////////////////////////////////////////////////////////////////
// �e�L�X�g�o��
///////////////////////////////////////////////////////////////////////////////
const u32 INT_POS_TABLE[10];
const u32 TIME_POS_TABLE[4];

// ���[���x���\������
void Locate (u32 x, u32 y);	// �J�[�\���ړ�
void LocateX(u32 x);		// y���W�͕ێ������܂�x���W�����ړ�
void Cls();					// ��ʃN���A
void ClsId(u32 id, u16 tile);
void Putc   (u8 ch);		// 1�����o��
void Putc2nd(u8 ch);		// ���C�h�L�����N�^�̌㔼���̕\��(���L�����X�N���[���p)
void PutsValX  (s32 v, s32 s, s32 keta, u8 pad, u32 f);	// �����o��
void PutsPointX(s32 v, s32 keta, s32 pre, s32 neg);		// �Œ菬���_�o��
void Puts(const u8* str);	// NULL�I�[������o��
u32  PutsLen(const u8* str);// �o�͕��v�Z(���p=+1,�S�p=+2)
void PutsLat(s32 v);		// �ܓx�o�͐�p
void PutsLon(s32 v);		// �o�x�o�͐�p

// ���O��p
void PutsName2(const u8* s, u32 len, u32 f);
#define PutsName(s)  (PutsName2((s), sizeof(s), 0)) // �L�������̂ݏo�́B�K���z��w��!
#define PutsNameB(s) (PutsName2((s), sizeof(s), 1)) // �󔒂ŕs�������𖄂߂�B�K���z��w��!
void PutsDistance (u32 m); // �E�l�ő�6����("   0m ", "1234m ", "9999km")
void PutsDistance2(s32 m); // ���l�ő�5����("0m   ",  "1234m",  "999km")

void PutsSpace(u32 n);					// �w�肵�����̋󔒂��o��
void PutsSpace2(s32 x, s32 y, u32 n);	// �w����W����󔒏o��
void PutsValSpace(u32 n, u32 keta);		// �����e�[�u�����󔒏o��

// ���W�w�蕶����o��
void DrawText(u32 x, u32 y, const u8* str);
void DrawTextCenter(u32 y, const u8* str);		// �Z���^�����O�\��(y���W�̂ݎw��)
void DrawTextUL(u32 x, u32 y, const u8* str);	// �A���_�[���C���t������
void UnderLine(u32 x, s32 y, s32 len);			// �A���_�[���C���̂�

// �Œ菬���\���}�N��
#define PutsPoint(v, keta, pr)     PutsPointX((v), (keta), pr, 0)	// �W��                ("12.3"��)
#define PutsPointSGN(v, keta, pr)  PutsPointX((v), (keta), pr, 2)	// ������������ON      ("+12.3"��)
#define PutsPoint0SGN(v, keta, pr) PutsPointX((v), (keta), pr, 6)	// �s������0�Ŗ��߂�   ("+012.3"��)

// �t�H�[�}�b�g�o��
u32  Putsf(const u8* fmt, ...);

// �|�b�v�A�b�v���b�Z�[�W���������ݒ�
void SetBootMsgTimeout(u32 n);

///////////////////////////////////////////////////////////////////////////////
// �T�E���h
///////////////////////////////////////////////////////////////////////////////

#define SOUND_CH_KEY	0 // �L�[�N���b�N�A��
#define SOUND_CH_VARIO	1 // �o���I���h�L

void EnableSound(u32 n, s32 vol);
u32  EnableSoundCheck(u32 n);
void PlaySG1 (const u16* s);	// �O�̉���������~���ĉ���炷
void PlaySG1X(const u16* s);	// �O�̉����I�����Ă���Ƃ������炷
void SetCh2Tone(u16 freq);		// �o���I�p�g�[���ݒ�

// ���F��`
extern const u16 SG1_OK[3];
extern const u16 SG1_CHANGE[3];
extern const u16 SG1_CANCEL[3];
extern const u16 SG1_SELECT[3];
extern const u16 SG1_PREV[3];
extern const u16 SG1_NEXT[3];
extern const u16 SG1_PYLON[3];
extern const u16 SG1_CONNECT[3];
extern const u16 SG1_MODE1[3];
extern const u16 SG1_MODE2[3];
extern const u16 SG1_MODE3[3];
extern const u16 SG1_MODE4[3];
extern const u16 SG1_EXIT[3];
extern const u16 SG1_CLEAR[3];
extern const u16 SG1_NEAR[3];
extern const u16 SG1_COMP1[3];
extern const u16 SG1_COMP2[3];
extern const u16 SG1_ALARM1[3];
extern const u16 SG1_ALARM2[3];
extern const u16 SG1_ALARM3[3];
extern const u16 SG1_ALARM4[3];

// �����f�[�^�w�b�_
#define WAV_MAGIC 0xAA550001

typedef struct {
	u32 magic;
	u32 count;
	u32 total_size;
	u32 rfu;
	u32 index[256 - 4];
} GbaWaveHeader;

typedef struct {
	u16 size;
	s16 freq;
	s8 data[1];
} VoiceData;

void VoiceStart(u32);
void VoiceStop();


///////////////////////////////////////////////////////////////////////////////
// Garmin�v���g�R���X�^�b�N
///////////////////////////////////////////////////////////////////////////////

typedef u32 EStatus;
enum {
	ERROR_SUCCESS			= 0,

	// LP���x���ōđ�
	ERROR_BAD_PID			= 10,
	ERROR_LENGTH			= 11,	// �����G���[
	ERROR_CHECKSUM			= 12,	// �`�F�b�N�T���G���[
	ERROR_DLE				= 13,	// DLE�X�^�b�t�B���O�G���[
	ERROR_LP_LEVEL,

	// �A�v�����x���őΉ�
	ERROR_TIMEOUT			= 100,	// �^�C���A�E�g
	ERROR_TOO_SHORT_BUFFER	= 101,
	ERROR_SENDING			= 102,	// ���ɒN�������M��
	ERROR_TOO_BIG_PACKET	= 103,	// ���M�p�P�b�g���傫������
	ERROR_APP_LEVEL,

	// �f�[�^���x��
	ERROR_INVALID_FIX		= 200,	// ������FIX�l
};

// ��{�p�P�b�gID
enum {
    Pid_Ack_Byte       =   6,
    Pid_Nak_Byte       =  21,
    Pid_Protocol_Array = 253,
    Pid_Product_Rqst   = 254,
    Pid_Product_Data   = 255
};

// �����N�v���g�R���P
enum {
    Pid_Command_Data     = 10,
    Pid_Xfer_Cmplt       = 12,
    Pid_Date_Time_Data   = 14,
    Pid_Position_Data    = 17,
    Pid_Prx_Wpt_Data     = 19,
    Pid_Records          = 27,
    Pid_Rte_Hdr          = 29,
    Pid_Rte_Wpt_Data     = 30,
    Pid_Almanac_Data     = 31,
    Pid_Trk_Data         = 34,
    Pid_Wpt_Data         = 35,
    Pid_Pvt_Data         = 51,
    Pid_Rte_Link_Data    = 98,
    Pid_Trk_Hdr          = 99
};

// �R�}���hID
enum {
    Cmnd_Abort_Transfer  = 0,  // ���݂̓`���̒��~
    Cmnd_Transfer_Alm    = 1,  // �O���v�f�̓`��
    Cmnd_Transfer_Posn   = 2,  // �ʒu�̓`��
    Cmnd_Transfer_Prx    = 3,  // �ߐڃE�F�C�|�C���g�̓`��
    Cmnd_Transfer_Rte    = 4,  // ���[�g�̓`��
    Cmnd_Transfer_Time   = 5,  // �����̓`��
    Cmnd_Transfer_Trk    = 6,  // �O�Ղ̓`��
    Cmnd_Transfer_Wpt    = 7,  // �E�F�C�|�C���g�̓`��
    Cmnd_Turn_Off_Pwr    = 8,  // �d���̐ؒf
    Cmnd_Start_Pvt_Data  = 49, // PVT�f�[�^�̓`���J�n
    Cmnd_Stop_Pvt_Data   = 50  // PVT�f�[�^�̓`���I��
};

enum {
    FIX_UNUSABLE    = 0,   // ��ԃ`�F�b�N���s
    FIX_INVALID     = 1,   // �������邢�͗��p�s�\ //
    FIX_2D          = 2,   // �Q����
    FIX_3D          = 3,   // �R����
    FIX_2D_diff     = 4,   // �f�B�t�@�����V�����Q�c
    FIX_3D_diff     = 5    // �f�B�t�@�����V�����R�c
};

typedef struct {
    u8 alt[4];			// float  WGS84�ȉ~�̂���̍��x(m)
    u8 epe[4];			// float  ����ʒu�덷�A2��(m)
    u8 eph[4];			// float  epe�̐�������(m)
    u8 epv[4];			// float  epe�̐�������(m)
    u8 fix[2];			// int    ���ʂ̕��@
    u8 tow[8];			// double �T�̎���(s)
    u8 lat[8];			// double �ܓx(radian)
    u8 lon[8];			// double �o�x(radian)
    u8 east[4];			// float  ���x�̓�����(m/s)
    u8 north[4];		// float  ���x�̖k����(m/s)
    u8 up[4];			// float  ���x�̏㐬��(m/s)
    u8 msl_hght[4];		// float  WGS84�ȉ~�̂�MSL��(m)
    u8 leap_scnds[2];	// int    GPS��UTC�̍�(s)
    u8 wn_days[4];		// long   �T�ԍ���(����: 1989/12/31 UTC���猻�݂̏T�̏��߂܂ł̓���)
} D800_Pvt_Data_Type;

// D800_Pvt_Data_Type�̓A���C�����g�������Ă��Ȃ��̂�PvtX�ɓ���Ȃ����Ďg��
#define INVALID_VAL 0x80000000
typedef struct {
	u32	counter;	// ��M�J�E���^
	u32 fix_sum[6];	// �e���ʂ̃J�E���^
    u32 fix;        // ���ʂ̕��@
    s32	lat;		// �ܓx[��]
    s32	lon;		// �o�x[��]
	s32	alt_mm;		// ���x[mm] (WGS84�ȉ~�� or ���ϊC�ʍ��x)
	s32	epe_mm;		// �덷�A2��(mm)
	s32	eph_mm;		// ����epe(mm)
	s32	epv_mm;		// ����epe(mm)
	s32	up_mm;		// �㏸���x(mm/s) ���̂Ƃ��͉��~���x
	s32	vh_mm;		// �������x(mm/s) ��0
	s32	v_mm;		// ���x
	s32 vn_mm;		// �����k����
	s32 ve_mm;		// ����������
	s32 g_mm;		// �����x

	u32	h_ang64;	// ��������(0000 �` ffff)
	u32	h_ang_c;	// ���Ε␳
	u32	v_ang64;	// ��������(c000 �` 4000) 0000-4000���㏸
	s32 ldK;		// L/D * 1000

	// ����
	u32 year;	// 1900�`
	u32 month;	// 1�`12 
	u32 day;	// 1�`31
	u32 week;	// 0:���j���`
	u32 hour;	// 0�`23
	u32 min;	// 0�`59
	u32 sec;	// 0�`59
	u32 dtime;	// 1989, 12/31 12:00AM UTC����o�߂����b��

	// ���Ϗ��
	s32 ldK_avg;	// ����L/D
	s32 vh_avg;		// ���ϑΒn���x
	s32 up_avg;		// ���ϐ������x

	s32 up_turn;	// Lift[mm]/Turn
	s32 up_r;		// ��]���a[m]
	s32 up_time;	// ��]���� s/Turn

	// �O���C�_�[�X�e�[�g
	u32 gstate;
	// PVT�^�C���A�E�g�Ď��p
	u32 pvt_timeout;
	u32 rx_pre;
} PvtX;

// D100,D101,D102,D103,D104,D107,D151,D152,D154,D155
// >56 byte
typedef struct {
    u8 ident[6];	// ���ʎq
	u8 lat[4];		// Semicircle �ܓx
	u8 lon[4];		// Semicircle �o�x
    u32 unused;		// �O���ݒ肳���
	u8  cmng[40];	// �R�����g
} D10X_Wpt_Type;

// D108
// 49�`59 byte
typedef struct {
	u8 wpt_class;	// byte �N���X
    u8 color;		// byte �F
	u8 dspl;		// byte �\���I�v�V����
	u8 attr;		// byte ����
    u8 smbl[2];		// Symbol_Type �E�F�C�|�C���g�V���{��
	u8 subclass[18];// u8[18] �T�u�N���X
	u8 lat[4];		// Semicircle �ܓx
	u8 lon[4];		// Semicircle �o�x
	u8 alt[4];		// float ���x(m)
	u8 dpth[4];		// float �[�x(m)
	u8 dist[4];		// float �ڋߋ���(m)
	u8 state[2];	// short ���
	u8 cc[2];		// short ���R�[�h
	u8 ident[1];	// �ϒ�������(attr�ˑ�)
} D108_Wpt_Type ;

// 
// D109
// 49�`59 byte
typedef struct {
	u8 wpt_class;	// byte �N���X
    u8 color;		// byte �F
	u8 dspl;		// byte �\���I�v�V����
	u8 attr;		// byte ����
    u8 smbl[2];		// Symbol_Type �E�F�C�|�C���g�V���{��
	u8 subclass[18];// u8[18] �T�u�N���X
	u8 lat[4];		// Semicircle �ܓx
	u8 lon[4];		// Semicircle �o�x
	u8 alt[4];		// float ���x(m)
	u8 dpth[4];		// float �[�x(m)
	u8 dist[4];		// float �ڋߋ���(m)
	u8 state[2];	// short ���
	u8 cc[2];		// short ���R�[�h
	u32 etc;		// etc;
	u8 ident[1];	// �ϒ�������(attr�ˑ�)
} D109_Wpt_Type ;

// D110
// 49�`59 byte
typedef struct {
	u8 wpt_class;	// byte �N���X
    u8 color;		// byte �F
	u8 dspl;		// byte �\���I�v�V����
	u8 attr;		// byte ����
    u8 smbl[2];		// Symbol_Type �E�F�C�|�C���g�V���{��
	u8 subclass[18];// u8[18] �T�u�N���X
	u8 lat[4];		// Semicircle �ܓx
	u8 lon[4];		// Semicircle �o�x
	u8 alt[4];		// float ���x(m)
	u8 dpth[4];		// float �[�x(m)
	u8 dist[4];		// float �ڋߋ���(m)
	u8 state[2];	// short ���
	u8 cc[2];		// short ���R�[�h
	u32 etc;		// etc;
	u8 temp[4];		// float ���x
	u32 time;		// �^�C���X�^���v
	u16 wpt_cat;	// waypoint �J�e�S���[
	u8 ident[1];	// �ϒ�������(attr�ˑ�)
} D110_Wpt_Type ;

//D105 11�`21 byte
typedef struct {
	u8 lat[4];		// Semicircle �ܓx
	u8 lon[4];		// Semicircle �o�x
	u8 smbl[2];		// Symbol_Type �E�F�C�|�C���g�V���{��
	u8 ident[1];	// NULL�I�[
} D105_Wpt_Type ;

//D106 25�`35 byte
typedef struct {
	u8 wpt_class;	// �N���X
	u8 subclass[13];// �T�u�N���X
	u8 lat[4];		// Semicircle �ܓx
	u8 lon[4];		// Semicircle �o�x
	u8 smbl[2];		// Symbol_Type �E�F�C�|�C���g�V���{��
	u8 ident[1];	// NULL�I�[
} D106_Wpt_Type ;

//D150 115byte
typedef struct {
	u8 ident[6];	// ���ʎq
    u8 cc[2];		// ���R�[�h
    u8 wpt_class;	// �N���X
	u8 lat[4];		// Semicircle �ܓx
	u8 lon[4];		// Semicircle �o�x
	u8 alt[2];		// ���x(m)
    u8 city[24];	// �s
    u8 state[2];	// �B
    u8 name[30];	// �{�ݖ�
    u8 cmnt[40];	// �R�����g
} D150_Wpt_Type ;


typedef struct {
	u8 lat[4];		// Semicircle �ܓx
	u8 lon[4];		// Semicircle �o�x
	u8 dtime[4];	// 1989, 12/31 12:00AM UTC �o�ߕb
	u8 new_trk;
} D300_Trk_Point_Type;

typedef struct {
	u8 lat[4];		// Semicircle �ܓx
	u8 lon[4];		// Semicircle �o�x
	u8 dtime[4];	// 1989, 12/31 12:00AM UTC �o�ߕb
	u8 alt[4];		// float ���x(m)
	u8 dpth[4];		// float �[�x(m)
    u8 new_trk;
} D301_Trk_Point_Type;

typedef struct {
    u8 dspl;		// �\���t���O
    u8 color;		// �\���F
	u8 trk_ident[];	// ���O�BMAX51byte
} D310_Trk_Hdr_Type;

#define INVALID_LAT 0x80000000
#define NMEA_DEV_ID 9999 // GARMIN���u�ɑ��݂��Ȃ�ID

enum {
	// NMEA�Z���e���X
	NMEA_LOG_ID_GGA,
	NMEA_LOG_ID_GSA,
	NMEA_LOG_ID_GSV,
	NMEA_LOG_ID_RMC,
	NMEA_LOG_ID_VTG,
	NMEA_LOG_ID_ZDA,
	NMEA_LOG_ID_GLL,
	NMEA_LOG_ID_ALM,

	// �G���[�֘A
	NMEA_LOG_ID_UNKNOWN,
	NMEA_LOG_ID_FIELD_ERR,
	NMEA_LOG_ID_TALKER_ERR,

	// ���O�p�ɉq�������R�R�ɒǉ�(PvtX�ł��ǂ����c)
	NMEA_LOG_ID_SAT_NUM,

	NMEA_LOG_ID_COUNT
};


enum{
	DL_FSM_WAIT_DLE,	// 0:DLE�҂�
	DL_FSM_WAIT_PID,	// 1:PID�҂�
	DL_FSM_WAIT_ANY,	// 2:�C�ӂ̃L�����N�^�҂�
	DL_FSM_GET_DLE,		// 3:DLE�擾����

	DL_FSM_NMEA_SUM,	// 4:NMEA'*'�܂�
	DL_FSM_NMEA_CR,		// 5:NMEA�I�[�҂�

	DL_FSM_ATC_WAIT,	// 6:AT�R�}���h���s�҂�(Bluetooth�p)
	DL_FSM_ATC_ANY,		// 7:AT�R�}���h�o�b�t�@(Bluetooth�p)

	// ����ȍ~�̏�Ԃ̓A�v���ŗv���N���A���K�v
	DL_FSM_COMPLETE,	// 8:GARMIN��M����
	DL_FSM_NMEA_END,	// 9:NMEA��1�Z���e���X�I��
	DL_FSM_ATC_END,		//10:AT�R�}���h��������(Bluetooth�p)

	// �ȍ~�A�G���[
	DL_FSM_E_UART,		//11:I/O�G���[
	DL_FSM_E_DROP,		//12:��肱�ڂ��G���[
	DL_FSM_E_PACKET,	//13:�o�b�t�@�s��(���肦�Ȃ��p�P�b�g��)
};

typedef struct {
	s32 dif[4];
	s32 val[4];
	s32 prc;
	u32 tpos;	// ���݂̃g���b�N���O�|�C���^
	u32 rep;
	u32 seg;

	u32 pre_sect;
	u8* sect_ptr;
} TrackInfo;

typedef struct {
	u32 state;		// �ʐMFSM

	s32 timeout;	// �ʐM�^�C���A�E�g�p�̃J�E���^
	s32 emu_nak;	// �G�~�����[�V�������[�h�^�C���A�E�g���o�p
	u32 vbc;		// �^�C���A�E�g�Ď��pVBC

	u16 pid;		// ���i���
	u16 version;	// FW�o�[�W����
	u8  name[128];	// ���i��
	u8  parray[128];// �L���p�r���e�B

	u32 pre_pvt;	// PVT�^�C���A�E�g�Ď�
	u32 pre_count;	// �_�E�����[�h�^�C���A�E�g�Ď��p
	u32 dl_accept;	// �_�E�����[�h���t���O(���f�p)
	u32 dl_num;		// �_�E�����[�h����
	u32 dl_count;	// �_�E�����[�h�o�ߐ�
	u32 dl_route;	// ���[�g�_�E�����[�h/�A�b�v���[�hIndex
	u32 dl_route_end;// �A�b�v���[�h�I�[
	u32 ul_acknak;	// ACK/NAK�҂�
	u32 ul_track;	// �A�b�v���[�h�J�n���̑��g���b�N��
	u32 ul_pre;		// ���O�̃A�b�v���[�h�J�E���^
	TrackInfo tinfo;// ���݂̃g���b�N���O���
} GarminInfo;

// GPS Ctrl
enum {
	GPS_GET_PINFO,
	GPS_GET_PINFO_WAIT,

	GPS_WPT,
	GPS_WPT_WAIT,

	GPS_ROUTE,
	GPS_ROUTE_WAIT,

	GPS_TRACK,
	GPS_TRACK_WAIT,

	// Bluetooth�p�����ǉ�
	GPS_BT_BR_CHECK1,
	GPS_BT_BR_WAIT1,
	GPS_BT_BR_CHECK2,
	GPS_BT_BR_WAIT2,
	GPS_BT_BR_CHECK3,
	GPS_BT_BR_WAIT3,
	GPS_BT_BR_CHECK4,
	GPS_BT_BR_WAIT4,
	GPS_BT_BR_CHECK5,
	GPS_BT_BR_WAIT5,
	GPS_BT_BR_CHANGE,
	GPS_BT_BR_CHANGE_WAIT,
	GPS_BT_BR_CANCEL,
	GPS_BT_BR_CANCEL_WAIT,
	GPS_BT_BR_ATH,
	GPS_BT_BR_ATH_WAIT,
	GPS_BT_BR_INFO,
	GPS_BT_BR_INFO_WAIT,
	GPS_BT_BR_INFO_CHECK,
	GPS_BT_BR_MODE0,
	GPS_BT_BR_MODE0_WAIT,
	GPS_BT_BR_RESET,
	GPS_BT_BR_RESET_WAIT,
	GPS_BT_BR_BOOTSLEEP,
	GPS_BT_BR_OK,


	GPS_BT_INQ_START,
	GPS_BT_INQ_WAIT,
	GPS_BT_INQ_DONE,

	GPS_BT_CON_SETKEY,
	GPS_BT_CON_SETKEY_WAIT,
	GPS_BT_CON_DIAL,
	GPS_BT_CON_DIAL_WAIT,
	GPS_BT_CON_DIAL_CHECK,
	GPS_BT_CON_STANDBY,
	GPS_BT_CON_STANDBY_WAIT,
	GPS_BT_CON_ATH,
	GPS_BT_CON_ATH_WAIT,
	GPS_BT_CON_ATH_CHECK,
	GPS_BT_CON_MODECHANGE,
	GPS_BT_CON_MODECHANGE_WAIT,
	GPS_BT_CON_RESET,
	GPS_BT_CON_RESET_WAIT,
	GPS_BT_CON_COMPLETE,

	GPS_BT_SCAN_START,
	GPS_BT_SCAN_START_WAIT,
	GPS_BT_SCAN_RESET,
	GPS_BT_SCAN_RESET_WAIT,
	GPS_BT_SCAN_COMPLETE,

	GPS_BT_IDLE_START,
	GPS_BT_IDLE_COMPLETE,

	GPS_BT_ERROR,


	GPS_STOP_DOWNLOAD,

	GPS_PVT,
	GPS_PVT_WAIT,

	GPS_EMUL_INFO,
	GPS_EMUL_INFO_WAIT,

	GPS_EMUL_WPT,
	GPS_EMUL_WPT_WAIT,

	GPS_EMUL_ROUTE,
	GPS_EMUL_ROUTE_WAIT,

	GPS_EMUL_TRACK,
	GPS_EMUL_TRACK_WAIT,

	GPS_EMUL_FINISH,
	GPS_EMUL_FINISH_WAIT,
};

static inline u32 NextIndex(u32 index, u32 max){
	return (index + 1 == max)? 0 : (index + 1);
}

// �A���C�����g���C�ɂ����f�[�^�擾���邽�߂̊֐�
s32 GetLong(const u8* p);
s16 GetInt (const u8* p);
void Copy4(void* dst, const void* src);
void Copy2(void* dst, const void* src);
static inline void SetDouble(void* dst, double d){
	Copy4(dst, ((u8*)&d) + 4);
	Copy4((u8*)dst + 4, &d);
}
static inline void SetFloat(void* dst, float f){
	Copy4(dst, &f);
}
static inline void SetLong(void* dst, s32 v){
	Copy4(dst, &v);
}
static inline void SetInt(void* dst, s16 v){
	Copy2(dst, &v);
}


void GetDateTime(u32 v, u32* year, u32* month, u32* day, u32* week, u32* hour, u32* min, u32* sec);
u32  GetDTTime  (u32 v, u32* hour, u32* min, u32* sec); // mod Time(���ɂ�)��Ԃ�
void GetDTDate  (u32 v, u32* year, u32* month, u32* day);

// GPS�ւ̃R�}���h���M
EStatus LpSendCmd(u16 cmd);

void StartGPSMode();

///////////////////////////////////////////////////////////////////////////////
// �����v���
///////////////////////////////////////////////////////////////////////////////
#define CALIB_END_TIME  (16384 * 10)	// �Œ�10�b

typedef struct {
	u16 calc_flag;
	u16 pre_level;
	u32 tm;			// 16384Hz / 72���ԃ^�C�}
	u32 pulse;
	u32 pre_tm;
	u32 dif_tm;

	// �␳�p
	u32 calib_flag;
	u32 calib_tm;
	u32 calib_pulse;
	u32 calib_input;
	u32 calib_vsum;
	u32 calib_vcnt;

	// �v�Z�ŋ��߂�l
	u32 avg_tm;		// ���ϗp�J�E���^
	u32 avg_pulse;	// ���ϗp�p���X�J�E���^
	u32 rpm;		// ���O�̒l
	u32 rpm_avg;	// 1�b(�ȏ�)����
	u32 vel;		// ����[m/s]
} Anemometer;

void CalcAnemometer();

///////////////////////////////////////////////////////////////////////////////
// ���[�V�������o���
///////////////////////////////////////////////////////////////////////////////
#define MAX_REC 60
typedef struct {
	u32 head;
	u32 tail;
	s32 val[MAX_REC];
} S32Queue;
typedef struct {
	S32Queue ang64;
	S32Queue vh_mm;
	S32Queue up_mm;
} PvtRec;
void AddS32Queue (S32Queue* sr, s32 val);
s32  AvgS32Queue  (const S32Queue* sr, u32 count);
s32  SumS32Queue  (const S32Queue* sr, u32 count);
s32  DiffXS32Queue(const S32Queue* sr, u32 count);
s32 S32QueueAngDiff(u32 count);
s32 S32QueueVhDiff (u32 count);

// �O���C�_���
enum {
	GSTATE_STOP			= 0,
	GSTATE_TURN_L		= 0x1,
	GSTATE_TURN_R		= 0x2,
	GSTATE_STRAIGHT		= 0x3,
	GSTATE_TURN_MASK	= 0x3, // MASK
	GSTATE_CENTERING	= 0x4,
	GSTATE_SPIRAL		= 0x8,
	GSTATE_STALL		= 0x10,
	GSTATE_CENTERING_X	= 0x20, // �Z���^�����O�����t���O(�����̎�1)
	// �X�e�[�g�͐�����]�̂݌v�Z�B�c��]�̃X�e�[�g�͏ȗ��c
	GSTATE_ROTATE		= GSTATE_CENTERING | GSTATE_SPIRAL,
	GSTATE_ROLL			= 0x00000100, // (���ۂɂ̓��[�����o�c)
	GSTATE_ROLL_MASK	= 0x0000ff00, // MASK
	GSTATE_ROLLTOP_L	= 0x00010000,
	GSTATE_ROLLTOP_X	= 0x00020000, // GSTATE_ROLLTOP����ւ��t���O
	GSTATE_PITCH		= 0x00100000,
	GSTATE_PITCH_MASK	= 0x0ff00000, // MASK
	GSTATE_PITCHTOP_D	= 0x10000000,
	GSTATE_PITCHTOP_X	= 0x20000000, // GSTATE_PITCHTOP����ւ��t���O
};

#define CETERING_DETECT (ANG64K(300)) // �Z���^�����O�ƔF����������

///////////////////////////////////////////////////////////////////////////////
// �E�F�C�|�C���g�A���[�g
///////////////////////////////////////////////////////////////////////////////
#define MAX_WPT			1000
#define MAX_ROUTE		20

#define WPT_NAMESIZE	10
#define ROUTE_NAMESIZE	14
// �E�F�C�|�C���g(20 byte)
typedef struct {
	u8	name[WPT_NAMESIZE];	// �E�F�C�|�C���g��
	s16	alt;		// ���x(m)
    s32	lat;		// �ܓx(��n)
    s32	lon;		// �o�x(��n)
} Wpt;

typedef struct {
	// �w�b�_���
	u32 magic;		// Route�����̂��ߖ��g�p
	u32 size;		// Route�����̂��ߖ��g�p
	u32 w_count;	// Route�����̂��ߖ��g�p
	u32 rfu;		// Route�����̂��ߖ��g�p

	Wpt wpt[MAX_WPT];
	s32 wpt_count;
	s32 def_ld;
} WptList;

typedef struct{
	u16 wpt;
	u16 cyl; // �V�����_�T�C�Y���e�p�C�����ɐݒ�ł���悤�ǉ�
} Pylon;

// ���[�g�f�[�^(416 byte)
#define MAX_ROUTE_PT 99
typedef struct {
	u8  name[ROUTE_NAMESIZE];	// ���[�g��
	u16	count;					// �E�F�C�|�C���g��
	u32 dist;					// ������[m]
	Pylon py[MAX_ROUTE_PT];// 0:WptID, 1:Cyl.
} Route;

typedef struct {
	// �w�b�_���
	u32 magic;		// FLBL_ROUTE_MAGIC�ł���΃��[�g���
	u32 size;		// Route/Wpt���v�T�C�Y
	u32 w_count;	// ���������񐔁BFlash�̏��������񐔃`�F�b�N�p
	u32 rfu;		// ���U�[�u�B�`�F�b�N�T���ɂł��g��?

	// ���[�g���
	Route route[MAX_ROUTE];
	s32 route_count;
} RouteList;


// �^�X�N���O�ۑ��p
typedef struct { // ���C�����
	u32 dtime;	// ��������
	u32 trip;	// �g���b�v���[�^[m]
	s32 lat;	// �����ܓx
	s32 lon;	// �����o�x
	s16 alt;	// [m] �������x

	u16	cyl;	// �f�t�H���g�V�����_���a or �p�C�����Q�b�g���̃V�����_���a
} TaskLog1; // 20 byte

typedef struct { // �T�u���B�󂫗̈悪����΋L�^����
	u8	wpt_name[WPT_NAMESIZE];	// �E�F�C�|�C���g��
	s16 wpt_alt;	// [m] �E�F�C�|�C���g���x
} TaskLog2; // 12 byte

// MAX:2044
typedef struct {
	u32 magic;		// ���O�p�}�W�b�N
	u32 log_time;	// ���O�p����
	u32 rt_dist;	// ���[�g����(�ύX�`�F�b�N�p)
	u8	rt_count;	// ���[�g��(�ύX�`�F�b�N�p)
	s8  pre_pylonX;	// �v���p�C�����ݒ�(�ۑ��p)
	u8  rt_name[ROUTE_NAMESIZE];	// ���[�g��
	u8  update_mark;
	u16 start_time;	// �ۑ��p
	union {
		TaskLog1 tl1[MAX_ROUTE_PT + 1]; // +1=takeoff	//	2000 byte
#define TLD_HEAD	(3 * 4 + 4 + ROUTE_NAMESIZE + 1 + 1)
#define TL2MAX	((2048 - TLD_HEAD - 4) / sizeof(TaskLog2))
		TaskLog2 tl2[TL2MAX]; // �\�Ȍ���E�F�C�|�C���g���𖄂߂�(62WPT�ȉ��Ȃ�S�ē���)
	};
} TaskLogData; // 2032 byte -> 2044 byte

typedef struct {
	u32 route_id;
	u32 pos;
	u32 dist[MAX_ROUTE];
} GoalDist;

void InitTaskLog(u32 keepTO);
void UpdateTaskLog(s32 n);
void UpdateAutoTargetLog(); // �I�[�g�^�[�Q�b�g�p�^�X�N���O
TaskLog2* GetTL2Addr(TaskLogData* tld, u32 n);

u32 SaveFLog(u32 pos, void* addr, u32 size, u32 msg_flag);

EStatus CalcPvtExt(PvtX* px);
EStatus CalcPvtGarmin(const D800_Pvt_Data_Type* pvt, PvtX* px);
EStatus CalcPvtNMEA(PvtX* px);
void    CalcDist(s32 lat0, s32 lon0, s32 lat1, s32 lon1, u32* len, u32* houi);
void    CalcWptDist(const Wpt* w0, const Wpt* w1, u32* len, u32* houi);


///////////////////////////////////////////////////////////////////////////////
// �R���t�B�O�ݒ���
///////////////////////////////////////////////////////////////////////////////
// �^�X�N�ݒ蓙 (Flash�ۑ��Ώ�)
typedef struct {
	// �w�b�_���
	u32 magic;		// FLBL_TASK_MAGIC�ł���΃^�X�N�ݒ���
	u32 size;		// sizeof(TaskConfig) - 16�B �ꉞ�T�C�Y���`�F�b�N����
	u32 w_count;	// ���������񐔁BFlash�̏��������񐔃`�F�b�N�p
	u32 rfu;		// ���U�[�u�B�`�F�b�N�T���ɂł��g��?

	// GPS���
	s32 tzone_m;	// �^�C���]�[��[��] (JST: +9 * 60 = +540)
	s32 n_calib;	// �k�␳ (�f�t�H���g��: -007��)
	s32 stop_dir;	// ���x������ȉ��̂Ƃ��́Aangle���X�V���Ȃ�
	u32 alt_type;	// 0: ���ϊC�ʍ��x, 1: WGS84�ȉ~�̍��x
	u32 calc_type;	// 0:�I�[�g�A 1: �Ȉ� 2:�����x
	s32 tally_clr;	// 0:���v���͕ێ��A1:�T�X�y���h���ɃN���A�A2:�X�^�[�g���ɃN���A
	u32 vario_mode;	// �o���I���[�h(0:�����A1:�L��)
	u32	vario_to;	// TO�O�̓o���I��炳�Ȃ�
	s32 vario_up;	// �o���I�㏸臒l
	s32 vario_down;	// �o���I���~臒l
	s32 waas_flag;	// WAAS���[�h(�\���̂�)
	u32 gps_warn;	// GPS�x��
	u32 nmea_up;	// NMEA�̏㏸���v�Z���@
	s32 rfu0[6];

	// �^�X�N���
	u32 task_mode;	// �^�X�N�^�C�v(0:�t���[�t���C�g�A1:�X�s�[�h���[�X�A2:�S�[�����[�X)
	s32	route;		// ���[�g�ԍ�
	u32 sector;		// �Z�N�^�T�C�Y(m)
	u32	cylinder;	// �V�����_���a(m)
	u32 rfu1;		// �X�^�[�g�V�����_(m) 0xffff��cylinder���g��
	u32 goal_type;	// �S�[���^�C�v
	u32 start_type;	// �X�^�[�g�^�C�v(0:�t���[�X�^�[�g�A1:�Z�N�^�[�C���A2:�Z�N�^�[�A�E�g)
	u32 start_time;	// �X�^�[�g����(��) 13:30�� 13*60+30 = 810�Bffff�͖���
	u32 close_time;	// �N���[�Y����(��) 13:30�� 13*60+30 = 810�Bffff�͖���
	u32 pre_pylon;	// �v���p�C������
	u32 cyl_end;	// �X�^�[�g�O�̃V�����_�i���ŃV�����_���[�h���甲����
	u32 skip_TO;	// �ŏ���TO���X�L�b�v
	s32 rfu2[1];

	// �I�[�g�^�[�Q�b�g
	u32 at_mode;	// �I�[�g�^�[�Q�b�g���[�h
	u32 at_min;		// �ŏ�����
	u32 at_max;		// �ő勗��
	u32 at_recheck;	// �Č�������

	// �i�r�ݒ�
	s32 view_mode;	// �r���[���[�h
	s32 near_beep;	// �����r�[�v(�V�����_�T�C�Y+��)
	s32 auto_lock;	// �����L�[���b�N
	s32 avg_type;	// L/D�v�Z���@
	s32 ar_max;		// �p�C�����A���[MAX[m]
	s32 ar_min;		// �p�C�����A���[MIN[m]
	s32 self_r;		// ���@�\�����a
	u32 spd_unit;	// �Βn���x�P��
	s32 my_ldK;		// �@�̂�L/D    (���[�X�p)
	s32 my_down;	// �@�̂̒����� (���[�X�p)
	s32 my_speed;	// �@�̂̑��x   (�����p)
	s32 spiral_spd;	// �X�p�C�������x
	s32 stall_spd;	// �X�g�[�����x
	s32 pitch_diff;	// �s�b�`���O���x��
	s32 pitch_freq; // �s�b�`���O�ő����
	s32 roll_diff;	// ���[���p�x��
	s32 roll_freq;	// ���[���ő����
	s32 start_spd;	// �X�^�[�g�X�s�[�h
	s32 pylon_type;	// �p�C�����\���^�C�v
	s32 initial_pos; // �C�j�V�����ʒu
	u32 vol_key;	// �L�[�{�����[��
	u32 vol_vario;	// �o���I�{�����[��
	u32 vm_enable;	// �����̗L��/����
	u32 vm_wait;	// �������[�h �e�C�N�I�t�܂��̉���
	u32 vm_normal;	// ���i���̉���
	u32 vm_stop;	// ��~���̉���
	u32 vm_center;	// �Z���^�����O���̉���
	s32 view_alt;	// �g���A���`�\��
	s32 anemo_unit;	// �ҋ@���x�P��
	u32 vm_near;	// �j�A�p�C�����̉���
	u32 vm_interval;// �����̊Ԋu
	u32 start_alarm;// �X�^�[�g�A���[��
	u32 sp_prio;	// SP���[�h�D��x

	s32 rfu3[3];

	// ���̑�
	s32 auto_off;	// �����d��OFF(��)
	s32 time_alarm;	// ���񃂁[�h
	s32 wind_cmd;	// ��������R�}���h
	s32 glider_cmd;	// �O���C�_��񑪒�R�}���h
	s32 thermal_cmd;// �T�[�}�����[�h�R�}���h
	s32 wind_update;// �O���C�_���̎������������t���O
	s32 bench_update;// �O���C�_���̎������������t���O
	s32 stable_angle;// �p�x�ω��}�[�W��
	s32 stable_speed;// ���x�ω��}�[�W��
	s32 init_wait;	 // �����E�F�C�g
	s32 wait_timeout;// �^�C���A�E�g�҂�
	s32 comp_timeout;// �R���v���[�g��̕\������
	s32 palette_conf;// �p���b�g�ݒ�
	s32 keep_range;	 // ���t�g�ł��V���N�ł����㏸��

	// �����v
	s32 anemo_coef;	// �����W��
	s32 cyl_cmd;	// �V�����_�}�b�v�R�}���h
	u32 cyl_near;	// �j�A�V�����_
	s32 rfu4[5];

	s32 locus_smp;	// �O�ՃT���v�����O����
	s32 locus_r;	// �O�Օ\�����a[m]
	s32 locus_cnt;	// �O�Ր�(MAX 120)
	s32 locus_up;	// �ヌ���W
	s32 locus_down;	// �������W
	s32 locus_range;// �J���[�����W
	u32 locus_pal;	// �O�ՃJ���[���[�h

	// Bluetooth
#define BT_ADDR_LEN 12	// �L�����N�^�Ŋi�[
#define BT_PIN_LEN	12	// �ő�12����(�d�l��Max16�����c)
	u32 bt_mode;		// Bluetooth�ڑ����[�h
	u8	bt_addr[BT_ADDR_LEN];// �ڑ���A�h���X
	u8	bt_pin [BT_PIN_LEN]; // �ڑ�PIN�R�[�h

	// �g���b�N���K�[
	u32 log_enable;	// �g���b�N���O�L��/����
	u32 rsv[2];
	u32 log_prec;	// �ۑ����x
	u32 log_intvl;	// ���O�ۑ��Ԋu
	u32 log_overwrite;// �㏑�����[�h
	u32 log_debug;

	u32 flog_as;	//�t���C�g���O�̃I�[�g�Z�[�u�ݒ�
	u32 tlog_as;	//�g���b�N���O�̃I�[�g�Z�[�u�ݒ�
	s32 rfu6[6];

} TaskConfig;


///////////////////////////////////////////////////////////////////////////////
// ���v���
///////////////////////////////////////////////////////////////////////////////
enum {
	TALLY_CENTER_COUNT,
	TALLY_CENTER_TURN,
	TALLY_CENTER_LIFT,
	TALLY_CENTER_LIFT_SUM,
	TALLY_CENTER_LIFT_MAX,
	TALLY_CENTER_SINK,
	TALLY_CENTER_SINK_SUM,
	TALLY_CENTER_SINK_MAX,
	TALLY_CENTER_SPEED,
	TALLY_CENTER_SPEED_MAX,
	TALLY_CENTER_G,
	TALLY_CENTER_G_MAX,
	TALLY_CENTER_TIMES,

	TALLY_CENTER_SIZE
};

#define VARIO_HIST_RANGE_MIN	100		// Max �}2.5m/s
#define VARIO_HIST_RANGE_MAX	2000	// Max �}80m/s
#define VARIO_HIST				48
#define VARIO_HIST_H			(VARIO_HIST / 2)

#define SPEED_HIST				50
#define SPEED_HIST_RANGE_MIN	1000  // Max 5km/s
#define SPEED_HIST_RANGE_MAX	50000  // Max 320km/s

#define ALT_HIST				50
#define ALT_HIST_RANGE_MIN		10000  // Max 500m
#define ALT_HIST_RANGE_MAX		100000 // Max 8000m

#define ANGLE_HIST				24

// Tally: 1044 byte
typedef struct {
	u32 magic;		// ���O�p�}�W�b�N
	u32 log_time;	// ���O�ŏI����(last_sec�Ɏ��Ă��邪�A�������2D�ȉ��ł��J�E���g)

	u32 takeoff_time;	// ���O�J�n����
	s32 start_lat;	// ���v�J�n�ʒu
	s32 start_lon;	// ���v�J�n�ʒu

	u32 count;		// ���v�J�E���^
	u64 sum_v;		// 3D���x���v
	u64 trip_mm;	// �g���b�v���[�^
	s32 last_lat;	// �Ō�̈ʒu(2D�ȏ�)
	s32 last_lon;	// �Ō�̈ʒu(2D�ȏ�)
	u32 last_sec;	// �Ō�̎���(2D�ȏ�)

	u32 sum_uv;		// �P�ʃx�N�g���J�E���^
	s32 sum_nv;		// �P�ʃx�N�g���k���v(�����p) 1024�{
	s32 sum_ev;		// �P�ʃx�N�g�������v(�����p) 1024�{
	s32 sum_nsv;	// �P�ʃx�N�g���k��Βl���v(�����p) 1024�{
	s32 sum_ewv;	// �P�ʃx�N�g���k��Βl���v(�����p) 1024�{
	u32 sum_gain;	// �g�[�^���Q�C��

	s32 max_v;		// �ō����x(3D)
	s32 max_up;		// �ō��㏸
	s32 min_up;		// �ō�����
	s32 max_alt;	// �ō����x
	s32 min_alt;	// �Œፂ�x
	s32 max_G;		// �ō������x
	s32 min_G;		// �Œ�����x

	s32 s_count;	// ���i���J�E���^
	s32 w_count;	// ��~���J�E���^
	s32 s_hv;		// ���i�����x���v(���i�����ϑ��x�p)
	s32 s_up;		// ���i��up���v(���i��L/D���ϗp)
	s32 turn_r;		// �E�����
	s32 turn_l;		// �������
	//	s32 pitch;		// �s�b�`���O��
	u16 pitch_s;	// ���i���s�b�`���O��(MAX65535�ɂȂ邪�\��?)
	u16 pitch_r;	// �Z���^�����O���s�b�`���O��
	s32 roll;		// ���[�����O��
	s32 center_sum;	// �Z���^�����O�p�x���v
	s32 spiral_sum;	// �X�p�C�����p�x���v
	s32 stall_sec;	// �X�g�[������

	s32 keep_range;

	s32 soaring_cnt[4];	// �\�A�����O���v
	s32 soaring_sum[4];	// �\�A�����O���v
	s32 sinking_cnt[4];	// �\�A�����O���v
	s32 sinking_sum[4];	// �\�A�����O���v
	s32 keeping_cnt[4];	// �\�A�����O���v
	s32 vario_cnt[4];	// �\�A�����O���v
	s32 vario_sum[4];	// �\�A�����O���v

	s32 centering[2][TALLY_CENTER_SIZE];

	s32 vario_hist_range;
	s32 vario_hist[VARIO_HIST];
	s32 speed_hist_range;
	s32 speed_hist[SPEED_HIST];
	s32 alt_hist_range;
	s32 alt_hist[ALT_HIST];
	s32 angle_hist[ANGLE_HIST];

} Tally;

void InitTally();


///////////////////////////////////////////////////////////////////////////////
// �O�Օ\���p
///////////////////////////////////////////////////////////////////////////////
#define MAX_LOCUS	1800 // 2s�T���v�����O��1����
#define LOCUS_LEVEL 10	 // 10�i�K�F����
#define LOCUS_BSFT	12	 // 100m���x (4000km�ȓ��Ȃ�1��A�n���̗��ɏu�ԃ��[�v���Ă�5��ŃA�W���X�g��)

// �O�Տ��
typedef struct {
	// lat/lon�̓������ߖ�̂��ߍ����ŕێ� (alt/up�̓O���t�ł��g���̂Ő�Βl���i�[)
	s16 lat_d;
	s16 lon_d;
	s16 alt_x; // �r�b�g0��LatLon�͈̔͊O�}�[�N�Ɏg��
	s16 up;
} LocusVal;

typedef struct {
	LocusVal val[MAX_LOCUS];
	u32 index;
	u32 sample;
} Locus;

typedef struct {
	s16 x;
	s16 y;
} Point;

// �X�v���C�g���`�����Ȃ��悤�ɁA�����X�v���C�g����}����
#define MAX_SPLITE	100 // 1���C���̍ő�X�v���C�g��
#define SPLITE_BASE	12	// VCount�؂�ւ��X�v���C�g�p�^�C���ԍ�
#define SPLITE_END	(128 - SPLITE_BASE)	// �ő�X�v���C�g�\����
// 808byte
typedef struct {
	u32 buf[MAX_SPLITE * 2];
	s16 pre_count;
	s16 cur_count;
	s16 full_check;
} SpliteHBuf;

// HSync���荞�݂ŃX�v���C�g�𑽏d�\��������
#define SPLITE_VCOUNT 20
// 18576byte
typedef struct {
	SpliteHBuf hBuf[SPLITE_VCOUNT]; // 808 * 20 =16160
	s32 flag;
	s32 cur_index;

	// near�͎��o���ʂ����コ���邽�߂̕��z�v�Z�p�e�[�u���ŁA�����X�v���C�g�����K��l��
	// �������ꍇ��cur_full���Z�b�g���ꎟ�񂩂�near�o�b�t�@�ł̕��z�`�F�b�N���L���ɂȂ�B
	// �������Acur_full���Z�b�g����Ă����j���[�\�����͔w�i�̃X�v���C�g��^�ʖڂɕ\����
	// ��K�v�͂Ȃ����߁A���z�X�v���C�g�͎g�p���Ȃ��B
	// �˂���𗘗p���A���j���[����FLog�`�F�b�N����ꍇ�́Anear�z��̃��������ؗp����I
	s32 pre_full;
	s32 cur_full;
	u8 near[240 / 4][160 / 4];		// 60 * 40 = 2400byte
} SpliteTable;


///////////////////////////////////////////////////////////////////////////////
// �g���b�N���O/���k
///////////////////////////////////////////////////////////////////////////////
typedef struct {
	s32 val[4]; // lat/lon/alt/sec;
} TrackLog;

typedef struct {
	u32 magic;
	u32 index;
} TrackLogHeader;

#define ABSF_SEPARETE 2
#define ABSF_CONTINUE 1

typedef struct {
	u32 index;			// ���O�C���f�b�N�X(���݂̃u���b�N�̏������݃I�t�Z�b�g
	u32 block;			// ���݂̃u���b�N(64KB�P��)
	u32 pkt_counter;	// �p�P�b�g�J�E���^(�����̃u�[�g�A�b�v��)
	u32 blk_counter;	// �u���b�N�J�E���^
	u32 abs_flag;		// ������Βl�p�P�b�g�t���O
	u32 full;			// �t���t���O
	u32 pre_pvtc;		// �X�V���o�p

	// �f�o�b�O�p���
	u32 t_count[16];	// PID�ʃJ�E���^
	u32 trk_counter;	// ���g���b�N��
	u32 err;			// �Ō�̏������݃G���[�l
	u32 drop;			// �������ݎ��s��

	// ���k���O
	s32 pre[4];			// ���O�l(Lat/Lon/Sec/Alt)
	s32 dif[4];			// ��K�����l(Lat/Lon/Sec/Alt)
	u32 pre_prec;		// ���O���x�ύX���o
	u32 repeat;			// ���s�[�g�J�E���^
	u32 opt;			// �I�v�V�����t���O
	s32 intvl_ctr;		// �C���^�[�o���J�E���^

	// ���݂̑��g���b�N���J�E���g(PC�A�b�v���[�h�p)
	u32 tc_state;	// ���g���b�N���v�Z�X�e�[�g
	u32 tc_lastadd;	// �Ō�Ƀg���b�N�ǉ�����VBC
	u32 tc_total;	// ���g���b�N��
	u32 tc_tpos;	// ���݂̌v�Z���̈ʒu�B�v�Z������͐擪�u���b�N�������B
} TrackLogParam;

enum {
	TC_NOTREADY,	// �v�Z�O
	TC_CALCULATING,	// �v�Z��
	TC_COMPLETE,	// �v�Z����
};

u32 TrackLogAdd(TrackLog* tl);
u32 TrackLogAddCurrentPVT();
u32 TrackLogClear();
void InitLogParams();

s32 FindFirstBlock();
s32 NextTrack(s32 countmode);

enum {
	TLOG_OPT_ENABLE		= 0x100,

	TLOG_OPT_LOST		= (1 << 0) | TLOG_OPT_ENABLE,
	TLOG_OPT_TIMEOUT	= (1 << 1) | TLOG_OPT_ENABLE,
	TLOG_OPT_MANUAL		= (1 << 2) | TLOG_OPT_ENABLE,
	TLOG_OPT_MASK		= 0xff,
};

// �g���b�N���O��64KB�P�ʂŃw�b�_�Ǘ�
#define BLOCK_SIZE			(64 * 1024) // �g���b�N���O�p
#define BLOCK_HEAD_SIZE		8
#define BLOCK_TRACK_SIZE	(BLOCK_SIZE - BLOCK_HEAD_SIZE)
#define BLOCK_TASK_SIZE		(8 * 1024)

typedef struct{
	u32 magic;
	s32 index; // �u���b�N�����ŏd�v�Ȃ̂͏�ʂ�8bit(����24bit��PacketCounter�͂ǂ��ł�����)
	u8 val[BLOCK_TRACK_SIZE];
} TrackBlock;

#define CUR_BLOCK_OFFSET(tlp) (FI_TRACK_OFFSET + (tlp)->block * BLOCK_SIZE)

u32 UploadLog(); // ���k�g���b�N���O��PC�֓]��

///////////////////////////////////////////////////////////////////////////////
// ���j���[�p
///////////////////////////////////////////////////////////////////////////////
enum{
	SP_VIEW_NONE,
	SP_VIEW_LOCUS,
	SP_VIEW_CYL,
	SP_VIEW_TEXT,
	SP_VIEW_WINDCHECK,
	SP_VIEW_BENCHMARK,

	SP_VIEW_COUNT
};

#define SAVEF_SAVE_WPT		1
#define SAVEF_UPDATE_CFG	2
#define SAVEF_UPDATE_WPT	4
#define SAVEF_UPDATE		(SAVEF_UPDATE_CFG|SAVEF_UPDATE_WPT)
#define SAVEF_CHANGE_WPT	(SAVEF_SAVE_WPT|SAVEF_UPDATE_WPT)

#define FLOG_SIZE (2 * 1024)
#define FLOG_COUNT 4
#define FLOG_TOTAL (FLOG_SIZE *  FLOG_COUNT)

// ���Ȃ���������L�����p���邽�߂ɁA�ʓ|�����\���̂ł�������T�C�Y���Ǘ�����
typedef u32 (*MenuProc)(u16 push);
typedef struct {
	u32 map;		// �I���̃}�b�v�ԍ�
	u32 gx;			// �e�L�X�g�J�[�\��X
	u32 gy;			// �e�L�X�g�J�[�\��Y
	u32 menuid;		// ���j���[�ԍ�
	u32 menu_x0;	// ���j���[��X0
	u32 menu_y0;	// ���j���[��X0
	u32 menu_x1;	// ���j���[��X0
	u32 menu_y1;	// ���j���[��X0
	const u8* help_msg;	// �w���v�X�N���[���ʒu
	u32 help_scr;	// �w���v�X�N���[���ʒu
	u32 help_vbc;	// �X�N���[�����x�����p
	MenuProc proc;	// ���j���[�֐�
	u32 sel;		// ���j���[�I���ʒu
	u32 pvt_check;	// PVT�X�V�`�F�b�N�p
	s32 wpt_sel;	// �E�F�C�|�C���g�I��
	s32 route_sel;	// ���[�g�I��
	s32 rtwpt_sel;	// ���[�gWPT�I��
	s32 rtwpt_dsp;	// ���[�gWPT�f�B�X�v���C�ʒu
	s32 rtwpt_state;	// ���[�gWPT�f�B�X�v���C�ʒu
	s32 rtwpt_calc;	// �������v�Z�J�E���^
	s32 rtwpt_sum;	// �������v�Z�J�E���^
	s32 ar_count;	// �I�[�g���s�[�g�̃J�E���^
	u32 ar_vbc;		// �I�[�g���s�[�g�p��VBC
	u16 ar_key;		// �I�[�g���s�[�g�̃L�[
	u32 mp_flag;	// �ėp�t���O
	u32 scr_pos;	// �X�N���[���ʒu
	u32 disp_mode;	// �\�����[�h
	u32 boot_msg;	// �u�[�g���b�Z�[�W�����p
	u32 name_table; // ���O���̓e�[�u��
	u32 save_flag;	// �Z�[�u�p�̕ύX�t���O
	u32 auto_off_cnt;//�����T�X�y���h�J�E���^
	u32 resume_vbc;	// ���W���[������
	u32 tips;		// tips�J�E���^
	u32 multiboot;	// �}���`�u�[�g�t���O
	u32 cyl_input;	// �V�����_�ʓ��͗p
	u32 pre_menu;	// �\���X�V�p���j���[���

	// CPU Load
	u32 load_test;
	u32 load_vbc;
	u32 last_load;
#define MAX_LOAD_LIST 16 // �� MyTask + 2!
	u32 load[MAX_LOAD_LIST];

	// �O���t�p
	s32 graph_mode; // �O���t���[�h(Alt/Vario)
	s32 graph_scale;// �O���t�X�P�[��
	s32 graph_range;// �o���I�����W
	s32 graph_minmax;// �O���t�^�C�v
	s32 min_alt; // �O���t�p �Œፂ�x
	s32 max_alt; // �O���t�p �ō����x
	s32 cur_lat;	// Locus�L�^�̍ŐV�ܓx
	s32 cur_lon;	// Locus�L�^�̍ŐV�o�x

	// �i�r�p
	u32 test_mode;	// �e�X�g���[�h
	u32 navi_update;// �X�V�t���O
	u32 key_lock;	// �L�[���b�N
	u32 cur_point;	// ���݂̃|�C���g�ԍ�
	u32 sect_ang64;	// ���̃Z�N�^�p
	u32 sect_type;	// ���̃Z�N�^�^�C�v
	u32 cur_view;	// ���݂̃r���[���[�h
	u32 cur_sect;	// ���݂̃Z�N�^�\��
	s32 pre_pylon;	// �O��̃p�C�����Q�b�g��
	s32 pre_index;	// �O��̃p�C�����ԍ�
	s32 pre_task;	// �O��̃^�X�N���[�h
	Route* pre_route;// �O��̃��[�g(�^�X�N���O�p)
	s32 sp_view;	// ��p�r���[���[�h
	s32 pre_sp_view;// �O��̐�p�r���[���[�h���
	s32 pre_sect;	// �O��̃Z�N�^
	s32 sp_enter;	// ��p�r���[���[�h�ɓ������Ƃ��̃R�}���h
	u32 sp_state;	// ��p���[�h�̃X�e�[�^�X
	u32 sp_angle;	// �����A�x���`�}�[�N�p�̑�����
	s32 sp_speed1;	// ��ꐅ�����x
	s32 sp_up1;		// ��ꐂ�����x
	u32 freeflight;	// ���������֎~
	u32 near_pylon;	// �ŒZ�p�C��������
	u32 cyl_dist;	// �V�����_�܂ł̋���
	u32 near_vbc;	// �j�A�T�E���h�p
	u32 pre_palette;// ���O��Locus�p���b�g

	// ����
	s32 voice_state;// �����X�e�[�^�X
	u32 voice_type;	// �����^�C�v
	u32 voice_chg;	// �ύX�\�t���O
	u32 vbuf_idx;	// �Đ��ӏ�
	u32 voice_vbc;	// ���p��
	s32 py_dir;		// �p�C��������(-180�`+180 [��])
	s32 py_dis;		// �p�C��������[m]
	s32 py2_dir;	// �����p�C��������(-180�`+180 [��])
	s32 py2_dis;	// �����p�C��������[m]

	// �j�AWPT
	u32 nw_target;	// �Ŋ�WPT
	u32 nw_search;	// WPT�����p
	u32 nw_s_dir;	// WPT��������
	u32 nw_cnd;		// WPT���
	u32 nw_cnd_len;	// WPT��⋗��
	u32 nw_tgt_len;	// �Ŋ�WPT����
	Wpt nw_start;	// �����J�n�ꏊ

	// �g���b�N���O
	TrackLogParam tlog;

	// ���v���(�d�������Ă���̓��v)
	Tally tally;
	u32* flog_addr;
	u32  flog_presave;
	u32  tlog_presave;
	u32	 flog_cache[FLOG_COUNT][2];	//���j���[�\���ɕK�v�ȃw�b�_�����̃L���b�V���p

	//�S�[���܂ł̋����v�Z�p
	GoalDist goal_dist;

	// GPS��M�f�o�b�O
	u32 last_pvt_len;
	u32 last_wpt_len;
	u32 last_route_len;
	u8  last_pvt[256];
	u8  last_wpt[256];
	u8  last_route[256];

	// �����^�[�Q�b�g�p
#define MAX_AUTOTARGET_CAND 32 // �ő�^�[�Q�b�g��␔�B�K�� 2^n �ɂ��邱��!
	u32 rand_val;
	u32 rand_init;

	u32 at_rollflag;
	u32 at_vbc;
	u32 atcand_count;
	u16 atcand[MAX_AUTOTARGET_CAND];

	// NMEA����p
	s32 nmea_xalt;

	// Bluetooth����p
	u32 pre_bt_mode;	// ���O��Bluetooth�ڑ����[�h
} MenuParam;


///////////////////////////////////////////////////////////////////////////////
// ���̓e���v���[�g�p
///////////////////////////////////////////////////////////////////////////////

// ���l���͗p�e���v���[�g
typedef struct {
	s32 keta;
	s32 disp;
	s32 prec;
	s32 min;
	s32 max;
	const u8* name;
	const u8* unit;
	s32* val;
} IntInputTemplate;
typedef struct {
	const IntInputTemplate* t;
	s32 pos;
	s32 val;
} IntInput;

//�������̓e���v���[�g
typedef struct {
	const u8* name;
	s32* val;
} TimeInputTemplate;
typedef struct {
	const TimeInputTemplate* t;
	s32 pos;
	s32 val;
} TimeInput;

// �񋓓��͗p�e���v���[�g
typedef struct {
	u32 max;
	u32* val;
	const u8* names[1];
} EnumInputTemplate;

// ���O���͗p�e���v���[�g
typedef struct {
	s32 max;
	const u8* name;
	u8* val;
} NameInputTemplate;
typedef struct {
	const NameInputTemplate* t;
	s32 pos;
	s32 x;
	s32 y;
	u8 val[32]; // ���͕�������MAX31����
} NameInput;

// �ܓx�o�x���͗p�e���v���[�g
typedef struct {
	s32 latlon;
	s32* val;
} LatInputTemplate;
typedef struct {
	const LatInputTemplate* t;
	s32 pos;
	s32 val;
} LatInput;

// ���͗p�f�[�^
typedef union {
	IntInput	i_int;
	TimeInput	i_time;
	NameInput	i_name;
	LatInput	i_lat;
} AnyInputParam;

// �E�F�C�|�C���g���͗p
typedef struct {
	u8	name[WPT_NAMESIZE];	// �E�F�C�|�C���g��
	s32	alt;		// ���x(m)
    s32	lat;		// �ܓx(��n)
    s32	lon;		// �o�x(��n)
} WptInput;

// �o���I���ǂ�
typedef struct {
	s32	cur_value;		// ���ݒl
	s32 cur_mode;		// ���݂̃��[�h
	s32	vario_test;		// �e�X�g�p
	u32	vario_blink;	// �Ԋu
	u32	vario_count;	// vsync�J�E���^
	u32 vario_vbc;		// vbc
} Vario;


///////////////////////////////////////////////////////////////////////////////
// �i�r
///////////////////////////////////////////////////////////////////////////////
enum {
	NAVI_CHECK_PVT		= 1 << 0,
	NAVI_CHECK_WPT		= 1 << 1,
	NAVI_CHECK_ROUTE	= 1 << 2,

	NAVI_ROUTE_DL		= 1 << 4,
	NAVI_PVT_CHANGE		= 1 << 8,

	NAVI_UPDATE_PVT		= NAVI_CHECK_PVT,
	NAVI_UPDATE_WPT		= NAVI_UPDATE_PVT | NAVI_CHECK_WPT,
	NAVI_UPDATE_ROUTE	= NAVI_UPDATE_WPT | NAVI_CHECK_ROUTE,

};

#define VIEW_MODE_CYLINDER	20
#define VIEW_MODE_LOCUS		24
#define VIEW_MODE_TEXT		28
#define MAX_VIEW_MODE		36
#define MAX_VIEW_MODE_NONDEBUG (MAX_VIEW_MODE - 4)

void ChangeNaviBasePal(s32 i, u32 f);

// �e�X�g����p
s32  DummyPlay();
void DummyPlayUpdate();

void InitNearNavi();
void SetNaviStartPos();


///////////////////////////////////////////////////////////////////////////////
// File I/F (�A�v���͒���Flash��SCSD���A�N�Z�X�����AFile IF���o�R���Đ��䂷��)
///////////////////////////////////////////////////////////////////////////////
enum {
	FLASH_SUCCESS = 0,

	FLASH_E_NO_CART			= 10,	// �F���\�ȃJ�[�g���b�W�Ȃ�
	FLASH_E_PARAM			= 11,	// �֐��p�����[�^�G���[
	FLASH_E_MAGIC			= 12,	// �}�W�b�N�G���[
	FLASH_E_SIZE			= 13,	// �T�C�Y�G���[
	FLASH_E_OFFSET			= 14,	// �I�t�Z�b�g�w��G���[

	// �g���b�N���O��p
	FLASH_E_LOG_NEWBLOCK	= 20,	// ���u���b�N�؂�ւ�
	FLASH_E_LOG_FULL		= 21,	// ���O�����ς�

	// JoyCarry�n
	FLASH_E_ID				= 30,	// ID�G���[
	FLASH_E_STATUS_TIMEOUT	= 31,	// �X�e�[�^�X�擾�^�C���A�E�g
	FLASH_E_SR_VCC			= 32,	// VCC�G���[
	FLASH_E_SR_PROTECT		= 33,	// �v���e�N�g�G���[
	FLASH_E_SR_WRITE		= 34,	// �������݃G���[
	FLASH_E_SR_ERASE		= 35,	// �����G���[
	FLASH_E_SR_CMD			= 36,	// �R�}���h�G���[
	FLASH_E_NOT_FFFF		= 37,	// �����G���[2
	FLASH_E_COMPARE			= 38,	// �R���y�A�G���[

	// SCSD�n��ǉ�
	FLASH_E_NO_STORAGE		= 50,	// �X�g���[�W�Ȃ�
	FLASH_E_NO_FILE			= 51,	// ParaNavi.dat�Ȃ�
	FLASH_E_MBR_ERROR		= 52,	// ���T�|�[�g��FAT
	FLASH_E_SECT_ERROR		= 53,	// ���T�|�[�g�̃Z�N�^�T�C�Y
	FLASH_E_CLST_ERROR		= 54,	// ���T�|�[�g�̃N���X�^�T�C�Y
	FLASH_E_DAT_SIZE		= 55,	// DAT�T�C�Y���傫������
	FLASH_E_CNUM_RANGE		= 56,	// �N���X�^�ԍ��ُ̈�

};

#define THEAD_SIZE			0x10
#define TDataSize(x)		(sizeof(x) - THEAD_SIZE)
#define FLBL_TASK_MAGIC		0x036b7354	// �^�X�N�}�W�b�N("Tsk?")
#define FLBL_ROUTE_MAGIC	0x01657452	// ���[�g�}�W�b�N("Rte?")
#define FLBL_TRACK_MAGIC	0x016b7254	// �g���b�N�}�W�b�N("Trk?")
#define FLOG_MAGIC_FLIGHT	0x014c4c46	// �t���C�g���O�}�W�b�N(FLL?)
#define FLOG_MAGIC_TASK		0x014c5354	// �^�X�N���O�}�W�b�N  (TKL?)

#define FI_CONFIG_OFFSET	0x00000000	// 8KB
#define FI_FLOG_OFFSET		0x00002000	// 8KB
#define FI_ROUTE_OFFSET		0x00004000	// 32KB
//                          0x0000C000�`16KB�̓��U�[�u
#define FI_TRACK_OFFSET		0x00010000	// 64KB�ȏ�

#define ROUTE_WPT_SIZE (1024 * 30 - 0x100)
#define BOOTBIN_SIZE 0x2000 //�L���v���O�����̈�B8KB

#define CARTCAP_WRITEABLE	1	// ���������\�J�[�g���b�W
#define CARTCAP_DIRECTMAP	2	// ���ǂ݉\�J�[�g���b�W
#define CARTCAP_DUPLICATE	4	// �����p�f�[�^����
#define CARTCAP_SECTWRITE	8	// �Z�N�^�P�ʂŏ�������

// �ȗ����p�}�N��
#define CART_IS_WRITEABLE() (IW->cw.cap & CARTCAP_WRITEABLE)
#define CART_IS_DIRECTMAP() (IW->cw.cap & CARTCAP_DIRECTMAP)
#define CART_IS_DUPLICATE() (IW->cw.cap & CARTCAP_DUPLICATE)
#define CART_IS_SECTWRITE() (IW->cw.cap & CARTCAP_SECTWRITE)

// C�ŃN���X�p�����g����Ηǂ��̂����c
typedef struct {
	s32 (*InitCart)();
	const u32* (*ReadDirect)(u32 offset);// �\�ł���΃������R�s�[���Ȃ�����Read(512byte�܂ŕۏ�)
	s32 (*ReadData) (u32 offset,       void* dst, s32 size, u32 magic); // magic����v���Ă���ꍇ�̂�read����
	s32 (*WriteData)(u32 offset, const void* src, s32 size);
	s32 (*EraseBlock)(u32 offset, s32 size, u32 mode);// �u���b�N����
	s32 (*AppendTLog)(u8* buf, u32 size);	// �g���b�N���O�ǉ��������ݐ�p
	s32 (*Flush)();							// �o�b�t�@�t���b�V��
	const u8* (*GetCodeAddr)(u32 type);		// �����p�̃v���O�����A�h���X����Ԃ�
} CartIF;
// ��ReadDirect/ReadData/WriteData/EraseBlock��Offset�̓Z�N�^���E���w�肷�邱��!

#define ERASE_FF		0	// �ʏ����(FF����)
#define ERASE_MAGIC		1	// �擪�}�W�b�N�̂ݏ������鍂���C���[�X
#define ERASE_CLEAR		2	// �l�������p (TODO �č����h����NSA�K�i�Ƃ�?)
#define ERASE_TRACK		3	// �g���b�N���O�����N���A�p

#define CODE_TYPE_BOOT	0
#define CODE_TYPE_NAVI	1
#define CODE_TYPE_VOICE	2
#define CODE_TYPE_TRACK	3
#define CODE_TYPE_TASK	4
#define CODE_TYPE_FLOG	5
#define CODE_TYPE_RTWPT	6

#define SECT_SIZE 512 // �Ƃ肠����512�Z�N�^�̂݃T�|�[�g

// �G�~�����[�^�p /////////////////////////////////////////////////////////////
const u8* Emul_GetCodeAddr(u32 type);

typedef struct {
	union { // �Z�N�^�L���b�V��
		u32 scache32[SECT_SIZE / 4]; // �g���₷���悤�ɂ��낢��ȃT�C�Y�Łc
		u16 scache16[SECT_SIZE / 2]; // �g���₷���悤�ɂ��낢��ȃT�C�Y�Łc
		u8  scache8 [SECT_SIZE    ]; // �g���₷���悤�ɂ��낢��ȃT�C�Y�Łc
	};
} EmulWork;

// �W���C�L�����[�J�[�g���b�W /////////////////////////////////////////////////
typedef struct {
	u32 pre_unlock;
} JoyCWork;

// SCSD ///////////////////////////////////////////////////////////////////////
typedef struct {
	// FAT���
	u32 sect_size;
	u32 sect_shift;
	u32 clst_size;		//
	u32 clst_shift;		// �N���X�^�v�Z�p�̃V�t�g�l
	u32 clst_limit;
	u32 fat_start;		// FAT�Z�N�^�J�n�ʒu
	u32 dir_start;		// ���[�gDir�Z�N�^�J�n�ʒu
	u32 dat_start;		// �f�[�^�Z�N�^�J�n�ʒu
	u32 datf_min;		// DAT�t�@�C���̃����W�`�F�b�N�p
	u32 datf_max;		// DAT�t�@�C���̃����W�`�F�b�N�p

	// �t�@�C�����
	u32 file_head;
	u32 file_size;

	u32 cache_sect; // �L���b�V���Z�N�^ (0 �͖���)
	u32 tlog_offset;// �g���b�N���O�L���b�V���I�t�Z�b�g
	u32 tlog_dirty;	// �g���b�N���O�L���b�V���_�[�e�B�r�b�g
	u32 tlog_vbc;	// �Œ�X�V���ԊǗ�

	union { // �Z�N�^�L���b�V��
		u32 scache32[SECT_SIZE / 4]; // �g���₷���悤�ɂ��낢��ȃT�C�Y�Łc
		u16 scache16[SECT_SIZE / 2]; // �g���₷���悤�ɂ��낢��ȃT�C�Y�Łc
		u8  scache8 [SECT_SIZE    ]; // �g���₷���悤�ɂ��낢��ȃT�C�Y�Łc
	};
	union { // �g���b�N���O�������݃o�b�t�@
		u32 tlog_cache32[SECT_SIZE / 4]; // �g���₷���悤�ɂ��낢��ȃT�C�Y�Łc
		u16 tlog_cache16[SECT_SIZE / 2]; // �g���₷���悤�ɂ��낢��ȃT�C�Y�Łc
		u8  tlog_cache8 [SECT_SIZE    ]; // �g���₷���悤�ɂ��낢��ȃT�C�Y�Łc
	};
} SCSDWork;

// ���� ///////////////////////////////////////////////////////////////////////
typedef struct {
	u32 cap;
	u32 tlog_block;
	u32 init_error;

	u32 phy_boot;
	u32 phy_navi;
	u32 phy_config;
	u32 phy_flog;
	u32 phy_rtwpt;

	u32 phy_tlog;
	u32 phy_voice;

	union {
		EmulWork emul_work;
		JoyCWork joyc_work;
		SCSDWork scsd_work;
	};
} CartWork;


// ���̂Ƃ���A�g����͈̂ȉ���4��
extern const CartIF CIF_NONE;
extern const CartIF CIF_EMULATOR;
extern const CartIF CIF_JOYCARRY;
extern const CartIF CIF_SUPERCARD_SD;

///////////////////////////////////////////////////////////////////////////////
// Bluetooth�p
#define MAX_ATC_RESPONSE 100
enum {
	ATCRES_NONE,
	ATCRES_OK,
	ATCRES_ERROR,
	ATCRES_CONNECT,
	ATCRES_DISCONNECT,
};

#define MAX_INQ_DEV 9 // �ő�9�f�o�C�X�܂Ō���
typedef struct {
	u32 inq_count;
	u32 last_mode;
	u32 baudrate_err;
	u8	inq_list[MAX_INQ_DEV][MAX_ATC_RESPONSE + 1]; // �X�L���������f�o�C�X�����i�[
} ATCInfo;

u32 IsBluetoothAddr(const u8* p);

///////////////////////////////////////////////////////////////////////////////
// RAM BLOCK
///////////////////////////////////////////////////////////////////////////////
// �悭�g���ϐ���IWRAM��ɔz�u�BIWRAM��32KB�����Ȃ��̂ŁA��������Ǘ����邱�Ƃɂ���(�ʓ|�����ǁc)
typedef struct {
	u8	global_reserve	[0x1000 - 0x0000];	// �O���[�o���ϐ��p�̃��U�[�u(�w�ǎg���Ă��Ȃ����f�o�b�O�p��4KB�m�ہc)

	// Intr(16 byte)
	vu32 vb_counter;//	[0x1004 - 0x1000]
	vu16 intr_flag;	//	[0x1006 - 0x1002]
	u8	 intr_rfu		[0x1010 - 0x1006];

	// SIO (2032 byte)
	vu32 tx_head;	//	[0x1014 - 0x1010]
	u32	 tx_tail;	//	[0x1018 - 0x1014]
	u32  rx_head;	//	[0x101C - 0x1018]
	vu32 rx_tail;	//	[0x1020 - 0x101C]
	vu32 rx_drop;	//	[0x1024 - 0x1020]
	vu32 uart_error;//	[0x1028 - 0x1024]
	u8	 tx_buf			[0x1400 - 0x1028];	//  984 byte
	u8	 rx_buf			[0x1800 - 0x1400];	// 1024 byte

	// Key (16 byte)
	vu16 key_state;	//	[0x1802 - 0x1800]
	vu16 key_chatter;//	[0x1804 - 0x1802]	// �`���^�����O�΍�
	u8	 key_rfu		[0x1810 - 0x1804];	// 12 byte

	// Garmin DataLink stack(272 byte)
	u32	dl_drop;	//	[0x1814 - 0x1810]
	u32	dl_drop_pkt;//	[0x1818 - 0x1814]
	u32	dl_timeout;	//	[0x181C - 0x1818] 
	u32	dl_fsm;		//	[0x1820 - 0x181C]
	u32	dl_size;	//	[0x1824 - 0x1820]
	u8	dl_loghead		[0x1828 - 0x1824];	//	4byte �f�o�b�O���O�p
	u8	dl_buf			[0x1928 - 0x1828];	//	256 byte
	u32	dl_nmea_chk;//	[0x192c - 0x1928]; // �`�F�b�N�T���i�[�p
	u32	dl_nmea_idx;//	[0x1930 - 0x192c]; // �`�F�b�N�T��Index
	u32	dl_nmea_ack;// 	[0x1934 - 0x1930]; // �ŏIACK-ID
	u8 	dl_rfu			[0x1940 - 0x1934];

	// GPS�f�[�^
	PvtX px;			// �ŐVPVT
	PvtX px2;			// 1�O��PVT or NMEA�̒��ԃf�[�^�p�o�b�t�@
	u32  nmea_log[16];	// NMEA��M���O
	u32  nmea_fix;		// NMEA�Z���e���X�ʎ�M�t���O
	u32  nmea_wgs84;	// WGS84���[�h�ؑ֍ς݃t���O
	D800_Pvt_Data_Type pvt_raw; // �f�o�b�O�p
	PvtRec pr;
	u8 	px_rfu			[0x1E00 - 0x1940 - sizeof(PvtX) * 2 - sizeof(u32) * 18 - sizeof(D800_Pvt_Data_Type) - sizeof(PvtRec)];

	// �^�X�N�ݒ� �ۑ��Ώ�(512 byte)
	TaskConfig tc;
	u8	tc_rfu			[0x2000 - 0x1E00 - sizeof(TaskConfig)];

	// �o���I���h�L�p
	Vario vm;			
	u8	vm_rfu			[0x2020 - 0x2000 - sizeof(Vario)];

	// WPT���͗p
	WptInput wpt_input;
	u8 wpt_rfu			[0x2100 - 0x2020 - sizeof(WptInput)];
	u16 wpt_sort[MAX_WPT];
	u32 wpt_sort_type;
#define SORTED_MARK 0x1000
#define GetSortMark() (IW->wpt_sort_type & SORTED_MARK)
#define GetSortType() (IW->wpt_sort_type & (SORTED_MARK - 1))
	u8 sort_rfu			[0x2900 - 0x2100 - MAX_WPT * sizeof(u16) - 4];

	// ���[�g���͗p
	Route route_input;
	u8 ri_rfu			[0x2B00 - 0x2900 - sizeof(Route)];

	// �v���~�e�B�u���͗p
	AnyInputParam aip;
	u8 aip_rfu			[0x2C00 - 0x2B00 - sizeof(AnyInputParam)];

	// GPS���(512 byte)
	GarminInfo gi;
	u8 gi_rfu			[0x2E00 - 0x2C00 - sizeof(GarminInfo)];

	// �^�X�N���O(2KB)
	TaskLogData	task_log;
	u8 tl_rfu			[0x3600 - 0x2E00 - sizeof(TaskLogData)];

	// �����o�b�t�@
#define VBUF_SIZE 126
	u32 voice_ctr;	//  [0x2E04 - 0x2E00]
	u16 voice_buf[VBUF_SIZE];//[0x2F00 - 0x2E04]

	// �����v
	Anemometer anemo;
	u8 anemo_rfu		[0x3800 - 0x3700 - sizeof(Anemometer)];

	// etc 4KB
	MenuParam mp;
	u8 mp_rfu			[0x4800 - 0x3800 - sizeof(MenuParam)];

	// Cart Work buf
	CartWork cw;
	u8 cw_rfu			[0x5000 - 0x4800 - sizeof(CartWork)];

	// RFU
	u8 rfu				[0x6000 - 0x5000];

	// stack etc
	u8  isr_cpy			[0x6FFC - 0x6000];	//�@���荞�݂�IW���ARM���s
	const CartIF* cif;						//  Cart����4 byte(Init�ŏ���������Ȃ��悤�R�R�ɒu��)
	u8  i_heap			[0x7C00 - 0x7000];	//
	u8  usr_stack		[0x7F00 - 0x7C00];	// 764 byte
	u8	isr_stack		[0x7FA0 - 0x7F00];	// 160 byte
	u8	svc_stack		[0x7FE0 - 0x7FA0];	// 64 byte
	u8	alloc_area		[0x7FF0 - 0x7FE0];	// 16 byte
	u32 sound_buf;	//	[0x7FF4 - 0x7FF0]	// 4 byte
	u32 alloc_ptr;	//	[0x7FF8 - 0x7FF4]	// 4 byte
	u32 intr_check;	//	[0x7FFC - 0x7FF8]	// 4 byte
	u32 irq_handler;//	[0x8000 - 0x7FFC]	// 4 byte
} MyIWRAM;

extern MyIWRAM* const IW;
extern WptList* const WPT;
extern RouteList* const ROUTE;
extern Locus* const LOCUS;
extern SpliteTable* const SPLITE;

#define VOICE_BUF_SIZE (8 * 1024) // 8KB�ŏ\���H
#define VOICE_FIFO_MAX (VOICE_BUF_SIZE / 16)

// �ȉ��ARAM�Ŏg�p����̈�B��납��ςށB
#define WPT_ADDRESS		(EX_WRAM_END - 20 * 1024)				// �Ō��20KB  20 * 1000 + 4(19.5KB) �ۑ����Ɠ����t�H�[�}�b�g
#define ROUTE_ADDRESS	(EX_WRAM_END - 30 * 1024)				// �Ō��10KB 416 *   20 + 4( 8.1KB) �ۑ����Ɠ����t�H�[�}�b�g
#define VOICE_ADDRESS	(ROUTE_ADDRESS - VOICE_BUF_SIZE)		// 8KB (Flash�����������̉���т�h������RAM�ɃR�s�[���Ė炷)
#define LOCUS_ADDRESS	(VOICE_ADDRESS - sizeof(Locus))			// 14.1KB 2�b�Ԋu��1���ԋL�^
#define SPLITE_ADDRESS	(LOCUS_ADDRESS - sizeof(SpliteTable))	// 18.1KB ���d�X�v���C�g�pDMA�o�b�t�@
#define FLOG_PTR		((u32*)SPLITE->near)					// Flog(2KB)�̓��j���[�\���������g��Ȃ��̂ŃX�v���C�g���z�e�[�u��(2.4KB)�̈�𗬗p����
#define ATC_PTR			((ATCInfo*)SPLITE->near)				// ATC(1KB)�̓��j���[�\���������g��Ȃ��̂ŃX�v���C�g���z�e�[�u��(2.4KB)�̈�𗬗p����
// �}���`�u�[�g�ɑΉ����邽�߂ɂ̓v���O������256KB�ȉ��ɗ}����K�v�����邪�A��L�f�[�^��
// RAM�Ɋm�ۂ��邽�߁A�v���O�����͎���256KB-78KB=176KB�ɐ�������邱�Ƃɒ��ӁI
// ����Ȃ��Ȃ�����v���O���������k���邱�Ƃ��l����K�v����B

#define PROGRAM_LIMIT (SPLITE_ADDRESS)
extern const u8 __iwram_overlay_lma; // PROGRAM�I�[ (&__iwram_overlay_lma < PROGRAM_LIMIT �ł��邱��)

u32 UpdateVBC(u32* pre);
#define VBC_TIMEOUT(v) ((v) * 60)

Route* GetTaskRoute();
Wpt* GetWptInfo(const Route* rt, s32 offset);
Wpt* GetCurWpt (const Route* rt, s32 offset);
u16  GetCurCyl (const Route* rt, u32 offset);
Wpt* GetDefaultLD();
Wpt* GetNearTarget();

u32 MP_Navi(u16 push);
s32 IsMenu();


#define DBGDMP_GPS  0x10000
#define DBGDMP_SCSD 0x20000
#define DBGDMP_COM  0x40000
#define DBGDMP_TYPE 0xf0000

#define IsEmulator() (IW->cif == &CIF_EMULATOR)

///////////////////////////////////////////////////////////////////////////////
// �o���I
///////////////////////////////////////////////////////////////////////////////
#define VARIO_TEST_DISABLE 0x10000000


///////////////////////////////////////////////////////////////////////////////
// �^�X�N
///////////////////////////////////////////////////////////////////////////////
s32 MT_GPSCtrl();
s32 MT_DataLink();
s32 MT_AppLayer();
s32 MT_Navi();
s32 MT_Menu();
s32 MT_Vario();
s32 MT_Voice();
s32 MT_TrackLog();

#endif

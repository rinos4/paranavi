///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// �L�[�N���b�N���≹����������̃t�@�C���ɂ܂Ƃ߂��B(�o���I���h�L�͕ʃt�@�C��)


///////////////////////////////////////////////////////////////////////////////
// �ݒ�
///////////////////////////////////////////////////////////////////////////////
const u16 ENABLE_SOUND0 = 0x1100;

void EnableSound(u32 n, s32 vol){
	if(vol == -1){
		REG16(REG_SGCNT0_L) &= ~(ENABLE_SOUND0 << n);
	} else {
		REG16(REG_SGCNT0_L) |= (ENABLE_SOUND0 << n) | (vol << 4) | (vol);
	}
}

u32 EnableSoundCheck(u32 n){
	return REG16(REG_SGCNT0_L) & (ENABLE_SOUND0 << n);
}
void SetCh2Tone(u16 freq){
//	REG16(REG_SG20) = 0xf800;
	REG16(REG_SG20) = 0x1800 | (IW->tc.vol_vario << 13);
	REG16(REG_SG21)	= 0x8000 | freq;
}

// �L�[�N���b�N�����Đ�
void PlaySG1(const u16* s){
	s32 sg10 = s[1] | (IW->tc.vol_key << 13);
	REG16(REG_SG10_L) = s[0];
	REG16(REG_SG10_H) = sg10;
	REG16(REG_SG11)   = s[2];
	EnableSound(SOUND_CH_KEY, 7);
	// ���@�ł́A�Ȃ���2�x�������Ȃ��Ɖ����o�Ȃ����Ƃ�����!?
	REG16(REG_SG10_L) = s[0];
	REG16(REG_SG10_H) = sg10;
	REG16(REG_SG11)   = s[2];
}
void PlaySG1X(const u16* s){
	if((REG16(REG_SGCNT1) & 1) == 0) PlaySG1(s);
}


///////////////////////////////////////////////////////////////////////////////
// �T�E���h��`
///////////////////////////////////////////////////////////////////////////////
#define NO_STOP 0x8fff

// t: Sweep Time; units of 7.8ms (0-7, min=7.8ms, max=54.7ms)
// f: Sweep Frequency Direction  (0=Increase, 1=Decrease)
// n: Number of sweep shift      (n=0-7)
#define SG10L(t, f, n)         (((t) << 4) | ((f) << 3) | (n))

// v:Initial Volume of envelope          (1-15, 0=No Sound)
//dr:Envelope Direction                  (0=Decrease, 1=Increase)
// t:Envelope Step-Time; units of n/64s  (1-7, 0=No Envelope)
//dt:Wave Pattern Duty                   (0-3, see below)
// l:Sound length; units of (64-n)/256s  (0-63)
//#define SG10H(v, dr, t, dt, l) (((v) << 12) | ((dr) << 11) | ((t) << 8) | ((dt) << 6) | (l))
#define SG10H(v, dr, t, dt, l) (((1) << 12) | ((dr) << 11) | ((t) << 8) | ((dt) << 6) | (l)) // �{�����[���͐ݒ肩��ςɂ���

// f:Frequency; 131072/(2048-n)Hz  (0-2047)
#define SG11(f)                (0xc000 | (f))

const u16 SG1_OK[3]		= {	SG10L(7,0,7), SG10H(15, 0, 1, 2,  0), SG11(2000) };
const u16 SG1_CHANGE[3]	= {	SG10L(1,0,7), SG10H(15, 0, 1, 2,  0), SG11(1900) };
const u16 SG1_CANCEL[3]	= {	SG10L(0,1,0), SG10H(15, 1, 0, 2, 40), SG11( 900) };
const u16 SG1_SELECT[3]	= {	SG10L(1,1,7), SG10H(15, 0, 3, 2, 60), SG11(1700) };
const u16 SG1_PREV[3]	= {	SG10L(6,1,6), SG10H(15, 0, 1, 2, 20), SG11(1600) };
const u16 SG1_NEXT[3]	= {	SG10L(6,0,6), SG10H(15, 0, 1, 2, 20), SG11(1600) };

const u16 SG1_PYLON[3]	= {	SG10L(4,0,7), SG10H(15, 0, 7, 2, 0), SG11(1700) & NO_STOP};
const u16 SG1_CONNECT[3]= {	SG10L(7,0,4), SG10H(15, 0, 7, 2, 0), SG11(1600) & NO_STOP};
const u16 SG1_MODE1[3]	= {	SG10L(7,0,2), SG10H(15, 0, 7, 2, 0), SG11( 500) & NO_STOP};
const u16 SG1_MODE2[3]	= {	SG10L(7,0,2), SG10H(15, 0, 7, 2, 0), SG11( 750) & NO_STOP};
const u16 SG1_MODE3[3]	= {	SG10L(7,0,2), SG10H(15, 0, 7, 2, 0), SG11(1000) & NO_STOP};
const u16 SG1_MODE4[3]	= {	SG10L(7,0,2), SG10H(15, 0, 7, 2, 0), SG11(1250) & NO_STOP};
const u16 SG1_EXIT[3]	= {	SG10L(3,1,7), SG10H(15, 0, 7, 2, 0), SG11(1900) & NO_STOP};
const u16 SG1_CLEAR[3]	= {	SG10L(1,1,4), SG10H(15, 0, 7, 2, 0), SG11(1900)};
const u16 SG1_NEAR[3]	= {	SG10L(7,1,3), SG10H(15, 0, 1, 2, 0), SG11(2000)};
const u16 SG1_COMP1[3]	= {	SG10L(0,1,0), SG10H(15, 0, 7, 2, 0), SG11(1760) & NO_STOP};
const u16 SG1_COMP2[3]	= {	SG10L(5,0,5), SG10H(15, 0, 7, 2, 0), SG11(1600)};

const u16 SG1_ALARM1[3]	= {	SG10L(0,1,0), SG10H(15, 0, 7, 2, 0), SG11(1875) & NO_STOP};
const u16 SG1_ALARM2[3]	= {	SG10L(0,1,0), SG10H(15, 0, 7, 2, 0), SG11(1725) & NO_STOP};
const u16 SG1_ALARM3[3]	= {	SG10L(0,1,0), SG10H(15, 0, 7, 2, 0), SG11(1575) & NO_STOP};
const u16 SG1_ALARM4[3]	= {	SG10L(0,1,0), SG10H(15, 0, 7, 2, 0), SG11(1425) & NO_STOP};


///////////////////////////////////////////////////////////////////////////////
// �����ݒ�
///////////////////////////////////////////////////////////////////////////////
enum {
	VOICE_10 = 10,
	VOICE_100 = 19,
	VOICE_1000 = 28,
	VOICE_10000 = 37,
	VOICE_TEN,
	VOICE_COMMA,

	VOICE_LIFT,
	VOICE_SINK,
	VOICE_DO,
	VOICE_HIDARI,
	VOICE_MIGI,
	VOICE_KITA,
	VOICE_HIGASHI,
	VOICE_MINAMI,
	VOICE_NISHI,
	VOICE_HOKU,

	VOICE_TO,
	VOICE_NAN,
	VOICE_SEI,
	VOICE_JISOKU,
	VOICE_BYOSOKU,
	VOICE_M,
	VOICE_KM,
	VOICE_PYLON,
	VOICE_KYORI,
	VOICE_KODO,

	VOICE_HOUI,
};

// �����^�C�v
enum {
	VC_WAIT,		// �E�F�C�g(���̃e�[�u���l���g��)
	VC_WAIT2,		// �����؂�ւ��E�F�C�g(���̃e�[�u���l���g��)
	VC_INTERVAL,	// ���[�U�ݒ�̃C���^�[�o��
	VC_UNIT_KODO,	// "���x"
	VC_VAL_KODO,	// ���ݍ��x[m]
	VC_VAL_KODO2,	// ���ݍ��x[m]
	VC_VAL_LIFT,	// ���݃��t�g[m/s] (���t�g/�V���N0�̂Ƃ��̓X�L�b�v)
	VC_VAL_LIFT2,	// ���݃��t�g[m/s] (����)
	VC_UNIT_HOUI,	// "����"
	VC_VAL_HOUI,	// ���݂̕���[16����]
	VC_VAL_HOUI2,	// ���݂̕���[360��]
	VC_UNIT_SOKUDO,	// "���x"
	VC_VAL_SOKUDO,	// ���ݑ��x[km/h]
	VC_VAL_PYLON,	// �p�C�����ԍ�
	VC_VAL_PY_DIR,	// �p�C��������
	VC_VAL_PY_DIR2,	// �p�C��������
	VC_VAL_PY_DIS,	// �p�C��������
	VC_VAL_PY_DIS2,	// �p�C��������
	VC_UNIT_PY_DIS,	// �p�C����"����"
};

const u16 DOT_WAIT[10] = {
	0, 0, 10, 0, 0, 10, 0, 0, 0, 0
};

#define MY_WAIT(x)  VC_WAIT,  (x / 17) // MAX
#define MY_WAIT2(x) VC_WAIT2, (x / 17) // MAX
const u16 VSEQ_NONE[] = {
	MY_WAIT2(1000),
};
const u16 VSEQ_ALT[] = {
	MY_WAIT2(1000),
	VC_UNIT_KODO,	// "���x"
	VC_VAL_KODO,	// ���ݍ��x[m]
	MY_WAIT2(30000)
};
const u16 VSEQ_ALT_I[] = {
	MY_WAIT2(1000),
	VC_UNIT_KODO,	// "���x"
	VC_VAL_KODO,	// ���ݍ��x[m]
	VC_INTERVAL
};

const u16 VSEQ_ALT_LIFT[] = {
	MY_WAIT2(1000),
	VC_VAL_LIFT2,	// ����"���t�gxx[m/s]"
	MY_WAIT2(1000),
	VC_UNIT_KODO,	// "���x"
	VC_VAL_KODO,	// ���ݍ��x"xx[m]"
};
const u16 VSEQ_ALT_LIFT_I[] = {
	MY_WAIT2(1000),
	VC_VAL_LIFT2,	// ����"���t�gxx[m/s]"
	MY_WAIT2(1000),
	VC_UNIT_KODO,	// "���x"
	VC_VAL_KODO,	// ���ݍ��x"xx[m]"
	VC_INTERVAL
};

const u16 VSEQ_PYLON[] = {
	MY_WAIT2(1000),
	VC_VAL_PYLON,	// �p�C�����ԍ�
	MY_WAIT(200),
	VC_VAL_PY_DIR,	// �p�C��������
	MY_WAIT(400),
	VC_UNIT_PY_DIS,	// �p�C����"����"
	VC_VAL_PY_DIS,	// �p�C��������
	MY_WAIT2(1000)
};
const u16 VSEQ_PYLON_I[] = {
	MY_WAIT2(1000),
	VC_VAL_PYLON,	// �p�C�����ԍ�
	MY_WAIT(200),
	VC_VAL_PY_DIR,	// �p�C��������
	MY_WAIT(400),
	VC_UNIT_PY_DIS,	// �p�C����"����"
	VC_VAL_PY_DIS,	// �p�C��������
	VC_INTERVAL
};

const u16 VSEQ_NORMAL[] = {
	MY_WAIT2(1000),
	VC_VAL_PYLON,	// �p�C�����ԍ�
	MY_WAIT(200),
	VC_VAL_PY_DIR,	// �p�C��������
	MY_WAIT2(400),
	VC_UNIT_PY_DIS,	// �p�C����"����"
	VC_VAL_PY_DIS,	// �p�C��������
	MY_WAIT2(3000),
	VC_UNIT_KODO,	// "���x"
	VC_VAL_KODO,	// ���ݍ��x[m]
	MY_WAIT2(100),
	VC_VAL_LIFT,	// ���݃��t�g[m/s]
	MY_WAIT2(100),
	VC_UNIT_HOUI,	// "����"
	VC_VAL_HOUI,	// ���݂̕���[16����]
//	VC_UNIT_HOUI,	// "����"
//	VC_VAL_HOUI2,	// ���݂̕���[360��]
	MY_WAIT2(100),
	VC_UNIT_SOKUDO,	// "���x"
	VC_VAL_SOKUDO,	// ���ݑ��x[km/h]
	MY_WAIT2(1000)
};
const u16 VSEQ_NORMAL_I[] = {
	MY_WAIT2(1000),
	VC_VAL_PYLON,	// �p�C�����ԍ�
	MY_WAIT(200),
	VC_VAL_PY_DIR,	// �p�C��������
	MY_WAIT2(400),
	VC_UNIT_PY_DIS,	// �p�C����"����"
	VC_VAL_PY_DIS,	// �p�C��������
	MY_WAIT2(3000),
	VC_UNIT_KODO,	// "���x"
	VC_VAL_KODO,	// ���ݍ��x[m]
	MY_WAIT2(100),
	VC_VAL_LIFT,	// ���݃��t�g[m/s]
	MY_WAIT2(100),
	VC_UNIT_HOUI,	// "����"
	VC_VAL_HOUI,	// ���݂̕���[16����]
//	VC_UNIT_HOUI,	// "����"
//	VC_VAL_HOUI2,	// ���݂̕���[360��]
	MY_WAIT2(100),
	VC_UNIT_SOKUDO,	// "���x"
	VC_VAL_SOKUDO,	// ���ݑ��x[km/h]
	VC_INTERVAL
};

const u16 VSEQ_NORMAL2[] = {
	MY_WAIT2(1000),
	VC_VAL_PYLON,	// �p�C�����ԍ�
	MY_WAIT(200),
//	VC_UNIT_HOUI,	// "����"
//	MY_WAIT2(100),
//	VC_VAL_PY_DIR2,	// �p�C��������
	VC_VAL_PY_DIR,	// �p�C��������
	MY_WAIT2(300),
	VC_UNIT_PY_DIS,	// �p�C����"����"
	VC_VAL_PY_DIS2,	// �p�C�������� �P�ʖ���
	MY_WAIT2(2000),
	VC_UNIT_KODO,	// "���x"
	VC_VAL_KODO2,	// ���ݍ��x[m]
	MY_WAIT2(500),
	VC_VAL_LIFT,	// ���݃��t�g[m/s]
	MY_WAIT2(2000)
};
const u16 VSEQ_NORMAL2_I[] = {
	MY_WAIT2(1000),
	VC_VAL_PYLON,	// �p�C�����ԍ�
	MY_WAIT(200),
//	VC_UNIT_HOUI,	// "����"
//	MY_WAIT2(100),
//	VC_VAL_PY_DIR2,	// �p�C��������
	VC_VAL_PY_DIR,	// �p�C��������
	MY_WAIT2(300),
	VC_UNIT_PY_DIS,	// �p�C����"����"
	VC_VAL_PY_DIS2,	// �p�C�������� �P�ʖ���
	MY_WAIT2(2000),
	VC_UNIT_KODO,	// "���x"
	VC_VAL_KODO2,	// ���ݍ��x[m]
	MY_WAIT2(500),
	VC_VAL_LIFT,	// ���݃��t�g[m/s]
	VC_INTERVAL
};

typedef struct {
	const u16* vseq;
	u32 size;
} VSeqInfo;
#define VSEQ_DEF(a) {a, ARRAY_SIZE(a)}
const VSeqInfo VSEQ_TABLE[] = {
	VSEQ_DEF(VSEQ_NONE),
	VSEQ_DEF(VSEQ_ALT),
	VSEQ_DEF(VSEQ_ALT_LIFT),
	VSEQ_DEF(VSEQ_PYLON),
	VSEQ_DEF(VSEQ_NORMAL),
	VSEQ_DEF(VSEQ_NORMAL2),
	VSEQ_DEF(VSEQ_ALT_I),
	VSEQ_DEF(VSEQ_ALT_LIFT_I),
	VSEQ_DEF(VSEQ_PYLON_I),
	VSEQ_DEF(VSEQ_NORMAL_I),
	VSEQ_DEF(VSEQ_NORMAL2_I),
};


///////////////////////////////////////////////////////////////////////////////
// �����o�b�t�@�ւ̉����ǉ�
///////////////////////////////////////////////////////////////////////////////
void AddVoice(s16 val){
	if(IW->mp.vbuf_idx < VBUF_SIZE){
		IW->voice_buf[IW->mp.vbuf_idx++] = val | 0x8000;
	}
}
void AddWait(s16 val){
	if(val && IW->mp.vbuf_idx < VBUF_SIZE){
		IW->voice_buf[IW->mp.vbuf_idx++] = val;
	}
}

const u32 VOICE_KETA[4] = {
	1 - 1,
	VOICE_10 - 1,
	VOICE_100 - 1,
	VOICE_1000 - 1,
};

void AddVoiceNum(s32 val){
	if(val == 0x80000000) return;
	s32 f = 0, keta = 0;
	if(val < 0){
		f = 1;
		val = -val;
	}
	if(!val) AddVoice(0);
	for(keta = 0 ; val ; ++keta){
		s32 m;
		val = BiosDiv(val, 10, &m);
		if(m){
			if(keta & 3) AddWait(5);
			AddVoice(VOICE_KETA[keta & 3] + m);
		}
		if(val && keta == 3){
//			AddWait(5);
			AddVoice(VOICE_10000);
		} else if(keta){
//			AddWait(3);
		}
	}
}

// �{���́��̂悤�ɂ��ׂ������ǁA�Ƃ肠�����A22.5��(0x1000)�ŕ���
//345���`15��: N     (30��)       
// 15���`37��: NNE   (22��)
// 38���`52��: NE    (14��) 
// 53���`75��: ENE   (22��)
// 75���`90��: E     (30��)
const u8 HOUI_TABLE[16][4] = { // �t���ɓǂ܂��
	{VOICE_KITA},
	{VOICE_TO, VOICE_HOKU, VOICE_HOKU},
	{VOICE_TO, VOICE_HOKU},
	{VOICE_TO, VOICE_HOKU, VOICE_TO},
	{VOICE_HIGASHI},
	{VOICE_TO, VOICE_NAN, VOICE_TO},
	{VOICE_TO, VOICE_NAN},
	{VOICE_TO, VOICE_NAN, VOICE_NAN},
	{VOICE_MINAMI},
	{VOICE_SEI, VOICE_NAN, VOICE_NAN},
	{VOICE_SEI, VOICE_NAN},
	{VOICE_SEI, VOICE_NAN, VOICE_SEI},
	{VOICE_NISHI},
	{VOICE_SEI, VOICE_HOKU, VOICE_SEI},
	{VOICE_SEI, VOICE_HOKU},
	{VOICE_SEI, VOICE_HOKU, VOICE_HOKU},
};

void AddVoiceHoui(u32 ang){
	const s8* p = HOUI_TABLE[((ang + 0x500) & 0xffff) >> 12];
	while(*p) AddVoice(*p++);
}


///////////////////////////////////////////////////////////////////////////////
// �����Đ��J�n
///////////////////////////////////////////////////////////////////////////////
void VoiceStart(u32 index){
	if(!IW->cw.phy_voice) return; // �����f�[�^����
	VoiceData* vd = (VoiceData*)(IW->cw.phy_voice + ((GbaWaveHeader*)IW->cw.phy_voice)->index[index]);

	s32 size = vd->size;
	if(size > VOICE_FIFO_MAX) size = VOICE_FIFO_MAX;
	DmaCopy(3, vd->data, VOICE_ADDRESS, size * 16, 32);// RAM�ɃR�s�[���čĐ�(�g���b�N���O�ۑ��Ή�)
	IW->voice_ctr = size;
	

	// ���̉������~�߂�
	REG16(REG_TM0CNT)   = 0;
	REG16(REG_DM1CNT_H) = 0;

	// �����J�n
	REG16(REG_SGCNT0_H) = 0x0b06; // �ő剹��(�ł��������c)

	REG32(REG_DM1DAD) = 0x040000a0;
	REG32(REG_DM1SAD) = VOICE_ADDRESS; // RAM�Đ�
	REG16(REG_DM1CNT_H) = 0xf640;
	
	REG16(REG_TM0D)   = vd->freq;
	REG16(REG_TM0CNT) = 0x80;

	REG16(REG_IE) |= DMA1_INTR_FLAG;
}

// �����������I�Ɏ~�߂�
void VoiceStop(){
	IW->voice_ctr = 0;
	IW->mp.voice_vbc = 0;
	IW->mp.vbuf_idx = 0;

	REG16(REG_TM0CNT)    = 0;
	REG16(REG_DM1CNT_H)  = 0;
	REG16(REG_IE)		&= ~DMA1_INTR_FLAG;
	REG16(REG_SGCNT0_H)  = 0x0006;
}

// �����^�C�v�̑I��
s32 ChangeVoiceType(){
	int type;
	if(IW->px.gstate == GSTATE_STOP)			type = IW->tc.vm_wait;	// �e�C�N�I�t�O
	else if(IW->px.gstate & GSTATE_CENTERING)	type = IW->tc.vm_center;// �Z���^�����O��
	else if(IW->px.vh_mm <= IW->tc.stop_dir)	type = IW->tc.vm_stop;	// ��~��
	else if(IW->mp.cyl_dist && IW->mp.cyl_dist < IW->tc.cyl_near)	type = IW->tc.vm_near;	// ��~��
	else										type = IW->tc.vm_normal;// �ʏ�

	if(IW->mp.voice_type != type){
		IW->mp.voice_type = type;
		IW->mp.voice_state = 0;
		VoiceStop(); // ���݂̉������~�߂�
		return 1;
	}
	return 0; // �ύX����
}

// �����Đ�������
u32 VoiceLeft(){
	if(IW->voice_ctr) return 1; // ���ݍĐ���

	// ���p����?
	if(IW->mp.voice_vbc){
		if(IW->mp.voice_chg && ChangeVoiceType()) return -1; // �����^�C�v�؂�ւ�����
		if(IW->vb_counter - IW->mp.voice_vbc < IW->voice_buf[IW->mp.vbuf_idx]) return 2;//���p����
		IW->mp.voice_vbc = 0;
	}
	// �����o�b�t�@�Ɏc�肠��?
	if(IW->mp.vbuf_idx){
		u32 t = IW->voice_buf[--IW->mp.vbuf_idx];
		if(t & 0x8000){
			// �����f�[�^
			VoiceStart(t & 0xfff);
		} else {
			// ���p���f�[�^
			IW->mp.voice_vbc = IW->vb_counter;
			if(!IW->mp.voice_vbc) IW->mp.voice_vbc--; // 0�͓��ʂȂ̂�1���炷(�e����16ms�̂�)
		}
		return 3;
	}

	return 0; // �������Đ����Ă��Ȃ�
}

///////////////////////////////////////////////////////////////////////////////
// �����^�X�N
///////////////////////////////////////////////////////////////////////////////

s32 MT_Voice(){
	if(!IW->tc.vm_enable) return 0; //�����̖���

	s32 t, t2;
	if(IW->px.fix < FIX_2D) return 0; // ��M�ł��Ă��Ȃ�

	if(IW->mp.voice_type >= ARRAY_SIZE(VSEQ_TABLE)) return 0; // ����`����!?
	VSeqInfo vsi = VSEQ_TABLE[IW->mp.voice_type];

	if(!vsi.size) return 0; // ����

	if(VoiceLeft()) return 0; // �����Đ����͐V����������ǉ����Ȃ�

	// �����o�b�t�@����ɂȂ����̂ŁA�V����������ǉ�����
	if(IW->mp.voice_state >= vsi.size) IW->mp.voice_state = 0;

	t2 = vsi.vseq[IW->mp.voice_state++];
	IW->mp.voice_chg = 0;
	switch(t2){
	case VC_WAIT2:
		if(ChangeVoiceType()) break;  // �����^�C�v�̐؂�ւ���WAIT2�C�x���g���̂�
		IW->mp.voice_chg = 1;
		// no break;
	case VC_WAIT: // �E�F�C�g(���̃e�[�u���l���g��)
		AddWait(vsi.vseq[IW->mp.voice_state++]);
		break;
	case VC_INTERVAL: // ���[�U�ݒ�̃E�F�C�g
		if(ChangeVoiceType()) break;  // �����^�C�v�̐؂�ւ���WAIT2�C�x���g���̂�
		IW->mp.voice_chg = 1;
		AddWait(IW->tc.vm_interval * 60);
		break;
	case VC_UNIT_KODO:	// "���x"
		AddWait(20);
		AddVoice(VOICE_KODO);
		break;
	case VC_VAL_KODO:	// ���ݍ��x[m]
		AddWait(5);
		AddVoice(VOICE_M);
		// no break;
	case VC_VAL_KODO2:	// ���ݍ��x[m]
		AddVoiceNum(RoundDiv(IW->px.alt_mm, 1000));
		break;
	case VC_VAL_LIFT:// ���݃��t�g[m/s] (���t�g/�V���N0�̂Ƃ��̓X�L�b�v)
	case VC_VAL_LIFT2:// ���݃��t�g[m/s] (����)
		t = RoundDiv(IW->px.up_mm, 100);
		if(t2 == VC_VAL_LIFT && !t){
			AddWait(100);
			break; // ���t�g���V���N�������Ƃ��ɂ̓X�L�b�v
		}
		if(t < 0) t = -t;
		t = BiosDiv(t, 10, &t2);

		AddWait(50);
		AddVoiceNum(t2);
		if(t){
			AddVoice(VOICE_TEN);
			BiosDiv(t, 10, &t2);
			AddWait(DOT_WAIT[t2]);
			AddVoiceNum(t);
		} else if(t2){
			AddVoice(VOICE_COMMA);
		}
		AddWait(20);
		if(IW->px.up_mm < 0){
			AddVoice(VOICE_SINK);
		} else {
			AddVoice(VOICE_LIFT);
		}
		break;
	case VC_UNIT_HOUI:	// "����"
		AddWait(20);
		AddVoice(VOICE_HOUI);
		break;
	case VC_VAL_HOUI:	// ���݂̕���[16����]
		AddWait(50);
		AddVoiceHoui(IW->px.h_ang64);
		break;
	case VC_VAL_HOUI2:	// ���݂̕���[360��]
		AddWait(50);
		t = ((IW->px.h_ang_c & 0xffff) * 360) >> 16;
		t = BiosDiv(t, 10, &t2);
		AddVoiceNum(t2);
		t = BiosDiv(t, 10, &t2);
		AddVoiceNum(t2);
		AddVoiceNum(t);
		break;
	case VC_UNIT_SOKUDO:	// "���x"
		AddWait(20);
		AddVoice((IW->tc.spd_unit)? VOICE_JISOKU : VOICE_BYOSOKU);
		break;
	case VC_VAL_SOKUDO:	// ���ݑ��x[km/h]
		AddWait(50);
		if(IW->tc.spd_unit){
			AddVoice(VOICE_KM);
			AddVoiceNum(RoundDiv(IW->px.vh_mm * 36, 10000));
		} else {
			t = RoundDiv(IW->px.vh_mm, 100);
			t = BiosDiv(t, 10, &t2);
			AddVoice(VOICE_M);
/*�����_�͏ȗ�
			if(t2){
				AddVoiceNum(t2);
				AddVoice(VOICE_TEN);
				if(t == 2) AddWait(10);
			}
*/
			AddVoiceNum(t);
		}
		break;
	case VC_VAL_PYLON:	// �p�C�����ԍ�
		if(IW->mp.py_dis){
			if(IW->tc.task_mode){
				AddWait(20);
				AddVoiceNum(IW->mp.cur_point + 1);
				AddWait(10);
			}
			AddVoice(VOICE_PYLON);
		}
		break;
	case VC_VAL_PY_DIR:// �p�C��������
		if(IW->mp.py_dis){
			if(IW->mp.py_dir){
//				AddVoice(VOICE_DO);
				AddVoiceNum(myAbs(IW->mp.py_dir));
				AddWait(20);
				AddVoice((IW->mp.py_dir < 0)? VOICE_HIDARI : VOICE_MIGI);
			} else {
				// ���i�͒���Ȃ�
			}
		}
		break;
/*
	case VC_VAL_PY_DIR2:// �p�C��������
		if(IW->mp.py_dis){
			if(IW->mp.py_dir){ // -180�`179
				t = IW->mp.py_dir;
				if(t < 0) t += 360;
				t = BiosDiv(t, 10, &t2);
				AddVoiceNum(t2);
				t = BiosDiv(t, 10, &t2);
				AddVoiceNum(t2);
				AddVoiceNum(t);
			} else {
				// ���i�͒���Ȃ�
			}
		}
		break;
*/
	case VC_UNIT_PY_DIS: // �p�C����"����"
		if(IW->mp.py_dis){
			AddWait(10);
			AddVoice(VOICE_KYORI);
		}
		break;
	case VC_VAL_PY_DIS:// �p�C��������
		if(IW->mp.py_dis){
			if(IW->mp.py_dis < 10000){
				AddWait(5);
				AddVoice(VOICE_M);
				AddVoiceNum(IW->mp.py_dis);
			} else {
				AddWait(5);
				AddVoice(VOICE_KM);
				AddVoiceNum(RoundDiv(IW->mp.py_dis, 1000));
			}
		}
		break;
	case VC_VAL_PY_DIS2:// �p�C��������
		if(IW->mp.py_dis){
			t = IW->mp.py_dis;
			if(t > 10000000) t = 10000000;
			AddVoiceNum(t);
		}
		break;
	}
	return 0;
}

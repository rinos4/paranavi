///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// ������������^�X�N�̃R���g���[�������̃t�@�C���ɂ܂Ƃ߂��B
// �^�X�N����͓Ǝ�OS�̌Ăяo������߁A�^�X�N�e�[�u���Ŏ��ȊǗ�����悤�ɂ����B


///////////////////////////////////////////////////////////////////////////////
// �萔
///////////////////////////////////////////////////////////////////////////////

#define KEY_RESUME		(KEY_L | KEY_R)
#define KEY_LOAD_GUARD	(KEY_L | KEY_R)

MyIWRAM*		const IW    = (MyIWRAM*)CPU_WRAM;
RouteList*		const ROUTE = (RouteList*)ROUTE_ADDRESS;
WptList*		const WPT   = (WptList*)WPT_ADDRESS;
Locus*			const LOCUS = (Locus*)LOCUS_ADDRESS;
SpliteTable*	const SPLITE= (SpliteTable*)SPLITE_ADDRESS;

// �R���t�B�O�̏����l��`
const TaskConfig INIT_TASK_CONFIG = {
	// �w�b�_���
	FLBL_TASK_MAGIC,		//u32 magic;
	TDataSize(TaskConfig),	//u32 size;
	0,						//u32 w_count;
	0,						//u32 rfu;
	
	// GPS�ݒ�
	9 * 60,	// tzone_m		JST(+9:00)
	-7,		// n_calib		(W007��)
	300,	// stop_dir     (0.3m/s)
	0,		// alt_type		(���ϊC�ʍ��x)
	0,		// calc_type	(�I�[�g)
	2,		// tally_clr;	Takeoff Reset
	1,		// vario_mode	�o���I���[�h �X���[�Y
	1,		// vario_to		�e�C�N�I�t�O�̓o���I��炳�Ȃ�
	+0,		// vario_up		+0.0�ȏ�
	-2000,	// vario_down	-2.0�ȉ�
	0,		// waas_flag	WAAS���[�h
	1,		// u32 gps_warn GPS�x������
	1,		// u32 nmea_up	�f�t�H���g�̓n�[�t
	{0, 0, 0, 0, 0, 0}, // RFU

	// �^�X�N�ݒ�
	0,		// task_mode	�t���[
	0,		// route		0��
	1000,	// sector		1000m
	200,	// cylinder		200m
	0,		// rfu1			0
	0,		// goal_type	0
	0,		// start_type	�t���[
	0,		// start_time	�g��Ȃ�
	0,		// close_time	�g��Ȃ�
	0,		// pre_pylon    0��
	0,		// cyl_end
	0,		// skip_TO;	// �ŏ���TO���X�L�b�v
	{0}, // RFU

	0,		// at_mode;	// �I�[�g�^�[�Q�b�g���[�h
	500,	// at_min;		// �ŏ�����
	5000,	// at_max;		// �ő勗��
	6000,	// at_recheck;	// �Č�������

	// �i�r�ݒ�
	0,		// view_mode;	�W����
	50,		// near_beep	�����r�[�v(�V�����_�T�C�Y+��)
	0,		// auto_lock;	�}�j���A��
	0,		// avg_type;	���ǂ�
	 200,	// ar_max;		// �p�C�����A���[MAX[m]
	-200,	// ar_min;		// �p�C�����A���[MIN[m]
	0,		// self_r;		// ���@�\�����a(�Z�N�^�Ɠ���)
	1,		// spd_unit;	// �Βn���x�P�� km/h
	7000,	// my_ldK;		7.0
	1000,	// my_down;		1.0m/s
	7000,	// my_speed;	// �@�̂̑��x (�����p)
	-4000,	// spiral_spd;	// �X�p�C�������x
	-8000,	// stall_spd;	// �X�g�[�����x
	3000,	// pitch_diff;	// �s�b�`���O���x��
	5,		// pitch_freq;	// �s�b�`���O�ő����
	120,	// roll_diff;	// ���[���p�x��(120��)
	5,		// roll_freq;	// ���[���ő����
	1000,	// start_spd;	// �X�^�[�g�X�s�[�h

	0,		// pylon_type;	// �p�C�����\���^�C�v
	1,		// initial_pos; // �C�j�V�����ʒu
	7,		// vol_key;		// �L�[�{�����[��
	1,		// vol_vario;	// �o���I�{�����[��
	1,		// vm_enable;	// �����̗L��/����
	0,		// vm_wait;		// �����Ȃ�
	9,		// vm_normal;	// ���i����
	1,		// vm_stop;		// ��~�������Ȃ�
	2,		// vm_center;	// �Z���^�����O��
	4,		// view_alt		// Next L/D
	1,		// anemo_unit;	// ��C���x�P�� km/h
	4,		// vm_near;		// �j�A�V�����_�̉���
	10,		// vm_interval;	// �����̊Ԋu
	0,		// start_alarm;	// �X�^�[�g�A���[��
	0,		// sp_prio;		// SP���[�h�D��x
	{0, 0, 0}, // RFU

	10,		// auto_off		�����d��OFF 10����
	0,		// time_alarm	����Ȃ�
	0,//1,	// wind_cmd;	���[�����Ox3
	0,//2,	// glider_cmd;	�s�b�`���Ox3
	1,		// thermal_cmd; �Z���^�����O
	1,		// wind_update; ������������On
	1,		// bench_update;������������On
	20,		// stable_angle;// �p�x�ω��}�[�W�� 20��
	1000,	// stable_speed;// ���x�ω��}�[�W�� 1 m/s
	5,		// init_wait;	// 5�b
	30,		// wait_timeout;// �^�C���A�E�g�҂�
	5,		// comp_timeout;// �R���v���[�g��̕\������
	2,		// palette_conf;// �p���b�g�ݒ�
	100,	// keep_range:	//���t�g�ł��V���N�ł��Ȃ��㏸��

	0,		// anemo_coef:	// �����W���@(�W���l�Ƃ���16384000 * 2�Ƃ��ł��ǂ���)
	1,		// cyl_cmd;		// �V�����_�}�b�v�R�}���h �j�A
	500,	// cyl_near;	// �j�A�V�����_
	{0, 0, 0, 0, 0},// RFU

	2,		// locus_smp;	2�b�T���v�����O(4��)
	300,	// locus_r;		300m
	1800,	// locus_cnt;	// �O�Ր�(MAX 1800)
	+3000,	// locus_up;	// �ヌ���W
	-3000,	// locus_down;	// �������W
	2000,	// locus_range; // 2m/s
	1,		// locus_pal;	// �p���b�g���[�h

	// Bluetooth
	0,		// bt_mode;					Bluetooth�Ȃ�
	{0},	// bt_addr[BT_ADDR_LEN];	�ڑ���A�h���X�͋�
	{"0000"},// bt_pin [BT_PIN_LEN];	���"0000"��"1234"�Ȃ̂ŁA0000���f�t�H���g�ɐݒ�

	0,		// log_enable;	// �g���b�N���O�L��/����
	{0, 0},	// rsv
	2,		// log_prec;	// �ۑ����x
	1,		// log_intvl;	// �ۑ��Ԋu
	1,		// log_overwrite// �㏑�����[�h
	0,		// log_debug;	// �f�o�b�O�I�t

	0,		// flog_as;		//�t���C�g���O�̃I�[�g�Z�[�u�ݒ�
	0,		// tlog_as;		//�g���b�N���O�̃I�[�g�Z�[�u�ݒ�
	{0, 0, 0, 0, 0, 0}, // RFU
};

///////////////////////////////////////////////////////////////////////////////
// BIOS�R�[�����͂����Œ�`
///////////////////////////////////////////////////////////////////////////////

#ifdef __thumb__
//#pragma optimize(0, "off")
// ARM���[�h�ɂ���̂��ʓ|�c
u32 InterlockedExchange(vu32* p, u32 val){
	s32 r;
	INTR_OFF(); // REG_IME�N���A�̂�
	r = *p;
	*p = val;
	INTR_ON(); // REG_IME�Z�b�g�̂�
	return r;
}
//#pragma optimize(0, "on")

#else
// ARM�̂�
u32 InterlockedExchange(vu32*p, u32 val){
  u32 ret;
  __asm (
	"swp %0, %2, [%1]\n"
	: "=&r"(ret)
	: "r"(p), "r"(val)
	: "memory");
  return ret;
}
u8 InterlockedExchange8(vu8*p, u8 val){
  u8 ret;
  __asm (
	"swpb %0, %2, [%1]\n"
	: "=&r"(ret)
	: "r"(p), "r"(val)
	: "memory");
  return ret;
}
#endif

//�Z�p�֘A
s32 BiosDiv(s32 num, s32 denom, s32* mod){
//	*mod  = num % denom;
//	return  num / denom;
  __asm (
	"\n"
#ifdef __thumb__
	"push	{r1-r3}\n"
	"swi	0x6\n"
	"str	r1, [%2]\n"
	"pop	{r1-r3}\n"
#else
	"stmfd	sp!, {r1-r3}\n"
	"swi	0x60000\n"
	"str	r1, [%2]\n"
	"ldmea	sp!, {r1-r3}\n"
#endif

	: "=&r"(num)	// %0

	: "r"(denom),	// %1
	  "r"(mod)		// %2

	: "memory");
  return num;
}

u32 BiosSqrt(u32 n){
  __asm (
	"\n"
#ifdef __thumb__
	"push	{r1,r3}\n"
	"swi	0x8\n"
	"pop	{r1,r3}\n"
#else
	"stmfd	sp!, {r1, r3}\n"
	"swi	0x80000\n"
	"ldmea	sp!, {r1, r3}\n"
#endif

	: "=&r"(n)	// %0
	:
	: "memory");
  return n;
}
s32 BiosArcTan2(s32 x, s32 y){
  __asm (
	"\n"
#ifdef __thumb__
	"push	{r1,r3}\n"
	"swi	0xA\n"
	"pop	{r1,r3}\n"
#else
	"stmfd	sp!, {r1, r3}\n"
	"swi	0xA0000\n"
	"ldmea	sp!, {r1, r3}\n"
#endif

	: "=&r"(x)	// %0
	:
	: "memory");
  return x;
}

// ������
void BiosRegisterRamReset(u32 n){
  __asm (
	"\n"
#ifdef __thumb__
	"push	{r1,r3}\n"
	"swi	0x1\n"
	"pop	{r1,r3}\n"
#else
	"stmfd	sp!, {r1, r3}\n"
	"swi	0x10000\n"
	"ldmea	sp!, {r1, r3}\n"
#endif

	: "=&r"(n)	// %0
	:
	: "memory");
}

// ���܂��֐�
u32 BiosHypot(s32 a, s32 b){
	if(a == 0x80000000 || b == 0x80000000) return -1; // �ő�l��Ԃ��Ă����c
 	if(a < 0) a = -a;
	if(b < 0) b = -b;
	s32 i = 0;
	for(i = 0 ; a + b >= 0x10000 ; ++i, a >>= 1, b >>= 1);
	return BiosSqrt(a * a + b * b) << i;
}
// sqrt(a^2 + b^2 + c^2)
u32 BiosTripot(s32 a, s32 b, s32 c){
	if(a == 0x80000000 || b == 0x80000000 || c == 0x80000000) return -1; // �ő�l��Ԃ��Ă����c
	if(a < 0) a = -a;
	if(b < 0) b = -b;
	if(c < 0) c = -c;
	s32 i = 0;
	for(i = 0 ; a + b + c >= 0x10000 ; ++i, a >>= 1, b >>= 1, c >>= 1);
	return BiosSqrt(a * a + b * b + c * c) << i;
}

// math�Ɠ��������� 64K�ŕԂ�
s32 BiosAtan64K(s32 yK, s32 xK){
	// 16bit�֐��K������BiosArcTan2���Ăяo��(���@�Ŏ�������A�Ȃ������K�����K�v�Ȃ��������c)
	for(; xK < -0x7fff || 0x7fff < xK || yK < -0x7fff || 0x7fff < yK ; xK >>= 1, yK >>= 1);
	return BiosArcTan2(xK, yK) & 0xffff;
}

s32 CountBit(s32 a){
	if(a == 0x80000000) return 32;
	if(a < 0) a = -a;
	s32 c = 0;
	for(; a ; a >>= 1) ++c;
	return c;
}

s32 BiosMBoot(MBootParam* p){
  __asm (
	"\n"
#ifdef __thumb__
	"push	{r1,r3, r7}\n"
	"mov	r7, r0\n"
	"mov	r1, #1\n"
	"swi	0x25\n"
	"pop	{r1,r3,r7}\n"
#else
	"stmfd	sp!, {r1, r3, r7}\n"
	"mov	r7, r0\n"
	"mov	r1, #0\n"
	"swi	0x250000\n"
	"ldmea	sp!, {r1, r3, r7}\n"
#endif

	: "=&r"(p)	// %0
	:
	: "memory");
	return (s32)p;
}

u32 BiosLZ77UnCompWram(const void* src, const void* dst){
	u32 num;
  __asm (
	"\n"
#ifdef __thumb__
	"push	{r1-r3}\n"
	"ldr	r2, [r0]\n"
	"swi	0x11\n"
	"lsr	%0, r2, #8\n"
	"pop	{r1-r3}\n"
#else
	"stmfd	sp!, {r1-r3}\n"
	"ldr	r2, [r0]\n"
	"swi	0x110000\n"
	"lsr	%0, r2, #8\n"
	"ldmea	sp!, {r1-r3}\n"
#endif
	: "=&r"(num)	// %0
	:
	: "memory");
	return num;
}
u32 BiosLZ77UnCompVram(const void* src, const void* dst){
	u32 num;
  __asm (
	"\n"
#ifdef __thumb__
	"push	{r1-r3}\n"
	"ldr	r2, [r0]\n"
	"swi	0x12\n"
	"lsr	%0, r2, #8\n"
	"pop	{r1-r3}\n"
#else
	"stmfd	sp!, {r1-r3}\n"
	"ldr	r2, [r0]\n"
	"swi	0x120000\n"
	"lsr	%0, r2, #8\n"
	"ldmea	sp!, {r1-r3}\n"
#endif
	: "=&r"(num)	// %0
	:
	: "memory");
	return num;
}


u32 UpdateVBC(u32* pre){
	u32 cur = IW->vb_counter;
	u32 ret = cur - *pre;
	*pre = cur;
	return ret;
}

s32 RoundDiv(s32 num, s32 denom){
	return BiosDiv(num + ((num > 0)? (denom/2) : (-denom/2)), denom, &num);
}

// �����͐��`�����@���g�p�B�����Ƃ��Ă̓C�}�C�`�����A�I�[�g�^�[�Q�b�g�p�Ȃ炱��ŏ\���B
void MySrand(u32 v){
	IW->mp.rand_val = v;
}
u32 MyRand(){
	IW->mp.rand_val = IW->mp.rand_val * 1103515245 + 12345;
	return IW->mp.rand_val >> 8; // LSB�̎�����256
}

///////////////////////////////////////////////////////////////////////////////
// ���[�g/�E�F�C�|�C���g����n�֐�
///////////////////////////////////////////////////////////////////////////////
void UpdateWptInfo(TaskLogData* tld, Route* rt, u32 n){
	if(!n) return;
	TaskLog1* tl1 = &tld->tl1[n];
	tl1->cyl = GetCurCyl(rt, n - 1);
	TaskLog2* tl2 = GetTL2Addr(tld, n);
	if(tl2){
		Wpt* w = GetWptInfo(rt, n - 1);
		tl2->wpt_alt = w->alt;
		memcpy(tl2->wpt_name, w->name, sizeof(tl2->wpt_name));
	}
}

s8 GetPrePylon(){
//	return IW->tc.pre_pylon;
	if(IW->tc.skip_TO) return IW->tc.pre_pylon + 0x81;
	return IW->tc.pre_pylon;
}

// �^�X�N���O
void InitTaskLog(u32 keepTO){ // ���O������
	TaskLogData* tld = &IW->task_log;

	TaskLog1 takeoff;
	if(keepTO) takeoff = tld->tl1[0]; // �e�C�N�I�t�����o�b�N�A�b�v
	DmaClear(3, 0, tld, sizeof(TaskLogData), 32);

	// �R���t�B�O���Z�b�g
	tld->magic = FLOG_MAGIC_TASK;
	tld->log_time = IW->px.dtime;
	tld->pre_pylonX	= GetPrePylon();
	tld->start_time = (u16)(IW->tc.start_type? IW->tc.start_time : 0);
	tld->update_mark= (keepTO && takeoff.dtime)? 1: 0;

	// ���[�g���Z�b�g
	Route* rt = GetTaskRoute();
	IW->mp.pre_route = rt;
	if(rt){
		tld->rt_dist	= rt->dist;
		tld->rt_count	= rt->count;
		tld->pre_pylonX	= GetPrePylon();
		memcpy(tld->rt_name, rt->name, sizeof(tld->rt_name));
		// �E�F�C�|�C���g���Z�b�g
		u32 i;
		for(i = 1 ; i <= rt->count ; ++i) UpdateWptInfo(tld, rt, i);
	} else {
		strcpy(tld->rt_name, "## NONE ##");
	}

	// �g���b�v���[�^�̏�����
	IW->mp.tally.trip_mm = 0;

	// �e�C�N�I�t���̐ݒ�
	if(keepTO) tld->tl1[0] = takeoff; // �e�C�N�I�t����߂�
	else       IW->mp.tlog_presave = -1; // ���S������
}

void SetTlog1(TaskLog1* tl1){
	tl1->dtime= IW->px.dtime;
	tl1->trip = IW->mp.tally.trip_mm / 1000; // u64!
	tl1->lat  = IW->px.lat;
	tl1->lon  = IW->px.lon;
	tl1->alt  = RoundDiv(IW->px.alt_mm, 1000);
	if(!tl1->lat) tl1->lat = 1; // 0����ꏈ�����Ă����B������A�o�x0���Ŏg���Ă�3mm�ʌ덷�̂����H
}

void UpdateTaskLog(s32 n){ // ���O�f�[�^�ǉ�
	Route* rt = GetTaskRoute();
	if(IW->mp.pre_route != rt) InitTaskLog(1);

	// ���[�g�ύX�����ɒl���ς��ϐ����A�b�v�f�[�g
	TaskLogData* tld = &IW->task_log;
	tld->pre_pylonX	 = GetPrePylon();
	tld->start_time  = (u16)(IW->tc.start_type? IW->tc.start_time : 0);
	tld->update_mark |= n? 2 : 1;

	// ���O�G���A�擾
	if(n > tld->rt_count) return; // !?
	SetTlog1(&tld->tl1[n]);

	// ���O�i�[
	if(n) UpdateWptInfo(tld, rt, n); // �O�̂��ߍX�V

	IW->mp.tally.trip_mm = 0;
}

#define AUTO_TARGET_ID (-2)
#define MAX_AT_LOG 60 // ����ȏ�͖��O��������
void UpdateAutoTargetLog(){
	// �ꉞ�^�[�Q�b�g�����邩�`�F�b�N
	Wpt* wpt = GetNearTarget();
	if(!wpt) return; // !?

	TaskLogData* tld = &IW->task_log;
	if((u32)IW->mp.pre_route != AUTO_TARGET_ID){
		 IW->mp.pre_route = (Route*)AUTO_TARGET_ID;
		 // �I�[�g�^�[�Q�b�g�p������

		TaskLog1 takeoff = tld->tl1[0]; // �e�C�N�I�t�����o�b�N�A�b�v
		DmaClear(3, 0, tld, sizeof(TaskLogData), 32);

		// �R���t�B�O���Z�b�g
		tld->magic = FLOG_MAGIC_TASK;
		tld->log_time = IW->px.dtime;
		tld->update_mark= (takeoff.dtime)? 1: 0;
		tld->pre_pylonX	= 0; // �e�C�N�I�t���X�^�[�g�p�C�����ɂ���
		// ���[�g���
		strcpy(tld->rt_name, "[Auto Target]");

		// �e�C�N�I�t���̐ݒ�
		tld->tl1[0] = takeoff; // �e�C�N�I�t����߂�
		IW->mp.tlog_presave = -1; // �ۑ��ꏊ��������
	}

	if(tld->rt_count >= MAX_AT_LOG) return; // ����ȏネ�O�ł��Ȃ�
	TaskLog1* tl0 = &tld->tl1[   tld->rt_count ];
	TaskLog1* tl1 = &tld->tl1[++(tld->rt_count)];
	SetTlog1(tl1);
	tl1->cyl = IW->tc.cylinder;
	TaskLog2* tl2 = GetTL2Addr(tld, tld->rt_count);
	if(tl2){
		tl2->wpt_alt = wpt->alt;
		memcpy(tl2->wpt_name, wpt->name, sizeof(tl2->wpt_name));
	}

	u32 len;
	CalcDist(tl0->lat, tl0->lon, tl1->lat, tl1->lon, &len, 0);
	tld->rt_dist += len;

	if(IW->tc.tlog_as){
		SaveFLog(IW->mp.tlog_presave, &IW->task_log, sizeof(TaskLogData), 0); // ���S�[�����͕\����ɕۑ�
	}
	IW->mp.tally.trip_mm = 0;
}

// 
TaskLog2* GetTL2Addr(TaskLogData* tld, u32 n){
	if(!n || (u32)&tld->tl1[tld->rt_count + 1] > (u32)&tld->tl2[TL2MAX - n]) return 0; // �������߂Ȃ�
	return &tld->tl2[TL2MAX - n]; // �������݃o�b�t�@����
}

Route* GetTaskRoute(){
	return (IW->tc.route < ROUTE->route_count)? &ROUTE->route[IW->tc.route] : 0;
}
Wpt* GetWptInfo(const Route* rt, s32 offset){
	if(!rt || offset < 0 || rt->count <= offset) return 0;
	return &WPT->wpt[rt->py[offset].wpt];
}
Wpt* GetCurWpt(const Route* rt, s32 offset){
	return GetWptInfo(rt, offset + IW->mp.cur_point);
}
u16 GetCurCyl(const Route* rt, u32 offset){ // offset = IW->mp.cur_point
	u16 ret = 0xffff;
	if(rt && offset < rt->count) ret = rt->py[offset].cyl;
	return (ret == 0xffff)? IW->tc.cylinder : ret;
}
Wpt* GetDefaultLD(){
	return ((u32)WPT->def_ld < (u32)WPT->wpt_count)? &WPT->wpt[WPT->def_ld] : 0;
}
Wpt* GetNearTarget(){
	return (IW->mp.nw_target < (u32)WPT->wpt_count)? &WPT->wpt[IW->mp.nw_target] : 0;
}


///////////////////////////////////////////////////////////////////////////////
// ����������
///////////////////////////////////////////////////////////////////////////////
void Isr();
void Isr_end();
void InitNavi();

// �v���O�����G���[�̃`�F�b�N
void InitError(){
	// �`�F�b�N�p�����[�^���o�͂��Ē�~
	Putsf("%mInitError%r" "IWSZ:%08x=03007ffc%r" "PGMS:%08x<%08x%r" "ISRS:%08x<%08x",
		&IW->irq_handler, &__iwram_overlay_lma, PROGRAM_LIMIT, (u32)Isr_end - (u32)Isr, sizeof(IW->isr_cpy));
	for(;;);// ��~
}

// �SI/O�̏�����
void Init(){
	// �܂����������������BIWRAM��0�N���A
	DmaClear(3, 0, IW->global_reserve, IW->isr_cpy - IW->global_reserve, 32); // �N���A����̂�isr_cpy���O�܂�
//	BiosRegisterRamReset(BIOSRESET_IWRAM); // �c�������̈������̂ŁA����͎g��Ȃ�

	// �O�Ճo�b�t�@��0x80000000�Ŗ��߂�
	DmaClear(3, INVALID_LAT, LOCUS->val, sizeof(LOCUS->val), 32);
	LOCUS->index     = LOCUS->sample = 0;
	IW->mp.cur_lat   = INVALID_LAT;
	IW->mp.nmea_xalt = INVALID_VAL;

	// Route/Wpt/Splite�̓w�b�_���̃N���A�����ŏ\��
	ROUTE->route_count = 0;
	WPT->wpt_count = 0;
	SPLITE->flag = 0; // ����X�v���C�g���[�h����

	// �����o�b�t�@�͏������̕K�v�Ȃ�
	
	// IWRAM�̈�̔�0�����l��ݒ�B�ʓ|�Ȃ̂ŁA�ł��邾��0�����l�Ŏg����悤�ɂ���c
	IW->mp.cur_view		= -1;
	IW->mp.pre_task		= -1;
	IW->mp.nw_target	= -1;
	IW->mp.goal_dist.pos= -1;
	IW->mp.boot_msg		= 1;
	IW->mp.proc			= MP_Navi; // �Z�b�g�����ŃR�[���͂��Ȃ�
	IW->mp.nw_start.lat	= INVALID_LAT;
	strcpy(IW->mp.nw_start.name, "SearchPos"); // 9����+1=10byte
	IW->px.lat			= INVALID_LAT;


	// ����I/O�������B�ŏ��̓r�f�I�B�r�f�I���g���Ȃ��ƃG���[�\�����o���Ȃ��̂Łc
	InitVideo();

	// �ȍ~�A�G���[���b�Z�[�W�\��OK

	//�T�E���h��{�ݒ�
	REG16(REG_SGCNT0_L) = 0x0000; // S1-4�͂����OnOff���䂷��
	REG16(REG_SGCNT0_H) = 0x0006;
	REG16(REG_SGCNT1)   = 0x0080;
	REG16(REG_SGBIAS)	= 0x0200;
	IW->vm.vario_test = VARIO_TEST_DISABLE;

	// UART������
	StartGPSMode(0); //�Ƃ肠����9600bps�ɐݒ�
	REG16(REG_IE) |= SIO_INTR_FLAG;

	// V BLANK������
	REG32(REG_DISPSTAT) = DISPSTAT_VBIRQ | DISPSTAT_VCIRQ;
	REG16(REG_IE) |= V_BLANK_INTR_FLAG;

	// �^�C�}������(Anemometer�p)
	REG16(REG_TM3CNT)   = 0x83;

	REG16(REG_TM2CNT)   = 0x00; // ���MFIFO Empty�̌��o�Ɏg���B�f�t�H���g�͖����B
	REG16(REG_IE) |= TIMER2_INTR_FLAG;

	// ���荞�݊J�n
	s32 i = (u32)Isr_end - (u32)Isr;// OK?
	if(i > sizeof(IW->isr_cpy))						InitError(); // IWRAM��ISR�̈�Ɏ��܂�Ȃ�!
	if((u32)&IW->irq_handler != 0x03007ffc)			InitError(); // MyIWRAM�T�C�Y�e�X�g�p
	if((u32)&__iwram_overlay_lma >= PROGRAM_LIMIT)	InitError(); // �v���O�����T�C�Y�e�X�g
	memcpy(IW->isr_cpy, (void*)Isr, i);
	IW->irq_handler = (u32)IW->isr_cpy;	// ISR�͍������삳���邽�߂�IWRAM��ARM���s����
	INTR_ON();


	// �^�X�N�ݒ��������
	IW->tc = INIT_TASK_CONFIG;

	// MultiBoot�`�F�b�N
	if(IS_MBOOT()){
		FillBox(8, 15, 21, 18);
		DrawTextCenter(16, "Multiboot");
	}

	DrawTextCenter(10, "Loading config..."); // 17����

	// �J�[�g���b�W��������
	if((IW->cw.init_error = IW->cif->InitCart()) != 0){
		// �������G���[���̓J�[�g���b�W�Ȃ�
		IW->cif = &CIF_NONE;
	}
		
	// �^�X�N�ݒ���J�[�g���b�W���烍�[�h
	if(((REG16(REG_KEYINPUT) ^ 0x3ff) & KEY_LOAD_GUARD) != KEY_LOAD_GUARD){ // ���荞�݌ォ�B���Ȃ̂ŁA�K�[�h�L�[�͎��O�Ń��W�X�^�A�N�Z�X
		s32 ret = IW->cif->ReadData(FI_CONFIG_OFFSET, &IW->tc, sizeof(IW->tc), FLBL_TASK_MAGIC);
		if(ret){
			FillTile(MAP_BG1, 0, 16, 29, 19, SELMENU_BK3); // �G���[���͐�Fill�ŋ���
			Putsf("%1.16mThe config was reset because%r" "of the E%d", ret);
			if(ret == FLASH_E_MAGIC) Puts(" (software upgrade)"); // FLASH_E_MAGIC�͏�����������Ŏ��ɔ���������̂ŃR�����g��t���Ă݂��B
		}

		// �����ă��[�g�E�F�C�|�C���g�����[�h
		IW->cif->ReadData(FI_ROUTE_OFFSET, ROUTE, ROUTE_WPT_SIZE, FLBL_ROUTE_MAGIC); // ���[�g�������Ă��G���[�ɂ͂��Ȃ�
	} else {
		// �^�X�N�f�[�^�̈悪���ċN�����Ȃ��Ȃ����ꍇ�ɔ����āA�f�t�H���g�l�ŋN������
		FillBox(8, 15, 21, 18);
		DrawTextCenter(16, "LoadGuard!");
	}
	
	// �ǂݍ��񂾃f�[�^�̐������`�F�b�N�ƏC��
	if((u32)ROUTE->route_count > MAX_ROUTE)		ROUTE->route_count= 0;
	if((u32)WPT->wpt_count     > MAX_WPT  )		WPT->wpt_count = 0;
	if((u32)WPT->def_ld >= (u32)WPT->wpt_count)	WPT->def_ld = 0;
	if(IW->tc.route >= ROUTE->route_count)		IW->tc.route = 0;

	// ���v���̏�����
	InitTally();

	// �^�X�N���O�̏�����
	InitTaskLog(0);

	// �p���b�g�̍Đݒ�
	InitPalette(IW->tc.palette_conf); // ���[�h�����^�X�N�ݒ�̃K���}�␳�𔽉f

	// �i�r������
	InitNavi();

	// �g���b�N���O�ϐ�������
	DrawTextCenter(10, "Checking TrackLog..."); // 20����
	InitLogParams();  // ����͒x��SD�J�[�h���g���Ɛ��b������ꍇ������

	// �����������B���Ƃ�GPS�̐ڑ��҂�
	DrawTextCenter(10, "Connecting to GPS..."); // 20����
}

///////////////////////////////////////////////////////////////////////////////
// �T�X�y���h/���W���[������
///////////////////////////////////////////////////////////////////////////////
void Suspend(){
	// GPS��~
	LpSendCmd(Cmnd_Turn_Off_Pwr);
	s32 t = IW->vb_counter;

	// ����~
	EnableSound(SOUND_CH_VARIO, -1);
	EnableSound(SOUND_CH_KEY,   -1);

	// �t���C�g���O�̃T�X�y���h�����Z�[�u
	if(IW->tc.flog_as && IW->mp.tally.log_time) SaveFLog(IW->mp.flog_presave, &IW->mp.tally, sizeof(Tally), 0);
	IW->cif->Flush(); // ���ۑ��̃f�[�^���t���b�V��������

	// GPS��~�҂�
	while(IW->vb_counter - t < VBC_TIMEOUT(2)); // ��ʃ��b�Z�[�W�������邽�߁A2�b�قǃE�F�C�g����

	//�T�X�y���h�J�n
	REG16(REG_DISPCNT)	= FORCE_BLANK;
	REG16(REG_IE)	   |= KEY_INTR_FLAG;
	REG16(REG_P1CNT)	= 0xc000 | KEY_RESUME; // L/R���������ōċN��
	BiosStop();


	// ... L+R�L�[�҂� ...


	// ���W���[������
	REG16(REG_DISPCNT) = DISP_MODE_1 | DISP_BG0_ON | DISP_BG1_ON | DISP_BG2_ON | DISP_OBJ_ON | DISP_OBJ_CHAR_1D_MAP;
	REG16(REG_IE) &= ~KEY_INTR_FLAG;
	IW->mp.sel = 0;
	IW->mp.freeflight = 0;
	IW->px.gstate = 0; // �O���C�_��~�X�e�[�g
	if(IW->gi.state == GPS_PVT_WAIT) IW->gi.state = GPS_PVT;
	IW->mp.auto_off_cnt = IW->vb_counter;
	IW->mp.resume_vbc   = IW->vb_counter;
	if(IW->tc.tally_clr == 1) InitTally();
	IW->mp.tlog.abs_flag |= ABSF_SEPARETE; // �g���b�N���O �Z�p���[�^�}��
	IW->mp.tally.log_time = 0;
}

///////////////////////////////////////////////////////////////////////////////
// �A�v���������A�^�X�N����
///////////////////////////////////////////////////////////////////////////////
typedef s32 (*MyTask)();
const MyTask MY_TASK[] = {
	MT_GPSCtrl,
	MT_DataLink,
	MT_AppLayer,
	MT_Navi,
	MT_Vario,
	MT_Menu,
	MT_Voice,
	MT_TrackLog,
	0
};

u32 UpdateCPULoad(){
	u32 cur = IW->anemo.tm;
	u32 pre = InterlockedExchange(&IW->mp.last_load, cur);
	return cur - pre;
}
u32 GetLoad99(u32 val, s32 sum){
	if(!sum) return 0;
	val = RoundDiv(val * 100, sum);
	if(val > 99) val = 99;
	return val;
}

s32 DoMTask(){
	const MyTask* p;
#define LOADLOG_TASKSTART 3
#define LOADLOG_TASKEND   (3 + 8)
	u32 load_num = LOADLOG_TASKSTART; // �^�X�N��#3����
	u32 ret = 0;
	for(p = MY_TASK ; *p && !ret; ++p){
		ret = (*p)();
		IW->mp.load[load_num++] += UpdateCPULoad();
	}
	return ret;
}

// crt����AgbMain�܂ł͑��΃W�����v�ŕK�����B
void AgbMain(){
	// �܂��ŏ��ɃJ�[�g���b�W���ʂƃ^�C�v�ʏ���
	if(GetPC() > (u32)AgbMain + 0x100){ // SuperCard�Ƃ�?
		// 08000000�Ԓn��ROM�N���̃J�[�g���b�W�g�p�����ARAM�փW�����v���ē��삳����
		DmaCopy(3, ROM_ADDRESS, EX_WRAM, ROM_SIZE, 32);

		// SCSD/�G�~�����[�^����
		StartGPSMode(0);
		if(REG16(REG_SIOCNT) & UART_DATA8){// VBA�͉��̂�UART_DATA8�������Ȃ��̂𗘗p�c
			IW->cif = &CIF_SUPERCARD_SD;
		} else {
			IW->cif = &CIF_EMULATOR;
		}
		JumpR0(AgbMain); // �����J�Ŏw�肵��EX_WRAM�I�t�Z�b�g�ւ̃W�����v�ƂȂ�
	}
	if(IS_MBOOT())	IW->cif = &CIF_NONE; // MultiBoot��RAM���s
	if(!IW->cif)	IW->cif = &CIF_JOYCARRY;
	// �����܂łɃJ�[�g���b�W�^�C�v�m�肵�Ă���

	// �������AI/O������
	Init();

	//Load �v�����J�n
	UpdateCPULoad();

	// �^�X�N���[�v
	for(;;){
		IW->intr_flag = 0;
		if(DoMTask()) continue; // �ď����v��������΍��D��x�̃^�X�N�֐����߂�(NMEA�A����荞�ݗp)

		// CPU Load�m�F�p
		if(IW->mp.load_test && IW->vb_counter - IW->mp.load_vbc > IW->mp.load_test){
			IW->mp.load_vbc = IW->vb_counter;

			u32 i, sum = 0;
			u32 diff[MAX_LOAD_LIST];
			for(i = 0 ; i < LOADLOG_TASKEND ; ++i){
				sum += diff[i] = InterlockedExchange(&IW->mp.load[i], 0); // ���ۂ�Interlock���K�v�Ȃ̂� i=0 �݂̂����c
			}

			FillTile(MAP_BG1, 24, 2, 29, 17, SELMENU_BK3);
			SelectMap(MAP_BG0);
			Putsf("%24.2m" "Intr%2d%r" "Main%2d%r" "GPS %2d%r" "Navi%2d%r" "Sund%2d%r" "Menu%2d%r" "TLog%2d%r" "Idle%2d",
#define LOAD_AVG(n) GetLoad99((n), sum)
				LOAD_AVG(diff[0]),						// intr
				LOAD_AVG(diff[2]),						// main
				LOAD_AVG(diff[3] + diff[4] + diff[5]),	// gps
				LOAD_AVG(diff[6]),						// navi
				LOAD_AVG(diff[7] + diff[9]),			// vario + Voice
				LOAD_AVG(diff[8]),						// menu
				LOAD_AVG(diff[10]),						// TrackLog
				LOAD_AVG(diff[1]));						// idle
		}
		IW->mp.load[2] += UpdateCPULoad(); // Main��#2 (�Ƃ͌����Ă�Load���肵�����ׂ̂�����d���͖������c)

		// ����d�������炷���߁A�ł��邾���X���[�v�ɓ����B
		while(!IW->intr_flag){
			// �A�C�h����
#define VBC_1min	3600 // 60frame*60sec
			if(IW->tc.auto_off && (IW->vb_counter - IW->mp.auto_off_cnt) > IW->tc.auto_off * VBC_1min){
				Suspend();	// GPS�I�t�Ŗ����삪��������A�I�[�g�T�X�y���h����
			} else {
				BiosHalt(); // �d�r���������邽�߂ɂł��邾���X���[�v�ɂ��Ă���
			}
			IW->mp.load[1] += UpdateCPULoad(); // �A�C�h����#1
		}
	}
}

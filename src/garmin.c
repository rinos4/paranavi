///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// Garmin GPS�̃o�C�i���v���g�R���w�������Ŏ����B���̂ق��AGPS�f�[�^��
// �g���Ղ�PvtX�ɕϊ�������A���[�V�������o�����������Œ�`�B

///////////////////////////////////////////////////////////////////////////////
// �萔
///////////////////////////////////////////////////////////////////////////////
const u8 DLE = 16;
const u8 ETX =  3;

const double D10XLAT	= 1000.0 * 60 * 60 * 180 / 0x80000000;
const double RAD2LAT	= 1000.0 * 60 * 60 * 180 / M_PI;
const double ANG_K		= 0x8000 / M_PI;
const double D10XLAT_i	= 1 / (1000.0 * 60 * 60 * 180 / 0x80000000);

const u8* const BAUDRATE_STR[4] = {
	"9600", "38400", "57600", "115200"
};

///////////////////////////////////////////////////////////////////////////////
// Local�֐�
///////////////////////////////////////////////////////////////////////////////
// rint�̂����B����s32��Ԃ�
s32 MyRound(double v){
	if(v < 0) return (s32)(v - 0.5);
	else      return (s32)(v + 0.5);
}

// �C�ӃA���C�����g�R�s�[(�y��memcpy)
void Copy2(void* dst, const void* src){
	((u8*)dst)[0] = ((u8*)src)[0];
	((u8*)dst)[1] = ((u8*)src)[1];
}
void Copy4(void* dst, const void* src){
	((u8*)dst)[0] = ((u8*)src)[0];
	((u8*)dst)[1] = ((u8*)src)[1];
	((u8*)dst)[2] = ((u8*)src)[2];
	((u8*)dst)[3] = ((u8*)src)[3];
}

static double GetDouble(const u8* p){
	double r;
	((u8*)&r)[0] = p[4];
	((u8*)&r)[1] = p[5];
	((u8*)&r)[2] = p[6];
	((u8*)&r)[3] = p[7];
	((u8*)&r)[4] = p[0];
	((u8*)&r)[5] = p[1];
	((u8*)&r)[6] = p[2];
	((u8*)&r)[7] = p[3];
	return r;
}
static inline s32 GetDoubleK(const u8* p){ return MyRound(GetDouble(p) * 1000); }

float GetFloat(const u8* p){
	float r;
	Copy4(&r, p);
	return r;
}
static inline s32 GetFloatK(const u8* p){ return MyRound(GetFloat(p) * 1000); }

s32 GetLong(const u8* p){
	s32 r;
	Copy4(&r, p);
	return r;
}

s16 GetInt(const u8* p){
	s16 r;
	Copy2(&r, p);
	return r;
}

// �[�N����
static inline u32 IsLeapYear(u32 year){
//	return !(year % 4) && ((year % 100) || !(year % 400));
	return !(year & 3); // 1990�`2058�Ȃ炱��ŏ[���c
}

static inline u32 GetYearDay(u32 year){
	return IsLeapYear(year)? 366 : 365;
}

// �L���[����(���[�V�������o�p)
void AddS32Queue(S32Queue* sr, s32 val){
	sr->val[sr->tail] = val;
	if(++sr->tail >= MAX_REC) sr->tail = 0;
	if(sr->head == sr->tail && ++sr->head >= MAX_REC) sr->head = 0;
}

s32 SumS32QueueBase(const S32Queue* sr, u32 count, u32* rc){
	s32 sum = 0, tail = sr->tail;
	for(*rc = 0 ; *rc < count &&  tail != sr->head ; ++*rc){
		if(!tail) tail = MAX_REC;
		sum += sr->val[--tail];
	}
	return sum;
}
s32 SumS32Queue(const S32Queue* sr, u32 count){
	return SumS32QueueBase(sr, count, &count);
}
s32 AvgS32Queue(const S32Queue* sr, u32 count){
	s32 sum = SumS32QueueBase(sr, count, &count);
	if(count) return RoundDiv(sum, count);
	return 0;
}
s32 S32QueueVhDiff(u32 count){
	const S32Queue* sr = &IW->pr.vh_mm;
	s32 i, diff = 0, pre = 0;
	s32 tail = sr->tail;
	for(i = 0 ; i <= count &&  tail != sr->head ; ++i){
		if(!tail) tail = MAX_REC;
		s32 cur = sr->val[--tail];
		if(i) diff += myAbs(pre - cur);
		pre = cur;
	}
	return diff;
}
s32 S32QueueAngDiff(u32 count){
	const S32Queue* sr = &IW->pr.ang64;
	s32 i, diff = 0;
	s32 tail = sr->tail;
	for(i = 0 ; i < count &&  tail != sr->head ; ++i){
		if(!tail) tail = MAX_REC;
		diff += myAbs(sr->val[--tail]);
	}
	return diff;
}

void LocusBigShift(s32 dif32, s32* cur, s16* dif){
	dif32 = Range32(-0x7fff, 0x7fff, dif32 >> LOCUS_BSFT);
	*dif  = (s16)dif32;
	*cur += dif32 << LOCUS_BSFT;
}


///////////////////////////////////////////////////////////////////////////////
// �O���C�_���
///////////////////////////////////////////////////////////////////////////////
// �y�X�e�[�g��`�z
// ������: �A���I�ȍ������ւ̃x�N�g���ω��̍��v��L45���ȏ�ƂȂ邱�� (10�x�N�g���ȓ����o)
// �E����: �A���I�ȉE�����ւ̃x�N�g���ω��̍��v��R45���ȏ�ƂȂ邱�� (10�x�N�g���ȓ����o)
// �Z���^�����O: �ߋ�60�|�C���g�̃x�N�g�������v��300���ȏ�ɂȂ邱�� (60�x�N�g���ȓ����o)
// ���[�����O: 120���ȏ�̐���ω������݂Ɍ��o����邱��(60�x�N�g���ȓ��ŉ񐔃J�E���g)
// �s�b�`���O: ���������ɉ����x1m/s�ȏ�̑��x�̕ω������݂Ɍ��o����邱��(60�x�N�g���ȓ��ŉ񐔃J�E���g)
// ���i:       ��L�ȊO�̏�ԂŁA�O�񂪒�~�łȂ��ꍇ�܂��� hv > stop m/s
// �q�X�e���V�X�͓��ɐ݂��Ȃ�

static inline s32 GetSgn(s32 v){
	if(v < 0) return -1;
	if(v > 0) return  1;
	return 0;
}
s32 CheckSgn(s32 v1, s32 v2){
	if(!v1 || !v2) return 1; // �ǂ��炩��0�̂Ƃ��́A�������]�Ȃ��Ƃ���
	return (v1 < 0) == (v2 < 0);
}

// �s�b�`���O���o
s32 CheckPitch(PvtX* px){
//	const S32Queue* up = &IW->pr.up_mm;
	const S32Queue* up = &IW->pr.vh_mm; // �������x�ł͂Ȃ��A�������x�Ŕ��f����
	s32 state = 0;
	s32 pre_val = 0, pre_diff = 0, sum = 0, next_up = 0, same = 0;

	s32 tail = up->tail, num = 0;
	for(; tail != up->head ; ++num){
		if(!tail) tail = MAX_REC;
		s32 cur_val = up->val[--tail];
		if(num){
			s32 diff = pre_val - cur_val;
			if(CheckSgn(pre_diff, diff)){
				// �s�b�`���O��������
				sum += diff;
				if(++same > IW->tc.pitch_freq) sum = 0;
			} else {
				// �s�b�`�����ω�
				if(same > 1 && myAbs(sum) > IW->tc.pitch_diff && CheckSgn(sum, next_up)){
					// �s�b�`���O�J�E���^UP
					state += GSTATE_PITCH;
					if(!next_up && sum < 0) state |= GSTATE_PITCHTOP_D;
					next_up = -sum; // �t�����ւ̐؂�Ԃ���҂�
				}
				sum = diff;
				same = 0;
			}
			pre_diff = diff;
		}
		pre_val = cur_val;
	}

	// �s�b�`���O�؂�ւ��`�F�b�N
	if((state & GSTATE_PITCH_MASK) > 2 && (state & GSTATE_PITCHTOP_D) != (px->gstate & GSTATE_PITCHTOP_D)){
		state |= GSTATE_PITCHTOP_X;
	}
	return state;
}

// ���[�����O/�Z���^�����O���o
s32 CheckRoll(PvtX* px){
	const S32Queue* ang = &IW->pr.ang64;
	const S32Queue* vhm = &IW->pr.vh_mm;

#define STRAIGHT_DETECT 16
	s32 sum_v[STRAIGHT_DETECT], sum_vi = 0;

	// ���񌟏o
	s32 state = 0;
	s32 diff = 0, pre_diff = 0, sum = 0, roll_sum = 0, next_roll = 0, turn = 0, same = 0, num = 0, t;
	s32 sum_r = 0, sum_l = 0;
	s32 centering_guard = 0, str_count = 0;

	s32 ang_i = ang->tail;
	s32 vhm_i = vhm->tail;
	if(!vhm_i) vhm_i = MAX_REC;
	s32 pre_vhm = vhm->val[--vhm_i], cur_vhm;

	for(; ang_i != ang->head && vhm_i != vhm->head ; ++num, pre_vhm = cur_vhm, pre_diff = diff){
		if(!ang_i) ang_i = MAX_REC;
		if(!vhm_i) vhm_i = MAX_REC;
		diff = ang->val[--ang_i];
		cur_vhm  = vhm->val[--vhm_i];
		if(cur_vhm < IW->tc.stop_dir || pre_vhm < IW->tc.stop_dir){
			pre_vhm = 0;//��~�t���O�Z�b�g
			diff = 0;
		}

		// �Z���^�����O�K�[�h����
		if(!centering_guard){
			// �A�����i�`�F�b�N
			if(myAbs(diff) < ANG64K(10)){
				sum_v[sum_vi] = sum;
				if(++sum_vi    >= STRAIGHT_DETECT) sum_vi = 0;
				if(++str_count >= STRAIGHT_DETECT && myAbs(sum_v[sum_vi] - sum) < ANG64K(30)) {
					centering_guard = 1;
				}
			} else {
				str_count = 0;
			}

			// �t�؂�Ԃ�60���`�F�b�N
			if(diff > 0) sum_l += diff;
			else         sum_r -= diff;
			if(sum_r > ANG64K(60) && sum_l > ANG64K(60)){
				centering_guard = 1;
			}
		}

		// ��~���A�܂��͒�~����̓���͍����p�������Ȃ̂ŃX�L�b�v
		if(!pre_vhm) continue;

		// ����ʂ��烍�[�����O�ƃZ���^�����O���`�F�b�N
		sum += diff;

		// �ω��ʂ��烍�[�����O�`�F�b�N
		if(CheckSgn(pre_diff, diff)){
			// �����������
			roll_sum  += diff;
			if(++same > IW->tc.roll_freq) roll_sum = 0;
		} else {
			turn++;
			// ��������ω�
			if(same > 1 && myAbs(roll_sum) > 182 * IW->tc.roll_diff && CheckSgn(roll_sum, next_roll)){
				// ���[�����O�J�E���^UP
				state += GSTATE_ROLL;
				if(!next_roll && roll_sum < 0) state |= GSTATE_ROLLTOP_L;
				next_roll = -roll_sum; // �t�����ւ�120���؂�Ԃ���҂�
			}
			roll_sum = diff;
			same = 0;
		}
		// ����`�F�b�N
		if(!turn && num < 10){
			if(sum > ANG64K( 45)) state |= GSTATE_TURN_L;
			if(sum < ANG64K(-45)) state |= GSTATE_TURN_R;
		}

		// �Z���^�����O�`�F�b�N
		if(centering_guard) continue;

		t = myAbs(sum);
		if(t > CETERING_DETECT){
			state |= GSTATE_CENTERING;
			if(sum > 0) state |= GSTATE_CENTERING_X;

			if(px->up_turn == INVALID_VAL && t > ANG64K(360)){
				px->up_turn = SumS32Queue(&IW->pr.up_mm, num); // �㏸��/turn���i�[
				px->up_r    = RoundDiv(SumS32Queue(&IW->pr.vh_mm, num) * 104, t); // ��]���a���i�[
				px->up_time = num;
			}
		}
	}
	return state;
}

// ���[�V�������o
s32 GetGliderState(PvtX* px){
	s32 state = 0;

//#define STOP_CHECK 5
//	if(SumS32Queue(&IW->pr.vh_mm, STOP_CHECK) > IW->tc.stop_dir * STOP_CHECK){
	if(px->gstate || px->vh_mm > IW->tc.start_spd){
		state = CheckRoll(px); // ��]�`�F�b�N�͈ړ����̂�

		// �X�p�C����/�X�g�[������
		if(state & GSTATE_CENTERING){
			if(px->up_mm < IW->tc.spiral_spd) state ^= GSTATE_SPIRAL | GSTATE_CENTERING;
		} else {
			if(px->up_mm < IW->tc.stall_spd)  state |= GSTATE_STALL;
		}

		// ���[���؂�ւ��`�F�b�N
		if((state & GSTATE_ROLL_MASK) > 2 && (state & GSTATE_ROLLTOP_L) != (px->gstate & GSTATE_ROLLTOP_L)){
			state |= GSTATE_ROLLTOP_X;
		}
	}

	// ���i���o
	if(!(state & GSTATE_TURN_MASK)){
		if(px->gstate || px->vh_mm > IW->tc.start_spd){
			state |= GSTATE_STRAIGHT;
			// �s�b�`���O���o
			state |= CheckPitch(px);
		}
	}

	// �e�C�N�I�t�́A�����Ō��o���Ă���
	if(!px->gstate && state){
		PlaySG1(SG1_COMP1);
		if(IW->tc.tally_clr == 2) InitTally();
		if(IW->tc.auto_lock & 1) IW->mp.key_lock = 1;
		if(!IW->task_log.tl1[0].dtime) UpdateTaskLog(0);
		IW->mp.tally.takeoff_time = px->dtime;
	}
	return state;
}


///////////////////////////////////////////////////////////////////////////////
// �����v�Z
///////////////////////////////////////////////////////////////////////////////
const u32 DAY_COUNT[2][12] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

u32 GetDTTime(u32 v, u32* hour, u32* min, u32* sec){
	// BiosDiv��s32�Ȃ̂ŁA�ŏ�ʃr�b�g����ꏈ������
	u32 t = v & 1;
	v = BiosDiv(v >> 1, 30, sec);
	*sec = (*sec << 1) | t;
	v = BiosDiv(v, 60, min);
	return BiosDiv(v, 24, hour);
}
void GetDTDate(u32 v, u32* year, u32* month, u32* day){
	// BiosDiv��s32�Ȃ̂ŁA�ŏ�ʃr�b�g����ꏈ������
	v = BiosDiv(v >> 1, 30 * 60 * 24, &v); // ���ɂ����o��

	// �N�v�Z
	for(*year = 1990 ;; ++*year){
		u32 day = GetYearDay(*year);
		if(v <= day) break;
		v -= day;
	}

	// ���v�Z
	const u32* mt = DAY_COUNT[IsLeapYear(*year)? 1 : 0]; 
	for(*month = 0 ;; ++*month){
		if(v <= mt[*month]) break;
		v -= mt[*month];
	}
	++*month;

	// �c�肪���ɂ�
	*day = v;
}
void GetDateTime(u32 v, u32* year, u32* month, u32* day, u32* week, u32* hour, u32* min, u32* sec){
	BiosDiv(GetDTTime(v, hour, min, sec), 7, week);
	GetDTDate(v, year, month, day);
}

const u32 DAY_SUM[12] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

// 1989/12/31����̗ݐϓ����擾
u32 DayCount(u32 y, u32 m, u32 d){
	// 1990�`2058�Ȃ炱��ŏ[���c
	if(IsLeapYear(y) && m > 2) ++d;
	y -= 1990;
	return 365 * y + (y + 1) / 4 + DAY_SUM[m - 1] + d;
}

// �q�X�g�O�������A�b�v�f�[�g
void AddHist(s32 val, s32* array, s32 array_size, s32* range, s32 min, s32 max){
	if(*range < 1) *range = min;
	while(val > array_size * *range && *range < max){
		if(*range){
			*range *= 2;
			s32 i, len = array_size >> 1;
			for(i = 0 ; i < len ; ++i) array[i] = array[i * 2] + array[i * 2 + 1];
			memset(&array[i], 0, array_size * 2);
		} else {
			*range = min;
		}
	}
	val = BiosDiv(val, *range, &val);
	if(val < 0) val = 0;
	else if(val >= array_size) val = array_size - 1;
	array[val]++;
}

// �o���I�����͓���
void AddHistVario(s32 val, s32* array, s32 array_size, s32* range, s32 min, s32 max){
	if(*range < 1) *range = 1;
	s32 val2 = myAbs(val) * 2;
	while(val2 > array_size * *range && *range < max){
		if(*range){
			*range *= 2;
			s32 i, len = array_size >> 2;
			s32* p1 = array + (array_size >> 1);
			s32* p2 = p1 - 1;
			for(i = 0 ; i < len ; ++i){
				p1[ i] = p1[i *  2] + p1[i *  2 + 1];
				p2[-i] = p2[i * -2] + p2[i * -2 - 1];
			}
			memset(p1 + i, 0, array_size);
			memset(array,  0, array_size);
		} else {
			*range = min;
		}
	}
	val = BiosDiv(val + (array_size >> 1) * *range, *range, &val);
	if(val < 0) val = 0;
	else if(val >= array_size) val = array_size - 1;
	array[val]++;
}

///////////////////////////////////////////////////////////////////////////////
// PVT�ϊ�
///////////////////////////////////////////////////////////////////////////////
// NMEA�ł͐������x���擾�ł��Ȃ����߁Aalt_mm�̍�����v�Z����B
EStatus CalcNMEAUp(PvtX* px, PvtX* pre_px){
	if(px->fix < FIX_3D || pre_px->fix < FIX_3D){
		// �M���ł��Ȃ��l�̏ꍇ�͑O�̒l�����̂܂܎g��
		px->up_mm = pre_px->up_mm;
		IW->mp.nmea_xalt = INVALID_VAL;
	} else {
		// 3D���ʎ��̂ݏ㏸���v�Z (V-DOP�ŏ\���Ȑ��x���`�F�b�N���������ǂ�?)
		if(IW->mp.nmea_xalt == INVALID_VAL) IW->mp.nmea_xalt = px->alt_mm;
		switch(IW->tc.nmea_up){
		case 0: // ���l
		case 1: // 1/2�ŕ��ω�
		case 2: // 1/4�ŕ��ω�
			// 0-3�͏㏸���ω���O��|�C���g�ŕ��ω�������B
			// �Ⴆ��0.0�̒�킩��+8.0�ŏ㏸���n�߂��ꍇ�͈ȉ��̂悤�ɐ���
			// 1/2: +4.0,+6.0,+7.0,+7.5,...
			// 1/4: +2.0,+3.5,+4.6,+5.5
			px->up_mm = (px->alt_mm - IW->mp.nmea_xalt) >> IW->tc.nmea_up;
			IW->mp.nmea_xalt += px->up_mm;
			break;
		}
		s32 dt = px->dtime  - pre_px->dtime;
		if(dt > 1)  px->up_mm = RoundDiv(px->up_mm, dt); // 1�b���x
	}
	return 0;
}

// NMEA��PvtX�ɕϊ�
EStatus CalcPvtNMEA(PvtX* px){
	// px2�Ɋi�[�����V�f�[�^�Ƌ�px����������
	PvtX* pre_px = &IW->px2, swap = *pre_px;
	*pre_px = *px;
	*px = swap;

	// LAT/LON/ALT�͑��ʑO�ł�����l�ƌ������̂��Ȃ��l��Ԃ��B(N36��, E136��,0m��)
	// fix�Ő��킩�ǂ����`�F�b�N����BNG�Ȃ�O�̒l�ɖ߂��B
	if(px->fix < FIX_2D){
		px->lat    = pre_px->lat;
		px->lon    = pre_px->lon;
		px->alt_mm = pre_px->alt_mm;
	}

	// �x�N�g���v�Z
	CalcNMEAUp(px, pre_px);// NMEA�ł͐������x�𒼐ڎ擾�ł��Ȃ��̂Łc
	px->v_mm  = BiosHypot(px->vh_mm, px->up_mm);
	px->vn_mm = (px->vh_mm * (Cos64K(px->h_ang64) >> 6)) >> 9;
	px->ve_mm = (px->vh_mm * (Sin64K(px->h_ang64) >> 6)) >> 9;

	// STOP���x�ȉ��̏ꍇ�͕��ʂ𒼑O�̒l�ɖ߂�
	if(px->vh_mm <= IW->tc.stop_dir){
		px->h_ang64 = pre_px->h_ang64; // ���O�̒l
		// px->v_ang64�͂��̂܂�
	} else {
		px->v_ang64 = BiosAtan64K(px->up_mm, px->vh_mm);
		// px->h_ang64�͍X�V�f�[�^���g��
	}

	// DTime�ݒ� (GPS52D�͓d�r�؂ꁕ�q���ߑ��O��23:59:48����̌o�ߎ��Ԃ�Ԃ������ɋ�ʂ͂��Ȃ�)
	px->dtime = DayCount(px->year, px->month, px->day) * 86400 +
				px->hour * 3600 + px->min * 60 + px->sec + IW->tc.tzone_m * 60;

	CalcPvtExt(px);	// Garmin/NMEA���ʂ̊g������ݒ肷��
	*pre_px = *px; // ����NMEA�p�P�b�g�̎�M�ɔ�����px2���ŐV���ɍX�V���Ă���
	return 0;
}

// GarminPvt��PvtX�ɕϊ�
EStatus CalcPvtGarmin(const D800_Pvt_Data_Type* pvt, PvtX* px){
	PvtX* pre_px = &IW->px2;
	*pre_px = *px;

	memcpy(&IW->pvt_raw, pvt, sizeof(D800_Pvt_Data_Type)); // �f�o�b�O�p�ɃR�s�[���Ă���

	// �ܓx�A�o�x(MAX ddd��mm'ss"xxx)
	px->lat = MyRound(GetDouble(pvt->lat) * RAD2LAT);
	px->lon = MyRound(GetDouble(pvt->lon) * RAD2LAT);

	// ���x
	px->alt_mm = GetFloatK(pvt->alt);
	if(!IW->tc.alt_type) px->alt_mm += GetFloatK(pvt->msl_hght);

	// �덷
	px->epe_mm = GetFloatK(pvt->epe);
	px->eph_mm = GetFloatK(pvt->eph);
	px->epv_mm = GetFloatK(pvt->epv);

	// ���x
	px->vn_mm = GetFloatK(pvt->north);
	px->ve_mm = GetFloatK(pvt->east);
	px->up_mm = GetFloatK(pvt->up);
	px->vh_mm = BiosHypot (px->ve_mm, px->vn_mm);
	px->v_mm  = BiosTripot(px->ve_mm, px->vn_mm, px->up_mm);

	// �p�x
	if(px->vh_mm > IW->tc.stop_dir){
		px->h_ang64 = BiosAtan64K(px->ve_mm, px->vn_mm); // �k���n�_�Ɏ��v���B
		px->v_ang64 = BiosAtan64K(px->up_mm, px->vh_mm);
	}

	// FIX�l
	px->fix = (u32)GetInt(pvt->fix);

	// �����v�Z(2038+20�N��肠��)
//	t = MyRound(GetDouble(pvt->tow)) - GetInt(pvt->leap_scnds) + GetLong(pvt->wn_days) * 86400 + IW->tc.tzone_m * 60;
	// TODO eTrex��leap_scnds �� 0x010e �ɂȂ邱�Ƃ�����H(�b��Ƃ��ĉ���1byte�̂ݎg�p����B�v����!)
	px->dtime = MyRound(GetDouble(pvt->tow)) - (s8)GetInt(pvt->leap_scnds) + GetLong(pvt->wn_days) * 86400 + IW->tc.tzone_m * 60;

	return CalcPvtExt(px);	// Garmin/NMEA���ʂ̊g������ݒ肷��
}

EStatus CalcPvtExt(PvtX* px){
	PvtX* pre_px = &IW->px2; // 1�O�̃f�[�^���i�[����Ă���

	// ��{�f�[�^�X�V
	IW->mp.navi_update |= NAVI_UPDATE_PVT| NAVI_PVT_CHANGE;
	px->pvt_timeout = 0;
	px->counter++;
	if(px->fix > 5) px->fix = 0;
	px->fix_sum[px->fix]++;

	// �����֌W����
	IW->mp.tally.log_time = IW->task_log.log_time = px->dtime;
	// ����1�񂾂��V�[�h��ݒ�
	if(px->dtime && !IW->mp.rand_init){
		IW->mp.rand_init = 1;
		MySrand(px->dtime);
	}
	// �N���[�Y�^�C������
	if(IW->tc.close_time && IW->tc.task_mode && IW->tc.close_time == px->hour * 60 + px->min){
		Route* rt = GetTaskRoute();
		if(rt && rt->count){
			PlaySG1(SG1_EXIT);
			IW->mp.nw_target = rt->py[rt->count - 1].wpt;
			IW->mp.navi_update |= NAVI_UPDATE_WPT;
			IW->mp.cur_view = -1;
			IW->tc.task_mode = 0; // �t���[�t���C�g�ɂ���
		}
	}

	// �����x
	if(pre_px->fix >= FIX_2D && px->fix >= FIX_2D){
		px->g_mm   = RoundDiv(BiosTripot(	pre_px->vn_mm - px->vn_mm,
											pre_px->ve_mm - px->ve_mm,
											pre_px->up_mm - px->up_mm - 9800) * 10, 98);
	} else {
		px->g_mm = -1;
	}
	px->ldK = px->up_mm? RoundDiv(px->vh_mm * -1000, px->up_mm) : INVALID_VAL;
	GetDateTime(px->dtime, &px->year, &px->month, &px->day, &px->week, &px->hour, &px->min, &px->sec);
	px->h_ang_c    = px->h_ang64 - IW->tc.n_calib * 182; // �␳��x182�ߎ��ŏ[��

	s32 ang_diff = GetAngLR(pre_px->h_ang64 - px->h_ang64);
	px->up_turn = INVALID_VAL; // �f�t�H���g�Ŗ����l��ݒ肵�Ă���
	AddS32Queue(&IW->pr.vh_mm, px->vh_mm);
	AddS32Queue(&IW->pr.up_mm, px->up_mm);
	AddS32Queue(&IW->pr.ang64, ang_diff);

	s32 pre_state = px->gstate;
	px->gstate = GetGliderState(px);

	// �O�Տ��L�^ ///////////////////////////////////////////////////////////
	if(++LOCUS->sample >= IW->tc.locus_smp){
		LOCUS->sample = 0;
		if(++LOCUS->index >= MAX_LOCUS) LOCUS->index = 0;
		LocusVal* lv = &LOCUS->val[LOCUS->index];
		lv->alt_x = RoundDiv(px->alt_mm, 1000) << 1; // �ŉ��ʃr�b�g��LatLon�͈̔͊O�}�[�N�Ɏg��
		lv->up  = px->up_mm;
		// �������ߖ�̂��ߍ����ŕێ�����悤�ύX
		if(px->lat == INVALID_LAT){
			*(u32*)&lv->lat_d = INVALID_LAT;
		} else {
			if(IW->mp.cur_lat == INVALID_LAT){ // �ŏ�?
				IW->mp.cur_lat = px->lat;
				IW->mp.cur_lon = px->lon;
			}
			// ���ŋL�^
			s32 dif_lat = px->lat - IW->mp.cur_lat;
			s32 dif_lon = px->lon - IW->mp.cur_lon;
			if(myAbs(dif_lat) < 0x8000 && myAbs(dif_lon) < 0x8000){
				// 1km���x(���b���O�Ń}�b�n3)�ȉ��Ȃ炱��Ŏ��܂�B(�ɒn�͂����Ə��������c)
				IW->mp.cur_lat += lv->lat_d = (s16)dif_lat;
				IW->mp.cur_lon += lv->lon_d = (s16)dif_lon;
			} else {
				// �����傫������Ƃ��͓��ꏈ��(���t�ύX���ړ����R�R�ɂȂ邪�c)
				LocusBigShift(dif_lat, &IW->mp.cur_lat, &lv->lat_d);
				LocusBigShift(dif_lon, &IW->mp.cur_lon, &lv->lon_d);
				lv->alt_x |= 1; // �͈͊O�}�[�N
			}
		}
	}

	// ���ϒl�v�Z /////////////////////////////////////////////////////////////
	s32 avg = 1;
	if(IW->mp.sp_view >= SP_VIEW_WINDCHECK){
#define SP_MODE_AVG 10
		avg = SP_MODE_AVG;
	} else {
		switch(IW->tc.avg_type){
		case 0: // ����
			if(!(px->gstate & GSTATE_ROTATE)){
				// �Z���^�����O/�X�g�[�����ȊO��10�b����
				avg = 10;
				break;
			}
			// �Z���^�����O/�X�g�[�����̓O���C�_�l�����炤
			// no break
		case 1: // �O���C�_�Œ�
			px->ldK_avg =  IW->tc.my_ldK;
			px->up_avg  = -IW->tc.my_down;
			px->vh_avg  =  RoundDiv(IW->tc.my_ldK * IW->tc.my_down, 1000);
			avg = 0;
			break;
		case 2:	avg =  1;	break;// ���A���^�C��
		case 3:	avg =  3;	break;// 3s �ւ�����
		case 4:	avg = 10;	break;// 10s �ւ�����
		case 5:	avg = 30;	break;// 30s �ւ�����
		}
	}
	if(avg){
		px->vh_avg = AvgS32Queue(&IW->pr.vh_mm, avg);
		px->up_avg = AvgS32Queue(&IW->pr.up_mm, avg);
		px->ldK_avg = px->up_avg? RoundDiv(px->vh_avg * -1000, px->up_avg) : INVALID_VAL;
	}

	// ���v��� ///////////////////////////////////////////////////////////////
	if(px->fix < FIX_2D) return 0; // 2D�ȏ�!

	Tally* tl = &IW->mp.tally;
	// ���v�J�n�ꏊ
	if(tl->start_lat == -1){
		tl->start_lat = px->lat;
		tl->start_lon = px->lon;
	}
	// �J�E���^�A�b�v
	tl->count++;
	u32 len;
	if(!tl->last_sec || tl->last_sec + 1 == px->dtime){
		len = px->v_mm; // 3D
	} else {
		// ���X�g���͋����v��
		CalcDist(tl->last_lat, tl->last_lon, px->lat, px->lon, &len, 0);
		len =  len * 1000 + 500;
	}
	tl->sum_v	+= len;
	tl->trip_mm += len;
	tl->last_lat = px->lat;
	tl->last_lon = px->lon;
	tl->last_sec = px->dtime;

	if(px->up_mm > 0) tl->sum_gain += px->up_mm;

	// �P�ʃx�N�g�����v(�����p)
	if(px->vh_mm > IW->tc.stop_dir){
		tl->sum_uv++;
		s32 nv = RoundDiv(px->vn_mm * 1000, px->vh_mm);
		s32 ev = RoundDiv(px->ve_mm * 1000, px->vh_mm);
		tl->sum_nv  += nv;
		tl->sum_ev  += ev;
		tl->sum_nsv += myAbs(nv);
		tl->sum_ewv += myAbs(ev);
	}

	// �ō��l
	CHANGE_MAX(tl->max_v,	px->v_mm);
	CHANGE_MAX(tl->max_up,  px->up_mm);
	CHANGE_MIN(tl->min_up,  px->up_mm);
	CHANGE_MAX(tl->max_alt, px->alt_mm);
	CHANGE_MIN(tl->min_alt, px->alt_mm);
	if(px->g_mm >= 0){
		CHANGE_MAX(tl->max_G,	px->g_mm);
		CHANGE_MIN(tl->min_G,	px->g_mm);
	}
	// �O���t�p
	CHANGE_MAX(IW->mp.max_alt, px->alt_mm);
	CHANGE_MIN(IW->mp.min_alt, px->alt_mm);
	CHANGE_MAX(IW->mp.graph_range, myAbs(px->up_mm));

	// �X�e�[�g��
	s32 state = 0;
	switch(px->gstate & GSTATE_TURN_MASK){
	case GSTATE_TURN_L:
		tl->turn_l++;
		state = 1;
		break;
	case GSTATE_TURN_R:
		tl->turn_r++;
		state = 1;
		break;
	case GSTATE_STRAIGHT:
		if(px->vh_mm < IW->tc.stop_dir){
			tl->w_count++;
		} else {
			tl->s_count++;
			tl->s_hv += px->vh_mm;
			tl->s_up += px->up_mm;
		}
		break;
	}
	if(px->vh_mm < IW->tc.stop_dir) state = 2;
	else if(px->gstate & (GSTATE_CENTERING | GSTATE_SPIRAL)) state = 3;// �Z���^�����Oor�X�p�C����

	// �\�A�����O���v
	if(px->up_mm > IW->tc.keep_range){
		tl->soaring_cnt[state]++;
	} else if(px->up_mm < -IW->tc.keep_range){
		tl->sinking_cnt[state]++;
	} else {
		tl->keeping_cnt[state]++;
	}
	if(px->up_mm > 0){
		tl->soaring_sum[state] += px->up_mm;
	} else {
		tl->sinking_sum[state] -= px->up_mm;
	}
	tl->vario_cnt[state]++;
	tl->vario_sum[state] += px->up_mm;

	if(state == 3){
		s32 a2 = myAbs(ang_diff);
		// �Z���^�����O���v
		s32* p  = tl->centering[(px->gstate & GSTATE_CENTERING_X)? 0 : 1];
		p[TALLY_CENTER_COUNT]++;
		p[TALLY_CENTER_TURN] += a2;
		if(px->up_mm > 0){
			p[TALLY_CENTER_LIFT]++;
			p[TALLY_CENTER_LIFT_SUM] += px->up_mm;
			if(p[TALLY_CENTER_LIFT_MAX] < px->up_mm) p[TALLY_CENTER_LIFT_MAX] = px->up_mm;
		} else {
			p[TALLY_CENTER_SINK]++;
			p[TALLY_CENTER_SINK_SUM] += px->up_mm;
			if(p[TALLY_CENTER_SINK_MAX] > px->up_mm) p[TALLY_CENTER_SINK_MAX] = px->up_mm;
		}
		p[TALLY_CENTER_SPEED] += px->vh_mm;
		if(p[TALLY_CENTER_SPEED_MAX] < px->vh_mm) p[TALLY_CENTER_SPEED_MAX] = px->vh_mm;
		p[TALLY_CENTER_G] += px->g_mm;
		if(p[TALLY_CENTER_G_MAX] < px->g_mm) p[TALLY_CENTER_G_MAX] = px->g_mm;
#define CENTERING_CHECK_MASK (GSTATE_CENTERING | GSTATE_CENTERING_X)
		if((pre_state & CENTERING_CHECK_MASK) != (px->gstate & CENTERING_CHECK_MASK)){
			p[TALLY_CENTER_TIMES]++;
			// �Z���^�����O���̐؂�Ԃ�
//(�Z���^�����O���o���W�b�N�ύX�ɂ��A�؂�Ԃ��O�ɃZ���^�����O����x�I������)
//			if(pre_state & (GSTATE_CENTERING | GSTATE_SPIRAL)){
//				s32* p2 = tl->centering[(px->gstate & GSTATE_CENTERING_X)? 1 : 0];
//				p [TALLY_CENTER_TURN] += CETERING_DETECT;
//				p2[TALLY_CENTER_TURN] -= CETERING_DETECT;
//			}
		}

		// �Z���^�����O/�X�p�C�����ʓ��v
		if(!(pre_state & (GSTATE_CENTERING | GSTATE_SPIRAL))) a2 += CETERING_DETECT; // xx_sum��2�͂����Ō��o�J�n���_�̃I�t�Z�b�g���v���X
		if(px->gstate & GSTATE_CENTERING)  tl->center_sum += a2;
		if(px->gstate & GSTATE_SPIRAL)     tl->spiral_sum += a2;
	}

	if(px->gstate & GSTATE_STALL)	   tl->stall_sec++;
	if((px->gstate & GSTATE_ROLLTOP_X ) && ((px->gstate & GSTATE_ROLL_MASK ) >>  8) > 1) tl->roll++;
	if((px->gstate & GSTATE_PITCHTOP_X) && ((px->gstate & GSTATE_PITCH_MASK) >> 20) > 1){
		if(state == 3) tl->pitch_r++;
		else           tl->pitch_s++;
	}

	// �q�X�g�O����
	AddHistVario(px->up_mm, tl->vario_hist, VARIO_HIST, &tl->vario_hist_range, VARIO_HIST_RANGE_MIN, VARIO_HIST_RANGE_MAX);
	AddHist(px->vh_mm * 36, tl->speed_hist, SPEED_HIST, &tl->speed_hist_range, SPEED_HIST_RANGE_MIN, SPEED_HIST_RANGE_MAX);
	AddHist(px->alt_mm,     tl->alt_hist,   ALT_HIST,   &tl->alt_hist_range,   ALT_HIST_RANGE_MIN,   ALT_HIST_RANGE_MAX);
	if(px->vh_mm > IW->tc.stop_dir){
		s32 hist = ((px->h_ang_c & 0xffff) * ANGLE_HIST) >> 16;
		if(hist < 0) hist = 0;
		else if(hist >= ANGLE_HIST) hist = ANGLE_HIST - 1;
		tl->angle_hist[hist]++;
	}

	return 0;
}

void InitTally(){
	// ��{��0�������B������0�ȊO�̒l������̂ŁA����͎��O�Őݒ�
	Tally* tl = &IW->mp.tally;
	DmaClear(3, 0, tl, sizeof(Tally), 32);
	tl->magic = FLOG_MAGIC_FLIGHT;
	tl->start_lat = tl->start_lon = -1;
	tl->max_up = tl->max_alt = -9999999;
	tl->min_up = tl->min_alt =  9999999;
	tl->min_G =                 99999;

	tl->keep_range = IW->tc.keep_range;
	IW->mp.flog_presave = -1; // ���S������
}


///////////////////////////////////////////////////////////////////////////////
// �E�F�C�|�C���g�_�E�����[�h
///////////////////////////////////////////////////////////////////////////////

#define DL_DISP() (IW->gi.dl_accept & 1)

// �g�p�\�ȕ��������𖼑O�ɒǉ�
void NameCopy(u8* dst, const u8* src, u32 size){
	for(; size && *src ; --size, ++src) if(IsHalf(*src)) *dst++ = *src;
}

// �E�F�C�|�C���g�����X�g�ɒǉ�
u32 AddCommonWpt(const u8* name, u32 name_size, s32 alt, s32 lat, s32 lon){
	// �E�F�C�|�C���g������(�����̃E�F�C�|�C���g������������㏑��)
	u32 i;
	for(i = 0 ; i < WPT->wpt_count && strncmp(name, WPT->wpt[i].name, name_size) ; ++i);
	if(i >= MAX_WPT){
		if(DL_DISP()) Puts("Overflow ");
		return -1; // �o�b�t�@�s��
	}

	// WPT�ύX/�ǉ�
	Wpt* dst = &WPT->wpt[i];
	if(i == WPT->wpt_count){
		WPT->wpt_count++;
		memset(dst->name, 0, WPT_NAMESIZE);
		NameCopy(dst->name, name, name_size);
		if(DL_DISP()) Puts("Add      ");
	} else {
		if(DL_DISP()) Puts("Overwrite");
	}
	if(DL_DISP()){
		Locate(4, 14);
		PutsNameB(dst->name);
	}

	dst->alt = alt;
	dst->lat = lat;
	dst->lon = lon;

	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ���[�g/WPT�ύX�t���O���Z�b�g
	IW->wpt_sort_type = GetSortType(); // �ă\�[�g�̂��߂̃��Z�b�g
	return i;
}
u32 AddD10XWPT(D10X_Wpt_Type* wpt, u32 len){
	return AddCommonWpt(wpt->ident, sizeof(wpt->ident), -1,                           MyRound(GetLong(wpt->lat) * D10XLAT), MyRound(GetLong(wpt->lon) * D10XLAT));
}
u32 AddD108WPT(D108_Wpt_Type* wpt, u32 len, u16* cyl){
	if(cyl) *cyl = (MyRound(GetFloat(wpt->dist)) & 0xffff);
	return AddCommonWpt(wpt->ident, WPT_NAMESIZE,        MyRound(GetFloat(wpt->alt)), MyRound(GetLong(wpt->lat) * D10XLAT), MyRound(GetLong(wpt->lon) * D10XLAT));
}
u32 AddD109WPT(D109_Wpt_Type* wpt, u32 len, u16* cyl){
	if(cyl) *cyl = (MyRound(GetFloat(wpt->dist)) & 0xffff);
	return AddCommonWpt(wpt->ident, WPT_NAMESIZE,        MyRound(GetFloat(wpt->alt)), MyRound(GetLong(wpt->lat) * D10XLAT), MyRound(GetLong(wpt->lon) * D10XLAT));
}
u32 AddD110WPT(D110_Wpt_Type* wpt, u32 len, u16* cyl){
	if(cyl) *cyl = (MyRound(GetFloat(wpt->dist)) & 0xffff);
	return AddCommonWpt(wpt->ident, WPT_NAMESIZE,        MyRound(GetFloat(wpt->alt)), MyRound(GetLong(wpt->lat) * D10XLAT), MyRound(GetLong(wpt->lon) * D10XLAT));
}
u32 AddD105WPT(D105_Wpt_Type* wpt, u32 len){
	return AddCommonWpt(wpt->ident, WPT_NAMESIZE,       -1,                           MyRound(GetLong(wpt->lat) * D10XLAT), MyRound(GetLong(wpt->lon) * D10XLAT));
}
u32 AddD106WPT(D106_Wpt_Type* wpt, u32 len){
	return AddCommonWpt(wpt->ident, WPT_NAMESIZE,       -1,                           MyRound(GetLong(wpt->lat) * D10XLAT), MyRound(GetLong(wpt->lon) * D10XLAT));
}
u32 AddD150WPT(D150_Wpt_Type* wpt, u32 len){
	return AddCommonWpt(wpt->ident, sizeof(wpt->ident), GetInt(wpt->alt),             MyRound(GetLong(wpt->lat) * D10XLAT), MyRound(GetLong(wpt->lon) * D10XLAT));
}
u32 AddWpt(u8* wpt, u32 len, s16* cyl){
	// �i����
	if(DL_DISP()){
		Putsf("%4.12m%d/%d", IW->gi.dl_count, IW->gi.dl_num);
		if(IW->gi.dl_num) Putsf("(%d%%)", RoundDiv(IW->gi.dl_count * 100, IW->gi.dl_num));
		Puts(": ");
	}

	// �f�o�C�XID����D10?�t�H�[�}�b�g��I�����Ă��ǂ����AGPS��r���ŕς����ꍇ���l�����ăf�[�^�T�C�Y�Ŏ������ʂ���B
	// ��������Ă��Ȃ��g���������A�E�F�C�|�C���g�f�[�^�̂ݑ���GPS�ɍ����ւ��ă��[�h���邱�Ƃ����肤��̂Łc�B
	if(len < 6) return -1; // ���肦�Ȃ��T�C�Y
	if(len >= sizeof(D105_Wpt_Type)){
		if(len <  sizeof(D106_Wpt_Type)) return AddD105WPT((D105_Wpt_Type*)wpt, len); // D105? 11byte�`
		if(len <  sizeof(D108_Wpt_Type)) return AddD106WPT((D106_Wpt_Type*)wpt, len); // D106? 25byte�`
		if(len == sizeof(D150_Wpt_Type)) return AddD150WPT((D150_Wpt_Type*)wpt, len); // D150? 115byte
		if(len >= sizeof(D10X_Wpt_Type) && 0x20 <= wpt[0] && wpt[0] < 0x80) return AddD10XWPT((D10X_Wpt_Type*)wpt, len); // 56 byte�`
		if(wpt[0] != 0x01) return AddD108WPT((D108_Wpt_Type*)wpt, len, cyl); // 49byte�`
		if(wpt[3] == 0x70) return AddD109WPT((D109_Wpt_Type*)wpt, len, cyl); // 49byte�`
		if(wpt[3] == 0x80) return AddD110WPT((D110_Wpt_Type*)wpt, len, cyl); // 49byte�`
	}

	if(DL_DISP()) Puts("Unknown format");
	return -4; // �ۗ�
}

///////////////////////////////////////////////////////////////////////////////
// ���[�g�_�E�����[�h
///////////////////////////////////////////////////////////////////////////////
// ���[�g�̒ǉ�
#define InvalidName(v) (!IsHalf(v)) //((v) < 0x20 || 0x7e < (v))

u32 AddRteHdr(u8* rh, u32 len){
	// �f�o�C�XID����D20?�t�H�[�}�b�g��I�����Ă��ǂ����AGPS��r���ŕς����ꍇ���l�����ăf�[�^�T�C�Y�Ŏ������ʂ���B
	// ��������Ă��Ȃ��g���������A���[�g�f�[�^�̂ݑ���GPS�ɍ����ւ��ă��[�h���邱�Ƃ����肤��̂Łc�B

	//D201_Rte_Hdr_Type�p�̃X�L�b�v
#define D201_Rte_Hdr_Type_SIZE (21)
//	if(len == D201_Rte_Hdr_Type_SIZE && (InvalidName(rh[0]) || InvalidName(rh[1]) || InvalidName(rh[2]) || InvalidName(rh[3]))) rh += 4;
	// �ėp�X�L�b�v?
	for(; len && InvalidName(*rh) ; --len) ++rh;

	if(!len){
#define DEFAULT_ROUTE_NAME "NoName"
		rh = DEFAULT_ROUTE_NAME; // 1�����o�^�ł��Ȃ����c
		len = sizeof(DEFAULT_ROUTE_NAME); // NULL����
	} else {
		u32 max = len;
		if(max > ROUTE_NAMESIZE) max = ROUTE_NAMESIZE;
		for(len = 1 ; len < max && !InvalidName(rh[len]) ; ++len);
		if(len < ROUTE_NAMESIZE) rh[len++] = 0; // MAX�ȉ��̏ꍇ��NULL���܂߂�
	}

	// D202_Rte_Hdr_Type�x�[�X
	Locate(4, 16);
	// ���[�g������(�����̃��[�g������������㏑��)
	u32 i;
	for(i = 0 ; i < ROUTE->route_count && strncmp(rh, ROUTE->route[i].name, len) ; ++i);
	if(i >= MAX_ROUTE){
		PlaySG1(SG1_CLEAR); // �x����
		if(DL_DISP()) Puts("ERROR! Route Overflow");
		return -1; // �o�b�t�@�s��
	}

	// ���[�g�ύX/�ǉ�
	Route* dst = &ROUTE->route[i];
	if(i == ROUTE->route_count){
		ROUTE->route_count++;
		memset (dst->name, 0, ROUTE_NAMESIZE);
		strncpy(dst->name, rh, len);
	}
	if(DL_DISP()) PutsNameB(dst->name);

	// ���[�g������
	dst->count = 0;
	dst->dist = 0;

	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ���[�g/WPT�ύX�t���O���Z�b�g
	return i; // ����RteHdr������܂ŁA���̃��[�g�ɃE�F�C�|�C���g��ǉ�����
}

// ���[�g�E�F�C�|�C���g�̒ǉ�
u32 AddRteWpt(u8* wpt, u32 len){
	// �T�C�Y�`�F�b�N
	if(IW->gi.dl_route == -1 || IW->gi.dl_route >= ROUTE->route_count) return -1; // ���[�g���ǉ��ł��Ȃ�
	Route* dst = &ROUTE->route[IW->gi.dl_route];
	if(dst->count >= MAX_ROUTE_PT) return -2; // ����ȏニ�[�g�E�F�C�|�C���g��ǉ��ł��Ȃ�

	u16 cyl = 0;
	u32 i = AddWpt(wpt, len, &cyl);
	if(i == -1) return -3; // ����ȏ�E�F�C�|�C���g���ǉ��ł��Ȃ�

	// ���[�g�֒ǉ�
	if(dst->count){
		// ���̃f�[�^�҂��܂ł̊Ԃɑ��������v�Z���Ă���
		u32 len;
		CalcWptDist(&WPT->wpt[dst->py[dst->count - 1].wpt],  &WPT->wpt[i], &len, 0);
		dst->dist += len;
	}
	Pylon* py = &dst->py[dst->count++];
	py->wpt = i;
	if(cyl == IW->tc.cylinder) py->cyl = -1; // �f�t�H���g
//	else if(cyl == 0xffff)     py->cyl =  0; // -1���w�肳�ꂽ�ꍇ��0m���g��
	else                       py->cyl =  cyl;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// �g���b�N�_�E�����[�h
///////////////////////////////////////////////////////////////////////////////
u32 AddTrack(u8* trk, u32 len){
	// �i����
	if(DL_DISP()){
		Putsf("%4.12m%d/%d", IW->gi.dl_count, IW->gi.dl_num);
		if(IW->gi.dl_num) Putsf("(%d%%)", RoundDiv(IW->gi.dl_count * 100, IW->gi.dl_num));
		Puts(": ");
	}
	if(len < 6) return -1; // ���肦�Ȃ��T�C�Y

	TrackLog tl;
	u32 new_trk = 0;
	// �g���b�N�t�H�[�}�b�g����
	// �f�o�C�XID����D30?�t�H�[�}�b�g��I�����Ă��ǂ����AGPS��r���ŕς����ꍇ���l�����ăf�[�^�T�C�Y�Ŏ������ʂ���B
	// ��������Ă��Ȃ��g���������A�r���ő���GPS�ɍ����ւ��Ďg�����Ƃ����肤��̂Łc�B
	if(len >= sizeof(D301_Trk_Point_Type)){
		if(DL_DISP()) Puts("D301");
		D301_Trk_Point_Type* d301 = (D301_Trk_Point_Type*)trk;
		tl.val[0] = MyRound(GetLong(d301->lat) * D10XLAT);
		tl.val[1] = MyRound(GetLong(d301->lon) * D10XLAT);
		tl.val[2] = MyRound(GetFloat(d301->alt));
		tl.val[3] = GetLong(d301->dtime) + IW->tc.tzone_m * 60;
		new_trk = d301->new_trk;
	} else if(len >= sizeof(D300_Trk_Point_Type)){
		if(DL_DISP()) Puts("D300");
		D300_Trk_Point_Type* d300 = (D300_Trk_Point_Type*)trk;
		tl.val[0] = MyRound(GetLong(d300->lat) * D10XLAT);
		tl.val[1] = MyRound(GetLong(d300->lon) * D10XLAT);
		tl.val[2] = 0;
		tl.val[3] = GetLong(d300->dtime) + IW->tc.tzone_m * 60;
		new_trk = d300->new_trk;
	} else {
		if(DL_DISP()) Puts("Unknown format");
		return -2;
	}

	if(IW->gi.dl_count == 1 || new_trk){
		IW->mp.tlog.abs_flag |= ABSF_SEPARETE; // �Z�p���[�^�}��
		IW->gi.dl_route++; // �g���b�N�ԍ��\���p
	}
	TrackLogAdd(&tl);
	if(DL_DISP()) Putsf("%4.14mTrack#%d", IW->gi.dl_route);
	return -4; // �ۗ�
}


///////////////////////////////////////////////////////////////////////////////
// Garmin �����N�v���g�R�� TX
///////////////////////////////////////////////////////////////////////////////
// �L�����N�^���M
void SioSend(u8 ch){
	u16 next = NextIndex(IW->tx_tail, sizeof(IW->tx_buf));
	if(next != IW->tx_head){ // �ꉞ�`�F�b�N
		IW->tx_buf[IW->tx_tail] = ch;
		IW->tx_tail = next;
	}
	// �f�o�b�O�p�_���v(route�̈悪���g�p�̏ꍇ�̂�)
	u32* tx_len = &IW->mp.last_route_len;
	if(!*tx_len || (*tx_len & DBGDMP_COM)){
		if((*tx_len & 0xfff) >= 256) *tx_len = 0;
		IW->mp.last_route[(*tx_len)++] = ch;
		*tx_len |= DBGDMP_COM;
	}
}

// DLE�X�^�b�t�B���O
void SioSendX(u8 ch){
	if(ch == DLE) SioSend(ch);
	SioSend(ch);
}

#define START_TX_FIFO_INT() (REG32(REG_TM2D) = 0x00c0ffff) 
// LP���M
EStatus LpSend(u8 pid, const u8* buf, u8 len){
	// ���M�G���[�`�F�b�N
//	if(IW->tx_head != IW->tx_tail)         return ERROR_SENDING; // ACK����̂��ߑ��d�]���̓K�[�h����
	if((u16)len * 2 >= sizeof(IW->tx_buf)) return ERROR_TOO_BIG_PACKET;

	// �w�b�_���M
	SioSend (DLE);
	SioSend (pid);
	SioSendX(len);
	pid += len; // �ȍ~pid�̓`�F�b�N�T���Ɏg�p

	// �f�[�^���M
	while(len--){
		pid += *buf;
		SioSendX(*buf++);
	}

	// �g���C�����M
	SioSendX(-pid); // �`�F�b�N�T��(2�̕␔)
	SioSend (DLE);
	SioSend (ETX);
	START_TX_FIFO_INT(); // FIFO���M���荞�݊J�n
	return ERROR_SUCCESS;
}

// ACK��2byte�B(GPS60��2byte ACK�����󂯕t���Ȃ��B����1byte/2byte�ǂ���ł�OK�Ȃ̂�2byte�œ���)
static inline EStatus LpSendAck(u16 pid){
	return LpSend(Pid_Ack_Byte, (u8*)&pid, 2);
}
static inline EStatus LpSendNak(u16 pid){
	return LpSend(Pid_Nak_Byte, (u8*)&pid, 2);
}
EStatus LpSendCmd(u16 cmd){
	return LpSend(Pid_Command_Data, (u8*)&cmd, 2);
}

// NMEA�f�o�C�X�p�R�}���h���M
extern const u8 V_HEX[];
EStatus NMEASend(const u8* buf){
	// START���M
	SioSend('$');

	// �f�[�^���M
	u8 sum = 0;
	while(*buf){
		SioSend(*buf);
		sum ^= *buf++;
	}

	// SUM���M
	SioSend('*');
	SioSend(V_HEX[sum >> 4]);
	SioSend(V_HEX[sum & 0xf]);
	SioSend('\r');
	SioSend('\n');

	START_TX_FIFO_INT(); // FIFO���M���荞�݊J�n
	return ERROR_SUCCESS;
}

// AT�R�}���h���M(Bluetooth���j�b�g�p)
EStatus ATCSend(const u8* buf, u32 max){
	// �f�[�^���M
	while(*buf && max--) SioSend(*buf++); // "+++"�G�X�P�[�v�͓��ɂ��Ȃ�
	START_TX_FIFO_INT(); // FIFO���M���荞�݊J�n
	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// �����N�v���g�R�� RX
///////////////////////////////////////////////////////////////////////////////
//��M�����p�P�b�g�̊m�F�B0������
EStatus CheckPacket(){
	// �T�C�Y�`�F�b�N
	u32 len = IW->dl_size;
	if(len < 3 || IW->dl_buf[1] != len - 3){
		return ERROR_LENGTH;
	}

	u8 sum = 0;
	while(len--) sum += IW->dl_buf[len];
	return sum? ERROR_CHECKSUM : ERROR_SUCCESS;
}

s32 GetHex(u8 ch){
	if(ch <  '0') return -1;
	if(ch <= '9') return ch - '0';
	ch |= 0x20;// tolower...
	if(ch <  'a') return -1;
	if(ch <= 'f') return ch - 'a' + 10;
	return -1;
}

///////////////////////////////////////////////////////////////////////////////
// �f�[�^�����N�v���g�R���^�X�N
///////////////////////////////////////////////////////////////////////////////
s32 MT_DataLink(){
	// FSM�`�F�b�N
	if(IW->dl_fsm >= DL_FSM_COMPLETE){
		 // �A�v�������҂�
		if(IW->rx_head != IW->rx_tail) IW->intr_flag = 1;// ��M�f�[�^����B�[�����荞�݃Z�b�g
		return 0;
	}
	// UART�G���[�`�F�b�N
	if(IW->uart_error || IW->rx_drop){
		IW->dl_drop += InterlockedExchange(&IW->rx_drop, 0);
		IW->dl_drop_pkt++;
		if(InterlockedExchange(&IW->uart_error, 0)){
			IW->dl_fsm = DL_FSM_E_UART; // ������̃G���[���D��
		} else {
			IW->dl_fsm = DL_FSM_E_DROP;
		}
		return 0;
	}

	// ��M�f�[�^�̊m�F
	while(IW->rx_head != IW->rx_tail){
		// �f�o�b�O�p�_���v(wpt�̈悪���g�p�̏ꍇ�̂�)
		u8 ch = IW->rx_buf[IW->rx_head];
		u32* rx_len = &IW->mp.last_wpt_len;
		if(!*rx_len || (*rx_len & DBGDMP_COM)){
			if((*rx_len & 0xfff) >= 256) *rx_len = 0;
			IW->mp.last_wpt[(*rx_len)++] = ch;
			*rx_len |= DBGDMP_COM;
		}

		// �p�P�b�g�o�b�t�@�T�C�Y�m�F
		if(IW->dl_size >= sizeof(IW->dl_buf)){
			// �o�b�t�@�s��!?
			IW->dl_fsm = DL_FSM_E_PACKET;
			IW->dl_drop_pkt++;
			IW->rx_head = IW->rx_tail;
			return 0;
		}

		// ��M�f�[�^����
		IW->rx_head = NextIndex(IW->rx_head, sizeof(IW->rx_buf));
		switch(IW->dl_fsm){
		case DL_FSM_WAIT_DLE:
			 // GARMIN/NMEA��������
			if(ch == DLE) IW->dl_fsm = DL_FSM_WAIT_PID; // GARMIN
			if(ch == '$') IW->dl_fsm = DL_FSM_NMEA_SUM; // NMEA
			continue; // DLE�̓p�P�b�g�ɒǉ����Ȃ�

		case DL_FSM_WAIT_PID:
			// �V�[�P���X�G���[�`�F�b�N
			if(ch == DLE){ // �\�����Ȃ���DLE
				IW->dl_drop_pkt++;
				continue; // ����ch��擪DLE�Ƃ��čē���
			}
			if(ch == ETX){ // �\�����Ȃ��I�[
				IW->dl_drop_pkt++;
				IW->dl_fsm = DL_FSM_WAIT_DLE; // �擪DLE�֍ē���
				IW->dl_size = 0;
				continue;
			}
			// ����Ɏ�M
			IW->dl_fsm = DL_FSM_WAIT_ANY;
			break;

		case DL_FSM_WAIT_ANY:
			if(ch != DLE) break; // ��M�p��
			IW->dl_fsm = DL_FSM_GET_DLE;
			continue; // ����ch�ŏ��������߂�

		case DL_FSM_GET_DLE:
			if(ch == DLE){ // DLE�X�^�b�t�B���O
				IW->dl_fsm = DL_FSM_WAIT_ANY;
				break;
			}
			if(ch == ETX){ // ��M����
				if(CheckPacket()){
					// �p�P�b�g�ɓ��
					IW->dl_drop_pkt++;
					IW->dl_fsm = DL_FSM_WAIT_DLE; // �擪DLE�֍ē���
					IW->dl_size = 0;
					LpSendNak(IW->dl_buf[0]);
					continue;
				}
				// �����M(ACK����)
				switch(IW->dl_buf[0]){
				case Pid_Ack_Byte:
				case Pid_Nak_Byte:
				case Pid_Pvt_Data:
					break; // ACK�s�v
				default:
					LpSendAck(IW->dl_buf[0]);
				}
				IW->dl_fsm = DL_FSM_COMPLETE;
				return 0;
			}
			// ���O��DLE�͐擪DLE�Ƃ��čē���(����ch��PID)
			IW->dl_drop_pkt++;
			IW->dl_fsm = DL_FSM_WAIT_ANY;
			break;

		// �ȍ~�ANMEA-0183�Ή�
		case DL_FSM_NMEA_SUM:
			// ACK�`�F�b�N(ACK�ɂ̓`�F�b�N�T�����Ȃ�)
			if(ch == '\r' || ch == '\n'){
#define ACK_ID_OFFSET 9
				if(IW->dl_size > ACK_ID_OFFSET && !strnicmp(IW->dl_buf, "ACK", 3)){
					IW->dl_buf[IW->dl_size] = 0;
					IW->dl_nmea_ack = strtoul(IW->dl_buf + ACK_ID_OFFSET, 0, 10);
					IW->dl_fsm = DL_FSM_WAIT_DLE;
					IW->dl_size = 0;
					continue;
				}
			}
			// �����L�����N�^�����o������DLE�ē���
			if(!IsHalf(ch)){
				IW->dl_fsm = (ch == DLE)? DL_FSM_WAIT_PID : DL_FSM_WAIT_DLE;
				IW->dl_drop_pkt++;
				IW->dl_size = 0;
				continue;
			}
			// �Z���e���X���̗\�����Ȃ��J�n�}�[�N���ē���
			if(ch == '$'){
				IW->dl_drop_pkt++;
				IW->dl_size = 0;
				continue;
			}
			// �`�F�b�N�T�����B
			if(ch == '*'){
				IW->dl_fsm = DL_FSM_NMEA_CR;
				IW->dl_nmea_chk = 0;
				IW->dl_nmea_idx = 0;
				continue;
			}
			break;

		case DL_FSM_NMEA_CR:
			if((ch == '\r' || ch == '\n') && IW->dl_nmea_idx == 2){ // �K��2��+CR
				// �T���`�F�b�N
				u8 sum = 0;
				u8* p = IW->dl_buf + IW->dl_size;
				while(p != IW->dl_buf) sum ^= *--p;
				if(sum == IW->dl_nmea_chk){
					// ����p�P�b�g��M
					IW->dl_fsm = DL_FSM_NMEA_END;
					return 0; // ����
				}
				// �T���G���[�B�ē����ŕ���
			}
			// �\�����Ȃ��J�n�}�[�N
			if(ch == '$' || ch == DLE){
				IW->dl_fsm = (ch == '$')? DL_FSM_NMEA_SUM : DL_FSM_WAIT_PID; // NMEA/GARMIN�ōē���
			} else {
				// �`�F�b�N�T���擾
				s32 hex = GetHex(ch);
				if(hex != -1){
					IW->dl_nmea_chk = IW->dl_nmea_chk * 0x10 + hex;
					IW->dl_nmea_idx++; // 2�ȏ�̓G���[�ɂȂ邪�A�����ł̓X���[�B
					continue; // �p��
				}
				IW->dl_fsm = DL_FSM_WAIT_DLE;
			}
			IW->dl_drop_pkt++;
			IW->dl_size = 0;
			continue;

		// �ȍ~�ABluetooth AT�R�}���h����p
		case DL_FSM_ATC_WAIT:
			if(ch == '\r' || ch == '\n'){
				IW->dl_fsm = DL_FSM_ATC_ANY;
				IW->dl_size = 0;
			}
			continue;
		case DL_FSM_ATC_ANY:
			if(ch == '\r' || ch == '\n'){
				if(!IW->dl_size) continue; // �L���f�[�^�Ȃ�
				IW->dl_buf[IW->dl_size] = 0;
				IW->dl_fsm = DL_FSM_ATC_END;
				return 0; // ����
			}
			if(!IsHalf(ch)){
				// �ُ�L�����N�^�����o
				IW->dl_fsm = DL_FSM_ATC_WAIT; // ���s�҂��ōē���
				continue;
			}
			break;
		}

		// �����o�b�t�@�֒ǉ�
#define DL_BUF_END (sizeof(IW->dl_buf) - 1)
		if(IW->dl_size < DL_BUF_END) IW->dl_buf[IW->dl_size++] = ch;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// NMEA-0183�f�R�[�_
///////////////////////////////////////////////////////////////////////////////
// x.y "."��؂��2������Ԃ�
char* NMEA_P_10(char* p, s32* a1, s32* a2, s32* a2_len){
	if(*p == ',') return 0; // �擪','�͋�f�[�^
	s32 sign = (*p != '-')? 1 : -1; // a2�p
	s32 v1 = strtol(p, &p, 10), v2 = 0, v2_len = 0;
	if(*p == '.'){
		char* pre = ++p;
		v2 = strtol(p, &p, 10) * sign;
		v2_len = p - pre;
	}
	if(*p != ',') return 0; // ���I�[�ł͂Ȃ�

	// ����t�B�[���h�ł��邱�Ƃ��m�F��Ɉ������A�b�v�f�[�g
	*a1 = v1;
	if(a2){
		*a2     = v2;
		*a2_len = v2_len;
	}
	return p + 1;
}

// �Œ菭��(1000�{)�Ƃ��Ė߂�
char* NMEA_P_10K(char* p, s32* v){
	s32 v1, v2, len;
	p = NMEA_P_10(p, &v1, &v2, &len);
	if(!p) return 0;

	// ����t�B�[���h�ł��邱�Ƃ��m�F��Ɉ������A�b�v�f�[�g
	*v = v1 * 1000;
	if(len < 4) *v += v2 * INT_POS_TABLE[3 - len];
	else        *v += RoundDiv(v2,  INT_POS_TABLE[len - 3]);
	return p;
}

// �ܓx/�o�x�̓x����Ԃ�
char* NMEA_P_LatLon(char* p, s32* v){
	s32 v1, v2, len;
	p = NMEA_P_10(p, &v1, &v2, &len);
	if(!p || p[1] != ',') return 0; // p[0]��'NESW'�̂����ꂩ�����A����͌�Ń`�F�b�N

	// ����t�B�[���h�ł��邱�Ƃ��m�F��Ɉ������A�b�v�f�[�g
	s32 d = BiosDiv(v1, 100, &v1);
	*v  = d * (60 * 60 * 1000) + v1 * (60 * 1000);
	if(len < 5) *v += v2 * 6 * INT_POS_TABLE[4 - len];
	else        *v += RoundDiv(v2 * 6,  INT_POS_TABLE[len - 4]);
	return p;
}

// �ȗ��t�B�[���h
char* NMEA_Nop(char* p){
	p = strchr(p, ',');
	return p? (p + 1) : 0;
}
// NOP x10
char* NMEA_Nop10(char* p){
	int i = 10;
	while(i-- && (p = NMEA_Nop(p)) != 0);
	return p? p : 0;
}

// ���x "xx.xx" + "M" (�C����WGS84�ȉ~�̍��x��gf�I��)
char* NMEA_P_Alt(char* p, s32 f){
	if(!f){
		// ���x��2�t�B�[���h�����X�L�b�v
		p = NMEA_Nop(p);
		if(p) p = NMEA_Nop(p);
		return p;
	}
	p = NMEA_P_10K(p, &f);
	if(!p || p[0] != 'M' || p[1] != ',') return 0;

	// ����t�B�[���h�ł��邱�Ƃ��m�F��Ƀp�����[�^���A�b�v�f�[�g
	IW->px2.alt_mm = f;
	return p + 2;
}

///////////////////////////////////////////////////////////////////////////////
// �t�B�[���h��͗p�R�[���o�b�N�֐�
typedef char* (*NMEAProc)(char*);

// UTC "HHMMSS.sss"
char* NMEA_Time(char* p){
	s32 v1;
	p = NMEA_P_10(p, &v1, 0, 0);
	if(!p) return 0;

	PvtX* px = &IW->px2;
	px->hour = BiosDiv(v1, 10000, &v1);
	px->min  = BiosDiv(v1,   100, &v1);
	px->sec  = v1;
	return p;
}
// �ܓx "DDDMM.mmmm" + [N/S]
char* NMEA_Lat(char* p){
	s32 v;
	p = NMEA_P_LatLon(p, &v);
	if(!p) return 0;
	if(*p == 'S') v = -v;
	else if(*p != 'N') return 0;

	IW->px2.lat = v;
	return p + 2;
}
// �o�x "DDDMM.mmmm" + [E/W]
char* NMEA_Lon(char* p){
	s32 v;
	p = NMEA_P_LatLon(p, &v);
	if(!p) return 0;
	if(*p == 'W') v = -v;
	else if(*p != 'E') return 0;

	IW->px2.lon = v;
	return p + 2;
}
// ���ʏ� [1|2|3]
char* NMEA_Status(char* p){
	s32 v;
	p = NMEA_P_10(p, &v, 0, 0);
	if(!p) return p;

	switch(v){
	case 1:	IW->px2.fix = FIX_INVALID;	break;
	case 2: IW->px2.fix = FIX_2D;		break;
	case 3: IW->px2.fix = FIX_3D;		break;
	default:IW->px2.fix = FIX_UNUSABLE;	break;
	}
	return p;
}
// ���ʏ�2 [0|1]
char* NMEA_Valid(char* p){
	// ��{�I�ɂ́A�����̑���NMEA_Status�𑸏d���邪�AGSA��5�b�Ԋu�ł���
	// ����Ȃ�GPS�����邽�߁AGGA�ł�Valid�`�F�b�N�����͂���
	s32 v;
	p = NMEA_P_10(p, &v, 0, 0);
	if(!p) return p;

	// Invalid/Valid���Ⴄ�ꍇ�����ݒ肷��
	if(v){
		if(IW->px2.fix <  FIX_2D) IW->px2.fix = FIX_2D;
	} else {
		if(IW->px2.fix >= FIX_2D) IW->px2.fix = FIX_INVALID;
	}
	return p;
}
// �q���⑫��(PvtX�ɂ͊܂܂�Ȃ��������p�Ɏ擾���Ă���) "xx"
char* NMEA_SatNum(char* p){
	return NMEA_P_10(p, &IW->nmea_log[NMEA_LOG_ID_SAT_NUM], 0, 0); // �f�o�b�O�p
}

// ���ʗ򉻌W�� "xx.xx"
char* NMEA_PDOP(char* p){
	return NMEA_P_10K(p, &IW->px2.epe_mm); // P-DOP��epe�Ɋi�[���Ă���
}
char* NMEA_HDOP(char* p){
	return NMEA_P_10K(p, &IW->px2.eph_mm); // H-DOP��eph�Ɋi�[���Ă���
}
char* NMEA_VDOP(char* p){
	return NMEA_P_10K(p, &IW->px2.epv_mm); // V-DOP��epv�Ɋi�[���Ă���
}

// ���x "xx.xx" + "M"
char* NMEA_Alt(char* p){
	return NMEA_P_Alt(p, !IW->tc.alt_type); // �C�����x + [M]
}
char* NMEA_Geo(char* p){			
	return NMEA_P_Alt(p,  IW->tc.alt_type); // ���ϊC���ʍ��x + [M]
}

// ���x[Knot]
char* NMEA_Knot(char* p){
	s32 v;
	p = NMEA_P_10K(p, &v);
	if(!p) return 0;
	if(v < 9999990){ // ������("9999.99")�͒l���X�V���Ȃ�
		// �ő�1000.000,N�Ɖ��肵10bit�V�t�g�Ōv�Z(1knot �� 0.5144444 m/s)
		IW->px2.vh_mm = (v * 527) >> 10;
	}
	return p;
}

// ����
char* NMEA_Dir(char* p){
	s32 v;
	p = NMEA_P_10K(p, &v);
	if(!p) return 0;
	if(v < 999990){ // ������("999.99")�͒l���X�V���Ȃ�
		// �ő�360.000�Ȃ̂�15bit�V�t�g�Ōv�Z
		IW->px2.h_ang64 = (v * 5965 ) >> 15;
	}
	return p;
}
//DDMMYYYY
char* NMEA_Date(char* p){
	s32 v;
	p = NMEA_P_10(p, &v, 0, 0);
	if(!p) return 0;

	PvtX* px = &IW->px2;
	px->day   = BiosDiv(v, 10000, &v);
	px->month = BiosDiv(v,   100, &v);
	px->year  = ((v < 90)? 2000 : 1900) + v;
	return p;
}
/*NMEA_Date�ł܂Ƃ߂Ď擾����̂ŕs�v
char* NMEA_Day(char* p){
	return NMEA_P_10(p, &IW->px2.day, 0, 0);
}
char* NMEA_Month(char* p){
	return NMEA_P_10(p, &IW->px2.month, 0, 0);
}
char* NMEA_Year(char* p){
	return NMEA_P_10(p, &IW->px2.year, 0, 0);
}
*/

const NMEAProc NMEA_SEQ_GGA[] = {
	NMEA_Time,	// hhmmss.sss
	NMEA_Lat,	// ddmm.mmmm,N/S
	NMEA_Lon,	// ddmm.mmmm,E/W
	NMEA_Valid, // ���ʏ��[0/1]
	NMEA_SatNum,// SatNum
	NMEA_HDOP,	// hh.h
	NMEA_Alt,	// hhhhh.h,M
	NMEA_Geo,	// ggggg.g,M
//	NMEA_Nop,	// DGPS_AGE,
//	NMEA_Nop,	// DGPS_ID,
	0
};
const NMEAProc NMEA_SEQ_GSA[] = {
	NMEA_Nop,	// ModeA/B (NMEA_Status�ȊO�͎g��Ȃ�)
	NMEA_Status,// ���ʏ��
	NMEA_Nop10,	// Sat
	NMEA_Nop,	// Sat
	NMEA_Nop,	// Sat
	NMEA_PDOP,	// pp.p
	NMEA_HDOP,	// hh.h
	NMEA_VDOP,	// vv.v
//	NMEA_Nop,	// StatusA
	0
};
const NMEAProc NMEA_SEQ_GSV[] = {
	// �g��Ȃ��c
	0
};
const NMEAProc NMEA_SEQ_RMC[] = {
	NMEA_Time,	// hhmmss.sss
	NMEA_Nop,	// Status A/V (NMEA_Status�ȊO�͎g��Ȃ�)
	NMEA_Lat,	// ddmm.mmmm,N/S
	NMEA_Lon,	// ddmm.mmmm,W/E
	NMEA_Knot,	// ssss.ss
	NMEA_Dir,	// hhh.hh
	NMEA_Date,	// ddmmyy
//	NMEA_Nop,	// NDiff
//	NMEA_Nop,	// NDiff_U
//	NMEA_Nop,	// ModeA
	0
};
const NMEAProc NMEA_SEQ_VTG[] = {
	// �g��Ȃ��c
//	NMEA_Dir,
//	NMEA_Nop,	// T
//	NMEA_Nop,	// MagN Dir
//	NMEA_Nop,	// M
//	NMEA_Knot,
//	NMEA_Nop,	// N
//	NMEA_Nop,	// km/h
//	NMEA_Nop,	// K
//	NMEA_Nop,	// ModeA
	0
};
const NMEAProc NMEA_SEQ_ZDA[] = {
	// �g��Ȃ��c
//	NMEA_Time,
//	NMEA_Day,
//	NMEA_Month,
//	NMEA_Year,
//	NMEA_Nop,	// Hour
//	NMEA_Nop,	// Min
	0
};
const NMEAProc NMEA_SEQ_GLL[] = {
	// �g��Ȃ��c
//	NMEA_Lat,
//	NMEA_Lon,
//	NMEA_Time,
//	NMEA_Nop,	// StatusA
//	NMEA_Nop,	// ModeA
	0
};
const NMEAProc NMEA_SEQ_ALM[] = {
	// �g��Ȃ��c
	0
};
//GPS-52D: GGA��GSA��GSV��RMC��VTG��ZDA�i1�b�Ԋu�j
const NMEAProc* const NMEA_SEQ_TABLE[] = {
	NMEA_SEQ_GGA,
	NMEA_SEQ_GSA,
	NMEA_SEQ_GSV,
	NMEA_SEQ_RMC,
	NMEA_SEQ_VTG,
	NMEA_SEQ_ZDA,
	NMEA_SEQ_GLL,
	NMEA_SEQ_ALM,
};

s32 NMEADecode(){
	u8* p = IW->dl_buf;

	// NMEA�Z���e���X���f�o�b�O�p�ɕۑ�
	if(IW->tc.log_debug == 1){
		u32 AppendNMEALog();
		AppendNMEALog();
	}
	
	// talker�`�F�b�N
	if(p[0] != 'G' || p[1] != 'P'){
		IW->nmea_log[NMEA_LOG_ID_TALKER_ERR]++;
		return 1; // GPS����̃R�}���h�ȊO�͖���
	}

	// �����NMEA�f�o�C�X���ڑ����ꂽ���Ƃ��L�^����
	if(IW->gi.pid != NMEA_DEV_ID){
		IW->gi.pid = NMEA_DEV_ID;
		IW->gi.version = 0;
		strcpy(IW->gi.name, "NMEA");
	}
	IW->gi.timeout = 0; // NMEA�̉���������Ԃ̓^�C���A�E�g���K�[�h����
	if(IW->px.fix == FIX_UNUSABLE) IW->px.fix = FIX_INVALID; 

	// NMEA ID�ʏ���
	u32 id = NMEA_LOG_ID_UNKNOWN;
	switch(GetLong(p + 2)){
	case 0x2C414747: id = NMEA_LOG_ID_GGA; break;
	case 0x2C415347: id = NMEA_LOG_ID_GSA; break;
	case 0x2C565347: id = NMEA_LOG_ID_GSV; break;
	case 0x2C434D52: id = NMEA_LOG_ID_RMC; break;
	case 0x2C475456: id = NMEA_LOG_ID_VTG; break;
	case 0x2C41445A: id = NMEA_LOG_ID_ZDA; break;
	case 0x2C4C4C47: id = NMEA_LOG_ID_GLL; break;
	case 0x2C4D4C41: id = NMEA_LOG_ID_ALM; break;
	}

	// �f�o�b�O�p���O
	IW->nmea_log[id]++;
	if(id == NMEA_LOG_ID_UNKNOWN) return 2;
	if(id == NMEA_LOG_ID_GGA){ // GGA��M�Ńo�b�t�@�擪����l�ߒ���
		IW->mp.last_pvt_len = DBGDMP_COM; 
		IW->mp.last_pvt_len = DBGDMP_COM;
	}
	s32 len = IW->dl_size;
#define NMEA_DBG_BUF 512 // PVT�̌����A�����Ďg��
	if(IW->mp.last_pvt_len + len > NMEA_DBG_BUF) len = NMEA_DBG_BUF - IW->mp.last_pvt_len;
	if(len > 0){
		memcpy(IW->mp.last_pvt + (IW->mp.last_pvt_len & (NMEA_DBG_BUF - 1)), p, len);
		IW->mp.last_pvt_len += len;
	}

	// �t�B�[���h���
	strcpy(p + IW->dl_size, ","); // �I�[�}�[�N���Ă���
	p += 6; // �w�b�_�X�L�b�v
	const NMEAProc* fn = NMEA_SEQ_TABLE[id];
	for(; *fn ; ++fn){
		if(!(p = (*fn)(p))){
			IW->nmea_log[NMEA_LOG_ID_FIELD_ERR]++;
			return 3; // �G���[
		}
	}

	// �t�B�[���h�����`�F�b�N
#define NMEA_COMPLETE	((1 << NMEA_LOG_ID_GGA) | (NMEA_LOG_ID_RMC)) // GGA/RMC��OK(GSA�̓I�v�V��������)
#define NMEA_SYNC_ID	NMEA_LOG_ID_RMC // RMC�����������Ɏg��(RMC���Ō�ł͂Ȃ��ꍇ�́c?)
#define WGS84_TIMEOUT	3				// GPS52D�ȊO���ڑ�����Ă���ꍇ���l�����āA�����ԂŃ^�C���A�E�g����
	IW->nmea_fix |= 1 << id; // TID�P�ʂŃ`�F�b�N�B�t�B�[���h�P�ʂ��ǂ�?
	if(id == NMEA_SYNC_ID && (IW->nmea_fix & NMEA_COMPLETE) == NMEA_COMPLETE){
		// GPS52D�̓f�t�H���g��WGS84�ł͂Ȃ��I�f�[�^����M�ł����R�R�Ń��[�h�ؑ֎��s
		if(IW->nmea_wgs84 < WGS84_TIMEOUT){
			if(IW->dl_nmea_ack == 106){
				// �؂�ւ�����
				IW->nmea_wgs84 = WGS84_TIMEOUT + 1; // �f�o�b�O�p�Ƀ^�C���A�E�g�Ɗ�������ʂł���悤�ɂ��Ă���
			} else {
				// WGS84�ɐ؂�ւ����s (����ɂ��Ă��A�f�t�H���g��WGS84�łȂ�GPS���܂�����Ƃ́c)
				NMEASend("PSRF106,21"); // ACK���Ԃ�܂�1�b��1��đ�����
				IW->nmea_wgs84++;
			}
		}

		// �f�[�^���g�p����̂�WGS84���[�h�ؑ֌�@(106�������Ȃ�GPS�̏ꍇ�͍ŏ���3�p�P�b�g�𖳎����邱�ƂɂȂ邪�c)
		if(IW->nmea_wgs84 >= WGS84_TIMEOUT) CalcPvtNMEA(&IW->px); 
		IW->nmea_fix = 0;
	}
	
	return 0; // success
}

///////////////////////////////////////////////////////////////////////////////
// Bluetooth AT�R�}���h���� (for ParaniESD100/110/200/210)
///////////////////////////////////////////////////////////////////////////////
u32 IsBluetoothAddr(const u8* p){
	const u8* end = p + 12;
	while(p < end) if(GetHex(*p++) < 0) return 0;
	return 1; // 12�����ׂ�Hex�Ȃ�Bluetooth�A�h���X�Ƃ���
}
void ATCCheck(){
	// AT���䒆��ul_acknak���g��Ȃ��̂ŁA������ATC���������Ă���
	if(!strnicmp(IW->dl_buf, "ok",          2)){
		IW->gi.ul_acknak = ATCRES_OK;
	} else if(!strnicmp(IW->dl_buf, "error",       5)){
		IW->gi.ul_acknak = ATCRES_ERROR;
	} else if(!strnicmp(IW->dl_buf, "connect",     7)){
		IW->gi.ul_acknak = ATCRES_CONNECT;
		PlaySG1(SG1_CONNECT);
	} else if(!strnicmp(IW->dl_buf, "disconnect", 10)){
		IW->gi.ul_acknak = ATCRES_DISCONNECT;
		PlaySG1(SG1_CLEAR);
	} else if(IsBluetoothAddr(IW->dl_buf) && IW->dl_buf[12] == ',' && ATC_PTR->inq_count < MAX_INQ_DEV){
		u8* mode = strstr(IW->dl_buf, ",MODE"); // MODE�Ŏn�܂�f�o�C�X�́c
		if(mode){
			// INFO�͂����Ŋi�[
			ATC_PTR->last_mode = mode[5] - '0'; 
		} else {
			// INQ���͂����Ŋi�[
			u8* src = IW->dl_buf;
			u8* dst = ATC_PTR->inq_list[ATC_PTR->inq_count++];
			u8* end = dst + MAX_ATC_RESPONSE;
			for(; dst < end && *src ; ++src, ++dst) *dst = IsHalf(*src)? *src : '_'; // �󎚕s�̃L�����N�^�̓G�X�P�[�v���Ă���
			*dst = 0;
			PlaySG1(SG1_COMP1);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Garmin�A�v���P�[�V�����v���g�R���^�X�N
///////////////////////////////////////////////////////////////////////////////

void GpsEmuMode(u32 mode, const u16* snd){
	if(snd) PlaySG1(snd);
	IW->gi.state    = mode;
	IW->gi.dl_count = 0;
	IW->gi.emu_nak  = 0;
}

s32 MT_AppLayer(){
	// ��ԑJ�ڊǗ�
	if(IW->dl_fsm < DL_FSM_COMPLETE){
		if(IW->px.rx_pre != IW->rx_tail){
			IW->px.rx_pre = IW->rx_tail;
			return 0;
		}
		return 0; // ���ɉ����Ȃ�
	}

	// NMEA�����͂����Ń`�F�b�N
	if(IW->dl_fsm == DL_FSM_NMEA_END){
		IW->mp.auto_off_cnt = IW->vb_counter;
		NMEADecode();
		IW->dl_fsm = DL_FSM_WAIT_DLE;
		IW->dl_size = 0;
		return 1; // ���L�����N�^�̃`�F�b�N
	}

	// Bluetooth AT�R�}���h����͂����Ń`�F�b�N
	if(IW->dl_fsm == DL_FSM_ATC_END){
		IW->mp.auto_off_cnt = IW->vb_counter;
		ATCCheck();
		IW->dl_fsm = DL_FSM_ATC_WAIT;
		IW->dl_size = 0;
		return 0;
	}

	if(IW->dl_fsm > DL_FSM_COMPLETE){
		IW->dl_fsm = (GPS_BT_BR_CHECK1 <= IW->gi.state && IW->gi.state <= GPS_BT_ERROR)? DL_FSM_ATC_WAIT : DL_FSM_WAIT_DLE;
		IW->dl_size = 0;
		return 0;
	}

	// �p�P�b�g����
	IW->mp.auto_off_cnt = IW->vb_counter;
	switch(IW->dl_buf[0]){
	// �V�[�P���X /////////////////////////////////////////
	case Pid_Ack_Byte:
	case Pid_Nak_Byte:
		IW->gi.ul_acknak = IW->dl_buf[0]; // ID�́c�Ƃ肠����������B
		break;

	// PVT ////////////////////////////////////////////////
	case Pid_Pvt_Data:
		if(IW->dl_buf[1] != sizeof(D800_Pvt_Data_Type) ||
			CalcPvtGarmin((D800_Pvt_Data_Type*)(IW->dl_buf + 2), &IW->px)){
			// �f�[�^�X�V���s
			IW->px.fix_sum[FIX_UNUSABLE]++;
			IW->mp.tlog.opt |= TLOG_OPT_LOST; // �f�R�[�h�G���[
		} else {
			IW->mp.test_mode = 0; // ��M������e�X�g���[�h���I������
		}
		// �f�o�b�O�p
		IW->mp.last_pvt_len = MinS32((u32)IW->dl_buf[1] + 2, sizeof(IW->mp.last_pvt));
		memcpy(IW->mp.last_pvt, IW->dl_buf, IW->mp.last_pvt_len);
		break;

	// ���i��� ///////////////////////////////////////////
	case Pid_Product_Data:
		if(IW->dl_buf[1] > 4){
			IW->gi.pid =     GetInt(IW->dl_buf + 2);
			IW->gi.version = GetInt(IW->dl_buf + 4);
			memcpy(IW->gi.name, IW->dl_buf + 6, IW->dl_buf[0] - 4);//�ő�251byte�Ȃ̂Ŏ���parray�ɂ͂ݏo���ꍇ�����邪�A���̍ۍ\��Ȃ�
			if(!memcmp(IW->gi.name, "ParaNavi", 8)){
				// GBA���ڑ����ꂽ?
				GpsEmuMode(GPS_EMUL_FINISH, 0); // ���ɂ�邱�Ƃ͖����̂�FINISH�ɓ���Ă���
			}
		}
		break;

	case Pid_Protocol_Array:
		// ����͎g��Ȃ��c
		break;

	// �_�E�����[�h���� ///////////////////////////////////
	case Pid_Records:
		if(IW->dl_buf[1] == 2){
			IW->gi.dl_num = GetInt(IW->dl_buf + 2);
			if(!IW->gi.dl_accept) IW->gi.dl_accept = 2; // PC�����GBA�փA�b�v���[�h���͔�\��
		}
		break;

	case Pid_Xfer_Cmplt:
		IW->gi.dl_count = -1;
		IW->gi.dl_accept &= ~2; // ���Ƃ��̂�2�̂݁B
		PlaySG1(SG1_CHANGE);
		break;

	// ���[�g /////////////////////////////////////////////
	case Pid_Rte_Hdr:
		if(IW->gi.dl_accept){
			IW->gi.dl_count++;
			IW->gi.dl_route = AddRteHdr(IW->dl_buf + 2, IW->dl_buf[1]);
			// �_�E�����[�h�͕����s����\�������邽�߁A�^�X�N�ւ̎����ݒ�͂��Ȃ�
			if(IW->gi.dl_route == IW->tc.route){
				IW->mp.navi_update |= NAVI_ROUTE_DL;
				IW->mp.pre_route = (Route*)-1;// �^�X�N���O��������
			}

			// �f�o�b�O�p
			IW->mp.last_route_len = MinS32((u32)IW->dl_buf[1] + 2, sizeof(IW->mp.last_route));
			memcpy(IW->mp.last_route, IW->dl_buf, IW->mp.last_route_len);
		}
		break;

	case Pid_Rte_Link_Data:
		if(IW->gi.dl_accept) IW->gi.dl_count++;
		break;

	// �E�F�C�|�C���g /////////////////////////////////////
	case Pid_Rte_Wpt_Data:
	case Pid_Wpt_Data:
		if(IW->gi.dl_accept){
			// �E�F�C�|�C���g�ǉ�
			IW->gi.dl_count++;
			if(IW->dl_buf[0] == Pid_Wpt_Data) AddWpt(IW->dl_buf + 2, IW->dl_buf[1], 0);
			else                              AddRteWpt(IW->dl_buf + 2, IW->dl_buf[1]);

			// �^�C���A�E�g���X�V
			if(IW->gi.dl_accept == 2) IW->gi.timeout = 0;

			// �f�o�b�O�p
			IW->mp.last_wpt_len = MinS32((u32)IW->dl_buf[1] + 2, sizeof(IW->mp.last_wpt));
			memcpy(IW->mp.last_wpt, IW->dl_buf, IW->mp.last_wpt_len);
		}
		break;

	// �g���b�N /////////////////////////////////////
	case Pid_Trk_Data:
		if(IW->gi.dl_accept){
			IW->gi.dl_count++;
			AddTrack(IW->dl_buf + 2, IW->dl_buf[1]);
		}
		break;
	case Pid_Trk_Hdr:
		// ���ɉ������Ȃ�
		break;

	// GPS�G�~�����[�V���� /////////////////////////////////////
	case Pid_Product_Rqst:
		// PC���炱�̃R�}���h���󂯂���GPS�G�~�����[�V�������[�h�ɓ���
		GpsEmuMode(GPS_EMUL_INFO, 0);
		break;

	case Pid_Command_Data:
		// �G�~�����[�V�������[�h�ŃT�|�[�g���Ă���R�}���h������
		switch(GetInt(IW->dl_buf + 2)){
		case Cmnd_Transfer_Wpt:
			GpsEmuMode(GPS_EMUL_WPT, SG1_MODE2);
			break;

		case Cmnd_Transfer_Rte:
			GpsEmuMode(GPS_EMUL_ROUTE, SG1_MODE3);
			break;

		case Cmnd_Transfer_Trk:
			if(IW->mp.tlog.tc_state != TC_COMPLETE){
				// �g���b�N���̃J�E���g���͉����ł��Ȃ��B(�R�}���h��t��ɃJ�E���g�J�n�����
				// �J�V�~�[�����^�C���A�E�g����̂ŁA�\�ߑ����𐔂��Ă����K�v������)
				PlaySG1(SG1_CANCEL);
				SelectMap(MAP_BG0);
				FillBox(6, 12, 23, 17);
				DrawTextCenter(14, "Please wait...");
				SetBootMsgTimeout(30);
				break;
			}
			memset(&IW->gi.tinfo, 0, sizeof(TrackInfo));
			IW->gi.tinfo.pre_sect = -1; // �����Z�N�^���w���Ă���
			IW->gi.tinfo.tpos     = IW->mp.tlog.tc_tpos;
			IW->gi.ul_track       = IW->mp.tlog.tc_total;
			IW->gi.ul_pre         = 0;
			GpsEmuMode(GPS_EMUL_TRACK, SG1_MODE4);
			break;

		case Cmnd_Start_Pvt_Data:
			// ���������APid_Product_Data��Ԃ��āA����ɐڑ��悪GBA�ł��邱�Ƃ�m�点��
			GpsEmuMode(GPS_EMUL_INFO, 0);
			break;
		}
		break;
	}

	IW->dl_fsm = DL_FSM_WAIT_DLE;
	IW->dl_size = 0;
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// GPS����^�X�N
///////////////////////////////////////////////////////////////////////////////

const u32 PINFO_TIMEOUT	= VBC_TIMEOUT(3);	// ���񂾂��Ȃ̂ŁA�p�ɂɃ`�F�b�N����
const u32 PVT_TIMEOUT	= VBC_TIMEOUT(10);	// PVT�͓r���Ŕ����Ă����A����̂ŁA���߂̃^�C���A�E�g
const u32 DL_TIMEOUT	= VBC_TIMEOUT(5);	// ���̑��������Ƃ��͊m���ɐڑ�����Ă���\��������?
const u32 EMUL_TIMEOUT	= VBC_TIMEOUT(10);	// ���̎��ԁA�V�R�}���h��������΃G�~�����[�^���[�h(��PC�ʐM)�𔲂��đ�GPS�ʐM�ɕ��A����
const u32 ACK_TIMEOUT	= VBC_TIMEOUT(1);	// �G�~�����[�^���[�h�̍ē]���͑��₩�ɍs��(PC�Ƃ̒ʐM�͕��ʐ؂�Ȃ�)
const u32 BTPPP_TIMEOUT	= VBC_TIMEOUT(3);	// Bluetooth�X�^���o�C�R�}���h�͑��₩�ɉ�������͂�
const u32 BTBOOT_TIMEOUT= VBC_TIMEOUT(4);	// Bluetooth���u�[�g�͒��߂ɑ҂�
const u32 BTCMD_TIMEOUT	= VBC_TIMEOUT(3);	// Bluetooth�̃R�}���h�����₩�ɉ�������͂�
const u32 BTINQ_TIMEOUT	= VBC_TIMEOUT(35);	// Bluetooth INQ�͍ő��30�b+���ҋ@����
const u32 BTATH_TIMEOUT	= VBC_TIMEOUT(35);	// Bluetooth ATH�͍ő��30�b+���ҋ@����
const u32 BTDIAL_TIMEOUT= VBC_TIMEOUT(305);	// Bluetooth��Dial�͍ő��5��+���ҋ@����

const u32 EMU_MAX_ERROR	= 5;				// �ʐM�ُ킪��������G�~�����[�^���[�h(��PC�ʐM)�𔲂��đ�GPS�ʐM�ɕ��A����

u32 CalcNameLenX(const u8* p, u32 max){
	u32 len = 0;
	while(*p && len < max) ++len; // �]���͔��p�ȊO���]��(IsHalf�̃`�F�b�N�͂��Ȃ�)
	return len;
}
#define CalcNameLen(n) (CalcNameLenX(n, sizeof(n)))

// PC�]���p�f�[�^���\�z�Bwd��indent�p�o�b�t�@�̂��ߔ{�p�ӂ��邱�ƁI
u32 MakeD108Wpt(D108_Wpt_Type wd[2], const Wpt* w, s32 cyl){
	// �e�������f�t�H���g(0)�Ŗ��߂�
	memset(wd, 0, sizeof(D108_Wpt_Type) * 2);

	// ���W�����Z�b�g
	SetLong (wd->lat, MyRound(w->lat * D10XLAT_i));
	SetLong (wd->lon, MyRound(w->lon * D10XLAT_i));
	SetFloat(wd->alt, w->alt);	// ���x�͕��������_�ϊ�

	// �V�����_�T�C�Y���Z�b�g (WPT/ROUTE�]���̎d�l�Ŗ���`�̃����o�����A�J�V�~�[���͂��̏����g���Ă����)
	if(cyl < 0) SetFloat(wd->dist, 1e25);
	else        SetFloat(wd->dist, cyl);

	// depth��ݒ�
	SetFloat(wd->dpth, 1e25);

	// attr�͎d�l�̂Ƃ���0x60���Z�b�g
	wd->attr = 0x60;

	// ���O�͉ϒ�
	u16 len = CalcNameLen(w->name); // �I�[NULL�̓C���i�C?
	strncpy(wd->ident, w->name, len); // wd[1]�̗̈���g�p

	// �\�z�����B�\�z����WPT�̃T�C�Y��Ԃ��B
	return sizeof(D108_Wpt_Type) + len + 6;
}

// �g���b�N�f�[�^�쐬
u32 MakeD301Trk(D301_Trk_Point_Type* tp){
	TrackInfo* ti = &IW->gi.tinfo;

	// �g���b�N�f�[�^�l�ߍ���
	SetLong (tp->lat,   MyRound((ti->val[0] << ti->prc) * D10XLAT_i));
	SetLong (tp->lon,   MyRound((ti->val[1] << ti->prc) * D10XLAT_i));
	SetLong (tp->dtime, ti->val[3] - IW->tc.tzone_m * 60);
	SetFloat(tp->alt,   ti->val[2]);

	SetFloat(tp->dpth, 1e25);
	tp->new_trk = ti->seg? 1 : 0;
	return sizeof(D301_Trk_Point_Type);
}

void Dec2Fill(u8* p, u32 v){
	BiosDiv(v, 100, &v);
	if(v < 100){
		p[0] = BiosDiv(v, 10, &v) + '0';
		p[1] = v + '0';
	} else {
		p[0] = p[1] = '*';
	}
}

// �g���b�N�f�[�^�w�b�_�쐬
const time_t UNIX2GARMIN = 631033200; // UNIX�b(1970)->GARMIN�b(1990)
u32 MakeD310TrkHead(u8* p){
	TrackInfo* ti = &IW->gi.tinfo;

	p[0] = 1; // dspl  �\��ON
	p[1] = 0; // color ��

	// ident�ɂ�"2000/01/02-03:45"�̂悤�ȓ��������Ă݂�
	struct tm utc;
	time_t sec = ti->val[3] + UNIX2GARMIN + IW->tc.tzone_m * 60;
	gmtime_r(&sec, &utc);
	u32 yL, yH = BiosDiv(utc.tm_year + 1900, 100, &yL);
	Dec2Fill(p +  2, yH);
	Dec2Fill(p +  4, yL);
	Dec2Fill(p +  7, utc.tm_mon + 1);
	Dec2Fill(p + 10, utc.tm_mday);
	p[6] = p[9] = '/';
	p[12] = '_';
	Dec2Fill(p + 13, utc.tm_hour);
	p[15] = ':';
	Dec2Fill(p + 16, utc.tm_min);
	p[18] = 0;

	return 19;
}


// FSM����p
void GpsCtrlWait(){
	IW->gi.vbc = IW->vb_counter;
	IW->gi.timeout = 0;
	IW->gi.state++;
	IW->gi.ul_acknak = 0;
}

s32 MT_GPSCtrl(){
	// �{�[���[�g��������ꍇ�͂����Őݒ�
	if(IW->mp.pre_bt_mode != IW->tc.bt_mode){
		IW->mp.pre_bt_mode = IW->tc.bt_mode;
		StartGPSMode(IW->tc.bt_mode);
	}
	switch(IW->gi.state){
	// ���i��� ///////////////////////////////////////////
	case GPS_GET_PINFO_WAIT: // ���i��񉞓��҂�
		if(IW->gi.pid){
			IW->gi.state = GPS_PVT;
			IW->px.pvt_timeout = 0;
			break;
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) < PINFO_TIMEOUT){ // �^�C���A�E�g����
			break;
		}
		IW->gi.state--;
		IW->dl_timeout++;
		if(!IW->mp.test_mode){
			IW->px.fix = FIX_UNUSABLE;
			IW->px.pvt_timeout = 1;
			IW->mp.navi_update |= NAVI_UPDATE_PVT | NAVI_PVT_CHANGE;
		}
		// no break;
	case GPS_GET_PINFO: // ���i���₢���킹
		if(IW->px.fix == FIX_UNUSABLE) LpSend(Pid_Product_Rqst, 0, 0);
		GpsCtrlWait();
		break;

	// PVT�擾 ////////////////////////////////////////////
	case GPS_PVT_WAIT:
		if(IW->gi.pre_pvt != IW->px.counter){
			IW->gi.pre_pvt = IW->px.counter;
			IW->gi.timeout = 0;
			break; // �X�e�[�g��PVT_WAIT�̂܂�
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) < PVT_TIMEOUT){ // 10�b�^�C���A�E�g����
			break;
		}
		IW->px.fix = FIX_UNUSABLE;
		IW->px.pvt_timeout = 1;
		IW->gi.state--;
		IW->dl_timeout++;
		IW->mp.tlog.opt |= TLOG_OPT_TIMEOUT; // �^�C���A�E�g�Ńg���b�N���O�ɃZ�O�����g�t���O��ݒ�
		IW->mp.navi_update |= NAVI_UPDATE_PVT| NAVI_PVT_CHANGE;

		// no break;
	case GPS_PVT:
		if(IW->px.fix == FIX_UNUSABLE) LpSendCmd(Cmnd_Start_Pvt_Data);
		GpsCtrlWait();
		break;

	// �_�E�����[�h���� ///////////////////////////////////
	case GPS_WPT_WAIT:
	case GPS_ROUTE_WAIT:
	case GPS_TRACK_WAIT:
		if(IW->gi.dl_count == -1){ // ����
			IW->gi.state = GPS_PVT_WAIT;
			break;
		} else if(IW->gi.pre_count != IW->gi.dl_count){
			IW->gi.pre_count = IW->gi.dl_count;
			IW->gi.timeout = 0;
			break;
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) < DL_TIMEOUT){ // 5�b�^�C���A�E�g����
			break;
		}
		IW->gi.state--;
		IW->dl_timeout++;
		break;

	case GPS_STOP_DOWNLOAD:
		// ���f�R�}���h
		LpSendCmd(Cmnd_Abort_Transfer);
		IW->gi.state = GPS_PVT_WAIT; // ���g���C�͖����c
		break;

	// �E�F�C�|�C���g�擾 /////////////////////////////////
	case GPS_WPT:
		// WPT�J�n
		LpSendCmd(Cmnd_Transfer_Wpt);
		IW->gi.dl_count = IW->gi.dl_num = 0;
		GpsCtrlWait();
		break;

	// ���[�g�擾 /////////////////////////////////////////
	case GPS_ROUTE:
		// Route�J�n
		LpSendCmd(Cmnd_Transfer_Rte);
		IW->gi.dl_count = IW->gi.dl_num = 0;
		IW->gi.dl_route = -1;
		GpsCtrlWait();
		break;

	// �g���b�N�擾 /////////////////////////////////////////
	case GPS_TRACK:
		// Track�J�n
		LpSendCmd(Cmnd_Transfer_Trk);
		IW->gi.dl_count = IW->gi.dl_num = IW->gi.dl_route = 0;
		GpsCtrlWait();
		break;

	// GPS�G�~�����[�V���� /////////////////////////////////////////

	case GPS_EMUL_INFO_WAIT:
	case GPS_EMUL_WPT_WAIT:
	case GPS_EMUL_ROUTE_WAIT:
	case GPS_EMUL_TRACK_WAIT:
		switch(IW->gi.ul_acknak){
		case 0:
			// SELECT�L�[�ŒʐM�������f
			if(IW->key_state == KEY_SELECT) IW->gi.emu_nak = EMU_MAX_ERROR; // &���Z�͎g�킸�A�P�̃`�F�b�N
			else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) < ACK_TIMEOUT) return 0; // �^�C���A�E�g1�ȓ� (�E�F�C�g)
			// no break;
		case Pid_Nak_Byte:
			if(IW->gi.emu_nak++ < EMU_MAX_ERROR) break; // ���g���C���ԓ�(�ē]��)
			//�G�~�����[�V�������[�h�̏I��
			PlaySG1(SG1_CANCEL);
			IW->gi.dl_accept &= ~2; // �]�����̃f�[�^�͈ȍ~��������
			IW->gi.state = GPS_GET_PINFO; // GPS�ƍē������J�n
			return 0;

		case Pid_Ack_Byte:
			//�]������
			IW->gi.emu_nak = 0;
			IW->gi.dl_count++;
			break;
		}
		IW->gi.state--;
		return MT_GPSCtrl();
//		break;

	case GPS_EMUL_INFO:
		if(!IW->gi.dl_count){
			// GBA��GPS�Ɍ���������PC(�J�V�~�[����)�Ɖ�b����
			const u8 GPS_ID[] = {"\x3a\x01\x01\x00ParaNavi GPS Emulator"}; // foretex�Ɍ�����
//			const u8 GPS_ID[] = {"\x3a\x99\x01\x00Test"}; // ���m��ID��Ԃ��ăJ�V�~�[�����GPS��I�΂���e�X�g
			LpSend(Pid_Product_Data, GPS_ID, sizeof(GPS_ID));
		} else {
			IW->gi.state = GPS_EMUL_FINISH;
			break;
		}
		GpsCtrlWait();
		break;

	case GPS_EMUL_WPT:
		if(!IW->gi.dl_count){
			// ��WPT���ʒm
			u16 n = (u16)WPT->wpt_count;
			if(n > MAX_WPT) n = MAX_WPT;
			LpSend(Pid_Records, (u8*)&n, sizeof(n));
		} else if(IW->gi.dl_count <= WPT->wpt_count){
			// WPT�ʒm(���x�����܂݁A�C�Ӓ��̖��O���w��ł���D108���g��)
			D108_Wpt_Type wd[2]; // wd[1]��indet�p�o�b�t�@
			LpSend(Pid_Wpt_Data, (u8*)wd, MakeD108Wpt(wd, &WPT->wpt[IW->gi.dl_count - 1], -1));
		} else if(IW->gi.dl_count == WPT->wpt_count + 1){
			// WPT�����ʒm
			u16 n = Cmnd_Transfer_Wpt;
			LpSend(Pid_Xfer_Cmplt, (u8*)&n, sizeof(n)); // �����ʒm
		} else {
			// �]���I��
			PlaySG1(SG1_CHANGE);
			IW->gi.state = GPS_EMUL_FINISH;
			break;
		}
		GpsCtrlWait();
		break;

	case GPS_EMUL_ROUTE:
		if(IW->gi.dl_count == 0){
			// ��Route/Wpt���ʒm
			u32 MP_SendRoute(u16 push);
			u32 i = IW->mp.route_sel; // ���[�g�]�����̑I�����[�g�ԍ� (���������ɕϊ�)
			if(IW->mp.proc == MP_SendRoute && i < ROUTE->route_count){
				// ���[�g�]�����j���[����̌Ăяo�����͑I�����Ă��郋�[�g�݂̂�1�����]������
				IW->gi.dl_route     = i;
				IW->gi.dl_route_end = i + 1;
			} else {
				// �S�Ẵ��[�g�𑗐M����
				IW->gi.dl_route     = 0;
				IW->gi.dl_route_end = ROUTE->route_count;
			}
			u16 n = 0; // ���M�f�[�^��
			for(i = IW->gi.dl_route ; i < IW->gi.dl_route_end ; ++i) n += ROUTE->route[i].count + 1; // �eWpt�������Z
			LpSend(Pid_Records, (u8*)&n, sizeof(n)); // �T�C�Y�ʒm
		} else if(IW->gi.dl_count == 1){
			if(IW->gi.dl_route >= IW->gi.dl_route_end){
				// Route�����ʒm
				u16 n = Cmnd_Transfer_Rte;
				LpSend(Pid_Xfer_Cmplt, (u8*)&n, sizeof(n)); // �����ʒm
			} else {
				// ���[�g���ʒm
				Route* rt = &ROUTE->route[IW->gi.dl_route];
				LpSend(Pid_Rte_Hdr, rt->name, CalcNameLen(rt->name));
			}
		} else {
			if(IW->gi.dl_route >= IW->gi.dl_route_end){
				// �]���I��
				PlaySG1(SG1_CHANGE);
				IW->gi.state = GPS_EMUL_FINISH;
				break;
			}
			// �����[�g�`�F�b�N
			Route* rt = &ROUTE->route[IW->gi.dl_route];
			u32 offset = IW->gi.dl_count - 2;
			if(offset >= rt->count){
				IW->gi.dl_route++;
				IW->gi.dl_count = 1;
				break; // ���݂̃��[�g����
			}

			// ���[�g�E�F�C�|�C���g��]��
			D108_Wpt_Type wd[2];
			LpSend(Pid_Rte_Wpt_Data, (u8*)wd, MakeD108Wpt(wd, GetWptInfo(rt, offset), GetCurCyl(rt, offset)));
		}
		GpsCtrlWait();
		break;

	case GPS_EMUL_TRACK:
		// Track�]���͈��kx115k�]����GetTrack.exe�̕������̃v���g�R�����200�{�������B
		// �܂��A���̃v���g�R���ł�MAX 65535�g���b�N�܂ł�������Ȃ��B
		// �R�����킴�킴�g���l�����邩������Ȃ����A�Ƃ肠���������͂��Ă����c�B
		if(!IW->gi.dl_count){
			// ��Track���ʒm
			u16 n = IW->gi.ul_track;
			LpSend(Pid_Records, (u8*)&n, sizeof(n)); // �T�C�Y�ʒm (�ő�65535�܂ł����w��ł��Ȃ�!)
		} else if(IW->gi.dl_count <= IW->gi.ul_track){ // �g���b�N�]��
			if(IW->gi.ul_pre != IW->gi.dl_count){
				IW->gi.ul_pre = IW->gi.dl_count;
				if(IW->gi.tinfo.seg == -1) IW->gi.tinfo.seg = 0;
				else {
					IW->gi.tinfo.pre_sect = -1;
					NextTrack(0); // ���擾���[�h
					if(IW->gi.dl_count == 1) IW->gi.tinfo.seg = -1; // �擪�͋����Z�O�����g
				}
			}
			if(IW->gi.tinfo.seg == -1){
				u8 buf[60];
				LpSend(Pid_Trk_Hdr, buf, MakeD310TrkHead(buf));
			} else {
				D301_Trk_Point_Type buf;
				LpSend(Pid_Trk_Data, (u8*)&buf, MakeD301Trk(&buf));
			}
		} else if(IW->gi.dl_count == IW->gi.ul_track + 1){
			// �g���b�N���O�����ʒm
			u16 n = Cmnd_Transfer_Wpt;
			LpSend(Pid_Xfer_Cmplt, (u8*)&n, sizeof(n)); // �����ʒm
		} else {
			// �]���I��
			PlaySG1(SG1_CHANGE);
			IW->gi.state = GPS_EMUL_FINISH;
			break;
		}
		GpsCtrlWait();
		break;

	case GPS_EMUL_FINISH:
		GpsCtrlWait();
		break;

	case GPS_EMUL_FINISH_WAIT:
		// ��莞��PC����R�}���h���Ȃ���΁A�^�C���A�E�g���Ēʏ탂�[�h�ɖ߂�
		if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) >= EMUL_TIMEOUT){
			IW->gi.dl_accept &= ~2; // �]�����̃f�[�^�͈ȍ~��������
			IW->gi.state = GPS_GET_PINFO; // GPS�ƍē������J�n
		}
		break;

	// �ȍ~�ABluetooth AT�R�}���h���� (for ParaniESD100/110/200/210)
	// ����WAIT����
	case GPS_BT_BR_CHANGE_WAIT:
	case GPS_BT_BR_INFO_WAIT:
	case GPS_BT_BR_RESET_WAIT:
	case GPS_BT_BR_CANCEL_WAIT:
	case GPS_BT_BR_MODE0_WAIT:
	case GPS_BT_CON_SETKEY_WAIT:
	case GPS_BT_CON_DIAL_WAIT:
	case GPS_BT_CON_STANDBY_WAIT:
	case GPS_BT_CON_ATH_WAIT:
	case GPS_BT_CON_MODECHANGE_WAIT:
	case GPS_BT_CON_RESET_WAIT:
	case GPS_BT_SCAN_START_WAIT:
	case GPS_BT_SCAN_RESET_WAIT:
		if(IW->gi.ul_acknak != ATCRES_NONE){
			if(IW->gi.ul_acknak == ATCRES_OK ||
				IW->gi.state == GPS_BT_BR_CANCEL_WAIT){ // �G���[�ł���
				GpsCtrlWait(); // ����(state++)
			} else {
				IW->gi.state = GPS_BT_ERROR; // �V�[�P���X���f
			}
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) > BTCMD_TIMEOUT){
			IW->gi.state--; // ���g���C
		}
		break;

	// �{�[���[�g�`�F�b�N���ύX�V�[�P���X
	// 1. "+++"					"OK"		(�����Ō��݂̃{�[���[�g�m�F)
	// 2. "AT+UARTCONFIG,BR..."	"OK"		(�ݒ�{�[���[�g�����ƈႤ�ꍇ�̂ݎ��s)
	// 3. "AT+BTCANCEL"			"OK/ERROR"	(�O�̂���standby�ɃZ�b�g)
	// 4. "ATH"					"OK+DISCONNECT/ERROR"	(�O�̂��ߐڑ��ؒf)
	// 5. "AT+BTINFO?"			"OK+status"	(���[�h�ؑւ��K�v���`�F�b�N)
	// 6. "AT+BTMODE,0"			"OK"	 	(�ݒ胂�[�h��0�ȊO�̏ꍇ�̂ݎ��s)
	// 7. "ATZ"					"OK"		(2 or 6 ���s���̂�)
	case GPS_BT_BR_CHECK1:
	case GPS_BT_BR_CHECK2:
	case GPS_BT_BR_CHECK3:
	case GPS_BT_BR_CHECK4:
	case GPS_BT_BR_CHECK5:
		StartGPSMode(IW->tc.bt_mode + (IW->gi.state - GPS_BT_BR_CHECK1) / 2);
		ATCSend("+++\r", -1);
		GpsCtrlWait(); // GPS_BT_BR_WAITx��
		break;

	// �{�[���[�g�ύX�p�����҂�����
	case GPS_BT_BR_WAIT1:
	case GPS_BT_BR_WAIT2:
	case GPS_BT_BR_WAIT3:
	case GPS_BT_BR_WAIT4:
	case GPS_BT_BR_WAIT5: // �f�t�H���g�{�[���[�g������2��`�F�b�N
		if(IW->gi.ul_acknak != ATCRES_NONE){ // ���{�[���[�g�ŒʐM��("OK"�ł�"ERROR"�ł����ł��ǂ�)
			if(IW->gi.state == GPS_BT_BR_WAIT1 || IW->gi.state == GPS_BT_BR_WAIT5){
				IW->gi.state = GPS_BT_BR_CANCEL; // �{�[���[�g�̕ύX�K�v�Ȃ�
				ATC_PTR->baudrate_err = 0;
			} else {
				IW->gi.state = GPS_BT_BR_CHANGE; // �{�[���[�g�ύX�V�[�P���X�J�n
				ATC_PTR->baudrate_err = 1;
			}
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) > BTPPP_TIMEOUT){ // +++�����Ȃ�?
			if(IW->gi.state == GPS_BT_BR_WAIT5){
				IW->gi.state = GPS_BT_ERROR; // �S�{�[���[�g�ŉ����Ȃ�
			} else {
				IW->gi.state++; // ���̃{�[���[�g�̃`�F�b�N��
			}
		}
		break;

	case GPS_BT_BR_CHANGE:
		// �ݒ肳�ꂽ�{�[���[�g�ɐ؂�ւ���
		ATCSend("AT+UARTCONFIG,", -1);
		ATCSend(BAUDRATE_STR[IW->tc.bt_mode & 3], -1); // �ݒ肳�ꂽ�{�[���[�g�ɐ؂�ւ�
		ATCSend(",N,1,1\r", -1); // �{�[���[�g�ȍ~�͌Œ�
		GpsCtrlWait(); // GPS_BT_BR_CHANGE_WAIT��
		break;

	case GPS_BT_BR_CANCEL:
		ATCSend("AT+BTCANCEL\r", -1);
		GpsCtrlWait(); // GPS_BT_BR_CANCEL_WAIT��
		break;

	case GPS_BT_BR_ATH:
		ATCSend("ATH\r", -1);
		GpsCtrlWait(); // GPS_BT_BR_ATH_WAIT��
		break;

	case GPS_BT_BR_ATH_WAIT:
		// �ڑ�����"OK"+"DISCONNECT���K�v�B���ڑ�����"ERROR"�݂̂�OK�B
		if(IW->gi.ul_acknak == ATCRES_ERROR || IW->gi.ul_acknak == ATCRES_DISCONNECT){
			GpsCtrlWait(); // ����(state++)
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) > BTCMD_TIMEOUT){
			IW->gi.state--; // ���g���C
		}
		break;

	case GPS_BT_BR_INFO:
		ATCSend("AT+BTINFO?\r", -1);
		ATC_PTR->last_mode = -1;
		GpsCtrlWait(); // GPS_BT_BR_INFO_WAIT��
		break;
		
	case GPS_BT_BR_INFO_CHECK:
		if(ATC_PTR->last_mode)			IW->gi.state = GPS_BT_BR_MODE0; // ���[�h�ؑ֗v
		else if(ATC_PTR->baudrate_err)	IW->gi.state = GPS_BT_BR_RESET; // �{�[���[�g�ύX�v
		else                   			IW->gi.state = GPS_BT_BR_OK;
		break;

	case GPS_BT_BR_MODE0:
		ATCSend("AT+BTMODE,0\r", -1);
		GpsCtrlWait(); // GPS_BT_BR_MODE0_WAIT��
		break;

	case GPS_BT_BR_RESET:
		// �ݒ肳�ꂽ�{�[���[�g/���[�h�ɐ؂�ւ��𔽉f������
		ATCSend("ATZ\r", -1);
		// GpsCtrlWait(); // GPS_BT_BR_RESET_WAIT��
		IW->gi.state = GPS_BT_BR_BOOTSLEEP; // �V�����{�[���[�g�ŉ������Ԃ�̂ŁA�����`�F�b�N�͏ȗ�
		IW->gi.vbc = IW->vb_counter;
		break;

	case GPS_BT_BR_BOOTSLEEP:
		// ���u�[�g�����҂�
		if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) > BTBOOT_TIMEOUT) IW->gi.state = GPS_BT_BR_CHECK1; // �O�̂��߃{�[���[�g�`�F�b�N����Ď��s
		break;

	case GPS_BT_BR_OK:
		// ���j���[���ł̏����҂�
		break;

	// �X�L�����V�[�P���X (�X�L�����������MODE1�ɂ��Ă��ؒf���ċ����I��MODE0�ɖ߂�)
	// 1. "AT+BTINQ?"	"BtAddr,FName,Class" x N (�ő�:�T�[�`30�b, N:15)
	//					"OK"
	case GPS_BT_INQ_START:
		ATCSend("AT+BTINQ?\r", -1);
		GpsCtrlWait(); // GPS_BT_INQ_START_WAIT��
		break;

	case GPS_BT_INQ_WAIT:
		if(IW->gi.ul_acknak == ATCRES_OK){
			IW->gi.state++; // ����
		} else if(IW->gi.ul_acknak != ATCRES_NONE){
			IW->gi.state = GPS_BT_ERROR; // �V�[�P���X���f
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) > BTINQ_TIMEOUT){
			if(ATC_PTR->inq_count) IW->gi.state++; // 1�ł�����������ΑI���B(�����ɂ��Ă���)
			else                   IW->gi.state--; // ���g���C
		}
		break;

	case GPS_BT_INQ_DONE:
		break; // ���j���[�I�������҂��B���ɉ������Ȃ��B

	// �ڑ��V�[�P���X
	// 1. "AT+BTKEY="PIN""	"OK" (PIN����łȂ��ꍇ�̂ݐݒ�)
	// 2. "ATDBtAddr"		"OK\nConnect BtAddr"
	// 3. "+++"				"OK\n"(�ȍ~�A�����ڑ����邽�߂̐ݒ�)
	// 4  "ATH"				"OK"
	// 5. "AT+BTMODE,1"		"OK" (���[�h�ؑ�)
	// 6. "ATZ"				"OK" (�ċN���ŁAMode1�؂�ւ����f)
	case GPS_BT_CON_SETKEY:
		if(!*IW->tc.bt_pin){
			// PIN�w�肵�Ȃ�
			IW->gi.state = GPS_BT_CON_DIAL;
		} else {
			// �F��PIN�ݒ�
			ATCSend("AT+BTKEY=\"", -1);
			ATCSend(IW->tc.bt_pin, BT_PIN_LEN);
			ATCSend("\"\r", -1);
			GpsCtrlWait(); // GPS_BT_CON_SETKEY_WAIT��
		}
		break;

	case GPS_BT_CON_DIAL:
		// PIN�ݒ�
		ATCSend("ATD", -1);
		ATCSend(IW->tc.bt_addr, BT_ADDR_LEN);
		ATCSend("\r", -1);
		GpsCtrlWait(); // GPS_BT_CON_DIAL_WAIT��
		break;

	case GPS_BT_CON_DIAL_CHECK:
		if(IW->gi.ul_acknak == ATCRES_CONNECT){
			// �ڑ�����
			if(IW->key_state & KEY_SELECT){
				IW->gi.state = GPS_BT_CON_COMPLETE; // �ڑ���A���[�h�ؑւ����ɂ��̂܂܎g�p����B�����[�h
			} else {
				IW->gi.state = GPS_BT_CON_STANDBY; // ���[�h�ؑւ��ēd��ON���Ɏ����ڑ��ł���悤�ݒ�
			}
		} else if(IW->gi.ul_acknak == ATCRES_ERROR){
			IW->gi.state = GPS_BT_ERROR; // �ڑ��G���[
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) > BTDIAL_TIMEOUT){
			IW->gi.state = GPS_BT_ERROR; // �ڑ��G���[
//			IW->gi.state = GPS_BT_CON_DIAL; // Dial���烊�g���C
		}
		break;
		// no break

	case GPS_BT_CON_STANDBY: // ���[�h�ؑւ̂��߂Ɉ�x�X�^���o�C�ɖ߂�
		ATCSend("+++\r", -1);
		GpsCtrlWait(); // GPS_BT_CON_STANDBY_WAIT��
		break;

	case GPS_BT_CON_ATH: // �ؒf
		ATCSend("ATH\r", -1);
		GpsCtrlWait(); // GPS_BT_CON_ATH_WAIT��
		break;

	case GPS_BT_CON_ATH_CHECK: // �ؒf�`�F�b�N
		if(IW->gi.ul_acknak == ATCRES_DISCONNECT){
			IW->gi.state = GPS_BT_CON_MODECHANGE; // �ؒf�����烂�[�h�ؑւ��L���ɂȂ�
		} else if(IW->gi.ul_acknak != ATCRES_NONE){ // �\�����Ȃ�����
			IW->gi.state = GPS_BT_ERROR; // �V�[�P���X���f
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) > BTATH_TIMEOUT){
			IW->gi.state = GPS_BT_CON_ATH; // ���g���C�̈Ӗ�����?
		}
		break;

	case GPS_BT_CON_MODECHANGE: // ���[�h�ؑ֎��s
		ATCSend("AT+BTMODE,1\r", -1);
		GpsCtrlWait(); // GPS_BT_CON_MODECHANGE_WAIT��
		break;

	case GPS_BT_CON_RESET: // ���u�[�g�Ŕ��f
		ATCSend("ATZ\r", -1);
		GpsCtrlWait(); // GPS_BT_CON_RESET_WAIT��
		break;

	case GPS_BT_CON_COMPLETE: // �����I��
		break; // ���j���[�I�������҂��B���ɉ������Ȃ��B

		
	case GPS_BT_SCAN_START:
		ATCSend("AT+BTMODE,3\r", -1);
		GpsCtrlWait(); // GPS_BT_SCAN_START_WAIT��
		break;

	case GPS_BT_SCAN_RESET: // ���u�[�g�Ŕ��f
		ATCSend("ATZ\r", -1);
		GpsCtrlWait(); // GPS_BT_SCAN_RESET_WAIT��
		break;

	case GPS_BT_SCAN_COMPLETE: // �����I��
		break; // ���j���[�I�������҂��B���ɉ������Ȃ��B

	case GPS_BT_IDLE_START:
		// �{�[���[�g�ύX�V�[�P���X�Ń��[�h�ؑւ��Ă���̂ŉ������Ȃ��ėǂ�
		IW->gi.state = GPS_BT_IDLE_COMPLETE;
		break;

	case GPS_BT_IDLE_COMPLETE: // �����I��
		break; // ���j���[�I�������҂��B���ɉ������Ȃ��B
		
	case GPS_BT_ERROR: // �ُ�I��
		// �G���[�������̓��g���C�����Ȃ� (���j���[�����҂�)
		break;

	}
	return 0;
}

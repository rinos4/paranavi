///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// Garmin GPSのバイナリプロトコル層をここで実装。そのほか、GPSデータを
// 使い易いPvtXに変換したり、モーション検出処理もここで定義。

///////////////////////////////////////////////////////////////////////////////
// 定数
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
// Local関数
///////////////////////////////////////////////////////////////////////////////
// rintのかわり。直接s32を返す
s32 MyRound(double v){
	if(v < 0) return (s32)(v - 0.5);
	else      return (s32)(v + 0.5);
}

// 任意アライメントコピー(軽量memcpy)
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

// 閏年判定
static inline u32 IsLeapYear(u32 year){
//	return !(year % 4) && ((year % 100) || !(year % 400));
	return !(year & 3); // 1990～2058ならこれで充分…
}

static inline u32 GetYearDay(u32 year){
	return IsLeapYear(year)? 366 : 365;
}

// キュー処理(モーション検出用)
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
// グライダ状態
///////////////////////////////////////////////////////////////////////////////
// 【ステート定義】
// 左旋回: 連続的な左方向へのベクトル変化の合計がL45°以上となること (10ベクトル以内検出)
// 右旋回: 連続的な右方向へのベクトル変化の合計がR45°以上となること (10ベクトル以内検出)
// センタリング: 過去60ポイントのベクトル差合計が300°以上になること (60ベクトル以内検出)
// ローリング: 120°以上の旋回変化が交互に検出されること(60ベクトル以内で回数カウント)
// ピッチング: 水平方向に加速度1m/s以上の速度の変化が交互に検出されること(60ベクトル以内で回数カウント)
// 直進:       上記以外の状態で、前回が停止でない場合または hv > stop m/s
// ヒステリシスは特に設けない

static inline s32 GetSgn(s32 v){
	if(v < 0) return -1;
	if(v > 0) return  1;
	return 0;
}
s32 CheckSgn(s32 v1, s32 v2){
	if(!v1 || !v2) return 1; // どちらかが0のときは、符号反転なしとする
	return (v1 < 0) == (v2 < 0);
}

// ピッチング検出
s32 CheckPitch(PvtX* px){
//	const S32Queue* up = &IW->pr.up_mm;
	const S32Queue* up = &IW->pr.vh_mm; // 垂直速度ではなく、水平速度で判断する
	s32 state = 0;
	s32 pre_val = 0, pre_diff = 0, sum = 0, next_up = 0, same = 0;

	s32 tail = up->tail, num = 0;
	for(; tail != up->head ; ++num){
		if(!tail) tail = MAX_REC;
		s32 cur_val = up->val[--tail];
		if(num){
			s32 diff = pre_val - cur_val;
			if(CheckSgn(pre_diff, diff)){
				// ピッチング方向同じ
				sum += diff;
				if(++same > IW->tc.pitch_freq) sum = 0;
			} else {
				// ピッチ方向変化
				if(same > 1 && myAbs(sum) > IW->tc.pitch_diff && CheckSgn(sum, next_up)){
					// ピッチングカウンタUP
					state += GSTATE_PITCH;
					if(!next_up && sum < 0) state |= GSTATE_PITCHTOP_D;
					next_up = -sum; // 逆方向への切り返しを待つ
				}
				sum = diff;
				same = 0;
			}
			pre_diff = diff;
		}
		pre_val = cur_val;
	}

	// ピッチング切り替わりチェック
	if((state & GSTATE_PITCH_MASK) > 2 && (state & GSTATE_PITCHTOP_D) != (px->gstate & GSTATE_PITCHTOP_D)){
		state |= GSTATE_PITCHTOP_X;
	}
	return state;
}

// ローリング/センタリング検出
s32 CheckRoll(PvtX* px){
	const S32Queue* ang = &IW->pr.ang64;
	const S32Queue* vhm = &IW->pr.vh_mm;

#define STRAIGHT_DETECT 16
	s32 sum_v[STRAIGHT_DETECT], sum_vi = 0;

	// 旋回検出
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
			pre_vhm = 0;//停止フラグセット
			diff = 0;
		}

		// センタリングガード判定
		if(!centering_guard){
			// 連続直進チェック
			if(myAbs(diff) < ANG64K(10)){
				sum_v[sum_vi] = sum;
				if(++sum_vi    >= STRAIGHT_DETECT) sum_vi = 0;
				if(++str_count >= STRAIGHT_DETECT && myAbs(sum_v[sum_vi] - sum) < ANG64K(30)) {
					centering_guard = 1;
				}
			} else {
				str_count = 0;
			}

			// 逆切り返し60°チェック
			if(diff > 0) sum_l += diff;
			else         sum_r -= diff;
			if(sum_r > ANG64K(60) && sum_l > ANG64K(60)){
				centering_guard = 1;
			}
		}

		// 停止中、または停止直後の動作は差分角が無効なのでスキップ
		if(!pre_vhm) continue;

		// 旋回量からローリングとセンタリングをチェック
		sum += diff;

		// 変化量からローリングチェック
		if(CheckSgn(pre_diff, diff)){
			// 旋回方向同じ
			roll_sum  += diff;
			if(++same > IW->tc.roll_freq) roll_sum = 0;
		} else {
			turn++;
			// 旋回方向変化
			if(same > 1 && myAbs(roll_sum) > 182 * IW->tc.roll_diff && CheckSgn(roll_sum, next_roll)){
				// ローリングカウンタUP
				state += GSTATE_ROLL;
				if(!next_roll && roll_sum < 0) state |= GSTATE_ROLLTOP_L;
				next_roll = -roll_sum; // 逆方向への120°切り返しを待つ
			}
			roll_sum = diff;
			same = 0;
		}
		// 旋回チェック
		if(!turn && num < 10){
			if(sum > ANG64K( 45)) state |= GSTATE_TURN_L;
			if(sum < ANG64K(-45)) state |= GSTATE_TURN_R;
		}

		// センタリングチェック
		if(centering_guard) continue;

		t = myAbs(sum);
		if(t > CETERING_DETECT){
			state |= GSTATE_CENTERING;
			if(sum > 0) state |= GSTATE_CENTERING_X;

			if(px->up_turn == INVALID_VAL && t > ANG64K(360)){
				px->up_turn = SumS32Queue(&IW->pr.up_mm, num); // 上昇率/turnを格納
				px->up_r    = RoundDiv(SumS32Queue(&IW->pr.vh_mm, num) * 104, t); // 回転半径を格納
				px->up_time = num;
			}
		}
	}
	return state;
}

// モーション検出
s32 GetGliderState(PvtX* px){
	s32 state = 0;

//#define STOP_CHECK 5
//	if(SumS32Queue(&IW->pr.vh_mm, STOP_CHECK) > IW->tc.stop_dir * STOP_CHECK){
	if(px->gstate || px->vh_mm > IW->tc.start_spd){
		state = CheckRoll(px); // 回転チェックは移動中のみ

		// スパイラル/ストール判定
		if(state & GSTATE_CENTERING){
			if(px->up_mm < IW->tc.spiral_spd) state ^= GSTATE_SPIRAL | GSTATE_CENTERING;
		} else {
			if(px->up_mm < IW->tc.stall_spd)  state |= GSTATE_STALL;
		}

		// ロール切り替わりチェック
		if((state & GSTATE_ROLL_MASK) > 2 && (state & GSTATE_ROLLTOP_L) != (px->gstate & GSTATE_ROLLTOP_L)){
			state |= GSTATE_ROLLTOP_X;
		}
	}

	// 直進検出
	if(!(state & GSTATE_TURN_MASK)){
		if(px->gstate || px->vh_mm > IW->tc.start_spd){
			state |= GSTATE_STRAIGHT;
			// ピッチング検出
			state |= CheckPitch(px);
		}
	}

	// テイクオフは、ここで検出しておく
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
// 日時計算
///////////////////////////////////////////////////////////////////////////////
const u32 DAY_COUNT[2][12] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

u32 GetDTTime(u32 v, u32* hour, u32* min, u32* sec){
	// BiosDivはs32なので、最上位ビットを特殊処理する
	u32 t = v & 1;
	v = BiosDiv(v >> 1, 30, sec);
	*sec = (*sec << 1) | t;
	v = BiosDiv(v, 60, min);
	return BiosDiv(v, 24, hour);
}
void GetDTDate(u32 v, u32* year, u32* month, u32* day){
	// BiosDivはs32なので、最上位ビットを特殊処理する
	v = BiosDiv(v >> 1, 30 * 60 * 24, &v); // 日にち取り出し

	// 年計算
	for(*year = 1990 ;; ++*year){
		u32 day = GetYearDay(*year);
		if(v <= day) break;
		v -= day;
	}

	// 月計算
	const u32* mt = DAY_COUNT[IsLeapYear(*year)? 1 : 0]; 
	for(*month = 0 ;; ++*month){
		if(v <= mt[*month]) break;
		v -= mt[*month];
	}
	++*month;

	// 残りが日にち
	*day = v;
}
void GetDateTime(u32 v, u32* year, u32* month, u32* day, u32* week, u32* hour, u32* min, u32* sec){
	BiosDiv(GetDTTime(v, hour, min, sec), 7, week);
	GetDTDate(v, year, month, day);
}

const u32 DAY_SUM[12] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

// 1989/12/31からの累積日数取得
u32 DayCount(u32 y, u32 m, u32 d){
	// 1990～2058ならこれで充分…
	if(IsLeapYear(y) && m > 2) ++d;
	y -= 1990;
	return 365 * y + (y + 1) / 4 + DAY_SUM[m - 1] + d;
}

// ヒストグラムをアップデート
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

// バリオだけは特殊
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
// PVT変換
///////////////////////////////////////////////////////////////////////////////
// NMEAでは垂直速度を取得できないため、alt_mmの差から計算する。
EStatus CalcNMEAUp(PvtX* px, PvtX* pre_px){
	if(px->fix < FIX_3D || pre_px->fix < FIX_3D){
		// 信頼できない値の場合は前の値をそのまま使う
		px->up_mm = pre_px->up_mm;
		IW->mp.nmea_xalt = INVALID_VAL;
	} else {
		// 3D測位時のみ上昇率計算 (V-DOPで十分な精度かチェックした方が良い?)
		if(IW->mp.nmea_xalt == INVALID_VAL) IW->mp.nmea_xalt = px->alt_mm;
		switch(IW->tc.nmea_up){
		case 0: // 直値
		case 1: // 1/2で平均化
		case 2: // 1/4で平均化
			// 0-3は上昇率変化を前後ポイントで平均化させる。
			// 例えば0.0の定常から+8.0で上昇を始めた場合は以下のように推移
			// 1/2: +4.0,+6.0,+7.0,+7.5,...
			// 1/4: +2.0,+3.5,+4.6,+5.5
			px->up_mm = (px->alt_mm - IW->mp.nmea_xalt) >> IW->tc.nmea_up;
			IW->mp.nmea_xalt += px->up_mm;
			break;
		}
		s32 dt = px->dtime  - pre_px->dtime;
		if(dt > 1)  px->up_mm = RoundDiv(px->up_mm, dt); // 1秒精度
	}
	return 0;
}

// NMEAをPvtXに変換
EStatus CalcPvtNMEA(PvtX* px){
	// px2に格納した新データと旧pxを交換する
	PvtX* pre_px = &IW->px2, swap = *pre_px;
	*pre_px = *px;
	*px = swap;

	// LAT/LON/ALTは測位前でも正常値と見分けのつかない値を返す。(N36°, E136°,0m等)
	// fixで正常かどうかチェックする。NGなら前の値に戻す。
	if(px->fix < FIX_2D){
		px->lat    = pre_px->lat;
		px->lon    = pre_px->lon;
		px->alt_mm = pre_px->alt_mm;
	}

	// ベクトル計算
	CalcNMEAUp(px, pre_px);// NMEAでは垂直速度を直接取得できないので…
	px->v_mm  = BiosHypot(px->vh_mm, px->up_mm);
	px->vn_mm = (px->vh_mm * (Cos64K(px->h_ang64) >> 6)) >> 9;
	px->ve_mm = (px->vh_mm * (Sin64K(px->h_ang64) >> 6)) >> 9;

	// STOP速度以下の場合は方位を直前の値に戻す
	if(px->vh_mm <= IW->tc.stop_dir){
		px->h_ang64 = pre_px->h_ang64; // 直前の値
		// px->v_ang64はそのまま
	} else {
		px->v_ang64 = BiosAtan64K(px->up_mm, px->vh_mm);
		// px->h_ang64は更新データを使う
	}

	// DTime設定 (GPS52Dは電池切れ＆衛星捕捉前は23:59:48からの経過時間を返すが特に区別はしない)
	px->dtime = DayCount(px->year, px->month, px->day) * 86400 +
				px->hour * 3600 + px->min * 60 + px->sec + IW->tc.tzone_m * 60;

	CalcPvtExt(px);	// Garmin/NMEA共通の拡張情報を設定する
	*pre_px = *px; // 次のNMEAパケットの受信に備えてpx2を最新情報に更新しておく
	return 0;
}

// GarminPvtをPvtXに変換
EStatus CalcPvtGarmin(const D800_Pvt_Data_Type* pvt, PvtX* px){
	PvtX* pre_px = &IW->px2;
	*pre_px = *px;

	memcpy(&IW->pvt_raw, pvt, sizeof(D800_Pvt_Data_Type)); // デバッグ用にコピーしておく

	// 緯度、経度(MAX ddd°mm'ss"xxx)
	px->lat = MyRound(GetDouble(pvt->lat) * RAD2LAT);
	px->lon = MyRound(GetDouble(pvt->lon) * RAD2LAT);

	// 高度
	px->alt_mm = GetFloatK(pvt->alt);
	if(!IW->tc.alt_type) px->alt_mm += GetFloatK(pvt->msl_hght);

	// 誤差
	px->epe_mm = GetFloatK(pvt->epe);
	px->eph_mm = GetFloatK(pvt->eph);
	px->epv_mm = GetFloatK(pvt->epv);

	// 速度
	px->vn_mm = GetFloatK(pvt->north);
	px->ve_mm = GetFloatK(pvt->east);
	px->up_mm = GetFloatK(pvt->up);
	px->vh_mm = BiosHypot (px->ve_mm, px->vn_mm);
	px->v_mm  = BiosTripot(px->ve_mm, px->vn_mm, px->up_mm);

	// 角度
	if(px->vh_mm > IW->tc.stop_dir){
		px->h_ang64 = BiosAtan64K(px->ve_mm, px->vn_mm); // 北を始点に時計回り。
		px->v_ang64 = BiosAtan64K(px->up_mm, px->vh_mm);
	}

	// FIX値
	px->fix = (u32)GetInt(pvt->fix);

	// 時刻計算(2038+20年問題あり)
//	t = MyRound(GetDouble(pvt->tow)) - GetInt(pvt->leap_scnds) + GetLong(pvt->wn_days) * 86400 + IW->tc.tzone_m * 60;
	// TODO eTrexでleap_scnds が 0x010e になることがある？(暫定として下位1byteのみ使用する。要調査!)
	px->dtime = MyRound(GetDouble(pvt->tow)) - (s8)GetInt(pvt->leap_scnds) + GetLong(pvt->wn_days) * 86400 + IW->tc.tzone_m * 60;

	return CalcPvtExt(px);	// Garmin/NMEA共通の拡張情報を設定する
}

EStatus CalcPvtExt(PvtX* px){
	PvtX* pre_px = &IW->px2; // 1つ前のデータが格納されている

	// 基本データ更新
	IW->mp.navi_update |= NAVI_UPDATE_PVT| NAVI_PVT_CHANGE;
	px->pvt_timeout = 0;
	px->counter++;
	if(px->fix > 5) px->fix = 0;
	px->fix_sum[px->fix]++;

	// 時刻関係処理
	IW->mp.tally.log_time = IW->task_log.log_time = px->dtime;
	// 初回1回だけシードを設定
	if(px->dtime && !IW->mp.rand_init){
		IW->mp.rand_init = 1;
		MySrand(px->dtime);
	}
	// クローズタイム処理
	if(IW->tc.close_time && IW->tc.task_mode && IW->tc.close_time == px->hour * 60 + px->min){
		Route* rt = GetTaskRoute();
		if(rt && rt->count){
			PlaySG1(SG1_EXIT);
			IW->mp.nw_target = rt->py[rt->count - 1].wpt;
			IW->mp.navi_update |= NAVI_UPDATE_WPT;
			IW->mp.cur_view = -1;
			IW->tc.task_mode = 0; // フリーフライトにする
		}
	}

	// 加速度
	if(pre_px->fix >= FIX_2D && px->fix >= FIX_2D){
		px->g_mm   = RoundDiv(BiosTripot(	pre_px->vn_mm - px->vn_mm,
											pre_px->ve_mm - px->ve_mm,
											pre_px->up_mm - px->up_mm - 9800) * 10, 98);
	} else {
		px->g_mm = -1;
	}
	px->ldK = px->up_mm? RoundDiv(px->vh_mm * -1000, px->up_mm) : INVALID_VAL;
	GetDateTime(px->dtime, &px->year, &px->month, &px->day, &px->week, &px->hour, &px->min, &px->sec);
	px->h_ang_c    = px->h_ang64 - IW->tc.n_calib * 182; // 補正はx182近似で充分

	s32 ang_diff = GetAngLR(pre_px->h_ang64 - px->h_ang64);
	px->up_turn = INVALID_VAL; // デフォルトで無効値を設定しておく
	AddS32Queue(&IW->pr.vh_mm, px->vh_mm);
	AddS32Queue(&IW->pr.up_mm, px->up_mm);
	AddS32Queue(&IW->pr.ang64, ang_diff);

	s32 pre_state = px->gstate;
	px->gstate = GetGliderState(px);

	// 軌跡情報記録 ///////////////////////////////////////////////////////////
	if(++LOCUS->sample >= IW->tc.locus_smp){
		LOCUS->sample = 0;
		if(++LOCUS->index >= MAX_LOCUS) LOCUS->index = 0;
		LocusVal* lv = &LOCUS->val[LOCUS->index];
		lv->alt_x = RoundDiv(px->alt_mm, 1000) << 1; // 最下位ビットはLatLonの範囲外マークに使う
		lv->up  = px->up_mm;
		// メモリ節約のため差分で保持するよう変更
		if(px->lat == INVALID_LAT){
			*(u32*)&lv->lat_d = INVALID_LAT;
		} else {
			if(IW->mp.cur_lat == INVALID_LAT){ // 最初?
				IW->mp.cur_lat = px->lat;
				IW->mp.cur_lon = px->lon;
			}
			// 差で記録
			s32 dif_lat = px->lat - IW->mp.cur_lat;
			s32 dif_lon = px->lon - IW->mp.cur_lon;
			if(myAbs(dif_lat) < 0x8000 && myAbs(dif_lon) < 0x8000){
				// 1km程度(毎秒ログでマッハ3)以下ならこれで収まる。(極地はもっと小さいが…)
				IW->mp.cur_lat += lv->lat_d = (s16)dif_lat;
				IW->mp.cur_lon += lv->lon_d = (s16)dif_lon;
			} else {
				// 差が大きすぎるときは特殊処理(日付変更線移動もココになるが…)
				LocusBigShift(dif_lat, &IW->mp.cur_lat, &lv->lat_d);
				LocusBigShift(dif_lon, &IW->mp.cur_lon, &lv->lon_d);
				lv->alt_x |= 1; // 範囲外マーク
			}
		}
	}

	// 平均値計算 /////////////////////////////////////////////////////////////
	s32 avg = 1;
	if(IW->mp.sp_view >= SP_VIEW_WINDCHECK){
#define SP_MODE_AVG 10
		avg = SP_MODE_AVG;
	} else {
		switch(IW->tc.avg_type){
		case 0: // 自動
			if(!(px->gstate & GSTATE_ROTATE)){
				// センタリング/ストール中以外は10秒平均
				avg = 10;
				break;
			}
			// センタリング/ストール中はグライダ値をもらう
			// no break
		case 1: // グライダ固定
			px->ldK_avg =  IW->tc.my_ldK;
			px->up_avg  = -IW->tc.my_down;
			px->vh_avg  =  RoundDiv(IW->tc.my_ldK * IW->tc.my_down, 1000);
			avg = 0;
			break;
		case 2:	avg =  1;	break;// リアルタイム
		case 3:	avg =  3;	break;// 3s へいきん
		case 4:	avg = 10;	break;// 10s へいきん
		case 5:	avg = 30;	break;// 30s へいきん
		}
	}
	if(avg){
		px->vh_avg = AvgS32Queue(&IW->pr.vh_mm, avg);
		px->up_avg = AvgS32Queue(&IW->pr.up_mm, avg);
		px->ldK_avg = px->up_avg? RoundDiv(px->vh_avg * -1000, px->up_avg) : INVALID_VAL;
	}

	// 統計情報 ///////////////////////////////////////////////////////////////
	if(px->fix < FIX_2D) return 0; // 2D以上!

	Tally* tl = &IW->mp.tally;
	// 統計開始場所
	if(tl->start_lat == -1){
		tl->start_lat = px->lat;
		tl->start_lon = px->lon;
	}
	// カウンタアップ
	tl->count++;
	u32 len;
	if(!tl->last_sec || tl->last_sec + 1 == px->dtime){
		len = px->v_mm; // 3D
	} else {
		// ロスト時は距離計測
		CalcDist(tl->last_lat, tl->last_lon, px->lat, px->lon, &len, 0);
		len =  len * 1000 + 500;
	}
	tl->sum_v	+= len;
	tl->trip_mm += len;
	tl->last_lat = px->lat;
	tl->last_lon = px->lon;
	tl->last_sec = px->dtime;

	if(px->up_mm > 0) tl->sum_gain += px->up_mm;

	// 単位ベクトル合計(風速用)
	if(px->vh_mm > IW->tc.stop_dir){
		tl->sum_uv++;
		s32 nv = RoundDiv(px->vn_mm * 1000, px->vh_mm);
		s32 ev = RoundDiv(px->ve_mm * 1000, px->vh_mm);
		tl->sum_nv  += nv;
		tl->sum_ev  += ev;
		tl->sum_nsv += myAbs(nv);
		tl->sum_ewv += myAbs(ev);
	}

	// 最高値
	CHANGE_MAX(tl->max_v,	px->v_mm);
	CHANGE_MAX(tl->max_up,  px->up_mm);
	CHANGE_MIN(tl->min_up,  px->up_mm);
	CHANGE_MAX(tl->max_alt, px->alt_mm);
	CHANGE_MIN(tl->min_alt, px->alt_mm);
	if(px->g_mm >= 0){
		CHANGE_MAX(tl->max_G,	px->g_mm);
		CHANGE_MIN(tl->min_G,	px->g_mm);
	}
	// グラフ用
	CHANGE_MAX(IW->mp.max_alt, px->alt_mm);
	CHANGE_MIN(IW->mp.min_alt, px->alt_mm);
	CHANGE_MAX(IW->mp.graph_range, myAbs(px->up_mm));

	// ステート別
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
	else if(px->gstate & (GSTATE_CENTERING | GSTATE_SPIRAL)) state = 3;// センタリングorスパイラル

	// ソアリング統計
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
		// センタリング統計
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
			// センタリング中の切り返し
//(センタリング検出ロジック変更により、切り返し前にセンタリングが一度終了する)
//			if(pre_state & (GSTATE_CENTERING | GSTATE_SPIRAL)){
//				s32* p2 = tl->centering[(px->gstate & GSTATE_CENTERING_X)? 1 : 0];
//				p [TALLY_CENTER_TURN] += CETERING_DETECT;
//				p2[TALLY_CENTER_TURN] -= CETERING_DETECT;
//			}
		}

		// センタリング/スパイラル別統計
		if(!(pre_state & (GSTATE_CENTERING | GSTATE_SPIRAL))) a2 += CETERING_DETECT; // xx_sumの2つはここで検出開始時点のオフセットをプラス
		if(px->gstate & GSTATE_CENTERING)  tl->center_sum += a2;
		if(px->gstate & GSTATE_SPIRAL)     tl->spiral_sum += a2;
	}

	if(px->gstate & GSTATE_STALL)	   tl->stall_sec++;
	if((px->gstate & GSTATE_ROLLTOP_X ) && ((px->gstate & GSTATE_ROLL_MASK ) >>  8) > 1) tl->roll++;
	if((px->gstate & GSTATE_PITCHTOP_X) && ((px->gstate & GSTATE_PITCH_MASK) >> 20) > 1){
		if(state == 3) tl->pitch_r++;
		else           tl->pitch_s++;
	}

	// ヒストグラム
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
	// 基本は0初期化。ただし0以外の値もあるので、それは自前で設定
	Tally* tl = &IW->mp.tally;
	DmaClear(3, 0, tl, sizeof(Tally), 32);
	tl->magic = FLOG_MAGIC_FLIGHT;
	tl->start_lat = tl->start_lon = -1;
	tl->max_up = tl->max_alt = -9999999;
	tl->min_up = tl->min_alt =  9999999;
	tl->min_G =                 99999;

	tl->keep_range = IW->tc.keep_range;
	IW->mp.flog_presave = -1; // 完全初期化
}


///////////////////////////////////////////////////////////////////////////////
// ウェイポイントダウンロード
///////////////////////////////////////////////////////////////////////////////

#define DL_DISP() (IW->gi.dl_accept & 1)

// 使用可能な文字だけを名前に追加
void NameCopy(u8* dst, const u8* src, u32 size){
	for(; size && *src ; --size, ++src) if(IsHalf(*src)) *dst++ = *src;
}

// ウェイポイントをリストに追加
u32 AddCommonWpt(const u8* name, u32 name_size, s32 alt, s32 lat, s32 lon){
	// ウェイポイント名検索(同名のウェイポイントが見つかったら上書き)
	u32 i;
	for(i = 0 ; i < WPT->wpt_count && strncmp(name, WPT->wpt[i].name, name_size) ; ++i);
	if(i >= MAX_WPT){
		if(DL_DISP()) Puts("Overflow ");
		return -1; // バッファ不足
	}

	// WPT変更/追加
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

	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ルート/WPT変更フラグをセット
	IW->wpt_sort_type = GetSortType(); // 再ソートのためのリセット
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
	// 進捗状況
	if(DL_DISP()){
		Putsf("%4.12m%d/%d", IW->gi.dl_count, IW->gi.dl_num);
		if(IW->gi.dl_num) Putsf("(%d%%)", RoundDiv(IW->gi.dl_count * 100, IW->gi.dl_num));
		Puts(": ");
	}

	// デバイスIDからD10?フォーマットを選択しても良いが、GPSを途中で変えた場合を考慮してデータサイズで自動判別する。
	// 推奨されていない使い方だが、ウェイポイントデータのみ他のGPSに差し替えてロードすることもありうるので…。
	if(len < 6) return -1; // ありえないサイズ
	if(len >= sizeof(D105_Wpt_Type)){
		if(len <  sizeof(D106_Wpt_Type)) return AddD105WPT((D105_Wpt_Type*)wpt, len); // D105? 11byte～
		if(len <  sizeof(D108_Wpt_Type)) return AddD106WPT((D106_Wpt_Type*)wpt, len); // D106? 25byte～
		if(len == sizeof(D150_Wpt_Type)) return AddD150WPT((D150_Wpt_Type*)wpt, len); // D150? 115byte
		if(len >= sizeof(D10X_Wpt_Type) && 0x20 <= wpt[0] && wpt[0] < 0x80) return AddD10XWPT((D10X_Wpt_Type*)wpt, len); // 56 byte～
		if(wpt[0] != 0x01) return AddD108WPT((D108_Wpt_Type*)wpt, len, cyl); // 49byte～
		if(wpt[3] == 0x70) return AddD109WPT((D109_Wpt_Type*)wpt, len, cyl); // 49byte～
		if(wpt[3] == 0x80) return AddD110WPT((D110_Wpt_Type*)wpt, len, cyl); // 49byte～
	}

	if(DL_DISP()) Puts("Unknown format");
	return -4; // 保留
}

///////////////////////////////////////////////////////////////////////////////
// ルートダウンロード
///////////////////////////////////////////////////////////////////////////////
// ルートの追加
#define InvalidName(v) (!IsHalf(v)) //((v) < 0x20 || 0x7e < (v))

u32 AddRteHdr(u8* rh, u32 len){
	// デバイスIDからD20?フォーマットを選択しても良いが、GPSを途中で変えた場合を考慮してデータサイズで自動判別する。
	// 推奨されていない使い方だが、ルートデータのみ他のGPSに差し替えてロードすることもありうるので…。

	//D201_Rte_Hdr_Type用のスキップ
#define D201_Rte_Hdr_Type_SIZE (21)
//	if(len == D201_Rte_Hdr_Type_SIZE && (InvalidName(rh[0]) || InvalidName(rh[1]) || InvalidName(rh[2]) || InvalidName(rh[3]))) rh += 4;
	// 汎用スキップ?
	for(; len && InvalidName(*rh) ; --len) ++rh;

	if(!len){
#define DEFAULT_ROUTE_NAME "NoName"
		rh = DEFAULT_ROUTE_NAME; // 1つしか登録できないが…
		len = sizeof(DEFAULT_ROUTE_NAME); // NULL入り
	} else {
		u32 max = len;
		if(max > ROUTE_NAMESIZE) max = ROUTE_NAMESIZE;
		for(len = 1 ; len < max && !InvalidName(rh[len]) ; ++len);
		if(len < ROUTE_NAMESIZE) rh[len++] = 0; // MAX以下の場合はNULLを含める
	}

	// D202_Rte_Hdr_Typeベース
	Locate(4, 16);
	// ルート名検索(同名のルートが見つかったら上書き)
	u32 i;
	for(i = 0 ; i < ROUTE->route_count && strncmp(rh, ROUTE->route[i].name, len) ; ++i);
	if(i >= MAX_ROUTE){
		PlaySG1(SG1_CLEAR); // 警告音
		if(DL_DISP()) Puts("ERROR! Route Overflow");
		return -1; // バッファ不足
	}

	// ルート変更/追加
	Route* dst = &ROUTE->route[i];
	if(i == ROUTE->route_count){
		ROUTE->route_count++;
		memset (dst->name, 0, ROUTE_NAMESIZE);
		strncpy(dst->name, rh, len);
	}
	if(DL_DISP()) PutsNameB(dst->name);

	// ルート初期化
	dst->count = 0;
	dst->dist = 0;

	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ルート/WPT変更フラグをセット
	return i; // 次にRteHdrが来るまで、このルートにウェイポイントを追加する
}

// ルートウェイポイントの追加
u32 AddRteWpt(u8* wpt, u32 len){
	// サイズチェック
	if(IW->gi.dl_route == -1 || IW->gi.dl_route >= ROUTE->route_count) return -1; // ルートが追加できない
	Route* dst = &ROUTE->route[IW->gi.dl_route];
	if(dst->count >= MAX_ROUTE_PT) return -2; // これ以上ルートウェイポイントを追加できない

	u16 cyl = 0;
	u32 i = AddWpt(wpt, len, &cyl);
	if(i == -1) return -3; // これ以上ウェイポイントが追加できない

	// ルートへ追加
	if(dst->count){
		// 次のデータ待ちまでの間に総距離を計算しておく
		u32 len;
		CalcWptDist(&WPT->wpt[dst->py[dst->count - 1].wpt],  &WPT->wpt[i], &len, 0);
		dst->dist += len;
	}
	Pylon* py = &dst->py[dst->count++];
	py->wpt = i;
	if(cyl == IW->tc.cylinder) py->cyl = -1; // デフォルト
//	else if(cyl == 0xffff)     py->cyl =  0; // -1が指定された場合に0mを使う
	else                       py->cyl =  cyl;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// トラックダウンロード
///////////////////////////////////////////////////////////////////////////////
u32 AddTrack(u8* trk, u32 len){
	// 進捗状況
	if(DL_DISP()){
		Putsf("%4.12m%d/%d", IW->gi.dl_count, IW->gi.dl_num);
		if(IW->gi.dl_num) Putsf("(%d%%)", RoundDiv(IW->gi.dl_count * 100, IW->gi.dl_num));
		Puts(": ");
	}
	if(len < 6) return -1; // ありえないサイズ

	TrackLog tl;
	u32 new_trk = 0;
	// トラックフォーマット判定
	// デバイスIDからD30?フォーマットを選択しても良いが、GPSを途中で変えた場合を考慮してデータサイズで自動判別する。
	// 推奨されていない使い方だが、途中で他のGPSに差し替えて使うこともありうるので…。
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
		IW->mp.tlog.abs_flag |= ABSF_SEPARETE; // セパレータ挿入
		IW->gi.dl_route++; // トラック番号表示用
	}
	TrackLogAdd(&tl);
	if(DL_DISP()) Putsf("%4.14mTrack#%d", IW->gi.dl_route);
	return -4; // 保留
}


///////////////////////////////////////////////////////////////////////////////
// Garmin リンクプロトコル TX
///////////////////////////////////////////////////////////////////////////////
// キャラクタ送信
void SioSend(u8 ch){
	u16 next = NextIndex(IW->tx_tail, sizeof(IW->tx_buf));
	if(next != IW->tx_head){ // 一応チェック
		IW->tx_buf[IW->tx_tail] = ch;
		IW->tx_tail = next;
	}
	// デバッグ用ダンプ(route領域が未使用の場合のみ)
	u32* tx_len = &IW->mp.last_route_len;
	if(!*tx_len || (*tx_len & DBGDMP_COM)){
		if((*tx_len & 0xfff) >= 256) *tx_len = 0;
		IW->mp.last_route[(*tx_len)++] = ch;
		*tx_len |= DBGDMP_COM;
	}
}

// DLEスタッフィング
void SioSendX(u8 ch){
	if(ch == DLE) SioSend(ch);
	SioSend(ch);
}

#define START_TX_FIFO_INT() (REG32(REG_TM2D) = 0x00c0ffff) 
// LP送信
EStatus LpSend(u8 pid, const u8* buf, u8 len){
	// 送信エラーチェック
//	if(IW->tx_head != IW->tx_tail)         return ERROR_SENDING; // ACK制御のため多重転送はガードする
	if((u16)len * 2 >= sizeof(IW->tx_buf)) return ERROR_TOO_BIG_PACKET;

	// ヘッダ送信
	SioSend (DLE);
	SioSend (pid);
	SioSendX(len);
	pid += len; // 以降pidはチェックサムに使用

	// データ送信
	while(len--){
		pid += *buf;
		SioSendX(*buf++);
	}

	// トレイラ送信
	SioSendX(-pid); // チェックサム(2の補数)
	SioSend (DLE);
	SioSend (ETX);
	START_TX_FIFO_INT(); // FIFO送信割り込み開始
	return ERROR_SUCCESS;
}

// ACKは2byte。(GPS60は2byte ACKしか受け付けない。他は1byte/2byteどちらでもOKなので2byteで統一)
static inline EStatus LpSendAck(u16 pid){
	return LpSend(Pid_Ack_Byte, (u8*)&pid, 2);
}
static inline EStatus LpSendNak(u16 pid){
	return LpSend(Pid_Nak_Byte, (u8*)&pid, 2);
}
EStatus LpSendCmd(u16 cmd){
	return LpSend(Pid_Command_Data, (u8*)&cmd, 2);
}

// NMEAデバイス用コマンド送信
extern const u8 V_HEX[];
EStatus NMEASend(const u8* buf){
	// START送信
	SioSend('$');

	// データ送信
	u8 sum = 0;
	while(*buf){
		SioSend(*buf);
		sum ^= *buf++;
	}

	// SUM送信
	SioSend('*');
	SioSend(V_HEX[sum >> 4]);
	SioSend(V_HEX[sum & 0xf]);
	SioSend('\r');
	SioSend('\n');

	START_TX_FIFO_INT(); // FIFO送信割り込み開始
	return ERROR_SUCCESS;
}

// ATコマンド送信(Bluetoothユニット用)
EStatus ATCSend(const u8* buf, u32 max){
	// データ送信
	while(*buf && max--) SioSend(*buf++); // "+++"エスケープは特にしない
	START_TX_FIFO_INT(); // FIFO送信割り込み開始
	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// リンクプロトコル RX
///////////////////////////////////////////////////////////////////////////////
//受信したパケットの確認。0が成功
EStatus CheckPacket(){
	// サイズチェック
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
// データリンクプロトコルタスク
///////////////////////////////////////////////////////////////////////////////
s32 MT_DataLink(){
	// FSMチェック
	if(IW->dl_fsm >= DL_FSM_COMPLETE){
		 // アプリ処理待ち
		if(IW->rx_head != IW->rx_tail) IW->intr_flag = 1;// 受信データあり。擬似割り込みセット
		return 0;
	}
	// UARTエラーチェック
	if(IW->uart_error || IW->rx_drop){
		IW->dl_drop += InterlockedExchange(&IW->rx_drop, 0);
		IW->dl_drop_pkt++;
		if(InterlockedExchange(&IW->uart_error, 0)){
			IW->dl_fsm = DL_FSM_E_UART; // こちらのエラーが優先
		} else {
			IW->dl_fsm = DL_FSM_E_DROP;
		}
		return 0;
	}

	// 受信データの確認
	while(IW->rx_head != IW->rx_tail){
		// デバッグ用ダンプ(wpt領域が未使用の場合のみ)
		u8 ch = IW->rx_buf[IW->rx_head];
		u32* rx_len = &IW->mp.last_wpt_len;
		if(!*rx_len || (*rx_len & DBGDMP_COM)){
			if((*rx_len & 0xfff) >= 256) *rx_len = 0;
			IW->mp.last_wpt[(*rx_len)++] = ch;
			*rx_len |= DBGDMP_COM;
		}

		// パケットバッファサイズ確認
		if(IW->dl_size >= sizeof(IW->dl_buf)){
			// バッファ不足!?
			IW->dl_fsm = DL_FSM_E_PACKET;
			IW->dl_drop_pkt++;
			IW->rx_head = IW->rx_tail;
			return 0;
		}

		// 受信データ処理
		IW->rx_head = NextIndex(IW->rx_head, sizeof(IW->rx_buf));
		switch(IW->dl_fsm){
		case DL_FSM_WAIT_DLE:
			 // GARMIN/NMEA自動判別
			if(ch == DLE) IW->dl_fsm = DL_FSM_WAIT_PID; // GARMIN
			if(ch == '$') IW->dl_fsm = DL_FSM_NMEA_SUM; // NMEA
			continue; // DLEはパケットに追加しない

		case DL_FSM_WAIT_PID:
			// シーケンスエラーチェック
			if(ch == DLE){ // 予期しない再DLE
				IW->dl_drop_pkt++;
				continue; // 今のchを先頭DLEとして再同期
			}
			if(ch == ETX){ // 予期しない終端
				IW->dl_drop_pkt++;
				IW->dl_fsm = DL_FSM_WAIT_DLE; // 先頭DLEへ再同期
				IW->dl_size = 0;
				continue;
			}
			// 正常に受信
			IW->dl_fsm = DL_FSM_WAIT_ANY;
			break;

		case DL_FSM_WAIT_ANY:
			if(ch != DLE) break; // 受信継続
			IW->dl_fsm = DL_FSM_GET_DLE;
			continue; // 次のchで処理を決める

		case DL_FSM_GET_DLE:
			if(ch == DLE){ // DLEスタッフィング
				IW->dl_fsm = DL_FSM_WAIT_ANY;
				break;
			}
			if(ch == ETX){ // 受信完了
				if(CheckPacket()){
					// パケットに難あり
					IW->dl_drop_pkt++;
					IW->dl_fsm = DL_FSM_WAIT_DLE; // 先頭DLEへ再同期
					IW->dl_size = 0;
					LpSendNak(IW->dl_buf[0]);
					continue;
				}
				// 正常受信(ACK制御)
				switch(IW->dl_buf[0]){
				case Pid_Ack_Byte:
				case Pid_Nak_Byte:
				case Pid_Pvt_Data:
					break; // ACK不要
				default:
					LpSendAck(IW->dl_buf[0]);
				}
				IW->dl_fsm = DL_FSM_COMPLETE;
				return 0;
			}
			// 直前のDLEは先頭DLEとして再同期(今のchがPID)
			IW->dl_drop_pkt++;
			IW->dl_fsm = DL_FSM_WAIT_ANY;
			break;

		// 以降、NMEA-0183対応
		case DL_FSM_NMEA_SUM:
			// ACKチェック(ACKにはチェックサムがない)
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
			// 無効キャラクタを検出したらDLE再同期
			if(!IsHalf(ch)){
				IW->dl_fsm = (ch == DLE)? DL_FSM_WAIT_PID : DL_FSM_WAIT_DLE;
				IW->dl_drop_pkt++;
				IW->dl_size = 0;
				continue;
			}
			// センテンス中の予期しない開始マークも再同期
			if(ch == '$'){
				IW->dl_drop_pkt++;
				IW->dl_size = 0;
				continue;
			}
			// チェックサム到達
			if(ch == '*'){
				IW->dl_fsm = DL_FSM_NMEA_CR;
				IW->dl_nmea_chk = 0;
				IW->dl_nmea_idx = 0;
				continue;
			}
			break;

		case DL_FSM_NMEA_CR:
			if((ch == '\r' || ch == '\n') && IW->dl_nmea_idx == 2){ // 必ず2桁+CR
				// サムチェック
				u8 sum = 0;
				u8* p = IW->dl_buf + IW->dl_size;
				while(p != IW->dl_buf) sum ^= *--p;
				if(sum == IW->dl_nmea_chk){
					// 正常パケット受信
					IW->dl_fsm = DL_FSM_NMEA_END;
					return 0; // 完了
				}
				// サムエラー。再同期で復旧
			}
			// 予期しない開始マーク
			if(ch == '$' || ch == DLE){
				IW->dl_fsm = (ch == '$')? DL_FSM_NMEA_SUM : DL_FSM_WAIT_PID; // NMEA/GARMINで再同期
			} else {
				// チェックサム取得
				s32 hex = GetHex(ch);
				if(hex != -1){
					IW->dl_nmea_chk = IW->dl_nmea_chk * 0x10 + hex;
					IW->dl_nmea_idx++; // 2以上はエラーになるが、ここではスルー。
					continue; // 継続
				}
				IW->dl_fsm = DL_FSM_WAIT_DLE;
			}
			IW->dl_drop_pkt++;
			IW->dl_size = 0;
			continue;

		// 以降、Bluetooth ATコマンド制御用
		case DL_FSM_ATC_WAIT:
			if(ch == '\r' || ch == '\n'){
				IW->dl_fsm = DL_FSM_ATC_ANY;
				IW->dl_size = 0;
			}
			continue;
		case DL_FSM_ATC_ANY:
			if(ch == '\r' || ch == '\n'){
				if(!IW->dl_size) continue; // 有効データなし
				IW->dl_buf[IW->dl_size] = 0;
				IW->dl_fsm = DL_FSM_ATC_END;
				return 0; // 完了
			}
			if(!IsHalf(ch)){
				// 異常キャラクタを検出
				IW->dl_fsm = DL_FSM_ATC_WAIT; // 改行待ちで再同期
				continue;
			}
			break;
		}

		// 内部バッファへ追加
#define DL_BUF_END (sizeof(IW->dl_buf) - 1)
		if(IW->dl_size < DL_BUF_END) IW->dl_buf[IW->dl_size++] = ch;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// NMEA-0183デコーダ
///////////////////////////////////////////////////////////////////////////////
// x.y "."区切りの2整数を返す
char* NMEA_P_10(char* p, s32* a1, s32* a2, s32* a2_len){
	if(*p == ',') return 0; // 先頭','は空データ
	s32 sign = (*p != '-')? 1 : -1; // a2用
	s32 v1 = strtol(p, &p, 10), v2 = 0, v2_len = 0;
	if(*p == '.'){
		char* pre = ++p;
		v2 = strtol(p, &p, 10) * sign;
		v2_len = p - pre;
	}
	if(*p != ',') return 0; // 次終端ではない

	// 正常フィールドであることを確認後に引数をアップデート
	*a1 = v1;
	if(a2){
		*a2     = v2;
		*a2_len = v2_len;
	}
	return p + 1;
}

// 固定少数(1000倍)として戻す
char* NMEA_P_10K(char* p, s32* v){
	s32 v1, v2, len;
	p = NMEA_P_10(p, &v1, &v2, &len);
	if(!p) return 0;

	// 正常フィールドであることを確認後に引数をアップデート
	*v = v1 * 1000;
	if(len < 4) *v += v2 * INT_POS_TABLE[3 - len];
	else        *v += RoundDiv(v2,  INT_POS_TABLE[len - 3]);
	return p;
}

// 緯度/経度の度数を返す
char* NMEA_P_LatLon(char* p, s32* v){
	s32 v1, v2, len;
	p = NMEA_P_10(p, &v1, &v2, &len);
	if(!p || p[1] != ',') return 0; // p[0]は'NESW'のいずれかだか、これは後でチェック

	// 正常フィールドであることを確認後に引数をアップデート
	s32 d = BiosDiv(v1, 100, &v1);
	*v  = d * (60 * 60 * 1000) + v1 * (60 * 1000);
	if(len < 5) *v += v2 * 6 * INT_POS_TABLE[4 - len];
	else        *v += RoundDiv(v2 * 6,  INT_POS_TABLE[len - 4]);
	return p;
}

// 省略フィールド
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

// 高度 "xx.xx" + "M" (海抜かWGS84楕円体高度をgf選択)
char* NMEA_P_Alt(char* p, s32 f){
	if(!f){
		// 高度の2フィールド分をスキップ
		p = NMEA_Nop(p);
		if(p) p = NMEA_Nop(p);
		return p;
	}
	p = NMEA_P_10K(p, &f);
	if(!p || p[0] != 'M' || p[1] != ',') return 0;

	// 正常フィールドであることを確認後にパラメータをアップデート
	IW->px2.alt_mm = f;
	return p + 2;
}

///////////////////////////////////////////////////////////////////////////////
// フィールド解析用コールバック関数
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
// 緯度 "DDDMM.mmmm" + [N/S]
char* NMEA_Lat(char* p){
	s32 v;
	p = NMEA_P_LatLon(p, &v);
	if(!p) return 0;
	if(*p == 'S') v = -v;
	else if(*p != 'N') return 0;

	IW->px2.lat = v;
	return p + 2;
}
// 経度 "DDDMM.mmmm" + [E/W]
char* NMEA_Lon(char* p){
	s32 v;
	p = NMEA_P_LatLon(p, &v);
	if(!p) return 0;
	if(*p == 'W') v = -v;
	else if(*p != 'E') return 0;

	IW->px2.lon = v;
	return p + 2;
}
// 測位状況 [1|2|3]
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
// 測位状況2 [0|1]
char* NMEA_Valid(char* p){
	// 基本的には、より情報の多いNMEA_Statusを尊重するが、GSAを5秒間隔でしか
	// 送らないGPSがいるため、GGAでもValidチェックだけはする
	s32 v;
	p = NMEA_P_10(p, &v, 0, 0);
	if(!p) return p;

	// Invalid/Validが違う場合だけ設定する
	if(v){
		if(IW->px2.fix <  FIX_2D) IW->px2.fix = FIX_2D;
	} else {
		if(IW->px2.fix >= FIX_2D) IW->px2.fix = FIX_INVALID;
	}
	return p;
}
// 衛星補足数(PvtXには含まれないが調査用に取得しておく) "xx"
char* NMEA_SatNum(char* p){
	return NMEA_P_10(p, &IW->nmea_log[NMEA_LOG_ID_SAT_NUM], 0, 0); // デバッグ用
}

// 測位劣化係数 "xx.xx"
char* NMEA_PDOP(char* p){
	return NMEA_P_10K(p, &IW->px2.epe_mm); // P-DOPはepeに格納しておく
}
char* NMEA_HDOP(char* p){
	return NMEA_P_10K(p, &IW->px2.eph_mm); // H-DOPはephに格納しておく
}
char* NMEA_VDOP(char* p){
	return NMEA_P_10K(p, &IW->px2.epv_mm); // V-DOPはepvに格納しておく
}

// 高度 "xx.xx" + "M"
char* NMEA_Alt(char* p){
	return NMEA_P_Alt(p, !IW->tc.alt_type); // 海抜高度 + [M]
}
char* NMEA_Geo(char* p){			
	return NMEA_P_Alt(p,  IW->tc.alt_type); // 平均海水面高度 + [M]
}

// 速度[Knot]
char* NMEA_Knot(char* p){
	s32 v;
	p = NMEA_P_10K(p, &v);
	if(!p) return 0;
	if(v < 9999990){ // 無効時("9999.99")は値を更新しない
		// 最大1000.000,Nと仮定し10bitシフトで計算(1knot ≒ 0.5144444 m/s)
		IW->px2.vh_mm = (v * 527) >> 10;
	}
	return p;
}

// 方位
char* NMEA_Dir(char* p){
	s32 v;
	p = NMEA_P_10K(p, &v);
	if(!p) return 0;
	if(v < 999990){ // 無効時("999.99")は値を更新しない
		// 最大360.000なので15bitシフトで計算
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
/*NMEA_Dateでまとめて取得するので不要
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
	NMEA_Valid, // 測位状態[0/1]
	NMEA_SatNum,// SatNum
	NMEA_HDOP,	// hh.h
	NMEA_Alt,	// hhhhh.h,M
	NMEA_Geo,	// ggggg.g,M
//	NMEA_Nop,	// DGPS_AGE,
//	NMEA_Nop,	// DGPS_ID,
	0
};
const NMEAProc NMEA_SEQ_GSA[] = {
	NMEA_Nop,	// ModeA/B (NMEA_Status以外は使わない)
	NMEA_Status,// 測位状態
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
	// 使わない…
	0
};
const NMEAProc NMEA_SEQ_RMC[] = {
	NMEA_Time,	// hhmmss.sss
	NMEA_Nop,	// Status A/V (NMEA_Status以外は使わない)
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
	// 使わない…
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
	// 使わない…
//	NMEA_Time,
//	NMEA_Day,
//	NMEA_Month,
//	NMEA_Year,
//	NMEA_Nop,	// Hour
//	NMEA_Nop,	// Min
	0
};
const NMEAProc NMEA_SEQ_GLL[] = {
	// 使わない…
//	NMEA_Lat,
//	NMEA_Lon,
//	NMEA_Time,
//	NMEA_Nop,	// StatusA
//	NMEA_Nop,	// ModeA
	0
};
const NMEAProc NMEA_SEQ_ALM[] = {
	// 使わない…
	0
};
//GPS-52D: GGA→GSA→GSV→RMC→VTG→ZDA（1秒間隔）
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

	// NMEAセンテンスをデバッグ用に保存
	if(IW->tc.log_debug == 1){
		u32 AppendNMEALog();
		AppendNMEALog();
	}
	
	// talkerチェック
	if(p[0] != 'G' || p[1] != 'P'){
		IW->nmea_log[NMEA_LOG_ID_TALKER_ERR]++;
		return 1; // GPSからのコマンド以外は無視
	}

	// 初回はNMEAデバイスが接続されたことを記録する
	if(IW->gi.pid != NMEA_DEV_ID){
		IW->gi.pid = NMEA_DEV_ID;
		IW->gi.version = 0;
		strcpy(IW->gi.name, "NMEA");
	}
	IW->gi.timeout = 0; // NMEAの応答がある間はタイムアウトをガードする
	if(IW->px.fix == FIX_UNUSABLE) IW->px.fix = FIX_INVALID; 

	// NMEA ID別処理
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

	// デバッグ用ログ
	IW->nmea_log[id]++;
	if(id == NMEA_LOG_ID_UNKNOWN) return 2;
	if(id == NMEA_LOG_ID_GGA){ // GGA受信でバッファ先頭から詰め直す
		IW->mp.last_pvt_len = DBGDMP_COM; 
		IW->mp.last_pvt_len = DBGDMP_COM;
	}
	s32 len = IW->dl_size;
#define NMEA_DBG_BUF 512 // PVTの後ろも連結して使う
	if(IW->mp.last_pvt_len + len > NMEA_DBG_BUF) len = NMEA_DBG_BUF - IW->mp.last_pvt_len;
	if(len > 0){
		memcpy(IW->mp.last_pvt + (IW->mp.last_pvt_len & (NMEA_DBG_BUF - 1)), p, len);
		IW->mp.last_pvt_len += len;
	}

	// フィールド解析
	strcpy(p + IW->dl_size, ","); // 終端マークしておく
	p += 6; // ヘッダスキップ
	const NMEAProc* fn = NMEA_SEQ_TABLE[id];
	for(; *fn ; ++fn){
		if(!(p = (*fn)(p))){
			IW->nmea_log[NMEA_LOG_ID_FIELD_ERR]++;
			return 3; // エラー
		}
	}

	// フィールド完了チェック
#define NMEA_COMPLETE	((1 << NMEA_LOG_ID_GGA) | (NMEA_LOG_ID_RMC)) // GGA/RMCでOK(GSAはオプション扱い)
#define NMEA_SYNC_ID	NMEA_LOG_ID_RMC // RMCを完了同期に使う(RMCが最後ではない場合は…?)
#define WGS84_TIMEOUT	3				// GPS52D以外が接続されている場合を考慮して、一定期間でタイムアウトする
	IW->nmea_fix |= 1 << id; // TID単位でチェック。フィールド単位が良い?
	if(id == NMEA_SYNC_ID && (IW->nmea_fix & NMEA_COMPLETE) == NMEA_COMPLETE){
		// GPS52DはデフォルトがWGS84ではない！データが受信できたココでモード切替実行
		if(IW->nmea_wgs84 < WGS84_TIMEOUT){
			if(IW->dl_nmea_ack == 106){
				// 切り替え完了
				IW->nmea_wgs84 = WGS84_TIMEOUT + 1; // デバッグ用にタイムアウトと完了を区別できるようにしておく
			} else {
				// WGS84に切り替え実行 (それにしても、デフォルトがWGS84でないGPSがまだあるとは…)
				NMEASend("PSRF106,21"); // ACKが返るまで1秒に1回再送する
				IW->nmea_wgs84++;
			}
		}

		// データを使用するのはWGS84モード切替後　(106応答しないGPSの場合は最初の3パケットを無視することになるが…)
		if(IW->nmea_wgs84 >= WGS84_TIMEOUT) CalcPvtNMEA(&IW->px); 
		IW->nmea_fix = 0;
	}
	
	return 0; // success
}

///////////////////////////////////////////////////////////////////////////////
// Bluetooth ATコマンド制御 (for ParaniESD100/110/200/210)
///////////////////////////////////////////////////////////////////////////////
u32 IsBluetoothAddr(const u8* p){
	const u8* end = p + 12;
	while(p < end) if(GetHex(*p++) < 0) return 0;
	return 1; // 12桁すべてHexならBluetoothアドレスとする
}
void ATCCheck(){
	// AT制御中はul_acknakを使わないので、ここにATC応答を入れておく
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
		u8* mode = strstr(IW->dl_buf, ",MODE"); // MODEで始まるデバイスは…
		if(mode){
			// INFOはここで格納
			ATC_PTR->last_mode = mode[5] - '0'; 
		} else {
			// INQ情報はここで格納
			u8* src = IW->dl_buf;
			u8* dst = ATC_PTR->inq_list[ATC_PTR->inq_count++];
			u8* end = dst + MAX_ATC_RESPONSE;
			for(; dst < end && *src ; ++src, ++dst) *dst = IsHalf(*src)? *src : '_'; // 印字不可のキャラクタはエスケープしておく
			*dst = 0;
			PlaySG1(SG1_COMP1);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Garminアプリケーションプロトコルタスク
///////////////////////////////////////////////////////////////////////////////

void GpsEmuMode(u32 mode, const u16* snd){
	if(snd) PlaySG1(snd);
	IW->gi.state    = mode;
	IW->gi.dl_count = 0;
	IW->gi.emu_nak  = 0;
}

s32 MT_AppLayer(){
	// 状態遷移管理
	if(IW->dl_fsm < DL_FSM_COMPLETE){
		if(IW->px.rx_pre != IW->rx_tail){
			IW->px.rx_pre = IW->rx_tail;
			return 0;
		}
		return 0; // 特に何もなし
	}

	// NMEA処理はここでチェック
	if(IW->dl_fsm == DL_FSM_NMEA_END){
		IW->mp.auto_off_cnt = IW->vb_counter;
		NMEADecode();
		IW->dl_fsm = DL_FSM_WAIT_DLE;
		IW->dl_size = 0;
		return 1; // 次キャラクタのチェック
	}

	// Bluetooth ATコマンド制御はここでチェック
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

	// パケット処理
	IW->mp.auto_off_cnt = IW->vb_counter;
	switch(IW->dl_buf[0]){
	// シーケンス /////////////////////////////////////////
	case Pid_Ack_Byte:
	case Pid_Nak_Byte:
		IW->gi.ul_acknak = IW->dl_buf[0]; // IDは…とりあえずいいや。
		break;

	// PVT ////////////////////////////////////////////////
	case Pid_Pvt_Data:
		if(IW->dl_buf[1] != sizeof(D800_Pvt_Data_Type) ||
			CalcPvtGarmin((D800_Pvt_Data_Type*)(IW->dl_buf + 2), &IW->px)){
			// データ更新失敗
			IW->px.fix_sum[FIX_UNUSABLE]++;
			IW->mp.tlog.opt |= TLOG_OPT_LOST; // デコードエラー
		} else {
			IW->mp.test_mode = 0; // 受信したらテストモードを終了する
		}
		// デバッグ用
		IW->mp.last_pvt_len = MinS32((u32)IW->dl_buf[1] + 2, sizeof(IW->mp.last_pvt));
		memcpy(IW->mp.last_pvt, IW->dl_buf, IW->mp.last_pvt_len);
		break;

	// 製品情報 ///////////////////////////////////////////
	case Pid_Product_Data:
		if(IW->dl_buf[1] > 4){
			IW->gi.pid =     GetInt(IW->dl_buf + 2);
			IW->gi.version = GetInt(IW->dl_buf + 4);
			memcpy(IW->gi.name, IW->dl_buf + 6, IW->dl_buf[0] - 4);//最大251byteなので次のparrayにはみ出す場合があるが、この際構わない
			if(!memcmp(IW->gi.name, "ParaNavi", 8)){
				// GBAが接続された?
				GpsEmuMode(GPS_EMUL_FINISH, 0); // 特にやることは無いのでFINISHに入れておく
			}
		}
		break;

	case Pid_Protocol_Array:
		// これは使わない…
		break;

	// ダウンロード共通 ///////////////////////////////////
	case Pid_Records:
		if(IW->dl_buf[1] == 2){
			IW->gi.dl_num = GetInt(IW->dl_buf + 2);
			if(!IW->gi.dl_accept) IW->gi.dl_accept = 2; // PCからのGBAへアップロード時は非表示
		}
		break;

	case Pid_Xfer_Cmplt:
		IW->gi.dl_count = -1;
		IW->gi.dl_accept &= ~2; // 落とすのは2のみ。
		PlaySG1(SG1_CHANGE);
		break;

	// ルート /////////////////////////////////////////////
	case Pid_Rte_Hdr:
		if(IW->gi.dl_accept){
			IW->gi.dl_count++;
			IW->gi.dl_route = AddRteHdr(IW->dl_buf + 2, IW->dl_buf[1]);
			// ダウンロードは複数行われる可能性があるため、タスクへの自動設定はしない
			if(IW->gi.dl_route == IW->tc.route){
				IW->mp.navi_update |= NAVI_ROUTE_DL;
				IW->mp.pre_route = (Route*)-1;// タスクログも初期化
			}

			// デバッグ用
			IW->mp.last_route_len = MinS32((u32)IW->dl_buf[1] + 2, sizeof(IW->mp.last_route));
			memcpy(IW->mp.last_route, IW->dl_buf, IW->mp.last_route_len);
		}
		break;

	case Pid_Rte_Link_Data:
		if(IW->gi.dl_accept) IW->gi.dl_count++;
		break;

	// ウェイポイント /////////////////////////////////////
	case Pid_Rte_Wpt_Data:
	case Pid_Wpt_Data:
		if(IW->gi.dl_accept){
			// ウェイポイント追加
			IW->gi.dl_count++;
			if(IW->dl_buf[0] == Pid_Wpt_Data) AddWpt(IW->dl_buf + 2, IW->dl_buf[1], 0);
			else                              AddRteWpt(IW->dl_buf + 2, IW->dl_buf[1]);

			// タイムアウトを更新
			if(IW->gi.dl_accept == 2) IW->gi.timeout = 0;

			// デバッグ用
			IW->mp.last_wpt_len = MinS32((u32)IW->dl_buf[1] + 2, sizeof(IW->mp.last_wpt));
			memcpy(IW->mp.last_wpt, IW->dl_buf, IW->mp.last_wpt_len);
		}
		break;

	// トラック /////////////////////////////////////
	case Pid_Trk_Data:
		if(IW->gi.dl_accept){
			IW->gi.dl_count++;
			AddTrack(IW->dl_buf + 2, IW->dl_buf[1]);
		}
		break;
	case Pid_Trk_Hdr:
		// 特に何もしない
		break;

	// GPSエミュレーション /////////////////////////////////////
	case Pid_Product_Rqst:
		// PCからこのコマンドを受けたらGPSエミュレーションモードに入る
		GpsEmuMode(GPS_EMUL_INFO, 0);
		break;

	case Pid_Command_Data:
		// エミュレーションモードでサポートしているコマンドを処理
		switch(GetInt(IW->dl_buf + 2)){
		case Cmnd_Transfer_Wpt:
			GpsEmuMode(GPS_EMUL_WPT, SG1_MODE2);
			break;

		case Cmnd_Transfer_Rte:
			GpsEmuMode(GPS_EMUL_ROUTE, SG1_MODE3);
			break;

		case Cmnd_Transfer_Trk:
			if(IW->mp.tlog.tc_state != TC_COMPLETE){
				// トラック数のカウント中は応答できない。(コマンド受付後にカウント開始すると
				// カシミールがタイムアウトするので、予め総数を数えておく必要がある)
				PlaySG1(SG1_CANCEL);
				SelectMap(MAP_BG0);
				FillBox(6, 12, 23, 17);
				DrawTextCenter(14, "Please wait...");
				SetBootMsgTimeout(30);
				break;
			}
			memset(&IW->gi.tinfo, 0, sizeof(TrackInfo));
			IW->gi.tinfo.pre_sect = -1; // 無効セクタを指しておく
			IW->gi.tinfo.tpos     = IW->mp.tlog.tc_tpos;
			IW->gi.ul_track       = IW->mp.tlog.tc_total;
			IW->gi.ul_pre         = 0;
			GpsEmuMode(GPS_EMUL_TRACK, SG1_MODE4);
			break;

		case Cmnd_Start_Pvt_Data:
			// 強引だが、Pid_Product_Dataを返して、相手に接続先がGBAであることを知らせる
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
// GPS制御タスク
///////////////////////////////////////////////////////////////////////////////

const u32 PINFO_TIMEOUT	= VBC_TIMEOUT(3);	// 初回だけなので、頻繁にチェックする
const u32 PVT_TIMEOUT	= VBC_TIMEOUT(10);	// PVTは途中で抜けても復帰するので、長めのタイムアウト
const u32 DL_TIMEOUT	= VBC_TIMEOUT(5);	// この操作をするときは確実に接続されている可能性が高い?
const u32 EMUL_TIMEOUT	= VBC_TIMEOUT(10);	// この時間、新コマンドが無ければエミュレータモード(対PC通信)を抜けて対GPS通信に復帰する
const u32 ACK_TIMEOUT	= VBC_TIMEOUT(1);	// エミュレータモードの再転送は速やかに行う(PCとの通信は普通切れない)
const u32 BTPPP_TIMEOUT	= VBC_TIMEOUT(3);	// Bluetoothスタンバイコマンドは速やかに応答するはず
const u32 BTBOOT_TIMEOUT= VBC_TIMEOUT(4);	// Bluetoothリブートは長めに待つ
const u32 BTCMD_TIMEOUT	= VBC_TIMEOUT(3);	// Bluetoothのコマンドも速やかに応答するはず
const u32 BTINQ_TIMEOUT	= VBC_TIMEOUT(35);	// Bluetooth INQは最大の30秒+α待機する
const u32 BTATH_TIMEOUT	= VBC_TIMEOUT(35);	// Bluetooth ATHは最大の30秒+α待機する
const u32 BTDIAL_TIMEOUT= VBC_TIMEOUT(305);	// BluetoothのDialは最大の5分+α待機する

const u32 EMU_MAX_ERROR	= 5;				// 通信異常が続いたらエミュレータモード(対PC通信)を抜けて対GPS通信に復帰する

u32 CalcNameLenX(const u8* p, u32 max){
	u32 len = 0;
	while(*p && len < max) ++len; // 転送は半角以外も転送(IsHalfのチェックはしない)
	return len;
}
#define CalcNameLen(n) (CalcNameLenX(n, sizeof(n)))

// PC転送用データを構築。wdはindent用バッファのため倍用意すること！
u32 MakeD108Wpt(D108_Wpt_Type wd[2], const Wpt* w, s32 cyl){
	// 各属性をデフォルト(0)で埋める
	memset(wd, 0, sizeof(D108_Wpt_Type) * 2);

	// 座標情報をセット
	SetLong (wd->lat, MyRound(w->lat * D10XLAT_i));
	SetLong (wd->lon, MyRound(w->lon * D10XLAT_i));
	SetFloat(wd->alt, w->alt);	// 高度は浮動小数点変換

	// シリンダサイズをセット (WPT/ROUTE転送の仕様で未定義のメンバだが、カシミールはこの情報を使ってくれる)
	if(cyl < 0) SetFloat(wd->dist, 1e25);
	else        SetFloat(wd->dist, cyl);

	// depthを設定
	SetFloat(wd->dpth, 1e25);

	// attrは仕様のとおり0x60をセット
	wd->attr = 0x60;

	// 名前は可変長
	u16 len = CalcNameLen(w->name); // 終端NULLはイラナイ?
	strncpy(wd->ident, w->name, len); // wd[1]の領域を使用

	// 構築完了。構築したWPTのサイズを返す。
	return sizeof(D108_Wpt_Type) + len + 6;
}

// トラックデータ作成
u32 MakeD301Trk(D301_Trk_Point_Type* tp){
	TrackInfo* ti = &IW->gi.tinfo;

	// トラックデータ詰め込み
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

// トラックデータヘッダ作成
const time_t UNIX2GARMIN = 631033200; // UNIX秒(1970)->GARMIN秒(1990)
u32 MakeD310TrkHead(u8* p){
	TrackInfo* ti = &IW->gi.tinfo;

	p[0] = 1; // dspl  表示ON
	p[1] = 0; // color 黒

	// identには"2000/01/02-03:45"のような日時を入れてみる
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


// FSM制御用
void GpsCtrlWait(){
	IW->gi.vbc = IW->vb_counter;
	IW->gi.timeout = 0;
	IW->gi.state++;
	IW->gi.ul_acknak = 0;
}

s32 MT_GPSCtrl(){
	// ボーレートが違った場合はここで設定
	if(IW->mp.pre_bt_mode != IW->tc.bt_mode){
		IW->mp.pre_bt_mode = IW->tc.bt_mode;
		StartGPSMode(IW->tc.bt_mode);
	}
	switch(IW->gi.state){
	// 製品情報 ///////////////////////////////////////////
	case GPS_GET_PINFO_WAIT: // 製品情報応答待ち
		if(IW->gi.pid){
			IW->gi.state = GPS_PVT;
			IW->px.pvt_timeout = 0;
			break;
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) < PINFO_TIMEOUT){ // タイムアウト未満
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
	case GPS_GET_PINFO: // 製品情報問い合わせ
		if(IW->px.fix == FIX_UNUSABLE) LpSend(Pid_Product_Rqst, 0, 0);
		GpsCtrlWait();
		break;

	// PVT取得 ////////////////////////////////////////////
	case GPS_PVT_WAIT:
		if(IW->gi.pre_pvt != IW->px.counter){
			IW->gi.pre_pvt = IW->px.counter;
			IW->gi.timeout = 0;
			break; // ステートはPVT_WAITのまま
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) < PVT_TIMEOUT){ // 10秒タイムアウト未満
			break;
		}
		IW->px.fix = FIX_UNUSABLE;
		IW->px.pvt_timeout = 1;
		IW->gi.state--;
		IW->dl_timeout++;
		IW->mp.tlog.opt |= TLOG_OPT_TIMEOUT; // タイムアウトでトラックログにセグメントフラグを設定
		IW->mp.navi_update |= NAVI_UPDATE_PVT| NAVI_PVT_CHANGE;

		// no break;
	case GPS_PVT:
		if(IW->px.fix == FIX_UNUSABLE) LpSendCmd(Cmnd_Start_Pvt_Data);
		GpsCtrlWait();
		break;

	// ダウンロード共通 ///////////////////////////////////
	case GPS_WPT_WAIT:
	case GPS_ROUTE_WAIT:
	case GPS_TRACK_WAIT:
		if(IW->gi.dl_count == -1){ // 完了
			IW->gi.state = GPS_PVT_WAIT;
			break;
		} else if(IW->gi.pre_count != IW->gi.dl_count){
			IW->gi.pre_count = IW->gi.dl_count;
			IW->gi.timeout = 0;
			break;
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) < DL_TIMEOUT){ // 5秒タイムアウト未満
			break;
		}
		IW->gi.state--;
		IW->dl_timeout++;
		break;

	case GPS_STOP_DOWNLOAD:
		// 中断コマンド
		LpSendCmd(Cmnd_Abort_Transfer);
		IW->gi.state = GPS_PVT_WAIT; // リトライは無し…
		break;

	// ウェイポイント取得 /////////////////////////////////
	case GPS_WPT:
		// WPT開始
		LpSendCmd(Cmnd_Transfer_Wpt);
		IW->gi.dl_count = IW->gi.dl_num = 0;
		GpsCtrlWait();
		break;

	// ルート取得 /////////////////////////////////////////
	case GPS_ROUTE:
		// Route開始
		LpSendCmd(Cmnd_Transfer_Rte);
		IW->gi.dl_count = IW->gi.dl_num = 0;
		IW->gi.dl_route = -1;
		GpsCtrlWait();
		break;

	// トラック取得 /////////////////////////////////////////
	case GPS_TRACK:
		// Track開始
		LpSendCmd(Cmnd_Transfer_Trk);
		IW->gi.dl_count = IW->gi.dl_num = IW->gi.dl_route = 0;
		GpsCtrlWait();
		break;

	// GPSエミュレーション /////////////////////////////////////////

	case GPS_EMUL_INFO_WAIT:
	case GPS_EMUL_WPT_WAIT:
	case GPS_EMUL_ROUTE_WAIT:
	case GPS_EMUL_TRACK_WAIT:
		switch(IW->gi.ul_acknak){
		case 0:
			// SELECTキーで通信強制中断
			if(IW->key_state == KEY_SELECT) IW->gi.emu_nak = EMU_MAX_ERROR; // &演算は使わず、単体チェック
			else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) < ACK_TIMEOUT) return 0; // タイムアウト1以内 (ウェイト)
			// no break;
		case Pid_Nak_Byte:
			if(IW->gi.emu_nak++ < EMU_MAX_ERROR) break; // リトライ時間内(再転送)
			//エミュレーションモードの終了
			PlaySG1(SG1_CANCEL);
			IW->gi.dl_accept &= ~2; // 転送中のデータは以降無視する
			IW->gi.state = GPS_GET_PINFO; // GPSと再同期を開始
			return 0;

		case Pid_Ack_Byte:
			//転送成功
			IW->gi.emu_nak = 0;
			IW->gi.dl_count++;
			break;
		}
		IW->gi.state--;
		return MT_GPSCtrl();
//		break;

	case GPS_EMUL_INFO:
		if(!IW->gi.dl_count){
			// GBAをGPSに見せかけてPC(カシミール等)と会話する
			const u8 GPS_ID[] = {"\x3a\x01\x01\x00ParaNavi GPS Emulator"}; // foretexに見せる
//			const u8 GPS_ID[] = {"\x3a\x99\x01\x00Test"}; // 未知のIDを返してカシミール上でGPSを選ばせるテスト
			LpSend(Pid_Product_Data, GPS_ID, sizeof(GPS_ID));
		} else {
			IW->gi.state = GPS_EMUL_FINISH;
			break;
		}
		GpsCtrlWait();
		break;

	case GPS_EMUL_WPT:
		if(!IW->gi.dl_count){
			// 総WPT数通知
			u16 n = (u16)WPT->wpt_count;
			if(n > MAX_WPT) n = MAX_WPT;
			LpSend(Pid_Records, (u8*)&n, sizeof(n));
		} else if(IW->gi.dl_count <= WPT->wpt_count){
			// WPT通知(高度情報を含み、任意長の名前が指定できるD108を使う)
			D108_Wpt_Type wd[2]; // wd[1]はindet用バッファ
			LpSend(Pid_Wpt_Data, (u8*)wd, MakeD108Wpt(wd, &WPT->wpt[IW->gi.dl_count - 1], -1));
		} else if(IW->gi.dl_count == WPT->wpt_count + 1){
			// WPT完了通知
			u16 n = Cmnd_Transfer_Wpt;
			LpSend(Pid_Xfer_Cmplt, (u8*)&n, sizeof(n)); // 完了通知
		} else {
			// 転送終了
			PlaySG1(SG1_CHANGE);
			IW->gi.state = GPS_EMUL_FINISH;
			break;
		}
		GpsCtrlWait();
		break;

	case GPS_EMUL_ROUTE:
		if(IW->gi.dl_count == 0){
			// 総Route/Wpt数通知
			u32 MP_SendRoute(u16 push);
			u32 i = IW->mp.route_sel; // ルート転送時の選択ルート番号 (符号無しに変換)
			if(IW->mp.proc == MP_SendRoute && i < ROUTE->route_count){
				// ルート転送メニューからの呼び出し時は選択しているルートのみを1つだけ転送する
				IW->gi.dl_route     = i;
				IW->gi.dl_route_end = i + 1;
			} else {
				// 全てのルートを送信する
				IW->gi.dl_route     = 0;
				IW->gi.dl_route_end = ROUTE->route_count;
			}
			u16 n = 0; // 送信データ数
			for(i = IW->gi.dl_route ; i < IW->gi.dl_route_end ; ++i) n += ROUTE->route[i].count + 1; // 各Wpt数を加算
			LpSend(Pid_Records, (u8*)&n, sizeof(n)); // サイズ通知
		} else if(IW->gi.dl_count == 1){
			if(IW->gi.dl_route >= IW->gi.dl_route_end){
				// Route完了通知
				u16 n = Cmnd_Transfer_Rte;
				LpSend(Pid_Xfer_Cmplt, (u8*)&n, sizeof(n)); // 完了通知
			} else {
				// ルート名通知
				Route* rt = &ROUTE->route[IW->gi.dl_route];
				LpSend(Pid_Rte_Hdr, rt->name, CalcNameLen(rt->name));
			}
		} else {
			if(IW->gi.dl_route >= IW->gi.dl_route_end){
				// 転送終了
				PlaySG1(SG1_CHANGE);
				IW->gi.state = GPS_EMUL_FINISH;
				break;
			}
			// 次ルートチェック
			Route* rt = &ROUTE->route[IW->gi.dl_route];
			u32 offset = IW->gi.dl_count - 2;
			if(offset >= rt->count){
				IW->gi.dl_route++;
				IW->gi.dl_count = 1;
				break; // 現在のルート完了
			}

			// ルートウェイポイントを転送
			D108_Wpt_Type wd[2];
			LpSend(Pid_Rte_Wpt_Data, (u8*)wd, MakeD108Wpt(wd, GetWptInfo(rt, offset), GetCurCyl(rt, offset)));
		}
		GpsCtrlWait();
		break;

	case GPS_EMUL_TRACK:
		// Track転送は圧縮x115k転送のGetTrack.exeの方がこのプロトコルより200倍程高速。
		// また、このプロトコルではMAX 65535トラックまでしか送れない。
		// コレをわざわざ使う人がいるか分からないが、とりあえず実装はしておく…。
		if(!IW->gi.dl_count){
			// 総Track数通知
			u16 n = IW->gi.ul_track;
			LpSend(Pid_Records, (u8*)&n, sizeof(n)); // サイズ通知 (最大65535までしか指定できない!)
		} else if(IW->gi.dl_count <= IW->gi.ul_track){ // トラック転送
			if(IW->gi.ul_pre != IW->gi.dl_count){
				IW->gi.ul_pre = IW->gi.dl_count;
				if(IW->gi.tinfo.seg == -1) IW->gi.tinfo.seg = 0;
				else {
					IW->gi.tinfo.pre_sect = -1;
					NextTrack(0); // 情報取得モード
					if(IW->gi.dl_count == 1) IW->gi.tinfo.seg = -1; // 先頭は強制セグメント
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
			// トラックログ完了通知
			u16 n = Cmnd_Transfer_Wpt;
			LpSend(Pid_Xfer_Cmplt, (u8*)&n, sizeof(n)); // 完了通知
		} else {
			// 転送終了
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
		// 一定時間PCからコマンドがなければ、タイムアウトして通常モードに戻る
		if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) >= EMUL_TIMEOUT){
			IW->gi.dl_accept &= ~2; // 転送中のデータは以降無視する
			IW->gi.state = GPS_GET_PINFO; // GPSと再同期を開始
		}
		break;

	// 以降、Bluetooth ATコマンド制御 (for ParaniESD100/110/200/210)
	// 共通WAIT処理
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
				IW->gi.state == GPS_BT_BR_CANCEL_WAIT){ // エラーでも可
				GpsCtrlWait(); // 成功(state++)
			} else {
				IW->gi.state = GPS_BT_ERROR; // シーケンス中断
			}
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) > BTCMD_TIMEOUT){
			IW->gi.state--; // リトライ
		}
		break;

	// ボーレートチェック＆変更シーケンス
	// 1. "+++"					"OK"		(ここで現在のボーレート確認)
	// 2. "AT+UARTCONFIG,BR..."	"OK"		(設定ボーレートが↑と違う場合のみ実行)
	// 3. "AT+BTCANCEL"			"OK/ERROR"	(念のためstandbyにセット)
	// 4. "ATH"					"OK+DISCONNECT/ERROR"	(念のため接続切断)
	// 5. "AT+BTINFO?"			"OK+status"	(モード切替が必要かチェック)
	// 6. "AT+BTMODE,0"			"OK"	 	(設定モードが0以外の場合のみ実行)
	// 7. "ATZ"					"OK"		(2 or 6 実行時のみ)
	case GPS_BT_BR_CHECK1:
	case GPS_BT_BR_CHECK2:
	case GPS_BT_BR_CHECK3:
	case GPS_BT_BR_CHECK4:
	case GPS_BT_BR_CHECK5:
		StartGPSMode(IW->tc.bt_mode + (IW->gi.state - GPS_BT_BR_CHECK1) / 2);
		ATCSend("+++\r", -1);
		GpsCtrlWait(); // GPS_BT_BR_WAITxへ
		break;

	// ボーレート変更用応答待ち処理
	case GPS_BT_BR_WAIT1:
	case GPS_BT_BR_WAIT2:
	case GPS_BT_BR_WAIT3:
	case GPS_BT_BR_WAIT4:
	case GPS_BT_BR_WAIT5: // デフォルトボーレートだけは2回チェック
		if(IW->gi.ul_acknak != ATCRES_NONE){ // 現ボーレートで通信可("OK"でも"ERROR"でも何でも良い)
			if(IW->gi.state == GPS_BT_BR_WAIT1 || IW->gi.state == GPS_BT_BR_WAIT5){
				IW->gi.state = GPS_BT_BR_CANCEL; // ボーレートの変更必要なし
				ATC_PTR->baudrate_err = 0;
			} else {
				IW->gi.state = GPS_BT_BR_CHANGE; // ボーレート変更シーケンス開始
				ATC_PTR->baudrate_err = 1;
			}
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) > BTPPP_TIMEOUT){ // +++応答なし?
			if(IW->gi.state == GPS_BT_BR_WAIT5){
				IW->gi.state = GPS_BT_ERROR; // 全ボーレートで応答なし
			} else {
				IW->gi.state++; // 次のボーレートのチェックへ
			}
		}
		break;

	case GPS_BT_BR_CHANGE:
		// 設定されたボーレートに切り替える
		ATCSend("AT+UARTCONFIG,", -1);
		ATCSend(BAUDRATE_STR[IW->tc.bt_mode & 3], -1); // 設定されたボーレートに切り替え
		ATCSend(",N,1,1\r", -1); // ボーレート以降は固定
		GpsCtrlWait(); // GPS_BT_BR_CHANGE_WAITへ
		break;

	case GPS_BT_BR_CANCEL:
		ATCSend("AT+BTCANCEL\r", -1);
		GpsCtrlWait(); // GPS_BT_BR_CANCEL_WAITへ
		break;

	case GPS_BT_BR_ATH:
		ATCSend("ATH\r", -1);
		GpsCtrlWait(); // GPS_BT_BR_ATH_WAITへ
		break;

	case GPS_BT_BR_ATH_WAIT:
		// 接続中は"OK"+"DISCONNECTが必要。未接続時は"ERROR"のみでOK。
		if(IW->gi.ul_acknak == ATCRES_ERROR || IW->gi.ul_acknak == ATCRES_DISCONNECT){
			GpsCtrlWait(); // 成功(state++)
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) > BTCMD_TIMEOUT){
			IW->gi.state--; // リトライ
		}
		break;

	case GPS_BT_BR_INFO:
		ATCSend("AT+BTINFO?\r", -1);
		ATC_PTR->last_mode = -1;
		GpsCtrlWait(); // GPS_BT_BR_INFO_WAITへ
		break;
		
	case GPS_BT_BR_INFO_CHECK:
		if(ATC_PTR->last_mode)			IW->gi.state = GPS_BT_BR_MODE0; // モード切替要
		else if(ATC_PTR->baudrate_err)	IW->gi.state = GPS_BT_BR_RESET; // ボーレート変更要
		else                   			IW->gi.state = GPS_BT_BR_OK;
		break;

	case GPS_BT_BR_MODE0:
		ATCSend("AT+BTMODE,0\r", -1);
		GpsCtrlWait(); // GPS_BT_BR_MODE0_WAITへ
		break;

	case GPS_BT_BR_RESET:
		// 設定されたボーレート/モードに切り替えを反映させる
		ATCSend("ATZ\r", -1);
		// GpsCtrlWait(); // GPS_BT_BR_RESET_WAITへ
		IW->gi.state = GPS_BT_BR_BOOTSLEEP; // 新しいボーレートで応答が返るので、応答チェックは省略
		IW->gi.vbc = IW->vb_counter;
		break;

	case GPS_BT_BR_BOOTSLEEP:
		// リブート完了待ち
		if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) > BTBOOT_TIMEOUT) IW->gi.state = GPS_BT_BR_CHECK1; // 念のためボーレートチェックから再実行
		break;

	case GPS_BT_BR_OK:
		// メニュー側での処理待ち
		break;

	// スキャンシーケンス (スキャンをするとMODE1にいても切断して強制的にMODE0に戻る)
	// 1. "AT+BTINQ?"	"BtAddr,FName,Class" x N (最大:サーチ30秒, N:15)
	//					"OK"
	case GPS_BT_INQ_START:
		ATCSend("AT+BTINQ?\r", -1);
		GpsCtrlWait(); // GPS_BT_INQ_START_WAITへ
		break;

	case GPS_BT_INQ_WAIT:
		if(IW->gi.ul_acknak == ATCRES_OK){
			IW->gi.state++; // 成功
		} else if(IW->gi.ul_acknak != ATCRES_NONE){
			IW->gi.state = GPS_BT_ERROR; // シーケンス中断
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) > BTINQ_TIMEOUT){
			if(ATC_PTR->inq_count) IW->gi.state++; // 1つでも応答があれば選択可。(成功にしておく)
			else                   IW->gi.state--; // リトライ
		}
		break;

	case GPS_BT_INQ_DONE:
		break; // メニュー選択処理待ち。特に何もしない。

	// 接続シーケンス
	// 1. "AT+BTKEY="PIN""	"OK" (PINが空でない場合のみ設定)
	// 2. "ATDBtAddr"		"OK\nConnect BtAddr"
	// 3. "+++"				"OK\n"(以降、自動接続するための設定)
	// 4  "ATH"				"OK"
	// 5. "AT+BTMODE,1"		"OK" (モード切替)
	// 6. "ATZ"				"OK" (再起動で、Mode1切り替え反映)
	case GPS_BT_CON_SETKEY:
		if(!*IW->tc.bt_pin){
			// PIN指定しなし
			IW->gi.state = GPS_BT_CON_DIAL;
		} else {
			// 認証PIN設定
			ATCSend("AT+BTKEY=\"", -1);
			ATCSend(IW->tc.bt_pin, BT_PIN_LEN);
			ATCSend("\"\r", -1);
			GpsCtrlWait(); // GPS_BT_CON_SETKEY_WAITへ
		}
		break;

	case GPS_BT_CON_DIAL:
		// PIN設定
		ATCSend("ATD", -1);
		ATCSend(IW->tc.bt_addr, BT_ADDR_LEN);
		ATCSend("\r", -1);
		GpsCtrlWait(); // GPS_BT_CON_DIAL_WAITへ
		break;

	case GPS_BT_CON_DIAL_CHECK:
		if(IW->gi.ul_acknak == ATCRES_CONNECT){
			// 接続完了
			if(IW->key_state & KEY_SELECT){
				IW->gi.state = GPS_BT_CON_COMPLETE; // 接続後、モード切替せずにそのまま使用する隠しモード
			} else {
				IW->gi.state = GPS_BT_CON_STANDBY; // モード切替して電源ON時に自動接続できるよう設定
			}
		} else if(IW->gi.ul_acknak == ATCRES_ERROR){
			IW->gi.state = GPS_BT_ERROR; // 接続エラー
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) > BTDIAL_TIMEOUT){
			IW->gi.state = GPS_BT_ERROR; // 接続エラー
//			IW->gi.state = GPS_BT_CON_DIAL; // Dialからリトライ
		}
		break;
		// no break

	case GPS_BT_CON_STANDBY: // モード切替のために一度スタンバイに戻す
		ATCSend("+++\r", -1);
		GpsCtrlWait(); // GPS_BT_CON_STANDBY_WAITへ
		break;

	case GPS_BT_CON_ATH: // 切断
		ATCSend("ATH\r", -1);
		GpsCtrlWait(); // GPS_BT_CON_ATH_WAITへ
		break;

	case GPS_BT_CON_ATH_CHECK: // 切断チェック
		if(IW->gi.ul_acknak == ATCRES_DISCONNECT){
			IW->gi.state = GPS_BT_CON_MODECHANGE; // 切断したらモード切替が有効になる
		} else if(IW->gi.ul_acknak != ATCRES_NONE){ // 予期しない応答
			IW->gi.state = GPS_BT_ERROR; // シーケンス中断
		} else if((IW->gi.timeout += UpdateVBC(&IW->gi.vbc)) > BTATH_TIMEOUT){
			IW->gi.state = GPS_BT_CON_ATH; // リトライの意味ある?
		}
		break;

	case GPS_BT_CON_MODECHANGE: // モード切替実行
		ATCSend("AT+BTMODE,1\r", -1);
		GpsCtrlWait(); // GPS_BT_CON_MODECHANGE_WAITへ
		break;

	case GPS_BT_CON_RESET: // リブートで反映
		ATCSend("ATZ\r", -1);
		GpsCtrlWait(); // GPS_BT_CON_RESET_WAITへ
		break;

	case GPS_BT_CON_COMPLETE: // 成功終了
		break; // メニュー選択処理待ち。特に何もしない。

		
	case GPS_BT_SCAN_START:
		ATCSend("AT+BTMODE,3\r", -1);
		GpsCtrlWait(); // GPS_BT_SCAN_START_WAITへ
		break;

	case GPS_BT_SCAN_RESET: // リブートで反映
		ATCSend("ATZ\r", -1);
		GpsCtrlWait(); // GPS_BT_SCAN_RESET_WAITへ
		break;

	case GPS_BT_SCAN_COMPLETE: // 成功終了
		break; // メニュー選択処理待ち。特に何もしない。

	case GPS_BT_IDLE_START:
		// ボーレート変更シーケンスでモード切替しているので何もしなくて良い
		IW->gi.state = GPS_BT_IDLE_COMPLETE;
		break;

	case GPS_BT_IDLE_COMPLETE: // 成功終了
		break; // メニュー選択処理待ち。特に何もしない。
		
	case GPS_BT_ERROR: // 異常終了
		// エラー応答時はリトライ処理なし (メニュー処理待ち)
		break;

	}
	return 0;
}

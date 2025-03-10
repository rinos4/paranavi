///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// 距離方位の計算関係をこのファイルにまとめた
// 風速計算もここに追加


///////////////////////////////////////////////////////////////////////////////
// 三角関数
///////////////////////////////////////////////////////////////////////////////
const s32 SIN256[257] = {
      0,  201,  402,  603,  804, 1005, 1206, 1407, 1607, 1808, 2009, 2210, 2410, 2611, 2811, 3011,
   3211, 3411, 3611, 3811, 4011, 4210, 4409, 4609, 4808, 5006, 5205, 5403, 5602, 5800, 5997, 6195,
   6392, 6589, 6786, 6983, 7179, 7375, 7571, 7766, 7961, 8156, 8351, 8545, 8739, 8933, 9126, 9319,
   9512, 9704, 9896,10087,10278,10469,10659,10849,11039,11228,11416,11605,11793,11980,12167,12353,
  12539,12725,12910,13094,13278,13462,13645,13828,14010,14191,14372,14552,14732,14912,15090,15269,
  15446,15623,15800,15976,16151,16325,16499,16673,16846,17018,17189,17360,17530,17700,17869,18037,
  18204,18371,18537,18703,18868,19032,19195,19358,19519,19681,19841,20001,20159,20318,20475,20631,
  20787,20942,21097,21250,21403,21555,21706,21856,22005,22154,22301,22448,22594,22740,22884,23027,
  23170,23312,23453,23593,23732,23870,24007,24144,24279,24414,24547,24680,24812,24943,25073,25201,
  25330,25457,25583,25708,25832,25955,26077,26199,26319,26438,26557,26674,26790,26905,27020,27133,
  27245,27356,27466,27576,27684,27791,27897,28002,28106,28208,28310,28411,28511,28609,28707,28803,
  28898,28993,29086,29178,29269,29359,29447,29535,29621,29707,29791,29874,29956,30037,30117,30196,
  30273,30350,30425,30499,30572,30644,30714,30784,30852,30919,30985,31050,31114,31176,31237,31298,
  31357,31414,31471,31526,31581,31634,31685,31736,31785,31834,31881,31927,31971,32015,32057,32098,
  32138,32176,32214,32250,32285,32319,32351,32383,32413,32442,32469,32496,32521,32545,32568,32589,
  32610,32629,32647,32663,32679,32693,32706,32718,32728,32737,32745,32752,32758,32762,32765,32767,
  32767 // 最後は念のため線形補完用に確保
};

// 高速版sin. -0x7fff ～ 0x7fffを返す
s32 Sin64K(s32 angle){
	s32 x = (angle >> 6) & 0xff;
	switch((angle >> 14) & 3){
	case 0:	return  SIN256[x];			//  0    ～ π/2
	case 1: return  SIN256[255 - x];	// π/2  ～  π
	case 2: return -SIN256[x];			//  π   ～ π3/2
	case 3: return -SIN256[255 - x];	// π3/2 ～  2π
	}
	return 0;// ここには到達しない
}

// 線形補完で精度を稼ぐ
s32 Sin64KX(s32 angle){
	s32 lx, lo, hi = (angle >> 6) & 0xff;
	if(angle & 0x4000){
		hi = 0xff - hi;
		lo = angle & 0x3f;
		lx = 64 - lo;
	} else {
		lx = angle & 0x3f;
		lo = 64 - lx;
	}
	lx = (SIN256[hi] * lo + SIN256[hi + 1] * lx) / 64;
	return (angle & 0x8000)? -lx : lx;
}


///////////////////////////////////////////////////////////////////////////////
// 距離/方位計算
///////////////////////////////////////////////////////////////////////////////

// 近距離高速版
void CalcDist1(s32 lat0, s32 lon0, s32 lat1, s32 lon1, u32* len, u32* houi){
	// ヒューベニ拡張式値
	// lon lat10k  lon10k
	//  0:110.564  111.307
	// 10:110.600  109.627
	// 20:110.700  104.635
	// 30:110.849   96.475
	// 40:111.032   85.383
	// 50:111.226   71.686
	// 60:111.408   55.793
	// 70:111.555   38.181
	// 80:111.649   19.391
	// 89:111.680    1.949

	// 楕円体表面の円弧ではなく、直線距離で計算
	s32 ang64 = (lat0 + lat1) * (5.0567901234567901234567901234568e-5 / 2);
	s32 y = (lat1 - lat0) * 0.030866974806026297227396863865729  * (1 - Cos64KX(ang64 * 2) * 1.526e-7);
	s32 x = (lon1 - lon0) * 9.4360687750139522468485224973412e-7 * Cos64KX(ang64) *
		                    (1 - (Cos64KX(ang64 * 2) - 32767) * 5.1480147709585863826410718100528e-8);
	if(len)  *len  = BiosHypot  (x, y);
	if(houi) *houi = BiosAtan64K(x, y); // 北から時計回り
}

// 高精度版。中村式を使用。誤差は１０００ｋｍ以内なら±１ｍ以内。
	/*
（１）地球の形
　　赤道半径　　　Ｒ＝６,３７８,１３７〔ｍ〕
　　扁平率　　　　Ｄ＝１／２９８.２５７
　　離心率の２乗　Ｅ2＝Ｄ＊(２－Ｄ）
（２）２地点(ａ，ｂ）の緯度・経度
　　ａ地点　緯度　φa〔rad〕　経度　λa〔rad〕
　　ｂ地点　緯度　φb〔rad〕　経度　λb〔rad〕
　　ａｂ中間緯度　φc＝(φa＋φb)／２　　（これをｃ地点とする）
（３）曲率半径（東西方向）と離心偏位
　　ａ点曲率半径　Ｒa＝Ｒ／sqr(１－Ｅ2＊sin(φa)^2)
　　　　離心偏位　Ｅa＝Ｒa＊Ｅ2＊sin(φa)
　　ｂ点曲率半径　Ｒb＝Ｒ／sqr(１－Ｅ2＊sin(φb)^2)
　　　　離心偏位　Ｅb＝Ｒb＊Ｅ2＊sin(φb)
　　ｃ点曲率半径　Ｒc＝Ｒ／sqr(１－Ｅ2＊sin(φc)^2)
　　　　離心偏位　Ｅc＝Ｒc＊Ｅ2＊sin(φc)
（４）中間地点(ｃ）から見た仮の地心を基準とした緯度・半径
　　ａ地点　緯度　ψa＝atan(tan(φa)－(Ｅa－Ｅc)／Ｒa／cos(φa))
　　　　　　半径　Ｑa＝Ｒa＊cos(φa)／cos(ψa)
　　ｂ地点　緯度　ψb＝atan(tan(φb)－(Ｅb－Ｅc)／Ｒb／cos(φb))
　　　　　　半径　Ｑb＝Ｒb＊cos(φb)／cos(ψb)
　　ａｂ点地心角　θc＝acos(sin(ψa)＊sin(ψb)
　　　　　　　　　　　　　　＋cos(ψa)＊cos(ψb)＊cos(λb－λa))
（５）２地点間の楕円体表面距離
　　ａｂ直線距離　Ｌc＝sqr(Ｑa^2＋Ｑb^2－２＊Ｑa＊Ｑb＊cos(θc))
　　ｃ点　凸高さ　Ｈc≒Ｒc－Ｑa＊Ｑb＊sin(θc)／Ｌc
　　近似曲率半径　Ｒs≒(Ｌc^2／４＋Ｈc^2)／Ｈc／２
　　ａｂ表面距離　Ｌs≒２＊Ｒs＊asin(Ｌc／Ｒs／２)
 */
void CalcDist2(s32 lat0, s32 lon0, s32 lat1, s32 lon1, u32* len, u32* houi){
	// TODO 最適化の余地あり…
	const double r  = 6378137;
	const double d  = 1 / 298.257;
	const double e2 = d * (2 - d);
	const double PVTX2RAD = M_PI / (180 * 1000 * 60 * 60);

	double dlat0 = lat0 * PVTX2RAD;
	double dlat1 = lat1 * PVTX2RAD;
	double dlon0 = lon0 * PVTX2RAD;
	double dlon1 = lon1 * PVTX2RAD;
	double dlat2 = (dlat0 + dlat1) / 2;

	if(len){
		double t;
		t = sin(dlat0);
		double ra = r / sqrt(1 - e2 * t * t);
		t = sin(dlat1);
		double rb = r / sqrt(1 - e2 * t * t);
		t = sin(dlat2);
		double rc = r / sqrt(1 - e2 * t * t);
		double ea = ra * e2 * sin(dlat0);
		double eb = rb * e2 * sin(dlat1);
		double ec = rc * e2 * sin(dlat2);
		double pa = atan(tan(dlat0) - (ea - ec) / ra / cos(dlat0));
		double pb = atan(tan(dlat1) - (eb - ec) / rb / cos(dlat1));
		double qa = ra * cos(dlat0) / cos(pa);
		double qb = rb * cos(dlat1) / cos(pb);
		double c  = acos(sin(pa) * sin(pb) + cos(pa) * cos(pb) * cos(dlon1 - dlon0));

		*len = (u32)(sqrt(qa * qa + qb * qb - 2 * qa * qb * cos(c)));
	}
	if(houi){
		double lx = lat0 / (1000.0 * 60 * 60);

		double nc = (dlat0 + dlat1) / 2 - 35;
		double nd = (dlat1 - dlat0);
		double ed = (dlon1 - dlon0);
		double k2 = nd * nd * ed * ed;
		double a = nc * (nc + 162) / 9000 + 110.941;
		double b = nc * (nc * (nc - 257.5) - 20550) / 18500 + 91.284;
		double c = lx * (91 - lx) / 3650 - 0.080;
		double Lc = c * ed * ed;
		double l2 = a * a * nd * nd + b * b * ed * ed;
		double lq = sqrt(l2 + Lc * Lc * (lx - 32) / 5 + k2 * nc / 12);
		double q = acos((a * nd + Lc) / lq);

		if(ed < 0){
			*houi = (u32)((2 - q / M_PI) * 0x8000);
		} else {
			*houi = (u32)(q / M_PI * 0x8000);
		}
		*houi &= 0xffff;
	}
}

// 設定に応じてアルゴリズムを選択
void CalcDist(s32 lat0, s32 lon0, s32 lat1, s32 lon1, u32* len, u32* houi){
	// TODO オート時はlat/lonの差分概算で切り替える?
	if(IW->tc.calc_type < 2)	CalcDist1(lat0, lon0, lat1, lon1, len, houi);
	else                     	CalcDist2(lat0, lon0, lat1, lon1, len, houi);
}

void CalcWptDist(const Wpt* w0, const Wpt* w1, u32* len, u32* houi){
	if(w0 && w1) CalcDist(w0->lat, w0->lon, w1->lat, w1->lon, len, houi);
}


///////////////////////////////////////////////////////////////////////////////
// 風速計算
///////////////////////////////////////////////////////////////////////////////
const u32 LIMIT_MIN = 16384;		// 1 sec
const u32 LIMIT_MAX = 16384 * 10;	// 10 sec
const u32 AVG_TIME	= 16384 * 9 / 10; // 1 sec * 0.9
const u32 RPM_COEF	= 16384 * 60 * 1000;

void CalcAnemometer(){
	Anemometer* am = &IW->anemo;
	u32 dif = am->tm - am->pre_tm;
	if(dif > Range32(LIMIT_MIN, LIMIT_MAX, am->dif_tm << 3)){// 前回の8倍以上パルス無し
		// 停止検出
		am->rpm = 0;
		am->rpm_avg = 0;
		am->vel = 0;
		am->calc_flag = 1;
		return;
	}
	if(am->calc_flag) return; // 計算済み
	am->calc_flag = 1;

	// difが0になるのは初回のみ
	dif = am->dif_tm;
	if(!dif) return;

	// 風速計算
	am->rpm = RoundDiv(RPM_COEF, dif);

	// 平均値
	u32 cur_tm = am->tm, adif_tm = cur_tm - am->avg_tm;
	if(adif_tm > AVG_TIME){
		u32 cur_pulse = am->pulse, adif_pulse = cur_pulse - am->avg_pulse;
		if(adif_pulse){
			am->rpm_avg = RoundDiv(RPM_COEF, adif_tm) * adif_pulse;
			am->avg_pulse = cur_pulse;
			if(IW->tc.anemo_coef && IW->tc.anemo_coef != -1){
				am->vel = RoundDiv(IW->tc.anemo_coef, adif_tm) * adif_pulse;
			}
		} else {
			am->rpm_avg = am->rpm;
			if(IW->tc.anemo_coef && IW->tc.anemo_coef != -1){
				am->vel = RoundDiv(IW->tc.anemo_coef, dif);
			}
		}
		am->avg_tm = cur_tm;
	}
}

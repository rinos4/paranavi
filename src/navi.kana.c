///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// ナビゲーション機能をこのファイルにまとめた。


///////////////////////////////////////////////////////////////////////////////
// 定数
///////////////////////////////////////////////////////////////////////////////
#define NO_SECTOR	0x80000000

#define BN_SPEED	(OBJ_BIG_NUM_BEGIN + 0)
#define BN_LENGTH	(OBJ_BIG_NUM_BEGIN + 4)
#define BN_ALT		(OBJ_BIG_NUM_BEGIN + 8)
#define BN_LENGTH2	(OBJ_BIG_NUM_BEGIN + 8) // BN_ALTと同時には使わない
#define BN_ANEMO	(OBJ_BIG_NUM_BEGIN + 8) // BN_ALTと同時には使わない

// テキストモード専用
#define BN_LAT		(OBJ_BIG_NUM_BEGIN + 12)
#define BN_LON		(OBJ_BIG_NUM_BEGIN + 22)
#define BN_TIME		(OBJ_BIG_NUM_BEGIN + 32)

#define TYPE_SECT 0x0001
#define TYPE_GOAL 0x0011

const u8 STATE_STR[4] = { 'W', 'L', 'R', 'S' };

///////////////////////////////////////////////////////////////////////////////
// 回転設定
///////////////////////////////////////////////////////////////////////////////
void RotateMoveOut(u32 rn){
	vu16* p = (u16*)(OAM + (rn << 5));
	p[ 3] =  0;
	p[ 7] =   10000;
	p[11] =  -10000;
	p[15] =  0;
}

void RotateNaviWin(u32 rn, s32 ang64, u32 xmag, u32 ymag){
	vu16* p = (u16*)(OAM + (rn << 5));
	ang64 += (IW->tc.view_mode & 3) << 14;

	s16 s = Sin64K(ang64);
	s16 c = Cos64K(ang64);

	p[ 3] =  (c * xmag) >> 16;
	p[ 7] =  (s * xmag) >> 16;
	p[11] = -(s * ymag) >> 16;
	p[15] =  (c * ymag) >> 16;
}

// BG2の縦横設定
void SetBG2Mode(){
	switch(IW->tc.view_mode & 3){
	case 0:
		// 横
		REG32(REG_BG2X_L) = 0;
		REG32(REG_BG2Y_L) = 0;
		REG16(REG_BG2PA)  = 0x0100;
		REG16(REG_BG2PB)  = 0x0000;
		REG16(REG_BG2PC)  = 0x0000;
		REG16(REG_BG2PD)  = 0x0100;
		break;
	case 1:
		// 縦
		REG32(REG_BG2X_L) = 0x100 * 0;
		REG32(REG_BG2Y_L) = 0x100 * 239;
		REG16(REG_BG2PA)  = 0x0000;
		REG16(REG_BG2PB)  = 0x0100;
		REG16(REG_BG2PC)  =-0x0100;
		REG16(REG_BG2PD)  = 0x0000;
		break;
	case 2:
		// 横
		REG32(REG_BG2X_L) = 0x100 * 239;
		REG32(REG_BG2Y_L) = 0x100 * 159;
		REG16(REG_BG2PA)  =-0x0100;
		REG16(REG_BG2PB)  = 0x0000;
		REG16(REG_BG2PC)  = 0x0000;
		REG16(REG_BG2PD)  =-0x0100;
		break;
	case 3:
		// 縦
		REG32(REG_BG2X_L) = 0x100 * 159;
		REG32(REG_BG2Y_L) = 0x100 * 0;
		REG16(REG_BG2PA)  = 0x0000;
		REG16(REG_BG2PB)  =-0x0100;
		REG16(REG_BG2PC)  = 0x0100;
		REG16(REG_BG2PD)  = 0x0000;
		break;
	}
}


const s32 REVERSE_TABLE[16][2] = {
	{ 8, 8}, {16,16}, {32,32}, {64,64},
	{16, 8}, {32, 8}, {32,16}, {64,32},
	{ 8,16}, { 8,32}, {16,32}, {32,64},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
};
void SetNaviObj(u32 id, u32 tile, u32 size, s32 x, s32 y) {
	if((IW->tc.view_mode & 2) && y < 160){
		const s32* rt = REVERSE_TABLE[((size >> 14) & 0x3) | ((size >> 28) & 0xc)];
		if(!(size & 0x01000000)) size ^= A3_VFLIP | A3_HFLIP;
		if(  size & 0x02000000){
			x = 240 - (rt[0] << 1) - x;
			y = 160 - (rt[1] << 1) - y;
		} else {
			x = 240 - rt[0] - x;
			y = 160 - rt[1] - y;
		}
	}
	SetNormalObj(id, tile | A2_PRI2, size | A3_BLEND, x, y);
}

///////////////////////////////////////////////////////////////////////////////
// ナビウインドウ描画補助
///////////////////////////////////////////////////////////////////////////////
static void PointX(vu16* p, u32 x, u8 pal){
	if(x & 1)	*p = (*p & 0x00ff) | (pal << 8);
	else 		*p = (*p & 0xff00) |  pal;
}
void NDrawPoint(u32 offset, u32 xmask, u32 x, u32 y, u8 pal){
	if(x >= 64 || y >= 64) return; // クリッピング

	// テーブルを作っても良かったが…
	vu16* p = (u16*)(((x & 0x38) + (y & 7)) * 8 + (x & 6) + (y & 0x38) * xmask + offset);
	PointX(p, x, pal);
}

void NDrawHLine(u32 offset, u32 xmask, u32 x, u32 y, u32 w, u8 pal){
	while(w--) NDrawPoint(offset, xmask, x++, y, pal);
}
void NDrawRect(u32 offset, u32 xmask, u32 x, u32 y, u32 w, u32 h, u8 pal){
	while(h--) NDrawHLine(offset, xmask, x, y++, w, pal);
}

// アンチエリアス三角形
void MyTri(u32 offset, u32 xmask, s32 center, s32 top, s32 bottom, u32 w, const u8* pal){
	u32 i;
	for(i = 0 ; top < bottom ; ++top, i += w){
		u8 pal2 = pal[(i >> 6) & 3];
		u32 j = i >> 8;
		if(pal2 != 0xff){
			NDrawPoint(offset, xmask, center - 1 - j, top, pal2);
			NDrawPoint(offset, xmask, center + 1 + j, top, pal2);
		}
		NDrawHLine(offset, xmask, center - j, top, j * 2 + 1, pal[3]);
	}
}

///////////////////////////////////////////////////////////////////////////////
// オブジェクト作成
///////////////////////////////////////////////////////////////////////////////
//星
#define S_64 (8*8)
#define S_32 (4*8)
#define NW_OFFSET_NEXT (0x6010000 + OBJ_TN_NEXT * 32)
#define NW_OFFSET_PYLN (0x6010000 + OBJ_TN_PYLN * 32)
#define NW_OFFSET_BASE (0x6010000 + OBJ_TN_BASE * 32)

typedef struct {
	s32 tile;
	s32 size;
	s32 x;
	s32 y;
} CircleInfo;
const CircleInfo CIRCLE_INFO[] = {
#define MyCircle(a, s, x, y) {OBJ_TN_CIRCLE + (a) * 2, (s), (x), (y)}
	MyCircle( 0, OBJSIZE_32x8,                        0,-64),
	MyCircle( 4, OBJSIZE_32x8,                       16,-56),
	MyCircle( 8, OBJSIZE_16x8,                       32,-48),

	MyCircle(18, OBJSIZE_8x8  | A3_VFLIP,            40,-40),
	MyCircle(14, OBJSIZE_8x32 | A3_VFLIP,            48,-48),
	MyCircle(10, OBJSIZE_8x32 | A3_VFLIP,            56,-32),

	MyCircle(10, OBJSIZE_8x32,                       56,  0),
	MyCircle(14, OBJSIZE_8x32,                       48, 16),
	MyCircle(18, OBJSIZE_8x8,                        40, 32),

	MyCircle( 0, OBJSIZE_32x8 | A3_VFLIP,             0, 56),
	MyCircle( 4, OBJSIZE_32x8 | A3_VFLIP,            16, 48),
	MyCircle( 8, OBJSIZE_16x8 | A3_VFLIP,            32, 40),

	MyCircle( 0, OBJSIZE_32x8 | A3_VFLIP | A3_HFLIP,-32, 56),
	MyCircle( 4, OBJSIZE_32x8 | A3_VFLIP | A3_HFLIP,-48, 48),
	MyCircle( 8, OBJSIZE_16x8 | A3_VFLIP | A3_HFLIP,-48, 40),

	MyCircle(10, OBJSIZE_8x32 |            A3_HFLIP,-64,  0),
	MyCircle(14, OBJSIZE_8x32 |            A3_HFLIP,-56, 16),
	MyCircle(18, OBJSIZE_8x8  |            A3_HFLIP,-48, 32),

	MyCircle(18, OBJSIZE_8x8  | A3_VFLIP | A3_HFLIP,-48,-40),
	MyCircle(14, OBJSIZE_8x32 | A3_VFLIP | A3_HFLIP,-56,-48),
	MyCircle(10, OBJSIZE_8x32 | A3_VFLIP | A3_HFLIP,-64,-32),

	MyCircle( 0, OBJSIZE_32x8 |            A3_HFLIP,-32,-64),
	MyCircle( 4, OBJSIZE_32x8 |            A3_HFLIP,-48,-56),
	MyCircle( 8, OBJSIZE_16x8 |            A3_HFLIP,-48,-48),
	{0, 0, 0, 0}
#undef MyCircle
};

void DispCircle(u32 x0, u32 y0){
	const CircleInfo* p = CIRCLE_INFO;
	int id = OBJ_CIRCLE_BEGIN;
	for(; p->tile ; ++p) SetNaviObj(id++, p->tile, p->size, x0 + p->x, y0 + p->y);
}

///////////////////////////////////////////////////////////////////////////////
// 次パイロン	
void InitNaviNext(){
	const u8 TRI0[] = {0x10, 0x10, 0x10, 0x10};
	const u8 TRI1[] = {0x30, 0x35, 0x3a, 0x3f};
	const u8 TRI2[] = {0x3a, 0x35, 0x30, 0x10};
	const u8 TRI3[] = {0x00, 0x00, 0x00, 0x00};
	// パイロン
	MyTri(NW_OFFSET_NEXT, S_32, 16, -1, 24,  96, TRI0);
	MyTri(NW_OFFSET_NEXT, S_32, 16,  1, 24,  96, TRI1);
	MyTri(NW_OFFSET_NEXT, S_32, 16, 16, 24, 256, TRI2);
	MyTri(NW_OFFSET_NEXT, S_32, 16, 18, 24, 256, TRI3);
}

///////////////////////////////////////////////////////////////////////////////
// パイロン	
void InitNaviPyln(){
	const u8 TRI1[] = {0x00, 0x01, 0x02, 0x03};
	const u8 TRI2[] = {0xff, 0xff, 0xff, 0x17};
	MyTri(NW_OFFSET_PYLN,  S_64, 32, 0, 14,  256, TRI1);
	MyTri(NW_OFFSET_PYLN,  S_64, 32, 1, 13,  256, TRI2);
	NDrawRect(NW_OFFSET_PYLN, S_64, 28, 14, 9, 49, 0x03);
	NDrawRect(NW_OFFSET_PYLN, S_64, 29, 13, 7, 49, 0x17);
}

///////////////////////////////////////////////////////////////////////////////
// ベース
void RotateBaseObj(u32 scale){
	u32 sec = ((IW->mp.cur_sect >> 8) != (IW->mp.cur_sect & 0xff))?  (IW->mp.sect_ang64 - IW->px.h_ang64) : 0; // 縦に同じ色が並ぶことは無い!?
	RotateNaviWin(OBJ_RN_BASE, sec, scale, scale);
}

// 角座標系描画
void NCirclePoint(u32 offset, u32 xmask, s32 ang, s32 r, u8 pal){
	s32 x = ((Sin64K(ang) * r) >> 18) + 32;
	s32 y = ((Cos64K(ang) * r) >> 18) + 32;
	pal += 0x50;
	if(x >= 32) pal += 4;
	if(y >= 32) pal += 8;
	NDrawPoint(offset, xmask, x, y, pal);
}

const u8 BASE_NOSEC[ ] = {0x40, 0x40, 0x05, 0x04};
const u8 BASE_SEC[]    = {0x08, 0x08, 0x0f, 0x0c};
const u8 BASE_NONE[]   = {0x40, 0x40, 0x40, 0x40};
const u8 BASE_CYL1[]   = {0x04, 0x04, 0x08, 0x1d}; // Green
const u8 BASE_CYL2[]   = {0x04, 0x04, 0x09, 0x1e}; // Green'
const u8 BASE_CYL3[]   = {0x05, 0x05, 0x0a, 0x1f}; // Yellow
const u8 BASE_CYL4[]   = {0x01, 0x01, 0x02, 0x17}; // Red

const u8* const BASE_TYPE[] = {
	BASE_NOSEC, BASE_SEC, BASE_NONE, BASE_CYL1, BASE_CYL2, BASE_CYL3, BASE_CYL4, BASE_NONE
};

extern const u16 PAL_DEF[5][4];
void ChangeNaviBasePal(s32 i, u32 f){
	if(i > 4) i = 4;
	const u16*  dif = PAL_DEF[i];
	vu16* pal = BGpal + 0x150;
	s32 j;
	for(j = 0 ; j < 4 ; ++j, f >>= 4){
		const u8* c = BASE_TYPE[f & 7];
		for(i = 0 ; i < 4 ; ++i){
			u8 r =  *c       & 0x3;
			u8 g = (*c >> 2) & 0x3;
			u8 b = (*c >> 4) & 0x3;
			*pal = (dif[b] << 10) | (dif[g] << 5) | dif[r];
			++pal;
			++c;
		}
	}
}

void ChangeNaviBase(u32 f){
	if(IW->mp.cur_sect != f){
		IW->mp.cur_sect = f;
		ChangeNaviBasePal(IW->tc.palette_conf, f);
	}
}

void InitNaviBase(){
	// ベース1
	u32 i, j;
	for(j = 1 ; j < 251 ; j += 6){
		for(i = 0 ; i <= 0x10000 ; i += 0x100) NCirclePoint(NW_OFFSET_BASE, S_64, i, j, j >> 6);
	}
	ChangeNaviBase(0x10000);
	RotateBaseObj(256);
}

///////////////////////////////////////////////////////////////////////////////
// ナビウインドウ初期化
void InitNavi(){
	InitNaviPyln();
	InitNaviNext();
	InitNaviBase();
	IW->mp.cur_sect = 0;
	IW->mp.navi_update = -1;
}

///////////////////////////////////////////////////////////////////////////////
// 磁石星描画
///////////////////////////////////////////////////////////////////////////////
typedef struct {
	s32 ang64;
	s32 tile;
} StarInfo;

const StarInfo STAR_INFO[] = {
	{0x0000, OBJ_TN_STAR2},
	{0x1555, OBJ_TN_STAR0},
	{0x2aab, OBJ_TN_STAR0},
	{0x4000, OBJ_TN_STAR1},
	{0x5555, OBJ_TN_STAR0},
	{0x6aab, OBJ_TN_STAR0},
	{0x8000, OBJ_TN_STAR1},
	{0x9555, OBJ_TN_STAR0},
	{0xaaab, OBJ_TN_STAR0},
	{0xc000, OBJ_TN_STAR1},
	{0xd555, OBJ_TN_STAR0},
	{0xeaab, OBJ_TN_STAR0},
	{0, 0}
};
const StarInfo NESW_INFO[] = {
	{ 0x0000, OBJ_TN_NESW + 0 * 4},
	{ 0x4000, OBJ_TN_NESW + 1 * 4},
	{ 0x8000, OBJ_TN_NESW + 2 * 4},
	{ 0xc000, OBJ_TN_NESW + 3 * 4},
	{0, 0}
};

void RotateStarObj(s32 ang64, s32 x, s32 y){
	if(IW->tc.view_mode & 1) ang64 += 0x4000;
	const StarInfo* p = STAR_INFO;
	int id = OBJ_STAR_BEGIN;
	for(; p->tile ; ++p, ++id){
		s32 a2 = ang64 + p->ang64;
#define STAR_R 114
		SetNaviObj(id, p->tile, OBJSIZE_8x8,
			x + Sin64K(a2) * STAR_R / 0x10000 - 4,
			y - Cos64K(a2) * STAR_R / 0x10000 - 4);
	}
	// NESW表示
	p = NESW_INFO;
	id = OBJ_NESW_BEGIN;
	for(; p->tile ; ++p, ++id){
		s32 a2 = ang64 + p->ang64;
		if(IW->tc.view_mode & 1){
			SetNaviObj(id, p->tile + (OBJ_TN_NESW_R - OBJ_TN_NESW), OBJSIZE_16x8,
				x + Sin64K(a2) * 90 / 0x10000 - 8,
				y - Cos64K(a2) * 96 / 0x10000 - 5);
		} else {
			SetNaviObj(id, p->tile, OBJSIZE_8x16,
				x + Sin64K(a2) * 96 / 0x10000 - 4,
				y - Cos64K(a2) * 90 / 0x10000 - 8);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// ゴール!!!
///////////////////////////////////////////////////////////////////////////////
void ClearNaviObj(){
	s32 i;
	for(i = OBJ_NAVI_START ; i <= OBJ_NW_BASE ; ++i) SetNormalObj(i, 0, 0, 0, 160);

	RotateMoveOut(OBJ_RN_BASE);
	RotateMoveOut(OBJ_RN_PYLN);
	RotateMoveOut(OBJ_RN_NEXT);
}

void InitGoalMenu(const Route* rt){
	if(IW->mp.cur_point == rt->count){
		IW->mp.cur_point++;
		//オブジェを1回消す
		Cls();
		ClearNaviObj();
		IW->mp.sect_ang64 = NO_SECTOR;
	}
}

u32 CalcGoalDistance(u32 force){
	u32 rid = 0;
	if(IW->tc.task_mode) rid = IW->tc.route;
	else if(!(rid = (u32)GetNearTarget() + (u32)GetDefaultLD())) rid = -1;

	GoalDist* gd = &IW->mp.goal_dist;
	if(!force && !gd->pos && gd->route_id == rid) return 1; // 完了済

	// フリーフライトモード
	if(!IW->tc.task_mode){
		gd->route_id	= rid;
		gd->pos			= 0;
		gd->dist[1]		= 0;
		gd->dist[0]		= 0; // デフォルト
		CalcWptDist(GetNearTarget(), GetDefaultLD(), gd->dist, 0);
		return 0;// 初完了
	}

	// タスクモード
	Route* rt = GetTaskRoute();
	if(!rt || rt->count < 1){
		gd->route_id	= 0;
		gd->pos			= 0;
		return 0; // 完了
	}

	// 新規計算?
	if(force || gd->route_id != rid){
		gd->route_id	= rid;
		gd->pos			= rt->count - 1;
		gd->dist[gd->pos] = 0;
	}

	// VBCの間だけ計算を進める
	s32 end = IW->vb_counter + 1; // タイムアウト設定
	while(gd->pos){
		u32 pre = gd->pos--;
		CalcWptDist(GetWptInfo(rt, gd->pos), GetWptInfo(rt, pre), &gd->dist[gd->pos], 0);
		gd->dist[gd->pos] += gd->dist[pre];

		// 時間切れチェック
		if(end - (s32)IW->vb_counter < 0){
			IW->intr_flag = 1;
			break;
		}
	}
	return 0; // 時間切れ or 初めての完了
}

u32 GetGoalDistance(u32 n){
	CalcGoalDistance(0);
	if(IW->mp.goal_dist.pos <= n) return IW->mp.goal_dist.dist[n];
	return -1; // 計算中
}

///////////////////////////////////////////////////////////////////////////////
// パイロンゲット判定
///////////////////////////////////////////////////////////////////////////////
u32 CheckPylon(const Route* rt, u32 len, u32 ang64){
	// シリンダチェック
	u32 cp  = IW->mp.cur_point;
	u32 cyl = IW->tc.cylinder;
	u32 sec = IW->tc.sector;
	u32 goal = 0;
	u32 near;

	if(rt->py[cp].cyl != 0xffff) cyl = rt->py[cp].cyl;
	if(cp == rt->count - 1){ // ゴールパイロン
		if(!IW->tc.goal_type)       goal = cyl; //ラインゴール
		if(!(IW->tc.goal_type & 1)) cyl = 0;
		if(!(IW->tc.goal_type & 2)) sec = 0;
	}

	if(len < cyl){
		if(IW->tc.cyl_end) IW->mp.cyl_dist = 1;
		if(IW->tc.start_type && cp == IW->tc.pre_pylon){
			if(IW->px.hour * 60 + IW->px.min < IW->tc.start_time){
				PlaySG1(SG1_PREV);
				IW->mp.pre_pylon = 1; // シリンダGETのマークはしておく
				return -1; // 開始前はキャンセル
			}
			if(IW->tc.start_type == 1 && IW->mp.pre_pylon){
				PlaySG1(SG1_NEXT);
				IW->mp.pre_pylon = 1; // シリンダGETのマークはしておく
				return -1; // InStartは一度Outしないとダメ
			}
		}
		IW->mp.pre_pylon = 1; // シリンダGET
		return 0;
	} else {
		near = len - cyl;
		IW->mp.cyl_dist = MaxS32(near, 1);
		// TODO シリンダが無いときにはセクタ距離をcyl_distにする?
	}
	
	// セクタチェック
#define SECT_MERGIN 0x10
	if(IW->mp.sect_ang64 != NO_SECTOR){
		// セクタ半径内
		u32 sec1 = (IW->mp.sect_ang64 + 0x4000 + SECT_MERGIN) & 0xffff;
		u32 sec2 = sec1 + ((IW->mp.sect_type == TYPE_GOAL)? 0x8000 : 0x4000) - SECT_MERGIN * 2;
		if((ang64 &= 0xffff) < sec1) ang64 += 0x10000;
		if(ang64 < sec2){
			// セクタ角内
			if(len < sec){
				IW->mp.pre_pylon = 2;// セクタGET
				return 0;
			}
			// ゴールライン
			if(len < goal){
				if(IW->mp.pre_pylon == 3) return 0; // ラインGET
				// ゴールライン直線との距離
				near = ((len * myAbs(Sin64K(ang64 - sec1))) >> 16) + 1;
			} else {
				//near = ... TODO セクタ線分との距離
				near = MinS32(near, len - sec); // 届かず
			}
		} else {
			// セクタ角外
			if(len < goal){
				// ゴールライン半径内のときは、あと一歩マーク
				IW->mp.pre_pylon = 3;
				return ((len * myAbs(Sin64K(ang64 - sec1))) >> 16) + 1; // ゴールライン直線との距離
			} else {
				//near = ... TODO セクタ線分との距離
			}
		}
	}

	IW->mp.pre_pylon = 0;
	if(!near) near = 1;
	return near;
}

s32 IsFinalApr(){
	if(!IW->tc.task_mode) return IW->mp.nw_target >= (u32)WPT->wpt_count;
	Route* rt = GetTaskRoute();
	if(!rt || rt->count < 1) return 0;

	return rt->count == IW->mp.cur_point + 1;
}

///////////////////////////////////////////////////////////////////////////////
// 到着高度計算
s32 ArriveAlt(u32 len, s32 alt){
	s32 ldK = IW->px.ldK_avg;
	if(ldK == INVALID_VAL)     return alt; // 平行移動
	if(!ldK || len > 1000000) return (IW->px.up_avg < 0)? -999999 : 999999; // オーバーフローする...
	return alt - BiosDiv(len * 1000, ldK, &len);
}

s32 ArriveScale(s32 len, s32 alt, s32 target_alt, s32 type){
	s32 xmin = IW->tc.ar_min;
	s32 xmax = IW->tc.ar_max;
	if(xmin >= xmax) xmax  = 1;
	else             xmax -= xmin;
	alt = ArriveAlt(len, alt) - target_alt - xmin;
	if(type) {
		// 線形スケール0-256(次次パイロン用)
		if(alt <=    0) return 0;
		if(alt >= xmax) return 256;
		return BiosDiv(alt * 256, xmax, &alt);
	} else {
		// スプライトの拡縮スケール 512～256(次パイロン用)
		if(alt <=    0) return 512;
		if(alt >= xmax) return 256;
		return BiosDiv(xmax * 512, alt + xmax, &alt);
	}
}

///////////////////////////////////////////////////////////////////////////////
// BigNumberの表示
void DispBnObj(u32 base, s32 x, s32 y, u32 valK, u32 dot){
	if(dot){ // 0
		dot = 4;
	} else if(valK < 10000 - 5){ // x.xx
		valK = BiosDiv(valK + 5, 10, &valK);
		dot = 1;
	} else if(valK < 100000 - 50){ // xx.x
		valK = BiosDiv(valK + 50, 100, &valK);
		dot = 2;
	} else { // xxxx
		valK = BiosDiv(valK + 500, 1000, &valK);
		dot = 5;
		if(valK > 9999) valK = 9999;
	}
	s32 i = 4;
	while(i--){
		s32 m;
		if(i == dot){
			m = 10;
		} else {
			valK = BiosDiv(valK, 10, &m);
			if(!valK && !m && i < dot - 1){
				MoveOut(base + i);
				continue; // 以降、全てMoveOut
			}
		}
		if(IW->tc.view_mode & 1){
			// 縦画面
			SetNaviObj(base + i, OBJ_TN_BIGNUM_R + m * 16, OBJSIZE_32x16, x, y);
			y -= 16;
		} else {
			// 横画面
			SetNaviObj(base + i, OBJ_TN_BIGNUM   + m * 16, OBJSIZE_16x32, x, y);
			x -= 16;
		}
	}
}
void DispBnObjInt(u32 base, s32 x, s32 y, u32 val, u32 keta){
	while(keta--){
		s32 m;
		val = BiosDiv(val, 10, &m);
		if(IW->tc.view_mode & 1){
			// 縦画面
			SetNaviObj(base++, OBJ_TN_BIGNUM_R + m * 16, OBJSIZE_32x16, x, y);
			y -= 16;
		} else {
			// 横画面
			SetNaviObj(base++, OBJ_TN_BIGNUM   + m * 16, OBJSIZE_16x32, x, y);
			x -= 16;
		}
	}
}
void DispBnObjLatLon(u32 base, s32 x, s32 y, s32 v){
	if(v < 0) v = -v;
	s32 h, m, s, *p = (IW->tc.view_mode & 1)? &y : &x;
	s = BiosDiv(v, 1000, &v);
	m = BiosDiv(s, 60, &s);
	h = BiosDiv(m, 60, &m);
	
	DispBnObjInt(base, x ,y, v, 3);
	x += (IW->tc.view_mode & 1)? 32 : -56;
	base += 3;
	DispBnObjInt(base, x ,y, s, 2);
	*p -= 40;
	base += 2;
	DispBnObjInt(base, x ,y, m, 2);
	*p -= 40;
	base += 2;
	DispBnObjInt(base, x ,y, h, 3);
}	
void DispBnObjTime(u32 base, s32 x, s32 y, u32 s){
	s32 h, m, *p = (IW->tc.view_mode & 1)? &y : &x;
	GetDTTime(s, &h, &m, &s);
	DispBnObjInt(base, x ,y, s, 2);
	*p -= 40;
	base += 2;
	DispBnObjInt(base, x ,y, m, 2);
	*p -= 40;
	base += 2;
	DispBnObjInt(base, x ,y, h, 2);
}

// 1000倍値を固定幅で出力
void PutsKVal(s32 t){
	if(t < 0){
		if(t > -10000 + 50){ // -x.x m
			Putsf("%2.1f", BiosDiv(t - 50, 100, &t));
		} else { // -xxx m
			if(t < -999999 + 500) t = -999;
			else t = BiosDiv(t - 500, 1000, &t);
			Putsf("%4d", t);
		}
	} else {
		if(t < 10000 - 5){ // x.xx m
			Putsf("%1.2f", BiosDiv(t + 5, 10, &t));
		} else if(t < 100000 - 50){ // xx.x m
			Putsf("%2.1f", BiosDiv(t + 50, 100, &t));
		} else {
			if(t > 9999999 - 500) t = 9999; // xxxx m
			else t = BiosDiv(t + 500, 1000, &t);
			Putsf("%4d", t);
		}
	}
}

// 差分を固定幅で出力(高度差用)
void PutsAltDiff(s32 d){
	if(d > 999) d = 999;
	else if(d < -999) d = -999;
	Putsf("%+4d", d);
}
void PutsAlt(s32 d){
	if(d > 9999) d = 9999;
	else if(d < -999) d = -999;
	Putsf("%4d", d);
}

// ldK表示専用
void PutsLdK(s32 t){
	if(t < -9900 || t >  99000 || IW->px.vh_mm < IW->tc.stop_dir) t = INVALID_VAL;
	if(t == INVALID_VAL){
		Puts(" ---");
	} else {
		Putsf("%2.1f", RoundDiv(t, 100));
	}
}

void PutsPylonInitial(const Route* rt){
	// イニシャル
	Wpt* w = 0;
	s32 t = 0;
	for(; t < 6 && (w = GetCurWpt(rt, t)) != 0 ; ++t){
		if(t) Putc('\\');

		s32 pos = IW->tc.initial_pos - 1;
		if(pos < 1) pos = 0;
		else{
			if(pos >= WPT_NAMESIZE) pos = WPT_NAMESIZE - 1;
			while(!w->name[pos] && --pos);
		}
		Putc(w->name[pos]);
	}
	if(!w){
		for(t *= 2 ; t++ < 12 ;) Putc(' ');
	}
}

void PutPylonDir(s32 ang, s32 f){
	if(IW->px.lat == INVALID_LAT) return;
	if(ang){
		s32 t2 = myAbs(ang);
		if(f) PutsValSpace(t2, 3);
		Putsf("%c%d", (ang < 0)? 'L' : 'R', t2);
		if(!f) PutsValSpace(t2, 3);
	} else {
		Puts(" <C>");
	}
}

Wpt* GetGoalWpt(){
	if(!IW->tc.task_mode) return GetDefaultLD();
	Route* rt = GetTaskRoute();
	if(rt) return GetWptInfo(rt, rt->count - 1);
	return 0;
}

u32 PutsNLD(const Wpt* w, u32 w_len, u32 type) {
	if(IW->px.lat == INVALID_LAT) return 0; 
	if(type & 1){
		u32 t = GetGoalDistance(IW->mp.cur_point);
		if(t == -1) return 0;
		w_len += t;
		w = GetGoalWpt();
	}
	if(!w) return 0;

	s32 alt = RoundDiv(IW->px.alt_mm, 1000) - w->alt;
	if(alt <= 0){
		if(alt < -999) alt = -999;
		if(!(type & 0x80000000)) Puts((type & 1)? "GDf:" : "NDf:");
		else                     Puts((type & 1)? "GD"   : "ND");
		Putsf("%4d", alt);
		if(!(type & 0x80000000)) Putc('m');
	} else if(alt){
		alt = RoundDiv(w_len * 10, alt);
		if(alt > 999) alt = 999;
		if(!(type & 0x80000000)) Puts((type & 1)? "GLD:" : "NLD:");
		else                     Puts((type & 1)? "GL"   : "NL");
		Putsf("%2.1f", alt);
		if(!(type & 0x80000000)) Putc(' ');
	}
	return 1;
}

void PutAtAlt(s32 cur_alt, const Wpt* target, s32 len, u32 type){
	switch(type & 0xff){
	case 0:
		if(IW->px.vh_mm < IW->tc.stop_dir) break;
		if(!(type & 0x80000000)) Puts("ArA:");
		PutsAlt(ArriveAlt(len, cur_alt));
		Putc((type & 0x80000000)? 'a' : 'm');
		return;
	case 1:
		if(IW->px.vh_mm < IW->tc.stop_dir) break;
		if(!(type & 0x80000000)) Puts("ArD:");
		PutsAltDiff(ArriveAlt(len, cur_alt) - target->alt);
		Putc((type & 0x80000000)? '@' : 'm');
		return;
	case 2:
		if(!(type & 0x80000000)) Puts("Dif:");
		PutsAltDiff(cur_alt - target->alt);
		Putc((type & 0x80000000)? 'd' : 'm');
		return;
	case 3:
		if(IW->px.vh_mm < IW->tc.stop_dir) break;
		len = GetGoalDistance(IW->mp.cur_point);
		if(len != -1){
			len += IW->mp.py_dis;
			Wpt* goal = GetGoalWpt();
			if(goal){
				if(!(type & 0x80000000)) Puts("ArG:");
				PutsAltDiff(ArriveAlt(len, cur_alt) - goal->alt);
				Putc((type & 0x80000000)? 'g' : 'm');
				return;
			}
		}
		break;
	case 4: // NLD
	case 5: // GLD
		if(PutsNLD(target, len, type & 0x80000001)) return;
	}
	if(type & 0x80000000)	Puts("---- ");
	else					Puts("    ---- ");
}

///////////////////////////////////////////////////////////////////////////////
// 予想到着時刻
#define MAX_TIME (360000 - 1) // 99:59:59
void PutATime(u32 len){
	if(IW->px.vh_mm < IW->tc.stop_dir){
		Puts("AT--------");
		return;
	}
	if(len > 4200000) len = MAX_TIME;
	else {
		len = BiosDiv(len * 1000, IW->px.vh_avg, &len);
		if(len > MAX_TIME) len = MAX_TIME;
	}

	// 時
	u8 b = ' ';
	s32 t = BiosDiv(len, 3600, &len);
	if(t){
		Putsf("AT%2d:", t);
		b = '0';
	} else {
		PutsSpace(2);
	}
	// 分
	t = BiosDiv(len, 60, &len);
	if(b == '0' || t){
		if(b != '0') Puts("AT ");
		PutsValX(t, 10, 2, b, 0);
		Putc(':');
		b = '0';
	} else {
		PutsSpace(3);
	}
	// 秒
	if(b != '0') Puts("AT ");
	PutsValX(len, 10, 2, b, 0);
}

///////////////////////////////////////////////////////////////////////////////
// バリオメータ グラフ表示
#define VARIO_BLOCK		500
#define VARIO_BLOCK_SFT 6

void PutVarioGraph(s32 x0, s32 y0){
	s32 i, j, k = OBJ_VERIO_BEGIN;
	for(j = -1 ; j <= 1; j += 2){
		s32 up = IW->px.up_mm * j;
		for(i = 0 ; i < 10 ; ++i, up -= VARIO_BLOCK){
			s32 tile;
			s32 s = 8 * j * i + 4 * j - (j + 1) / 2;
			u32 flipF = (IW->tc.view_mode & 1)? A3_HFLIP : A3_VFLIP;
			u32 flip = (j < 0)? 0 : flipF;
			if(up > VARIO_BLOCK * 11){
				// バー
				if(!(i & 1)) continue;
				tile = 0; // tile = 1; //厚いバー
				s += 8 * j;
			} else if(up > VARIO_BLOCK * 10){
				tile = ((VARIO_BLOCK * 11 - up) >> VARIO_BLOCK_SFT);
				if(tile > 6) tile = 6;
				s -= j;
				flip ^= flipF;
			} else if(up > VARIO_BLOCK * 9){
				tile = 7;
				if(i & 1){
					flip ^= flipF;
					s -= j;
				}
			} else if(up > 0){
				tile = (up >> VARIO_BLOCK_SFT);
				if(tile > 6) tile = (i & 1)? 6 : 7;
			} else {
				// バー
				if(i & 1) continue;
				tile = 0; // 薄いバー
			}
			if(IW->tc.view_mode & 1){
				// 縦
				SetNaviObj(k++, tile * 4 + OBJ_TN_VARIO_R, OBJSIZE_8x16 | flip, x0 + s, y0);
			} else {
				// 横
				SetNaviObj(k++, tile * 4 + OBJ_TN_VARIO,   OBJSIZE_16x8 | flip, x0, y0 - s);
			}
		}
	}
	if(IW->tc.view_mode & 1) SetNaviObj(k++, OBJ_TN_VARIO_CR, OBJSIZE_8x32, x0, y0 - 4);
	else					 SetNaviObj(k++, OBJ_TN_VARIO_C,  OBJSIZE_32x8, x0 - 4, y0);
	while(k <= OBJ_VERIO_END) MoveOut(k++);
}

///////////////////////////////////////////////////////////////////////////////
// ステータス表示
void DispStatus(s32 x, s32 y, u32 id){
	s32 t;
	switch(IW->px.fix){
	case FIX_UNUSABLE:
		t = OBJ_TN_STATUS_NG1;
		break;
	case FIX_INVALID:
		t = OBJ_TN_STATUS_NG2;
		break;
	case FIX_2D:
	case FIX_2D_diff:
		t = OBJ_TN_STATUS_2D;
		break;
	case FIX_3D:
	case FIX_3D_diff:
		if(IW->tc.waas_flag){
			t = (IW->px.fix == FIX_3D_diff)? OBJ_TN_STATUS_GOOD : OBJ_TN_STATUS_3D;
		} else {
			t = (IW->px.epe_mm < 30000)?     OBJ_TN_STATUS_GOOD : OBJ_TN_STATUS_3D;
		}
		break;
	default: // !?
		for(t = id ; t < id + 4 ; ++t) MoveOut(t);
		return;
	}
	SetNaviObj(id + 0, t, OBJSIZE_8x8,                       x - 8, y - 8);
	SetNaviObj(id + 1, t, OBJSIZE_8x8 | A3_HFLIP,            x,     y - 8);
	SetNaviObj(id + 2, t, OBJSIZE_8x8 | A3_HFLIP | A3_VFLIP, x,     y);
	SetNaviObj(id + 3, t, OBJSIZE_8x8 |            A3_VFLIP, x - 8, y);
}


///////////////////////////////////////////////////////////////////////////////
// 垂直GPS
void AddNumObj(u32* id, u32 tile, s32* x, s32* y){
	if(IW->tc.view_mode & 1){
		*y -= 8;
		if(*x < -16 || 240 < *x || *y < -16 || 160 < *y) return;
		SetNaviObj((*id)++, tile * 4 + OBJ_TN_SMLNUM_R, OBJSIZE_16x8, *x, *y);
	} else {
		*x -= 8;
		if(*x < -16 || 240 < *x || *y < -16 || 160 < *y) return;
		SetNaviObj((*id)++, tile * 4 + OBJ_TN_SMLNUM,   OBJSIZE_8x16, *x, *y);
	}
}
u32 DispSmallFont(u32 id, s32 x, s32 y, s32 val, s32 keta, const u8* unit){
	if(unit){
		while(*unit) AddNumObj(&id, *unit++, &x, &y);
	}
	s32 v = val;
	if(v < 0) v = -v;
	while(keta-- > 0 || v){
		s32 mod;
		v = BiosDiv(v, 10, &mod);
		AddNumObj(&id, mod, &x, &y);
	}
	if(val < 0) AddNumObj(&id, 10, &x, &y);
	return id;
}

const char* const NESW_NAME[24] = {
//	"N", 0, 0, "NNE", 0, 0, "ENE", 0, 0, "E", 0, 0, "ESE", 0, 0, "SSE", 0, 0,
//	"S", 0, 0, "SSW", 0, 0, "WSW", 0, 0, "W", 0, 0, "WNW", 0, 0, "NNW", 0, 0,
//	"1", 0, 0, "112", 0, 0, "212", 0, 0, "2", 0, 0, "232", 0, 0, "332", 0, 0,
//	"3", 0, 0, "334", 0, 0, "434", 0, 0, "4", 0, 0, "414", 0, 0, "114", 0, 0,
	"1", 0, 0, "12", 0, 0, "2", 0, 0, "32", 0, 0, 
	"3", 0, 0, "34", 0, 0, "4", 0, 0, "14", 0, 0,
};
void AddNeswObj(u32* id, u32 tile, s32* x, s32* y){
	if(IW->tc.view_mode & 1)	SetNaviObj((*id)++, tile * 4 + OBJ_TN_NESW_R, OBJSIZE_16x8, *x, *y += 8);
	else 						SetNaviObj((*id)++, tile * 4 + OBJ_TN_NESW,   OBJSIZE_8x16, *x += 8, *y);
}
u32 DispNeswName(u32 id, s32 x, s32 y, const char* name){
	if(name[1]){
		if(IW->tc.view_mode & 1) y -= 4;
		else					 x -= 4;
	}
	for(; *name ; ++name){
		AddNeswObj(&id, *name - '1', &x, &y);
	}
	return id;
}


void DispVBox(s32 x0, s32 y0){
	s32 i, j, id = OBJ_V_BOX;

	// 仰角
	s32 ang = IW->px.v_ang64 & 0xffff;
	if(ang > 0x4000) ang -= 0x10000;
#define VA_SCALE 8
#define VH_SCALE 8
	s32 y = (ang - 0x4000)  * 36 * VA_SCALE / 65536 + y0;
	j = 1;
	for(i = 6 ; i >= -6 ; --i, y += (VA_SCALE + VA_SCALE / 2)){
		if(!j--) j = 2;
		if(y < -4) continue;
		if(y > 140) break;
		if(!j){
			id = DispSmallFont(id, x0 - 88, y - 7, i * 15, 1, 0);
			SetNaviObj(id++, OBJ_TN_VARIO,  OBJSIZE_16x8, x0 - 88, y);
		} else {
			SetNaviObj(id++, OBJ_TN_VARIO,  OBJSIZE_8x8,  x0 - 80, y);
		}
	}

	// 方位
	ang = IW->px.h_ang_c & 0xffff;
	s32 x = x0 - ang * 36 * VH_SCALE / 65536 + 36 * VH_SCALE - 8;
	for(i = 0 ; i < 24 ; i++, x += VH_SCALE + VH_SCALE / 2){
		if(x > 240) x -= 36 * VH_SCALE;
		if(x < -16) continue;
		const char* hn = NESW_NAME[i];
		if(hn) id = DispNeswName(id, x - 4, y0 + 80, hn);
		if(x < -4) continue;
		SetNaviObj(id++, OBJ_TN_VARIO_R,  hn? OBJSIZE_8x16 : OBJSIZE_8x8,  x, y0 + 64);
	}

	// 基準線
	SetNaviObj(id++, OBJ_TN_VARIO_C,  OBJSIZE_16x8, x0 - 88, y0 - 4);
	SetNaviObj(id++, OBJ_TN_VARIO_CR, OBJSIZE_8x16, x0 - 4, y0 + 64);

	// オブジェ非表示
	while(id <= OBJ_V_BOX_END) MoveOut(id++);
}

void DispVBoxTate(s32 x0, s32 y0){
	s32 i, j, id = OBJ_V_BOX;

	// 仰角
	s32 ang = IW->px.v_ang64 & 0xffff;
	if(ang > 0x4000) ang -= 0x10000;
	s32 x = x0 - (ang - 0x4000)  * 36 * VA_SCALE / 65536 - 8;
	j = 1;
	for(i = 6 ; i >= -6 ; --i, x -= (VA_SCALE + VA_SCALE / 2)){
		if(!j--) j = 2;
		if(x < 24) break;
		if(x > 240) continue;
		if(!j){
			id = DispSmallFont(id, x - 1, y0 - 68, i * 15, 1, 0);
			SetNaviObj(id++, OBJ_TN_VARIO_R,  OBJSIZE_8x16, x, y0 - 80);
		} else {
			SetNaviObj(id++, OBJ_TN_VARIO_R,  OBJSIZE_8x8,  x, y0 - 72);
		}
	}

	// 方位
	ang = IW->px.h_ang_c & 0xffff;
	s32 y = y0 - ang * 36 * VH_SCALE / 65536 + 36 * VH_SCALE - 4;
	for(i = 0 ; i < 24 ; i++, y += VH_SCALE + VH_SCALE / 2){
		if(y > 240) y -= 36 * VH_SCALE;
		if(y < -16) continue;
		const char* hn = NESW_NAME[i];
		if(hn) id = DispNeswName(id, x0 - 96, y - 8, hn);
		if(y < -4) continue;
		if(hn) SetNaviObj(id++, OBJ_TN_VARIO,  OBJSIZE_16x8 | A3_HFLIP, x0 - 80, y + 4);
		else   SetNaviObj(id++, OBJ_TN_VARIO,  OBJSIZE_8x8  | A3_HFLIP, x0 - 72, y + 4);
	}

	// 基準線
	SetNaviObj(id++, OBJ_TN_VARIO_CR, OBJSIZE_8x16, x0 -  4, y0 - 80);
	SetNaviObj(id++, OBJ_TN_VARIO_C,  OBJSIZE_16x8, x0 - 80, y0 -  4);

	// オブジェ非表示
	while(id <= OBJ_V_BOX_END) MoveOut(id++);
}

///////////////////////////////////////////////////////////////////////////////
// 垂直パイロン
void DispVPyln(s32 x0, s32 y0, s32 alt, s32 len, s32 ha, const Wpt* w, s32 type){
	s32 id = type? OBJ_V_P2 : OBJ_V_P1;
	if(w){
		s32 va = GetAngLR(BiosAtan64K(w->alt - alt, len)) - GetAngLR( IW->px.v_ang64);
		ha = GetAngLR(ha);
		s32 x1, x =   ha * 36 * VH_SCALE / 65536;
		s32 y1, y = - va * 36 * VA_SCALE / 65536;

		s32 ang = BiosAtan64K(y, x) + 0x4000;
		if(IW->tc.view_mode & 1){
			s32 t = x;
			x = -y;
			y = t;
		}
		// ターゲット方向表示
		if(x * x + y * y > 60 * 60){
			if(type){
				RotateNaviWin(OBJ_RN_NEXT, ang, 512, 512);
				if(IW->tc.view_mode & 1) ang += 0x4000;
				x1 = x0 + ((Sin64K(ang) * 80) >> 16) - 16;
				y1 = y0 - ((Cos64K(ang) * 80) >> 16) - 16;
				SetNaviObj(OBJ_NW_PYLN2, OBJ_TN_NEXT, OBJSIZE_32x32 | A3_ROTATE_N(OBJ_RN_NEXT), x1, y1);
			} else {
				RotateNaviWin(OBJ_RN_PYLN, ang, 768, 1024);
				if(IW->tc.view_mode & 1) ang += 0x4000;
				s32 s = Sin64K(ang);
				s32 c = Cos64K(ang);
				x1 = x0 + ((s * 88) >> 16) - 32;
				y1 = y0 - ((c * 88) >> 16) - 32;
				SetNaviObj(OBJ_NW_PYLN, OBJ_TN_PYLN, OBJSIZE_64x64 | A3_ROTATE_N(OBJ_RN_PYLN), x1, y1);
				if(IW->tc.view_mode & 1){
					x1 = x0 + ((s * 64) >> 16) + 16;
					y1 = y0 - ((c * 48) >> 16) - 4;
				} else {
					x1 = x0 + ((s * 48) >> 16) + 24;
					y1 = y0 - ((c * 64) >> 16) - 8;
				}
			}

		} else {
			RotateMoveOut(type? OBJ_RN_NEXT : OBJ_RN_PYLN);
			MoveOut(type? OBJ_NW_PYLN2 : OBJ_NW_PYLN);
			if(type == 0){
				x1 = x0 + x + 48;
				if(y < 4){
					y1 = y0 + y + 24;
					SetNaviObj(id++, OBJ_TN_LEAD, OBJSIZE_16x16, x1 - 40, y1 - 16);
				} else {
					y1 = y0 + y - 40;
					SetNaviObj(id++, OBJ_TN_LEAD, OBJSIZE_16x16 | A3_HFLIP, x1 - 40, y1 + 16);
				}
			}
		}
		if(type == 0){
			// 距離表示
			const char* unit = "\014"; // m
			if(len > 9999){
				len = BiosDiv(len + 500, 1000, &len);
				unit = "\014\013"; // km
				if(len > 999) len = 999;
			}
			if(IW->tc.view_mode & 1){
				x1 -= 24;
				y1 += 24;
			}
			id = DispSmallFont(id, x1, y1, len, 1, unit);

			// セクタ表示
			if(IW->mp.sect_ang64 == NO_SECTOR){// || MyABS(x) > 60){
				RotateMoveOut(OBJ_RN_BASE);
			} else {
				s32 xmag, ymag;
				if(IW->tc.view_mode & 1){
					SetNaviObj(OBJ_NW_BASE, OBJ_TN_BASE, OBJSIZE_64x64 | A3_ROTATE_D(OBJ_RN_BASE), x0 - 144, y0 -64);
					xmag = 960;
					ymag = 256;
				} else {
					SetNaviObj(OBJ_NW_BASE, OBJ_TN_BASE, OBJSIZE_64x64 | A3_ROTATE_D(OBJ_RN_BASE), x0 - 64, y0 + 15);
					xmag = 256;
					ymag = 960;
				}
				ang = IW->mp.sect_ang64 - IW->px.h_ang64;
				if(IW->tc.view_mode & 1) ang += 0x4000;
				s16 s = Sin64K(ang);
				s16 c = Cos64K(ang);
				vu16* p = (u16*)(OAM + (OBJ_RN_BASE << 5));
				p[ 3] =  (c * xmag) >> 16;
				p[ 7] =  (s * ymag) >> 16;
				p[11] = -(s * xmag) >> 16;
				p[15] =  (c * ymag) >> 16;
			}
		}

		x += x0;
		y += y0;
		if(-16 < x && x < 220 && -16 < y && y < 176 && (!(IW->tc.view_mode & 1) || x < 204)){
			x += 8;
			y -= 8;
			if(IW->tc.task_mode){
				if(IW->tc.view_mode & 1)id = DispSmallFont(id, x - 16, y + 16, type + IW->mp.cur_point + 1, 2, 0);
				else					id = DispSmallFont(id, x, y, type + IW->mp.cur_point + 1, 2, 0);
			} else {
				SetNaviObj(id++, type? OBJ_TN_STAR1 : OBJ_TN_STAR2, OBJSIZE_8x8, x - 12, y + 4);
			}
			alt = type? OBJ_TN_STATUS_2D : OBJ_TN_STATUS_3D;

			SetNaviObj(id++, alt, OBJSIZE_8x8,                       x - 20, y - 4);
			SetNaviObj(id++, alt, OBJSIZE_8x8 | A3_HFLIP,            x -  4, y - 4);
			SetNaviObj(id++, alt, OBJSIZE_8x8 | A3_HFLIP | A3_VFLIP, x -  4, y + 12);
			SetNaviObj(id++, alt, OBJSIZE_8x8 |            A3_VFLIP, x - 20, y + 12);
		}
	} else {
		RotateMoveOut(OBJ_RN_NEXT);
	}
	// オブジェ非表示
	while(id <= (type? OBJ_V_P2_END : OBJ_V_P1_END)) MoveOut(id++);
}

///////////////////////////////////////////////////////////////////////////////
// シリンダマップ
///////////////////////////////////////////////////////////////////////////////
s32 IsPutOfDisp(s32 x, s32 y){
	return x < -16 || y < -16 || 256 < x || 176 < y;
}
s32 PutPoint(s32 id, s32 x, s32 y, s32 tile){
	switch(tile){
	case 0: SetNaviObj(id, OBJ_TN_GUIDE1, OBJSIZE_8x8 | A3_ROTATE_N(OBJ_RN_CYL1), x - 4, y - 4); break;
	case 1: SetNaviObj(id, OBJ_TN_GUIDE2, OBJSIZE_8x8 | A3_ROTATE_N(OBJ_RN_CYL2), x - 4, y - 4); break;
	case 2: SetNaviObj(id, OBJ_TN_GUIDE3, OBJSIZE_8x8 | A3_ROTATE_N(OBJ_RN_CYL3), x - 4, y - 4); break;
	case 3: SetNaviObj(id, OBJ_TN_GUIDE3, OBJSIZE_8x8 | A3_ROTATE_N(OBJ_RN_CYL4), x - 4, y - 4); break;
	}
	return 1;
}

u32 DrawLine(u32 id, s32 x0, s32 y0, s32 x1, s32 y1, s32 tile){
	if(x0 == x1 && y0 == y1) return id;

	s32 sx = ( x1 > x0 )? 1 : -1;
	s32 sy = ( y1 > y0 )? 1 : -1;
	s32 dx = ( x1 > x0 )? (x1 - x0) : (x0 - x1);
	s32 dy = ( y1 > y0 )? (y1 - y0) : (y0 - y1);
	s32 dx2 = dx * 2;
	s32 dy2 = dy * 2;
	s32 E, i = 0;

	s32 ang = BiosAtan64K(y1 - y0, x1 - x0) + 0x4000;
	RotateNaviWin(OBJ_RN_CYL1 + tile, ang + ((IW->tc.view_mode & 1)? 0xc000 : 0), 768, (tile < 2)? 512 : 256);
	s32 step = (tile < 2)? 11 : 12;

	if( dx >= dy ) {
		E = -dx;
		step = myAbs(Sin64K(ang) >> step);

		// クリッピング
		if(x0 < 0){
			if(sx < 0) return id;
			i = -BiosDiv(x0, step, &x0) * step;
		} else if(x0 > 240){
			if(sx > 0) return id;
			i = BiosDiv(x0 - 240, step, &x0) * step;
			x0 += 240;
		}
		if(i){
			E += dy2 * i;
			if(E >= 0){
				y0 += sy * (BiosDiv(E, dx2, &E) + 1);
				E -= dx2;
			}
		}
		// ライン描画
		for(; i < dx; i += step) {
			if( (sx < 0 && x0 < 0) ||
				(sx > 0 && x0 > 240)) return id;
			if(i && !IsPutOfDisp(x0, y0)) id += PutPoint(id, x0, y0, tile);
			x0 += sx * step;
			E += dy2 * step;
			while( E >= 0 ) { // 割り算より早い?
				y0 += sy;
				E -= dx2;
			}
		}
	} else {
		E = -dy;
		step = myAbs(Cos64K(ang) >> step);

		// クリッピング
		if(y0 < 0){
			if(sy < 0) return id;
			i = -BiosDiv(y0, step, &y0) * step;
		} else if(y0 > 160){
			if(sy > 0) return id;
			i = BiosDiv(y0 - 160, step, &y0) * step;
			y0 += 160;
		}
		if(i){
			E += dx2 * i;
			if(E >= 0){
				x0 += sx * (BiosDiv(E, dy2, &E) + 1);
				E -= dy2;
			}
		}
		// ライン描画
		for(; i < dy; i += step) {
			if( (sy < 0 && y0 < 0) ||
				(sy > 0 && y0 > 160)) return id;
			if(i && !IsPutOfDisp(x0, y0)) id += PutPoint(id, x0, y0, tile);
			y0 += sy * step;
			E += dx2 * step;
			while( E >= 0 ) { // 割り算より早い?
				x0 += sx;
				E -= dy2;
			}
		}
	}
	return id;
}

s32 CalcPylonPos(s32 len, s32 ang, s32* x, s32* y, s32 scale){
	if(IW->px.lat == INVALID_LAT) return -1;
	if(len > (1 << 24)) return -2; // 遠すぎ

	len = RoundDiv(len * 32, scale);
	s32 t = len, sft = 15;
	for(; t > 0x8000 ; t >>= 1) if(sft) sft--;
	*x = -((Sin64K(ang) * t) >> sft);
	*y =  ((Cos64K(ang) * t) >> sft);
	return len; // success
}

u32 PutCylPylon(u32 id, s32 nx, s32 ny, s32 x, s32 y, s32 scale, s32 len, s32 ang, s32 val, s32 line){
	s32 x0, y0;
	if(CalcPylonPos(len, ang, &x0, &y0, scale) >= 0){
		x0 += nx;
		y0 += ny;
		if(x0 > -32 && y0 > -32 && x0 < 248 && y0 < 168){
			if(IW->tc.task_mode){
				if(IW->tc.view_mode & 1) id = DispSmallFont(id, x0 - 8, y0 + 8, val, 2, 0);
				else                     id = DispSmallFont(id, x0 + 8, y0 - 8, val, 2, 0);
			} else {
				SetNaviObj(id++, line? OBJ_TN_STAR1 : OBJ_TN_STAR0, OBJSIZE_8x8, x0 - 4, y0 - 4);
			}
		}
		if(line) id = DrawLine(id, x, y, x0, y0, line);
		else     id = DrawLine(id, x0, y0, x, y, line);
	}
	return id;
}

void UpdateCylView(const Route* rt, s32 len, s32 nextAng, s32 p2Len, s32 p2Ang64, const Wpt* wpre, const Wpt* w0, const Wpt* w1){
	if(IW->px.lat == INVALID_LAT) return;

	s32 id = OBJ_V_BOX;
	s32 nx, ny;
	if(IW->tc.view_mode & 1){
		nx = 32;
		ny = 80;
	} else {
		nx = 120;
		ny = 128;
	}
	if(len >= 0){
		s32 a2 = nextAng + ((IW->tc.view_mode & 1)? 0xc000 : 0x8000);
		s32 x, y;
		s32 scale = IW->tc.cylinder;
		s32 sector = 0;
		u32 cp  = IW->mp.cur_point;
		if(rt && cp < rt->count && rt->py[cp].cyl != 0xffff) scale = rt->py[cp].cyl;

		// 予想到着高度で色分け
		s32 aa = ArriveAlt(len, RoundDiv(IW->px.alt_mm, 1000)) - w0->alt;
		if(aa < IW->tc.ar_min)        aa = 0x6666;
		else if(aa < 0)               aa = 0x5555;
		else if(aa < IW->tc.ar_max)   aa = 0x4444;
		else                          aa = 0x3333;
		if(scale < 1 || (!w1 && IW->tc.goal_type == 2)){
			if(IW->tc.sector < 2 || !wpre){
				scale = 100; // シリンダもセクタも0mのパイロンは無いが、もし設定されていたらとりあえず100mで…
				ChangeNaviBase(0);
			} else {
				scale = IW->tc.sector >> 1;
				sector = 1;
				ChangeNaviBase(aa | (w1? 0x7770 : 0x7700));
			}
		} else {
			if(IW->tc.task_mode && !w1 && IW->tc.goal_type == 0) ChangeNaviBase(0x7777); // ラインゴール
			else                                                 ChangeNaviBase(aa);
		}

		s32 t = CalcPylonPos(len, a2, &x, &y, scale);
		if(t >= 0){
			x += nx;
			y += ny;
//			if(-64 < x && -64 < y && x < 304 && y < 224){ // セクタが下からはみ出す!?
			if(-64 < x && -32 < y && x < 304 && y < 224){
				if(sector){
					SetNaviObj(OBJ_NW_BASE, OBJ_TN_BASE, OBJSIZE_64x64 | A3_ROTATE_D(OBJ_RN_BASE), x - 64, y - 64);
					RotateBaseObj(256);
				} else {
					SetNaviObj(OBJ_NW_BASE, OBJ_TN_BASE, OBJSIZE_64x64 | A3_ROTATE_N(OBJ_RN_BASE), x - 32, y - 32);
					RotateBaseObj(512);
				}
				if(IW->tc.task_mode){
					if(IW->tc.view_mode & 1) id = DispSmallFont(id, x - 8, y + 8, IW->mp.cur_point + 1, 2, 0);
					else                     id = DispSmallFont(id, x + 8, y - 8, IW->mp.cur_point + 1, 2, 0);
				} else {
					SetNaviObj(id++, OBJ_TN_STAR2, OBJSIZE_8x8, x - 4, y - 4);
				}
			} else {
				RotateMoveOut(OBJ_RN_BASE);
				MoveOut(OBJ_NW_BASE);
			}
			if(id == OBJ_V_BOX){
				// 方向表示
				SetNaviObj(OBJ_CYL_NW_PYLN, OBJ_TN_PYLN, OBJSIZE_64x64 | A3_ROTATE_N(OBJ_RN_PYLN),
							nx - 33 - ((Sin64K(a2) * 24) >> 15), ny - 32 + ((Cos64K(a2) * 24) >> 15));
				RotateNaviWin(OBJ_RN_PYLN, nextAng, 1024, 1024);
			} else {
				RotateMoveOut(OBJ_RN_PYLN);
				MoveOut(OBJ_CYL_NW_PYLN);
			}

			// セクタ
			if(!sector && IW->mp.sect_ang64 != NO_SECTOR && /*scale < IW->tc.sector &&*/ (w1 || IW->tc.goal_type != 1)){
				s32 sang = IW->mp.sect_ang64 - IW->px.h_ang64;
				s32 slen = (!w1 && !IW->tc.goal_type)? 64 : RoundDiv(IW->tc.sector * 64, scale);
				s32 shift = 16;
				while(slen > 0x10000){
					slen >>= 1;
					shift--;
				}
				if(!(IW->tc.view_mode & 1)) sang -= 0x4000;
				id = DrawLine(id, x + ((Sin64K(sang) * slen) >> shift),
					              y - ((Cos64K(sang) * slen) >> shift), x, y, 2);

				sang += (IW->mp.sect_type == TYPE_GOAL)? 0x8000 : 0x4000;
				id = DrawLine(id, x + ((Sin64K(sang) * slen) >> shift),
					              y - ((Cos64K(sang) * slen) >> shift), x, y, 3);
			}


			// 前パイロン
			if(!wpre && !IW->tc.task_mode && IW->mp.nw_start.lat != INVALID_LAT) wpre = &IW->mp.nw_start;
			if(wpre){
				s32 preLen, preAng;
				CalcDist(IW->px.lat, IW->px.lon, wpre->lat, wpre->lon, &preLen, &preAng);
				id = PutCylPylon(id, nx, ny, x, y, scale, preLen, preAng - IW->px.h_ang64 + ((IW->tc.view_mode & 1)? 0xc000 : 0x8000), IW->mp.cur_point, 0);
			}
			// 次次パイロン
			if(w1){
				id = PutCylPylon(id, nx, ny, x, y, scale, p2Len, p2Ang64 - IW->px.h_ang64 + ((IW->tc.view_mode & 1)? 0xc000 : 0x8000), IW->mp.cur_point + 2, 1);
			}
		}
	}

	// 自機
	if(IW->tc.view_mode & 1) SetNaviObj(OBJ_MY_POS, OBJ_TN_SELF_R, OBJSIZE_16x8, nx - 8, ny - 4);
	else                     SetNaviObj(OBJ_MY_POS, OBJ_TN_SELF,   OBJSIZE_8x16, nx - 4, ny - 8);

	// 北矢印
	if(IW->tc.view_mode & 1){
		nx = 52;
		ny = 30;
	} else {
		nx = 30;
		ny = 122;
	}
	s32 ang64 = IW->px.h_ang_c;
	SetNaviObj(id++, OBJ_TN_NEXT, OBJSIZE_32x32 | A3_ROTATE_D(OBJ_RN_NEXT), nx - 32, ny - 32);
	RotateNaviWin(OBJ_RN_NEXT, -ang64, 896, 384);
	// N表示
	if(IW->tc.view_mode & 1) ang64 -= 0x4000;
	const StarInfo* p = NESW_INFO;
	s32 a2 = p->ang64 - ang64;
	if(IW->tc.view_mode & 1){
		SetNaviObj(id++, p->tile + (OBJ_TN_NESW_R - OBJ_TN_NESW), OBJSIZE_16x8,
			nx - 8 + ((Sin64K(a2) * 60) >> 16),
			ny - 4 - ((Cos64K(a2) * 52) >> 16));
	} else {
		SetNaviObj(id++, p->tile, OBJSIZE_8x16,
			nx - 4 + ((Sin64K(a2) * 52) >> 16),
			ny - 8 - ((Cos64K(a2) * 60) >> 16));
	}

	// オブジェ非表示
	while(id < OBJ_CYL_END) MoveOut(id++);
}


///////////////////////////////////////////////////////////////////////////////
// 特殊モード
///////////////////////////////////////////////////////////////////////////////
void InitSpliteTable(){
	s32 i;
	for(i = 0 ; i < SPLITE_VCOUNT ; ++i){
		SPLITE->hBuf[i].cur_count = SPLITE->hBuf[i].pre_count = 0;
	}
}
void ResetSpliteTable(){
	s32 i;
	for(i = 0 ; i < SPLITE_VCOUNT ; ++i){
		SPLITE->hBuf[i].cur_count = 0;
		SPLITE->hBuf[i].full_check = 0;

	}
	SPLITE->cur_full = 0;
}
void UpdateSpliteTable(){
	s32 i;
	for(i = 0 ; i < SPLITE_VCOUNT ; ++i){
		SPLITE->hBuf[i].pre_count = SPLITE->hBuf[i].cur_count;
		if(SPLITE->hBuf[i].full_check + SPLITE->hBuf[i].pre_count >= MAX_SPLITE) SPLITE->cur_full = 1;
	}
	SPLITE->pre_full = SPLITE->cur_full;
	if(SPLITE->pre_full && !IsMenu()) DmaClear(3, 0, SPLITE->near, sizeof(SPLITE->near), 32);
}

void AddSpliteSub(u32 index, u32 valA, u32 valB){
	if(index >= SPLITE_VCOUNT) return;
	SpliteHBuf* p = SPLITE->hBuf + index;
	p->full_check++;
	if(!valB) return;
	if(p->cur_count >= MAX_SPLITE){
		SPLITE->cur_full = 1;
		return; // スプライト限界
	}

	u32* buf = &p->buf[p->cur_count++ * 2];
	buf[0] = valA;
	buf[1] = valB;
}

void AddSpliteTable(s32 x, s32 y, s32 tile){
	if(x < -5 || 238 < x || y < -5 || 158 < y) return; // 範囲外

	if(IW->tc.view_mode & 2){
		x = 240 - 8 - x;
		y = 160 - 8 - y;
	}
	u32 valA = (y & 0xff) | 0x2400 |  ((x & 0x1ff) << 16);
	u32 valB = (tile & 0x3ff) | A2_PRI2;

	// 重複チェック
	if(SPLITE->pre_full && !IsMenu()){ // 前回FULLの時だけ重複チェックする
		u8* near = &SPLITE->near[(x < 0)? 0 : (x >> 2)]
			                    [(y < 0)? 0 : (y >> 2)];
		if(*near) valB = 0;// 重複マーク
//			SPLITE->cur_full++;
//			return;
//		}
		*near = 1;
	}

	if(++y < 0){
		AddSpliteSub(0, valA, valB);
	} else{
        x = y >> 3;
		AddSpliteSub(x    , valA, valB);
		if((y & 7) > 2)
		AddSpliteSub(x + 1, valA, valB);
	}
}

void PutsCurTime(){
	// 現在時刻
	Putsf("%02d:%02d:%02d", IW->px.hour, IW->px.min, IW->px.sec);
}

void UpdateLocusMessages(const Wpt *w, u32 w_len){
	if(IsMenu()){
		Cls();
	} else {
//		Locate((IW->tc.view_mode & 1)?0 : 10, 0);
		Locate(0, 0);
		if(!(IW->tc.view_mode & 1)){
			PutsCurTime();
			Locate(10, 0);
		}
		PutsKVal(IW->px.alt_mm);
		Putsf("m %+2.1fm/s ", Range32(-99, 99, RoundDiv(IW->px.up_mm, 100)));

		if(w){
			// 高度差情報
			PutAtAlt(RoundDiv(IW->px.alt_mm, 1000), w, w_len, IW->tc.view_alt | 0x80000000);
		} else {
			// 加速度
			if(IW->px.g_mm < 0) Puts("----");
			else                PutsKVal(IW->px.g_mm);
			Puts("G");
		}


		// 上昇率/回転
		Locate(0, (IW->tc.view_mode & 1)?28 : 18);
		if(IW->px.up_turn == INVALID_VAL){
			PutsSpace(20);
		} else {
			Putsf("%+4.1f m/turn r", RoundDiv(IW->px.up_turn, 100));
			s32 t = IW->px.up_r;
			if(t > 999){
				t = RoundDiv(t, 10);
				if(t > 999) t = 999;
				Putsf("%d", t);
			} else {
				Putsf("%1.1f", t);
			}
			Puts("m  ");
		}
	}
}
void UpdateLocusNESW(s32 cx, s32 cy, s32 ang64, s32 w_ang, u32 w_len){
	// NESW表示
	const StarInfo* p = NESW_INFO;
	s32 id = OBJ_LOCUS_NESW;
	for(; p->tile ; ++p, ++id){
		s32 a2 = p->ang64 - ang64;
		if(IW->tc.view_mode & 1){
			SetNaviObj(id, p->tile + (OBJ_TN_NESW_R - OBJ_TN_NESW), OBJSIZE_16x8,
				cx + Sin64K(a2) * 132 / 0x10000 - 4,
				cy - Cos64K(a2) * 128 / 0x10000 - 0);
		} else {
			SetNaviObj(id, p->tile, OBJSIZE_8x16,
				cx + Sin64K(a2) * 120 / 0x10000 - 0,
				cy - Cos64K(a2) * 112 / 0x10000 - 6);
		}
	}

	// 自機
	if(IW->tc.view_mode & 1) SetNaviObj(id++, OBJ_TN_SELF_R, OBJSIZE_16x8, cx, cy);
	else                     SetNaviObj(id++, OBJ_TN_SELF,   OBJSIZE_8x16, cx, cy - 10);

	// 次パイロン方向
	if(w_ang != -1 && IW->px.lat != INVALID_LAT){
		if(IW->tc.view_mode & 1){
			SetNaviObj(id++, OBJ_TN_PYLN, OBJSIZE_64x64 | A3_ROTATE_D(OBJ_RN_PYLN), -11, 72);
			Locate(14, 26);
		} else {
			SetNaviObj(id++, OBJ_TN_PYLN, OBJSIZE_64x64 | A3_ROTATE_D(OBJ_RN_PYLN), 152, 59);
			Locate(24, 18);
		}
		RotateNaviWin(OBJ_RN_PYLN, w_ang - IW->px.h_ang64, 1024, 768);
		if(!IsMenu()) PutsDistance(w_len);
	}
}

// 軌跡
void UpdateLocusView(const Wpt *w, s32 w_ang, u32 w_len){
	// 特殊スプライトモードに移行する
	if(!SPLITE->flag) InitSpliteTable();

	// 磁石表示
	s32 LOCUS_CX = (IW->tc.view_mode & 1)? 100 : 120;
	s32 LOCUS_CY = (IW->tc.view_mode & 1)?  76 :  80;
	s32 ang64 = IW->px.h_ang_c;
	if(IW->tc.view_mode & 1) ang64 -= 0x4000;
	w_ang = w? (w_ang & 0xffff) : -1;
	UpdateLocusNESW(LOCUS_CX, LOCUS_CY, ang64, w_ang, w_len);

	// 回転計算
	ang64 = 0x8000 - IW->px.h_ang64;
	if(IW->tc.view_mode & 1) ang64 += 0x4000;
	s32 s = Sin64K(ang64) >> 7;
	s32 c = Cos64K(ang64) >> 7;

	// 拡大計算 lat:10000(10s) -> 309m
	s32 index = LOCUS->index, i;
	s32 shift = CountBit(IW->tc.locus_r);
	u32 my = BiosDiv(10125 << shift, IW->tc.locus_r, &i) >> 7;		// locus_r ⇒ 80 dotに変換
	u32 mx = (my * Cos64K(BiosDiv(IW->px.lat, 19776, &i))) >> 15;	// 極に近づかなければ、この近似で充分
	s32 shift2 = shift / 2;
	shift -= shift2 - 13;

	// 次パイロン番号
	s32 id = OBJ_LOCUS_TARGET;
	if(w_ang != -1 && IW->px.lat != INVALID_LAT && IW->tc.task_mode){
		s32 sx = ((IW->px.lon - w->lon) >> shift2) * mx;
		s32 sy = ((IW->px.lat - w->lat) >> shift2) * mx;
		s32 dx = LOCUS_CX + (( c * sx + s * sy) >> shift);
		s32 dy = LOCUS_CY - ((-s * sx + c * sy) >> shift);

		if(IW->tc.view_mode & 1) dx -=  0, dy += 12;
		else                     dx += 12, dy -=  8;
		if(dx > -8 && dx < 240 && dy > -8 && dy < 160){
			id = DispSmallFont(id, dx, dy, IW->mp.cur_point + 1, 2, 0);
		}
	}
	while(id < OBJ_LOCUS) MoveOut(id++);

	// 高度レンジ計算
	s32 alt = RoundDiv(IW->px.alt_mm, 1000);
	ResetSpliteTable();
	s32 end = IW->tc.locus_cnt;
	if(!IW->tc.locus_cnt || end > MAX_LOCUS) end = MAX_LOCUS;
	if(IsMenu()) end = MAX_SPLITE;
	s32 last_lat = IW->mp.cur_lat;
	s32 last_lon = IW->mp.cur_lon;
	for(i = 0 ; i < end ; ++i){
		LocusVal* lv = &LOCUS->val[index];
		if(--index < 0) index = MAX_LOCUS - 1;
		s32 diff = (lv->alt_x >> 1) - alt;

		// メモリ節約のため差分で保持するよう変更
		s32 sx = (IW->px.lon - last_lon) >> shift2;
		s32 sy = (IW->px.lat - last_lat) >> shift2;
		if(*(u32*)&lv->lat_d != INVALID_LAT){
			// 次座標を計算しておく
			if(lv->alt_x & 1){
				last_lat -= (s32)lv->lat_d << LOCUS_BSFT;
				last_lon -= (s32)lv->lon_d << LOCUS_BSFT;
			} else {
				last_lat -= lv->lat_d;
				last_lon -= lv->lon_d;
			}
		}

		// 高度範囲チェック
		if(*(u32*)&lv->lat_d == INVALID_LAT || diff < IW->tc.locus_down || diff > IW->tc.locus_up || myAbs(sx) > 0x4000 || myAbs(sy) > 0x4000){
			continue;
		}

		// 表示範囲チェック
		sx *= mx;
		sy *= my;
		s32 dx = LOCUS_CX + (( c * sx + s * sy) >> shift);
		s32 dy = LOCUS_CY - ((-s * sx + c * sy) >> shift);
		if(dx < -8 || dx > 240 || dy < -8 || dy > 160) continue;

		// 表示確定
		s32 col = IW->tc.locus_range;
		if(col < 1) col = 2000; // デフォルト
		s32 col2 = BiosDiv(col, 5, &col2);
		if(!col2) col2 = 1;
		s32 up = BiosDiv(lv->up + col, col2, &up);
		if(up >= LOCUS_LEVEL) up = LOCUS_LEVEL - 1;
		else if(up < 0)       up = 0;
		AddSpliteTable(dx, dy, OBJ_TN_LOCUS + up * 2);
	}
	UpdateSpliteTable();

	UpdateLocusMessages(w, w_len);
	SPLITE->flag = 3;
}

//#define SP_LIMIT 30 // 30sで終了
//#define SP_LIMIT 999 // 30sで終了
const u32 SP_MODE_COORD[2][6] = {
	{ // 横モード
		200, 112, // タイムアウト値
		168,  24, // 磁石
		196,  48, // 磁石N
	}, { // 縦モード
		32, 128, // タイムアウト値
		16,  16, // 磁石
		40,  44, // 磁石N
	}
};

#define SP_STATE_COUNT_MASK 0xffff
#define SP_STATE_MASK	0xff000000
#define SP_STATE_NEXT	0x01000000
#define SP_STATE_TURN	0x02000000
#define SP_STATE_COMP	0x03000000

#define STABLE_COUNT 6
s32 GetStability(s32* ang, s32* spd){
	*ang = *spd = 0;
	s32 t;
	for(t = 3 ; t < STABLE_COUNT ; ++t){
		if(S32QueueAngDiff(t) < IW->tc.stable_angle * 182) ++*ang;
		if(S32QueueVhDiff (t) < IW->tc.stable_speed)       ++*spd;
	}
	return MinS32(*ang, *spd);
}
s32 GetStability2(){
    s32 d_ang, d_spd;
    return GetStability(&d_ang, &d_spd);
}
void PutStability(){
	s32 d_ang, d_spd, min = GetStability(&d_ang, &d_spd);
	if(min){
		DrawText(0, 3, "Under measurement:");
		Putsf("%2d", STABLE_COUNT - 3 - min);
		PutsSpace2(0, 5, 20);
	} else {
		if(d_ang) PutsSpace2(0, 3, 20);
		else      DrawText  (0, 3, "Go straight!        ");
		if(d_spd) PutsSpace2(0, 5, 20);
		else      DrawText  (0, 5, "Stabilize speed!    ");
	}
}

s32 DispSpCompus(){
	// 北表示
	const u32* coord = SP_MODE_COORD[IW->tc.view_mode & 1];
	s32 ang64 = IW->px.h_ang_c;
	DispBnObj(BN_SPEED,  coord[0], coord[1], (IW->mp.sp_state & SP_STATE_COUNT_MASK) + 1, 1);
	if(IW->mp.sp_state < SP_STATE_COMP){

		if(IW->mp.sp_state < SP_STATE_TURN){
			// 北矢印
			SetNaviObj(OBJ_NW_PYLN, OBJ_TN_PYLN, OBJSIZE_64x64 | A3_ROTATE_N(OBJ_RN_PYLN), coord[2], coord[3]);
			RotateNaviWin(OBJ_RN_PYLN, -ang64, 1024, 512);
			RotateMoveOut(OBJ_RN_NEXT);
			MoveOut(OBJ_STAR_BEGIN);
		} else {
			// 反対方向指示
			SetNaviObj(OBJ_STAR_BEGIN, OBJ_TN_STAR2, OBJSIZE_8x8, coord[2] + 28, coord[3] + 28);
			RotateMoveOut(OBJ_RN_PYLN);
			s32 a2 = IW->mp.sp_angle - ang64;
			RotateNaviWin(OBJ_RN_NEXT, a2, 512, 256);//これは内部的に縦横変換する
			if(IW->tc.view_mode & 1) a2 += 0x4000;
			SetNaviObj(OBJ_NEXT_PYLON, OBJ_TN_NEXT, OBJSIZE_32x32 | A3_ROTATE_D(OBJ_RN_NEXT),
				coord[2] + ((Sin64K(a2) * 16) >> 16),
				coord[3] - ((Cos64K(a2) * 16) >> 16));
		}

		// N表示
		if(IW->tc.view_mode & 1) ang64 -= 0x4000;
		const StarInfo* p = NESW_INFO;
		s32 id = OBJ_LOCUS_NESW;
		s32 a2 = p->ang64 - ang64;
		if(IW->tc.view_mode & 1){
			SetNaviObj(id, p->tile + (OBJ_TN_NESW_R - OBJ_TN_NESW), OBJSIZE_16x8,
				coord[4] + Sin64K(a2) * 80 / 0x10000,
				coord[5] - Cos64K(a2) * 72 / 0x10000);
		} else {
			SetNaviObj(id, p->tile, OBJSIZE_8x16,
				coord[4] + Sin64K(a2) * 72 / 0x10000,
				coord[5] - Cos64K(a2) * 80 / 0x10000);
		}
	} else {
		RotateMoveOut(OBJ_RN_PYLN);
		RotateMoveOut(OBJ_RN_NEXT);
		MoveOut(OBJ_LOCUS_NESW);
		MoveOut(OBJ_STAR_BEGIN);
	}
	return 0;
}

s32 DispSpObj(s32 vh_avg, s32 up_avg){
	// Init
	if(!IW->mp.sp_state) IW->mp.sp_state = IW->tc.wait_timeout + IW->tc.init_wait;

	// タイムアウトチェック
	s32 t = IW->mp.sp_state & SP_STATE_COUNT_MASK;
	if(!t){
		IW->mp.sp_state = -1; // SPモードの終了
		//メニュー中でも強制復帰
		PlaySG1(SG1_CANCEL);
		IW->mp.sp_view = SP_VIEW_NONE;
		return -1;
	}
	if(IW->mp.navi_update & NAVI_PVT_CHANGE) IW->mp.sp_state--;

	// ステート別処理
	switch(IW->mp.sp_state >> 24){
	case 0: // 安定待ち
		if(t <= IW->tc.wait_timeout) IW->mp.sp_state += SP_STATE_NEXT;
		break;

	case 1: // 測定中
		if(GetStability2() > STABLE_COUNT - 4){ // 一次測定完了?
			IW->mp.sp_angle = (IW->px.h_ang_c + 0x8000) & 0xffff;
			IW->mp.sp_speed1= vh_avg;
			IW->mp.sp_up1	= up_avg;
			IW->mp.sp_state &= ~SP_STATE_COUNT_MASK;
			IW->mp.sp_state += SP_STATE_NEXT + IW->tc.wait_timeout;
			PlaySG1(SG1_COMP1);
		}
		break;

	case 2: // ターン
		if(GetAngDiff(IW->mp.sp_angle - IW->px.h_ang_c) <= IW->tc.stable_angle * 182){
			if(GetStability2() > STABLE_COUNT - 4){ // 二次測定完了?
				IW->mp.sp_state &= ~SP_STATE_COUNT_MASK;
				IW->mp.sp_state += SP_STATE_NEXT + IW->tc.comp_timeout;
				IW->mp.sp_up1    = up_avg;
				IW->mp.sp_speed1 = vh_avg;
				PlaySG1(SG1_COMP2);
			}
		}
		break;

	case 3: // 完了
		break;
	}

	// ステート別表示
	switch(IW->mp.sp_state >> 24){
	case 0: // 安定待ち
		// 方向メッセージ
		DrawText(0, 3, "Glide to windward   ");
		DrawText(0, 5, "         or leeward!");
		break;

	case 1: // 測定中
		PutStability();
		break;

	case 2: // ターン
		t = GetAngDiff(IW->mp.sp_angle - IW->px.h_ang_c);
		if(t > IW->tc.stable_angle * 182){
			DrawText(0, 3, "Turn ");
			Putc((GetAngLR(IW->mp.sp_angle - IW->px.h_ang_c) < 0)? 'L' : 'R');
			Putsf("%d° in order  ", (t * 360) >> 16);
			DrawText(0, 5, "to improve accuracy.");
		} else {
			PutStability();
		}
		break;

	case 3: // 完了
		DrawText(0, 3, "  <> COMPLETE <>    ");
		PutsSpace2(0, 5, 20);
		break;
	}
	DispSpCompus();
	return 0;
}

void GetAvgData(s32* vh, s32* up, s32* ldk){
	*vh  = AvgS32Queue(&IW->pr.vh_mm, STABLE_COUNT);
	*up  = AvgS32Queue(&IW->pr.up_mm, STABLE_COUNT);
	if(!ldk) return;
	switch(IW->mp.sp_state >> 24){
	case 0: // 安定待ち
	case 1: // 測定中
		break;
	case 2: // ターン
		*vh = (*vh + IW->mp.sp_speed1) / 2;
		*up = (*up + IW->mp.sp_up1   ) / 2;
		break;
	case 3: // 完了
		*vh = IW->mp.sp_speed1;
		*up = IW->mp.sp_up1;
		break;
	}
	*ldk = *up? BiosDiv(*vh * -1000, *up, ldk) : INVALID_VAL;
}

void UpdateBenchmarkView(){
	s32 vh, up, ldk;
	GetAvgData(&vh, &up, &ldk);
	DrawText(0, 0, "-- Benchmark mode --");
	DrawText(0, 8, "Speed:     ");
	PutsKVal(vh);
	Puts("m/s");

	DrawText(0, 10, "Sink rate: ");
	PutsKVal(-up);
	Puts("m/s");

	DrawText(0, 12, "L/D:      ");
	PutsLdK(ldk);

	if(DispSpObj(vh, up) == 2 && IW->tc.bench_update && ldk != INVALID_VAL){
		IW->tc.my_ldK  = Range32(1, 99999, ldk);
		IW->tc.my_down = Range32(1,  9999, -up);
	}
}

// 本来は↓のようにすべきだけど、とりあえず、22.5°(0x1000)で分割
//345°～15°: N     (30°)       
// 15°～37°: NNE   (22°)
// 38°～52°: NE    (14°) 
// 53°～75°: ENE   (22°)
// 75°～90°: E     (30°)
const char* const NESW16[16] = {
	" N ", "NNE", "NE ", "ENE",
	" E ", "ESE", "SE ", "SSE",
	" S ", "SSW", "SW ", "WSW",
	" W ", "WNW", "NW ", "NNW",
};

void UpdateWindCheckView(){
	s32 vh, up, ms = 0;
	GetAvgData(&vh, &up, 0);
	DrawText(0, 0, "- Wind Check mode -");

	s32 spd = IW->tc.my_speed;
	s32 ang = IW->px.h_ang_c;
	switch(IW->mp.sp_state >> 24){
	case 0: // 安定待ち
	case 1: // 測定中
		DrawText(0, 12, "(GliderSpd:");
		PutsKVal(spd);
		Puts("m/s)");
		spd -= vh; // 速度差
		break;

	case 2: // ターン
		spd = IW->mp.sp_speed1 - vh;
		ms  = (IW->mp.sp_speed1 + vh) / 2;
		vh = spd;
		PutsSpace2(0, 12, 20);
		break;

	case 3: // 完了
		spd = IW->mp.sp_speed1;
		ang = IW->mp.sp_up1;
		break;
	}
	if(spd < 0){
		spd = -spd;
		ang += 0x8000;
	}
	DrawText(0, 8, "Wind spd: ");
	PutsKVal(spd);
	Puts("m/s");

	DrawText(0, 10, "Windward: ");
	Putsf("%03d°(%s)", ((ang & 0xffff) * 360 + 0x8000) >> 16, NESW16[((ang + 0x500) & 0xffff) >> 12]);

	if(DispSpObj(vh, ang) == 2 && IW->tc.wind_update){
		IW->tc.my_speed  = Range32(1, 99999, ms);
	}
}

///////////////////////////////////////////////////////////////////////////////
// ナビ画面設定
///////////////////////////////////////////////////////////////////////////////
// 表示情報定義用
typedef struct {
	u32 id;
	s16 x;
	s16 y;
} NaviInfo;

// 表示タイプ
enum {
	NAVI_END,//リスト終了フラグ

	// GPS情報
	NAVI_STATUS,		// 測位状態
	NAVI_V_STATUS,		// 測位状態(低優先度)
	NAVI_CIRCLE,		// 外枠
	NAVI_DIRECTION,		// 磁石(星)
	NAVI_STARONLY,		// 磁石(星のみ)
	NAVI_BSPEED,		// 対地速度(拡大)
	NAVI_BSPEED_UNIT1,	// 対地速度単位1(拡大)
	NAVI_BSPEED_UNIT1X,	// 対地速度単位1(拡大) メニュー時消去
	NAVI_ALT,			// 高度
	NAVI_BALT,			// 高度(拡大)
	NAVI_BALT_UNIT1,	// 高度単位(拡大)
	NAVI_VARIO,			// バリオ
	NAVI_VARIO_GRAPH,	// バリオグラフ
	NAVI_LD,			// L/D
	NAVI_LD2,			// Goal L/D, Near L/D
	NAVI_BTIME,			// 時刻(拡大)
	NAVI_BTIME_UNIT,	// 時刻(拡大)
	NAVI_LAT,			// 緯度(拡大)
	NAVI_LAT_UNIT,		// 緯度(拡大)
	NAVI_LON,			// 経度(拡大)
	NAVI_LON_UNIT,		// 経度(拡大)
	NAVI_LATLON_UNIT,	// 緯度(拡大)
	NAVI_GPS_WARN,		// GPS警告
	NAVI_TIME,			// 時刻
	NAVI_V_DISP,		// 垂直表示
	NAVI_DEBUG1,		// デバッグ情報
	NAVI_DEBUG2,		// デバッグ情報
	NAVI_BANEMO,		// 大気速度(拡大)
	NAVI_BANEMO_UNIT1,	// 大気速度単位1(拡大)
	NAVI_BANEMO_UNIT2,	// 大気速度単位2(拡大)
	NAVI_ANEMO_DIF,		// 風速

	// パイロン情報
	NAVI_WPTNAME,		// WPT名
	NAVI_WPTNAMEX,		// WPT名 メニュー時消去
	NAVI_WPTNAME_INI,	// WPT名(強制イニシャル)
	NAVI_WPTNAME1,		// WPT名(次パイロンフル)
	NAVI_WPTNAME2,		// WPT名(次次パイロンフル)
	NAVI_WPTNUM,		// WPT番号
	NAVI_WPTNUM_V,		// WPT番号
	NAVI_ROUTENAME,		// ルート名
	NAVI_PYLON_DIR,		// パイロン方向
	NAVI_BPYLON_LEN,	// パイロン距離(拡大)
	NAVI_BPYLON_UNIT,	// パイロン距離単位(拡大)
	NAVI_BPYLON_UNITX,	// パイロン距離単位(拡大) メニュー時消去
	NAVI_BPYLON2_LEN,	// 次次パイロン距離(拡大)
	NAVI_BPYLON2_UNIT,	// 次次パイロン距離単位(拡大)
	NAVI_PYLON_DIFF,	// パイロン高度差
	NAVI_PYLON2_DIFF,	// 次次パイロン高度差
	NAVI_GOAL_INFO,		// ゴールパイロン情報
	NAVI_GOAL_LEN,		// ゴールパイロン距離
	NAVI_PYLON_DIR_NUM,	// パイロン方向(数字表示)
	NAVI_PYLON2_DIR_NUM,// 次次パイロン方向(数字表示)
	NAVI_PYLON_DIR_N2,	// パイロン方向(右詰数字)
	NAVI_PYLON_DIR_N2X,	// パイロン方向(右詰数字)(メニュー無視)
	NAVI_PYLON2_DIR_N2,	// 次次パイロン方向(右詰数字)
	NAVI_P2_DIR,		// 次次パイロン方向
	NAVI_A_TIME,		// 予想到着時間
	NAVI_V_PYLN,		// 垂直表示 パイロン
	NAVI_V_PYLN2,		// 垂直表示 次パイロン
	NAVI_CYL_MAP,		// シリンダマップ

	// SPモード
	NAVI_SP_LOCUS,		// 軌跡モード
	// ゴール
	NAVI_GOAL,			// ゴールメッセージ
};

///////////////////////////////////////////////////////////////////////////////
// 横画面用 表示テーブル
#define NCX 91
#define NCY 80
const NaviInfo GPS_INFO_1a[] = {
	// GPS情報
	{NAVI_STATUS,		NCX,NCY},	// 測位状態
	{NAVI_CIRCLE,		NCX,NCY},	// 外枠
	{NAVI_DIRECTION,	NCX,NCY},	// 磁石(星)
	{NAVI_BSPEED,		207, 40},	// 対地速度
	{NAVI_BSPEED_UNIT1,	 28,  5},	// 対地速度単位1
	{NAVI_ALT,			 21, 12},	// 高度
//	{NAVI_VARIO,		 21, 16},	// バリオ
	{NAVI_LD2,			 21, 16},	// L/D
	{NAVI_VARIO_GRAPH,	  4, 77},	// バリオメータ
	{NAVI_LD,			 21, 18},	// L/D
	{NAVI_TIME,			  3,  0},	// 時刻

	// パイロン情報
	{NAVI_WPTNAME,		  3, 18},	// WPT名
	{NAVI_PYLON_DIR,    NCX,NCY},	// パイロン方向
	{NAVI_BPYLON_LEN,	207,  0},	// パイロン距離
	{NAVI_BPYLON_UNIT,	 28,  2},	// パイロン距離単位
	{NAVI_PYLON_DIFF,	 21, 14},	// パイロン高度差
//	{NAVI_PYLON_DIR_N2X, 16, 16},	// パイロン方向(数字表示)
	{NAVI_GOAL_LEN,		 12,  0},	// ゴール距離
	
	{NAVI_P2_DIR,		NCX,NCY},	// 次次パイロン方向
	{NAVI_A_TIME,		 20,  9},	// 予想到着時間

	// ゴールメッセージ
	{NAVI_GOAL,			  3, 18},	// ゴールメッセージ

	// 終了
	{NAVI_END, 0, 0}
};
#undef NCX
#undef NCY

///////////////////////////////////////////////////////////////////////////////
// 縦画面用 表示テーブル
#define NCX 160
#define NCY 91
const NaviInfo GPS_INFO_1b[] = {
	// GPS情報
	{NAVI_STATUS,		NCX,NCY},	// 測位状態
	{NAVI_CIRCLE,		NCX,NCY},	// 外枠
	{NAVI_DIRECTION,	NCX,NCY},	// 磁石(星)
	{NAVI_BSPEED,		 32,127},	// 対地速度
	{NAVI_BSPEED_UNIT1,	 18, 22},	// 対地速度単位1
	{NAVI_ALT,			  0, 20},	// 高度
//	{NAVI_VARIO,		  0, 24},	// バリオ
	{NAVI_VARIO_GRAPH,NCX-3,  4},	// バリオメータ
	{NAVI_LD,			  0, 26},	// L/D
	{NAVI_LD2,			  0, 24},	// L/D
	{NAVI_TIME,			  3,  0},	// 時刻

	// パイロン情報
	{NAVI_WPTNAME,		  2, 28},	// WPT名
	{NAVI_PYLON_DIR,    NCX,NCY},	// パイロン方向
	{NAVI_BPYLON_LEN,	 64,127},	// パイロン距離
	{NAVI_BPYLON_UNIT,	 18, 20},	// パイロン距離単位
	{NAVI_PYLON_DIFF,	  0, 22},	// パイロン高度差
	{NAVI_PYLON_DIR_N2X,  3, 17},	// パイロン方向(数字表示)
	{NAVI_GOAL_LEN,		 13,  0},		// ゴール距離
	
	{NAVI_P2_DIR,		NCX,NCY},	// 次次パイロン方向
	{NAVI_A_TIME,		 10, 26},	// 予想到着時間

	// ゴールメッセージ
	{NAVI_GOAL,			  2, 28},	// ゴールメッセージ

	{NAVI_END, 0, 0}
};
#undef NCX
#undef NCY


///////////////////////////////////////////////////////////////////////////////
// シンプル横
#define NCX 91
#define NCY 80
const NaviInfo GPS_INFO_2a[] = {
	// GPS情報
	{NAVI_STATUS,		NCX,NCY},	// 測位状態
	{NAVI_CIRCLE,		NCX,NCY},	// 外枠
	{NAVI_DIRECTION,	NCX,NCY},	// 磁石(星)
	{NAVI_BSPEED,		207, 64},	// 対地速度
	{NAVI_BSPEED_UNIT1,	 28,  8},	// 対地速度単位1
	{NAVI_BALT,			208,112},	// 高度
	{NAVI_BALT_UNIT1,	 17, 16},	// 高度単位1
	{NAVI_VARIO_GRAPH,	  4, 77},	// バリオメータ
	{NAVI_TIME,			  7,  0},	// 時刻

	// パイロン情報
	{NAVI_WPTNAME,		  3, 18},	// WPT名
//	{NAVI_ROUTENAME,	  3,  0},	// ルート名
	{NAVI_PYLON_DIR,    NCX,NCY},	// パイロン方向
	{NAVI_BPYLON_LEN,	207, 16},	// パイロン距離
	{NAVI_BPYLON_UNIT,	 28,  4},	// パイロン距離単位
	{NAVI_P2_DIR,		NCX,NCY},	// 次次パイロン方向

	// ゴールメッセージ
	{NAVI_GOAL,			  3, 18},	// ゴールメッセージ

	// 終了
	{NAVI_END, 0, 0}
};
#undef NCX
#undef NCY

///////////////////////////////////////////////////////////////////////////////
// シンプル縦
#define NCX 156
#define NCY 91
const NaviInfo GPS_INFO_2b[] = {
	// GPS情報
	{NAVI_STATUS,		NCX,NCY},	// 測位状態
	{NAVI_CIRCLE,		NCX,NCY},	// 外枠
	{NAVI_DIRECTION,	NCX,NCY},	// 磁石(星)
	{NAVI_BSPEED,		 16,128},	// 対地速度
	{NAVI_BSPEED_UNIT1,	 18, 24},	// 対地速度単位1
	{NAVI_BALT,			 56,128},	// 高度
	{NAVI_BALT_UNIT1,	  7, 21},	// 高度単位1
	{NAVI_VARIO_GRAPH,NCX-3,  4},	// バリオメータ
	{NAVI_TIME,			  7,  0},	// 時刻

	// パイロン情報
	{NAVI_WPTNAME,		  2, 28},	// WPT名
//	{NAVI_ROUTENAME,	  4,  0},	// ルート名
	{NAVI_PYLON_DIR,    NCX,NCY},	// パイロン方向
	{NAVI_BPYLON_LEN,	 16, 48},	// パイロン距離
	{NAVI_BPYLON_UNIT,	  8, 26},	// パイロン距離単位
	{NAVI_P2_DIR,		NCX,NCY},	// 次次パイロン方向

	// ゴールメッセージ
	{NAVI_GOAL,			  2, 28},	// ゴールメッセージ

	{NAVI_END, 0, 0}
};
#undef NCX
#undef NCY

///////////////////////////////////////////////////////////////////////////////
// 垂直横
#define NCX 112
#define NCY 64
const NaviInfo GPS_INFO_3a[] = {
	// GPS情報
	{NAVI_V_STATUS,		NCX,NCY},	// 測位状態
	{NAVI_CIRCLE,		NCX,NCY},	// 外枠
	{NAVI_V_DISP,       NCX,NCY},	// 垂直表示
	{NAVI_BSPEED,		207,  0},	// 対地速度
	{NAVI_BSPEED_UNIT1,	 28,  0},	// 対地速度単位1

	// パイロン情報
	{NAVI_V_PYLN,       NCX,NCY},	// 垂直表示
	{NAVI_V_PYLN2,      NCX,NCY},	// 垂直表示
	{NAVI_WPTNUM,		 28, 12},	// WPT番号
	{NAVI_PYLON_DIR_N2,  26, 14},	// パイロン方向(右詰数字)

	// ゴールメッセージ
	{NAVI_GOAL,			  6,  5},	// ゴールメッセージ

	{NAVI_END, 0, 0}
};
#undef NCX
#undef NCY

///////////////////////////////////////////////////////////////////////////////
// 垂直縦
#define NCX 112
#define NCY 90
const NaviInfo GPS_INFO_3b[] = {
	// GPS情報
	{NAVI_V_STATUS,		NCX,NCY},	// 測位状態
	{NAVI_CIRCLE,		NCX,NCY},	// 外枠
	{NAVI_V_DISP,       NCX,NCY},	// 垂直表示
	{NAVI_BSPEED,		207,126},	// 対地速度
	{NAVI_BSPEED_UNIT1,	 18,  0},	// 対地速度単位1

	// パイロン情報
	{NAVI_V_PYLN,       NCX,NCY},	// 垂直表示
	{NAVI_V_PYLN2,      NCX,NCY},	// 垂直表示
	{NAVI_WPTNUM_V,		  4,  0},	// WPT番号
	{NAVI_PYLON_DIR_NUM,  4,  2},	// パイロン方向(数字表示)

	// ゴールメッセージ
	{NAVI_GOAL,			  3,  5},	// ゴールメッセージ

	{NAVI_END, 0, 0}
};
#undef NCX
#undef NCY


///////////////////////////////////////////////////////////////////////////////
// パイロンx2横
#define NCX 67
#define NCY 80
const NaviInfo GPS_INFO_4a[] = {
	// GPS情報
	{NAVI_STATUS,		NCX,NCY},	// 測位状態
	{NAVI_CIRCLE,		NCX,NCY},	// 外枠
	{NAVI_DIRECTION,	NCX,NCY},	// 磁石(星)

	// パイロン情報
	{NAVI_TIME,			  4,  0},	// 時刻
	{NAVI_WPTNAME_INI,	  0, 18},	// WPT名

	{NAVI_WPTNAME1,		 17,  0},	// WPT1名
	{NAVI_BPYLON_LEN,	207, 16},	// パイロン距離
	{NAVI_BPYLON_UNIT,	 28,  4},	// パイロン距離単位
	{NAVI_PYLON_DIFF,    19,  6},	// パイロン高度差
//	{NAVI_PYLON_DIR_N2X, 26,  6},	// パイロン方向(右詰数字)(メニュー無視)

	{NAVI_WPTNAME2,		 17,  9},	// WPT2名
	{NAVI_BPYLON2_LEN,	207, 88},	// パイロン距離
	{NAVI_BPYLON2_UNIT,	 28, 13},	// パイロン距離単位
	{NAVI_PYLON2_DIFF,   19, 15},	// パイロン高度差
//	{NAVI_PYLON2_DIR_NUM,26, 15},	// パイロン方向(右詰数字)

	{NAVI_GOAL_INFO,	 17, 18},

//	{NAVI_ROUTENAME,	  1, 18},	// ルート名
	{NAVI_PYLON_DIR,    NCX,NCY},	// パイロン方向
	{NAVI_P2_DIR,		NCX,NCY},	// 次次パイロン方向

	// ゴールメッセージ
	{NAVI_GOAL,			  1, 18},	// ゴールメッセージ

	// 終了
	{NAVI_END, 0, 0}
};
#undef NCX
#undef NCY

///////////////////////////////////////////////////////////////////////////////
// パイロンx2縦
#define NCX 112
#define NCY 81
const NaviInfo GPS_INFO_4b[] = {
	// GPS情報
	{NAVI_STATUS,		NCX,NCY},	// 測位状態
	{NAVI_CIRCLE,		NCX,NCY},	// 外枠
	{NAVI_DIRECTION,	NCX,NCY},	// 磁石(星)

	// パイロン情報
//	{NAVI_WPTNAME_INI,	  2, 28},	// WPT名
	{NAVI_TIME,			 12,  0},	// 時刻

	{NAVI_WPTNAME1,		  1,  2},	// WPT1名
	{NAVI_BPYLON_LEN,	176, 62},	// パイロン距離
	{NAVI_BPYLON_UNIT,	 10,  6},	// パイロン距離単位
	{NAVI_PYLON_DIFF,    11,  4},	// パイロン高度差
	{NAVI_PYLON_DIR_N2X, 13,  6},	// パイロン方向(右詰数字)(メニュー無視)

	{NAVI_WPTNAME2,		  1, 24},	// WPT2名
	{NAVI_BPYLON2_LEN,	  0, 62},	// パイロン距離
	{NAVI_BPYLON2_UNIT,	 10, 28},	// パイロン距離単位
	{NAVI_PYLON2_DIFF,   11, 26},	// パイロン高度差
	{NAVI_PYLON2_DIR_NUM,13, 28},	// パイロン方向(右詰数字)

	{NAVI_PYLON_DIR,    NCX,NCY},	// パイロン方向
	{NAVI_P2_DIR,		NCX,NCY},	// 次次パイロン方向

	// ゴールメッセージ
	{NAVI_GOAL,			  2, 25},	// ゴールメッセージ

	{NAVI_END, 0, 0}
};
#undef NCX
#undef NCY

///////////////////////////////////////////////////////////////////////////////
// 風速計
#define NCX 91
#define NCY 80
const NaviInfo GPS_INFO_5a[] = {
	// GPS情報
	{NAVI_STATUS,		NCX,NCY},	// 測位状態
	{NAVI_CIRCLE,		NCX,NCY},	// 外枠
	{NAVI_DIRECTION,	NCX,NCY},	// 磁石(星)
	{NAVI_BSPEED,		207, 32},	// 対地速度
	{NAVI_BSPEED_UNIT1,	 28,  4},	// 対地速度単位1
	{NAVI_ALT,			 21, 11},	// 高度
//	{NAVI_VARIO,		 21, 16},	// バリオ
	{NAVI_VARIO_GRAPH,	  4, 77},	// バリオメータ
	{NAVI_LD,			 21, 13},	// L/D
	{NAVI_TIME,			  7,  0},	// 時刻
	{NAVI_BANEMO,		207,  0},	// 大気速度
	{NAVI_BANEMO_UNIT1,	 28,  0},	// 大気速度単位1
	{NAVI_BANEMO_UNIT2,	 17,  0},	// 大気速度単位2
	{NAVI_ANEMO_DIF,	 22,  8},	// 風速

	// パイロン情報
	{NAVI_WPTNAME,		  3, 18},	// WPT名
	{NAVI_PYLON_DIR,    NCX,NCY},	// パイロン方向
	{NAVI_BPYLON_LEN,	207,128},	// パイロン距離
	{NAVI_BPYLON_UNIT,	 28, 18},	// パイロン距離単位
//	{NAVI_PYLON_DIFF,	 24, 14},	// パイロン高度差
//	{NAVI_PYLON_DIR_N2X, 14,  0},	// パイロン方向(数字表示)
	{NAVI_P2_DIR,		NCX,NCY},	// 次次パイロン方向
//	{NAVI_A_TIME,		 20,  9},	// 予想到着時間

	// ゴールメッセージ
	{NAVI_GOAL,			  3, 18},	// ゴールメッセージ

	// 終了
	{NAVI_END, 0, 0}
};
#undef NCX
#undef NCY

///////////////////////////////////////////////////////////////////////////////
// 縦画面用 風速計
#define NCX 160
#define NCY 91
const NaviInfo GPS_INFO_5b[] = {
	// GPS情報
	{NAVI_STATUS,		NCX,NCY},	// 測位状態
	{NAVI_CIRCLE,		NCX,NCY},	// 外枠
	{NAVI_DIRECTION,	NCX,NCY},	// 磁石(星)
	{NAVI_BSPEED,		 32,127},	// 対地速度
	{NAVI_BSPEED_UNIT1,	 18, 22},	// 対地速度単位1
	{NAVI_ALT,			  0, 20},	// 高度
//	{NAVI_VARIO,		  0, 24},	// バリオ
	{NAVI_VARIO_GRAPH,NCX-3,  4},	// バリオメータ
	{NAVI_LD,			  0, 22},	// L/D
	{NAVI_TIME,			  7,  0},	// 時刻
	{NAVI_BANEMO,		 64,127},	// 大気速度
	{NAVI_BANEMO_UNIT1,	 18, 18},	// 対地速度単位1
	{NAVI_BANEMO_UNIT2,	  7, 18},	// 対地速度単位1
//	{NAVI_BANEMO_UNIT2,	 17, 16},	// 対地速度単位1
	{NAVI_ANEMO_DIF,	 12, 26},	// 風速

	// パイロン情報
	{NAVI_WPTNAME,		  2, 28},	// WPT名
	{NAVI_PYLON_DIR,    NCX,NCY},	// パイロン方向
	{NAVI_BPYLON_LEN,	 16, 48},	// パイロン距離
	{NAVI_BPYLON_UNIT,	  8, 26},	// パイロン距離単位
//	{NAVI_BPYLON_LEN,	 64,127},	// パイロン距離
//	{NAVI_BPYLON_UNIT,	 18, 20},	// パイロン距離単位
//	{NAVI_PYLON_DIFF,	  3, 22},	// パイロン高度差
//	{NAVI_PYLON_DIR_N2X,  3, 17},	// パイロン方向(数字表示)
	{NAVI_P2_DIR,		NCX,NCY},	// 次次パイロン方向
//	{NAVI_A_TIME,		 10, 26},	// 予想到着時間

	// ゴールメッセージ
	{NAVI_GOAL,			  2, 28},	// ゴールメッセージ

	{NAVI_END, 0, 0}
};
#undef NCX
#undef NCY

///////////////////////////////////////////////////////////////////////////////
// シリンダマップ横
//#define NCX 120
//#define NCY 128
const NaviInfo GPS_INFO_6a[] = {
	// GPS情報
	{NAVI_STATUS,		 64,152},	// 測位状態
//	{NAVI_CIRCLE,		NCX,NCY},	// 外枠
//	{NAVI_DIRECTION,	NCX,NCY},	// 磁石(星)
	{NAVI_BSPEED,		207,  0},	// 対地速度
	{NAVI_BSPEED_UNIT1X, 28,  0},	// 対地速度単位1
//	{NAVI_BALT,			208,112},	// 高度
//	{NAVI_BALT_UNIT1,	 17, 16},	// 高度単位1
//	{NAVI_VARIO_GRAPH,	  4, 77},	// バリオメータ
	{NAVI_TIME,			 22, 16},	// 時刻


	// パイロン情報
	{NAVI_CYL_MAP,		  0,  0},	// シリンダ
	{NAVI_WPTNAMEX,		  9, 18},	// WPT名
//	{NAVI_ROUTENAME,	  3,  0},	// ルート名
//	{NAVI_PYLON_DIR,    NCX,NCY},	// パイロン方向
	{NAVI_BPYLON_LEN,	 48,  0},	// パイロン距離
	{NAVI_BPYLON_UNITX,	  8,  2},	// パイロン距離単位
//	{NAVI_P2_DIR,		NCX,NCY},	// 次次パイロン方向

	// ゴールメッセージ
	{NAVI_GOAL,			  9, 18},	// ゴールメッセージ

	// 終了
	{NAVI_END, 0, 0}
};
//#undef NCX
//#undef NCY

///////////////////////////////////////////////////////////////////////////////
// シリンダマップ縦
//#define NCX 32
//#define NCY 80
const NaviInfo GPS_INFO_6b[] = {
	// GPS情報
	{NAVI_STATUS,		  8, 16},	// 測位状態
//	{NAVI_CIRCLE,		NCX,NCY},	// 外枠
//	{NAVI_DIRECTION,	NCX,NCY},	// 磁石(星)
	{NAVI_BSPEED,		208,128},	// 対地速度
	{NAVI_BSPEED_UNIT1X,	 18,  0},	// 対地速度単位1
//	{NAVI_BALT,			 56,128},	// 高度
//	{NAVI_BALT_UNIT1,	  7, 21},	// 高度単位1
//	{NAVI_VARIO_GRAPH,NCX-3,  4},	// バリオメータ
	{NAVI_TIME,			 12, 26},	// 時刻

	// パイロン情報
	{NAVI_CYL_MAP,		  0,  0},	// シリンダ
	{NAVI_WPTNAMEX,		  3, 28},	// WPT名
//	{NAVI_ROUTENAME,	  4,  0},	// ルート名
//	{NAVI_PYLON_DIR,    NCX,NCY},	// パイロン方向
	{NAVI_BPYLON_LEN,	208, 48},	// パイロン距離
	{NAVI_BPYLON_UNITX,	  8,  2},	// パイロン距離単位
//	{NAVI_P2_DIR,		NCX,NCY},	// 次次パイロン方向

	// ゴールメッセージ
	{NAVI_GOAL,			  3, 28},	// ゴールメッセージ

	{NAVI_END, 0, 0}
};
//#undef NCX
//#undef NCY

///////////////////////////////////////////////////////////////////////////////
// 軌跡横
const NaviInfo GPS_INFO_9a[] = {
	{NAVI_SP_LOCUS,		  0,  0},	// ゴールメッセージ

	// 終了
	{NAVI_END, 0, 0}
};
#undef NCX
#undef NCY

///////////////////////////////////////////////////////////////////////////////
// 軌跡縦
const NaviInfo GPS_INFO_9b[] = {
	{NAVI_SP_LOCUS,		  0,  0},	// ゴールメッセージ

	{NAVI_END, 0, 0}
};
#undef NCX
#undef NCY

///////////////////////////////////////////////////////////////////////////////
// テキスト横
const NaviInfo GPS_INFO_7a[] = {
	// GPS情報
	{NAVI_LAT,			206, 16},
	{NAVI_LAT_UNIT,		  3,  4},
	{NAVI_LATLON_UNIT,	 11,  2},
	{NAVI_LON,			206, 56},
	{NAVI_LON_UNIT,		  3,  9},
	{NAVI_LATLON_UNIT,	 11,  7},
	{NAVI_BALT,			208,120},
	{NAVI_BALT_UNIT1,	 17, 17},
	{NAVI_BTIME,		104,120},
	{NAVI_BTIME_UNIT,	  5, 15},
	{NAVI_GPS_WARN,		  6, 12},

	// 終了
	{NAVI_END, 0, 0}
};

///////////////////////////////////////////////////////////////////////////////
// テキスト縦
const NaviInfo GPS_INFO_7b[] = {
	// GPS情報
	{NAVI_LAT,			160,134},
	{NAVI_LAT_UNIT,		  1,  4},
	{NAVI_LATLON_UNIT,	  9,  2},
	{NAVI_LON,			 80,134},
	{NAVI_LON_UNIT,		  1, 14},
	{NAVI_LATLON_UNIT,	  9, 12},
	{NAVI_BALT,			 32,128},
	{NAVI_BALT_UNIT1,	  7, 24},
//	{NAVI_BTIME,		 16,128}, // 縦画面はスペースが厳しいので
//	{NAVI_BTIME_UNIT,	  8, 24}, // 時計は表示しない…
	{NAVI_GPS_WARN,		  1, 27},

	// 終了
	{NAVI_END, 0, 0}
};

///////////////////////////////////////////////////////////////////////////////
// デバッグ横
#define NCX 174
#define NCY 80
const NaviInfo GPS_INFO_Xa[] = {
	// GPS情報
//	{NAVI_STATUS,		NCX,NCY},	// 測位状態
//	{NAVI_CIRCLE,		NCX,NCY},	// 外枠
//	{NAVI_STARONLY,		NCX,NCY},	// 磁石(星)
	{NAVI_DEBUG1,		 15,  3},	// デバッグ
	{NAVI_DEBUG2,		  0,  3},	// デバッグ

	// パイロン情報
//	{NAVI_WPTNAME,		  0, 18},	// WPT名
//	{NAVI_ROUTENAME,	  0,  0},	// ルート名
//	{NAVI_PYLON_DIR,    NCX,NCY},	// パイロン方向
//	{NAVI_P2_DIR,		NCX,NCY},	// 次次パイロン方向

	// ゴールメッセージ
//	{NAVI_GOAL,			  0, 18},	// ゴールメッセージ

	// 終了
	{NAVI_END, 0, 0}
};
#undef NCX
#undef NCY

///////////////////////////////////////////////////////////////////////////////
// デバッグ縦
#define NCX 172
#define NCY  80
const NaviInfo GPS_INFO_Xb[] = {
	// GPS情報
//	{NAVI_STATUS,		NCX,NCY},	// 測位状態
//	{NAVI_CIRCLE,		NCX,NCY},	// 外枠
//	{NAVI_STARONLY,		NCX,NCY},	// 磁石(星)
	{NAVI_DEBUG1,		  3, 15},	// デバッグ
	{NAVI_DEBUG2,		  3,  0},	// デバッグ

	// パイロン情報
//	{NAVI_WPTNAME,		  3, 28},	// WPT名
//	{NAVI_ROUTENAME,	  4, 10},	// ルート名
//	{NAVI_PYLON_DIR,    NCX,NCY},	// パイロン方向
//	{NAVI_P2_DIR,		NCX,NCY},	// 次次パイロン方向

	// ゴールメッセージ
//	{NAVI_GOAL,			  3, 28},	// ゴールメッセージ

	{NAVI_END, 0, 0}
};
#undef NCX
#undef NCY

const NaviInfo* const NAVI_INFO[] = {
	GPS_INFO_1a, GPS_INFO_1b, GPS_INFO_1a, GPS_INFO_1b, 
	GPS_INFO_2a, GPS_INFO_2b, GPS_INFO_2a, GPS_INFO_2b,
	GPS_INFO_3a, GPS_INFO_3b, GPS_INFO_3a, GPS_INFO_3b,
	GPS_INFO_4a, GPS_INFO_4b, GPS_INFO_4a, GPS_INFO_4b,
	GPS_INFO_5a, GPS_INFO_5b, GPS_INFO_5a, GPS_INFO_5b,
	GPS_INFO_6a, GPS_INFO_6b, GPS_INFO_6a, GPS_INFO_6b,
	GPS_INFO_9a, GPS_INFO_9b, GPS_INFO_9a, GPS_INFO_9b,
	GPS_INFO_7a, GPS_INFO_7b, GPS_INFO_7a, GPS_INFO_7b,
	GPS_INFO_Xa, GPS_INFO_Xb, GPS_INFO_Xa, GPS_INFO_Xb,
};

///////////////////////////////////////////////////////////////////////////////
// ナビ画面表示
///////////////////////////////////////////////////////////////////////////////
void UpdateGpsInfo(u32 view, const Wpt *w, s32 w_ang, u32 w_len){
	s32 t;
	const NaviInfo* ni = NAVI_INFO[view];
	for(; ni->id != NAVI_END ; ++ni){
		switch(ni->id){
		case NAVI_STATUS:	// 測位状態
			DispStatus(ni->x, ni->y, OBJ_STATUS);
			break;
		case NAVI_V_STATUS:	// 測位状態
			DispStatus(ni->x, ni->y, OBJ_V_STATUS);
			break;

		case NAVI_CIRCLE: // 外枠
			DispCircle(ni->x, ni->y);
			ChangeNaviBase(IW->mp.sect_type);
			break;

		case NAVI_DIRECTION: // 磁石表示
			SetNaviObj(OBJ_NW_BASE, OBJ_TN_BASE, OBJSIZE_64x64 | A3_ROTATE_D(OBJ_RN_BASE), ni->x - 64, ni->y - 64);
			RotateBaseObj(256);
			RotateStarObj(-IW->px.h_ang_c, ni->x, ni->y);
			break;

		case NAVI_STARONLY: // 星のみ
			SetNaviObj(OBJ_NW_BASE, OBJ_TN_BASE, OBJSIZE_64x64 | A3_ROTATE_D(OBJ_RN_BASE), ni->x - 64, ni->y - 64);
			RotateMoveOut(OBJ_RN_BASE);
			RotateStarObj(-IW->px.h_ang_c, ni->x, ni->y);
			break;

		case NAVI_BSPEED: // 対地速度
			t = (IW->px.vh_mm > IW->tc.stop_dir)? IW->px.vh_mm : 0;
			if(IW->tc.spd_unit)	t = RoundDiv(t * 36, 10);
			if(t < 10) t = 0;
			DispBnObj(BN_SPEED, ni->x, ni->y, t, !t);
			break;

		case NAVI_BSPEED_UNIT1X:	// 対地速度単位1(拡大) メニュー時消去
			if(IsMenu()){
				PutsSpace2(ni->x, ni->y,     2);
				PutsSpace2(ni->x, ni->y + 2, 2);
				break;
			}
			// no break;
		case NAVI_BSPEED_UNIT1: // 対地速度単位1
			if(IW->tc.spd_unit){
				DrawText(ni->x, ni->y,     "km");
				DrawText(ni->x, ni->y + 2, "/h");
			} else {
				DrawText(ni->x, ni->y,     " m");
				DrawText(ni->x, ni->y + 2, "/s");
			}
			break;

		case NAVI_BANEMO: // 大気速度
			CalcAnemometer();
			t = IW->anemo.vel;
			if(IW->tc.anemo_unit) t = RoundDiv(t * 36, 10);
			if(t < 10) t = 0;
			DispBnObj(BN_ANEMO, ni->x, ni->y, t, !t);
			break;

		case NAVI_BANEMO_UNIT1: // 大気速度単位1
			if(IW->tc.anemo_unit){
				DrawText(ni->x, ni->y,     "km");
				DrawText(ni->x, ni->y + 2, "/h");
			} else {
				DrawText(ni->x, ni->y,     " m");
				DrawText(ni->x, ni->y + 2, "/s");
			}
			break;
		case NAVI_BANEMO_UNIT2: // 大気速度単位2
			DrawText(ni->x, ni->y,     "Air");
			break;
		case NAVI_ANEMO_DIF: // 大気速度単位2
			CalcAnemometer();
			t = RoundDiv(IW->px.vh_mm - IW->anemo.vel, 100);
			Putsf("%M%+3.1fm/s", ni->x, ni->y, Range32(-999, 999, t));
			break;

		case NAVI_ALT:// 高度
			DrawText(ni->x, ni->y, "Alt:");
			PutsKVal(IW->px.alt_mm);
			Puts("m");
			break;

		case NAVI_BALT:// 高度
			if((t = IW->px.alt_mm) < 1) t = 0;
			DispBnObj(BN_ALT, ni->x, ni->y, t, !t);
			break;
		case NAVI_BALT_UNIT1:// 高度
			DrawText(ni->x, ni->y, "Alt        m");
			break;

		case NAVI_LAT:// 緯度
			if(IW->px.lat != INVALID_VAL){
				DispBnObjLatLon(BN_LAT, ni->x, ni->y, IW->px.lat);
			}
			break;
		case NAVI_LAT_UNIT:// 緯度単位
			if(IW->px.lat != INVALID_VAL){
				DrawText(ni->x, ni->y, (IW->px.lat < 0)? "S" : "N");
			}
			break;
		case NAVI_LON:// 経度
			if(IW->px.lat != INVALID_VAL){
				DispBnObjLatLon(BN_LON, ni->x, ni->y, IW->px.lon);
			}
			break;
		case NAVI_LON_UNIT:// 経度単位
			if(IW->px.lat != INVALID_VAL){
				DrawText(ni->x, ni->y, (IW->px.lon < 0)? "W" : "E");
			}
			break;
		case NAVI_LATLON_UNIT:// 緯度単位
			if(IW->px.lat != INVALID_VAL){
				DrawText(ni->x, ni->y, "°    '    \"");
			}
			break;
		case NAVI_BTIME:// 時刻
			DispBnObjTime(BN_TIME, ni->x, ni->y, IW->px.dtime);
			break;
		case NAVI_BTIME_UNIT:// 時刻
			Putsf("%M^    ^%r^    ^", ni->x, ni->y);
			break;
		case NAVI_GPS_WARN:// GPS警告
			// テキストモード(のSP_VIEW)表示時は、ポップアップ警告で緯度経度が隠れないように画面内に警告表示する
			Locate(ni->x, ni->y);
			switch(IW->px.fix){
			case FIX_UNUSABLE: Puts("!! GPS Timeout !! ");	break;
			case FIX_INVALID:  Puts("! Satellite Lost !");	break;
			default: PutsSpace(18);
			}
			break;

		case NAVI_LD:// L/D
			Locate(ni->x, ni->y);
			if(IW->px.ldK > 0){
				Puts("L/D:");
				PutsLdK(IW->px.ldK);
				break;
			}
			// No break; // バリオを表示

		case NAVI_VARIO:// バリオ
			Putsf("%MVar:%+2.1f", ni->x, ni->y, Range32(-99, 99, RoundDiv(IW->px.up_mm, 100)));
			break;

		case NAVI_VARIO_GRAPH:// バリオ
			PutVarioGraph(ni->x, ni->y);
			break;

		case NAVI_TIME:	// 現在時刻
			Locate(ni->x, ni->y);
			if(IsMenu()){
				PutsSpace(8);
			} else {
				PutsCurTime();
			}
			break;

		case NAVI_V_DISP: // 垂直表示
			if(IW->tc.view_mode & 1) DispVBoxTate(ni->x, ni->y);
			else					 DispVBox    (ni->x, ni->y);
			break;

		case NAVI_DEBUG1:// デバッグ
			Putsf("%M0x%08x(%c)%r" "Pitching: %4d%r" "Rolling : %4d%r" "Rotation:  ",
				ni->x, ni->y,
				IW->px.gstate, STATE_STR[IW->px.gstate & GSTATE_TURN_MASK],
				(IW->px.gstate & GSTATE_PITCH_MASK) >> 20,
				(IW->px.gstate & GSTATE_ROLL_MASK)  >>  8);
			if(IW->px.gstate & GSTATE_CENTERING)   Puts("On ");
			else if(IW->px.gstate & GSTATE_SPIRAL) Puts("-S-");
			else                                   Puts("Off");

			Putsf("%M" "G:%12d%r" "vh_mm: %7d%r" "up_mm: %7d",
				ni->x, ni->y + 8,
				IW->px.g_mm, IW->px.vh_mm, IW->px.up_mm);
			break;

		case NAVI_DEBUG2:// デバッグ
			Putsf("%M" "Fix:%10d",
				ni->x, ni->y,
				IW->px.fix);
			Locate(ni->x, ni->y + 2);
			if(IW->px.lat != INVALID_VAL) PutsLat(IW->px.lat);
			else                          Puts("Latitude");
			Locate(ni->x, ni->y + 4);
			if(IW->px.lat != INVALID_VAL) PutsLon(IW->px.lon);
			else                          Puts("Longitude");

			Putsf("%M" "Alt_mm:%7d%r" "EPE_mm:%7d%r" "EPH_mm:%7d%r" "EPV_mm:%7d",
				ni->x, ni->y + 6,
				IW->px.alt_mm, IW->px.epe_mm, IW->px.eph_mm, IW->px.epv_mm);
			
			break;

		case NAVI_CYL_MAP:
			if(!w) UpdateCylView(0, -1, 0, 0, 0, 0, 0, 0);
			break;

		case NAVI_SP_LOCUS: // 軌跡モード
			if(IW->mp.pre_palette != IW->tc.locus_pal){
				IW->mp.pre_palette = IW->tc.locus_pal;
				UpdateLocusPalette(IW->mp.pre_palette);
			}
			UpdateLocusView(w, w_ang, w_len);
			break;
		}
	}
}

// パイロン情報を計算する
void UpdatePylonInfo(u32 view, const Route* rt, s32 len, s32 ang64, const Wpt* w0, const Wpt* w1, const Wpt* wpre){
	s32 t, alt = 0, nextAng = 0;
	s32 p2Len, p2Ang64, p2LenX = -1;
	if(IW->px.lat != INVALID_LAT){
		alt = RoundDiv(IW->px.alt_mm, 1000);
		nextAng = ang64 - IW->px.h_ang64;

		// 音声共用のデータをセット
		IW->mp.py_dis = len;
		IW->mp.py_dir = ((nextAng & 0xffff) * 360) >> 16;
		if(IW->mp.py_dir >= 180) IW->mp.py_dir -= 360; // -180～+180°

		if(w1){
			CalcDist(IW->px.lat, IW->px.lon, w1->lat, w1->lon, &p2Len, &p2Ang64);
			p2LenX = GetGoalDistance(IW->mp.cur_point);
			if(p2LenX != -1){
				// 合計距離が計算できた後は次パイロン取得後の距離を使用
				p2LenX = p2LenX - GetGoalDistance(IW->mp.cur_point + 1) + len;
			}
			IW->mp.py2_dis = p2LenX;
			IW->mp.py2_dir = (((p2Ang64 - IW->px.h_ang64) & 0xffff) * 360) >> 16;
			if(IW->mp.py2_dir >= 180) IW->mp.py2_dir -= 360; // -180～+180°
		}
	} else {
		IW->mp.py_dis = 0; // 音声用データを初期化する
	}

	if(IW->mp.sp_view){
		if(IW->mp.sp_view == SP_VIEW_CYL) view = (view & 0x3) + VIEW_MODE_CYLINDER;
		else return; // 専用モードでは表示しない
	}


	const NaviInfo* ni = NAVI_INFO[view];
	for(; ni->id != NAVI_END ; ++ni){
		switch(ni->id){
		case NAVI_PYLON_DIR: // パイロン方向
			if(IW->px.lat != INVALID_LAT){
				SetNaviObj(OBJ_NW_PYLN, OBJ_TN_PYLN, OBJSIZE_64x64 | A3_ROTATE_D(OBJ_RN_PYLN), ni->x - 64, ni->y - 64);
				RotateNaviWin(OBJ_RN_PYLN, nextAng, 512, ArriveScale(len, alt, w0->alt, 0));

				// 自機位置目安(セクタ位置確認用)
				MoveOut(OBJ_MY_POS); // 一旦非表示にする(チラツキも可)
				if(!IsMenu() && len < (1 << 24) &&
					(t = IW->tc.self_r? IW->tc.self_r : IW->tc.sector) != 0 &&
					(t = BiosDiv(len * 64, t, &t)) < 200){
					s32 a2 = nextAng + 0x8000;
					if(IW->tc.view_mode & 1) a2 += 0x4000;
					s32 x = ni->x + ((Sin64K(a2) * t) >> 15);
					s32 y = ni->y - ((Cos64K(a2) * t) >> 15);
					if(x > -16 && y > -16 && x < 256 && y < 172){
						if(IW->tc.view_mode & 1) SetNaviObj(OBJ_MY_POS, OBJ_TN_SELF_R, OBJSIZE_16x8, x - 8, y - 4);
						else                     SetNaviObj(OBJ_MY_POS, OBJ_TN_SELF,   OBJSIZE_8x16, x - 4, y - 8);
					}
				}
			}
			if(IW->tc.view_mode < MAX_VIEW_MODE_NONDEBUG) RotateBaseObj(256);
			break;

		case NAVI_BPYLON_LEN: // パイロン距離
			if(IW->px.lat != INVALID_LAT) DispBnObj(BN_LENGTH, ni->x, ni->y, len, len < 10000);
			break;
		case NAVI_BPYLON2_LEN: // パイロン距離
			if(IW->px.lat != INVALID_LAT && w1 && p2LenX != -1) DispBnObj(BN_LENGTH2, ni->x, ni->y, p2LenX, p2LenX < 10000);
			break;

		case NAVI_BPYLON_UNITX:	// パイロン距離単位(拡大) メニュー時消去
			if(IsMenu()){
				PutsSpace2(ni->x, ni->y, 2);
				break;
			}
			// no break;
		case NAVI_BPYLON_UNIT: // パイロン距離単位
			if(IW->px.lat != INVALID_LAT){
				Locate(ni->x, ni->y);
				Puts((len < 10000)? "m " : "km");
			}
			break;
		case NAVI_BPYLON2_UNIT: // パイロン距離単位
			if(IW->px.lat != INVALID_LAT && w1){
				Locate(ni->x, ni->y);
				Puts((p2LenX < 10000)? "m " : "km");
			}
			break;

		case NAVI_PYLON_DIFF: // パイロン高度差
			if(IW->px.lat != INVALID_LAT){
				Locate(ni->x, ni->y);
				PutAtAlt(alt, w0, len, IW->tc.view_alt);
			}
			break;

		case NAVI_PYLON2_DIFF: // パイロン高度差
			if(IW->px.lat != INVALID_LAT && w1){
				Locate(ni->x, ni->y);
				if(IW->tc.view_alt == 3 || IW->tc.view_alt == 5){
					// 同じゴール情報を表示させてもしょうがないので交換表示する
					PutAtAlt(alt, w0, len, IW->tc.view_alt ^ 6); // あくまで次パイロンからの合計で計算
				} else {
					PutAtAlt(alt, w1, p2LenX, IW->tc.view_alt);
				}
			}
			break;

		case NAVI_LD2:// L/D
			if(IW->px.lat != INVALID_LAT){
				Locate(ni->x, ni->y);
				if(!w0){
					PutsSpace(8);
					break;
				} else if(IW->tc.view_alt < 4 || !IsFinalApr()){
					if(PutsNLD(w0, len, IW->tc.view_alt != 5)) break;
				}
				PutAtAlt(alt, w0, len, 2); // 重複を避けて高度差を表示
			}
			break;

		case NAVI_GOAL_LEN:
			if(IW->px.lat != INVALID_LAT && (t = GetGoalDistance(IW->mp.cur_point)) != -1 && t){
				DrawText(ni->x, ni->y, IW->tc.task_mode? "G:" : "L:");
				PutsDistance2(t + len);
			} else {
				PutsSpace2(ni->x, ni->y, 6);
			}
			break;
		case NAVI_GOAL_INFO: // ゴール情報
			if(IW->px.lat != INVALID_LAT && (t = GetGoalDistance(IW->mp.cur_point)) != -1){
				if(IW->tc.task_mode){
					DrawText(ni->x, ni->y, "G:");
					PutsDistance2(t + len);
//					Putc(' ');
					PutAtAlt(alt, 0, 0, 0x80000003);// ゴール専用
				} else {
					DrawText(ni->x, ni->y, "LD:");
					PutAtAlt(alt, 0, 0, 0x80000003);// ゴール専用
					PutsSpace(4);
				}
			} else {
				PutsSpace2(ni->x, ni->y, 13);
			}
			break;

		case NAVI_PYLON_DIR_NUM: // パイロン方向(数字表示)
		case NAVI_PYLON_DIR_N2:	// パイロン方向(右詰数字)
			if(IsMenu()){
				PutsSpace2(ni->x, ni->y, 4);
				break;
			}
			// no break;
		case NAVI_PYLON_DIR_N2X:	// パイロン方向(メニュー無視)
			Locate(ni->x, ni->y);
			PutPylonDir(IW->mp.py_dir, ni->id == NAVI_PYLON_DIR_N2);
			break;

		case NAVI_PYLON2_DIR_NUM: // パイロン方向(数字表示)
		case NAVI_PYLON2_DIR_N2:	// パイロン方向(右詰数字)
			if(w1){
				Locate(ni->x, ni->y);
				PutPylonDir(IW->mp.py2_dir, ni->id == NAVI_PYLON2_DIR_N2);
			}
			break;

		case NAVI_P2_DIR:	// 次パイロン方向表示
			if(IW->px.lat != INVALID_LAT){
				if(w1){
					// 次パイロン位置 計算
					s32 xa = p2Ang64 - IW->px.h_ang64;
					s32 xl = 128 + ArriveScale(p2LenX, alt, w1->alt, 1);

					// オブジェ描画
					RotateNaviWin(OBJ_RN_NEXT, xa, 512, 512);//これは内部的に縦横変換する
					if(IW->tc.view_mode & 1) xa += 0x4000;
					SetNaviObj(OBJ_NEXT_PYLON, OBJ_TN_NEXT, OBJSIZE_32x32 | A3_ROTATE_N(OBJ_RN_NEXT),
						ni->x - 16 + ((Sin64K(xa) * xl) >> 18),
						ni->y - 16 - ((Cos64K(xa) * xl) >> 18));
				} else {
					RotateMoveOut(OBJ_RN_NEXT);
				}
			}
			break;

		case NAVI_WPTNAME_INI: // 次パイロン名(強制イニシャル)
			if(rt){
				Putsf("%M%d/%d:", ni->x, ni->y, IW->mp.cur_point + 1, rt->count);
				PutsPylonInitial(rt);
			} else {
				Locate(ni->x, ni->y);
				// 最寄WPT
				if(GetNearTarget()) Puts("Goto \\");
				else                Puts("LD \\\\\\");
				PutsNameB(w0->name);
			}
			break;
		case NAVI_WPTNAME1: // 次パイロン名(強制フルネーム)
			if(w0){
				Putsf("%M%d:", ni->x, ni->y, IW->mp.cur_point + 1);
				PutsNameB(w0->name); // フルネーム
			}
			break;
		case NAVI_WPTNAME2: // 次次パイロン名(強制フルネーム)
			if(w1){
				Putsf("%M%d:", ni->x, ni->y, IW->mp.cur_point + 2);
				PutsNameB(w1->name); // フルネーム
			}
			break;

		case NAVI_WPTNAMEX: // 次パイロン名 (自動選択)
			if(IsMenu()){
				PutsSpace2(ni->x, ni->y, 16);
				break;
			}
			// no break;
		case NAVI_WPTNAME: // 次パイロン名 (自動選択)
			Locate(ni->x, ni->y);
			if(rt){
				Putsf("%d/%d:", IW->mp.cur_point + 1, rt->count);

				t = IW->tc.pylon_type;
				if(t == 2 && (IW->vb_counter & 0x100)) t = 0; // 約4秒間隔でブリンク
				if(t) PutsPylonInitial(rt);	// イニシャル
				else  PutsNameB(w0->name);	// フルネーム
				Putc(' ');
			} else {
				// 最寄WPT
				if(GetNearTarget()) Puts("Goto \\");
				else                Puts("<LD> \\");
				PutsNameB(w0->name);
			}
			break;

		case NAVI_WPTNUM: // 次パイロン番号
			if(rt){
				DrawText(ni->x, ni->y - 2, (rt->count > 9)? "__" : " _");
				Putsf("%M%2d", ni->x, ni->y - 3, IW->mp.cur_point + 1);
				Putsf("%M%2d", ni->x, ni->y,     rt->count);
			} else {
				Locate(ni->x - 2, ni->y);
				if(IsMenu()){
					PutsSpace(4);
					break;
				}
				Putsf("#%03d", IW->mp.nw_target);
			}
			break;

		case NAVI_WPTNUM_V: // 次パイロン番号(縦)
			Locate(ni->x, ni->y);
			if(rt){
				Putsf("%d/%d", IW->mp.cur_point + 1, rt->count);
				PutsSpace(2);
			} else {
				// 最寄WPT
				Putsf("#%03d", IW->mp.nw_target);
			}
			break;

		case NAVI_ROUTENAME: // ルート名
			Locate(ni->x, ni->y);
			if(rt){
				PutsNameB(rt->name);
			} else {
				PutsSpace(14);
			}
			break;

		case NAVI_A_TIME: // 予想到着時刻
			if(IW->px.lat != INVALID_LAT){
				Locate(ni->x, ni->y);
				PutATime(len);
			}
			break;

		case NAVI_V_PYLN:	// 垂直表示
			if(IW->px.lat != INVALID_LAT) DispVPyln(ni->x, ni->y, alt, len, nextAng, w0, 0);
			break;

		case NAVI_V_PYLN2:	// 垂直表示
			if(IW->px.lat != INVALID_LAT && w1){
				DispVPyln(ni->x, ni->y, alt, p2Len, p2Ang64 - IW->px.h_ang64, w1, 1);
			}
			break;

		case NAVI_CYL_MAP:
			UpdateCylView(rt, len, nextAng, p2Len, p2Ang64, wpre, w0, w1);
			break;
		}
	}
}

void UpdateGoalInfo(u32 view){
	IW->mp.py_dis = 0; // 音声用データを初期化する
	if(IW->mp.sp_view){
		if(IW->mp.sp_view == SP_VIEW_CYL) view = (view & 0x3) + VIEW_MODE_CYLINDER;
		else return; // 専用モードでは表示しない
	}

	const NaviInfo* ni = NAVI_INFO[view];
	for(; ni->id != NAVI_END ; ++ni){
		switch(ni->id){
		case NAVI_GOAL:	// ゴール表示
			if(IW->tc.task_mode){
				DrawText(ni->x, ni->y, GetTaskRoute()? " **** GOAL **** " : " ## NO ROUTE ## ");
			} else {
				DrawText(ni->x, ni->y, " < FreeFlight > ");
			}
			break;
		}
	}
	IW->mp.sect_ang64 = NO_SECTOR;
	IW->mp.sect_type = 0;
	ChangeNaviBase(0);
}

///////////////////////////////////////////////////////////////////////////////
// ナビタスク
///////////////////////////////////////////////////////////////////////////////
void CalcSectAngle(const Route* rt, const Wpt* w0, const Wpt* w1){
	if(IW->mp.cur_point > IW->tc.pre_pylon && (!w1 || IW->tc.sector)){ // セクタ角度計算
		Wpt* wp = GetCurWpt(rt, -1);
		if(w1){
			// セクタ角計算
			u32 ang64w1, ang64w2;
			CalcDist(wp->lat, wp->lon, w0->lat, w0->lon, 0, &ang64w1);
			CalcDist(w1->lat, w1->lon, w0->lat, w0->lon, 0, &ang64w2);
			IW->mp.sect_ang64 = (ang64w1 + ang64w2) / 2 + 0x2000;
			if(MaxS32(ang64w1, ang64w2) - MinS32(ang64w1, ang64w2) > 0x8000) IW->mp.sect_ang64 += 0x8000;
			IW->mp.sect_type = TYPE_SECT;
		} else {
			// ゴールセクタ計算
			CalcDist(wp->lat, wp->lon, w0->lat, w0->lon, 0, &IW->mp.sect_ang64);
			if(IW->tc.goal_type == 3){// 通常セクタ
				IW->mp.sect_type = TYPE_SECT;
				IW->mp.sect_ang64 = (IW->mp.sect_ang64 + 0x2000) & 0xffff;
			} else {
				IW->mp.sect_type = TYPE_GOAL;
			}
		}
	} else {
		// スタートはセクタなし
		IW->mp.sect_ang64 = NO_SECTOR;
		IW->mp.sect_type = 0;
	}
}

void UpdateSpView(const Wpt *w, s32 len, s32 ang){
	s32 view = IW->tc.view_mode;
	switch(IW->mp.sp_view){
	case SP_VIEW_WINDCHECK:	UpdateWindCheckView();						return;
	case SP_VIEW_BENCHMARK:	UpdateBenchmarkView();						return;
	case SP_VIEW_CYL:		view = (view & 0x3) + VIEW_MODE_CYLINDER;	break;
	case SP_VIEW_LOCUS:		view = (view & 0x3) + VIEW_MODE_LOCUS;		break;
	case SP_VIEW_TEXT:		view = (view & 0x3) + VIEW_MODE_TEXT;		break;
	}
	UpdateGpsInfo(view, w, ang, len);
}

void TaskGoal(const Route* rt){
	UpdateSpView(0, 0, 0);// GPS情報表示
	InitGoalMenu(rt);
	IW->mp.nw_target = -1;
	UpdateGoalInfo(IW->tc.view_mode);
	IW->mp.navi_update = 0;
	return;
}

#define ALARM_NUM 5
const u16* const ALARM_SOUND[ALARM_NUM] = {
	SG1_CLEAR, SG1_ALARM1, SG1_ALARM2, SG1_ALARM3, SG1_ALARM4
};
#define ALARM_INTERVAL 40 // 666ms間隔
#define NEAR_INTERVAL  30 // 500ms間隔

void CheckAlarm(){
	// スタートアラーム優先
	if(IW->tc.start_alarm && IW->px.sec < 3 && IW->px.fix >= FIX_INVALID){ // 40/60sec間隔で合計5回。Invalidでも時計だけはOK?
		u32 t = ALARM_NUM; // 無効
		switch(IW->tc.start_time - (IW->px.hour * 60 + IW->px.min)){
		case 0:				t = 0;	break; //start
		case 1:	case -1439: t = 1;	break; // 1min.
		case 5:	case -1435: t = 2;	break; // 5min.
		case 10:case -1430: t = 3;	break; //10min.
		case 30:case -1410: t = 4;	break; //30min.
		}
		if(t < ALARM_NUM && IW->vb_counter - IW->mp.near_vbc > ALARM_INTERVAL){
			IW->mp.near_vbc = IW->vb_counter;
			PlaySG1(ALARM_SOUND[t]);
			return;
		}
	}

	// ニアサウンドチェック
	if(IW->mp.near_pylon && IW->mp.near_pylon < IW->tc.near_beep && IW->px.fix >= FIX_2D){
		// ニアパイロン状態
		if(IW->vb_counter - IW->mp.near_vbc > NEAR_INTERVAL){
			IW->mp.near_vbc = IW->vb_counter;
			PlaySG1X(SG1_NEAR); // 32カウンタ(約0.5秒)
			return;
		}
	}
}

s32 MT_Navi(){
	SelectMap(MAP_BG2);

	if(IW->mp.test_mode){
		const u8 TEST_BOOST[4] = {0, 60, 12, 0};
		if(++IW->px_rfu[1] > TEST_BOOST[IW->mp.test_mode]){
			IW->px_rfu[1] = 0;
			DummyPlay();
			DummyPlayUpdate();
		}
	}

	if(IW->mp.pre_task != !IW->tc.task_mode){// 0 or 非0
		IW->mp.navi_update |= NAVI_UPDATE_ROUTE;
		IW->mp.pre_task = !IW->tc.task_mode;
	}

	// 表示モードの切替チェック
	if(IW->mp.cur_view != IW->tc.view_mode || 
		IW->mp.sp_view != IW->mp.pre_sp_view ||
		IW->mp.pre_sect!= IW->tc.sector){
		IW->mp.cur_view    = IW->tc.view_mode;
		IW->mp.pre_sp_view = IW->mp.sp_view;
		IW->mp.pre_sect    = IW->tc.sector;
		SPLITE->flag = 0; // 特殊スプライトモード解除
		Cls();
		SetBG2Mode();
		IW->mp.navi_update |= NAVI_CHECK_WPT;
		//オブジェを1回消す
		ClearNaviObj();
	}

	// メニュー表示切替時は表示を更新
	if(IW->mp.pre_menu != IsMenu()){
		IW->mp.pre_menu = IsMenu();
		IW->mp.navi_update |= NAVI_UPDATE_PVT;
	}

	// ここでアラームチェック
	CheckAlarm();

	// ゴールまでの総距離計算は合間にやる
	if(CalcGoalDistance(IW->mp.navi_update & (NAVI_CHECK_ROUTE | NAVI_ROUTE_DL)) && !IW->mp.navi_update) return 0; 

	// ニアパイロンを初期化しておく
	IW->mp.near_pylon = -1;
	IW->mp.cyl_dist = -1; // 最大値

	// ダウンロードで現在のルートが更新された?
	if((IW->mp.navi_update & NAVI_ROUTE_DL) && IW->gi.state > GPS_STOP_DOWNLOAD) IW->mp.navi_update = NAVI_UPDATE_ROUTE;

	// フリーモード?
	const Wpt *w0, *w1;
	s32 len, ang64;
	if(!IW->tc.task_mode){
		IW->mp.sect_ang64 = NO_SECTOR;
		IW->mp.sect_type = 0;
		IW->mp.cur_point = 0;
		w0 = GetNearTarget();
		if(w0){
			w1 = GetDefaultLD();
			if(w0 == w1) w1 = 0;
		} else {
			w0 = GetDefaultLD();
			w1 = 0;
		}
		if(w0){
			// 最寄WPTの表示
			CalcDist(IW->px.lat, IW->px.lon, w0->lat, w0->lon, &len, &ang64);
			IW->mp.sect_ang64 = NO_SECTOR;
			UpdateSpView(w0, len, ang64);// GPS情報表示
			UpdatePylonInfo(IW->tc.view_mode, 0, len, ang64, w0, w1, 0);
			IW->mp.cyl_dist = MaxS32(len - IW->tc.cylinder, 1);

			// 自動ターゲットモード (メニュー表示中や次ターゲット検索中はターゲットゲットをガード)
			u32 MP_SearchNearWpt(u16 push);
			if(GetNearTarget() && IW->tc.at_mode && !IsMenu() && IW->mp.proc != MP_SearchNearWpt){
				IW->mp.near_pylon = IW->mp.cyl_dist;

				// タイプ2のローリング検出
				if(IW->tc.at_mode == 3){
					// ローリングを検出したら、一度ローリングを止めるまで次検出しない
					u32 detect = ((IW->px.gstate & GSTATE_ROLL_MASK)  >>  8) >= 3;
					if( IW->mp.at_rollflag && !detect) IW->mp.at_rollflag = 0;
					if(!IW->mp.at_rollflag &&  detect) IW->mp.at_rollflag = 1;
				}
				if(len < IW->tc.cylinder ||		// ターゲットゲット
					IW->mp.at_rollflag == 1 ||	// ローリングコマンド
					(IW->px.fix >= FIX_2D && IW->tc.at_recheck && IW->tc.at_recheck < len && IW->vb_counter - IW->mp.at_vbc > VBC_TIMEOUT(60))){ // 圏外再検索
					IW->mp.nw_s_dir =  2; // オートターゲットモード開始
					if(len < IW->tc.cylinder){ // ターゲットを取ったときだけログ追加
						PlaySG1(SG1_PYLON);
						UpdateAutoTargetLog();
					} else {
						PlaySG1(SG1_OK);
						if(IW->mp.at_rollflag != 1) IW->mp.nw_s_dir  = 3; // 自動再サーチのときは、自身をターゲットに含める特殊モード
					}
					IW->mp.nw_tgt_len = IW->tc.at_min + IW->tc.cylinder;
					IW->mp.atcand_count = 0;
					SetNaviStartPos(); // 検索開始場所のセット

					// コールバックを変える
					IW->mp.proc = MP_SearchNearWpt;
					IW->mp.at_rollflag = 2; // 連続検出ガード
				}
			}
		} else {
			UpdateSpView(0, 0, 0);// GPS情報表示
			UpdateGoalInfo(IW->tc.view_mode);
		}
		IW->mp.navi_update = 0;
		return 0;
	}

	// ルート変更フラグON?
	if(IW->mp.navi_update & NAVI_CHECK_ROUTE){
		IW->mp.cur_point = 0;
		IW->mp.sect_ang64 = NO_SECTOR;
	}

	// ルート有効チェック
	const Route* rt = GetTaskRoute();
	if(!rt){
		UpdateSpView(0, 0, 0);// GPS情報表示
		InitGoalMenu(rt);
		UpdateGoalInfo(IW->tc.view_mode);
		IW->mp.navi_update = 0;
		return 0;
	}

	// パイロン判定
	u32 get_pylon = 0;
	for(;;) {
		// ゴールチェック
		if(IW->mp.cur_point >= rt->count){
			TaskGoal(rt);
			return 0;
		}

		w0 = GetCurWpt(rt, 0);
		w1 = GetCurWpt(rt, 1);
		// ウェイポイント変更フラグONならセクタ角を再計算
		if(IW->mp.navi_update & NAVI_CHECK_WPT) CalcSectAngle(rt, w0, w1);

		// パイロンゲット?
		CalcDist(IW->px.lat, IW->px.lon, w0->lat, w0->lon, &len, &ang64);

		// 最初のパイロンがテイクオフでスキップ対象の場合は無条件で先に進む
		IW->mp.near_pylon = CheckPylon(rt, len, ang64);
		if(IW->tc.skip_TO && IW->mp.cur_point == 0){
			if(!IW->px.dtime) break;
			PlaySG1(SG1_COMP1);
		} else {
			if(IW->mp.near_pylon) break; // 0でパイロンゲット
			PlaySG1(SG1_PYLON);
		}

		// ログクリアした場合は、何かパイロンをとった時にTakeOffを更新
		if(IW->tc.skip_TO && !IW->task_log.tl1[1].dtime){
			UpdateTaskLog(1);
		}

		// 次のWPTへ進む
		IW->mp.navi_update |= NAVI_UPDATE_WPT;
		UpdateTaskLog(++IW->mp.cur_point);
		if(IW->mp.cur_point >= rt->count && IW->tc.tlog_as){
			TaskGoal(rt);
			SaveFLog(IW->mp.tlog_presave, &IW->task_log, sizeof(TaskLogData), 0); // ゴール時はここで保存
			return 0;
		} else if(IW->tc.tlog_as == 2){
			get_pylon = 1; // 連続取得を1回にまとめる
		}

		//オブジェを1回消す
		Cls();
		ClearNaviObj();
	}

	// パイロン情報の更新
	UpdateSpView(w0, len, ang64);
	UpdatePylonInfo(IW->tc.view_mode, rt, len, ang64, w0, w1, GetCurWpt(rt, -1));

	IW->mp.navi_update = 0;
	if(get_pylon) SaveFLog(IW->mp.tlog_presave, &IW->task_log, sizeof(TaskLogData), 0); // 未ゴール時は表示後に保存
	return 0;
}

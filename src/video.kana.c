///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// 画面制御をこのファイルにまとめた
// 画像データは、モジュールサイズ削減のため圧縮ファイルのcomp.cに移した


///////////////////////////////////////////////////////////////////////////////
// タイル
///////////////////////////////////////////////////////////////////////////////
// キャラクタをタイルIDに変換
void Ascii2Tile(u8 ch, u32* tile, u32* size){
	ch -= 0x20;
	if(ch < FONT_WIDE){
		*tile = ((u32)ch << 1);
		*size = 1;
	} else {
		*tile = ((u32)ch << 2) - 0xB6;
		*size = 2;
	}
}

///////////////////////////////////////////////////////////////////////////////
// オブジェ
///////////////////////////////////////////////////////////////////////////////
const u8 LOCUS_TILE_BASE[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x15,0x15,0x00,0x00,0x00,
	0x00,0x00,0x15,0x2a,0x2a,0x15,0x00,0x00,
	0x00,0x15,0x2a,0x3f,0x3f,0x2a,0x15,0x00,
	0x00,0x15,0x2a,0x3f,0x3f,0x2a,0x15,0x00,
	0x00,0x00,0x15,0x2a,0x2a,0x15,0x00,0x00,
	0x00,0x00,0x00,0x15,0x15,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

#define NUM_PALETTE 3
const u8 LOCUS_PALETTE[NUM_PALETTE][LOCUS_LEVEL * 3] = {
	{ // パレット1
		0x20, 0x30, 0x34, // -5 紺
		0x30, 0x34, 0x38, // -4
		0x30, 0x38, 0x3c, // -3 青
		0x30, 0x39, 0x3e, // -2
		0x34, 0x3d, 0x3e, // -1 水色
		0x11, 0x12, 0x13, // +1 赤
		0x02, 0x17, 0x1b, // +2
		0x07, 0x1b, 0x3f, // +3 黄色
		0x1b, 0x0f, 0x3f, // +4 
		0x2f, 0x3f, 0x3f, // +5 白
	}, { // パレット2
		0x30, 0x34, 0x38, // -5 青
		0x30, 0x38, 0x3c, // -4
		0x20, 0x3c, 0x3e, // -3 水
		0x20, 0x3d, 0x3f, // -2
		0x14, 0x3f, 0x3f, // -1 白
		0x05, 0x3f, 0x3f, // +1 白
		0x0a, 0x0f, 0x3f, // +2
		0x07, 0x1b, 0x2f, // +3 黄
		0x02, 0x07, 0x1f, // +4
		0x03, 0x03, 0x07, // +5 赤
	}, { // パレット3 レインボー
		0x30, 0x34, 0x38, // -5 青
		0x20, 0x3c, 0x3e, // -4
		0x14, 0x28, 0x3c, // -3 水
		0x1c, 0x2c, 0x3c, // -2
		0x04, 0x1c, 0x2c, // -1 緑
		0x04, 0x0d, 0x0e, // -1 緑
		0x0d, 0x0e, 0x0f, // -2
		0x0f, 0x1f, 0x1f, // +3 黄
		0x07, 0x1b, 0x1b, // +4
		0x03, 0x03, 0x07, // +5 赤
	}
};

// スプライト移動
void SetNormalObj(u32 id, u32 tile, u32 size, s32 x, s32 y){
	vu16* p = (u16*)(OAM + (id << 3));
	p[0] = (y & 0x00ff) | (size >> 16) | 0x2000;
	p[1] = (x & 0x01ff) | (size & 0xffff);
	p[2] = tile;
}
void MoveObj(u32 id, s32 x, s32 y){
	vu16* p = (u16*)(OAM + (id << 3));
	p[0] = (p[0] & ~0x00ff) | (y & 0x00ff);
	p[1] = (p[1] & ~0x01ff) | (x & 0x01ff);
}
void MoveOut(u32 id){
	vu16* p = (u16*)(OAM + (id << 3));
	p[0] = (p[0] & ~0x00ff) | 160; // 範囲外へ
	p[1] = (p[1] & ~0x01ff);
}

///////////////////////////////////////////////////////////////////////////////
// タイル作成処理
///////////////////////////////////////////////////////////////////////////////
// グレースケールフォント展開。RL圧縮の方が良い?
const u16 FONTPAL[4] = {0, 0x15, 0x2a, 0x3f};
u16* Create4xTile(u16* head, const u16* font, u32 size){
	while(size--){
		u32 y;
		for(y = 0 ; y < 8 ; ++y){
			u32 x;
			u16 v = *font++;
			for(x = 0 ; x < 8 ; x += 2){
				*head++ = (FONTPAL[v & 3]) | (FONTPAL[(v >> 2) & 3] << 8);
				v >>= 4;
			}
		}
	}
	return head;
}

// 縦画面用のオブジェ作成用
void ObjRotateBlock(u16* dst, u16* src){ // 90°回転
	u8* s2 = (u8*)src; // Readは1byte単位OK
	s32 x, y;
	for(x = 0 ; x < 8 ; ++x){
		for(y = 6 * 8 ; y >= 0 ; y -= 16){
			*dst++ = (s2[x + y] << 8) | (s2[x + y + 8] << 0);
		}
	}
}
//OBJ単位の回転(1D方式のみ対応)
void ObjRotateCopy(u16* dst, u16* src, u32 src_w, u32 src_h){
	u32 x, y;
	for(x = 0 ; x < src_w ; ++x){
		for(y = src_h ; y-- ;){
			ObjRotateBlock(dst, &src[(x + y * src_w) * 32]);
			dst += 32;
		}
	}
}

// テキストデータからのOBJ色変換コピー
u8 TransFontPal(u8 src, const u8* pal){
	switch(src){
	case 0x15:	return pal[0];
	case 0x2a:	return pal[1];
	case 0x3f:	return pal[2];
	}
	return 0;
}
void ObjColorCopy(u16* dst, u16* src, u32 size, const u8* pal){
	for(size *= 32 ; size-- ; ++dst, ++src){
		*dst = TransFontPal(*src & 0xff, pal) | (TransFontPal(*src >> 8, pal) << 8);
	}
}

const u16 PAL_DEF[5][4] = {
	{ 0,  8, 18, 30},
	{ 0, 16, 24, 31},
	{ 2, 20, 26, 31},
	{ 5, 24, 28, 31},
	{ 8, 27, 29, 31},
};

// タイルパレット設定
void InitPalette(u32 n){
	if(n > 4) n = 4;
	const u16* pal = PAL_DEF[n];
	// パレット初期化
	vu16* p = (u16*)BGpal;
	// 64色
	u32 r, g, b;
	for(b = 0 ; b < 4 ; ++b){
		for(g = 0 ; g < 4 ; ++g){
			for(r = 0 ; r < 4 ; ++r){
				p[0] = p[0x100] = (pal[b] << 10) | (pal[g] << 5) | pal[r];
				p++;
			}
		}
	}
	// 指定色
	p[0] = p[0x100] = (pal[0] << 10) | (pal[0] << 5) | (pal[0]);
	p[1] = p[0x101] = (4 << 10) | (4 << 5) | (4);
}

///////////////////////////////////////////////////////////////////////////////
// ビデオ初期化
///////////////////////////////////////////////////////////////////////////////

inline void memcpy16(u16* dst, const u16* src, s32 size){
	DmaCopy(3, src, dst, size, 16);
}

inline u8 GetPixelColor(s32 x, s32 y, const u16* src){
	return ((u8*)src)[(y << 3) + x];
}
// 1xY専用フチ付きコピー
const s8 EDGE_TABLE[] = {
	-1,  0,
	 1,  0,
	 0, -1,
	 0,  1,
	 9,  9,//END
};
u16 GetPixelColor2(s32 x, s32 y, s32 ey, const u16* src){
	u16 ret = GetPixelColor(x, y, src);
	if(!ret){
		const s8* p = EDGE_TABLE;
		while(*p != 9){
			s32 x2 = x + *p++;
			s32 y2 = y + *p++;
			if(x2 < 0 || 7 < x2 || y2 < 0 || ey <= y2) continue;
			if(GetPixelColor(x2, y2, src)){
				ret = 0x40; // 微妙に青?
				break;
			}
		}
	}
	return ret;
}
void memcpy16x(u16* dst, const u16* src, s32 size){
	s32 x, y, endy = size / 8;
	for(y = 0 ; y < endy ; y++){
		for(x = 0 ; x < 8 ; x += 2){
			*dst++ = (GetPixelColor2(x + 1, y, endy, src) << 8) | GetPixelColor2(x, y, endy, src);
		}
	}
}

void SetVideoMode(s32 t){
	if(t){
		// グラフ用
		REG16(REG_BG0CNT) = 2 | BG_COLOR_256 | (29 << 8);
		REG16(REG_BG1CNT) = 3 | BG_COLOR_256 | (30 << 8);
		REG16(REG_BG2CNT) = 0 | BG_COLOR_256 | (31 << 8) | 0x4000; // 設定上はナビ画面が最前面(αブレンドで暗くする)
	} else {
		REG16(REG_BG0CNT) = 1 | BG_COLOR_256 | (29 << 8);
		REG16(REG_BG1CNT) = 2 | BG_COLOR_256 | (30 << 8);
		REG16(REG_BG2CNT) = 0 | BG_COLOR_256 | (31 << 8) | 0x4000; // 設定上はナビ画面が最前面(αブレンドで暗くする)
	}
	if(t & 2){
		// αブレンド設定
		REG16(REG_BLDMOD) = 0x0344;
		REG16(REG_COLEV)  = 0x1010; // αブレンド低減
	} else {
		// αブレンド設定
		REG16(REG_BLDMOD) = 0x0344;
		REG16(REG_COLEV)  = 0x1004; // αブレンド低減
	}
}

void UpdateLocusPalette(u32 v){
	if(v >= NUM_PALETTE) v = 0;
	u16* p = OAMdata + OBJ_TN_LOCUS   * 16;
	const u8* pal = LOCUS_PALETTE[v];
	s32 i;
	for(i = 0 ; i < LOCUS_LEVEL ; ++i, p += 32, pal += 3){
		ObjColorCopy(p, (u16*)LOCUS_TILE_BASE, 1, pal);
	}
	ObjRotateCopy(OAMdata + OBJ_TN_UP_R   * 16, OAMdata + OBJ_TN_UP   * 16, 1, 1); // 縦画面用
	ObjRotateCopy(OAMdata + OBJ_TN_SELF_R * 16, OAMdata + OBJ_TN_SELF * 16, 1, 2); // 縦画面用
}

void InitVideo(){
	// 画面モード設定
	SetVideoMode(0);
	REG16(REG_DISPCNT) = DISP_MODE_1 | DISP_BG0_ON | DISP_BG1_ON;
	FillBox(3, 5, 26, 13);
	DrawTextCenter(7, "ParaNavi " NAVI_VERSION);
	DrawTextCenter(10, "Initializing..."); // 20文字以内

	// パレット初期化
	InitPalette(0); // デフォルト値で初期化

	// BG用フォント作成
#define COMP_WORK ((u16*)VOICE_ADDRESS) // ここはまだ未使用なので空いている
	BiosLZ77UnCompWram(FONT_COMP, COMP_WORK);
	u16* p = Create4xTile(VideoBuffer, COMP_WORK, FONT_WIDE * 2 + (0x100 - FONT_WIDE) * 4);

	// BG用 定義済タイル登録
	p += BiosLZ77UnCompVram(BG_TILE_COMP, p) >> 1;

	// 塗りつぶしタイル作成
	const u8 FILL_TILE[] = { 0xf0, 0xf0, 0x10, 0x40, 0x41, 0x01} ; // 塗りつぶし
	s32 i, j;
	for(i = 0 ; i < sizeof(FILL_TILE) ; ++i){
		memset(p, FILL_TILE[i], 8 * 8);
		p += 8 * 4;
	}
	// グラフタイル
	for(i = 0 ; i < 9 ; ++i){
		for(j = 0; j < i ; ++j){
			if(j){
				p[0] = 0x3f24;
				p[1] = 0x243f;
			} else {
				p[0] = 0x3f2d;
				p[1] = 0x2d3f;
			}
			p[2] = p[3] = 0;
			p += 4;
		}
		while(j++ < 8){
			p[0] = p[1] = (j == 1)? 0x0808 : 0x4141;//4040;
			p[2] = p[3] = 0;
			p += 4;
		}
	}

	// オブジェ用 定義済タイル登録
	BiosLZ77UnCompVram(OBJ_TILE_COMP, OAMdata);

	// 拡大数字タイル登録
	p = OAMdata + OBJ_TN_BIGNUM * 16;
	BiosLZ77UnCompWram(BIGNUM_COMP, COMP_WORK);
	u16* q = Create4xTile(p, COMP_WORK, BIGNUM_COUNT * BIGNUM_SIZE);

	// 縦画面用に回転データを作成
	for(i = 0 ; i < BIGNUM_COUNT ; ++i){
		ObjRotateCopy(q, p, 2, 4); // 2x4
		p += 32 * BIGNUM_SIZE;
		q += 32 * BIGNUM_SIZE;
	}

	// 数字（小）
	p = OAMdata + OBJ_TN_SMLNUM   * 16;
	q = OAMdata + OBJ_TN_SMLNUM_R * 16;
	for(i = 0 ; i < 13 ; ++i){
		const u8 SMALL_NUM[] = "0123456789-km";
		memcpy16x(p, VideoBuffer + (SMALL_NUM[i] - 0x20) * 64, 128);
		ObjRotateCopy(q, p, 1, 2); // 1x2
		p += 32 * 2;
		q += 32 * 2;
	}

	// バリオタイル登録
	p = OAMdata + OBJ_TN_VARIO   * 16;
	q = OAMdata + OBJ_TN_VARIO_L * 16;
	for(i = 0 ; i < VARIOOBJ_COUNT ; ++i){
		memcpy16(p,      q ,     8 * (i + 1));
		memcpy16(p + 32, q + 32, 8 * (i + 1));
		ObjRotateCopy(p + VARIOOBJ_SIZE * VARIOOBJ_COUNT * 32, p, 2, 1); // 2x1 縦画面用
		p += 32 * VARIOOBJ_SIZE;
	}
	ObjRotateCopy(OAMdata + OBJ_TN_VARIO_CR * 16, OAMdata + OBJ_TN_VARIO_C * 16, 4, 1);

	// NESW タイル登録
	const u8 T_PAL0[] = {0x14, 0x15, 0x2f};
	const u8 T_PAL1[] = {0x14, 0x15, 0x3e};
	const u8* tpal = T_PAL0;
	p = OAMdata + OBJ_TN_NESW   * 16;
	q = OAMdata + OBJ_TN_NESW_R * 16;
	for(i = 0 ; i < 4 ; ++i, p += 32 * NESWOBJ_SIZE, q += 32 * NESWOBJ_SIZE){
		ObjRotateCopy(q, p, 1, 2); // 2x1 縦画面用
		tpal = T_PAL1;
	}

	// 外枠
	p = OAMdata + OBJ_TN_CIRCLE   * 16;
	q = OAMdata + OBJ_TN_CIRCLE_R * 16;
	for(i = 0 ; i < 9 ; ++i, p += 32, q += 32) ObjRotateCopy(q, p, 1, 1);

	// 軌跡
	UpdateLocusPalette(0);

	// オブジェを外に移動
	for(i = 0 ; i < 128 ; ++i) MoveOut(i);

	// カーソル設定
	SetNormalObj(OBJ_MENU_CURSOR, 0, OBJSIZE_8x8, 0, 160);
	SetNormalObj(OBJ_MENU_KETA,   2, OBJSIZE_8x8, 0, 160);
	SetNormalObj(OBJ_MENU_UP,	  4, OBJSIZE_8x8, 0, 160);
	SetNormalObj(OBJ_MENU_DOWN,	  4, OBJSIZE_8x8 | A3_VFLIP, 0, 160);

	// 全BG表示
	REG16(REG_DISPCNT) = DISP_MODE_1 | DISP_BG0_ON | DISP_BG1_ON | DISP_BG2_ON | DISP_OBJ_ON | DISP_OBJ_CHAR_1D_MAP;
}

///////////////////////////////////////////////////////////////////////////////
// マップ
///////////////////////////////////////////////////////////////////////////////
#define BG_SIZE  0x800

u16* const BG_BASE[4] = {
	(u16*)(VRAM + BG_SIZE * 29),
	(u16*)(VRAM + BG_SIZE * 30),
	(u16*)(VRAM + BG_SIZE * 31),
};

void SelectMap(u32 n){
	IW->mp.map = n;
}

void SetChar_Base(u32 map, u32 x, u32 y, u16 tile){
	if(map != 2) {
		BG_BASE[map][x + 32 * y] = tile;
	} else {
		u16* p = &BG_BASE[2][x / 2 + 16 * y];
		if(x & 1) *p = (*p & 0x00ff) | (tile << 8);
		else      *p = (*p & 0xff00) | tile ;
	}
}
static inline void SetChar(u32 x, u32 y, u16 tile){
	SetChar_Base(IW->mp.map, x, y, tile);
}
void AddChar2(u16 tile){
	if(IW->mp.gx >= 30){
		IW->mp.gx = 0;
		IW->mp.gy += 2;
	}
	if(IW->mp.gy >= 30) return;
	SetChar(IW->mp.gx,   IW->mp.gy,     tile);
	SetChar(IW->mp.gx++, IW->mp.gy + 1, tile + 1);
}

// 画面消去
void ClsId(u32 id, u16 tile){
	u16* p = (u16*)BG_BASE[id];
	u32 i = 0;
	for(i = 0 ; i < BG_SIZE / 2 ; ++i) *p++ = tile;
}
void Cls(){
	ClsId(IW->mp.map, 0);
	IW->mp.gx = IW->mp.gy = 0;
}

// 1文字出力
void Putc(u8 ch){
	u32 tile, size;
	Ascii2Tile(ch, &tile, &size);
	AddChar2(tile);
	if(size > 1) AddChar2(tile + 2);
}
// ワイドcharの後半のみ出力
void Putc2nd(u8 ch){
	u32 tile, size;
	Ascii2Tile(ch, &tile, &size);
	if(size > 1) AddChar2(tile + 2);
}

void Locate(u32 x, u32 y){
	IW->mp.gx = x;
	IW->mp.gy = y;
}
void LocateX(u32 x){
	IW->mp.gx = x;
}

///////////////////////////////////////////////////////////////////////////////
// 罫線
///////////////////////////////////////////////////////////////////////////////
void FillTile(u32 map, u32 x0, u32 y0, u32 x1, u32 y1, u16 tile){
	for(; y0 <= y1 ; ++y0){
		int xx;
		for(xx = x0 ; xx <= x1 ; ++xx){
			SetChar_Base(map, xx, y0, tile);
		}
	}
}
void DrawBox_Base(u32 x0, u32 y0, u32 x1, u32 y1){
	FillTile(IW->mp.map, x0 + 1, y0, x1 - 1, y0, BD_H);
	FillTile(IW->mp.map, x0 + 1, y1, x1 - 1, y1, BD_H);
	FillTile(IW->mp.map, x0, y0 + 1, x0, y1 - 1, BD_V);
	FillTile(IW->mp.map, x1, y0 + 1, x1, y1 - 1, BD_V);
	SetChar_Base(IW->mp.map, x0, y0, BD_UL);
	SetChar_Base(IW->mp.map, x1, y0, BD_UR);
	SetChar_Base(IW->mp.map, x0, y1, BD_BL);
	SetChar_Base(IW->mp.map, x1, y1, BD_BR);
}

void DrawBox(u32 x0, u32 y0, u32 x1, u32 y1){
	IW->mp.menu_x0 = x0 + 1;
	IW->mp.menu_y0 = y0;
	IW->mp.menu_x1 = x1 - 1;
	IW->mp.menu_y1 = y1;

	FillTile(MAP_BG1, x0 + 1, y0 + 1, x1 - 1, y1 - 1, SELMENU_BG);
	DrawBox_Base(x0, y0, x1, y1);
}

void FillBox(u32 x0, u32 y0, u32 x1, u32 y1){
	DrawBox(x0, y0, x1, y1);
	FillTile(IW->mp.map, x0 + 1, y0 + 1, x1 - 1, y1 - 1, 0);
}

///////////////////////////////////////////////////////////////////////////////
// メニュー選択
///////////////////////////////////////////////////////////////////////////////
void MoveSelBg2(u32 y, u32 ix1, u32 ix2, u32 type1, u32 type2){
	u32 x0 = IW->mp.menu_x0;
	u32 y0 = IW->mp.menu_y0;
	u32 x1 = IW->mp.menu_x1;
	u32 y1 = IW->mp.menu_y1;
	y = y0 + y * 2;
	if(type1 != SELMENU_1) ++y;
	if(ix1 == ix2){
		FillTile(MAP_BG1, x0, y0, x1, y1, SELMENU_BG);
	} else {
		FillTile(MAP_BG1, x0,  y0, ix1, y1, SELMENU_BG);
		FillTile(MAP_BG1, ix1, y0, ix2, y1, type2);
		FillTile(MAP_BG1, ix2, y0, x1,  y1, SELMENU_BG);
	}
	FillTile(MAP_BG1, x0,  y,  x1,  y,  type1);
	y++;
	FillTile(MAP_BG1, x0,  y, x1,  y, type1 + 1);
}

///////////////////////////////////////////////////////////////////////////////
// テキスト出力
///////////////////////////////////////////////////////////////////////////////

const u8 V_HEX[] = "0123456789ABCDEF";

void Puts(const u8* str){
	while(*str) Putc(*str++);
}

void PutsValX(s32 v, s32 s, s32 keta, u8 pad, u32 f){
	// 無限大は特別処理
	if(v == INT_MAX || v == INT_MIN){
		while(--keta < 2) Putc(' ');
		Putc((v < 0)? '-' : '+'); // fは無視して必ず符号をつける
		Putc('!');
		return;
	}

	// 通常の数字を出力(MAX 29桁)
	u8 buf[30], *p = buf + sizeof(buf);
	if(s > 16 || keta >= sizeof(buf)) return;
	*--p = 0;
	if(v == 0){
		*--p = '0';
	} else {
		if(v < 0){
			v = -v;
			f |= 1; // '-'
		}
		while(v){
			s32 mod;
			v = BiosDiv(v, s, &mod);
			*--p = V_HEX[mod];
		}
	}

	if(pad != '0'){
		// 符号追加
		if(f & 1)      *--p = '-';
		else if(f & 2) *--p = '+';
	} else {
		if(f & 3){
			--keta;
			Putc((f & 1)? '-' : '+');
		}
	}

	//桁調整
	keta -= sizeof(buf) - (p - buf);
	while(keta-- >= 0) Putc(pad);
	Puts(p);
}

// 整数表示簡易マクロ
#define PutsVal(v, s)           PutsValX((v), (s), 0,   0,   0)		// 標準                ("123"等)
#define PutsVal0(v, s, k)       PutsValX((v), (s), (k), '0', 0)		// 不足桁を0で埋める。 ("00123"等)
#define PutsValB(v, s, k)       PutsValX((v), (s), (k), ' ', 0)		// 不足桁を空白で埋める("  123"等)
#define PutsValSGN(v, s, f)     PutsValX((v), (s), 0,   0,   (f))	// 正符号も強制ON      ("+123"等)
#define PutsVal0SGN(v, s, k, f) PutsValX((v), (s), (k), '0', (f))	//                     ("+00123"等)
#define PutsValBSGN(v, s, k, f) PutsValX((v), (s), (k), ' ', (f))	//                     ("+  123"等)


//neg&1 強制-追加オプション、 neg&2、±追加オプション
void PutsPointX(s32 v, s32 keta, s32 pre, s32 neg){
	if(pre == 0){
		if(neg & 4) PutsVal0SGN(v, 10, keta, neg);
		else        PutsValBSGN(v, 10, keta, neg);
		return;
	}
	if(v < 0){
		neg |= 1;
		v = -v;
	}
	s32 dv1 = INT_POS_TABLE[pre];
	v = BiosDiv(v, dv1, &dv1);
	if(neg & 4) PutsVal0SGN(v, 10, keta, neg);
	else        PutsValBSGN(v, 10, keta, neg);
	Putc('.');
	if(pre) PutsVal0(dv1, 10, pre);
}

u32 PutsLen(const u8* str){
	u32 n = 0;
	for(; *str ; ++str)	n += ((u8)(*str - 0x20) < FONT_WIDE)? 1 : 2;
	return n;
}

void PutsLatLon(s32 v){
	s32 h, m, s;
	s = BiosDiv(v, 1000, &v);
	m = BiosDiv(s, 60, &s);
	h = BiosDiv(m, 60, &m);

	PutsValB(h, 10, 3);		Putc('°');
	PutsVal0(m, 10, 2);		Putc('\'');
	PutsVal0(s, 10, 2);		Putc('"');
	PutsVal0(v, 10, 3);
}
void PutsLat(s32 v){
	if(v < 0){
		Putc('S');
		v = -v;
	} else {
		Putc('N');
	}
	PutsLatLon(v);
}

void PutsLon(s32 v){
	if(v < 0){
		Putc('W');
		v = -v;
	} else {
		Putc('E');
	}
	PutsLatLon(v);
}

void PutsDistance(u32 m){ // 最大6文字
	if(m < 10000){		// 10km未満
		Putsf("%4dm", m); // 9999m
	} else {
		if(m < 100000 - 50){ // 10km以上 100km未満
			Putsf("%.1fkm", RoundDiv(m, 100)); // 12.3km
		} else if(m < 9998500){		// 100km以上
			Putsf("%4dkm", RoundDiv(m, 1000)); // 1234km
		} else {
			Puts("9999km"); // 9999km
		}
	}
}
void PutsDistance2(s32 m){ // 最大5文字
	if(m < 0){
		PutsSpace(5);
	} else if(m < 10000){
		Putsf("%dm", m); // 9999m
		PutsValSpace(m, 4);
	} else {
		m = RoundDiv(m, 1000);
		if(m > 999) m = 999;
		Putsf("%dkm", m); // 999km
		PutsValSpace(m, 3);
	}
}

void PutsSpace(u32 n){
	while(n--) Putc(' ');
}

void PutsSpace2(s32 x, s32 y, u32 n){
	Locate(x, y);
	PutsSpace(n);
}

void PutsValSpace(u32 n, u32 keta){
	while(n < INT_POS_TABLE[--keta]) Putc(' ');
}

///////////////////////////////////////////////////////////////////////////////
// 座標指定タイプ
///////////////////////////////////////////////////////////////////////////////
void DrawText(u32 x, u32 y, const u8* str){
	Locate(x, y);
	Puts(str);
}

void UnderLine(u32 x, s32 y, s32 len){
	FillTile(MAP_BG0, x, y, x + len,  y, SELMENU_UDL);
}

void DrawTextCenter(u32 y, const u8* str){
	s32 n = (30 - PutsLen(str)) / 2;
	Locate((n < 0)? 0 : n, y);
	Puts(str);
}

void DrawTextUL(u32 x, u32 y, const u8* str){
	DrawText(x, y, str);
	UnderLine(x, y + 2, IW->mp.gx - x - 1);
}

///////////////////////////////////////////////////////////////////////////////
// フォーマット出力
///////////////////////////////////////////////////////////////////////////////
#include <stdarg.h>

// printf似だが、独自仕様なのでprintfと混同しないよう別名にした
u32 Putsf(const u8* fmt, ...){
	u32 left_pos = 0;
	va_list ap;
	va_start(ap, fmt);
	for(; *fmt ; ++fmt){
		if(*fmt != '%'){
			Putc(*fmt);
			continue;
		}
		u32  keta[2] = {0};
		u32* pketa = keta;
		u32 sign = 0;
		u8  pad = ' ';
		for(;;){
			++fmt;

			// プレフィックス
			if(*fmt == '+'){		// 強制符号
				sign = 2;
				continue;
			}
			if(*fmt == '.'){		// 小数点桁数、ほか用
				pketa = keta + 1;
				continue;
			}
			if(*fmt == '0'){
				if(*pketa) *pketa *= 10;
				else       pad = *fmt; // 先頭0は'0'詰め
				continue;
			}
			if('1' <= *fmt && *fmt <= '9'){
				*pketa = *pketa * 10 + *fmt - '0';
				continue;
			}

			// 出力コード
			if(*fmt == '%'){
				Putc(*fmt);
				break;
			}
			if(*fmt == 'd'){ // 整数10進
				PutsValX(va_arg(ap, u32), 10,  keta[0], pad, sign);
				break;
			}
			if(*fmt == 'x'){ // 整数16進
				PutsValX(va_arg(ap, u32), 16,  keta[0], pad, sign);
				break;
			}
			if(*fmt == 'f'){ // 固定少数
				PutsPointX(va_arg(ap, u32), keta[0], keta[1], (pad == ' ')? sign : (sign | 4));
				break;
			}
			if(*fmt == 'c'){ // 文字出力
				Putc((u8)va_arg(ap, u32));
				break;
			}
			if(*fmt == 's'){ // 文字列
				u8* p = (u8*)va_arg(ap, u32);
				if(p) Puts(p); // 一応NULLチェック
				break;
			}
			if(*fmt == 'N'){ // 緯度
				PutsLat(va_arg(ap, u32));
				break;
			}
			if(*fmt == 'E'){ // 経度
				PutsLon(va_arg(ap, u32));
				break;
			}
			if(*fmt == 'r'){ // ボックス改行
				IW->mp.gx = left_pos;
				IW->mp.gy += keta[0]? keta[0] : 2;
				break;
			}

			// 特殊フォーマット（再帰あり）
			if(*fmt == 't'){ // DT値の時刻表示専用
				u32 hour, min, sec = va_arg(ap, u32);
				if(sec){
					GetDTTime(sec, &hour, &min, &sec);
					Putsf("%02d:%02d:%02d", hour, min, sec); // 再帰…
				} else {
					Puts("--:--:--");
				}
				break;
			}
			if(*fmt == 'F'){ // チェック付4.3出力 (arg=2)
				s32 val = va_arg(ap, u32); // 必ずロードしておく
				if(va_arg(ap, u32)){
					PutsPointX(val, 4, 3, sign);
				} else {
					Puts("____.___");
				}
				break;
			}
			if(*fmt == 'D'){ // チェック付8出力 (arg=2)
				s32 val = va_arg(ap, u32); // 必ずロードしておく
				if(va_arg(ap, u32)){
					PutsValX(val, 10,  8, pad, sign);
				} else {
					Puts("________");
				}
				break;
			}
			if(*fmt == 'T'){ // DT値の日付時刻表示専用
				u32 year, month, day = va_arg(ap, u32);
				if(day){
					GetDTDate(day, &year, &month, &day);
					Putsf("%04d/%02d/%02d", year, month, day); // 再帰…
				} else {
					Puts("----/--/--");
				}
				break;
			}
			if(*fmt == 'S'){ // 秒数表示(時/分変換)
				s32 s = va_arg(ap, u32);
				s32 s_org = s;
				if(sign) sign = va_arg(ap, u32);

				if(s < 0) Puts("--:--:--");
				else {
#define MAX_SEC7 ( 9 * 3600 + 59 * 60 + 59)
#define MAX_SEC8 (99 * 3600 + 59 * 60 + 59)
					if(keta[0] == 7 && s > MAX_SEC7) s = MAX_SEC7;
					if(keta[0] == 8 && s > MAX_SEC8) s = MAX_SEC8;
					s32 h = BiosDiv(s, 3600, &s);
					s32 m = BiosDiv(s,   60, &s);
					if(h) Putsf("%2d:%02d:%02d", h, m, s);
					else  Putsf("   %2d:%02d", m, s);

					// パーセント表示  (arg=2)
					if(sign) Putsf("(%3d%%)", RoundDiv(s_org * 100, sign));
				}
				break;
			}

			// 非出力系制御コマンド
			if(*fmt == 'm'){ // ボックス表示用カーソル移動(直指定　%10.20m)
				IW->mp.gx = left_pos = keta[0];
				IW->mp.gy = keta[1];
				break;
			}
			if(*fmt == 'P'){ // X座標直指定カーソル移動
				IW->mp.gx = keta[0];
				break;
			}
			if(*fmt == 'M'){ // ボックス表示用カーソル移動(引数指定)
				IW->mp.gx = left_pos = va_arg(ap, u32);
				IW->mp.gy = va_arg(ap, u32);
				break;
			}

			// フォーマットエラーチェック
			if(!*fmt){
				va_end(ap);
				return -1;
			}
		}
	}
	va_end(ap);
	return 0;
}

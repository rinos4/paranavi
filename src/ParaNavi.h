///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005-2008 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////
// v0.1  2005/01/03 rinos
//       ・初版作成
// v0.2  2005/03/26 rinos
//       ・ウェイポイント/ルートデータを増量
//       ・90°回転画面対応
//       ・音声ナビ対応
//       ・タスク制御を全てアプリで処理するよう変更(自作OS削除)
// v1.0  2005/05/05 rinos
//       ・SPモード(軌跡モード、ベンチマークモード、風速計モード)追加
// v1.1  2005/07/03 rinos
//       ・フォント変更
// v1.2  2005/09/17 rinos
//       ・SBAS用精度マーカ表示対応(ひまわり6号MSAS試験運用開始記念)
// v1.3  2005/10/02 rinos
//       ・パイロン名の表示設定追加
//       ・軌跡色レンジ設定追加
//       ・ステート別の音声切り替え対応
// v1.4  2005/11/03 rinos
//       ・パイロン毎のシリンダサイズ設定対応
//       ・メニュー欄外へのプチ情報表示を追加
// v1.5  2005/11/12 rinos
//       ・1周あたりの上昇率の表示を追加
//       ・カートリッジの部分コピー対応
// v1.6  2005/12/29 rinos
//       ・180°、270°の回転画面追加
//       ・軌跡ポイント数を1800に増量
// v1.7  2006/02/19 rinos
//       ・ゴール判定(ライン、180°セクタ)の切り替えをメニューに追加
//       ・トラックログの記録対応
// v1.8  2006/03/18 rinos
//       ・パイロン名リスト表示時の文字位置の指定機能を追加
//       ・シンプル音声シーケンスを追加
// v1.9  2006/05/28 rinos
//       ・センタリング半径の表示を追加
// v1.10 2006/09/17 rinos
//       ・個人情報初期化対応
// v2.0  2006/11/25 rinos
//       ・SPモードにシリンダマップ表示を追加
//       ・プレパイロンの設定を追加
//       ・フリーフライトモード用にランディング設定を追加
//       ・PCへトラックログをアップロードする機能を追加
//       ・風速計接続対応
// v2.1  2007/01/14 rinos
//       ・２コ先パイロンの予想高度の計算を変更
//       ・ゴールまでの残距離、予想ゴール高度差の表示機能追加
//       ・フライトログ保存機能を追加
//       ・音声のインターバル設定機能追加
//       ・90°セクタゴールの設定を追加
// v2.2  2007/05/12 rinos
//       ・GPS60とGPS76CSに対応
//       ・ゴールと次パイロンのL/D表示を追加
//       ・トラックログの保存領域拡張
//       ・スタートアラーム音対応
// v2.3  2007/11/03 rinos
//       ・最大シリンダサイズの拡張(65km)
//       ・第一パイロンのスキップ設定追加(TOパイロン無視)
//       ・GPS通信のポップアップ警告表示対応
//       ・Locusモードのカラータイプを追加
//       ・PCとルート/ウェイポイントを交換する機能を追加
// v2.4  2007/12/09 rinos
//       ・ピッチング統計をセンタリング/非センタリング中に分けて表示するよう変更
//       ・軌跡モードとシリンダモードの表示優先度設定を追加
// v2.5  2008/02/23 Mr.Takemura
//       ・GPS MAP 60CSx対応 (D109/D110_Wpt_Type追加)
// v2.6  2008/05/25 rinos
//       ・GPS-52D用にNMEA-0183へ対応 (NMEAデコーダ追加、タスク制御変更)
//       ・Bluetooth接続対応。(BTモジュールはParani-ESD、Bluetooth GPSはSPPデバイスに対応)
//       ・緯度経度を拡大表示するテキストモードを追加(Aボタンx3で表示)
//       ・ルートメニューにルートを1つだけ選択して転送する機能を追加
//       ・シンプル画面のルート名の欄を現在時刻に変更
//       ・メモリ不足対策でLocus用緯度経度のデータを圧縮(-7KB)

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
#include "font/comp.h" // メモリが少なくなってきたため、圧縮してみた


///////////////////////////////////////////////////////////////////////////////
// inline関数
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
// BIOSコール
///////////////////////////////////////////////////////////////////////////////
#ifdef __thumb__
#define BiosHalt() __asm("swi 0x2\n")
#define BiosStop() __asm("swi 0x3\n")
#else
#define BiosHalt() __asm("swi 0x20000\n")
#define BiosStop() __asm("swi 0x30000\n")
#endif

// 算術
s32 BiosDiv(s32 num, s32 denom, s32* mod);
u32 BiosSqrt(u32 n);
s32 BiosArcTan2(s32 x, s32 y);
s32 RoundDiv(s32 num, s32 denom);
static inline s32 RoundShift(s32 v, s32 sft){
	return (v + (1 << (sft - 1))) >> sft;
}

// BiosFuncを使用した高速関数
u32 BiosHypot(s32 a, s32 b);			// BiosSqrtを使った高速hypot
u32 BiosTripot(s32 a, s32 b, s32 c);	// BiosSqrtを使った高速tripot
s32 BiosAtan64K(s32 yK, s32 xK);		// mathと同じ引数で 64Kで返す

s32 CountBit(s32 a);
static inline s32 MulBit(s32 a, s32 b) { return CountBit(a) + CountBit(b); }


// 初期化
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

// LZ77展開
u32 BiosLZ77UnCompVram(const void* src, const void* dst); // word alignment!
u32 BiosLZ77UnCompWram(const void* src, const void* dst); // word alignment!


// その他(BIOSコールではないが、ここで宣言)
u32  GetPC();
void JumpR0();
void MySrand(u32 v);
u32 MyRand();

u32 InterlockedExchange(vu32* p, u32 val);
void Suspend();

#define IS_MBOOT() (*(u8*)0x020000c4)
#define ROM_SIZE 0x40000 // RAMコピー用


///////////////////////////////////////////////////////////////////////////////
// 算術系
///////////////////////////////////////////////////////////////////////////////
// 高速sin
s32 Sin64K(s32 angle);
static inline s32 Cos64K(s32 angle){ return Sin64K(angle + 0x4000);}
// 高速sin(距離計算精度を高めるための線形補完型)
s32 Sin64KX(s32 angle);
static inline s32 Cos64KX(s32 angle){ return Sin64KX(angle + 0x4000);}

void CalcDist1(s32 lat0, s32 lon0, s32 lat1, s32 lon1, u32* len, u32* houi);// 高精度
void CalcDist2(s32 lat0, s32 lon0, s32 lat1, s32 lon1, u32* len, u32* houi);// 高速
void CalcDist (s32 lat0, s32 lon0, s32 lat1, s32 lon1, u32* len, u32* houi);// 自動

#define ANG64K(ang360) (ang360 * 0x10000 / 360) // 定数作成用なので、心置きなく割り算を使用…


///////////////////////////////////////////////////////////////////////////////
// オブジェ(スプライト)
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

//以降はNAVI用(メニュー時には透過)
#define OBJ_NAVI_START		4
#define OBJ_BIG_NUM_BEGIN	4 // 拡大数字 速度4桁+距離4桁
#define OBJ_BIG_NUM_END		15
#define OBJ_MY_POS			16	// 自機位置目安
#define OBJ_STATUS			18	// ～ 測位状態
#define OBJ_VARIO_CENTER	24
#define OBJ_STAR_BEGIN		25  // ～ 磁石星
#define OBJ_NEXT_PYLON		37	// 次パイロン(水平用)
#define OBJ_NESW_BEGIN		38  // NESW
#define OBJ_VERIO_BEGIN		52	// ～ バリオメータ用
#define OBJ_VERIO_END		71
//RFU                       72-100 RFU
#define OBJ_NW_PYLN			101	// NaviWin パイロン
#define OBJ_NW_PYLN2		102	// NaviWin 次パイロン(垂直用)
#define OBJ_CIRCLE_BEGIN	103 // ～126 外枠
#define OBJ_NW_BASE			127 // NaviWin ベース(セクタ)

// 垂直表示用
// BIGNUM                   4-7
#define OBJ_V_P1			8
#define OBJ_V_P1_END		19
#define OBJ_V_P2			20
#define OBJ_V_P2_END		27
#define OBJ_V_BOX			28
#define OBJ_V_BOX_END		94
#define OBJ_V_STATUS		95 // 垂直用(優先度低)
// RFU                      96-102 RFU

#define OBJ_CYL_BEGIN		28 // Star以降を利用
#define OBJ_CYL_END			125
#define OBJ_CYL_NW_PYLN		126

//Locus View
#define OBJ_LOCUS_NESW		4	// ～9 NESW+Self+Pylon
#define OBJ_LOCUS_TARGET	10	// 2桁 WPT番号
#define OBJ_LOCUS			12	// ～127
#define OBJ_LOCUS_END		127	// (118個)


// タイルマッピング (0-1023 ÷2)
#define OBJ_TN_CURSOR		0 // メニュー選択
#define OBJ_TN_KETA			2 // 数値入力用カレット
#define OBJ_TN_UP			4 // スクロール矢印
#define OBJ_TN_STAR0		6 // 磁石星
#define OBJ_TN_STAR1		8
#define OBJ_TN_STAR2		10
#define OBJ_TN_STATUS_NG1	12 // timeout
#define OBJ_TN_STATUS_NG2	14 // invalid
#define OBJ_TN_STATUS_2D	16 // 2D捕捉
#define OBJ_TN_STATUS_3D	18 // 3D捕捉
#define OBJ_TN_STATUS_GOOD	20 // SBAS捕捉
#define OBJ_TN_NESW			22 // ～35 東西南北 横画面用 16block(1x2*4)
#define OBJ_TN_SELF			38 // 自機
#define OBJ_TN_VARIO_C		42 // バリオメータ
#define OBJ_TN_VARIO_L		50
#define OBJ_TN_VARIO_R		52
#define OBJ_TN_SELF_R		50 // OBJ_TN_VARIO_L/Rを上書き

#define OBJ_TN_LEAD			54	// 引込み線(2x2)
#define OBJ_TN_GUIDE1		62	// パイロンガイド
#define OBJ_TN_GUIDE2		64	// パイロンガイド
#define OBJ_TN_GUIDE3		66	// パイロンガイド

#define OBJ_TN_CIRCLE		68  // 磁石の外枠
#define OBJ_TN_CIRCLE_R		88  // 磁石の外枠 縦
#define OBJ_TN_SMLNUM		106 // 数字(小)
#define OBJ_TN_SMLNUM_R		158 // 数字(小) 縦

// 以降は計算で描画する
#define OBJ_TN_LOCUS		212 // 軌跡
#define OBJ_TN_UP_R			278
#define OBJ_TN_VARIO_CR		280
#define OBJ_TN_NEXT			288	// 次パイロン        32block(4x4)
#define OBJ_TN_NESW_R		336	// 東西南北 縦画面用 16block(2x1*4)
#define OBJ_TN_VARIO		352	// バリオ横画面用    32block(2x1*8)
#define OBJ_TN_VARIO_R		384	// バリオ縦画面用    32block(1x2*8)
#define OBJ_TN_BIGNUM		416	// 縦画面用         176block(2x4*11)
#define OBJ_TN_BIGNUM_R		592 // 縦画面用         176block(4x2*11)
#define OBJ_TN_PYLN			768 // パイロン矢印      64block(8x8)
#define OBJ_TN_BASE			896 // セクタ            64block(8x8)


// 回転設定(0-31)
#define OBJ_RN_PYLN		0	// パイロン
#define OBJ_RN_BASE		1	// ベース(セクタ)
#define OBJ_RN_NEXT		2	// 次パイロン
#define OBJ_RN_CYL1		3	// 前シリンダガイド
#define OBJ_RN_CYL2		4	// 次シリンダガイド
#define OBJ_RN_CYL3		5	// セクタガイド1
#define OBJ_RN_CYL4		6	// セクタガイド2


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
// マップ
///////////////////////////////////////////////////////////////////////////////
// 0x00..0x5a * 2 = 182 半角英数
// 0x5b..0xff * 4 = 660 全角かな
//               計 842(0x34A)
// #842-#847:     = 6   ボーダー 
// #848:          = 1   メニュー選択1
// #849:          = 1   メニュー選択2
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
#define SELMENU_GR2 (BD_INDEX + 9 + (1 << 10)) // 左右反転

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
// テキスト出力
///////////////////////////////////////////////////////////////////////////////
const u32 INT_POS_TABLE[10];
const u32 TIME_POS_TABLE[4];

// ローレベル表示制御
void Locate (u32 x, u32 y);	// カーソル移動
void LocateX(u32 x);		// y座標は保持したままx座標だけ移動
void Cls();					// 画面クリア
void ClsId(u32 id, u16 tile);
void Putc   (u8 ch);		// 1文字出力
void Putc2nd(u8 ch);		// ワイドキャラクタの後半部の表示(半キャラスクロール用)
void PutsValX  (s32 v, s32 s, s32 keta, u8 pad, u32 f);	// 整数出力
void PutsPointX(s32 v, s32 keta, s32 pre, s32 neg);		// 固定小数点出力
void Puts(const u8* str);	// NULL終端文字列出力
u32  PutsLen(const u8* str);// 出力幅計算(半角=+1,全角=+2)
void PutsLat(s32 v);		// 緯度出力専用
void PutsLon(s32 v);		// 経度出力専用

// 名前専用
void PutsName2(const u8* s, u32 len, u32 f);
#define PutsName(s)  (PutsName2((s), sizeof(s), 0)) // 有効文字のみ出力。必ず配列指定!
#define PutsNameB(s) (PutsName2((s), sizeof(s), 1)) // 空白で不足文字を埋める。必ず配列指定!
void PutsDistance (u32 m); // 右詰最大6文字("   0m ", "1234m ", "9999km")
void PutsDistance2(s32 m); // 左詰最大5文字("0m   ",  "1234m",  "999km")

void PutsSpace(u32 n);					// 指定した数の空白を出力
void PutsSpace2(s32 x, s32 y, u32 n);	// 指定座標から空白出力
void PutsValSpace(u32 n, u32 keta);		// 桁数テーブル分空白出力

// 座標指定文字列出力
void DrawText(u32 x, u32 y, const u8* str);
void DrawTextCenter(u32 y, const u8* str);		// センタリング表示(y座標のみ指定)
void DrawTextUL(u32 x, u32 y, const u8* str);	// アンダーライン付文字列
void UnderLine(u32 x, s32 y, s32 len);			// アンダーラインのみ

// 固定小数表示マクロ
#define PutsPoint(v, keta, pr)     PutsPointX((v), (keta), pr, 0)	// 標準                ("12.3"等)
#define PutsPointSGN(v, keta, pr)  PutsPointX((v), (keta), pr, 2)	// 正符号も強制ON      ("+12.3"等)
#define PutsPoint0SGN(v, keta, pr) PutsPointX((v), (keta), pr, 6)	// 不足桁を0で埋める   ("+012.3"等)

// フォーマット出力
u32  Putsf(const u8* fmt, ...);

// ポップアップメッセージ自動消去設定
void SetBootMsgTimeout(u32 n);

///////////////////////////////////////////////////////////////////////////////
// サウンド
///////////////////////////////////////////////////////////////////////////////

#define SOUND_CH_KEY	0 // キークリック、他
#define SOUND_CH_VARIO	1 // バリオモドキ

void EnableSound(u32 n, s32 vol);
u32  EnableSoundCheck(u32 n);
void PlaySG1 (const u16* s);	// 前の音を強制停止して音を鳴らす
void PlaySG1X(const u16* s);	// 前の音が終了しているときだけ鳴らす
void SetCh2Tone(u16 freq);		// バリオ用トーン設定

// 音色定義
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

// 音声データヘッダ
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
// Garminプロトコルスタック
///////////////////////////////////////////////////////////////////////////////

typedef u32 EStatus;
enum {
	ERROR_SUCCESS			= 0,

	// LPレベルで再送
	ERROR_BAD_PID			= 10,
	ERROR_LENGTH			= 11,	// 長さエラー
	ERROR_CHECKSUM			= 12,	// チェックサムエラー
	ERROR_DLE				= 13,	// DLEスタッフィングエラー
	ERROR_LP_LEVEL,

	// アプリレベルで対応
	ERROR_TIMEOUT			= 100,	// タイムアウト
	ERROR_TOO_SHORT_BUFFER	= 101,
	ERROR_SENDING			= 102,	// 既に誰かが送信中
	ERROR_TOO_BIG_PACKET	= 103,	// 送信パケットが大きすぎる
	ERROR_APP_LEVEL,

	// データレベル
	ERROR_INVALID_FIX		= 200,	// 無効なFIX値
};

// 基本パケットID
enum {
    Pid_Ack_Byte       =   6,
    Pid_Nak_Byte       =  21,
    Pid_Protocol_Array = 253,
    Pid_Product_Rqst   = 254,
    Pid_Product_Data   = 255
};

// リンクプロトコル１
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

// コマンドID
enum {
    Cmnd_Abort_Transfer  = 0,  // 現在の伝送の中止
    Cmnd_Transfer_Alm    = 1,  // 軌道要素の伝送
    Cmnd_Transfer_Posn   = 2,  // 位置の伝送
    Cmnd_Transfer_Prx    = 3,  // 近接ウェイポイントの伝送
    Cmnd_Transfer_Rte    = 4,  // ルートの伝送
    Cmnd_Transfer_Time   = 5,  // 時刻の伝送
    Cmnd_Transfer_Trk    = 6,  // 軌跡の伝送
    Cmnd_Transfer_Wpt    = 7,  // ウェイポイントの伝送
    Cmnd_Turn_Off_Pwr    = 8,  // 電源の切断
    Cmnd_Start_Pvt_Data  = 49, // PVTデータの伝送開始
    Cmnd_Stop_Pvt_Data   = 50  // PVTデータの伝送終了
};

enum {
    FIX_UNUSABLE    = 0,   // 状態チェック失敗
    FIX_INVALID     = 1,   // 無効あるいは利用不能 //
    FIX_2D          = 2,   // ２次元
    FIX_3D          = 3,   // ３次元
    FIX_2D_diff     = 4,   // ディファレンシャル２Ｄ
    FIX_3D_diff     = 5    // ディファレンシャル３Ｄ
};

typedef struct {
    u8 alt[4];			// float  WGS84楕円体からの高度(m)
    u8 epe[4];			// float  推定位置誤差、2σ(m)
    u8 eph[4];			// float  epeの水平成分(m)
    u8 epv[4];			// float  epeの垂直成分(m)
    u8 fix[2];			// int    測位の方法
    u8 tow[8];			// double 週の時間(s)
    u8 lat[8];			// double 緯度(radian)
    u8 lon[8];			// double 経度(radian)
    u8 east[4];			// float  速度の東成分(m/s)
    u8 north[4];		// float  速度の北成分(m/s)
    u8 up[4];			// float  速度の上成分(m/s)
    u8 msl_hght[4];		// float  WGS84楕円体のMSL高(m)
    u8 leap_scnds[2];	// int    GPSとUTCの差(s)
    u8 wn_days[4];		// long   週番号日(注意: 1989/12/31 UTCから現在の週の初めまでの日数)
} D800_Pvt_Data_Type;

// D800_Pvt_Data_Typeはアライメントが揃っていないのでPvtXに入れなおして使う
#define INVALID_VAL 0x80000000
typedef struct {
	u32	counter;	// 受信カウンタ
	u32 fix_sum[6];	// 各測位のカウンタ
    u32 fix;        // 測位の方法
    s32	lat;		// 緯度[°]
    s32	lon;		// 経度[°]
	s32	alt_mm;		// 高度[mm] (WGS84楕円体 or 平均海面高度)
	s32	epe_mm;		// 誤差、2σ(mm)
	s32	eph_mm;		// 水平epe(mm)
	s32	epv_mm;		// 垂直epe(mm)
	s32	up_mm;		// 上昇速度(mm/s) 負のときは下降速度
	s32	vh_mm;		// 水平速度(mm/s) ≧0
	s32	v_mm;		// 速度
	s32 vn_mm;		// 水平北成分
	s32 ve_mm;		// 水平東成分
	s32 g_mm;		// 加速度

	u32	h_ang64;	// 水平方位(0000 ～ ffff)
	u32	h_ang_c;	// 磁石補正
	u32	v_ang64;	// 垂直方位(c000 ～ 4000) 0000-4000が上昇
	s32 ldK;		// L/D * 1000

	// 時間
	u32 year;	// 1900～
	u32 month;	// 1～12 
	u32 day;	// 1～31
	u32 week;	// 0:日曜日～
	u32 hour;	// 0～23
	u32 min;	// 0～59
	u32 sec;	// 0～59
	u32 dtime;	// 1989, 12/31 12:00AM UTCから経過した秒数

	// 平均情報
	s32 ldK_avg;	// 平均L/D
	s32 vh_avg;		// 平均対地速度
	s32 up_avg;		// 平均垂直速度

	s32 up_turn;	// Lift[mm]/Turn
	s32 up_r;		// 回転半径[m]
	s32 up_time;	// 回転時間 s/Turn

	// グライダーステート
	u32 gstate;
	// PVTタイムアウト監視用
	u32 pvt_timeout;
	u32 rx_pre;
} PvtX;

// D100,D101,D102,D103,D104,D107,D151,D152,D154,D155
// >56 byte
typedef struct {
    u8 ident[6];	// 識別子
	u8 lat[4];		// Semicircle 緯度
	u8 lon[4];		// Semicircle 経度
    u32 unused;		// ０が設定される
	u8  cmng[40];	// コメント
} D10X_Wpt_Type;

// D108
// 49～59 byte
typedef struct {
	u8 wpt_class;	// byte クラス
    u8 color;		// byte 色
	u8 dspl;		// byte 表示オプション
	u8 attr;		// byte 属性
    u8 smbl[2];		// Symbol_Type ウェイポイントシンボル
	u8 subclass[18];// u8[18] サブクラス
	u8 lat[4];		// Semicircle 緯度
	u8 lon[4];		// Semicircle 経度
	u8 alt[4];		// float 高度(m)
	u8 dpth[4];		// float 深度(m)
	u8 dist[4];		// float 接近距離(m)
	u8 state[2];	// short 状態
	u8 cc[2];		// short 国コード
	u8 ident[1];	// 可変長文字列(attr依存)
} D108_Wpt_Type ;

// 
// D109
// 49～59 byte
typedef struct {
	u8 wpt_class;	// byte クラス
    u8 color;		// byte 色
	u8 dspl;		// byte 表示オプション
	u8 attr;		// byte 属性
    u8 smbl[2];		// Symbol_Type ウェイポイントシンボル
	u8 subclass[18];// u8[18] サブクラス
	u8 lat[4];		// Semicircle 緯度
	u8 lon[4];		// Semicircle 経度
	u8 alt[4];		// float 高度(m)
	u8 dpth[4];		// float 深度(m)
	u8 dist[4];		// float 接近距離(m)
	u8 state[2];	// short 状態
	u8 cc[2];		// short 国コード
	u32 etc;		// etc;
	u8 ident[1];	// 可変長文字列(attr依存)
} D109_Wpt_Type ;

// D110
// 49～59 byte
typedef struct {
	u8 wpt_class;	// byte クラス
    u8 color;		// byte 色
	u8 dspl;		// byte 表示オプション
	u8 attr;		// byte 属性
    u8 smbl[2];		// Symbol_Type ウェイポイントシンボル
	u8 subclass[18];// u8[18] サブクラス
	u8 lat[4];		// Semicircle 緯度
	u8 lon[4];		// Semicircle 経度
	u8 alt[4];		// float 高度(m)
	u8 dpth[4];		// float 深度(m)
	u8 dist[4];		// float 接近距離(m)
	u8 state[2];	// short 状態
	u8 cc[2];		// short 国コード
	u32 etc;		// etc;
	u8 temp[4];		// float 温度
	u32 time;		// タイムスタンプ
	u16 wpt_cat;	// waypoint カテゴリー
	u8 ident[1];	// 可変長文字列(attr依存)
} D110_Wpt_Type ;

//D105 11～21 byte
typedef struct {
	u8 lat[4];		// Semicircle 緯度
	u8 lon[4];		// Semicircle 経度
	u8 smbl[2];		// Symbol_Type ウェイポイントシンボル
	u8 ident[1];	// NULL終端
} D105_Wpt_Type ;

//D106 25～35 byte
typedef struct {
	u8 wpt_class;	// クラス
	u8 subclass[13];// サブクラス
	u8 lat[4];		// Semicircle 緯度
	u8 lon[4];		// Semicircle 経度
	u8 smbl[2];		// Symbol_Type ウェイポイントシンボル
	u8 ident[1];	// NULL終端
} D106_Wpt_Type ;

//D150 115byte
typedef struct {
	u8 ident[6];	// 識別子
    u8 cc[2];		// 国コード
    u8 wpt_class;	// クラス
	u8 lat[4];		// Semicircle 緯度
	u8 lon[4];		// Semicircle 経度
	u8 alt[2];		// 高度(m)
    u8 city[24];	// 市
    u8 state[2];	// 州
    u8 name[30];	// 施設名
    u8 cmnt[40];	// コメント
} D150_Wpt_Type ;


typedef struct {
	u8 lat[4];		// Semicircle 緯度
	u8 lon[4];		// Semicircle 経度
	u8 dtime[4];	// 1989, 12/31 12:00AM UTC 経過秒
	u8 new_trk;
} D300_Trk_Point_Type;

typedef struct {
	u8 lat[4];		// Semicircle 緯度
	u8 lon[4];		// Semicircle 経度
	u8 dtime[4];	// 1989, 12/31 12:00AM UTC 経過秒
	u8 alt[4];		// float 高度(m)
	u8 dpth[4];		// float 深度(m)
    u8 new_trk;
} D301_Trk_Point_Type;

typedef struct {
    u8 dspl;		// 表示フラグ
    u8 color;		// 表示色
	u8 trk_ident[];	// 名前。MAX51byte
} D310_Trk_Hdr_Type;

#define INVALID_LAT 0x80000000
#define NMEA_DEV_ID 9999 // GARMIN装置に存在しないID

enum {
	// NMEAセンテンス
	NMEA_LOG_ID_GGA,
	NMEA_LOG_ID_GSA,
	NMEA_LOG_ID_GSV,
	NMEA_LOG_ID_RMC,
	NMEA_LOG_ID_VTG,
	NMEA_LOG_ID_ZDA,
	NMEA_LOG_ID_GLL,
	NMEA_LOG_ID_ALM,

	// エラー関連
	NMEA_LOG_ID_UNKNOWN,
	NMEA_LOG_ID_FIELD_ERR,
	NMEA_LOG_ID_TALKER_ERR,

	// ログ用に衛星数をココに追加(PvtXでも良いが…)
	NMEA_LOG_ID_SAT_NUM,

	NMEA_LOG_ID_COUNT
};


enum{
	DL_FSM_WAIT_DLE,	// 0:DLE待ち
	DL_FSM_WAIT_PID,	// 1:PID待ち
	DL_FSM_WAIT_ANY,	// 2:任意のキャラクタ待ち
	DL_FSM_GET_DLE,		// 3:DLE取得直後

	DL_FSM_NMEA_SUM,	// 4:NMEA'*'まち
	DL_FSM_NMEA_CR,		// 5:NMEA終端待ち

	DL_FSM_ATC_WAIT,	// 6:ATコマンド改行待ち(Bluetooth用)
	DL_FSM_ATC_ANY,		// 7:ATコマンドバッファ(Bluetooth用)

	// これ以降の状態はアプリで要因クリアが必要
	DL_FSM_COMPLETE,	// 8:GARMIN受信完了
	DL_FSM_NMEA_END,	// 9:NMEAの1センテンス終了
	DL_FSM_ATC_END,		//10:ATコマンド応答完了(Bluetooth用)

	// 以降、エラー
	DL_FSM_E_UART,		//11:I/Oエラー
	DL_FSM_E_DROP,		//12:取りこぼしエラー
	DL_FSM_E_PACKET,	//13:バッファ不足(ありえないパケット長)
};

typedef struct {
	s32 dif[4];
	s32 val[4];
	s32 prc;
	u32 tpos;	// 現在のトラックログポインタ
	u32 rep;
	u32 seg;

	u32 pre_sect;
	u8* sect_ptr;
} TrackInfo;

typedef struct {
	u32 state;		// 通信FSM

	s32 timeout;	// 通信タイムアウト用のカウンタ
	s32 emu_nak;	// エミュレーションモードタイムアウト検出用
	u32 vbc;		// タイムアウト監視用VBC

	u16 pid;		// 製品情報
	u16 version;	// FWバージョン
	u8  name[128];	// 製品名
	u8  parray[128];// キャパビリティ

	u32 pre_pvt;	// PVTタイムアウト監視
	u32 pre_count;	// ダウンロードタイムアウト監視用
	u32 dl_accept;	// ダウンロード許可フラグ(中断用)
	u32 dl_num;		// ダウンロード総数
	u32 dl_count;	// ダウンロード経過数
	u32 dl_route;	// ルートダウンロード/アップロードIndex
	u32 dl_route_end;// アップロード終端
	u32 ul_acknak;	// ACK/NAK待ち
	u32 ul_track;	// アップロード開始時の総トラック数
	u32 ul_pre;		// 直前のアップロードカウンタ
	TrackInfo tinfo;// 現在のトラックログ情報
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

	// Bluetooth用制御を追加
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

// アライメントを気にせずデータ取得するための関数
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
u32  GetDTTime  (u32 v, u32* hour, u32* min, u32* sec); // mod Time(日にち)を返す
void GetDTDate  (u32 v, u32* year, u32* month, u32* day);

// GPSへのコマンド送信
EStatus LpSendCmd(u16 cmd);

void StartGPSMode();

///////////////////////////////////////////////////////////////////////////////
// 風速計情報
///////////////////////////////////////////////////////////////////////////////
#define CALIB_END_TIME  (16384 * 10)	// 最低10秒

typedef struct {
	u16 calc_flag;
	u16 pre_level;
	u32 tm;			// 16384Hz / 72時間タイマ
	u32 pulse;
	u32 pre_tm;
	u32 dif_tm;

	// 補正用
	u32 calib_flag;
	u32 calib_tm;
	u32 calib_pulse;
	u32 calib_input;
	u32 calib_vsum;
	u32 calib_vcnt;

	// 計算で求める値
	u32 avg_tm;		// 平均用カウンタ
	u32 avg_pulse;	// 平均用パルスカウンタ
	u32 rpm;		// 直前の値
	u32 rpm_avg;	// 1秒(以上)平均
	u32 vel;		// 風速[m/s]
} Anemometer;

void CalcAnemometer();

///////////////////////////////////////////////////////////////////////////////
// モーション検出情報
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

// グライダ状態
enum {
	GSTATE_STOP			= 0,
	GSTATE_TURN_L		= 0x1,
	GSTATE_TURN_R		= 0x2,
	GSTATE_STRAIGHT		= 0x3,
	GSTATE_TURN_MASK	= 0x3, // MASK
	GSTATE_CENTERING	= 0x4,
	GSTATE_SPIRAL		= 0x8,
	GSTATE_STALL		= 0x10,
	GSTATE_CENTERING_X	= 0x20, // センタリング方向フラグ(左回りの時1)
	// ステートは水平回転のみ計算。縦回転のステートは省略…
	GSTATE_ROTATE		= GSTATE_CENTERING | GSTATE_SPIRAL,
	GSTATE_ROLL			= 0x00000100, // (実際にはヨーも検出…)
	GSTATE_ROLL_MASK	= 0x0000ff00, // MASK
	GSTATE_ROLLTOP_L	= 0x00010000,
	GSTATE_ROLLTOP_X	= 0x00020000, // GSTATE_ROLLTOP入れ替えフラグ
	GSTATE_PITCH		= 0x00100000,
	GSTATE_PITCH_MASK	= 0x0ff00000, // MASK
	GSTATE_PITCHTOP_D	= 0x10000000,
	GSTATE_PITCHTOP_X	= 0x20000000, // GSTATE_PITCHTOP入れ替えフラグ
};

#define CETERING_DETECT (ANG64K(300)) // センタリングと認識する旋回量

///////////////////////////////////////////////////////////////////////////////
// ウェイポイント、ルート
///////////////////////////////////////////////////////////////////////////////
#define MAX_WPT			1000
#define MAX_ROUTE		20

#define WPT_NAMESIZE	10
#define ROUTE_NAMESIZE	14
// ウェイポイント(20 byte)
typedef struct {
	u8	name[WPT_NAMESIZE];	// ウェイポイント名
	s16	alt;		// 高度(m)
    s32	lat;		// 緯度(°n)
    s32	lon;		// 経度(°n)
} Wpt;

typedef struct {
	// ヘッダ情報
	u32 magic;		// Route混合のため未使用
	u32 size;		// Route混合のため未使用
	u32 w_count;	// Route混合のため未使用
	u32 rfu;		// Route混合のため未使用

	Wpt wpt[MAX_WPT];
	s32 wpt_count;
	s32 def_ld;
} WptList;

typedef struct{
	u16 wpt;
	u16 cyl; // シリンダサイズを各パイロンに設定できるよう追加
} Pylon;

// ルートデータ(416 byte)
#define MAX_ROUTE_PT 99
typedef struct {
	u8  name[ROUTE_NAMESIZE];	// ルート名
	u16	count;					// ウェイポイント数
	u32 dist;					// 総距離[m]
	Pylon py[MAX_ROUTE_PT];// 0:WptID, 1:Cyl.
} Route;

typedef struct {
	// ヘッダ情報
	u32 magic;		// FLBL_ROUTE_MAGICであればルート情報
	u32 size;		// Route/Wpt合計サイズ
	u32 w_count;	// 書き換え回数。Flashの書き換え回数チェック用
	u32 rfu;		// リザーブ。チェックサムにでも使う?

	// ルート情報
	Route route[MAX_ROUTE];
	s32 route_count;
} RouteList;


// タスクログ保存用
typedef struct { // メイン情報
	u32 dtime;	// 到着時刻
	u32 trip;	// トリップメータ[m]
	s32 lat;	// 到着緯度
	s32 lon;	// 到着経度
	s16 alt;	// [m] 到着高度

	u16	cyl;	// デフォルトシリンダ半径 or パイロンゲット時のシリンダ半径
} TaskLog1; // 20 byte

typedef struct { // サブ情報。空き領域があれば記録する
	u8	wpt_name[WPT_NAMESIZE];	// ウェイポイント名
	s16 wpt_alt;	// [m] ウェイポイント高度
} TaskLog2; // 12 byte

// MAX:2044
typedef struct {
	u32 magic;		// ログ用マジック
	u32 log_time;	// ログ用時間
	u32 rt_dist;	// ルート距離(変更チェック用)
	u8	rt_count;	// ルート数(変更チェック用)
	s8  pre_pylonX;	// プレパイロン設定(保存用)
	u8  rt_name[ROUTE_NAMESIZE];	// ルート名
	u8  update_mark;
	u16 start_time;	// 保存用
	union {
		TaskLog1 tl1[MAX_ROUTE_PT + 1]; // +1=takeoff	//	2000 byte
#define TLD_HEAD	(3 * 4 + 4 + ROUTE_NAMESIZE + 1 + 1)
#define TL2MAX	((2048 - TLD_HEAD - 4) / sizeof(TaskLog2))
		TaskLog2 tl2[TL2MAX]; // 可能な限りウェイポイント情報を埋める(62WPT以下なら全て入る)
	};
} TaskLogData; // 2032 byte -> 2044 byte

typedef struct {
	u32 route_id;
	u32 pos;
	u32 dist[MAX_ROUTE];
} GoalDist;

void InitTaskLog(u32 keepTO);
void UpdateTaskLog(s32 n);
void UpdateAutoTargetLog(); // オートターゲット用タスクログ
TaskLog2* GetTL2Addr(TaskLogData* tld, u32 n);

u32 SaveFLog(u32 pos, void* addr, u32 size, u32 msg_flag);

EStatus CalcPvtExt(PvtX* px);
EStatus CalcPvtGarmin(const D800_Pvt_Data_Type* pvt, PvtX* px);
EStatus CalcPvtNMEA(PvtX* px);
void    CalcDist(s32 lat0, s32 lon0, s32 lat1, s32 lon1, u32* len, u32* houi);
void    CalcWptDist(const Wpt* w0, const Wpt* w1, u32* len, u32* houi);


///////////////////////////////////////////////////////////////////////////////
// コンフィグ設定情報
///////////////////////////////////////////////////////////////////////////////
// タスク設定等 (Flash保存対象)
typedef struct {
	// ヘッダ情報
	u32 magic;		// FLBL_TASK_MAGICであればタスク設定情報
	u32 size;		// sizeof(TaskConfig) - 16。 一応サイズもチェックする
	u32 w_count;	// 書き換え回数。Flashの書き換え回数チェック用
	u32 rfu;		// リザーブ。チェックサムにでも使う?

	// GPS情報
	s32 tzone_m;	// タイムゾーン[分] (JST: +9 * 60 = +540)
	s32 n_calib;	// 北補正 (デフォルトは: -007°)
	s32 stop_dir;	// 速度がこれ以下のときは、angleを更新しない
	u32 alt_type;	// 0: 平均海面高度, 1: WGS84楕円体高度
	u32 calc_type;	// 0:オート、 1: 簡易 2:高精度
	s32 tally_clr;	// 0:統計情報は保持、1:サスペンド時にクリア、2:スタート時にクリア
	u32 vario_mode;	// バリオモード(0:無効、1:有効)
	u32	vario_to;	// TO前はバリオを鳴らさない
	s32 vario_up;	// バリオ上昇閾値
	s32 vario_down;	// バリオ下降閾値
	s32 waas_flag;	// WAASモード(表示のみ)
	u32 gps_warn;	// GPS警告
	u32 nmea_up;	// NMEAの上昇率計算方法
	s32 rfu0[6];

	// タスク情報
	u32 task_mode;	// タスクタイプ(0:フリーフライト、1:スピードレース、2:ゴールレース)
	s32	route;		// ルート番号
	u32 sector;		// セクタサイズ(m)
	u32	cylinder;	// シリンダ半径(m)
	u32 rfu1;		// スタートシリンダ(m) 0xffffはcylinderを使う
	u32 goal_type;	// ゴールタイプ
	u32 start_type;	// スタートタイプ(0:フリースタート、1:セクターイン、2:セクターアウト)
	u32 start_time;	// スタート時間(分) 13:30→ 13*60+30 = 810。ffffは無効
	u32 close_time;	// クローズ時間(分) 13:30→ 13*60+30 = 810。ffffは無効
	u32 pre_pylon;	// プレパイロン数
	u32 cyl_end;	// スタート前のシリンダ進入でシリンダモードから抜ける
	u32 skip_TO;	// 最初のTOをスキップ
	s32 rfu2[1];

	// オートターゲット
	u32 at_mode;	// オートターゲットモード
	u32 at_min;		// 最小距離
	u32 at_max;		// 最大距離
	u32 at_recheck;	// 再検索距離

	// ナビ設定
	s32 view_mode;	// ビューモード
	s32 near_beep;	// 距離ビープ(シリンダサイズ+α)
	s32 auto_lock;	// 自動キーロック
	s32 avg_type;	// L/D計算方法
	s32 ar_max;		// パイロンアローMAX[m]
	s32 ar_min;		// パイロンアローMIN[m]
	s32 self_r;		// 自機表示半径
	u32 spd_unit;	// 対地速度単位
	s32 my_ldK;		// 機体のL/D    (レース用)
	s32 my_down;	// 機体の沈下率 (レース用)
	s32 my_speed;	// 機体の速度   (風速用)
	s32 spiral_spd;	// スパイラル速度
	s32 stall_spd;	// ストール速度
	s32 pitch_diff;	// ピッチング速度差
	s32 pitch_freq; // ピッチング最大周期
	s32 roll_diff;	// ロール角度差
	s32 roll_freq;	// ロール最大周期
	s32 start_spd;	// スタートスピード
	s32 pylon_type;	// パイロン表示タイプ
	s32 initial_pos; // イニシャル位置
	u32 vol_key;	// キーボリューム
	u32 vol_vario;	// バリオボリューム
	u32 vm_enable;	// 音声の有効/無効
	u32 vm_wait;	// 音声モード テイクオフまえの音声
	u32 vm_normal;	// 直進中の音声
	u32 vm_stop;	// 停止中の音声
	u32 vm_center;	// センタリング中の音声
	s32 view_alt;	// 拡張アルチ表示
	s32 anemo_unit;	// 待機速度単位
	u32 vm_near;	// ニアパイロンの音声
	u32 vm_interval;// 音声の間隔
	u32 start_alarm;// スタートアラーム
	u32 sp_prio;	// SPモード優先度

	s32 rfu3[3];

	// その他
	s32 auto_off;	// 自動電源OFF(分)
	s32 time_alarm;	// 時報モード
	s32 wind_cmd;	// 風速測定コマンド
	s32 glider_cmd;	// グライダ情報測定コマンド
	s32 thermal_cmd;// サーマルモードコマンド
	s32 wind_update;// グライダ情報の自動書き換えフラグ
	s32 bench_update;// グライダ情報の自動書き換えフラグ
	s32 stable_angle;// 角度変化マージン
	s32 stable_speed;// 速度変化マージン
	s32 init_wait;	 // 初期ウェイト
	s32 wait_timeout;// タイムアウト待ち
	s32 comp_timeout;// コンプリート後の表示時間
	s32 palette_conf;// パレット設定
	s32 keep_range;	 // リフトでもシンクでも内上昇率

	// 風速計
	s32 anemo_coef;	// 風速係数
	s32 cyl_cmd;	// シリンダマップコマンド
	u32 cyl_near;	// ニアシリンダ
	s32 rfu4[5];

	s32 locus_smp;	// 軌跡サンプリング時間
	s32 locus_r;	// 軌跡表示半径[m]
	s32 locus_cnt;	// 軌跡数(MAX 120)
	s32 locus_up;	// 上レンジ
	s32 locus_down;	// 下レンジ
	s32 locus_range;// カラーレンジ
	u32 locus_pal;	// 軌跡カラーモード

	// Bluetooth
#define BT_ADDR_LEN 12	// キャラクタで格納
#define BT_PIN_LEN	12	// 最大12文字(仕様はMax16だが…)
	u32 bt_mode;		// Bluetooth接続モード
	u8	bt_addr[BT_ADDR_LEN];// 接続先アドレス
	u8	bt_pin [BT_PIN_LEN]; // 接続PINコード

	// トラックロガー
	u32 log_enable;	// トラックログ有効/無効
	u32 rsv[2];
	u32 log_prec;	// 保存精度
	u32 log_intvl;	// ログ保存間隔
	u32 log_overwrite;// 上書きモード
	u32 log_debug;

	u32 flog_as;	//フライトログのオートセーブ設定
	u32 tlog_as;	//トラックログのオートセーブ設定
	s32 rfu6[6];

} TaskConfig;


///////////////////////////////////////////////////////////////////////////////
// 統計情報
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

#define VARIO_HIST_RANGE_MIN	100		// Max ±2.5m/s
#define VARIO_HIST_RANGE_MAX	2000	// Max ±80m/s
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
	u32 magic;		// ログ用マジック
	u32 log_time;	// ログ最終時刻(last_secに似ているが、こちらは2D以下でもカウント)

	u32 takeoff_time;	// ログ開始時刻
	s32 start_lat;	// 統計開始位置
	s32 start_lon;	// 統計開始位置

	u32 count;		// 統計カウンタ
	u64 sum_v;		// 3D速度合計
	u64 trip_mm;	// トリップメータ
	s32 last_lat;	// 最後の位置(2D以上)
	s32 last_lon;	// 最後の位置(2D以上)
	u32 last_sec;	// 最後の時間(2D以上)

	u32 sum_uv;		// 単位ベクトルカウンタ
	s32 sum_nv;		// 単位ベクトル北合計(風速用) 1024倍
	s32 sum_ev;		// 単位ベクトル東合計(風速用) 1024倍
	s32 sum_nsv;	// 単位ベクトル北絶対値合計(風速用) 1024倍
	s32 sum_ewv;	// 単位ベクトル北絶対値合計(風速用) 1024倍
	u32 sum_gain;	// トータルゲイン

	s32 max_v;		// 最高速度(3D)
	s32 max_up;		// 最高上昇
	s32 min_up;		// 最高沈下
	s32 max_alt;	// 最高高度
	s32 min_alt;	// 最低高度
	s32 max_G;		// 最高加速度
	s32 min_G;		// 最低加速度

	s32 s_count;	// 直進中カウンタ
	s32 w_count;	// 停止中カウンタ
	s32 s_hv;		// 直進中速度合計(直進中平均速度用)
	s32 s_up;		// 直進中up合計(直進中L/D平均用)
	s32 turn_r;		// 右旋回回数
	s32 turn_l;		// 左旋回回数
	//	s32 pitch;		// ピッチング回数
	u16 pitch_s;	// 直進中ピッチング回数(MAX65535になるが十分?)
	u16 pitch_r;	// センタリング中ピッチング回数
	s32 roll;		// ローリング回数
	s32 center_sum;	// センタリング角度合計
	s32 spiral_sum;	// スパイラル角度合計
	s32 stall_sec;	// ストール時間

	s32 keep_range;

	s32 soaring_cnt[4];	// ソアリング統計
	s32 soaring_sum[4];	// ソアリング統計
	s32 sinking_cnt[4];	// ソアリング統計
	s32 sinking_sum[4];	// ソアリング統計
	s32 keeping_cnt[4];	// ソアリング統計
	s32 vario_cnt[4];	// ソアリング統計
	s32 vario_sum[4];	// ソアリング統計

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
// 軌跡表示用
///////////////////////////////////////////////////////////////////////////////
#define MAX_LOCUS	1800 // 2sサンプリングで1時間
#define LOCUS_LEVEL 10	 // 10段階色分け
#define LOCUS_BSFT	12	 // 100m精度 (4000km以内なら1回、地球の裏に瞬間ワープしても5回でアジャスト可)

// 軌跡情報
typedef struct {
	// lat/lonはメモリ節約のため差分で保持 (alt/upはグラフでも使うので絶対値を格納)
	s16 lat_d;
	s16 lon_d;
	s16 alt_x; // ビット0はLatLonの範囲外マークに使う
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

// スプライトがチラつかないように、水平スプライト数を抑える
#define MAX_SPLITE	100 // 1ラインの最大スプライト数
#define SPLITE_BASE	12	// VCount切り替えスプライト用タイル番号
#define SPLITE_END	(128 - SPLITE_BASE)	// 最大スプライト表示数
// 808byte
typedef struct {
	u32 buf[MAX_SPLITE * 2];
	s16 pre_count;
	s16 cur_count;
	s16 full_check;
} SpliteHBuf;

// HSync割り込みでスプライトを多重表示させる
#define SPLITE_VCOUNT 20
// 18576byte
typedef struct {
	SpliteHBuf hBuf[SPLITE_VCOUNT]; // 808 * 20 =16160
	s32 flag;
	s32 cur_index;

	// nearは視覚効果を向上させるための分配計算用テーブルで、水平スプライト数が規定値を
	// 超えた場合にcur_fullがセットされ次回からnearバッファでの分配チェックが有効になる。
	// ただし、cur_fullがセットされてもメニュー表示中は背景のスプライトを真面目に表示す
	// る必要はないため、分配スプライトは使用しない。
	// ⇒これを利用し、メニューからFLogチェックする場合は、near配列のメモリを借用する！
	s32 pre_full;
	s32 cur_full;
	u8 near[240 / 4][160 / 4];		// 60 * 40 = 2400byte
} SpliteTable;


///////////////////////////////////////////////////////////////////////////////
// トラックログ/圧縮
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
	u32 index;			// ログインデックス(現在のブロックの書き込みオフセット
	u32 block;			// 現在のブロック(64KB単位)
	u32 pkt_counter;	// パケットカウンタ(今日のブートアップ後)
	u32 blk_counter;	// ブロックカウンタ
	u32 abs_flag;		// 強制絶対値パケットフラグ
	u32 full;			// フルフラグ
	u32 pre_pvtc;		// 更新検出用

	// デバッグ用情報
	u32 t_count[16];	// PID別カウンタ
	u32 trk_counter;	// 総トラック数
	u32 err;			// 最後の書き込みエラー値
	u32 drop;			// 書き込み失敗回数

	// 圧縮ログ
	s32 pre[4];			// 直前値(Lat/Lon/Sec/Alt)
	s32 dif[4];			// 二階微分値(Lat/Lon/Sec/Alt)
	u32 pre_prec;		// ログ精度変更検出
	u32 repeat;			// リピートカウンタ
	u32 opt;			// オプションフラグ
	s32 intvl_ctr;		// インターバルカウンタ

	// 現在の総トラック数カウント(PCアップロード用)
	u32 tc_state;	// 総トラック数計算ステート
	u32 tc_lastadd;	// 最後にトラック追加したVBC
	u32 tc_total;	// 総トラック数
	u32 tc_tpos;	// 現在の計算中の位置。計算完了後は先頭ブロックを示す。
} TrackLogParam;

enum {
	TC_NOTREADY,	// 計算前
	TC_CALCULATING,	// 計算中
	TC_COMPLETE,	// 計算完了
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

// トラックログは64KB単位でヘッダ管理
#define BLOCK_SIZE			(64 * 1024) // トラックログ用
#define BLOCK_HEAD_SIZE		8
#define BLOCK_TRACK_SIZE	(BLOCK_SIZE - BLOCK_HEAD_SIZE)
#define BLOCK_TASK_SIZE		(8 * 1024)

typedef struct{
	u32 magic;
	s32 index; // ブロック走査で重要なのは上位の8bit(下位24bitのPacketCounterはどうでもいい)
	u8 val[BLOCK_TRACK_SIZE];
} TrackBlock;

#define CUR_BLOCK_OFFSET(tlp) (FI_TRACK_OFFSET + (tlp)->block * BLOCK_SIZE)

u32 UploadLog(); // 圧縮トラックログをPCへ転送

///////////////////////////////////////////////////////////////////////////////
// メニュー用
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

// 少ないメモリを有効活用するために、面倒だが構造体できっちりサイズを管理する
typedef u32 (*MenuProc)(u16 push);
typedef struct {
	u32 map;		// 選択のマップ番号
	u32 gx;			// テキストカーソルX
	u32 gy;			// テキストカーソルY
	u32 menuid;		// メニュー番号
	u32 menu_x0;	// メニューのX0
	u32 menu_y0;	// メニューのX0
	u32 menu_x1;	// メニューのX0
	u32 menu_y1;	// メニューのX0
	const u8* help_msg;	// ヘルプスクロール位置
	u32 help_scr;	// ヘルプスクロール位置
	u32 help_vbc;	// スクロール速度調整用
	MenuProc proc;	// メニュー関数
	u32 sel;		// メニュー選択位置
	u32 pvt_check;	// PVT更新チェック用
	s32 wpt_sel;	// ウェイポイント選択
	s32 route_sel;	// ルート選択
	s32 rtwpt_sel;	// ルートWPT選択
	s32 rtwpt_dsp;	// ルートWPTディスプレイ位置
	s32 rtwpt_state;	// ルートWPTディスプレイ位置
	s32 rtwpt_calc;	// 総距離計算カウンタ
	s32 rtwpt_sum;	// 総距離計算カウンタ
	s32 ar_count;	// オートリピートのカウンタ
	u32 ar_vbc;		// オートリピート用のVBC
	u16 ar_key;		// オートリピートのキー
	u32 mp_flag;	// 汎用フラグ
	u32 scr_pos;	// スクロール位置
	u32 disp_mode;	// 表示モード
	u32 boot_msg;	// ブートメッセージ消去用
	u32 name_table; // 名前入力テーブル
	u32 save_flag;	// セーブ用の変更フラグ
	u32 auto_off_cnt;//自動サスペンドカウンタ
	u32 resume_vbc;	// リジューム時刻
	u32 tips;		// tipsカウンタ
	u32 multiboot;	// マルチブートフラグ
	u32 cyl_input;	// シリンダ個別入力用
	u32 pre_menu;	// 表示更新用メニュー状態

	// CPU Load
	u32 load_test;
	u32 load_vbc;
	u32 last_load;
#define MAX_LOAD_LIST 16 // ≧ MyTask + 2!
	u32 load[MAX_LOAD_LIST];

	// グラフ用
	s32 graph_mode; // グラフモード(Alt/Vario)
	s32 graph_scale;// グラフスケール
	s32 graph_range;// バリオレンジ
	s32 graph_minmax;// グラフタイプ
	s32 min_alt; // グラフ用 最低高度
	s32 max_alt; // グラフ用 最高高度
	s32 cur_lat;	// Locus記録の最新緯度
	s32 cur_lon;	// Locus記録の最新経度

	// ナビ用
	u32 test_mode;	// テストモード
	u32 navi_update;// 更新フラグ
	u32 key_lock;	// キーロック
	u32 cur_point;	// 現在のポイント番号
	u32 sect_ang64;	// 次のセクタ角
	u32 sect_type;	// 次のセクタタイプ
	u32 cur_view;	// 現在のビューモード
	u32 cur_sect;	// 現在のセクタ表示
	s32 pre_pylon;	// 前回のパイロンゲット状況
	s32 pre_index;	// 前回のパイロン番号
	s32 pre_task;	// 前回のタスクモード
	Route* pre_route;// 前回のルート(タスクログ用)
	s32 sp_view;	// 専用ビューモード
	s32 pre_sp_view;// 前回の専用ビューモード状態
	s32 pre_sect;	// 前回のセクタ
	s32 sp_enter;	// 専用ビューモードに入ったときのコマンド
	u32 sp_state;	// 専用モードのステータス
	u32 sp_angle;	// 風速、ベンチマーク用の第一方位
	s32 sp_speed1;	// 第一水平速度
	s32 sp_up1;		// 第一垂直速度
	u32 freeflight;	// 自動検索禁止
	u32 near_pylon;	// 最短パイロン距離
	u32 cyl_dist;	// シリンダまでの距離
	u32 near_vbc;	// ニアサウンド用
	u32 pre_palette;// 直前のLocusパレット

	// 音声
	s32 voice_state;// 音声ステータス
	u32 voice_type;	// 音声タイプ
	u32 voice_chg;	// 変更可能フラグ
	u32 vbuf_idx;	// 再生箇所
	u32 voice_vbc;	// 息継ぎ
	s32 py_dir;		// パイロン方向(-180～+180 [°])
	s32 py_dis;		// パイロン距離[m]
	s32 py2_dir;	// 次次パイロン方向(-180～+180 [°])
	s32 py2_dis;	// 次次パイロン距離[m]

	// ニアWPT
	u32 nw_target;	// 最寄WPT
	u32 nw_search;	// WPT検索用
	u32 nw_s_dir;	// WPT検索方向
	u32 nw_cnd;		// WPT候補
	u32 nw_cnd_len;	// WPT候補距離
	u32 nw_tgt_len;	// 最寄WPT距離
	Wpt nw_start;	// 検索開始場所

	// トラックログ
	TrackLogParam tlog;

	// 統計情報(電源を入れてからの統計)
	Tally tally;
	u32* flog_addr;
	u32  flog_presave;
	u32  tlog_presave;
	u32	 flog_cache[FLOG_COUNT][2];	//メニュー表示に必要なヘッダ部分のキャッシュ用

	//ゴールまでの距離計算用
	GoalDist goal_dist;

	// GPS受信デバッグ
	u32 last_pvt_len;
	u32 last_wpt_len;
	u32 last_route_len;
	u8  last_pvt[256];
	u8  last_wpt[256];
	u8  last_route[256];

	// 自動ターゲット用
#define MAX_AUTOTARGET_CAND 32 // 最大ターゲット候補数。必ず 2^n にすること!
	u32 rand_val;
	u32 rand_init;

	u32 at_rollflag;
	u32 at_vbc;
	u32 atcand_count;
	u16 atcand[MAX_AUTOTARGET_CAND];

	// NMEA制御用
	s32 nmea_xalt;

	// Bluetooth制御用
	u32 pre_bt_mode;	// 直前のBluetooth接続モード
} MenuParam;


///////////////////////////////////////////////////////////////////////////////
// 入力テンプレート用
///////////////////////////////////////////////////////////////////////////////

// 数値入力用テンプレート
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

//時刻入力テンプレート
typedef struct {
	const u8* name;
	s32* val;
} TimeInputTemplate;
typedef struct {
	const TimeInputTemplate* t;
	s32 pos;
	s32 val;
} TimeInput;

// 列挙入力用テンプレート
typedef struct {
	u32 max;
	u32* val;
	const u8* names[1];
} EnumInputTemplate;

// 名前入力用テンプレート
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
	u8 val[32]; // 入力文字数はMAX31文字
} NameInput;

// 緯度経度入力用テンプレート
typedef struct {
	s32 latlon;
	s32* val;
} LatInputTemplate;
typedef struct {
	const LatInputTemplate* t;
	s32 pos;
	s32 val;
} LatInput;

// 入力用データ
typedef union {
	IntInput	i_int;
	TimeInput	i_time;
	NameInput	i_name;
	LatInput	i_lat;
} AnyInputParam;

// ウェイポイント入力用
typedef struct {
	u8	name[WPT_NAMESIZE];	// ウェイポイント名
	s32	alt;		// 高度(m)
    s32	lat;		// 緯度(°n)
    s32	lon;		// 経度(°n)
} WptInput;

// バリオもどき
typedef struct {
	s32	cur_value;		// 現在値
	s32 cur_mode;		// 現在のモード
	s32	vario_test;		// テスト用
	u32	vario_blink;	// 間隔
	u32	vario_count;	// vsyncカウンタ
	u32 vario_vbc;		// vbc
} Vario;


///////////////////////////////////////////////////////////////////////////////
// ナビ
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

// テスト動作用
s32  DummyPlay();
void DummyPlayUpdate();

void InitNearNavi();
void SetNaviStartPos();


///////////////////////////////////////////////////////////////////////////////
// File I/F (アプリは直接FlashやSCSDをアクセスせず、File IFを経由して制御する)
///////////////////////////////////////////////////////////////////////////////
enum {
	FLASH_SUCCESS = 0,

	FLASH_E_NO_CART			= 10,	// 認識可能なカートリッジなし
	FLASH_E_PARAM			= 11,	// 関数パラメータエラー
	FLASH_E_MAGIC			= 12,	// マジックエラー
	FLASH_E_SIZE			= 13,	// サイズエラー
	FLASH_E_OFFSET			= 14,	// オフセット指定エラー

	// トラックログ専用
	FLASH_E_LOG_NEWBLOCK	= 20,	// 次ブロック切り替え
	FLASH_E_LOG_FULL		= 21,	// ログいっぱい

	// JoyCarry系
	FLASH_E_ID				= 30,	// IDエラー
	FLASH_E_STATUS_TIMEOUT	= 31,	// ステータス取得タイムアウト
	FLASH_E_SR_VCC			= 32,	// VCCエラー
	FLASH_E_SR_PROTECT		= 33,	// プロテクトエラー
	FLASH_E_SR_WRITE		= 34,	// 書き込みエラー
	FLASH_E_SR_ERASE		= 35,	// 消去エラー
	FLASH_E_SR_CMD			= 36,	// コマンドエラー
	FLASH_E_NOT_FFFF		= 37,	// 消去エラー2
	FLASH_E_COMPARE			= 38,	// コンペアエラー

	// SCSD系を追加
	FLASH_E_NO_STORAGE		= 50,	// ストレージなし
	FLASH_E_NO_FILE			= 51,	// ParaNavi.datなし
	FLASH_E_MBR_ERROR		= 52,	// 未サポートのFAT
	FLASH_E_SECT_ERROR		= 53,	// 未サポートのセクタサイズ
	FLASH_E_CLST_ERROR		= 54,	// 未サポートのクラスタサイズ
	FLASH_E_DAT_SIZE		= 55,	// DATサイズが大きすぎる
	FLASH_E_CNUM_RANGE		= 56,	// クラスタ番号の異常

};

#define THEAD_SIZE			0x10
#define TDataSize(x)		(sizeof(x) - THEAD_SIZE)
#define FLBL_TASK_MAGIC		0x036b7354	// タスクマジック("Tsk?")
#define FLBL_ROUTE_MAGIC	0x01657452	// ルートマジック("Rte?")
#define FLBL_TRACK_MAGIC	0x016b7254	// トラックマジック("Trk?")
#define FLOG_MAGIC_FLIGHT	0x014c4c46	// フライトログマジック(FLL?)
#define FLOG_MAGIC_TASK		0x014c5354	// タスクログマジック  (TKL?)

#define FI_CONFIG_OFFSET	0x00000000	// 8KB
#define FI_FLOG_OFFSET		0x00002000	// 8KB
#define FI_ROUTE_OFFSET		0x00004000	// 32KB
//                          0x0000C000～16KBはリザーブ
#define FI_TRACK_OFFSET		0x00010000	// 64KB以上

#define ROUTE_WPT_SIZE (1024 * 30 - 0x100)
#define BOOTBIN_SIZE 0x2000 //有効プログラム領域。8KB

#define CARTCAP_WRITEABLE	1	// 書き換え可能カートリッジ
#define CARTCAP_DIRECTMAP	2	// 直読み可能カートリッジ
#define CARTCAP_DUPLICATE	4	// 複製用データあり
#define CARTCAP_SECTWRITE	8	// セクタ単位で書き換え

// 簡略化用マクロ
#define CART_IS_WRITEABLE() (IW->cw.cap & CARTCAP_WRITEABLE)
#define CART_IS_DIRECTMAP() (IW->cw.cap & CARTCAP_DIRECTMAP)
#define CART_IS_DUPLICATE() (IW->cw.cap & CARTCAP_DUPLICATE)
#define CART_IS_SECTWRITE() (IW->cw.cap & CARTCAP_SECTWRITE)

// Cでクラス継承が使えれば良いのだが…
typedef struct {
	s32 (*InitCart)();
	const u32* (*ReadDirect)(u32 offset);// 可能であればメモリコピーしない高速Read(512byteまで保証)
	s32 (*ReadData) (u32 offset,       void* dst, s32 size, u32 magic); // magicが一致している場合のみreadする
	s32 (*WriteData)(u32 offset, const void* src, s32 size);
	s32 (*EraseBlock)(u32 offset, s32 size, u32 mode);// ブロック消去
	s32 (*AppendTLog)(u8* buf, u32 size);	// トラックログ追加書き込み専用
	s32 (*Flush)();							// バッファフラッシュ
	const u8* (*GetCodeAddr)(u32 type);		// 複製用のプログラムアドレス等を返す
} CartIF;
// ※ReadDirect/ReadData/WriteData/EraseBlockのOffsetはセクタ境界を指定すること!

#define ERASE_FF		0	// 通常消去(FF埋め)
#define ERASE_MAGIC		1	// 先頭マジックのみ消去する高速イレース
#define ERASE_CLEAR		2	// 個人情報消去用 (TODO 米国国防総省NSA規格とか?)
#define ERASE_TRACK		3	// トラックログ高速クリア用

#define CODE_TYPE_BOOT	0
#define CODE_TYPE_NAVI	1
#define CODE_TYPE_VOICE	2
#define CODE_TYPE_TRACK	3
#define CODE_TYPE_TASK	4
#define CODE_TYPE_FLOG	5
#define CODE_TYPE_RTWPT	6

#define SECT_SIZE 512 // とりあえず512セクタのみサポート

// エミュレータ用 /////////////////////////////////////////////////////////////
const u8* Emul_GetCodeAddr(u32 type);

typedef struct {
	union { // セクタキャッシュ
		u32 scache32[SECT_SIZE / 4]; // 使いやすいようにいろいろなサイズで…
		u16 scache16[SECT_SIZE / 2]; // 使いやすいようにいろいろなサイズで…
		u8  scache8 [SECT_SIZE    ]; // 使いやすいようにいろいろなサイズで…
	};
} EmulWork;

// ジョイキャリーカートリッジ /////////////////////////////////////////////////
typedef struct {
	u32 pre_unlock;
} JoyCWork;

// SCSD ///////////////////////////////////////////////////////////////////////
typedef struct {
	// FAT情報
	u32 sect_size;
	u32 sect_shift;
	u32 clst_size;		//
	u32 clst_shift;		// クラスタ計算用のシフト値
	u32 clst_limit;
	u32 fat_start;		// FATセクタ開始位置
	u32 dir_start;		// ルートDirセクタ開始位置
	u32 dat_start;		// データセクタ開始位置
	u32 datf_min;		// DATファイルのレンジチェック用
	u32 datf_max;		// DATファイルのレンジチェック用

	// ファイル情報
	u32 file_head;
	u32 file_size;

	u32 cache_sect; // キャッシュセクタ (0 は無効)
	u32 tlog_offset;// トラックログキャッシュオフセット
	u32 tlog_dirty;	// トラックログキャッシュダーティビット
	u32 tlog_vbc;	// 最低更新時間管理

	union { // セクタキャッシュ
		u32 scache32[SECT_SIZE / 4]; // 使いやすいようにいろいろなサイズで…
		u16 scache16[SECT_SIZE / 2]; // 使いやすいようにいろいろなサイズで…
		u8  scache8 [SECT_SIZE    ]; // 使いやすいようにいろいろなサイズで…
	};
	union { // トラックログ書き込みバッファ
		u32 tlog_cache32[SECT_SIZE / 4]; // 使いやすいようにいろいろなサイズで…
		u16 tlog_cache16[SECT_SIZE / 2]; // 使いやすいようにいろいろなサイズで…
		u8  tlog_cache8 [SECT_SIZE    ]; // 使いやすいようにいろいろなサイズで…
	};
} SCSDWork;

// 共通 ///////////////////////////////////////////////////////////////////////
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


// 今のところ、使えるのは以下の4種
extern const CartIF CIF_NONE;
extern const CartIF CIF_EMULATOR;
extern const CartIF CIF_JOYCARRY;
extern const CartIF CIF_SUPERCARD_SD;

///////////////////////////////////////////////////////////////////////////////
// Bluetooth用
#define MAX_ATC_RESPONSE 100
enum {
	ATCRES_NONE,
	ATCRES_OK,
	ATCRES_ERROR,
	ATCRES_CONNECT,
	ATCRES_DISCONNECT,
};

#define MAX_INQ_DEV 9 // 最大9デバイスまで検索
typedef struct {
	u32 inq_count;
	u32 last_mode;
	u32 baudrate_err;
	u8	inq_list[MAX_INQ_DEV][MAX_ATC_RESPONSE + 1]; // スキャンしたデバイス情報を格納
} ATCInfo;

u32 IsBluetoothAddr(const u8* p);

///////////////////////////////////////////////////////////////////////////////
// RAM BLOCK
///////////////////////////////////////////////////////////////////////////////
// よく使う変数はIWRAM上に配置。IWRAMは32KBしかないので、きっちり管理することにした(面倒だけど…)
typedef struct {
	u8	global_reserve	[0x1000 - 0x0000];	// グローバル変数用のリザーブ(殆ど使っていないがデバッグ用に4KB確保…)

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
	vu16 key_chatter;//	[0x1804 - 0x1802]	// チャタリング対策
	u8	 key_rfu		[0x1810 - 0x1804];	// 12 byte

	// Garmin DataLink stack(272 byte)
	u32	dl_drop;	//	[0x1814 - 0x1810]
	u32	dl_drop_pkt;//	[0x1818 - 0x1814]
	u32	dl_timeout;	//	[0x181C - 0x1818] 
	u32	dl_fsm;		//	[0x1820 - 0x181C]
	u32	dl_size;	//	[0x1824 - 0x1820]
	u8	dl_loghead		[0x1828 - 0x1824];	//	4byte デバッグログ用
	u8	dl_buf			[0x1928 - 0x1828];	//	256 byte
	u32	dl_nmea_chk;//	[0x192c - 0x1928]; // チェックサム格納用
	u32	dl_nmea_idx;//	[0x1930 - 0x192c]; // チェックサムIndex
	u32	dl_nmea_ack;// 	[0x1934 - 0x1930]; // 最終ACK-ID
	u8 	dl_rfu			[0x1940 - 0x1934];

	// GPSデータ
	PvtX px;			// 最新PVT
	PvtX px2;			// 1つ前のPVT or NMEAの中間データ用バッファ
	u32  nmea_log[16];	// NMEA受信ログ
	u32  nmea_fix;		// NMEAセンテンス別受信フラグ
	u32  nmea_wgs84;	// WGS84モード切替済みフラグ
	D800_Pvt_Data_Type pvt_raw; // デバッグ用
	PvtRec pr;
	u8 	px_rfu			[0x1E00 - 0x1940 - sizeof(PvtX) * 2 - sizeof(u32) * 18 - sizeof(D800_Pvt_Data_Type) - sizeof(PvtRec)];

	// タスク設定 保存対象(512 byte)
	TaskConfig tc;
	u8	tc_rfu			[0x2000 - 0x1E00 - sizeof(TaskConfig)];

	// バリオモドキ用
	Vario vm;			
	u8	vm_rfu			[0x2020 - 0x2000 - sizeof(Vario)];

	// WPT入力用
	WptInput wpt_input;
	u8 wpt_rfu			[0x2100 - 0x2020 - sizeof(WptInput)];
	u16 wpt_sort[MAX_WPT];
	u32 wpt_sort_type;
#define SORTED_MARK 0x1000
#define GetSortMark() (IW->wpt_sort_type & SORTED_MARK)
#define GetSortType() (IW->wpt_sort_type & (SORTED_MARK - 1))
	u8 sort_rfu			[0x2900 - 0x2100 - MAX_WPT * sizeof(u16) - 4];

	// ルート入力用
	Route route_input;
	u8 ri_rfu			[0x2B00 - 0x2900 - sizeof(Route)];

	// プリミティブ入力用
	AnyInputParam aip;
	u8 aip_rfu			[0x2C00 - 0x2B00 - sizeof(AnyInputParam)];

	// GPS情報(512 byte)
	GarminInfo gi;
	u8 gi_rfu			[0x2E00 - 0x2C00 - sizeof(GarminInfo)];

	// タスクログ(2KB)
	TaskLogData	task_log;
	u8 tl_rfu			[0x3600 - 0x2E00 - sizeof(TaskLogData)];

	// 音声バッファ
#define VBUF_SIZE 126
	u32 voice_ctr;	//  [0x2E04 - 0x2E00]
	u16 voice_buf[VBUF_SIZE];//[0x2F00 - 0x2E04]

	// 風速計
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
	u8  isr_cpy			[0x6FFC - 0x6000];	//　割り込みはIW上でARM実行
	const CartIF* cif;						//  Cart判別4 byte(Initで初期化されないようココに置く)
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

#define VOICE_BUF_SIZE (8 * 1024) // 8KBで十分？
#define VOICE_FIFO_MAX (VOICE_BUF_SIZE / 16)

// 以下、RAMで使用する領域。後ろから積む。
#define WPT_ADDRESS		(EX_WRAM_END - 20 * 1024)				// 最後の20KB  20 * 1000 + 4(19.5KB) 保存時と同じフォーマット
#define ROUTE_ADDRESS	(EX_WRAM_END - 30 * 1024)				// 最後の10KB 416 *   20 + 4( 8.1KB) 保存時と同じフォーマット
#define VOICE_ADDRESS	(ROUTE_ADDRESS - VOICE_BUF_SIZE)		// 8KB (Flash書き換え時の音飛びを防ぐためRAMにコピーして鳴らす)
#define LOCUS_ADDRESS	(VOICE_ADDRESS - sizeof(Locus))			// 14.1KB 2秒間隔で1時間記録
#define SPLITE_ADDRESS	(LOCUS_ADDRESS - sizeof(SpliteTable))	// 18.1KB 多重スプライト用DMAバッファ
#define FLOG_PTR		((u32*)SPLITE->near)					// Flog(2KB)はメニュー表示中しか使わないのでスプライト分配テーブル(2.4KB)領域を流用する
#define ATC_PTR			((ATCInfo*)SPLITE->near)				// ATC(1KB)はメニュー表示中しか使わないのでスプライト分配テーブル(2.4KB)領域を流用する
// マルチブートに対応するためにはプログラムを256KB以下に抑える必要があるが、上記データを
// RAMに確保するため、プログラムは実質256KB-78KB=176KBに制限されることに注意！
// 足りなくなったらプログラムを圧縮することを考える必要あり。

#define PROGRAM_LIMIT (SPLITE_ADDRESS)
extern const u8 __iwram_overlay_lma; // PROGRAM終端 (&__iwram_overlay_lma < PROGRAM_LIMIT であること)

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
// バリオ
///////////////////////////////////////////////////////////////////////////////
#define VARIO_TEST_DISABLE 0x10000000


///////////////////////////////////////////////////////////////////////////////
// タスク
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

///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// エミュレータ用とカートリッジ無し用のファイルI/Fをまとめた。

///////////////////////////////////////////////////////////////////////////////
// 定数
///////////////////////////////////////////////////////////////////////////////

// エミュレータでのテストでは64KB SRAMエリアを保存領域として使用する
// !! コンフィグとトラックログでSRAMを共有している。後書き優先を意識してテストすること !!
#define EMUL_CONFIG_ADDR	0x0E000000
#define EMUL_FLOG_ADDR		0x0E002000
#define EMUL_RTWPT_ADDR		0x0E004000
#define EMUL_LOG_ADDR		0x0E000000 // EMUL_CONFIG_ADDRと同じ。トラックログ保存で設定は消える!
#define EMUL_BLOCK_NUM		1

///////////////////////////////////////////////////////////////////////////////
// ROMに格納された音声データの検索用
const u32 VOICE_ADDR_FLASH_CAND[] = { // Flash cart
	((u32)&__iwram_overlay_lma) + (0x08000000 - 0x02000000), // FLASH Cart ROM結合
	((u32)&__iwram_overlay_lma) + (0x08000000 - 0x02000000 + BOOTBIN_SIZE), // Boot.binの後ろ
	0
};
void SearchRomVoice(){
	CartWork* cw  = &IW->cw;
	const u32* p = VOICE_ADDR_FLASH_CAND;
	for(; *p ; ++p){
		if(*(u32*)*p == WAV_MAGIC){
			cw->phy_voice = *p; // 有効データを検出
			break;
		}
	}
}

// 標準memcpyはinlineで最適化されるため、SRAM用に確実に1byteアクセスする関数を用意
void memcpy8(vu8* dst, const u8* src, s32 size){
	while(size--) *dst++ = *src++;
}
void memset8(vu8* dst, u8 val, s32 size){
	while(size--) *dst++ = val;
}

///////////////////////////////////////////////////////////////////////////////
// File I/F
///////////////////////////////////////////////////////////////////////////////
static s32 Emul_InitCart(){
	CartWork* cw = &IW->cw;

	// 音声アドレスを設定
	SearchRomVoice();

	// トラックログの有効ブロックを検索
	cw->tlog_block	= EMUL_BLOCK_NUM;
	cw->phy_tlog	= EMUL_LOG_ADDR;

	// ワーク情報設定
	cw->phy_navi	= ROM_ADDRESS;
	cw->phy_config	= EMUL_CONFIG_ADDR;
	cw->phy_flog	= EMUL_FLOG_ADDR;
	cw->phy_rtwpt	= EMUL_RTWPT_ADDR;
	cw->cap			= CARTCAP_WRITEABLE | CARTCAP_DUPLICATE | CARTCAP_SECTWRITE; // 8bit制限があるためDirectMapは使えない
	return 0;
}

const u8* Emul_ReadDirectX(u32 offset){
	if(offset < FI_TRACK_OFFSET){
		return (u8*)(IW->cw.phy_config + offset); // config / flog / (route)
	}
	if(offset < FI_TRACK_OFFSET + IW->cw.tlog_block * BLOCK_SIZE){
		return (u8*)(IW->cw.phy_tlog + (offset - FI_TRACK_OFFSET)); // tracklog
	}
	return 0; // 無効オフセット
}
const u32* Emul_ReadDirectSect(u32 offset){
	const u8* p = Emul_ReadDirectX(offset);
	if(!p) return 0;

	EmulWork* emw = &IW->cw.emul_work;
	memcpy8(emw->scache8, p, SECT_SIZE);
	return emw->scache32;
}

// ヘッダが一致している場合のみreadする
static s32 Emul_ReadData(u32 offset, void* dst, s32 size, u32 magic){
	// デバッグ用チェック
	if(0 < magic && magic < 10){ // デバッグ用コード
		memcpy8(dst, (u8*)(offset + EMUL_LOG_ADDR), size); // メモリ直読み
		return 0;
	}

	const u8* src = Emul_ReadDirectX(offset);
	if(!src) return FLASH_E_OFFSET;
	if(magic && GetLong(src) != magic) return FLASH_E_MAGIC; // 0マジックは使わない
	memcpy8(dst, src, size);
	return 0;
}

static s32 Emul_WriteData(u32 offset, const void* src, s32 size){
	u8* dst = (u8*)Emul_ReadDirectX(offset); // アドレス取得用
	if(!dst) return FLASH_E_OFFSET;
	memcpy8(dst, src, size);
	return 0;
}

static s32 Emul_EraseBlock(u32 offset, s32 size, u32 mode){
	u8* dst = (u8*)Emul_ReadDirectX(offset); // アドレス取得用
	if(!dst) return FLASH_E_OFFSET;

	if(mode == ERASE_MAGIC) *dst &= 00;				// マジックのみ消去の特殊モード
	else					memset8(dst, -1,size);	// その他は全部共通…
	return 0;
}

static s32 Emul_AppendTLog(u8* buf, u32 size){
	TrackLogParam* tlp = &IW->mp.tlog;
	u8* dst = (u8*)Emul_ReadDirectX(CUR_BLOCK_OFFSET(tlp));
	if(!dst) return FLASH_E_OFFSET;
	memcpy8(dst + tlp->index, buf, size);
	tlp->index += size;
	return 0;
}

// フラッシュは何もしない
static s32 Emul_Flush(){
	return 0;
}

// 複製用のプログラムアドレスを返す (メンバ変数に入れるようにしたので、Emul以外も汎用で使う)
const u8* Emul_GetCodeAddr(u32 type){
	switch(type){
	case CODE_TYPE_BOOT:	return (u8*)IW->cw.phy_boot;
	case CODE_TYPE_NAVI:	return (u8*)IW->cw.phy_navi;
	case CODE_TYPE_VOICE:	return (u8*)IW->cw.phy_voice;
	case CODE_TYPE_TRACK:	return (u8*)IW->cw.phy_tlog;
	case CODE_TYPE_TASK:	return (u8*)IW->cw.phy_config;
	case CODE_TYPE_FLOG:	return (u8*)IW->cw.phy_flog;
	}
	return 0;
}

const CartIF CIF_EMULATOR = {
	Emul_InitCart,
	Emul_ReadDirectSect,
	Emul_ReadData,
	Emul_WriteData,
	Emul_EraseBlock,
	Emul_AppendTLog,
	Emul_Flush,
	Emul_GetCodeAddr,
};

///////////////////////////////////////////////////////////////////////////////
// File I/F カートリッジなしもココで定義
///////////////////////////////////////////////////////////////////////////////

static s32 None_InitCart(){
	IW->cw.init_error = FLASH_E_NO_CART;
	return 0;
}
const u32* None_ReadDirect(u32 offset){
	return 0;
}
static s32 None_ReadData(u32 offset, void* dst, s32 size, u32 magic){
	return IW->cw.init_error;
}
static s32 None_WriteData(u32 offset, const void* src, s32 size){
	return IW->cw.init_error;
}
static s32 None_EraseBlock(u32 offset, s32 sizem, u32 mode){
	return IW->cw.init_error;
}
static s32 None_AppendTLog(u8* buf, u32 size){
	return IW->cw.init_error;
}
static s32 None_Flush(){
	return IW->cw.init_error;
}
const u8* None_GetCodeAddr(u32 type){
	return 0;
}
const CartIF CIF_NONE = {
	None_InitCart,
	None_ReadDirect,
	None_ReadData,
	None_WriteData,
	None_EraseBlock,
	None_AppendTLog,
	None_Flush,
	None_GetCodeAddr,
};

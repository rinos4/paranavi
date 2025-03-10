///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// ヒカルの碁3付属のジョイキャリーカートリッジに搭載されたLH28F800BLEフラッ
// シュ用のファイルI/Fをまとめた

///////////////////////////////////////////////////////////////////////////////
// メモリマップ
///////////////////////////////////////////////////////////////////////////////
// タスク設定用
//	{ 0x00000000, (u16*)0x09ff0000, 0x02000 }, // 15
//	{ 0x00002000, (u16*)0x09ff2000, 0x02000 }, // 16 フライトログ/タスクログ保存
// ルート/ウェイポイント用
//	{ 0x00004000, (u16*)0x09ff4000, 0x02000 }, // 17
//	{ 0x00006000, (u16*)0x09ff6000, 0x02000 },
//	{ 0x00008000, (u16*)0x09ff8000, 0x02000 },
//	{ 0x0000a000, (u16*)0x09ffa000, 0x02000 },
// 以下は使用できない
//	{ 0x0000c000, (u16*)0x09ffc000, 0x02000 }, // BOOT BLOCK
//	{ 0x0000e000, (u16*)0x09ffe000, 0x02000 }, // Read Only BLOCK
// プログラム用
//	{ 0x00010000, (u16*)0x09f00000, 0x10000 }, // 0
//	{ 0x00020000, (u16*)0x09f10000, 0x10000 },
//	{ 0x00030000, (u16*)0x09f20000, 0x10000 },
//	{ 0x00040000, (u16*)0x09f30000, 0x10000 },
// 音声用?
//	{ 0x00050000, (u16*)0x09f40000, 0x10000 }, // 4
//	{ 0x00060000, (u16*)0x09f50000, 0x10000 },
//	{ 0x00070000, (u16*)0x09f60000, 0x10000 },
//	{ 0x00080000, (u16*)0x09f70000, 0x10000 },
// トラック用?
//	{ 0x00090000, (u16*)0x09f80000, 0x10000 }, // 8
//	{ 0x000A0000, (u16*)0x09f90000, 0x10000 },
//	{ 0x000B0000, (u16*)0x09fa0000, 0x10000 },
//	{ 0x000C0000, (u16*)0x09fb0000, 0x10000 },
//	{ 0x000D0000, (u16*)0x09fc0000, 0x10000 },
//	{ 0x000E0000, (u16*)0x09fd0000, 0x10000 },
//	{ 0x000F0000, (u16*)0x09fe0000, 0x10000 }, // 14


///////////////////////////////////////////////////////////////////////////////
// 定数
///////////////////////////////////////////////////////////////////////////////

#define FLASH_ID_VAL	0xB0 // キャリリングカートリッジ
#define FLASH_TIMEOUT	(60 * 3) // 3秒

#define FLASH_CMD_READ		0x00ff
#define FLASH_CMD_ID		0x0090
#define FLASH_CMD_STATUS	0x0070
#define FLASH_CMD_CLEAR		0x0050
#define FLASH_CMD_WRITE		0x0040
#define FLASH_CMD_ERASE1	0x0020
#define FLASH_CMD_ERASE2	0x00d0
#define FLASH_CMD_UNLOCK1	0x0060
#define FLASH_CMD_UNLOCK2	0x00d0

#define JC_CONFIG_ADDR	0x09ff0000
#define JC_ROUTE_ADDR	0x09ff4000
#define JC_DAT_ADDR		0x09f00000
#define JC_DAT_END		0x09ff0000


void ByteShift(u8* buf, u32 size);


///////////////////////////////////////////////////////////////////////////////
// Flash制御関数
///////////////////////////////////////////////////////////////////////////////
// カートリッジ検証用 VID取得
static u16 JC_FlashGetID(vu16* dst){
	*dst = FLASH_CMD_ID;
	u16 ret = *dst;
	*dst = FLASH_CMD_READ;
	return ret;
}

// カートリッジステータス、エラーチェック
static s32 JC_FlashReadStatus(vu16* dst){
	*dst = FLASH_CMD_STATUS;
	s32 timeout = IW->vb_counter + FLASH_TIMEOUT;
	u16 s;
	do {
		if((s = *dst) & 0x80){
			if(s & 0x7e) break;
			return FLASH_SUCCESS;
		}
	} while(timeout - (s32)IW->vb_counter > 0);

	// ステータスレジスタのクリア & ステータス応答
	*dst = FLASH_CMD_CLEAR;
	*dst = FLASH_CMD_READ;  // ARRAYモードに戻しておく
	if(!(s & 0x80))			return FLASH_E_STATUS_TIMEOUT;
	if(s & 0x08)			return FLASH_E_SR_VCC;
	if(s & 0x02)			return FLASH_E_SR_PROTECT;
	if((s & 0x30) == 0x30)	return FLASH_E_SR_CMD;
	if(s & 0x10)			return FLASH_E_SR_WRITE;
	if(s & 0x20)			return FLASH_E_SR_ERASE;
	return -1; // !?
}

// ロックビットクリア。対象ブロックの消去前に呼び出すこと。
static s32 JC_FlashUnlockBlock(vu16* dst){
	// ステータスチェック
	s32 ret;
	if((ret = JC_FlashReadStatus(dst)) != FLASH_SUCCESS) return ret;

	// ブロックロックビットクリア
	*dst = FLASH_CMD_UNLOCK1;
	*dst = FLASH_CMD_UNLOCK2;
	if((ret = JC_FlashReadStatus(dst)) != FLASH_SUCCESS) return ret;
	*dst = FLASH_CMD_READ;  // ARRAYモードに戻しておく
	return FLASH_SUCCESS;
}

// ブロック消去
static s32 JC_FlashEraseBlock(vu16* dst){
	s32 ret;
	if((ret = JC_FlashUnlockBlock(dst)) != FLASH_SUCCESS) return ret;

	// ブロック消去
	*dst = FLASH_CMD_ERASE1;
	*dst = FLASH_CMD_ERASE2;
	ret = JC_FlashReadStatus(dst);
	*dst = FLASH_CMD_READ;  // ARRAYモードに戻しておく
	return FLASH_SUCCESS;
}

// Flash書き込み。書き込み領域がイレースされていない場合はエラーとなる
static s32 JC_FlashWriteBlock(vu16* dst, const u16* src, s32 size){
	// ステータスチェック
	s32 ret;
	if((ret = JC_FlashReadStatus(dst)) != FLASH_SUCCESS) return ret;

	// word書き込み
	for(; size > 0 ; size -= 2, ++src, ++dst){
		if(*src == 0xffff) continue; // 書き込み不要
		*dst = FLASH_CMD_WRITE;
		*dst = *src;
		if((ret = JC_FlashReadStatus(dst)) != FLASH_SUCCESS) return ret;
	}

	*dst = FLASH_CMD_READ;  // ARRAYモードに戻しておく
	return FLASH_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// File I/F
///////////////////////////////////////////////////////////////////////////////
// 変換マップ
// 0x00000000 -> 0x09ff0000 (Blk 15)
// 0x00002000 -> 0x09ff2000 (Blk 16)
// 0x00004000 -> 0x09ff4000 (Blk 17)
// 0x00010000 -> 0x09f80000?(音声データや他プログラムにより可変)

// 音声データ候補アドレスリスト
const u32 VOICE_ADDR_JOYCARRY_CAND[] = { // JoyCarry
	((u32)&__iwram_overlay_lma) + (0x09f00000 - 0x02000000), // BANK0結合
	0x09f00000 + 256 * 1024 * 1, // BANK1
	0x09f00000 + 256 * 1024 * 2, // BANK2
	0x09f00000 + 256 * 1024 * 3, // BANK3
	0
};

#define ROM_BOOT_SIG 0x51AEFF24
#define IsUsedBlock(v) ((v)[1] == ROM_BOOT_SIG || (v)[0] == WAV_MAGIC)

static s32 JoyC_InitCart(){
	CartWork* cw = &IW->cw;

	// 音声アドレスを設定
	const u32* p = VOICE_ADDR_JOYCARRY_CAND;
	for(; *p ; ++p){
		if(*(u32*)*p == WAV_MAGIC){
			cw->phy_voice = *p; // 有効データを検出
			break;
		}
	}

	// トラックログの有効ブロックを設定
#define SKIP_BLOCK (256 * 1024)
	u32 addr = JC_DAT_ADDR;
	for(; addr < JC_DAT_END ; addr += SKIP_BLOCK){
		if(!IsUsedBlock((u32*)addr)) break; // Program/Voice領域はbreakせず継続
	}
	cw->phy_tlog = addr;
	cw->tlog_block = 0;
	for(; addr < JC_DAT_END ; addr += BLOCK_SIZE){
		if(IsUsedBlock((u32*)addr)) break; // Program/Voice領域まで
		cw->tlog_block++;
	}

	// ワーク情報設定
	cw->phy_boot	= 0x09ffc000;
	cw->phy_navi	= 0x09f00000;
	cw->phy_config	= 0x09ff0000;
	cw->phy_flog	= 0x09ff2000;
	cw->phy_rtwpt	= 0x09ff4000;
	cw->cap			= CARTCAP_WRITEABLE | CARTCAP_DIRECTMAP | CARTCAP_DUPLICATE;
	return 0;
}

// ダイレクトマップ
const u32* JoyC_ReadDirect(u32 offset){
	// Flashは直読み可能なので、基本的にはアドレス計算のみ
	if(offset < FI_TRACK_OFFSET){
		return (u32*)(JC_CONFIG_ADDR + offset); // config / flog / route
	}
	if(offset < FI_TRACK_OFFSET + IW->cw.tlog_block * BLOCK_SIZE){
		return (u32*)(IW->cw.phy_tlog + (offset - FI_TRACK_OFFSET)); // tracklog
	}
	return 0; // 無効オフセット
}

// ヘッダが一致している場合のみreadする
static s32 JoyC_ReadData(u32 offset, void* dst, s32 size, u32 magic){
	// デバッグ用チェック
	if(0 < magic && magic < 10){ // デバッグ用コード
		DmaCopy(3, offset + 0x09f00000, dst, size, 32); // メモリ直読み
		return 0;
	}

	const u32* p = JoyC_ReadDirect(offset);
	if(!p) return FLASH_E_OFFSET;
	if(JC_FlashGetID((u16*)p) != FLASH_ID_VAL) return FLASH_E_ID;
	if(magic && *p != magic) return FLASH_E_MAGIC; // 0マジックは使わない
	DmaCopy(3, p, dst, size, 32); // 32bit Read
	return 0;
}

static s32 GetBlockSize(u32 offset){
	return (offset < FI_TRACK_OFFSET)? 0x02000 : 0x10000; // ブロック単位消去
}

//offsetはブロック境界になっていること！
static s32 JoyC_WriteData(u32 offset, const void* src, s32 size){
	u16* dst = (u16*)JoyC_ReadDirect(offset);
	if(!dst) return FLASH_E_OFFSET;

	if(JC_FlashGetID(dst) != FLASH_ID_VAL) return FLASH_E_ID;

	// 複数ブロック連続消去
	s32 pos = offset & ~(GetBlockSize(offset) - 1);
	for(; pos < offset + size ; pos += GetBlockSize(pos)){
		u16* p = (u16*)JoyC_ReadDirect(pos);
		if(!p) return FLASH_E_OFFSET;
		s32 ret = JC_FlashEraseBlock(p);
		if(ret) return ret;
	}
	return JC_FlashWriteBlock(dst, (u16*)src, size); // 書き込みは一括でOK
}

static s32 JoyC_EraseBlock(u32 offset, s32 size, u32 mode){
	u16* dst = (u16*)JoyC_ReadDirect(offset);
	if(!dst) return FLASH_E_OFFSET;
	if(mode != ERASE_MAGIC) return JC_FlashEraseBlock(dst); // Magic消去以外は全部共通…

	// ブロック先頭のマジックのみ消去する高速イレース
	s32 ret = JC_FlashUnlockBlock(dst);
	if(ret) return ret;
	u16 dat = ~(*dst & 0xff); // 先頭1バイトのみ0セット
	return JC_FlashWriteBlock(dst, &dat, 2);
}

// bufは2byteアライメント。バッファの後ろに2byteの遊びがあること!
static s32 JoyC_AppendTLog(u8* buf, u32 buf_size){
	// 開始アライメント
	TrackLogParam* tlp = &IW->mp.tlog;
	u16* dst = (u16*)JoyC_ReadDirect(CUR_BLOCK_OFFSET(tlp));
	if(!dst) return FLASH_E_PARAM;

	// Unlock
	if(IW->cw.joyc_work.pre_unlock != (u32)dst){
		JC_FlashUnlockBlock(dst);
		IW->cw.joyc_work.pre_unlock = (u32)dst;
	}

	dst += tlp->index >> 1;
	u32 verify_start = 0, verify_size = buf_size;
	u8* verify_addr = (u8*)dst;
	if(tlp->index & 1){
		tlp->index += buf_size;
		ByteShift(buf, buf_size++);
		buf[0] = 0xff;
		++verify_start;
	} else {
		tlp->index += buf_size;
	}
	// 終端アライメント
	if(buf_size & 1) buf[buf_size++] = 0xff;

	// 書き込み前チェック(実データ部分のみ)
	u8* ff = verify_addr + verify_start;
	s32 i;
	for(i = verify_size ; i-- ; ++ff) if(*ff != 0xff) return FLASH_E_NOT_FFFF;

	// 書き込み
	u16* buf16 = (u16*)buf;
	s32 ret = JC_FlashWriteBlock(dst, buf16, buf_size);

	// 書き込み後チェック(実データ部分のみ)
	if(!ret && memcmp(buf + verify_start, verify_addr + verify_start, verify_size)) return FLASH_E_COMPARE;
	return ret;
}

// フラッシュは何もしない
static s32 JoyC_Flush(){
	return 0;
}

const CartIF CIF_JOYCARRY = {
	JoyC_InitCart,
	JoyC_ReadDirect,
	JoyC_ReadData,
	JoyC_WriteData,
	JoyC_EraseBlock,
	JoyC_AppendTLog,
	JoyC_Flush,
	Emul_GetCodeAddr,
};

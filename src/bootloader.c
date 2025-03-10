///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

// パラナビ用のブートローダ＋MBOOT式ROMアップデータ
// (パラナビ本体から独立したboot.binモジュールとして生成)

// MBOOTアップデートは、ジョイキャリーカートリッジに対応

#include "gba.h"

#define GEN_COUNTER 0xC8 // 世代情報格納用オフセット

///////////////////////////////////////////////////////////////////////////////
// 制御用
///////////////////////////////////////////////////////////////////////////////
u32 GetPCX(){
	u32 ret;
	asm(
		"mov r0, lr\n"
		: "=&r"(ret)
		:
		: "memory");
	return ret;
}

void JumpR0X(u32 addr){
	asm("bx r0");
}

// VBLANKウェイトの割り込みが使えない時のためのスピンループ
void WaitMS(u32 n){
	n *= 5555;
	while(n--); // 1cycle 60ns x 3 step = 180ns
}

void BiosReset(u32 type){
	if(type) {
		// ROMブート
		asm("mov r0,#253");
		asm("swi 1");
		*((u8 *)0x03007ffa) = 0;
		asm("swi 0");
	} else {
		// RAMブート
		asm("mov r0,#252");
		asm("swi 1");
		*((u8 *)0x03007ffa) = 1;
		asm("swi 0");
	}
	// ここには到達しない
}

///////////////////////////////////////////////////////////////////////////////
// ジョイキャリーフラッシュ書き込み
///////////////////////////////////////////////////////////////////////////////
#define FLASH_ID_VAL	0xB0 // キャリリングカートリッジID(WEBで見つけたDataSheetと違うが、現物に合わせた)
#define FLASH_TIMEOUT	(60 * 5) // 5秒で十分?

#define FLASH_CMD_READ		0x00ff
#define FLASH_CMD_ID		0x0090
#define FLASH_CMD_STATUS	0x0070
#define FLASH_CMD_CLEAR		0x0050
#define FLASH_CMD_WRITE		0x0040
#define FLASH_CMD_ERASE1	0x0020
#define FLASH_CMD_ERASE2	0x00d0
#define FLASH_CMD_UNLOCK1	0x0060
#define FLASH_CMD_UNLOCK2	0x00d0

enum {
	FLASH_SUCCESS = 0,
	FLASH_E_PARAM,				// 関数パラメータエラー
	FLASH_E_ID,					// IDエラー
	FLASH_E_STATUS_TIMEOUT,		// ステータス取得タイムアウト
	FLASH_E_SR_VCC,				// VCCエラー
	FLASH_E_SR_PROTECT,			// プロテクトエラー
	FLASH_E_SR_WRITE,			// 書き込みエラー
	FLASH_E_SR_ERASE,			// 消去エラー
	FLASH_E_SR_CMD,				// コマンドエラー

	FLASH_E_MAGIC,				// マジックエラー
	FLASH_E_SIZE,				// サイズエラー
};

// 書き込みアドレス(ブートだけはここで定義。他アドレスは通信で対向のParaNaviから受信)
#define BOOTSEC_ADDR ((u16*)0x09ffc000)
#define BOOTSEC_SIZE (1024 * 8) // 8KB

///////////////////////////////////////////////////////////////////////////////
// フラッシュ制御
// (パラナビ本体では書き込みタイムアウト等の制御をしているが、BOOTは基本的には成功するまでの無限待ち)

u16 FlashGetID(vu16* dst){
	*dst = FLASH_CMD_ID;
	u16 ret = *dst;
	*dst = FLASH_CMD_READ;
	return ret;
}

s32 FlashReadStatus(vu16* dst){
	*dst = FLASH_CMD_STATUS;
	u16 s;
	do {
		if((s = *dst) & 0x80){
			if(s & 0x7e) break;
			return FLASH_SUCCESS;
		}
	} while(1);

	// ステータスレジスタのクリア & ステータス応答
	*dst = FLASH_CMD_CLEAR;
	*dst = FLASH_CMD_READ;  // ARRAYモードに戻しておく
	if(!(s & 0x80))			return FLASH_E_STATUS_TIMEOUT;
	if(s & 0x08)			return FLASH_E_SR_VCC;
	if(s & 0x02)			return FLASH_E_SR_PROTECT;
	if((s & 0x30) == 0x30)	return FLASH_E_SR_CMD;
	if(s & 0x10)			return FLASH_E_SR_WRITE;
	if(s & 0x20)			return FLASH_E_SR_ERASE;
	return -1; // !? 未知のエラー
}

s32 FlashUnlockBlock(vu16* dst){
	// ステータスチェック
	s32 ret;
	if((ret = FlashReadStatus(dst)) != FLASH_SUCCESS) return ret;

	// ブロックロックビットクリア
	*dst = FLASH_CMD_UNLOCK1;
	*dst = FLASH_CMD_UNLOCK2;
	if((ret = FlashReadStatus(dst)) != FLASH_SUCCESS) return ret;
	*dst = FLASH_CMD_READ;  // ARRAYモードに戻しておく
	return FLASH_SUCCESS;
}

s32 FlashEraseBlock(vu16* dst){
	s32 ret;
	if((ret = FlashUnlockBlock(dst)) != FLASH_SUCCESS) return ret;

	// ブロック消去
	*dst = FLASH_CMD_ERASE1;
	*dst = FLASH_CMD_ERASE2;
	if((ret = FlashReadStatus(dst)) != FLASH_SUCCESS) return ret;
	return FLASH_SUCCESS;
}

s32 FlashWriteBlock(vu16* dst, const u16* src, s32 size){
	// ステータスチェック
	s32 ret;
	if((ret = FlashReadStatus(dst)) != FLASH_SUCCESS) return ret;

	// word書き込み
	for(; size > 0 ; size -= 2, ++src, ++dst){
		if(*src == 0xffff) continue; // 書き込み不要
		*dst = FLASH_CMD_WRITE;
		*dst = *src;
		if((ret = FlashReadStatus(dst)) != FLASH_SUCCESS) return ret;
	}

	*dst = FLASH_CMD_READ;  // ARRAYモードに戻しておく
	return FLASH_SUCCESS;
}

// Read/Write
s32 FlashWrite(u16* dst, const void* src, s32 size){
	if(FlashGetID(dst) != FLASH_ID_VAL) return FLASH_E_ID;

	s32 ret = FlashEraseBlock(dst);
	if(ret) return ret;
	ret = FlashWriteBlock(dst, (u16*)src, size);
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// ダウンロード実行
///////////////////////////////////////////////////////////////////////////////
#define SIO_NORMAL_MASTER	0x1001
#define SIO_NORMAL_SLAVE	0x1000
#define SIO_MPLAY_MASTER	0x2003

void SetCommMode(u32 mode){
	REG16(REG_RCNT)		= 0;
	REG16(REG_SIOCNT)	= mode;
}

// マルチプレイモード用
u32 SendRecv(u32 val){
	REG16(0x400012a) = val;
//	while(REG16(REG_SIOCNT) & 0x4);
	REG16(REG_SIOCNT) |= 0x80;
	while(REG16(REG_SIOCNT) & 0x80);
	return REG16(0x4000120);
}
#define SendResv2() ((SendRecv(0xa) << 16) | SendRecv(0xb))

// ノーマルモード用
u32 SendRecv32(u32 val){
	REG32(0x04000120) = val;
	REG16(REG_SIOCNT) |= 0x80;
	while(REG16(REG_SIOCNT) & 0x80);
	return REG32(0x04000120);
}

#define DUP_MODE_WRAM	0x1000
#define DUP_MODE_JOYC	0x1001

void Download(u32 cart){
	// とりあえず、画面をつける
	REG16(REG_DISPCNT) = DISP_MODE_3 | DISP_BG0_ON | DISP_BG1_ON | DISP_BG2_ON | DISP_OBJ_ON | DISP_OBJ_CHAR_1D_MAP;
	u16* vram = (u16*)VRAM;
	int pos = 0;

	// カートリッジ有無を渡す
	cart = cart? DUP_MODE_JOYC : DUP_MODE_WRAM; // カートリッジ書き込み or RAMモード動作用コピー
	for(;;) {
		SetCommMode(SIO_MPLAY_MASTER);
		WaitMS(100);
		if(SendRecv(cart) == 0x5555) break;
	}

	// データのダウンロード
	for(;;){
		// 同期待ち
		if(SendRecv(0xaaaa) != 0x5555) continue;

		// サイズ送信 & ヘッダチェック
		u32 size = SendResv2();
		if(size > 256 * 1024) continue; // MAX 256KB
		if(size != ~SendResv2()) continue;
		u32 dst = SendResv2();
		if(dst != ~SendResv2()) continue;
		SendRecv(0x1000); // ack
		if(!size) break; // 終了

		u16* p = (u16*)EX_WRAM;
		if(dst < CPU_WRAM) p = (u16*)dst; // RAMコピー時はそのままRAMに書く
		u32 bcc = 0;
		u32 i;
		for(i = 0 ; i < size ; i += 2){
			*p = SendRecv(1);
			if(pos ++ >= 160 * 240) pos = 0;
			vram[pos] = *p; // デバッグ用に画面に受信データを表示。
			bcc += *p++;
		}
		if(SendResv2() != bcc){
			SendRecv(0x1ff1); // BCCエラー
			continue;
		}
		SendRecv(0x1001); // ダウンロード成功

		if(dst > CPU_WRAM){
			if(FlashWrite((u16*)dst, (void*)EX_WRAM, size) != FLASH_SUCCESS){
				SendRecv(0x1ff2); // Flash書き込みエラー
				continue;
			}
		}
		SendRecv(0x1002); // 書き込み完了
		SendRecv(0x1002); // 念のため二度書き
	}
}

// ZZZ TEST CODE //////////////////////////////////////////////////////////////
u16 FlashGetID2(vu16* dst, u16* detail){
	*dst = FLASH_CMD_ID;
	u16 ret = *dst;
	*detail = dst[1];
	*dst = FLASH_CMD_READ;
	return ret;
}

void UnlockBit_AllBlock(){
	u32 i;
	for(i = 0x08000000 ; i < 0xfe000000 ; i += 0x4000){ // ちょっと範囲が広すぎないか？
		*(u16*)i = 0x60; // FLASH_CMD_UNLOCK1
		*(u16*)i = 0xd0; // FLASH_CMD_UNLOCK2
		*(u16*)i = 0xff; // FLASH_CMD_READ
		*(u16*)i = 0x60; // FLASH_CMD_UNLOCK1
		*(u16*)i = 0xd0; // FLASH_CMD_UNLOCK2
		*(u16*)i = 0xff; // FLASH_CMD_READ
	}
}
void CartXWrite(u32 offset, u16 val, u16 len){
	vu16* dst = (u16*)(0x08000000 + (offset << 1));
	while(len--) *dst = val;
}
void CartXCmd(){
	CartXWrite(0x00987654, 0x5354, 1);
	CartXWrite(0x00012345, 0x1234, 500);
	CartXWrite(    0x7654, 0x5354, 1);
	CartXWrite(0x00012345, 0x5354, 1);
	CartXWrite(0x00012345, 0x5678, 500);
	CartXWrite(0x00987654, 0x5354, 1);
	CartXWrite(0x00012345, 0x5354, 1);
	CartXWrite(0x00765400, 0x5678, 1);
	CartXWrite(0x00013450, 0x1234, 1);
	CartXWrite(0x00012345, 0xabcd, 500);
	CartXWrite(0x00987654, 0x5354, 1);
	return;
}
void CartXUnlockFlash(){
	CartXCmd();
	CartXWrite(0x00F12345, 0x9413, 1);
}
void CartXSetFlash(u16 val){
	CartXCmd();
	CartXWrite(0x00B5AC97, val, 1);
}

// デバッグLED代わりのドット表示
void PutDot(u32 x, u32 y, u32 val){
	u16* vram = (u16*)VRAM + y * 240 + x;
	for(x = 0x80000000 ; x ; x >>= 1){
		*vram = (val & x)? 0xffff : 0x001f;
		if(x & 0x00010000) vram += 1;
		if(x & 0x01010100) vram += 1;
		if(x & 0x11111110) vram += 1;
		vram += 2;
	}
}

void MBLoader(){
	// 自身をIWRAMへコピー(EWRAMはコピー用に使う)
	DmaCopy(3, EX_WRAM, CPU_WRAM, BOOTSEC_SIZE, 32); // とりあえず最大の8KBをコピーしておく
	JumpR0X(GetPCX() + (CPU_WRAM - EX_WRAM));

	// 以降、IWRAMで動作 //////////////////////////////////////////////////////
	u32 cart_ok = 0;
	++*(u32*)(EX_WRAM + GEN_COUNTER); // 世代を1つ増やして記録する

	REG16(REG_DISPCNT) = DISP_MODE_3 | DISP_BG0_ON | DISP_BG1_ON | DISP_BG2_ON | DISP_OBJ_ON | DISP_OBJ_CHAR_1D_MAP;

	u16 key = (*(vu16 *)REG_KEYINPUT) ^ 0x03ff;
	if(key == KEY_A){
	// TEST CODE //////////////////////////////////////////////////////////////
	//	REG16(REG_WSCNT) = 0x0314;
		UnlockBit_AllBlock();
		PutDot(0, 1, 4);
		CartXUnlockFlash();
		CartXSetFlash(0x607); // 0x01ffc000 → (03ff & 0xfc00)
		UnlockBit_AllBlock();
		PutDot(0, 3, 2);

		u32 ret = FlashWrite(BOOTSEC_ADDR, (void*)EX_WRAM, BOOTSEC_SIZE);
		if(ret == FLASH_SUCCESS) cart_ok = 1; // ブートセクタに書き込みできるかでチェック
		else {
			PutDot(0, 10, ret);
			vu32* dst = (u32*)BOOTSEC_ADDR;
			*dst = FLASH_CMD_CLEAR;
			*dst = FLASH_CMD_READ;  // ARRAYモードに戻しておく

			u16 ret16;
			ret = FlashGetID2(BOOTSEC_ADDR, &ret16);
			PutDot(0, 12, ret | (ret16 << 16));

			PutDot(10, 70, -1);
			for(;;);
		}
	} else {
		if(FlashWrite(BOOTSEC_ADDR, (void*)EX_WRAM, BOOTSEC_SIZE) == FLASH_SUCCESS) cart_ok = 1; // ブートセクタに書き込みできるかでチェック
	}

	// ダウンロード
	Download(cart_ok);
	BiosReset(cart_ok);
}

///////////////////////////////////////////////////////////////////////////////
// RAMコピー実行
///////////////////////////////////////////////////////////////////////////////
#define ROM_SEC0 ((u32*)0x09f00000)
#define ROM_SEC1 ((u32*)0x09f40000)
#define ROM_SEC2 ((u32*)0x09f80000)
#define ROM_SIZE 0x40000
#define ROM_MARK 0x51aeff24 // コードあり

// RAMへコピーして実行
void EramBoot(){
	// 起動セクタを選択
	u32* src = ROM_SEC0; // 通常はSEC0から起動
	switch((*(vu16 *)REG_KEYINPUT) ^ 0x03ff){// おまけモード判別用
	case KEY_A: src = ROM_SEC1; break;	// 起動時Aボタンで、おまけブート1
	case KEY_B:	src = ROM_SEC2; break;	// 起動時Bボタンで、おまけブート2
	}
	if(src[1] != ROM_MARK) src = ROM_SEC0; // コード無しの場合はSEC0に戻す

	// ERAMにコピーして実行
	DmaCopy(3, src, EX_WRAM, ROM_SIZE, 32);
	BiosReset(0);
}

///////////////////////////////////////////////////////////////////////////////
// Bootloaderメイン
///////////////////////////////////////////////////////////////////////////////
void AgbMain(){
	if(GetPCX() < CPU_WRAM) MBLoader(); // RAM動作時はMultiBootなので受信モードに遷移
	else                   EramBoot(); // ROM動作時はRAMコピーして実行
}

///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2007 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// SuperCard SD/MiniSD用のファイルI/Fをまとめた。
// FATファイルシステムもここで実装(他のFATデバイス用に分離すべき?)

// SCSDでは最小書き込み単位がセクタサイズになるため、書き込み速度と書き換え回数制限上、トラックログは
// ライトキャッシュを使わざるを得ない。このため、電源を切るときはサスペンドを使ってWriteCacheをフラッ
// シュさせる必要あり。
// ※ キャッシュ無しの場合、1秒間隔で512byteセクタを毎回書き換えるため、64KBの最小ログファイルを使用す
//　　るとFlash書き換え回数リミット(10万回)に 100000 * 65536 / 512 [s] ≒ 148[日]で達してしまう。
//    (512byteセクタのライトキャッシュを使えば、1byte/point圧縮率環境の連続運用で約200年持つ)


// TODO 書き込みに500ms程度要する異様に遅いセクタがある? カード不良!?

// …なんか、SCSD使うとSDカードアクセスしなくてもJoyCarryの半分位しか電池が
// 持たない。 音声非再生時にSCSDのSDRAMの電源OFFすれば改善可能？

///////////////////////////////////////////////////////////////////////////////
// 定数
///////////////////////////////////////////////////////////////////////////////

// "\Paranavi.dat"をデータファイルに指定する
#define DAT_FILE_NAME "paranavidat " // 最後のスペース(0x20)はアトリビュート

#define ENABLE_WRITE_VERIFY 1 // 遅くなるがベリファイは重要


///////////////////////////////////////////////////////////////////////////////
// FATファイルシステム
///////////////////////////////////////////////////////////////////////////////
//マスタブートレコード
typedef struct {
	u8 x86code[446];
	
	// パーティションテーブル
	u8 p0_boot		[1];
	u8 p0_first_sect[3];
	u8 p0_fat_desc	[1];
	u8 p0_last_sect	[3];
	u8 p0_log_sect	[4];
	u8 p0_num_sect	[4];

	u8 p1_boot		[1];
	u8 p1_first_sect[3];
	u8 p1_fat_desc	[1];
	u8 p1_last_sect	[3];
	u8 p1_log_sect	[4];
	u8 p1_num_sect	[4];

	u8 p2_boot		[1];
	u8 p2_first_sect[3];
	u8 p2_fat_desc	[1];
	u8 p2_last_sect	[3];
	u8 p2_log_sect	[4];
	u8 p2_num_sect	[4];

	u8 p3_boot		[1];
	u8 p3_first_sect[3];
	u8 p3_fat_desc	[1];
	u8 p3_last_sect	[3];
	u8 p3_log_sect	[4];
	u8 p3_num_sect	[4];

	// チェックコード 55 aa
	u8 check[2];
} MBR;

// FATヘッダ
typedef struct {
	// HEAD				//		UMAX MiniSD 1GB
	u8  jmp;			// 0	EB
	u8  ipl;			// 1	(3C)
	u8  nop;			// 2	90
	u8  name[8];		// 0    "MSDOS 5.0"

	// BPB
	u8  sect_size[2];	// B    512byte
	u8  clst_size;		// D    32 (= 16KB) 16KB*65536 = 1GB
	u16 start_sect;		// E    2
	u8  fat_num;		// 10   2 backup
	u8  dir_entry[2];	// 11   512
	u8  log_sect[2];	// 13   0 (->log_sect2)
	u8  media_desc;		// 15   0xF8
	u16 fat_sect;		// 16   0x00EF
	u16 phy_sec;		// 18   0x003F
	u16 head_count;		// 1A   0x00FF
	u32 hide_sec;		// 1C	0x81
	u32 log_sect2;		// 20	0x001dc37f (* 512 = 1GB)
	// 後はどうでもいい…
} FatHeader;

typedef struct {
	u8 name[8];		// 0
	u8 ext[3];		// 8
	u8 attr;		// B
	u8 NT;			// C
	u8 c_time_ms;
	u8 c_time[2];
	u8 c_date[2];
	u8 a_date[2];
	u8 clst_h[2];
	u8 u_time[2];
	u8 u_date[2];
	u16 head;		// クラスタヘッダ
	u32 size;		// ファイルサイズ
} DirEntry;


#define DIR_HEAD_OFFSET 0x1a
#define DIR_SIZE_OFFSET 0x1c

// 毎回クラスタ計算していたら遅くて使えないので、ファイルオフセットから
// セクタ位置を計算するためのクラスタキャッシュをSCのSDRAM上に作成する。
u32* const CLUSTER_CACHE = (u32*)(0x08000000 + 0x80000); // +512KB(≧Paranavi.bin+Voice.bin+Boot.bin)

#define SCSD_TIMEOUT	(60 * 1) // 1秒

///////////////////////////////////////////////////////////////////////////////
// SCSD制御
///////////////////////////////////////////////////////////////////////////////
// SuperCard WEBサイトのarmコードがベース。Cに起こしてエラータイムアウト等を追加。
// コンパイラでthumbになる。
#define SC_SD_CMD		(*(vu16*)0x09800000)
#define SC_SD_CMD32A	(*(vu32*)0x09800000)
#define SC_SD_CMD32B	(*(vu32*)0x09800004)
#define SC_SD_CMD32C	(*(vu32*)0x09800008)
#define SC_SD_CMD32D	(*(vu32*)0x0980000c)
#define SC_SD_DATA		(*(vu16*)0x09000000)
#define SC_SD_DATA32A	(*(vu32*)0x09000000)
#define SC_SD_DATA32B	(*(vu32*)0x09000004)
#define SC_SD_READ		(*(vu16*)0x09100000)
#define SC_SD_READ32A	(*(vu32*)0x09100000)
#define SC_SD_READ32B	(*(vu32*)0x09100004)
#define SC_SD_RESET		(*(vu16*)0x09440000)
#define SC_UNLOCK		(*(vu16*)0x09FFFFFE)

#define ENABLE_SDRAM  5
#define ENABLE_SDCARD 3

u32 SCSDIsInserted(){
//  return (SC_SD_CMD & 0x0300);
	return (SC_SD_CMD);
}

// 512 byte sector write
static void sd_data_write16(const u16* src, s32 n){
	while(n--){
		u32 v = *src++;
		// 何か面倒なコントローラだな…。誤書き込み防止か?
		SC_SD_DATA32A = (v += v << 20);
		SC_SD_DATA32B = v >> 8;
	}
}

static s32 sd_data_write_s(const u16* src, const u16* crc16){
	// アイドル待ち
	s32 timeout = IW->vb_counter + SCSD_TIMEOUT;
	while(!(SC_SD_DATA & 0x100)){
		if(timeout - (s32)IW->vb_counter < 0) return FLASH_E_STATUS_TIMEOUT;
	}
	SC_SD_DATA; // 空読み

	// 512セクタ単位で書き込み
	SC_SD_DATA = 0;		// start bit
	sd_data_write16(src, 256);
	if(crc16) sd_data_write16(crc16, 4);
	SC_SD_DATA = 0xFF;	// end bit

	// 完了待ち
	timeout = IW->vb_counter + SCSD_TIMEOUT;
	while(SC_SD_DATA & 0x100){
		if(timeout - (s32)IW->vb_counter < 0) return FLASH_E_STATUS_TIMEOUT;
	}
	SC_SD_DATA32A;// 空読み
	SC_SD_DATA32B;// 空読み
	return 0;
}

static s32 sd_data_read_s(u16* dst){
	// アイドル待ち
	s32 timeout = IW->vb_counter + SCSD_TIMEOUT;
	while(SC_SD_READ & 0x100){
		if(timeout - (s32)IW->vb_counter < 0) return FLASH_E_STATUS_TIMEOUT;
	}
//	SC_SD_READ; // 空読み (TODO ここでReadするとビットずれが起こる!?とりあえずコメントにしておく)

	u32 i = 0;
	for (i = 0; i < 256; i++){
		SC_SD_READ32A; // 読み捨て
		*dst++ = (u16)(SC_SD_READ32B >> 16);
	}

	// CRCは読み捨て(本当はチェックすべき…)
	SC_SD_READ32A;
	SC_SD_READ32B;
	SC_SD_READ32A;
	SC_SD_READ32B;
	SC_SD_READ32A;
	SC_SD_READ32B;
	SC_SD_READ32A;
	SC_SD_READ32B;

	SC_SD_READ; // 空読み
	return 0;
}

// CRC計算
static void sd_crc16_s(const u8* src, u16 num, u8* crc16){
	u32 r3 = 0, r4 = 0, r5 = 0, r6 = 0;

	u32 r7 = 0x80808080;
	u32 r8 = 0x1021;
	u32 r2 = 0;
	for(num <<= 3 ; num ; num -= 4){
		if (r7 & 0x80) r2 = *src++;

		r3 <<= 1;
		if (r3 & 0x10000)  r3 ^= r8;
		if (r2 & (r7>>24)) r3 ^= r8;

		r4 <<= 1;
		if (r4 & 0x10000)  r4 ^= r8;
		if (r2 & (r7>>25)) r4 ^= r8;

		r5 <<= 1;
		if (r5 & 0x10000)  r5 ^= r8;
		if (r2 & (r7>>26)) r5 ^= r8;

		r6 <<= 1;
		if (r6 & 0x10000)  r6 ^= r8;
		if (r2 & (r7>>27)) r6 ^= r8;

		r7 = (r7 >> 4) | (r7 << 28); // ror
	}

	for(num = 16 ; num ;){
		r7 <<= 4;
		if (r3 & 0x8000) r7 |= 8;
		if (r4 & 0x8000) r7 |= 4;
		if (r5 & 0x8000) r7 |= 2;
		if (r6 & 0x8000) r7 |= 1;

		r3 <<= 1;
		r4 <<= 1;
		r5 <<= 1;
		r6 <<= 1;

		if (num-- & 1) *crc16++ = (u8)r7;
	}
}

static u32 sd_crc7_s(const u8* src, u16 num){
	u32 r3 = 0;
	u32 r4 = 0x80808080;
	u32 r2 = 0;

	for(num <<= 3 ; num ; --num){
		if (r4 & 0x80) r2 = *src++;

		r3 = r3 << 1;
		if (r3 & 0x80)     r3 ^= 9;
		if (r2 & (r4>>24)) r3 ^= 9;
		r4 = (r4 >> 1) | (r4 << 31); // ror
	}

	return (r3 << 1) + 1;
}

static s32 sd_com_read_s(u32 num){
	// アイドル待ち
	s32 timeout = IW->vb_counter + SCSD_TIMEOUT;
	while(SC_SD_CMD & 1){
		if(timeout - (s32)IW->vb_counter < 0) return FLASH_E_STATUS_TIMEOUT;
	}

	// 空読み
	while(num--){
		SC_SD_CMD32A;
		SC_SD_CMD32B;
		SC_SD_CMD32C;
		SC_SD_CMD32D;
	}
	return 0;
}

static inline s32 sd_get_resp(){
	return sd_com_read_s(6);
}

static void sc_mode(u16 data){
	SC_UNLOCK = 0xA55A;
	SC_UNLOCK = 0xA55A;
	SC_UNLOCK = data;
	SC_UNLOCK = data;
}

static void sc_sdcard_reset(){
	SC_SD_RESET = 0;
}

static s32 sd_com_write_s(const u8* src, u32 num){
	// アイドル待ち
	s32 timeout = IW->vb_counter + SCSD_TIMEOUT;
	while(!(SC_SD_CMD & 1)){
		if(timeout - (s32)IW->vb_counter < 0) return FLASH_E_STATUS_TIMEOUT;
	}
	SC_SD_CMD; // 空読み

	while(num--){
		u32 v= *src++;
		v += v << 17;
		SC_SD_CMD32A = v;
		SC_SD_CMD32B = v << 2;
		SC_SD_CMD32C = v << 4;
		SC_SD_CMD32D = v << 6;
	}
	return 0;
}

static void sd_send_clk(u32 count){
	while(count--) SC_SD_CMD; // クロック
}

static s32 sd_cmd(u8 cmd, u32 sector){
	u8 buf[6];
	buf[0] = (u8)(cmd + 0x40);
	buf[1] = (u8)(sector >> 24);
	buf[2] = (u8)(sector >> 16);
	buf[3] = (u8)(sector >>  8);
	buf[4] = (u8)(sector      );
	buf[5] = (u8)sd_crc7_s(buf, 5);
	return sd_com_write_s(buf, 6);
}

///////////////////////////////////////////////////////////////////////////////
// セクタ読み込み
static s32 SDReadSector(u16* buf, u32 sector){
	sc_mode(ENABLE_SDCARD);
	sc_sdcard_reset();

	s32 ret = 0;
#define CHECK_RET(f) do { if((ret = (f)) != 0) return ret; } while(0)
	CHECK_RET(sd_cmd(18, sector << 9));
	CHECK_RET(sd_data_read_s(buf));
	CHECK_RET(sd_cmd(12, 0));
	CHECK_RET(sd_get_resp());
	sd_send_clk(0x10);
	return 0;
}

// セクタ書き込み
static s32 SDWriteSector(const u16* buf, u32 sector){
	sc_mode(ENABLE_SDCARD);
	sc_sdcard_reset();

	s32 ret = 0;
	CHECK_RET(sd_cmd(25, sector << 9));
	CHECK_RET(sd_get_resp());
	sd_send_clk(0x10); 

	u16 crc16[5];
	sd_crc16_s((u8*)buf, 512, (u8*)crc16);
	CHECK_RET(sd_data_write_s(buf, crc16));
	sd_send_clk(0x10); 

	CHECK_RET(sd_cmd(12, 0));
	CHECK_RET(sd_get_resp());
	sd_send_clk(0x10);

	// 書き込み完了待ち
	s32 timeout = IW->vb_counter + SCSD_TIMEOUT;
	while(!(SC_SD_DATA & 0x0100)){
		if(timeout - (s32)IW->vb_counter < 0){
			ret = FLASH_E_STATUS_TIMEOUT;
			break;
		}
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// DATファイルアクセス制御
///////////////////////////////////////////////////////////////////////////////
// セクタ位置を直読みするために、初期化時にクラスタキャッシュを作成しておく
// とりあえずFAT16のみサポート (FAT12はイラナイ?)
static s32 CreateClusterCache(u32 fat_desc){
	SCSDWork* ssw = &IW->cw.scsd_work;

	u32 clst_bytes = ssw->clst_size * SECT_SIZE;

	// まず、未使用のVOICE_ADDRESS領域にクラスタキャッシュを作成
	u32 offset = 0;
	u16 clst = ssw->file_head; // 最初のクラスタをセット
	u32* dst = (u32*)VOICE_ADDRESS;
	u32 pre_fat = -1;
	ssw->clst_limit = 0;
	ssw->datf_min = -1;
	ssw->datf_max =  0;

	for(offset = 0 ; offset < ssw->file_size ; offset += clst_bytes){
		if(clst < 0x0002 || 0xfff6 < clst) return FLASH_E_CNUM_RANGE; // 有効クラスタ番号以外はエラー
		u32 sect = (clst - 2) * ssw->clst_size + ssw->dat_start; // 先頭のセクタ番号を書き込む
		*dst++ = sect;

		// 書き込みチェック用の値を設定
		if(ssw->datf_min > sect) ssw->datf_min = sect;
		if(ssw->datf_max < sect) ssw->datf_max = sect;

		// 対象クラスタのFATをリード
		u32 cur_fat = clst >> 8; //TODO FAT12
		if(pre_fat != cur_fat){
			pre_fat = cur_fat;
			s32 ret = SDReadSector(ssw->scache16, ssw->fat_start + cur_fat);
			if(ret) return ret;
		}
		clst = ssw->scache16[clst & 0xff]; // TODO FAT12
#define CLUSTER_CACHE_LIMIT 0x10000	// 256KB
		if(++ssw->clst_limit > CLUSTER_CACHE_LIMIT) return FLASH_E_DAT_SIZE;
	}
	// VOICE_ADDRESSからSCSDのSDRAMへクラスタキャッシュ転送
	sc_mode(ENABLE_SDRAM); // SDRAM書き込み許可してから転送
	DmaCopy(3, VOICE_ADDRESS, CLUSTER_CACHE, (u32)dst - (u32)VOICE_ADDRESS, 32); // 32bit WriteでOK
	sc_mode(ENABLE_SDCARD); // 通常のSDカードアクセスモードに戻す
	return 0;
}

static u32 GetShiftPos(u32 val){
	u32 i = 0;
	for(; val ; ++i, val >>= 1){
		if(val & 1){
			if(val > 1) break; // 2つ以上"1"あり
			return i;
		}
	}
	return -1;
}

// デバッグ用にダンプエリアにセクタ情報を書き込み
static s32 MBR_Error(const MBR* mbr){
	DmaCopy(3, mbr, IW->mp.last_wpt, SECT_SIZE, 32); // wpt-routeエリアに連結書き込み
	IW->mp.last_wpt_len = SECT_SIZE + DBGDMP_SCSD;
	IW->mp.last_route_len = 0;
	return FLASH_E_MBR_ERROR;
}
static s32 FAT_Error(const FatHeader* fh, s32 err){
	DmaCopy(3, fh, IW->mp.last_pvt, 256, 32); // ヘッダ前半をダンプ(last_pvtに収まる範囲で…)
	IW->mp.last_pvt_len = 256 + DBGDMP_SCSD;
	return err;
}

static s32 InitDatFileInfo(){
	SCSDWork* ssw = &IW->cw.scsd_work;

	sc_mode(ENABLE_SDCARD);
	sc_sdcard_reset();

	// 物理セクタ0チェック
	u32 log_sect = 0;
	u32 fat_desc = 0;
	{
		s32 ret = SDReadSector(ssw->scache16, 0);
		if(ret) return ret;
		MBR* mbr = (MBR*)ssw->scache16;
		// FAT12/16のみサポート
		fat_desc = mbr->p0_fat_desc[0];
		if(fat_desc != 1 && fat_desc != 4 && fat_desc != 6){
//			return MBR_Error(mbr);
			// セクタ0が論理セクタのカードがある!?　とりあえずlog_sect=0で進める…
		} else {
			// BPBのセクタサイズチェック
			log_sect = GetLong(mbr->p0_log_sect);
		}

		//TODO REMOVE デバッグ用にダンプ
		MBR_Error(mbr);

		// mbrが使えるのはココまで
	}

	{
		// セクタサイズチェック
		s32 ret = SDReadSector(ssw->scache16, log_sect); // 論理セクタ0をリード
		if(ret) return ret;
		FatHeader* fh = (FatHeader*)ssw->scache16;
		ssw->sect_size = GetInt(fh->sect_size);
		ssw->sect_shift = GetShiftPos(ssw->sect_size);
		if(ssw->sect_shift == -1) return FAT_Error(fh, FLASH_E_SECT_ERROR); // sect_size ≠ 2^n  !!

		// とりあえず 512 byteセクタのみサポート。1024以上にするならセクタキャッシュを増やす必要あり!
		if(ssw->sect_size != SECT_SIZE) return FAT_Error(fh, FLASH_E_SECT_ERROR);

		// クラスタサイズチェック
		ssw->clst_size = fh->clst_size;
		ssw->clst_shift = GetShiftPos(ssw->clst_size);
		if(ssw->clst_shift == -1) return FAT_Error(fh, FLASH_E_CLST_ERROR); // clst_size ≠ 2^n  !!
		ssw->clst_shift += ssw->sect_shift;

		// FATエントリからルートディレクトリ情報を取得
		ssw->fat_start = fh->start_sect + log_sect;
		ssw->dir_start = ssw->fat_start + fh->fat_num * fh->fat_sect;
		ssw->dat_start = ssw->dir_start + (GetInt(fh->dir_entry) >> 4); // max / (512/32)

		//TODO REMOVE デバッグ用にダンプ
		FAT_Error(fh, 0);

		// fhが使えるのはココまで
	}

	// ルートディレクトリからdatファイルを探す
	u32 sect;
	for(sect = ssw->dir_start ; sect < ssw->dat_start ; ++sect){
		s32 ret = SDReadSector(ssw->scache16, sect); // ディレクトリエントリをリード
		if(ret) return ret;
#define DIRENT_SEC 16 // 512 / 32
		u32 j;
		for(j = 0 ; j < DIRENT_SEC ; ++j){
			u8* p = &ssw->scache8[j << 5];
			if(!*p) return FLASH_E_NO_FILE; // 終端。見つからなかった。
			if(!strnicmp(p, DAT_FILE_NAME, sizeof(DAT_FILE_NAME) - 1)){
				// ファイルを見つけた
				ssw->file_head = GetInt (p + DIR_HEAD_OFFSET);
				ssw->file_size = GetLong(p + DIR_SIZE_OFFSET) & ~(BLOCK_SIZE - 1);// 使用するファイル領域をBLOCK_SIZEの倍数に正規化
				return CreateClusterCache(fat_desc);
			}
		}
	}
	return FLASH_E_NO_FILE;
}

static u32 GetSector(u32 offset){
	SCSDWork* ssw = &IW->cw.scsd_work;
	u32 clst = offset >> ssw->clst_shift;
	if(clst >= ssw->clst_limit) return -1; // 大きすぎるオフセット指定

	// 完全ではないが、クラスタキャッシュの異常チェックもしておく
	u32 sect_num = CLUSTER_CACHE[clst];
	if(sect_num < ssw->datf_min || ssw->datf_max < sect_num) return -1; // 連続クラスタならDATファイルに閉じられる

	return sect_num + ((offset >> ssw->sect_shift) & (ssw->clst_size - 1)); // 正常セクタ番号?
}

// size はSECT_SIZEの倍数であること!
static s32 ReadDatFile(u16* buf, u32 offset, s32 size){
	SCSDWork* ssw = &IW->cw.scsd_work;
	while(size > 0){
		u32 sect = GetSector(offset);
		if(sect == -1) return FLASH_E_OFFSET;

		// セクタキャッシュチェック
		if(sect == ssw->cache_sect){
			if(buf != ssw->scache16) DmaCopy(3, ssw->scache16, buf, SECT_SIZE, 32);
		} else {
			s32 ret = SDReadSector(buf, sect);
			if(ret){
				if(buf == ssw->scache16) ssw->cache_sect = 0; // セクタキャッシュ破壊
				return ret;
			}
			if(buf == ssw->scache16) ssw->cache_sect = sect; // セクタキャッシュ更新
		}
		buf    += SECT_SIZE >> 1;
		offset += SECT_SIZE;
		size   -= SECT_SIZE;
	}
	return 0;
}

static s32 WriteDatFile(const u16* buf, u32 offset, s32 size){
	SCSDWork* ssw = &IW->cw.scsd_work;
	while(size > 0){
		// データ書き込み
		u32 sect = GetSector(offset);
		if(buf == ssw->scache16) ssw->cache_sect = sect; // セクタキャッシュ更新(-1なら無効として記録)
		if(sect == -1) return FLASH_E_OFFSET;
		s32 ret = SDWriteSector(buf, sect);
		if(ret) return ret; // 書き込みに失敗してもセクタキャッシュはクリアしない

#ifdef ENABLE_WRITE_VERIFY
		// コンペア
		u16 rdata[SECT_SIZE >> 1]; // スタックギリギリ!!
		ret = SDReadSector(rdata, sect);
		if(ret) return ret;
		if(memcmp(rdata, buf, SECT_SIZE)){
			/*
			// コンペアエラー箇所をダンプ
			*(u32*)IW->mp.last_wpt = sect;
			DmaCopy(3, rdata, IW->mp.last_wpt + 4, 256 - 4, 32);
			IW->mp.last_wpt_len = 256 + DBGDMP_SCSD;
			*(u32*)IW->mp.last_pvt = offset;
			DmaCopy(3, buf, IW->mp.last_pvt + 4, 256 - 4, 32);
			IW->mp.last_pvt_len = 256 + DBGDMP_SCSD;
			*/
			return FLASH_E_COMPARE;
		}
#endif

		// 書き込みOK
		buf    += SECT_SIZE >> 1;
		offset += SECT_SIZE;
		size   -= SECT_SIZE;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// File I/F SuperCard SD
///////////////////////////////////////////////////////////////////////////////
void SearchRomVoice();
static s32 SCSD_InitCart(){
	CartWork* cw  = &IW->cw;
	SCSDWork* ssw = &cw->scsd_work;
	s32 ret = InitDatFileInfo();
	if(ret) return ret;
	if(ssw->file_size < FI_TRACK_OFFSET) return FLASH_E_SIZE;

	// 音声アドレスを設定
	SearchRomVoice();

	// トラックログの有効ブロックを設定
	cw->phy_tlog = 0; // TODO ログは複製対象外
	cw->tlog_block = ssw->file_size / FI_TRACK_OFFSET - 1;

	// ワーク情報設定
	cw->phy_boot	= ((u32)&__iwram_overlay_lma) + (0x08000000 - 0x02000000);
	cw->phy_navi	= 0x08000000;
	cw->phy_config	= (u32)&IW->tc; // RAMからコピー。後ろにゴミが付くがとりあえず気にしない…
	cw->phy_flog	= 0; // TODO ログは複製対象外
	cw->phy_rtwpt	= (u32)ROUTE; // RAMからコピー。後ろにゴミが付くがとりあえず気にしない…
	cw->cap			= CARTCAP_WRITEABLE | CARTCAP_DUPLICATE | CARTCAP_SECTWRITE;
	return 0;
}

// 512byteセクタのみリード
const u32* SCSD_ReadDirect(u32 offset){
	SCSDWork* ssw = &IW->cw.scsd_work;
	if(!ReadDatFile(ssw->scache16, offset, SECT_SIZE)) return ssw->scache32;
	return 0;
}

// ヘッダが一致している場合のみreadする
static s32 SCSD_ReadData(u32 offset, void* dst, s32 size, u32 magic){
	// デバッグ用チェック
	if(0 < magic && magic < 10){ // デバッグ用コード
		if(magic == 1) return SDReadSector((u16*)dst, offset >> 9); // セクタ直読み
//		DmaCopy(3, CLUSTER_CACHE, dst, size, 16); // 16bit Read
		DmaCopy(3, CLUSTER_CACHE, dst, size, 32); // 32bit Read
		return 0;
	}
		
	const u32* p = SCSD_ReadDirect(offset);
	if(!p) return FLASH_E_OFFSET;
	if(magic && *p != magic) return FLASH_E_MAGIC; // 0マジックは使わない
	return ReadDatFile(dst, offset, size);
}

static s32 SCSD_WriteData(u32 offset, const void* src, s32 size){
	return WriteDatFile((u16*)src, offset, size);
}

static s32 SCSD_Flush();
static s32 SCSD_EraseBlock(u32 offset, s32 size, u32 mode){
	SCSDWork* ssw = &IW->cw.scsd_work;

	// トラックログは64KBブロックを全消去すると遅くて使えないので先頭セクタのみ消去する特殊処理
	if(mode == ERASE_TRACK){
		DmaClear(3, -1, ssw->tlog_cache32, SECT_SIZE, 32); // ログキャッシュを使用
		ssw->tlog_offset = offset; // Eraseのoffsetは必ず64KBブロックにアライメントされている
		ssw->tlog_dirty = 1; // 強制書き込み指定
		return SCSD_Flush();
	}

	// SCSDではmagic消去も最小限のセクタ単位で消去する
	if(mode == ERASE_MAGIC) size = SECT_SIZE;

	// ffを書き込み
	DmaClear(3, -1, ssw->scache32, SECT_SIZE, 32);
	ssw->cache_sect = 0; // キャッシュ破壊
	for(; size > 0 ; size -= SECT_SIZE, offset += SECT_SIZE){
		s32 ret = WriteDatFile(ssw->scache16, offset, SECT_SIZE);
		if(ret) return ret;
	}
	return 0;
}

// トラックログ追記()
static s32 SCSD_AppendTLog(u8* buf, u32 size){ // size < 256 !
	// 書き込み位置計算
	SCSDWork* ssw = &IW->cw.scsd_work;
	TrackLogParam* tlp = &IW->mp.tlog;
	s32 ret = CUR_BLOCK_OFFSET(tlp) + tlp->index;
	u32 offset_hi = ret & ~(SECT_SIZE - 1); // != 0
	u32 offset_lo = ret &  (SECT_SIZE - 1);
	s32 size2 = offset_lo + size - SECT_SIZE;
	s32 size1 = (size2 > 0)? (size - size2) : size;

	// セクタ更新チェック
	if(ssw->tlog_offset != offset_hi){
		if(ssw->tlog_offset){
			// 新セクタは初期状態(ff)から開始
			ret = SCSD_Flush();
			if(ret) return ret;
			DmaClear(3, -1, ssw->tlog_cache32, SECT_SIZE, 32);
		} else {
			// 初回がアペンドの時のみSCSDからデータ読み込み
			ret = ReadDatFile(ssw->tlog_cache16, offset_hi, SECT_SIZE); // 大抵はscacheにデータが残ってるのでコピーのみ
			if(ret) return ret;
		}
		ssw->tlog_offset = offset_hi; // キャッシュオフセット != 0
	}

	// 第1セクタ書き込み
	memcpy(ssw->tlog_cache8 + offset_lo, buf, size1); // トラックログ追加
	ssw->tlog_dirty += size1; // Dirtyビットセット

	// キャッシュ書き込み判断
#define TLOG_CACHE_LIMIT (VBC_TIMEOUT(600)) // 10分に1回は強制Flush
	if(size2 >= 0 || IW->vb_counter - ssw->tlog_vbc > TLOG_CACHE_LIMIT){
		ret = SCSD_Flush();
		if(ret) return ret;
	}

	// 第2セクタ書き込み
	if(size2 >= 0){
		// 次セクタの初期化(Dirtyは必ず落ちている)
		DmaClear(3, -1, ssw->tlog_cache32, SECT_SIZE, 32);
		if(size2) memcpy(ssw->tlog_cache8, buf + size1, size2);
		ssw->tlog_offset = offset_hi + SECT_SIZE;
		if(ssw->tlog_offset >= ssw->file_size) ssw->tlog_offset = FI_TRACK_OFFSET; // ローテーションを考慮。file_sizeは正規化されている

		// 中間セクタでは、電源オフに備えて終端検出用に次セクタを予備イレースしておく
		if(tlp->index + size < BLOCK_SIZE){
			ssw->tlog_dirty = 1; // 強制書き込み指定
			ret = SCSD_Flush();
			if(ret) return ret;
		}
	}

	tlp->index += size; // 書き込みに成功したらindexを更新
	return 0;
}

// トラックログのライトキャッシュをフラッシュ
static s32 SCSD_Flush(){
	SCSDWork* ssw = &IW->cw.scsd_work;
	if(ssw->tlog_dirty && ssw->tlog_offset){
		// 書き込み必要
		s32 ret = WriteDatFile(ssw->tlog_cache16, ssw->tlog_offset, SECT_SIZE);
		if(ret) return ret;
		ssw->tlog_dirty = 0; // Dirty解除
	}
	ssw->tlog_vbc = IW->vb_counter;
	return 0;
}

const CartIF CIF_SUPERCARD_SD = {
	SCSD_InitCart,
	SCSD_ReadDirect,
	SCSD_ReadData,
	SCSD_WriteData,
	SCSD_EraseBlock,
	SCSD_AppendTLog,
	SCSD_Flush,
	Emul_GetCodeAddr,
};

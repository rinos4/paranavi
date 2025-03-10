///////////////////////////////////////////////////////////////////////////////
// 圧縮トラックログ → .trkファイル コンバータ Version 1.1 
// Copyright(C) 2006-2007 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////
// gcc -O -Wa gba2trk.cpp -o gba2trk.exe    (mingw32/gcc3.4.2)

//【圧縮トラックログフォーマット Version 0x01】
//PID Size:  L(Lat) o(Lon) A(Alt) S(Sec) R(Repeat) P(Precision) B(BCC)
// 0  1byte:                            RRRRRR00 (Repeat:R+2 [R:0-58])// 完全等速(最大1分Rep)
// 1  1byte:                            LLooAA01 (Lat:   2,A: 2,S: 0) // 慣性移動中、一定間隔
// 2  2byte:                   LLLLLLoo ooooAA10 (Lat:  32,A: 2,S: 0) // 安定移動中、一定間隔
// 3  3byte:          LLLLLLLL Looooooo ooAAA011 (Lat: 256,A: 4,S: 0) // 不安定移動、一定間隔
// 4  4byte: LLLLLLLL LLLLoooo oooooooo AAA00111 (Lat:2048,A: 4,S: 0) // 高速旋回、  一定間隔
// 5  4byte: LLLLLLLL LLLooooo ooooooAA AAA01111 (Lat:1024,A:16,S: 0) // 強サーマル、一定間隔
// 6  4byte: LLLLLLLL Looooooo ooAAAASS SSS10111 (Lat: 256,A: 8,S:16) // ログ間隔変更
// 7  5byte: L*13 o*13 A* 7 S*0          0011111 (Lat:4096,A:64,S: 0) // アクロ、    一定間隔
// 8  5byte: L*11 o*11 A* 5 S*6          0111111 (Lat:1024,A:16,S:32) // 可変間隔ログDownload
// 9  6byte: L*15 o*15 A*11 S*0          1011111 (Lat: 16K,A:1K,S: 0) // 超加速、    一定間隔
//10  7byte: L*15 o*15 A*11 S*7         01111111 (Lat: 16K,A:1K,S:64) // UnidentifiedFlyingObj
//11 15byte: L*30 o*31 A*16 S*32    PPP 11101100 絶対値パケット<継続> (P:-4～3)   [R59]
//12 15byte: L*30 o*31 A*16 S*32    PPP 11110000 絶対値パケット<新規> (セパレータ)[R60]
//13  2byte:                   BBBBBBBB 11110100 検査パケット(BCC: 0-255)         [R61]
//14  2byte: Option*8                   11111000 1byte オプション(Total:2byte)    [R62]
//15  ?byte: Option*n*8 Type*8 nnnnnnnn 11111100 nbyte オプション(Total=n+3 byte) [R63]
//16  1byte:                            11111111 未書き込み(FlashROM初期状態)

// 解像度(Lat/Lon):   P0: 1ms(約3cm)、P1: 8ms(24cm) P2: 32ms(1m) P3: 128ms(4m) 他: RFU
// 1byteオプション:   [RFU*5 ManualSeg*1 CommTimeout*1 LostFlag*1] (セグメント分割用)
// 変数初期値:        二階微分: lat/lon/alt:0, sec:+1  (絶対値パケット受信時にリセット)
//                    BCC(1byte): 0 (ヘッダまたは検査パケット受信時にリセット)
// FlashBlock:        BlockHead = 'Trk?pppb' (Magic[24] + Version[8] + Blk#[8] + Pkt#[24])
//                    Flashブロック(64KB)の先頭は、継続時でも必ずBlockHeadを挿入する(走査用)
//                    各ブロックの最初のトラックデータは絶対値パケットを使用
// 検査パケット:      Flashブロック末尾、またはサスペンド時に検査パケットを挿入(オプション)
//                    3600パケットに1回は検査パケットを挿入すべき→WriteVerifyしているので不要?


///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

typedef unsigned char u8; // ParaNavi構造体との共通化用
typedef   signed long s32;
typedef unsigned long u32;

///////////////////////////////////////////////////////////////////////////////
// .trkフォーマット出力
///////////////////////////////////////////////////////////////////////////////
// WGS84をDMSフォーマットで出力
const char TRACKLOGHEAD[] = // Kashmirの出力を参考にしてみた…
	"H  SOFTWARE NAME & VERSION\n"
	"I  PCX5 2.09\n\n"
	"H  COORDINATE SYSTEM\n"
	"U  LAT LON DMS\n\n"
	"H  R DATUM                IDX DA            DF            DX            DY            DZ\n"
	"M  G WGS 84               121 +0.000000e+00 +0.000000e+00 +0.000000e+00 +0.000000e+00 +0.000000e+00\n";

const char* MONTH[13] = {
	"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};
const time_t UNIX2GARMIN = 631033200; // UNIX秒(1970)->GARMIN秒(1990)

// 出力ファイル
static FILE* gOut = 0;

// デバッグ用
static u32 gSegFlag, gSegCount, gOption = 0xff; // デフォルトは全てのセグメントフラグで分割する
static u32 gTrackCount, gPacketCount[20];// 統計用カウンタ
const  u32 PACKET_SIZE[] = {1, 1, 2, 3, 4, 4, 4, 5, 5, 6, 7, 15, 15, 2, 1, 1};

// DMS変換
s32 DivLatLon(s32 v, s32* d, s32* m, s32* s, s32* x){
	*x = (v < 0)? -v : v;
	*s  = *x / 1000;  *m  = *s / 60;  *d  = *m / 60;
	*x %=      1000;  *s %=      60;  *m %=      60;
	return v < 0;
}

// ログ開始時間をファイル名にして新規作成(同一時刻のファイルは上書き)
bool OpenTrackLog(time_t sec){
	// UTCではなくlocaltimeをファイル名にする(Lat/Lonから地域時刻を求めたほうが良いかも)
	struct tm* t = localtime(&sec);
	char fname[30];
	sprintf(fname, "%04d%02d%02d-%02d%02d.trk", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
	gOut = fopen(fname, "w");
	if(!gOut){
		fprintf(stderr, "Can't create '%s'\n", fname);
		return false; // 作成エラー
	}

	// 統計用データ初期化
	memset(gPacketCount, 0, sizeof(gPacketCount));
	gTrackCount = 0;
	gSegCount   = 0;
	gSegFlag    = 1;
	printf( "%s: ", fname);

	// ログファイルの先頭にヘッダを入れる
	return fputs(TRACKLOGHEAD, gOut) >= 0;// 書き込みチェック
}

// lat/lon/alt/sec -> .trkコラム変換
bool WriteTrackLog(const s32* v, s32 prc){
	if(!gOut) return false; // 念のためチェック

	// 時刻変換
	time_t sec = v[3] + UNIX2GARMIN;
	gTrackCount++; // 統計表示用にインクリメントしておく

	if(gSegFlag){ // セグメント分割フラグ
		struct tm* t = localtime(&sec); // セグメント名にはローカルタイムを使う
		fprintf(gOut, "\nH %04d/%02d/%02d(#%d)\n\n", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, ++gSegCount);
		gSegFlag = 0; // セグメント追加済みマーク
	}

	// 緯度と経度は精度値でシフトして出力
	s32 d, m, s ,x, f;
	f = DivLatLon(v[0] << prc, &d, &m, &s, &x);
	fprintf(gOut, "T  %c%02d%02d%02d.%03d", f? 'S' : 'N', d, m, s, x);	// 緯度
	f = DivLatLon(v[1] << prc, &d, &m, &s, &x);
	fprintf(gOut,   " %c%03d%02d%02d.%03d", f? 'W' : 'E', d, m, s, x);	// 経度

	// トラックはUTCで出力
	struct tm* utc = gmtime(&sec);
    return fprintf(gOut, " %02d-%s-%02d %02d:%02d:%02d %d\n", utc->tm_mday, MONTH[utc->tm_mon], utc->tm_year % 100, utc->tm_hour, utc->tm_min, utc->tm_sec, v[2]) > 0; // 書き込みチェック
}

// ファイルへフラッシュ
bool CloseTrackLog(){
	if(!gOut) return true; // 既にクローズ済

	// ファイルクローズチェック
	if(fclose(gOut)){
		fprintf(stderr, "fclose: failed\n");
		return false;
	}
	gOut = 0;

	// 統計情報を表示しておく
	if(gTrackCount){
		s32 sum = 0;
		for(int i = 0 ; i < 16 ; ++i) sum += PACKET_SIZE[i] * gPacketCount[i];
		printf("Track=%d Segment=%d (%.2fbyte/trk)\n", gTrackCount, gSegCount, sum / (double)gTrackCount);
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// 圧縮トラックデータデコード
///////////////////////////////////////////////////////////////////////////////
typedef struct{
	u32 bit[4];		// [lat/lon/alt/sec]
	u8  flag;		// 判別コード
	u8  mask;		// 判別マスク
	u8  mask_size;	// 判別サイズ
	s32 pid;
} PacketType; // デコードに必要なデータのみ

#define BitFill(n)  ((1 <<  (n)) - 1)
#define BitRange(n) ( 1 << ((n) - 1))
#define DEF_PKT(lat, lon, alt, sec, flag, mask, pid) {lat, lon, alt, sec, flag, BitFill(mask), mask, pid }
const PacketType PACKET_TABLE[] = {
	//     lat lon alt sec   flag ms pid
	DEF_PKT( 2,  2,  2,  0,  0x01, 2, 1),
	DEF_PKT( 6,  6,  2,  0,  0x02, 2, 2),
	DEF_PKT( 9,  9,  3,  0,  0x03, 3, 3),
	DEF_PKT(12, 12,  3,  0,  0x07, 5, 4),
	DEF_PKT(11, 11,  5,  0,  0x0f, 5, 5),
	DEF_PKT( 9,  9,  4,  5,  0x17, 5, 6),
	DEF_PKT(13, 13,  7,  0,  0x1f, 7, 7),
	DEF_PKT(11, 11,  5,  6,  0x3f, 7, 8),
	DEF_PKT(15, 15, 11,  0,  0x5f, 7, 9),
	DEF_PKT(15, 15, 11,  7,  0x7f, 8, 10),
	DEF_PKT(30, 31, 16, 32,  0xec, 8, 11),
	DEF_PKT(30, 31, 16, 32,  0xf0, 8, 12),
	DEF_PKT( 0,  0,  0,  0,  0xf4, 8, 13),
	DEF_PKT( 0,  0,  0,  0,  0xf8, 8, 14),
	DEF_PKT( 0,  0,  0,  0,  0xfc, 8, 15),
	DEF_PKT( 0,  0,  0,  0,  0xff, 8, 16),
	DEF_PKT( 0,  0,  0,  0,  0x00, 2, 0),
	DEF_PKT( 0,  0,  0,  0,  0,    0, 0), // END(ここに到達することはないが…)
};
const s32 PRECISION_TABLE[8] = { // 保存解像度
	0, 3, 5, 7, 0, 0, 0, 0 // 1ms/8ms/32ms/128ms, 他はデフォルトの1ms(3cm)とする
};

// ビット取り出し
static inline s32 SgnShift(u32 val, s32 shift){
	return (shift < 0)? (val >> -shift) : (val << shift);
}
s32 UnpackBit(u8** buf, s32* cur_bit, s32 bit_size){
	if(!bit_size) return 0; // 取り出し不要
	s32 ret = 0, shift = -*cur_bit, bs = bit_size;
	while(bs){
		s32 next = *cur_bit + bs;
		if(next > 8) next = 8;
		ret |= SgnShift(**buf & BitFill(next) & ~BitFill(*cur_bit), shift);
		bs -= next - *cur_bit;
		*cur_bit = next & 7;
		shift += 8;
		if(next == 8) ++*buf;
	}
	if(ret & BitRange(bit_size)) ret |= ~BitFill(bit_size); // 負数拡張
	return ret;
}

// デコーダ
s32 DecodeTrackLog(u8* buf, u8* end){
	s32 dif[4], val[4], prc = 0, i, j;
	while(buf < end){
		// PID検索(とりあえずループで。256type検索テーブルが良いかも)
		const PacketType* pt = PACKET_TABLE;
		while((pt->mask & *buf) != pt->flag) ++pt;
		gPacketCount[pt->pid]++; // 統計表示用にカウントしておく

		// PID別処理
		switch(pt->pid){
		case 0: // リピートパケット
			for(j = (*buf++ >> 2) + 2 ; j-- ;){
				for(i = 4 ; i-- ;) val[i] += dif[i];
				WriteTrackLog(val, prc);
			}
			break;
		case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10: // 二階微分パケット
			j = pt->mask_size;
			for(i = 4 ; i-- ;) val[i] += dif[i] += UnpackBit(&buf, &j, pt->bit[i]);
			WriteTrackLog(val, prc);
			break;
		case 12: // 絶対値パケット<新規>
			if(!CloseTrackLog()) return 2; // FileSystemFullとか?
			// no break　(セパレート処理以外は<継続>と同じ)
		case 11: // 絶対値パケット<継続>
			j = 0;
			prc = PRECISION_TABLE[UnpackBit(&++buf, &j, 3) & 7]; // 保存解像度取得
			for(i = 4 ; i-- ;) val[i] = UnpackBit(&buf, &j, pt->bit[i]);
			dif[0] = dif[1] = dif[2] = 0;	// 微分値を初期化
			dif[3] = 1;						// secだけ1が初期値
			if(!gOut && !OpenTrackLog(val[3] + UNIX2GARMIN)) return 1; // 新規作成失敗?
			WriteTrackLog(val, prc);
			break;
		case 13: // BCC(とりあえずBCCエラーは無視して、できるだけデコードしてみる)
			buf += 2;
			break;
		case 14: // 1byte コマンド
			gSegFlag |= buf[1] & gOption; // 次パケットでセグメント分割(オプション選択)
			buf += 2;
			break;
		case 15: // n byte コマンド
			gPacketCount[pt->pid] += buf[1] + 2; // オプションは可変サイズ
			buf += (u32)buf[1] + 3;
			break;
		default:
			return 0; // 終端マーク
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// フラッシュROMブロック走査
///////////////////////////////////////////////////////////////////////////////
#define FLBL_TRACK_MAGIC	0x016b7254	// トラックマジック("Trk?")
#define BLOCK_SIZE			(64 * 1024)
#define BLOCK_HEAD_SIZE		8
#define BLOCK_TRACK_SIZE	(BLOCK_SIZE - BLOCK_HEAD_SIZE)
typedef struct{
	u32 magic; // ログ判別用
	s32 index; // ブロック走査で重要なのは上位の8bit(下位24bitのPacketCounterはどうでもいい)
	u8 val[BLOCK_TRACK_SIZE];
} TrackBlock;

// 64KBブロック単位で走査
int SearchTrackLogBlock(u8* buf, u32 size){
	TrackBlock* tb = (TrackBlock*)buf;
//	size /= BLOCK_SIZE;
	size = (size + BLOCK_SIZE - 1) / BLOCK_SIZE; // FIX1.1

	// スタートブロックの検索
	u32 start = 0, i;
	while(tb[start].magic != FLBL_TRACK_MAGIC){
		if(++start >= size){ // 64KB未満の特殊データを考慮して#0ブロックはサイズ検査なし(#1以降はここで検査)
			fprintf(stderr, "Can't find TrackLog block\n"); // 1つもトラックログが見つからなかった
			return 1;
		}
	}
	// 上書きモードのローテーションを考慮してスタートをシフトする
	for(i = start; ++i < size ;){
		if(tb[i].magic == FLBL_TRACK_MAGIC && tb[i].index - tb[start].index < 0){
			start = i;
			break;
		}
	}
	// スタートブロックから順番に書き出し
	i = start;
	do {
		if(tb[i].magic == FLBL_TRACK_MAGIC) DecodeTrackLog(tb[i].val, tb[i].val + BLOCK_TRACK_SIZE);
		if(++i >= size) i = 0;
	} while(i != start);
	return CloseTrackLog()? 0 : 1; // ファイルクローズエラーチェック
}

///////////////////////////////////////////////////////////////////////////////
// 圧縮トラックデータ読み込み
///////////////////////////////////////////////////////////////////////////////
static u8 gBuf[1024 * 1024]; // とりあえず1MBをグローバルに…

int main(int argc, char** argv){
	switch(argc){
	case 3:
		gOption = strtoul(argv[2], 0, 16);
		// no break;
	case 2:
		if(argv[1][0] != '-') break;
		// no break
	default:
		fprintf(stderr, "usage: %s file [segment_option]\n", *argv);
		return 1;
	}

	// ログ読み込み
	FILE* in = fopen(argv[1], "rb");
	if(!in){
		fprintf(stderr, "Can' open %s\n", argv[1]);
		return 2;
	}
	size_t size = fread(gBuf, 1, sizeof(gBuf), in); // ROM(1MB)を丸ごと読み込む
	fclose(in);

	// デコード
	memset(gBuf + size, -1, sizeof(gBuf) - size);// 64KB未満の特殊データを考慮して後半をPID:16で埋める
	return SearchTrackLogBlock(gBuf, size);
}

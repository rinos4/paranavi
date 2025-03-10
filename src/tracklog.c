///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2006 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// トラックログの圧縮/展開ロジック、ファイル書き込み制御をこのファイルで定義

///////////////////////////////////////////////////////////////////////////////
// トラックログ
///////////////////////////////////////////////////////////////////////////////
//【圧縮トラックログフォーマット Version 0x01】
//PID Size:  Detail= L(Lat) o(Lon) A(Alt) S(Sec) R(Repeat) P(Precision) B(BCC)
// 0  1byte:                            RRRRRR00 (Repeat:R+2 [R:0-58])// 完全等速(最大1分Rep)
// 1  1byte:                            LLooAA01 (Lat:   2,A: 2,S: 0) // 慣性移動中、一定間隔
// 2  2byte:                   LLLLLLoo ooooAA10 (Lat:  32,A: 2,S: 0) // 安定移動中、一定間隔
// 3  3byte:          LLLLLLLL Looooooo ooAAA011 (Lat: 256,A: 4,S: 0) // 不安定移動、一定間隔
// 4  4byte: LLLLLLLL LLLLoooo oooooooo AAA00111 (Lat:2048,A: 4,S: 0) // 高速旋回、  一定間隔
// 5  4byte: LLLLLLLL LLLooooo ooooooAA AAA01111 (Lat:1024,A:16,S: 0) // 強サーマル、一定間隔
// 6  4byte: LLLLLLLL Looooooo ooAAAASS SSS10111 (Lat: 256,A: 8,S:16) // ログ間隔変更
// 7  5byte: L*13 o*13 A* 7 S*0          0011111 (Lat:4096,A:64,S: 0) // アクロ、    一定間隔
// 8  5byte: L*11 o*11 A* 5 S*6          0111111 (Lat:1024,A:16,S:32) // 可変間隔ログ
// 9  6byte: L*15 o*15 A*11 S*0          1011111 (Lat: 16K,A:1K,S: 0) // 超加速、    一定間隔
//10  7byte: L*15 o*15 A*11 S*7         01111111 (Lat: 16K,A:1K,S:64) // UnidentifiedFlyingObj
//11 15byte: L*30 o*31 A*16 S*32    PPP 11101100 絶対値パケット<継続> (P:-4～3)   [R59]
//12 15byte: L*30 o*31 A*16 S*32    PPP 11110000 絶対値パケット<新規> (セパレータ)[R60]
//13  2byte:                   BBBBBBBB 11110100 検査パケット(BCC: 0-255)         [R61]
//14  2byte: Option*8                   11111000 1byte オプション(Total:2byte)    [R62]
//15  ?byte: Option*n*8 Type*8 nnnnnnnn 11111100 nbyte オプション(Total=n+3 byte) [R63] RFU
//16  1byte:                            11111111 未書き込み(FlashROM初期状態)
//    8byte:Magic[24] + Version[8] + Blk#[8] + Pkt#[24]) 'Trk?pppb'  // Flashブロックヘッダ

// 記録精度(Lat/Lon): P:0 1ms(約3cm)、P:1 8ms(24cm) P:2 32ms(96cm) P:3 128ms(4m) P:他 RFU
// 1byteオプション:   [RFU*4 ManualSeg*1 CommTimeout*1 LostFlag*1] (セグメント分割用)
// 変数初期値:        二階微分: lat/lon/alt:0, sec:+1  (絶対値パケット受信時にリセット)
//                    BCC(1byte): 0 (ヘッダまたは検査パケット受信時にリセット)
// Flashブロック:     BLOCK_HEAD [PID#14-15]* PID#11-12 [PID#0-15]* [PID#16] (最大64KB)
//                    Flashブロック(64KB)の先頭は、継続時でも必ずブロックヘッダを挿入する(走査用)
//                    各ブロックの最初のトラックデータは上書きモードに備えて絶対値パケットを使用
// 検査パケット:      Flashブロック末尾、またはサスペンド時に検査パケットを挿入(オプション)
//                    少なくとも3600パケットに1回は検査パケットを挿入すべき→イラナイ?

// 注意事項:
// Repeat中はFlashへの書き込みは行なわないため、等速加速中に突然電源OFFすると最大1分間ロストする


typedef struct{
	u32 packet_size;
	u32 bit[4];		// [lat/lon/alt/sec]
	s32 range[4];	// [lat/lon/alt/sec]
	u8  flag;
	u8  mask;
	u8  mask_size;
	u32 pid;
} PacketType2;

#define BitFill(n)  ((1 <<  (n)) - 1) // n=32でオーバーフロー
#define BitRange(n) ((n)? (1 << ((n) - 1)) : 0)
#define BitMask(n)  (-1 >> (32 - n))
#define DEF_CMP(lat, lon, alt, sec, flag, masks, pid) {\
			(lat + lon + alt + sec + masks + 7) / 8, \
			{lat, lon, alt, sec}, \
			{BitRange(lat), BitRange(lon), BitRange(alt), BitRange(sec)}, \
			flag, BitFill(masks), masks, pid \
		}

const PacketType2 PACKET_TABLE[] = {
	//     lat lon alt sec   flag mask
	DEF_CMP( 2,  2,  2,  0,  0x01, 2, 1),
	DEF_CMP( 6,  6,  2,  0,  0x02, 2, 2),
	DEF_CMP( 9,  9,  3,  0,  0x03, 3, 3),
	DEF_CMP(12, 12,  3,  0,  0x07, 5, 4),
	DEF_CMP(11, 11,  5,  0,  0x0f, 5, 5),
	DEF_CMP( 9,  9,  4,  5,  0x17, 5, 6),
	DEF_CMP(13, 13,  7,  0,  0x1f, 7, 7),
	DEF_CMP(11, 11,  5,  6,  0x3f, 7, 8),
	DEF_CMP(15, 15, 11,  0,  0x5f, 7, 9),
	DEF_CMP(15, 15, 11,  7,  0x7f, 8, 10),
	DEF_CMP(30, 31, 16, 32,  0xec, 8, 11), // END=11

	// デコード用(終端検索)
	DEF_CMP(30, 31, 16, 32,  0xf0, 8, 12),
	DEF_CMP( 0,  0,  0,  0,  0xf4, 8, 13),
	DEF_CMP( 0,  0,  0,  0,  0xf8, 8, 14),
	DEF_CMP( 0,  0,  0,  0,  0xfc, 8, 15),
	DEF_CMP( 0,  0,  0,  0,  0xff, 8, 16),
	DEF_CMP( 0,  0,  0,  0,  0x00, 2, 0),
	DEF_CMP( 0,  0,  0,  0,  0,    0, 16), // END(ここに到達することはないが…)
};

#define END_INDEX 11

const PacketType2 ABS_TABLE[] = {
	DEF_CMP(30, 31, 16, 32,  0xec, 8, 11), // 個別指定用
};
const PacketType2 SEP_TABLE[] = {
	DEF_CMP(30, 31, 16, 32,  0xf0, 8, 12), // 個別指定用
};

const s32 PRECISION_TABLE[8] = { // 保存精度
	0, 3, 5, 7, 0, 0, 0, 0 // 1ms/8ms/32ms/128ms, 他はデフォルトの1ms(3cm)とする
};

#define MAX_REPEAT 58
const u32 PACKET_SIZE[] = {1, 1, 2, 3, 4, 4, 4, 5, 5, 6, 7, 15, 15, 2, 2, 3, 1};

#define OPT_1byte 0xf8
#define OPT_Nbyte 0xfc


static s32 BldRepeat(TrackLogParam* tlp, u8* buf);


///////////////////////////////////////////////////////////////////////////////
// ログ書き込み処理
///////////////////////////////////////////////////////////////////////////////
// 1バイトづつシフトさせる
void ByteShift(u8* buf, u32 size){
	while(size--) buf[size + 1] = buf[size];// memmovの代わり
}

// 次のブロックへ進む(チェック付き)
static s32 GotoNextBlock(TrackLogParam* tlp){
	if(tlp->index){
		tlp->index = 0;
		tlp->block++;
	}
	if(tlp->block >= IW->cw.tlog_block) tlp->block = 0; // 巻き戻し
	tlp->abs_flag |= ABSF_CONTINUE; // 次のデータパケットは絶対値フォーマットが必要
	if(!IW->tc.log_overwrite){
		if(*IW->cif->ReadDirect(CUR_BLOCK_OFFSET(tlp)) == FLBL_TRACK_MAGIC){
			tlp->full = 1;
			return FLASH_E_LOG_FULL;
		}
	}
	tlp->full = 0;
	return FLASH_SUCCESS;
}

// トラックログの追記。bufは2byteアライメント。2byteの遊びがあること!
u32 AppendTrackLog(TrackLogParam* tlp, u8* buf, u32 buf_size){
	if(!buf_size) return FLASH_SUCCESS; // いちおうチェック…

	// 停止モードから上書きモードへの変更用チェック
	s32 ret;
	if(tlp->index >= BLOCK_SIZE || tlp->full){ // エラー時のリカバリ
		ret = GotoNextBlock(tlp);
		if(ret) return ret; // FULL
	
		// ログ停止中にロストしたrepeatはあきらめる
		tlp->repeat = 0;
	}

	// リピートチェック
	if(tlp->repeat){
		tlp->pkt_counter++; // 書き込みカウンタ
		if(tlp->index + buf_size < BLOCK_SIZE){
			// 現在のバッファに便乗書き込みしても足りる
			ByteShift(buf, buf_size++);
			BldRepeat(tlp, buf); // repeatフラグはこの中で落ちる
		} else {
			// リピートパケットで終端(tlp->index=0はありえない)
			u8 rep[2]; // +1はアライメント用マージン
			BldRepeat(tlp, rep); // repeatフラグはこの中で落ちる
			ret = IW->cif->AppendTLog(rep, 1); // リピートのみで書き込む(1byteは絶対空きがある)
			if(ret) return ret;
		}
	}

	// 書き込みブロック計算
	if(tlp->index + buf_size > BLOCK_SIZE){
		ret = GotoNextBlock(tlp); // このブロックは空きが足りないので次のブロックへ進む(TODO BCC追加)
		if(ret) return ret; // フル
	}

	// ブロック最初の書き込み?
	if(!tlp->index){
		// ブロックの消去
		ret = IW->cif->EraseBlock(CUR_BLOCK_OFFSET(tlp), BLOCK_SIZE, ERASE_TRACK);
		if(ret) return ret;

		// ヘッダ書き込み
		TrackLogHeader tlh;
		tlh.magic = FLBL_TRACK_MAGIC;
		tlh.index = (tlp->pkt_counter & 0x00ffffff) | (++tlp->blk_counter << 24);
		ret = IW->cif->AppendTLog((u8*)&tlh, sizeof(tlh));
		if(ret) return ret; // エラー検出

		tlp->abs_flag |= ABSF_CONTINUE; // 次のデータパケットは絶対値フォーマットが必要
		return FLASH_E_LOG_NEWBLOCK; // 新しいブロックに切り替わったことを通知(絶対値パケットの要求)
	} 

	// 書き込み
	tlp->pkt_counter++; // 書き込みカウンタ
	ret = IW->cif->AppendTLog(buf, buf_size);
	if(tlp->index >= BLOCK_SIZE) GotoNextBlock(tlp); // 一杯になったら次へ
	return ret;
}


///////////////////////////////////////////////////////////////////////////////
// 圧縮ロジック
///////////////////////////////////////////////////////////////////////////////
static inline s32 SgnShift(s32 val, s32 shift){
	return (shift < 0)? (val >> -shift) : (val << shift);
}
void PackBit(u8** buf, u32* cur_bit, s32 bit_size, s32 val){
	s32 shift = *cur_bit;
	while(bit_size > 0){
		u32 next = *cur_bit + bit_size;
		if(next > 8) next = 8;
		**buf |= (u8)(SgnShift(val, shift) & BitFill(next) & ~BitFill(*cur_bit));
		if(next < 8){
			*cur_bit = next;
			break;
		}
		bit_size -= next - *cur_bit;
		shift    -= 8;
		*cur_bit  = 0;
		++*buf;
	}
}

// リピートパケット作成
static s32 BldRepeat(TrackLogParam* tlp, u8* buf){
	switch(tlp->repeat){
	case 0:
		return 0; //!?
	case 1:
		*buf = 0x1;// PID#1 ゼロ加速x1
		tlp->t_count[1]++; // PID#1追加
		break;
	default: // リピート
		*buf = (tlp->repeat - 2) << 2;
		tlp->t_count[0]++; // PID#0追加
	}
	tlp->repeat = 0;
	return 1;
}

// 絶対値パケット作成
static s32 BldAbsolute(TrackLogParam* tlp, u8* buf, const PacketType2* p, const TrackLog* tl){
	u8* start_buf = buf;
	memset(buf, 0, 15);

	u32 bit = 0, i;
	PackBit(&buf, &bit, p->mask_size + 3, p->flag | ((tlp->pre_prec & 7) << 8));
	for(i = 4 ; i-- ;) PackBit(&buf, &bit, p->bit[i], tl->val[i]);
	tlp->dif[0] = tlp->dif[1] = tlp->dif[2] = 0;
	tlp->dif[3] = 1;// secだけ1がデフォルト
	tlp->t_count[p->pid]++; // PID追加
	tlp->abs_flag = 0; // 以降、微分パケットOK
	return buf - start_buf;
}

// 二階微分パケット作成
static s32 BldDiff(TrackLogParam* tlp, u8* buf, const PacketType2* p, const TrackLog* tl, s32* dif2){
	u8* start_buf = buf;
	// 二階微分パケット
	memset(buf, 0, p->packet_size);
	s32 bit = 0, i;
	PackBit(&buf, &bit, p->mask_size, p->flag);
	for(i = 4 ; i-- ;){
		PackBit(&buf, &bit, p->bit[i], dif2[i]);
		tlp->dif[i] = tl->val[i] - tlp->pre[i];
	}
	tlp->t_count[p->pid]++; // PID追加
	return buf - start_buf;
}

static u32 EncodeTrackLog(TrackLogParam* tlp, TrackLog* tl, u8* buf){
	// 差分計算
	s32 dif2[4], i, same_flag = 1;
	for(i = 4 ; i-- ;){
		dif2[i] = tl->val[i] - tlp->pre[i] - tlp->dif[i];
		if(dif2[i]) same_flag = 0;
	}

	// リピートチェック
	if(same_flag){
		if(tlp->repeat++ < MAX_REPEAT) return 0;
		return BldRepeat(tlp, buf); // リピートパケット確定、完了
	}

	// 圧縮パケットビルド
	const PacketType2* p = PACKET_TABLE;
	for(; p->pid < END_INDEX ; ++p){
		for(i = 4 ; i-- ;){
			if(dif2[i] && (dif2[i] < -p->range[i] || p->range[i] <= dif2[i])) break;
		}
		if(i < 0) return BldDiff(tlp, buf, p, tl, dif2); // このPIDで圧縮可
	}

	// 絶対値パケット
	return BldAbsolute(tlp, buf, p, tl);
}

s32 ErrorLog(TrackLogParam* tlp, s32 err){
	tlp->err = err; // 一応記録
	tlp->drop++;
	tlp->abs_flag |= ABSF_CONTINUE; //エラー後は絶対値パケットで復旧させる
	tlp->repeat = 0; // リピートもロスト
	return err;
}

//圧縮ログ書き込み
u32 TrackLogAdd(TrackLog* tl){
	TrackLogParam* tlp = &IW->mp.tlog;
	if(!IW->cw.tlog_block) return 1; // 領域無し

	tlp->trk_counter++; // デバッグ用カウンタ
	IW->mp.tlog.tc_state = 0; // 再計算要求
	IW->mp.tlog.tc_lastadd = IW->vb_counter;

	// 書き込み精度の変更検出
	if(tlp->pre_prec != IW->tc.log_prec){
		tlp->pre_prec = IW->tc.log_prec;
		tlp->abs_flag |= ABSF_CONTINUE; // 絶対値パケットで精度変更を通知
	}
	s32 t= PRECISION_TABLE[tlp->pre_prec & 7];
	if(t){
		tl->val[0] = RoundShift(tl->val[0], t); // lat
		tl->val[1] = RoundShift(tl->val[1], t); // lon
	}

	u32 buf32[80 / 3]; // PVT=64byte アライメント用マージン付き
	u8* buf = (u8*)buf32;
	u32 buf_size = 0;

	// オプション書き込み
	if(tlp->opt & TLOG_OPT_ENABLE){
		buf[0] = OPT_1byte;
		buf[1] = tlp->opt & TLOG_OPT_MASK;
		tlp->t_count[14]++; // PID#14(1byteオプション)追加
		t = AppendTrackLog(tlp, buf, 2); // 2byteオプション
		if(t == FLASH_E_LOG_NEWBLOCK) t = AppendTrackLog(tlp, buf, 2); // オプションはそのまま次ブロックへ書き込み
		if(t) return ErrorLog(tlp, t);
		tlp->opt = 0; // オプションクリア
	}

	if(IW->tc.log_debug == 1){ // PVTデバッグ
#define PVTRAW_SIZE (sizeof(D800_Pvt_Data_Type) + 3)
		buf[0] = OPT_Nbyte;
		buf[1] = sizeof(D800_Pvt_Data_Type);
		buf[2] = 0;
		memcpy(buf + 3, &IW->pvt_raw, sizeof(D800_Pvt_Data_Type));
		tlp->t_count[15] += PVTRAW_SIZE;
		t = AppendTrackLog(tlp, buf, PVTRAW_SIZE);
		if(t == FLASH_E_LOG_NEWBLOCK) t = AppendTrackLog(tlp, buf, PVTRAW_SIZE); // オプションはそのまま次ブロックへ書き込み
		if(t) return ErrorLog(tlp, t);

		static s32 sPreTime = 0;
		if(sPreTime && tl->val[3] - sPreTime < 1){
			tlp->t_count[13] ++; // 異常タイム検出
		}
		sPreTime = tl->val[3];
	}


	// トラックデータ書き込み
	if(tlp->abs_flag) buf_size = BldAbsolute(tlp, buf, (tlp->abs_flag & ABSF_SEPARETE)? SEP_TABLE : ABS_TABLE, tl);
	else 			  buf_size = EncodeTrackLog(tlp, tl, buf);

	if(buf_size){
		t = AppendTrackLog(tlp, buf, buf_size);
		if(t == FLASH_E_LOG_NEWBLOCK){
			// 絶対値パケットに変換
			if(buf_size != 15) buf_size = BldAbsolute(tlp, buf, ABS_TABLE, tl);
			else               tlp->abs_flag = 0; // 既に絶対値パケットを使用
			t = AppendTrackLog(tlp, buf, buf_size);
		}
		if(t) return ErrorLog(tlp, t);
	}

	// 現在値を更新しておく
	s32 i;
	for(i = 4 ; i-- ;) tlp->pre[i] = tl->val[i];
	return 0;
}

// 現在のPVTをトラックに追加
u32 TrackLogAddCurrentPVT(){
	// トラックログのダウンロード中は、現在のPVTが混ざらないようにガードする
	if(IW->tc.log_enable && IW->gi.state != GPS_TRACK && IW->gi.state != GPS_TRACK_WAIT){
		if(IW->px.fix > FIX_INVALID){
			if(++IW->mp.tlog.intvl_ctr >= IW->tc.log_intvl){
				IW->mp.tlog.intvl_ctr = 0;
				TrackLog tl;
				tl.val[0] = IW->px.lat;
				tl.val[1] = IW->px.lon;
				tl.val[2] = RoundDiv(IW->px.alt_mm, 1000); // 高度は1m精度に落とす
				tl.val[3] = IW->px.dtime;
				return TrackLogAdd(&tl);
			}
		} else {
			IW->mp.tlog.opt |= TLOG_OPT_LOST; // INVALID受信
		}
	}
	return 0; // 追加するものなし
}

// 全てのトラックログを消去する(マジックのみ消去の高速版)
u32 TrackLogClear(){
	u32 i, offset = FI_TRACK_OFFSET;
	for(i = 0 ; i < IW->cw.tlog_block ; ++i, offset += BLOCK_SIZE){
		if(*IW->cif->ReadDirect(offset) == FLBL_TRACK_MAGIC){
			s32 ret = IW->cif->EraseBlock(offset, BLOCK_SIZE, ERASE_MAGIC);
			if(ret) return ret;
		}
	}
	InitLogParams();
	return 0;
}

#define SECT_MASK 0x1ff

u8 GetLogByte(u32 offset, u32* pre_sect, u8** ptr){
	u32 sect = offset & ~SECT_MASK;
	if(*pre_sect != sect){
		*pre_sect = sect;
		*ptr = (u8*)IW->cif->ReadDirect(sect);
	}
	return (*ptr)[offset & SECT_MASK];
}

// 書き込み済みのトラックログの終端オフセットを探す。(tlp->blockに検索ブロックをセット)
// 戻り値はトラックログ数(ログPC転送用。パケット数ではないので注意！)
u32 SearchTrackEnd(TrackLogParam* tlp){
	tlp->index = 0;
	if(IW->cw.tlog_block <= tlp->block) return 0;//領域外

	u32 start = CUR_BLOCK_OFFSET(tlp);
	u32 pre_sect = -1;
	u8* p;
	if(*IW->cif->ReadDirect(start) != FLBL_TRACK_MAGIC) return 0;// 何もかかれていない

	u32 i = BLOCK_HEAD_SIZE;
	u32 ret = 0;
	while(i < BLOCK_SIZE){
		// PID検索(とりあえずループで。256type検索テーブルが良いかも)
		const PacketType2* pt = PACKET_TABLE;
		u8 code = GetLogByte(i + start, &pre_sect, &p);
		while((pt->mask & code) != pt->flag) ++pt;
		if(pt->pid == 16) break;
		if(pt->pid == 15){
			i += GetLogByte(i + start + 1, &pre_sect, &p); // 可変サイズオプション
		} else if(!pt->pid){
			ret += (GetLogByte(i + start + 1, &pre_sect, &p) >> 2) + 2;
		} else if(pt->pid == 12){
			ret += 2;
		} else if(pt->pid < 13){
			ret++;
		}
		i += PACKET_SIZE[pt->pid];
		tlp->pkt_counter++; // ここでカウントアップしておく
	}
	tlp->index = i;
	return ret;
}

// ログ書き込み先を初期化
void InitLogParams(){
	TrackLogParam* tlp = &IW->mp.tlog;
	DmaClear(3, 0, tlp, sizeof(TrackLogParam), 32);
	tlp->abs_flag = ABSF_SEPARETE; //セパレータ

	if(!IW->cw.tlog_block) return;

	// スタートブロックの検索
	const u32* tb = 0;
	u32 end = IW->cw.tlog_block;
	for(tlp->block = 0 ; tlp->block < end ; tlp->block++){
		tb = IW->cif->ReadDirect(CUR_BLOCK_OFFSET(tlp));
		if(!tb) return; // エラー!?
		if(tb[0] == FLBL_TRACK_MAGIC) break;
	}
	if(tlp->block == end){
		tlp->block = 0;
		return; // ログなし
	}

	s32 pre_index = tb[1];
	for(end-- ; tlp->block < end ; tlp->block++){
		tb = IW->cif->ReadDirect(CUR_BLOCK_OFFSET(tlp) + BLOCK_SIZE);
		if(!tb) return; // エラー!?
		if(tb[0] != FLBL_TRACK_MAGIC || pre_index - (s32)tb[1] > 0) break;
		pre_index = tb[1];
	}
	tlp->blk_counter = ((u32)pre_index) >> 24;
	tlp->pkt_counter = ((u32)pre_index) & 0xffffff;

	// 終端を探す
	SearchTrackEnd(tlp);
	if(tlp->index >= BLOCK_SIZE) GotoNextBlock(tlp);//ここではエラーログ不要
}


///////////////////////////////////////////////////////////////////////////////
// NMEAセンテンスをデバッグ用に保存
///////////////////////////////////////////////////////////////////////////////
u32 AppendNMEALog(){
	if(IW->cw.tlog_block) return 1; //領域なし
	
	// 可変サイズオプションヘッダ作成
	TrackLogParam* tlp = &IW->mp.tlog;
	u32 len = IW->dl_size;
#define MAX_NMEA_LOG (256 - 4)
	if(len > MAX_NMEA_LOG) len = MAX_NMEA_LOG;
	u8* head = IW->dl_loghead + 1;
	head[0] = OPT_Nbyte;
	head[1] = len;
	head[2] = 0;

	// 書き込み
	len += 3;
	tlp->t_count[15] += len;
	s32 t = AppendTrackLog(tlp, head, len);
	if(t == FLASH_E_LOG_NEWBLOCK) t = AppendTrackLog(tlp, head, len);
	return t;
}


///////////////////////////////////////////////////////////////////////////////
// ログPC転送用
///////////////////////////////////////////////////////////////////////////////
// ビット取り出し
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

// 先頭ブロックをサーチ
s32 FindFirstBlock(){
	TrackLogParam* tlp = &IW->mp.tlog;
	u32 block = tlp->block;
	if(IW->cw.tlog_block <= block) return 0; // とりあえず無効値の場合は先頭を返しておく

	while(*IW->cif->ReadDirect(FI_TRACK_OFFSET + block * BLOCK_SIZE) == FLBL_TRACK_MAGIC){
		if(!block--) block = IW->cw.tlog_block - 1;
		if(block == tlp->block) break; // 最後のブロックに戻った
	}
	if(++block < IW->cw.tlog_block) return block; // 無効トラックの1つ先が最初の有効ブロック
	return 0; // 先頭が最初のブロック
}

// 次のトラックに進める
s32 NextTrack(s32 countmode){
	TrackInfo* ti = &IW->gi.tinfo;
	ti->seg = 0; // セグメントマークを初期化
	s32 i, j;
	char dbuf[20];
	int timeout = 0;
	for(;;){
		if(++timeout > 0x10000) break; // 念のため…
		// リピートパケット継続
		if(ti->rep){
			ti->rep--;
			for(i = 4 ; i-- ;) ti->val[i] += ti->dif[i];
			return 1;
		}

		// ブロック切り替え処理
		if((ti->tpos & (BLOCK_SIZE - 1)) < BLOCK_HEAD_SIZE){
			if((ti->tpos >> 16) >= IW->cw.tlog_block) ti->tpos = 0; // 巻き戻し
			else ti->tpos &= ~(BLOCK_SIZE - 1); // 念のため先頭に再セット

			// ログなしブロックならここで停止する
			if(*IW->cif->ReadDirect(FI_TRACK_OFFSET + ti->tpos) != FLBL_TRACK_MAGIC) return 0;

			ti->tpos += BLOCK_HEAD_SIZE; // 開始位置調整
		}

		// PID検索(とりあえずループで。256type検索テーブルが良いかも)
		*dbuf = GetLogByte(FI_TRACK_OFFSET + ti->tpos, &ti->pre_sect, &ti->sect_ptr);
		const PacketType2* pt = PACKET_TABLE;
		while((pt->mask & *dbuf) != pt->flag) ++pt;

		// 終端パケットチェック
		if(pt->pid == 16){
			if((ti->tpos >> 16) == IW->mp.tlog.block) break; // 最終ブロック

			//次ブロックへ
			ti->tpos = (ti->tpos + BLOCK_SIZE) & ~(BLOCK_SIZE - 1);
			continue;
		}

		// パケットサイズ分のデータ取り込み
		u32 size = PACKET_SIZE[pt->pid];
		for(i = 1 ; i < size && i < sizeof(dbuf) ; ++i){
			dbuf[i] = GetLogByte(FI_TRACK_OFFSET + ti->tpos + i, &ti->pre_sect, &ti->sect_ptr);
		}
		ti->tpos += size;

		// PID別処理
		u8* buf = dbuf;
		switch(pt->pid){
		case 0: // リピートパケット
			i = (*buf++ >> 2) + 2;
			if(countmode) return i; // カウントモード時は個数を直接返す
			ti->rep = i;
			continue;

		case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10: // 二階微分パケット
			if(countmode) return 1; // カウントモード時
			j = pt->mask_size;
			for(i = 4 ; i-- ;) ti->val[i] += ti->dif[i] += UnpackBit(&buf, &j, pt->bit[i]);
			return 1;

		case 12: // 絶対値パケット<新規>
			ti->seg = -1;
			// no break　(セパレート処理以外は<継続>と同じ)
		case 11: // 絶対値パケット<継続>
			if(countmode) return 1; // カウントモード時
			j = 0;
			++buf;
			ti->prc = PRECISION_TABLE[UnpackBit(&buf, &j, 3) & 7]; // 保存解像度取得
			for(i = 4 ; i-- ;) ti->val[i] = UnpackBit(&buf, &j, pt->bit[i]);
			ti->dif[0] = ti->dif[1] = ti->dif[2] = 0;	// 微分値を初期化
			ti->dif[3] = 1;						// secだけ1が初期値
			return 1;

		case 13: // BCC(とりあえずBCCエラーは無視して、できるだけデコードしてみる)
			continue;

		case 14: // 1byte コマンド
			ti->seg |= buf[1]; // 次パケットでセグメント分割(オプション選択)
			continue;

		case 15: // n byte コマンド
			ti->tpos += (u32)buf[1];
			continue;
		}
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// トラックログ保存タスク
///////////////////////////////////////////////////////////////////////////////

// 時間のかかるFlash書き込み処理は最後に実行
s32 MT_TrackLog(){
	if(IW->mp.tlog.pre_pvtc != IW->px.counter){
		IW->mp.tlog.pre_pvtc = IW->px.counter;
		TrackLogAddCurrentPVT(); // トラックログ保存
		if(IW->tc.log_enable) IW->mp.tlog.tc_lastadd = IW->vb_counter; // リアルタイムログ中はログ間隔に関係なくカウントチェックをガードする
	} else if(IW->vb_counter - IW->mp.tlog.tc_lastadd > VBC_TIMEOUT(3)){ // 3秒間PVT通信がなければ計算を開始
		// 空いている時間に総トラック数をカウントしておく(正常接続時はカウントしない)
		TrackLogParam* tlp = &IW->mp.tlog;
		TrackInfo* ti = &IW->gi.tinfo;
		s32 end = IW->vb_counter + 1; // タイムアウト設定
		switch(tlp->tc_state){
		case 0:
			memset(ti, 0, sizeof(TrackInfo));
			ti->pre_sect = -1; // 無効セクタを指しておく
			ti->seg = -1;
			tlp->tc_tpos  = ti->tpos = FindFirstBlock() << 16;
			tlp->tc_total = 0;
			tlp->tc_state = 1;
			// no break;
		case 1:
			ti->pre_sect = -1;
			for(;;){
				s32 count = NextTrack(1);
				if(!count){
					// 完了
					tlp->tc_state = 2;
					break;
				}
				if(ti->seg == -1){
					ti->seg = 0;
					++count;
				}
				tlp->tc_total += count;
//#define MAX_TX_LOG 0xffff // 最大転送トラック数。プロトコル上これより多くのトラックを転送できないので、ここで計測を停止しておく。
#define MAX_TX_LOG 0x7fff // カシミールに32768以上を渡すとメモリ不足エラーが表示される…。とりあえずエラーの出ない最大値を設定しておく。
				if(tlp->tc_total > MAX_TX_LOG){
					tlp->tc_total = MAX_TX_LOG;
					// 完了
					tlp->tc_state = 2;
					break;
				}
				// 時間切れ中断
				if(end - *(vs32*)&IW->vb_counter < 0){
					IW->intr_flag = 1;
					break;
				}
			}
		}
	}
	return 0;
}

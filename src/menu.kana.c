///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005-2007 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// メニュー画面の制御をこのファイルにまとめた。

///////////////////////////////////////////////////////////////////////////////
// 定数
///////////////////////////////////////////////////////////////////////////////
#define KEY_LOCUS		(KEY_B | KEY_A)
#define KEY_CYL			(KEY_B | KEY_R)
//#define KEY_TEXT		(KEY_B | KEY_L)
#define KEY_WINDCHECK	(KEY_B | KEY_START)
#define KEY_BENCHMARK	(KEY_B | KEY_SELECT)
#define KEY_LOG_NEXT	(KEY_A | KEY_R | KEY_RIGHT | KEY_DOWN)
#define KEY_LOG_PREV	(KEY_L | KEY_LEFT | KEY_UP)
#define KEY_SP_NEXT		(KEY_B | KEY_A)

const u16 AUTO_REPEATE_START	= 800 / 16; // オートリピート開始(0.8秒)
const u16 AUTO_REPEATE_INTERVAL	= 100 / 16; // オートリピート間隔(0.1秒)
const u16 AUTO_REPEATE_ENABLE = ~(KEY_START | KEY_SELECT); // START, SELECTはAutoRepeat無効


const u32 INT_POS_TABLE[] = {
	1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000
};
const u32 TIME_POS_TABLE[] = {
	1, 10, 60, 600
};

const u32 LAT_MAX[2] = {
	180 * 3600000,
	 90 * 3600000,
};
const u32 LAT_POS_TABLE[] = {
	1, 10, 100, 1000, 10000, 60000, 600000, 3600000, 36000000, 360000000
};
const u32 LAT_VAL_POS[2][11] = {
	{0, 1, 2, 4, 5, 7, 8, 10, 11, 12, 13},
	{0, 1, 2, 4, 5, 7, 8, 10, 11, 13, 0},
};

const u8* const WEEK_NAME[7] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

#define GEN_COUNTER (*(u32*)(0x08000000 + 0xC8)) // 世代情報オフセット

///////////////////////////////////////////////////////////////////////////////
// メニュー用構造体
///////////////////////////////////////////////////////////////////////////////
typedef struct {
	const u8*	menu;
	const u8*	help;
	u32			action;
	const void*	data;
} MenuItem;

typedef const MenuItem* MenuPage;

typedef void (*TallyProc)(void);

///////////////////////////////////////////////////////////////////////////////
// メニューリスト
///////////////////////////////////////////////////////////////////////////////

enum {
	// メニュー番号
	MENU_ID_MAIN,
	MENU_ID_TASK,
	MENU_ID_TASK_START,
	MENU_ID_ROUTE,
	MENU_ID_NEW_ROUTE,
	MENU_ID_CHANGE_ROUTE,
	MENU_ID_WAYPOINT,
	MENU_ID_NEW_WPT,
	MENU_ID_CHANGE_WPT,
	MENU_ID_CONFIG,
	MENU_ID_VOICE,
	MENU_ID_DISPLAY,
	MENU_ID_INFO,
	MENU_ID_CONFIG_GPS,
	MENU_ID_CONFIG_NAVI,
	MENU_ID_CONFIG_EXTDEVICE,
	MENU_ID_CONFIG_ANEMOMETER,
	MENU_ID_CONFIG_VARIO,
	MENU_ID_CONFIG_LOG,
	MENU_ID_CONFIG_LOG2,
	MENU_ID_BLUETOOTH,
	MENU_ID_LOG,
	MENU_ID_LOG_LIST,
	MENU_ID_LOG_FL_SAVE,
	MENU_ID_LOG_TL_SAVE,
	MENU_ID_LOG_AUTO_SAVE,
	MENU_ID_CONFIG_ETC,
	MENU_ID_CONFIG_PARAM,
	MENU_ID_CONFIG_PARAM2,
	MENU_ID_CONFIG_AUTOTARGET,
	MENU_ID_CONFIG_THERMAL,
	MENU_ID_CONFIG_CYL,
	MENU_ID_CONFIG_WINDCHECK,
	MENU_ID_CONFIG_BENCHMARK,
	MENU_ID_TEST,
	MENU_ID_MAX,

	MENU_FCALL,		// 関数呼び出し
	MENU_SEL_ENUM,	// 列挙選択
	MENU_SEL_NAME,	// 名前入力
	MENU_SEL_LAT,	// 緯度経度入力
	MENU_SEL_VAL,	// 数値入力
	MENU_SEL_TIME,	// 時刻入力
	MENU_SEL_RTWPT,	// ルートウェイポイント入力
	MENU_SEL_TASK,  // タスク選択
	MENU_SEL_START,	// スタート設定
	MENU_SEL_SPEED,	// スピード表示専用
	MENU_SEL_PALETTE,// パレット設定専用
	MENU_SEL_FLOG,	// ログ時刻表示
};

extern const MenuPage MENU_LIST[];

///////////////////////////////////////////////////////////////////////////////
// 共通処理
///////////////////////////////////////////////////////////////////////////////
void MenuHideObj(){
	MoveOut(OBJ_MENU_UP);
	MoveOut(OBJ_MENU_DOWN);
	MoveOut(OBJ_MENU_CURSOR);
	MoveOut(OBJ_MENU_KETA);
}
void MenuFillBox(u32 x0, u32 y0, u32 x1, u32 y1){
	FillBox(x0, y0, x1, y1);
	MenuHideObj();
}

void MenuCls(u32 tile){
	ClsId(MAP_BG1, tile);
	Cls();
	MenuHideObj();
	IW->mp.help_msg = 0;
}

void RangeAdd(s32* val, s32 add, s32 max){
	if((*val += add) < 0) *val = max - 1;
	else if(*val >= max)  *val = 0;
}

void PutsName2(const u8* s, u32 len, u32 f){
	for(; len && *s; ++s, --len) Putc(*s);
	if(f) while(len--) Putc(' ');
}

u32 PutSameNameMessage(const u8* str){
	PlaySG1(SG1_CANCEL);
	MenuFillBox(0, 4, 29, 11);
	Putsf("%1.6mおなじ なまえの %sが%rすでに とうろく されています", str);
	return 0;
}
u32 PutBadNameMessage(){
	PlaySG1(SG1_CANCEL);
	MenuFillBox(1, 4, 28, 9);
	Putsf("%3.6mなまえを いれてください!");
	return 0;
}

u32 PutDelConf(){
	PlaySG1(SG1_CHANGE);
	MenuFillBox(0, 4, 29, 12);
	Putsf("%1.6mほんとうに さくじょしますか?%3rSTARTボタン: さくじょ");
	return 0;
}

void PutLastBuf(u8* buf, s32 len, u32 mode){
	s32 i, j, k = IW->mp.scr_pos << 3;
	len &= 0xffff;
	buf += k;
	Locate(0, 0);
	for(i = 0 ; i < 10 ; ++i){
		Putsf("%r%04x:", k);
		for(j = 0 ; j < 8 ; ++j, ++k, ++buf){
			if(k >= len){
				Puts(" __");
				continue;
			}
			if(mode && IsHalf(*buf)){
				Puts("  ");
				Putc(*buf);
			} else {
				Putsf(" %02x", *buf);
			}
		}
	}
}

u32 DispMenuArrow(u32 up, u32 down){
	u32 y = IW->vb_counter & 0x1f;
	if(y > 12) y = 12;
	MoveObj(OBJ_MENU_UP,   up?   112 : 240,  27 - y);
	MoveObj(OBJ_MENU_DOWN, down? 112 : 240, 125 + y);
	return 0;
}

void PutFlashError(u32 err){
	MenuFillBox(1, 4, 28, 9);
	const u8* msg;
	switch(err){
	case FLASH_SUCCESS:
		msg ="セーブしました";
		break;
	case FLASH_E_NO_CART:
	case FLASH_E_ID:
		msg ="カートリッジがありません!";
		break;
	case FLASH_E_NO_STORAGE:
		msg ="メモリカードがありません!";
		break;
	case FLASH_E_NO_FILE:
		msg ="ファイルがみつかりません!";
		break;
	case FLASH_E_SR_PROTECT:
		msg ="ライトプロテクトエラー!";
		break;
	case FLASH_E_SR_WRITE:
		msg ="かきこみエラー!";
		break;
	case FLASH_E_NOT_FFFF:
	case FLASH_E_SR_ERASE:
		msg ="しょうきょエラー!";
		break;
	case FLASH_E_COMPARE:
		msg ="コンペアエラー!";
		break;
	case FLASH_E_MBR_ERROR:
	case FLASH_E_SECT_ERROR:
	case FLASH_E_CLST_ERROR:
	case FLASH_E_DAT_SIZE:
	case FLASH_E_CNUM_RANGE:
		msg ="フォーマットエラー!";
		break;
	default:
		Putsf("%6.6mFlash Error (%d)", err);
		return;
	}
	DrawTextCenter(6, msg);
}

void DrawGraph(s32 val, s32 min, s32 max, s32 x, s32 y, s32 range, s32 neg){
	s32 range8 = range * 8;
	max -= min;
	val = RoundDiv((val - min) * range8, (max < 1)? 1 : max);
	if(neg == 2 && val < 1) val = 1; // ベースライン
	if(val >= range8) val = range8 - 1; // 区切りスペース

	u32 map  = (x & 1)? 0 : 1;
	u32 base = GRAPH_START + ((x & 1)? (1 << 10) : 0) + (neg? (1 << 11) : 0);
	x >>= 1;
	s32 i;
	for(i = 0 ; i < range ; ++i){
		s32 tile = base;
		if(val >= 8) tile += 8;
		else if(val > 0) tile += val;
		val -= 8;
		SetChar_Base(map, x, y, tile);
		y += neg? -1 : 1;
	}
}

// GPS通信の停止と再開
void StopGPSMode(){
	// UART割り込みスキップ
	IW->mp.multiboot = 1;

	// 音声DMA停止
	VoiceStop();
	EnableSound(SOUND_CH_VARIO, -1); // バリオ音も停止
}
void StartGPSMode(u32 baudrate){
	// UART復帰
	REG16(REG_RCNT)		= 0; // SIO Enable
	REG16(REG_SIOCNT)	= UART_MODE | UART_FIFO_DISABLE; // Clear FIFO
	REG16(REG_SIOCNT)	= (baudrate & 3) | UART_DATA8 | UART_FIFO_ENABLE | UART_PARITY_NONE |
						  UART_SEND_ENABLE | UART_RECV_ENABLE | UART_MODE | UART_IRQ_ENABLE; // Garmin用設定
	
	// UART割り込み復帰
	IW->mp.multiboot = 0;
}

//FLogの先頭2バイトのアドレスを返す
u32* GetFLogHead(u32 pos){
	if(pos < FLOG_COUNT){
		// キャッシュチェック
		u32* cache = IW->mp.flog_cache[pos];
		if(!*cache){
			// キャッシュにデータ格納
			const u32* addr = IW->cif->ReadDirect(FI_FLOG_OFFSET + pos * FLOG_SIZE);
			if(addr){
				cache[0] = addr[0];
				cache[1] = addr[1];
			} else {
				cache[0] = 1;//無効Magicを設定
			}
		}
		if(*cache == FLOG_MAGIC_FLIGHT || *cache== FLOG_MAGIC_TASK) return cache; // キャッシュのアドレスを返す
	}
	return 0; // データなし
}

Tally* GetTallyData(){
	if(IW->mp.flog_addr && *IW->mp.flog_addr == FLOG_MAGIC_FLIGHT) return (Tally*)IW->mp.flog_addr;
	return &IW->mp.tally;
}
TaskLogData* GetTaskData(){
	if(IW->mp.flog_addr && *IW->mp.flog_addr == FLOG_MAGIC_TASK) return (TaskLogData*)IW->mp.flog_addr;
	return &IW->task_log;
}
static inline u32 IsCurFlog() { return !IW->mp.flog_addr; }

// ルート距離計算
u32 CalcRouteDistance(u32 i){
//	MenuFillBox(2, 4, 27, 12);
//	DrawText(4, 6, "きょりをけいさんちゅう");
	Route* r = &ROUTE->route[i];
//	Locate(4, 9);
//	PutsName(r->name);

	r->dist = 0;
	for(i = r->count ; --i ;){
		u32 len;
		CalcWptDist(&WPT->wpt[r->py[i - 1].wpt], &WPT->wpt[r->py[i].wpt], &len, 0);
		r->dist += len;
	}
	return r->dist;
}

u32 MP_Navi(u16 push);
u32 MP_SearchNearWpt(u16 push);
s32 IsMenu(){
	return IW->mp.proc != MP_Navi && IW->mp.proc != MP_SearchNearWpt;
}

///////////////////////////////////////////////////////////////////////////////
// メニュー表示
///////////////////////////////////////////////////////////////////////////////
// ヘルプ行のスクロール表示
void DispHelp(const u8* p){
	Locate(0, 18);
	if(p){
		IW->mp.help_msg = p;
		IW->mp.help_scr = (PutsLen(p) <= 30)? -1 : 0;
		if(IW->mp.help_scr == -1){
			PutsName2(p, 30, 1);
			return;
		}
	} else {
		if(IW->mp.help_scr == -1 || !(p = IW->mp.help_msg)) return;
		if((s32)(IW->vb_counter - IW->mp.help_vbc) < 0) return;
	}

	// スクロール表示
	IW->mp.help_vbc = IW->vb_counter + ((IW->mp.help_scr)? 8 : 60);
	p += IW->mp.help_scr / 2;
	if(!*p) IW->mp.help_scr = 0;
	else if(IsHalf(*p)) IW->mp.help_scr += 2;
	else if(!((IW->mp.help_scr += 1) & 1)) Putc2nd(*p++); // 半キャラずらし

	// ヘルプ表示
	while(IW->mp.gy < 20){
		if(!*p) p = IW->mp.help_msg; // 巻き戻し
		Putc(*p++);
	}
}

// カーソル表示
void DispCursor(){
	// 位置変更あり。カーソルとヘルプを更新
	DispHelp(MENU_LIST[IW->mp.menuid][IW->mp.sel].help);
	MoveObj(OBJ_MENU_CURSOR, 1, IW->mp.sel * 16 + 10);
	MoveSelBg(IW->mp.sel, SELMENU_0);
}

// 時間表示専用
void PutsMin(s32 m){
	s32 h = BiosDiv(m, 60, &m);
	Putsf("%02d:%02d", h, m);
}

void PutsSec4(s32 s, s32 div){
	if(div) Putsf("%7S", RoundDiv(s, div));
	else	Puts("   -----");
}

void PutsGammaLevel(u32 n){
	u32 i;
	for(i = 0 ; i < 7 ; ++i){
		Puts((i <= n)? "<>" : "  ");
	}
}

const u8* const PRECISION_NAME[4] = {
	"3cm", "24cm", "1m", "4m"
};

void PutRouteWptCount(){
	Putsf("%15.6mRoute   : %d", ROUTE->route_count);
	UnderLine(15, 8, 13);
	Putsf("%15.10mWaypoint: %d", WPT->wpt_count);
	UnderLine(15, 12, 13);
}

void DispMenu(u32 id){
	MenuCls(SELMENU_BK2);

	const MenuItem* mi = MENU_LIST[id];
	u32 t, i = 0, max = 0, sel = 0;
	for(i = 0 ; mi[i].menu ; ++i){
		DrawText(1, i * 2 + 1, mi[i].menu);
		max = MaxS32(max, IW->mp.gx);
		if(mi[i].action > MENU_FCALL){
			sel = 1;
#define VAL_POS 15
			LocateX(VAL_POS);
			switch(mi[i].action){
			case MENU_SEL_VAL: // 数値選択
				{
					const IntInputTemplate* t = (IntInputTemplate*)mi[i].data;
					if(t->disp) PutsPointSGN(*t->val, 0, t->prec);
					else        PutsPoint   (*t->val, 0, t->prec);
					Puts(t->unit);
				}
				break;

			case MENU_SEL_TIME: // 時刻選択
				{
					const TimeInputTemplate* t = (TimeInputTemplate*)mi[i].data;
					PutsMin(*t->val);
				}
				break;
			
			case MENU_SEL_NAME: // 名前選択
				{
					const NameInputTemplate* t = (NameInputTemplate*)mi[i].data;
					u32 j = 0;
					while(j < t->max && IsHalf(t->val[j])) Putc(t->val[j++]);
				}
				break;
			
			case MENU_SEL_LAT: // 緯度経度
				{
					const LatInputTemplate* t = (LatInputTemplate*)mi[i].data;
					if(t->latlon) PutsLat(*t->val);
					else          PutsLon(*t->val);
				}
				break;
			
			case MENU_SEL_ENUM: // 列挙選択
				{
					const EnumInputTemplate* t = (EnumInputTemplate*)mi[i].data;
					u32 sel = *t->val;
					if(sel >= t->max) sel = 0; // 念のため
					Puts(t->names[sel]);
				}
				break;

			case MENU_SEL_RTWPT: // ルートウェイポイント入力
				{
					// システムに1つしかない
					Putsf("%dポイント", IW->route_input.count);
				}
				break;
			
			case MENU_SEL_TASK: // タスク(ルート表示)
				{
					// システムに1つしかない
					const Route* rt = GetTaskRoute();
					if(rt) PutsName(rt->name);
					else   Puts("なし");
				}
				break;
			
			case MENU_SEL_START:// スタート設定
				{
					// システムに1つしかない
					switch(IW->tc.start_type){
					case 0:
						Puts("Free");
						break;
					case 1:
						Puts("In");
						PutsMin(IW->tc.start_time);
						break;
					case 2:
						Puts("Out");
						PutsMin(IW->tc.start_time);
						break;
					}
					Putsf(" Pre%d", IW->tc.pre_pylon);
				}
				break;
			
			case MENU_SEL_SPEED: // グライダスピード表示専用
				if(IW->tc.spd_unit){
					Putsf("%1.1fkm/h", BiosDiv(IW->tc.my_ldK * IW->tc.my_down, 27777, &t));
				} else {
					Putsf("%1.1fm/s", BiosDiv(IW->tc.my_ldK * IW->tc.my_down, 100000, &t));
				}
				break;
			
			case MENU_SEL_PALETTE: // パレット設定専用
				PutsGammaLevel(IW->tc.palette_conf);
				break;

			case MENU_SEL_FLOG: // ログ設定専用
				{
					u32* addr = GetFLogHead(i); // 実際にはキャッシュを読む
					if(addr){
						switch(*addr){
						case FLOG_MAGIC_FLIGHT:	Putc('F'); break;
						case FLOG_MAGIC_TASK:	Putc('T'); break;
						default:				Putc('?'); break;
						}
						u32 year, month, day, week, hour, min, sec;
						GetDateTime(addr[1], &year, &month, &day, &week, &hour, &min, &sec);
						// addr[1]: date, addr[2]: time
						Putsf("%04d%02d%02d-%02d%02d", year, month, day, hour, min);
					} else {
						Puts("データなし");
					}
				}
				break;
			}
		}
	}
	if(sel) max = 29;
	DrawBox(0, 0, max, i * 2 + 1);
	DispCursor();

	// 特別メニューにはプチ情報を表示
	switch(id){
	case MENU_ID_WAYPOINT:
		if(GetDefaultLD()){
			DrawText(15, 14, "LD: ");
			PutsNameB(GetDefaultLD()->name);
			UnderLine(15, 16, 13);
		}
		// no break;
	case MENU_ID_ROUTE:
		if(id == MENU_ID_WAYPOINT) DrawText(14, 2, "<Waypoint Menu>");
		else                       DrawText(15, 2, "<Route Menu>");
		break;

	case MENU_ID_CONFIG_LOG:
		DrawText(14, 2, "<Track Log Menu>");

		DrawText(15, 6, "Mode: ");
		Puts(IW->tc.log_enable? "Realtime" : "Manual");
		UnderLine(15, 8, 14);
		Putsf("%15.10mStep:%ds %s", IW->tc.log_intvl, PRECISION_NAME[IW->tc.log_prec & 3]);
		UnderLine(15, 12, 14);

		if(IW->tc.log_debug){
			DrawBox_Base(16, 13, 27, 16);
			DrawText(17, 14, "DEBUG MODE");
		}
		break;

	case MENU_ID_LOG_LIST:
		Putsf("%1.12mカートリッジからよみこむログ%r" "をえらんでください.");
		break;
	case MENU_ID_LOG_FL_SAVE:
		Putsf("%1.12mフライトログのかきこみエリア%r" "をえらんでください.");
		break;
	case MENU_ID_LOG_TL_SAVE:
		Putsf("%1.12mタスクログのかきこみエリアを%r" "えらんでください.");
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////
// ブートメッセージ処理
///////////////////////////////////////////////////////////////////////////////
void SetBootMsgTimeout(u32 n){
	IW->mp.boot_msg = IW->vb_counter + n;
	if(IW->mp.boot_msg < 2) IW->mp.boot_msg = 2;
}
// ブート画面の処理
u32 CheckBootMsg(){
	if(IW->mp.boot_msg == 1){// 接続完了待ち
		if(IW->gi.state > GPS_GET_PINFO_WAIT){
			PlaySG1(SG1_CONNECT);
			DrawTextCenter(10, "     Connected.     ");
			SetBootMsgTimeout(60 * 2);
		}
	} else {
		// 画面消去タイムアウト待ち
		if((s32)(IW->vb_counter - IW->mp.boot_msg) > 0) return 1;
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// 共通コールバックに変更する
///////////////////////////////////////////////////////////////////////////////
// キー待ち
u32 SetKeyWaitCB(u16 push){
	if(push == 0xffff){
		IW->mp.proc = SetKeyWaitCB; // コールバックを変える
		return 0;
	}
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	return 0;
}

// ダウンロード待ち
u32 MP_DownloadWpt(u16 push);
u32 MP_DownloadRoute(u16 push);
u32 SetDownloadCB(u16 push){
	if(push == 0xffff){
		if(IW->gi.state <= GPS_STOP_DOWNLOAD){
			PlaySG1(SG1_CANCEL);
			MenuFillBox(2, 4, 28, 9);
			DrawText(4, 6, "ダウンロードできません!");
			return SetKeyWaitCB(0xffff);
		}

		PlaySG1(SG1_COMP1);
		MenuFillBox(0, 3, 29, 19);
		Locate(2, 5);
		if(IW->mp.proc == MP_DownloadRoute){
			IW->gi.state = GPS_ROUTE;
			Puts("ルート");
		} else if(IW->mp.proc == MP_DownloadWpt){
			IW->gi.state = GPS_WPT;
			Puts("WPT");
		}
		Puts("をダウンロードします");
		DrawText(4, 8, "(Select: ちゅうだん)");
		IW->gi.dl_accept = 1;
		MT_GPSCtrl();
		IW->mp.proc = SetDownloadCB;
		return 0;
	}

	if(push & KEY_SELECT){ // 中断
		PlaySG1(SG1_CANCEL);
		IW->gi.state = GPS_STOP_DOWNLOAD;
		IW->gi.dl_accept = 0;
		IW->mp.navi_update |= NAVI_UPDATE_WPT;
		return 1;
	}

	if(IW->gi.state <= GPS_STOP_DOWNLOAD) return 0;
	IW->gi.dl_accept = 0;

	PlaySG1(SG1_COMP2);
	IW->mp.navi_update |= NAVI_UPDATE_WPT;
	return 1;
}


///////////////////////////////////////////////////////////////////////////////
// 最寄WPTの検索
///////////////////////////////////////////////////////////////////////////////
u32 BacktoNavi(){
	SetBootMsgTimeout(60 * 2);
	IW->mp.proc = MP_Navi;
	IW->mp.at_vbc = IW->vb_counter;
	return 0;
}

void InitNearNavi(){
	IW->mp.nw_target = -1;
	IW->mp.navi_update |= NAVI_UPDATE_WPT;
	IW->mp.cur_view = -1;	// 再描画
	IW->mp.freeflight = 1;
}

void SetNaviStartPos(){
	// フリーフライトモードの開始場所のセット
	IW->mp.nw_start.alt = RoundDiv(IW->px.alt_mm, 1000);
	IW->mp.nw_start.lat = IW->px.lat;
	IW->mp.nw_start.lon = IW->px.lon;
	IW->mp.nw_cnd_len = -1;
	IW->mp.nw_search  = 0;
}

u32 MP_SearchNearWpt(u16 push){
	if(IW->key_state == KEY_DOWN){// &演算は使わず、単体チェック
		PlaySG1(SG1_CHANGE);
		FillBox(2, 6, 26, 13);
		DrawTextCenter(9, "フリーフライト");
		IW->mp.ar_key = 0;		// オートリピート停止
		InitNearNavi();
		SetNaviStartPos();
		return BacktoNavi();
	}

	s32 lat = IW->px.lat;
	s32 lon = IW->px.lon;

	if(push & 0x8000){
		push &= ~0x8000;
		if(push != KEY_LEFT && push != KEY_RIGHT && push != KEY_UP){
			IW->mp.ar_key = 0;		// オートリピート停止
			return 0;
		}
		FillBox(2, 6, 26, 13);
		DrawTextCenter( 8, "ターゲットけんさく...");
		DrawTextCenter(10, "(Downでフリーフライト)");
		if(push != KEY_UP && IW->mp.nw_target < (u32)WPT->wpt_count){
			const Wpt* w = &WPT->wpt[IW->mp.nw_target];
			CalcDist(w->lat, w->lon, lat, lon, &IW->mp.nw_tgt_len, 0);
			IW->mp.nw_cnd     = IW->mp.nw_target;
			IW->mp.nw_s_dir   = (push == KEY_RIGHT)? 1 : 0;
		} else {
			// 一番近いWPTを探す
			if(IW->tc.at_mode){
				IW->mp.nw_tgt_len = IW->tc.at_min + IW->tc.cylinder;
				IW->mp.nw_s_dir   =  2; // オートターゲットモード
				IW->mp.atcand_count = 0;
				if(IW->mp.nw_target < (u32)WPT->wpt_count){
					IW->mp.nw_cnd     = IW->mp.nw_target;
					IW->mp.nw_cnd_len = -1;
				} else {
					IW->mp.nw_cnd     = -1;
				}
			} else {
				IW->mp.nw_target  = -1;
				IW->mp.nw_tgt_len = -1;
				IW->mp.nw_cnd     = -1;
				IW->mp.nw_s_dir   =  1;
			}
			IW->mp.ar_key	  =  0;		// オートリピート停止
		}
		SetNaviStartPos(); // 検索開始場所のセット
		IW->mp.proc = MP_SearchNearWpt; // コールバックを変える
	}

	// オートターゲットの初回表示
	if(!IW->mp.nw_search && IW->mp.nw_s_dir > 1){
		FillBox(2, 6, 26, 13);
		Putsf("%4.8mRange:%dm-%dm%r" "オートターゲット...", IW->mp.nw_tgt_len, IW->tc.at_max);
	}

	s32 end = IW->vb_counter + 1; // タイムアウト設定
	const Wpt* w = &WPT->wpt[IW->mp.nw_search];
	for(; IW->mp.nw_search < WPT->wpt_count ; ++IW->mp.nw_search, ++w){
		if(IW->mp.nw_s_dir != 3 && IW->mp.nw_search == IW->mp.nw_target) continue; // 現在のターゲットは次候補から外す
		u32 len;
		CalcDist(w->lat, w->lon, lat, lon, &len, 0);
		switch(IW->mp.nw_s_dir){
		case 0:
			// 1つ近いWPTを探す
			if((len < IW->mp.nw_tgt_len && (s32)len > (s32)IW->mp.nw_cnd_len) ||
               (len < IW->mp.nw_tgt_len && len == IW->mp.nw_cnd_len) ||
			   (len == IW->mp.nw_tgt_len && IW->mp.nw_search < IW->mp.nw_target)){
				IW->mp.nw_cnd     = IW->mp.nw_search;
				IW->mp.nw_cnd_len = len;
			}
			break;

		case 1:
			// 1つ遠いWPTを探す
			if(((s32)len > (s32)IW->mp.nw_tgt_len && len < IW->mp.nw_cnd_len) || 
			   (len == IW->mp.nw_tgt_len && len != IW->mp.nw_cnd_len && IW->mp.nw_search > IW->mp.nw_target)){
				IW->mp.nw_cnd     = IW->mp.nw_search;
				IW->mp.nw_cnd_len = len;
			}
			break;

		default:
			// オートターゲットモード
			if((len >= IW->mp.nw_tgt_len && len <= IW->tc.at_max)){
				if(IW->mp.atcand_count < MAX_AUTOTARGET_CAND){
					IW->mp.atcand[IW->mp.atcand_count++] = (u16)IW->mp.nw_search;
				} else {
					// 万が一バッファが溢れた時
					IW->mp.atcand[MyRand() & (MAX_AUTOTARGET_CAND - 1)] = (u16)IW->mp.nw_search; // これでは一様にならないが、あまり厳密じゃなくても良い
				}
			} else if(IW->tc.at_mode > 1 && !IW->mp.atcand_count){
				// 有効レンジに無い場合に備えて、最もレンジに近いウェイポイントを記録しておく
				if(len < IW->tc.cylinder) len  = 0x7fffffff; // シリンダ内は低優先
				else if(len < IW->mp.nw_tgt_len) len  = IW->mp.nw_tgt_len - len;
				else                        len -= IW->tc.at_max;
				if(IW->mp.nw_cnd == -1 || len < IW->mp.nw_cnd_len){
					IW->mp.nw_cnd = IW->mp.nw_search;
					IW->mp.nw_cnd_len = len;
				}
			}
			break;
		}

		// 時間切れ中断
		if(end - *(vs32*)&IW->vb_counter < 0){
			IW->intr_flag = 1;
			IW->mp.nw_search++;
			return 0;
		}
	}

	// オートターゲット処理
	if(IW->mp.nw_s_dir > 1){
		if(IW->mp.atcand_count){
			u32 mod;
			BiosDiv(MyRand(), IW->mp.atcand_count, &mod);
			IW->mp.nw_cnd = IW->mp.atcand[mod];
		} else if(IW->mp.nw_cnd == -1){
			// 候補無しの場合は、ターゲット指定無し(LDに向かう)
			Putsf("%4.10mターゲットなし!     ");
		}
	}

	// ターゲット変更
	if(IW->mp.nw_target == IW->mp.nw_cnd){
		PlaySG1(SG1_CANCEL);
	} else {
		switch(IW->mp.nw_s_dir){
		case 0:	PlaySG1 (SG1_PREV);		break;
		case 1:	PlaySG1 (SG1_NEXT);		break;
		default:PlaySG1X(SG1_CHANGE);	break; // パイロン音を優先
		}
		IW->mp.nw_target = IW->mp.nw_cnd;
		IW->mp.navi_update |= NAVI_UPDATE_WPT;
		IW->mp.cur_view = -1;
	}
	if(IW->mp.nw_target != -1){
		Putsf("%4.8m\\\\#%03d:", IW->mp.nw_target + 1);
		PutsNameB(WPT->wpt[IW->mp.nw_target].name);
		PutsSpace(5);
		IW->mp.freeflight = 0;
	} else {
		IW->mp.freeflight = 1;
	}
	return BacktoNavi();
}

///////////////////////////////////////////////////////////////////////////////
// デバッグ操作
///////////////////////////////////////////////////////////////////////////////
s32 DummyPlay(){
	Wpt xx;
	PvtX* px = &IW->px;

	if(IW->px.lat == INVALID_LAT){
#define IDO(d, m, s, ms) (((d * 60 + m) * 60+ s) * 1000 + ms)
		IW->px.lat		= IDO( 35, 22, 21, 000);
		IW->px.lon		= IDO(138, 32, 48, 000);
		IW->px.alt_mm	= 1234567;
	}

	Wpt* w0 = 0;
	if(IW->tc.task_mode){
		Route* rt = GetTaskRoute();
		w0 = GetCurWpt(rt, 0);
	}
	if(!w0){
		if(IW->mp.nw_target < (u32)WPT->wpt_count){
			w0 = &WPT->wpt[IW->mp.nw_target];
		} else if(WPT->wpt_count){
			w0 = &WPT->wpt[0];
		} else{
			xx.alt = 200;
			xx.lat = 100;
			xx.lon = 100;
			w0 = &xx;
		}
	}
	s32* turn   = (s32*)&IW->px_rfu[4];
	s32* pvh_mm = (s32*)&IW->px_rfu[8];
	s32* pup_mm = (s32*)&IW->px_rfu[12];

#define DUMMY_PLAY_KEY	(KEY_L | KEY_R | KEY_UP | KEY_DOWN)
	u16 push = IW->key_state;

	// マニュアル操縦
	if(!IsMenu() && !(push & KEY_B) && (push & DUMMY_PLAY_KEY)){
		if(push & KEY_L)		*turn -= 7;
		if(push & KEY_R)		*turn += 7;
		if(push & KEY_DOWN)		*pup_mm -= 40;
		if(push & KEY_UP)		*pup_mm += 40;

		if(*turn   <  -700) *turn   =  -700;
		if(*turn   >   700) *turn   =   700;
		if(*pup_mm < -9999) *pup_mm = -9999;
		if(*pup_mm >  9999) *pup_mm =  9999;

		if(push & KEY_UP)   *pvh_mm += 100;
		if(push & KEY_DOWN) *pvh_mm -= 100;
		if(*pvh_mm < 0) *pvh_mm = 0;

		return 1;
	}

	// 自動操縦
	// up
	*pup_mm += ((((px->lat) & 0xfff) - ((px->lon) & 0xfff)) >> 8) +
				(((w0->alt + 256) * 1000 - px->alt_mm) >> 13);
	if(myAbs(*pup_mm) > 3000) *pup_mm -= *pup_mm >> 7;
	// vh
	s32 t = px->h_ang64;
	if(t > 0x8000) t = 0x10000 - t;
	*pvh_mm = ((t >> 2) + 12000) - (px->up_mm >> 1);
	//turn
	s32 len, ang;
	if(!w0) return 0;
	CalcDist(IW->px.lat, IW->px.lon, w0->lat, w0->lon, &len, &ang);
	if(px->up_mm > 500 && px->alt_mm < (w0->alt + 500) * 1000){
		// センタリング
		if(*turn < 0)	*turn -= 3;
		else			*turn += 3;
		// センタリング回転リミッタ
		if(*turn < -150)		*turn = -150;
		else if(*turn > 150)	*turn =  150;

		if(myAbs(*turn) > 100) *turn += (MyRand() & 0x3f) - 32; // 揺れ
	} else {
		// ターゲット
//		ang += 0x8000; // ZZZ 逆走用
		ang = (GetAngLR(px->h_ang64 - ang) >> 7) + *turn;
		if(ang) *turn -= ang >> 2;
		if(px->alt_mm < (w0->alt + 300) * 1000){
			if(px->alt_mm < 1000) *pup_mm += 50;
		} else if(*pup_mm > -100){
			*pup_mm -= 50;
		} else if(*pup_mm < -3000){
			*pup_mm += 50;
		}
	}
	if((t = MyRand() & 7) < 7) *turn += t - 3; // 揺れ

	return 0;
}

const double RAD2LATr	= 1 / (1000.0 * 60 * 60 * 180 / M_PI);
void DummyPlayUpdate(){
	IW->mp.navi_update |= 1;

	D800_Pvt_Data_Type pvt;
	memset(&pvt, 0, sizeof(pvt));
	PvtX* px = &IW->px;
	s32 turn   = *(s32*)&IW->px_rfu[4];
	s32 pvh_mm = *(s32*)&IW->px_rfu[8];
	s32 pup_mm = *(s32*)&IW->px_rfu[12];

	if((IW->key_state & KEY_START) == KEY_START){
		pvh_mm >>= 3;
	}
	
	s32 rot = px->h_ang64 + turn * 0x20;
    SetFloat (pvt.alt, (px->alt_mm + pup_mm) / 1000.0);
    SetInt   (pvt.fix, FIX_3D);
    SetDouble(pvt.tow, px->counter);
	double vn = Cos64K(rot) / 65556000.0 * pvh_mm;
	double ve = Sin64K(rot) / 65536000.0 * pvh_mm;
    SetFloat (pvt.north, vn);
    SetFloat (pvt.east,  ve);
    SetFloat (pvt.up,    pup_mm / 1000.0);

    
	s32 ang64 = (px->lat * 2) * (5.0567901234567901234567901234568e-5 / 2);
	vn /= 0.030866974806026297227396863865729  * (1 - Cos64KX(ang64 * 2) * 1.526e-7);
	ve /= 9.4360687750139522468485224973412e-7 * Cos64KX(ang64) *
          (1 - (Cos64KX(ang64 * 2) - 32767) * 5.1480147709585863826410718100528e-8);
	if(IW->px.lat == INVALID_LAT) IW->px.lat = 0;
	SetDouble(pvt.lat, (px->lat + vn) * RAD2LATr);
    SetDouble(pvt.lon, (px->lon + ve) * RAD2LATr);

	SetLong(pvt.wn_days, 365 * 25);
	CalcPvtGarmin(&pvt, px);
	IW->mp.auto_off_cnt = IW->vb_counter;
}

///////////////////////////////////////////////////////////////////////////////
// ナビメニュー
///////////////////////////////////////////////////////////////////////////////
#define LOCK_KEY (KEY_START | KEY_SELECT)
#define SUSPEND_KEY (KEY_START | KEY_SELECT | KEY_L | KEY_R)

u8* const VIEW_NAME[MAX_VIEW_MODE] = {
	"#1:スタンダード",
	"#1:スタンダード R1",
	"#1:スタンダード R2",
	"#1:スタンダード R3",
	"#2:シンプル",
	"#2:シンプル R1",
	"#2:シンプル R2",
	"#2:シンプル R3",
	"#3:ターゲット",
	"#3:ターゲット R1",
	"#3:ターゲット R2",
	"#3:ターゲット R3",
	"#4:パイロンx2",
	"#4:パイロンx2 R1",
	"#4:パイロンx2 R2",
	"#4:パイロンx2 R3",
	"#5:ウインド",
	"#5:ウインド R1",
	"#5:ウインド R2",
	"#5:ウインド R3",
	"#6:*Cylinder",
	"#6:*Cylinder R1",
	"#6:*Cylinder R2",
	"#6:*Cylinder R3",
	"#7:*Locus",
	"#7:*Locus R1",
	"#7:*Locus R2",
	"#7:*Locus R3",
	"#8:テキスト",
	"#8:テキスト R1",
	"#8:テキスト R2",
	"#8:テキスト R3",
	"#9:*Debug",
	"#9:*Debug R1",
	"#9:*Debug R2",
	"#9:*Debug R3",
};


const u16* const MODE_SOUND[SP_VIEW_COUNT] = {
	SG1_CANCEL, SG1_MODE1, SG1_MODE2, SG1_MODE4, SG1_MODE3, SG1_MODE4,
};
void PlayModeSound(u32 mode){
	if(mode >= SP_VIEW_COUNT) mode = SP_VIEW_COUNT - 1;
	PlaySG1(MODE_SOUND[mode]);
}

s32 CheckSpMode(s32 mode, s16 push, s16 key, s32 cmd){
	if(IW->mp.sp_view){
		s32 s_mode = 1 << IW->mp.sp_view;
		// どのモードもBボタンで復帰する
		if(push == KEY_B){
			PlaySG1(SG1_CANCEL);
			IW->mp.sp_enter |= s_mode; // ガードフラグ
			IW->mp.sp_view = SP_VIEW_NONE;
			return 1;
		}
		// 解除条件チェック
		if((IW->mp.sp_view == mode && !cmd && (IW->mp.sp_enter & (1 << mode))) || IW->mp.sp_state == -1){
			PlaySG1X(SG1_CANCEL);
			IW->mp.sp_view = SP_VIEW_NONE;
			IW->mp.sp_enter &= ~s_mode;
			return 1;
		}
	} else {
		s32 s_mode = 1 << mode;
		// コマンドボタンチェック
		if((push & key) && IW->key_state == key){
			PlayModeSound(mode);
			// 専用モード切替
			IW->mp.ar_key = 0; // オートリピート停止
			IW->mp.sp_view = mode;
			IW->mp.sp_enter &= ~s_mode;
			IW->mp.sp_state = 0;
			return 1;
		}
		// コマンド入力チェック
		if(!(IW->mp.sp_enter & s_mode)){
			if(cmd){
				PlayModeSound(mode);
				IW->mp.sp_view = mode;
				IW->mp.sp_enter |= s_mode;
				IW->mp.sp_state = 0;
				return 1;
			}
		} else {
			if(!cmd) IW->mp.sp_enter &= ~s_mode;
		}
	}
	return 0;
}

s32 CheckSpModeCommand(u16 push){
	if(IW->mp.key_lock){
		if((push & KEY_SP_NEXT) && (IW->key_state & KEY_SP_NEXT) == KEY_SP_NEXT && IW->tc.auto_lock > 1){
			// メッセージなしのキーロック中にA+Bを押すとLocusとシリンダを順にまわす
			IW->mp.ar_key = 0; // オートリピートストップ
			IW->mp.sp_enter |= 1 << IW->mp.sp_view;
			IW->mp.sp_state = 0;
			if(IW->mp.sp_view++ >= SP_VIEW_CYL) IW->mp.sp_view = 0;
			PlayModeSound(IW->mp.sp_state);
			return 1; // 変更
		}
		push = 0;
	}

	// 軌跡とシリンダの優先度設定を最初にしておく
	s32 state[5] = {0};
	s32 f_locus = (IW->px.gstate & GSTATE_CENTERING);
	s32 f_cyl   = (IW->mp.cyl_dist && IW->mp.cyl_dist < IW->tc.cyl_near);

	if(f_locus && IW->tc.thermal_cmd && f_cyl && IW->tc.cyl_cmd && IW->tc.sp_prio){ // 要優先度チェック
		s32 prio = IW->tc.sp_prio;
		if(prio == 3) prio = (IW->vb_counter & (1 << 8))? 1 : 2; // トグル(1/60*2^8=4秒トグル)
		if((prio == 1 && IW->mp.sp_view == SP_VIEW_LOCUS && !(IW->mp.sp_enter & (1 << SP_VIEW_CYL))) ||
		   (prio == 2 && IW->mp.sp_view == SP_VIEW_CYL   && !(IW->mp.sp_enter & (1 << SP_VIEW_LOCUS)))){
			IW->mp.sp_enter &= ~(1 << IW->mp.sp_view);
			IW->mp.sp_view = SP_VIEW_NONE;
			if(prio == 1) f_locus = 0;
			else          f_cyl = 0;
		}
	}

	// コマンドに応じて各SPモードに入る
	state[1] = f_locus;
	if(CheckSpMode(SP_VIEW_LOCUS, push, KEY_LOCUS, state[IW->tc.thermal_cmd])) return 1;
	state[1] = f_cyl;
	if(CheckSpMode(SP_VIEW_CYL,			push, KEY_CYL,		 state[IW->tc.cyl_cmd])) return 4; 
//	if(CheckSpMode(SP_VIEW_TEXT,		push, KEY_TEXT,		 0)) return 5; // B+Lでテキストモードに入る → メニュー内で表示。B+LはRFUとして温存。
	if(IW->mp.sp_view){
		state[1] = 1;
		state[2] = 1;
		state[3] = 1;
		state[4] = 1;
	} else {
		state[1] =  ((IW->px.gstate & GSTATE_ROLL_MASK)  >>  8) >= 3;
		state[2] = (((IW->px.gstate & GSTATE_PITCH_MASK) >> 20) >= 5) &&
			         (IW->px.gstate & (GSTATE_TURN_MASK | GSTATE_ROTATE | GSTATE_ROLL)) == GSTATE_STRAIGHT;
		state[3] = IW->px.gstate & GSTATE_SPIRAL;
		state[4] = IW->px.gstate & GSTATE_STALL;
	}
	if(CheckSpMode(SP_VIEW_WINDCHECK,	push, KEY_WINDCHECK, state[IW->tc.wind_cmd]))   return 2;
	if(CheckSpMode(SP_VIEW_BENCHMARK,	push, KEY_BENCHMARK, state[IW->tc.glider_cmd])) return 3; 
	return 0;
}

u32 MP_Navi(u16 push){
	if(push == 0xffff){
		MenuCls(0);
		IW->mp.cur_view = -1;
		IW->mp.mp_flag = 0;
		IW->mp.proc = MP_Navi;
		return 0;
	}

	// まず、スペシャルモードコマンドをチェック
	if(CheckSpModeCommand(push)) return 0;

	// キーロックチェック
	if(push & LOCK_KEY){
		if((IW->key_state & SUSPEND_KEY) == SUSPEND_KEY){
			MenuFillBox(3, 4, 26, 9);
			DrawTextCenter(6, "サスペンドします...");
			Suspend();
			MenuCls(0);
			return 0;
		}
		if((IW->key_state & LOCK_KEY) == LOCK_KEY){
			IW->mp.key_lock = !IW->mp.key_lock;
			FillBox(6, 6, 23, 11);
			DrawTextCenter(8, IW->mp.key_lock? "キーロック" : "ロックかいじょ");
			SetBootMsgTimeout(30);
		}
		return 0;
	}
	if(IW->mp.key_lock && push){
		if(!(IW->tc.auto_lock & 2)){
			FillBox(2, 6, 27, 11);
			DrawTextCenter(8, "START+SELECTでアンロック");
			SetBootMsgTimeout(40);
		}
		push = 0;
		IW->mp.ar_key = 0; // オートリピート停止
	}

	// ナビ画面切替
	if((IW->key_state & KEY_B) && (push & KEY_CURSOR)){
		PlaySG1(SG1_SELECT);
		switch(push){
		case KEY_RIGHT:
			if((IW->tc.view_mode += 4) >= MAX_VIEW_MODE - 4) IW->tc.view_mode -= MAX_VIEW_MODE - 4;
			break;
		case KEY_LEFT:
			if((IW->tc.view_mode -= 4) < 0) IW->tc.view_mode += MAX_VIEW_MODE - (IW->key_state & KEY_L? 0 : 4); // debug
			break;
		case KEY_DOWN:
			IW->tc.view_mode = (IW->tc.view_mode & ~0x3) | ((IW->tc.view_mode + 1) & 0x3);
			break;
		case KEY_UP:
			IW->tc.view_mode = (IW->tc.view_mode & ~0x3) | ((IW->tc.view_mode - 1) & 0x3);
			break;
		}
		FillBox(5, 6, 24, 11);
		DrawTextCenter(8, VIEW_NAME[IW->tc.view_mode]);
		SetBootMsgTimeout(30);
		return 0;
	}

	if(!IW->tc.task_mode && !IW->mp.freeflight && WPT->wpt_count && IW->mp.nw_target >= WPT->wpt_count && IW->px.fix >= FIX_2D){
		return MP_SearchNearWpt(KEY_UP | 0x8000);
	}

	// 次へ進める
	if(push & KEY_CURSOR){
		push = IW->key_state & KEY_CURSOR;
		if(!WPT->wpt_count) return 0; // WPTが無い
		if(!IW->tc.task_mode) return MP_SearchNearWpt(push | 0x8000);
		const Route* rt = GetTaskRoute();
		if(push == KEY_RIGHT){ // &演算は使ず、単体チェック
			PlaySG1(SG1_NEXT);
			if(rt->count <= ++IW->mp.cur_point) IW->mp.cur_point = 0;
		} else if(push == KEY_LEFT){// &演算は使ず、単体チェック
			PlaySG1(SG1_PREV);
			if(rt->count <= IW->mp.cur_point || !IW->mp.cur_point--) IW->mp.cur_point = rt->count - 1;
		} else {
			return 0; // 単体押し以外は何もしない
		}
		IW->mp.navi_update |= NAVI_UPDATE_WPT;
		IW->mp.cur_view = -1;
		return 0;
	}

	// メニューチェック
	if(push & KEY_A){
		PlaySG1(SG1_OK);
		IW->mp.boot_msg = 0;
		return 1;
	}

	// ブートメッセージ処理
	if(IW->mp.boot_msg){
		if(push || CheckBootMsg()){
			IW->mp.boot_msg = 0;
			MenuCls(0);
		}
	}

	// 警告ポップアップ
	if(IW->tc.gps_warn && !IW->mp.boot_msg && IW->mp.sp_view != SP_VIEW_TEXT){
		const u8* warn = 0;
		if(IW->gi.state >= GPS_EMUL_FINISH)		warn = "PCつうしんモード";
		else if(IW->gi.state >= GPS_EMUL_TRACK)	warn = "Trackそうしん...";
		else if(IW->gi.state >= GPS_EMUL_ROUTE)	warn = "Routeそうしん...";
		else if(IW->gi.state >= GPS_EMUL_WPT)	warn = "WPTそうしん...";
		else if(IW->gi.state >= GPS_EMUL_INFO)	warn = "PCつうしんかいし";
		else if(IW->px.fix == FIX_UNUSABLE)		warn = "GPSタイムアウト!";
		else if(IW->px.fix == FIX_INVALID)		warn = "サテライトロスト";

		if(warn){ // 警告あり
			FillBox(5, 6, 24, 11); // 共通サイズ
			DrawTextCenter(8, warn);
			SetBootMsgTimeout(90); // 1.5秒間は警告表示する
		}
	}
	return 0;
}

s32 CheckConfigDiff(){
	if(!CART_IS_WRITEABLE()) return 0; // 書き込み不可のカートリッジ
	if(IW->mp.save_flag & SAVEF_UPDATE_WPT) return 1; // ルート変更時は強制
	if(IW->mp.save_flag & SAVEF_UPDATE_CFG){// コンフィグ変更時は設定を戻していないかチェックしておく
		// DirectMapが使えないSCSD等の場合には、厳密チェックより操作性向上を優先…
		if(!CART_IS_DIRECTMAP()) return 1; // フラグ判断のみで、データコンペアは省略

		// JoyCarry等の直読み可能なカートリッジは、元に設定を戻した場合にメッセージを出さないようチェック
		return memcmp(&IW->tc, IW->cif->ReadDirect(FI_CONFIG_OFFSET), sizeof(IW->tc));
	}
	return 0;
}

u32 MP_SaveCheck(u16 push){
	if(push == 0xffff){
		if(CheckConfigDiff()){
			FillBox(1, 2, 28, 14);
			Putsf("%3.4mせっていがへんこうされて%r"
				        "います.  セーブしますか?%3r"
						"Aボタン: セーブ%r"
						"Bボタン: セーブしない");
			IW->mp.mp_flag = 0;
			IW->mp.save_flag &= ~SAVEF_UPDATE; //フラグは落としておく
			return 0;
		}
		IW->mp.save_flag &= ~SAVEF_UPDATE;
		return MP_Navi(0xffff);
	}

	if(IW->mp.mp_flag == 1){ // 保存済
		if(push & (KEY_A | KEY_B)) return MP_Navi(0xffff);
		return 0;
	}

	// 保存確認中
	if(push & KEY_B) return MP_Navi(0xffff);
	if(!(push & KEY_A)) return 0;

	// セーブ
	void SaveFlash();
	SaveFlash();
	IW->mp.mp_flag = 1;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// サスペンド
///////////////////////////////////////////////////////////////////////////////
u32 MP_Suspend(u16 push){
	if(push == 0xffff){
		PlaySG1(SG1_CHANGE);
		MenuFillBox(1, 3, 28, 11);
		Putsf("%5.5mサスペンドしますか?%3r" "STARTボタン:サスペンド");
		IW->mp.mp_flag = 0;
		return 0;
	}

	// キャンセルチェック
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}

	if(!(push & KEY_START)) return 0;

	// サスペンド開始
	MenuFillBox(0, 3, 29, 10);
	DrawText(1,  6, "L+Rボタンで リジュームします");
	Suspend();
	return 1;
}


///////////////////////////////////////////////////////////////////////////////
// バリオテスト
///////////////////////////////////////////////////////////////////////////////
u32 MP_VarioTest(u16 push){
	// 最初の呼び出し
	if(push == 0xffff){
		MenuFillBox(9, 4, 20, 12);
		DrawText(11, 6, "テスト");
		Putc('♪');
		IW->vm.vario_test = 0;
	} else if(push & (KEY_A | KEY_B)){ // 復帰ボタンチェック
		IW->vm.vario_test = VARIO_TEST_DISABLE;
		return 1; // MP終了
	} else if(push & KEY_UP)	IW->vm.vario_test += 100;
	else if(push & KEY_DOWN)	IW->vm.vario_test -= 100;
	else if(push & KEY_LEFT)	IW->vm.vario_test += 1000;
	else if(push & KEY_RIGHT)	IW->vm.vario_test -= 1000;
	else if(push & KEY_L)		IW->vm.vario_test += 3000;
	else if(push & KEY_R)		IW->vm.vario_test -= 3000;
	else return 0;

	if(IW->vm.vario_test >  9900) IW->vm.vario_test =  9900;
	if(IW->vm.vario_test < -9900) IW->vm.vario_test = -9900;
	// 音程変更
	Putsf("%12.9m%+.3f%16Pm/s", IW->vm.vario_test);
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// バージョン表示
///////////////////////////////////////////////////////////////////////////////

u32 MP_Version(u16 push){
	MenuCls(SELMENU_BK1);
	FillBox(1, 4, 28, 12);
	DrawText( 7,  1, "--- パラナビ ---");
	DrawText( 3,  6, "バージョン: " NAVI_VERSION);
	DrawText( 3,  9, "さくせいび: " __DATE__);
	DrawText( 0, 15, "COPYRIGHT(C) 2005-2008 Rinos.");
	DrawText(10, 17, "All rights reserved.");
	return SetKeyWaitCB(0xffff);
}

///////////////////////////////////////////////////////////////////////////////
// 現在地を拡大表示(テキストモードの表示を利用)
///////////////////////////////////////////////////////////////////////////////
u32 MP_LatLon(u16 push){
	if(push == 0xffff){
		// ビューモードを強制的に切り替える
		IW->mp.mp_flag = (IW->mp.sp_view << 8) | (IW->tc.view_mode & 0xff); // バックアップ
		IW->mp.sp_view = SP_VIEW_TEXT;
		MenuCls(0);
		DrawBox(0, 0, 29, 19);
		SetVideoMode(2);
	} else if(push == KEY_A){
		// 現在のナビ画面角度と横画面をトグル
		PlaySG1(SG1_CHANGE);
		if(IW->tc.view_mode & 3) IW->tc.view_mode &= ~3;
		else                     IW->tc.view_mode |= IW->mp.mp_flag & 3;
	} else if(push == KEY_B){
		PlaySG1(SG1_CANCEL);
		// 表示復帰
		IW->mp.sp_view   = IW->mp.mp_flag >> 8;
		IW->tc.view_mode = IW->mp.mp_flag & 0xff;
		SetVideoMode(0);
		return 1; // 終了
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Tips
///////////////////////////////////////////////////////////////////////////////
const u8* const TIPS[] = {
//	"SLELECボタンをおしながら TIPSをひょうじすると,テストモードになります.",
	"ナビがめんでSTART+SELECTをおすと,キーロックします.",
	"ナビがめんでB+カーソルをおすと,ビュータイプをへんこうできます.",
	"L+Rボタンをおしながらブートすると,コンフィグのロードをしません.",
	"レースモードでLeftボタンをおすと,まえのパイロンにもどります.",
	"レースモードでRightボタンをおすと,つぎのパイロンにすすみます.",
	"フリーフライトモードでUp/Left/Rightボタンをおすと,Gotoモードになります.",
	"GotoモードでLeftボタンをおすと,1つちかくのWPTにナビゲートします.",
	"GotoモードでRightボタンをおすと,1つとおくのWPTにナビゲートします.",
	"GotoモードでUpボタンをおすと,いちばんちかくのWPTにナビゲートします.",
	"GotoモードでDownボタンをおすと,フリーフライトモードになります.",
	"GotoモードでのWPTサーチじかんは, ウェイポイントすうに ひれいします.",
	"パイロンアローのながさは,とうたつこうどの めやすをあらわしています.",
	"ナビがめんでB+Aボタンをおすと, Locusモードになります.",
	"ナビがめんでB+Rボタンをおすと, シリンダモードになります.",
	"ナビがめんでB+SELECTボタンをおすと,グライダベンチマークモードになります.",
	"ナビがめんでB+STARTボタンをおすと,ウインドチェックモードになります.",
	"グラフのスケールは,Locusメニューの\"サンプリング\"に,れんどうしています.",
	0
};

u32 MP_Tips(u16 push){
	// テストモードのチェック
	if(IW->key_state & KEY_SELECT){
		IW->mp.menuid = MENU_ID_TEST;
		IW->mp.sel = 0;
		IW->mp.proc = 0;
		DispMenu(IW->mp.menuid);
		return 1;
	}

	// 以降、本当のTips
	if(!IW->mp.tips){
		BiosDiv(IW->vb_counter, ARRAY_SIZE(TIPS), &IW->mp.tips);
	}

	if(++IW->mp.tips >= ARRAY_SIZE(TIPS)) IW->mp.tips = 1;

	MenuFillBox(0, 3, 29, 14);
	DrawTextCenter(4, "Tips");
	Locate(2, 7);
	const u8* p = TIPS[IW->mp.tips - 1];
	for(;*p ; ++p){
		Putc(*p);
		if(IW->mp.gx > 26){
			IW->mp.gx  = 2;
			IW->mp.gy += 2;
		}
	}
	return SetKeyWaitCB(0xffff);
}

//  サンプルルートデバッグ登録
u32 MP_TestRoute(u16 push){
	u32 AddAsagiriRoute();
	AddAsagiriRoute();

	Route* rt = GetTaskRoute();
	Wpt* w0 = GetCurWpt(rt, 0);
	IW->px.lat		= w0->lat + 30000; // 100≒ 3m
	IW->px.lon		= w0->lon + 30000;
	IW->px.alt_mm	= w0->alt * 1000;
	IW->px.dtime	= 25 * 365 * 86400 + 23 * 3600; // 適当

	IW->mp.tlog.opt |= TLOG_OPT_MANUAL; // セグメント分割
	IW->mp.tally.last_lat = IW->px.lat;
	IW->mp.tally.last_lon = IW->px.lon;
	return SetKeyWaitCB(0xffff);
}

///////////////////////////////////////////////////////////////////////////////
// トラックログ
///////////////////////////////////////////////////////////////////////////////

const u32 SIZE_TABLE[] = {
	1, 1, 2, 3, 4, 4, 4, 5, 5, 6, 7, 15, 15, 2, 2, 
};

u32 MP_LogCheck(u16 push){
	if(push == 0xffff){
		MenuCls(SELMENU_BG);
		return 0;
	}

	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}

	TrackLogParam* tlp = &IW->mp.tlog;
	if(push & KEY_START){
		// デバッグ用情報をクリア
		PlaySG1(SG1_CLEAR);
		memset(tlp->t_count, 0, sizeof(tlp->t_count));
		tlp->err = 0;
		tlp->drop = 0;
		tlp->trk_counter = 0;
		MenuCls(SELMENU_BG);
	}

	s32 i;
	Putsf("%.8mStatistics: (%d point)", tlp->trk_counter);

	s32 sum2 = 0;
	Locate(0, 10);
	for(i = 0 ; i < 15 ; i++){
		const u8 DISP_LIST[] = {0,3,6,9,12,  1,4,7,10,13,  2,5,8,11,14};
		Putsf("%6d", tlp->t_count[DISP_LIST[i]]);
		sum2 += SIZE_TABLE[i] * tlp->t_count[i];
	}
	DrawText(20, 16, "Avg.");
	s32 t;
	if(tlp->trk_counter) Putsf("%2.3f", BiosDiv(sum2 * 1000, tlp->trk_counter, &t));
	else Putc('0');
	Putsf("%.16mRep:%2d%7PErr:%2d", tlp->repeat, tlp->drop);
	if(tlp->drop) Putsf("(%2d)", tlp->err);

	DrawTextCenter(0, "< Track log information >");
	DrawText(0, 3, "Use:");
	u32 bnum = IW->cw.tlog_block;
	if(tlp->full) i = bnum * BLOCK_SIZE;
	else if(!bnum || bnum < tlp->block) i = 0; // !?
	else if(tlp->blk_counter >= bnum) i = (bnum - 1) * BLOCK_SIZE + tlp->index;
	else if(tlp->blk_counter) i = (tlp->blk_counter - 1) * BLOCK_SIZE + tlp->index;
	else i = tlp->index;


	Putsf("%dKB/%dKB", i >> 10, bnum * 64);
	if(bnum)	Putsf("(%d%%) Tx%x%c  ", BiosDiv(i, bnum * 655, &i), IW->mp.tlog.tc_total, (tlp->tc_state == 2)? ' ' : '?');
	else		Puts("   ** NO SPACE **");
	Putsf("%.5mBlk#%d%8PPkt#%d%21Pidx:%04x", tlp->blk_counter, tlp->pkt_counter, tlp->index);

	DrawTextCenter(18, "(STARTで とうけいをクリア)");
	return 0;
}

u32 MP_DownloadLog(u16 push){
	if(push == 0xffff){
		if(IW->gi.state <= GPS_STOP_DOWNLOAD){
			PlaySG1(SG1_CANCEL);
			MenuFillBox(2, 4, 28, 9);
			DrawText(4, 6, "ダウンロードできません!");
			return SetKeyWaitCB(0xffff);
		}

		PlaySG1(SG1_COMP1);
		MenuFillBox(0, 3, 29, 17);
		Locate(2, 5);
		IW->gi.state = GPS_TRACK;
		IW->gi.dl_accept = 1;
		Puts("トラックログのダウンロード");
		DrawText(4, 8, "(Select: ちゅうだん)");
		MT_GPSCtrl();
		return 0;
	}

	if(push & KEY_SELECT){ // 中断
		PlaySG1(SG1_CANCEL);
		IW->mp.tlog.abs_flag |= ABSF_SEPARETE; // セパレータ挿入
		IW->gi.state = GPS_STOP_DOWNLOAD;
		IW->gi.dl_accept = 0;
		IW->mp.navi_update |= NAVI_UPDATE_WPT;
		return 1;
	}

	if(IW->gi.state <= GPS_STOP_DOWNLOAD) return 0;
//	IW->gi.dl_accept = 1;

	IW->mp.tlog.abs_flag |= ABSF_SEPARETE; // セパレータ挿入
	PlaySG1(SG1_COMP2);
	return 1;
}

u32 MP_DownloadLogPre(u16 push){
	if(IW->tc.log_enable){
		if(push == 0xffff){
			PlaySG1(SG1_NEXT);
			MenuFillBox(0, 3, 29, 13);
			DrawText(1, 5,  "ダウンロードちゅうは リアル");
			DrawText(1, 7,  "タイムほぞんを ていしします.");
			DrawText(3, 10, "STARTボタン: ダウンロード");
			return 0;
		}
		if(push & (KEY_A | KEY_B)){
			PlaySG1(SG1_CANCEL);
			return 1;
		}
		if(!(push & KEY_START)) return 0;
	}
	IW->mp.proc = MP_DownloadLog;
	return (*IW->mp.proc)(0xffff);
}

u32 MP_SegmentLog(u16 push){
	if(push == 0xffff){
		PlaySG1(SG1_NEXT);
		MenuFillBox(0, 3, 29, 13);
		DrawText(1, 5,  "つぎのポイントから ログを");
		DrawText(1, 7,  "ぶんかつして よろしいですか?");
		DrawText(4, 10, "STARTボタン: ぶんかつ");
		return 0;
	}
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	if(!(push & KEY_START)) return 0;

	PlaySG1(SG1_COMP2);
	IW->mp.tlog.opt |= TLOG_OPT_MANUAL; // セグメント分割
	return 1;
}

u32 MP_ClearLog(u16 push){
	if(push == 0xffff){
		PlaySG1(SG1_NEXT);
		MenuFillBox(0, 3, 29, 13);
		DrawText(1, 5,  "すべてのトラックログを");
		DrawText(1, 7,  "さくじょして よろしいですか?");
		DrawText(4, 10, "STARTボタン: さくじょ");
		return 0;
	}
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	if(!(push & KEY_START)) return 0;

	PlaySG1(SG1_CLEAR);
	MenuFillBox(3, 4, 26, 9);
	DrawText(5, 6,  "クリアしています...");
	s32 ret = TrackLogClear();
	if(ret){
		PutFlashError(ret);
		return SetKeyWaitCB(0xffff);
	}
	return 1;
}

// 速度アップのため、割り込みを使わず直接転送処理を行う
enum {
	UPS_POLL,
	UPS_POLL_WAIT,
	UPS_BAUD,
	UPS_BAUD_VAL,
	UPS_BAUD_WAIT,
	UPS_DATA,
	UPS_DATA_WAIT,
	UPS_END,
	UPS_END_WAIT,
};

#define PID_POLL '0'
#define PID_BAUD '2'
#define PID_DATA '4'
#define PID_END  '8'

#define PID_ACK 'A'
#define PID_NAK (~PID_ACK)

#define VBC_TIMEOUT(v) ((v) * 60)

s32 CheckSend(u8 ch, u8* sum){
	do {
		if(IW->key_state & KEY_SELECT) return 1; // 中断
	} while(REG16(REG_SIOCNT) & UART_SEND_FULL);
	REG16(REG_SIODATA8) = ch;
	if(sum) *sum -= ch;
	return 0; // 成功
}
s32 RxPurge(){
	while(!(REG16(REG_SIOCNT) & UART_RECV_EMPTY)){
		if(IW->key_state & KEY_SELECT) return 1; // 中断
		REG16(REG_SIODATA8);
	}
	return 0;
}
s32 SendPacketHead(u8 pid, u16 len){
	u8 sum = 0;
	if(CheckSend(pid,      &sum)) return 1;
	if(CheckSend(len >> 8, &sum)) return 1;
	if(CheckSend(len,      &sum)) return 1;

	if(RxPurge()) return 1;// ここで受信をクリアしておく
	if(CheckSend(sum, 0)) return 1; // 1byte sum
	return 0; // 成功
}
// offsetは512の倍数で渡すこと！
s32 SendTrackData(u32 offset, u16 len){
	u8 sum = 0;
	if(CART_IS_DIRECTMAP()){
		// JoyCarryは高速な直ROM読みで処理
		const u8* phy = (u8*)IW->cif->ReadDirect(offset);
		while(len--) if(CheckSend(*phy++, &sum)) return 1;
	} else {
		// SCSDはバッファに読みながら送信。offset/lenは512の倍数であること!
		for(; len ; len -= SECT_SIZE, offset += SECT_SIZE){
			const u8* p = (u8*)IW->cif->ReadDirect(offset); // 512byteしか有効でない
			if(!p) return 1; // リードエラー
			u32 i;
			for(i = 0 ; i < SECT_SIZE ; ++i) if(CheckSend(*p++, &sum)) return 1;
		}
	}
	// sum送信
	if(CheckSend(sum, 0)) return 1;
	return 0; // 成功
}

const u8* const BAUDRATE[4] = {
	"9600", "38400", "57600", "115200"
};

#define UART_9600			0
#define UART_38400			1
#define UART_57600			2
#define UART_115200			3

u32 UploadLog(){
	TrackLogParam* tlp = &IW->mp.tlog;
	u32 full_mode = IW->key_state & KEY_UP;
	u32 blk = IW->cw.tlog_block;
	if(!full_mode && tlp->pkt_counter < 0x100000) blk = MinS32(blk, tlp->blk_counter); // 有効ブロックのみ送信
	
	MenuCls(SELMENU_BG);
	//				123456789012345678901234567890
	DrawTextCenter(0, "< Track Log Uploader >");
	DrawTextCenter(18, "(SELECTボタンでキャンセル)");
	
//	u16 baudrate = IW->tc.log_baudrate & 3;
	REG16(REG_RCNT)		= 0; // SIO Enable
	REG16(REG_SIOCNT)	= UART_MODE | UART_FIFO_DISABLE; // Clear FIFO
	REG16(REG_SIOCNT)	= (IW->tc.bt_mode & 3) | UART_DATA8 | UART_FIFO_ENABLE | UART_PARITY_NONE |
						  UART_SEND_ENABLE | UART_RECV_ENABLE | UART_MODE;

	u32 send_size = 1024;
	DrawText(0, 3, "Connecting... ");
	u32 state = UPS_POLL, vbc = 0;
	u32 send_start = FI_TRACK_OFFSET;
	u32 send_end   = FI_TRACK_OFFSET + BLOCK_SIZE * blk;

	// スタートブロックの検索
	const u32* p;
	while((p = IW->cif->ReadDirect(send_start)) != 0 && *p != FLBL_TRACK_MAGIC && send_start + BLOCK_SIZE < send_end){
		send_start += BLOCK_SIZE;
	}
	u32 send_pos   = send_start;
	u8 toggle = 0;

	while(!(IW->key_state & KEY_SELECT)){
		int ch = -1;
		if(!(REG16(REG_SIOCNT) & UART_RECV_EMPTY)) ch = REG16(REG_SIODATA8);

		switch(state){
		case UPS_POLL:
			if(SendPacketHead(PID_POLL | toggle, 0)) return 1; // 中断
			vbc = IW->vb_counter;
			state = UPS_POLL_WAIT;
			break;

		case UPS_POLL_WAIT:
			if(ch == PID_ACK){
				toggle = 1 - toggle;
				state = UPS_BAUD;
				PlaySG1(SG1_COMP1);
				Putsf("OK%r");
			} else if(ch == PID_NAK || IW->vb_counter - vbc > VBC_TIMEOUT(1)){
				PlaySG1(SG1_CANCEL);
				state = UPS_POLL; // 再送
			}
			break;

		case UPS_BAUD:
			if(SendPacketHead(PID_BAUD | toggle, 0)) return 1; // 中断
			vbc = IW->vb_counter;
			state = UPS_BAUD_VAL;
			break;

		case UPS_BAUD_VAL:
			if(ch != -1){
				if(ch & ~3){ // NAK同等
					PlaySG1(SG1_CANCEL);
					state = UPS_BAUD; // 再送
					break;
				}
				if(ch && !IW->tc.bt_mode) { // ボーレート変更はBluetooth未使用時のみ
					REG16(REG_SIOCNT)	= ch | UART_DATA8 | UART_FIFO_ENABLE | UART_PARITY_NONE |
										  UART_SEND_ENABLE | UART_RECV_ENABLE | UART_MODE;
				}
				Puts("Baudrate ");
				Puts(BAUDRATE[ch]);
				Puts(" bps ");
				send_size <<= ch;
				state = UPS_BAUD_WAIT;
			} else if(IW->vb_counter - vbc > VBC_TIMEOUT(3)){
				PlaySG1(SG1_CANCEL);
				state = UPS_BAUD; // 再送
			}
			break;

		case UPS_BAUD_WAIT:
			if(ch == PID_ACK){
				state = UPS_DATA;
				Putsf("OK%r" "Uploading...");
			} else if(ch == PID_NAK || IW->vb_counter - vbc > VBC_TIMEOUT(3)){
				PlaySG1(SG1_CANCEL);
//				state = UPS_BAUD; // 再送
				state = UPS_DATA;
				Putsf("Skip%r" "Uploading...");
			}
			break;

		case UPS_DATA:
			if(SendPacketHead(PID_DATA | toggle, send_size + 1) || 
				SendTrackData(send_pos, send_size)) return 1; // 中断
			vbc   = IW->vb_counter;
			state = UPS_DATA_WAIT;
			break;

		case UPS_DATA_WAIT:
			if(ch == PID_ACK){
				toggle = 1 - toggle;
				send_pos += send_size;

				// 終端チェック(DirectMap可能なカートリッジのみ実行)
				// DirectMapできない場合は、終端チェックするほうが遅くなる…
				if(!full_mode && send_pos + BLOCK_SIZE > send_end && CART_IS_DIRECTMAP()){
					s32 check = 20; // デバッグ用モードでもOK?
					const u8* phy = (const u8*)IW->cif->ReadDirect(send_pos);
					while(check-- && phy[check] == 0xff);
					if(check < 0) send_end = send_pos; // 強引に終端にする
				}

				if(send_pos < send_end){
					state = UPS_DATA; // 次へ
					Putsf("%13.7m%d bytes", send_pos - send_start);
					break;
				} else {
					PlaySG1(SG1_CHANGE);
					state = UPS_END; // 終了
					Putsf(" OK%r");
				}
			} else if(ch == PID_NAK || IW->vb_counter - vbc > VBC_TIMEOUT(1)){
#define MIN_SEND_SIZE 512 // セクタサイズより小さくしない
				if(send_size > MIN_SEND_SIZE) send_size >>= 1; // サイズを小さくして再送
				state = UPS_DATA; // 再送
				PlaySG1(SG1_CANCEL);
			}
			break;

		case UPS_END:
			if(SendPacketHead(PID_END | toggle, 0)) return 1; // 中断
			vbc = IW->vb_counter;
			state = UPS_END_WAIT;
			break;

		case UPS_END_WAIT:
			if(ch == PID_ACK){
				toggle = 1 - toggle;
				Puts("Done.");
				return 1; // 完了
			} else if(ch == PID_NAK || IW->vb_counter - vbc > VBC_TIMEOUT(1)){
				state = UPS_END; // 再送
				PlaySG1(SG1_CANCEL);
			}
			break;
		}
	}
	return 0;
}

u32 MP_UploadLogPre(u16 push){
	if(push == 0xffff){
		if(!IW->mp.tlog.blk_counter){
			MenuFillBox(0, 3, 29, 8);
			DrawText(3, 5,  "ログデータがありません!");
			return SetKeyWaitCB(0xffff);
		} else {
			MenuFillBox(0, 3, 29, 11);
			DrawText(3, 5,  "PCへアップロードしますか?");
			DrawText(3, 8,  "STARTボタン: アップロード");
		}
		return 0;
	}
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	if(push & KEY_START){
		PlaySG1(SG1_OK);
		StopGPSMode();

		// アップロード
		UploadLog();

		StartGPSMode(IW->tc.bt_mode);
		return 1;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// ルート/ウェイポイントアップロード(テスト用に作成したetrex系専用アップローダ)
///////////////////////////////////////////////////////////////////////////////
u32 UploadTest(u32 mode){
	MenuFillBox(2, 3, 27, 8);
	if(IW->gi.state < GPS_EMUL_INFO || GPS_EMUL_FINISH <= IW->gi.state){
		void GpsEmuMode(u32 mode, const u16* snd);
		// いきなり開始
		GpsEmuMode(mode, SG1_COMP1);
		DrawTextCenter(5, "アップロードかいし");
	} else {
		PlaySG1(SG1_CANCEL);
		DrawTextCenter(5, "そうしんちゅう!");
	}
	return SetKeyWaitCB(0xffff);
}
u32 MP_UploadWpt  (u16 push) { return UploadTest(GPS_EMUL_WPT);  }
u32 MP_UploadRoute(u16 push) { return UploadTest(GPS_EMUL_ROUTE);}

///////////////////////////////////////////////////////////////////////////////
// カートリッジ
///////////////////////////////////////////////////////////////////////////////
s32 PutsHeadInfo(u32 offset, u32 magic){
	const u32* p = IW->cif->ReadDirect(offset);
	if(!p){
		Puts("Read error");
		return 0;
	}
	if(p[0] != magic){
		Puts("No data");
		return 1;
	}
	Putsf("Rewrite %d", p[2]);
	return 2;
}

typedef struct {
	u32 src_dat;
	u32 dst_addr;
	u32 size;
} RomArea;

// RAMブート用
const RomArea RA_RAM[] = {
	{0x00000001,	0x02000000, 	1024 * 256},
	{0, 0, 0}
};

// ジョイキャリーへの複製用
const RomArea RA_CART1[] = {
	// プログラム
	{0x00000001,	0x09f00000, 	1024 * 64},
	{0x00010001,	0x09f10000, 	1024 * 64},
	{0x00020001,	0x09f20000, 	1024 * 64},
	{0x00030001,	0x09f30000, 	1024 * 64},

	// 音声
	{0x00000002,	0x09f40000, 	1024 * 64},
	{0x00010002,	0x09f50000, 	1024 * 64},
	{0x00020002,	0x09f60000, 	1024 * 64},
	{0x00030002,	0x09f70000, 	1024 * 64},
	{0, 0, 0}
};

// ジョイキャリー複製用(隠しモード)
const RomArea RA_CART2[] = {
	// プログラム
	{0x00000001,	0x09f00000, 	1024 * 64},
	{0x00010001,	0x09f10000, 	1024 * 64},
	{0x00020001,	0x09f20000, 	1024 * 64},
	{0x00030001,	0x09f30000, 	1024 * 64},

	// 音声
	{0x00000002,	0x09f40000, 	1024 * 64},
	{0x00010002,	0x09f50000, 	1024 * 64},
	{0x00020002,	0x09f60000, 	1024 * 64},
	{0x00030002,	0x09f70000, 	1024 * 64},

	// タスク設定
	{0x00000004,	0x09ff0000, 	1024 * 8},

	// ルート/ウェイポイント
	{0x00000006,	0x09ff4000, 	1024 * 8},
	{0x00002003,	0x09ff6000, 	1024 * 8},
	{0x00004003,	0x09ff8000, 	1024 * 8},
	{0x00006003,	0x09ffa000, 	1024 * 8},

	// SCSDの場合はココまでしかコピーしない

	// フライトログ
	{0x00000005,	0x09ff2000, 	1024 * 8},

	// トラック
	{0x00000003,	0x09f80000, 	1024 * 64},
	{0x00010003,	0x09f90000, 	1024 * 64},
	{0x00020003,	0x09fa0000, 	1024 * 64},
	{0x00030003,	0x09fb0000, 	1024 * 64},
	{0x00040003,	0x09fc0000, 	1024 * 64},
	{0x00050003,	0x09fd0000, 	1024 * 64},
	{0x00060003,	0x09fe0000, 	1024 * 64},

	{0, 0, 0}
};

u32 GetTotalSize(const RomArea* ra){
	u32 size = 0;
	for(; ra->size && IW->cif->GetCodeAddr(ra->src_dat & 0xf) ; ++ra) size += ra->size;
	return size;
}

#define VBC_500ms	30
#define VBC_1000ms	60
#define VBC_2000ms	120

#define SIO_NORMAL_MASTER	0x1001
#define SIO_NORMAL_SLAVE	0x1000
#define SIO_MPLAY_MASTER	0x2003

void SetCommMode(u32 mode){
	REG16(REG_RCNT)		= 0;
	REG16(REG_SIOCNT)	= mode;
}

u32 SendRecv(u32 val){
	REG16(0x400012a) = val;
	REG16(REG_SIOCNT) |= 0x80;
	while(REG16(REG_SIOCNT) & 0x80){
		if(IW->key_state & KEY_SELECT) return -1;
	}
	return REG16(0x4000122);
}

// ACK=0x1XXX
u32 WaitAck(u32 n){
	for(;;){
		u32 val = SendRecv(0);
		if(val == -1) return -1;	// キャンセル
		if(val == 0xaaaa) return -2;// 同期文字
		if((val & 0xf000) == 0x1000) return val & 0xfff; // ACK受信
	}
}

u32 SendRecv32(u32 val){
	REG32(0x4000120) = val;
	REG16(REG_SIOCNT) |= 0x80;
	while(REG16(REG_SIOCNT) & 0x80){
		if(IW->key_state & KEY_SELECT) return -1;
	}
	return REG32(0x4000120); // -1は返さない!
}

void WaitMS(u32 ms){
	ms = (ms >> 4) + 1; // これで十分
	u32 cur = IW->vb_counter;
	while(IW->vb_counter - cur <= ms) BiosHalt();
}

const u32 BOOTBIN_ADDR[] = {
	0x09ffc000, // JoyCarry
	((u32)&__iwram_overlay_lma) + (0x08000000 - 0x02000000) // SuperCard結合
};
u32 MBoot(){
	SetCommMode(SIO_MPLAY_MASTER);

	MenuCls(SELMENU_BG);
	//				123456789012345678901234567890
	DrawText(0, 0, "1.GBAをケーブルせつぞくします.");
	DrawText(0, 2, "  (コピーさき: グレイコネクタ)");
	DrawText(0, 4, "2.START+SELECTボタンをおしなが");
	DrawText(0, 6, "  ら,コピーさきをブートします.");
	DrawTextCenter(18, "SELECTボタンでキャンセル");

	// 最初にブートセクタのブートコードを送り込む /////////////////////////////
	PlaySG1(SG1_OK);
	FillBox(2, 9, 27, 16);
	DrawTextCenter(11, "  Connecting...  ");
	u32 id, val;
	u32 vbc = IW->vb_counter;
	while(((id = SendRecv(0x6200)) & 0xfff0) != 0x7200){
		if(id == -1) return 1; // キャンセルボタン

		// 一定時間待っても通信できないときには、UARTを初期化してリトライする
		if(IW->vb_counter - vbc > VBC_500ms){
			WaitMS(100);
			SetCommMode(SIO_MPLAY_MASTER);// リセット
			WaitMS(100);
			vbc = IW->vb_counter;
		}
	}
	id &= 0xf;

	PlaySG1(SG1_CONNECT);

	DrawTextCenter(11, "Sending header...");
#define SendRecvX(val)  do { if(SendRecv(val) == -1) return 1; } while(0)
#define SendRecvX2(val) do { if(SendRecv(val >> 16) == -1 || SendRecv(val & 0xffff) == -1) return 1; } while(0)
	SendRecvX(0x6100 | id);
	u16* p = (u16*)IW->cif->GetCodeAddr(CODE_TYPE_BOOT);
	s32 i;
	for(i = 0 ; i < 0xC0 / 2 ; ++i) SendRecvX(*p++);
	SendRecvX(0x6200);
	SendRecvX(0x6200 | id);

	MBootParam mbp;
	memset(&mbp, 0, sizeof(mbp));
	mbp.pal  = 0xef;
	mbp.cb	 = id;
	mbp.pc	 = 0xd1;
	mbp.srcp = (u32)p;
	mbp.endp = mbp.srcp + 8 * 1024 - 0xc0;
	SendRecvX(0x6300 | mbp.pal);
	SendRecvX(0x6300 | mbp.pal);
	u8 key=0x11;
	key += mbp.cd[0] = REG16(0x4000122) & 0xff;
	key += mbp.cd[1] = REG16(0x4000124) & 0xff;
	key += mbp.cd[2] = REG16(0x4000126) & 0xff;
	mbp.handshake_data = key;
	SendRecvX(0x6400 | key);

	DrawTextCenter(11, "Sending loader...");
	BiosMBoot(&mbp);


	// データ送信 /////////////////////////////////////////////////////////////
	DrawTextCenter(11, "Checking target...");
	WaitMS(1);
	SetCommMode(SIO_MPLAY_MASTER); // 念のため再設定

	// 接続先情報取得(カートリッジ有無)
	val = WaitAck(0);
	const RomArea* rap = (val & 0xf)? RA_CART1 : RA_RAM;
	if(IW->key_state & KEY_UP) rap = RA_CART2; // 強制完全コピー

	// プログレス表示用
	u32 total_size = GetTotalSize(rap), cur_size = 0;

	PlaySG1(SG1_COMP1);
	if(rap == RA_RAM){
		DrawTextCenter(11, "Copying WRAM...   ");
	} else if(rap == RA_CART1){
		DrawTextCenter(11, "Duplicating cart...");
	} else if(rap == RA_CART2){
		DrawTextCenter(11, "Duplicating full...");
	} else {
		DrawTextCenter(11, "Duplicating X3...");
	}

	// データ送信
	for(;;){
		// 同期
		val = SendRecv(0x5555);
		if(val == -1) return 1;
		if(val != 0xaaaa) continue;

		// サイズ情報取得
		s32 size = rap->size;
		s32 dst  = rap->dst_addr;
		u16* src = (u16*)(IW->cif->GetCodeAddr(rap->src_dat & 0xf));
		if(!src) size = dst = 0; // 強制終了
		SendRecvX2(size); // 0サイズ通知で対向装置が完了検出して自動リブートする。
		SendRecvX2(~size);
		SendRecvX2(dst);
		SendRecvX2(~dst);
		val = WaitAck(2);
		if(val == -2) continue; // 再同期
		if(!size) break;

		// データ送信
		if(!src) break;
		src += (rap->src_dat & ~0xf) >> 1; // word
		u32 bcc = 0;
		vbc = IW->vb_counter;
		for(i = 0 ; i < size ; i += 2){
			val = SendRecv(*src);
			if(val == -1 || val == 0xffff || !val) return 1;
			bcc += *src++;
			if(IW->vb_counter - vbc > VBC_1000ms){
				Putsf("%12.13m%3d%%", BiosDiv((cur_size + i) * 100, total_size, &val));
				vbc = IW->vb_counter;
			}
		}

		// BCCチェック
		SendRecvX2(bcc);
		val = WaitAck(4);
		if(val != 1) continue;

		// 書き込み待ち
		val = WaitAck(6);
		if(val != 2) continue;

		// ブロック転送完了
		cur_size += rap->size;
		rap++; // 次のデータへ
	}

	return 0; // success
}

///////////////////////////////////////////////////////////////////////////////
// 個人情報対策の完全消去
///////////////////////////////////////////////////////////////////////////////
const u32 CLEAR_8192[] = {
//	FI_CONFIG_OFFSET, // Configには個人情報は含まれないので削除しない。
	FI_FLOG_OFFSET,
	FI_ROUTE_OFFSET + BLOCK_TASK_SIZE * 0,
	FI_ROUTE_OFFSET + BLOCK_TASK_SIZE * 1,
	FI_ROUTE_OFFSET + BLOCK_TASK_SIZE * 2,
	FI_ROUTE_OFFSET + BLOCK_TASK_SIZE * 3,
	-1
};

// 乱数データでの上書きも必要?
u32 FullClear(){
	// 8KBブロック消去
	const u32* p = CLEAR_8192;
	while(*p != -1){
		s32 ret = IW->cif->EraseBlock(*p++, BLOCK_TASK_SIZE, ERASE_CLEAR);
		if(ret) return ret;
	}

	// 64KBトラックログ消去
	u32 i, offset = FI_TRACK_OFFSET;
	for(i = 0 ; i < IW->cw.tlog_block ; ++i, offset += BLOCK_SIZE){
		Putsf("%9.10mTrack%d/%d", i + 1, IW->cw.tlog_block);
		s32 ret = IW->cif->EraseBlock(offset, BLOCK_SIZE, ERASE_CLEAR);
		if(ret) return ret;
	}
	return 0;
}

u32 MP_Initialize(u16 push){
	if(push == 0xffff){
		PlaySG1(SG1_CHANGE);
		MenuFillBox(0, 4, 29, 16);
		DrawTextUL(1, 1, "こじんじょうほうの しょきか");
		DrawText(2, 6, "Route/Waypoint/Track/Logを");
		DrawText(2, 8, "かんぜんに しょきかします.");
		DrawText(4,11, "STARTボタン: しょきか");
		DrawText(4,13, "Bボタン:     キャンセル");
		IW->mp.proc = MP_Initialize; // コールバックを変える
		return 0;
	}
	if(push & KEY_START){
		PlaySG1(SG1_CLEAR);
		MenuFillBox(2, 6, 27, 13);
		DrawTextCenter(8, "しょきか しています...");

		// 完全消去
		s32 ret = FullClear();
		if(ret){
			PutFlashError(ret);
			return SetKeyWaitCB(0xffff);
		}

		// 自分のデータも初期化しておく
		ROUTE->route_count = 0;
		WPT->wpt_count = 0;
		InitLogParams();
		push = KEY_A;
	}
	if(push & (KEY_A | KEY_B)){
		u32 MP_Cart(u16 push);
		IW->mp.proc = MP_Cart; // コールバックを戻す
		PlaySG1(SG1_CANCEL);
		return MP_Cart(0xffff);
	}
	return 0;
}

#define CARTTEST_OFFSET FI_CONFIG_OFFSET	// コンフィグブロック
#define CARTTEST_SIZE	(8192)				// 8KB
#define TEST_NUM		1
void PutsPerf(u32 err, u32 t){
	if(err){
		Putsf("Error (%d)", err);
		return;
	}
	Putsf("%d ms", ((IW->anemo.tm - t) * 1000) >> 14);
}

u32 MP_CartTest(u16 push){
	if(push == 0xffff){
		PlaySG1(SG1_CHANGE);
		MenuFillBox(3, 2, 26, 17);
		DrawTextUL(6, 4, "カートリッジテスト");
		DrawText  (6, 7, "STARTボタン:かいし");
		return 0;
	}
	if(push & KEY_START){
		VoiceStop(); // テストに音声バッファを使うため音声DMA停止
		PlaySG1(SG1_COMP2);
		u32 i;
		Putsf("%6.7mTestSize=%d * %d ", CARTTEST_SIZE, TEST_NUM);
		u32 ret = 0, t;

		Putsf("%9.10mErase: "); // JoyC:133ms, SCSD-UMAX:251ms
		t = IW->anemo.tm; // 16.78KHz
		for(i = 0 ; i < TEST_NUM ; ++i){
			ret = IW->cif->EraseBlock(CARTTEST_OFFSET, CARTTEST_SIZE, ERASE_FF);
			if(ret) break;
		}
		PutsPerf(ret, t);

		DmaClear(3, 0x55aaaa55, VOICE_ADDRESS, VOICE_BUF_SIZE, 32); // テストパターンは55aaaa55
		Putsf("%9.12mWrite: "); // JoyC:301ms, SCSD-UMAX:251ms
		t = IW->anemo.tm; // 16.78KHz
		for(i = 0 ; i < TEST_NUM ; ++i){
			ret = IW->cif->WriteData(CARTTEST_OFFSET, (void*)VOICE_ADDRESS, CARTTEST_SIZE); // コピー元は音声用バッファ
			if(ret) break;
			ret = IW->cif->Flush();
			if(ret) break;
		}
		PutsPerf(ret, t);

		Putsf("%9.14mRead:  "); // JoyC:0ms, SCSD-UMAX:33ms
		t = IW->anemo.tm; // 16.78KHz
		for(i = 0 ; i < TEST_NUM ; ++i){
			ret = IW->cif->ReadData(CARTTEST_OFFSET, (void*)VOICE_ADDRESS, CARTTEST_SIZE, 0); // コピー先は音声用バッファ
			if(ret) break;
		}
		PutsPerf(ret, t);

		// 設定を書き戻しておく
		IW->cif->WriteData(FI_CONFIG_OFFSET, &IW->tc, sizeof(TaskConfig));
	}
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	return 0;
}

#define MAX_POS 55 // MAX 512 byte
#define TEST_PAGE_JUMP1 (SECT_SIZE * 16)  //  8KB
#define TEST_PAGE_JUMP2 (SECT_SIZE * 128)  // 64KB
#define TEST_PAGE_JUMP3 (SECT_SIZE * 2048) // 1MB
#define TEST_LINE_JUMP (9)

u32 MP_CartTest2(u16 push){
	if(push == 0xffff){
		MenuCls(SELMENU_BG);
		IW->mp.mp_flag = 0;
		IW->mp.scr_pos = 0;
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 1;
	} else if((push & KEY_LEFT) && IW->mp.mp_flag >= SECT_SIZE){
		IW->mp.mp_flag -= (IW->key_state & KEY_A)? TEST_PAGE_JUMP1 : SECT_SIZE;
		if((s32)IW->mp.mp_flag < 0) IW->mp.mp_flag = 0;
	} else if(push & KEY_RIGHT){
		IW->mp.mp_flag += (IW->key_state & KEY_A)? TEST_PAGE_JUMP1 : SECT_SIZE;
	} else if((push & KEY_L) && IW->mp.mp_flag >= SECT_SIZE){
		IW->mp.mp_flag -= (IW->key_state & KEY_A)? TEST_PAGE_JUMP3 : TEST_PAGE_JUMP2;
		if((s32)IW->mp.mp_flag < 0) IW->mp.mp_flag = 0;
	} else if(push & KEY_R){
		IW->mp.mp_flag += (IW->key_state & KEY_A)? TEST_PAGE_JUMP3 : TEST_PAGE_JUMP2;
	} else if((push & KEY_UP) && IW->mp.scr_pos){
		IW->mp.scr_pos -= (IW->key_state & KEY_A)? TEST_LINE_JUMP : 1;
		if((s32)IW->mp.scr_pos < 0) IW->mp.scr_pos = 0;
	} else if((push & KEY_DOWN) && IW->mp.scr_pos < MAX_POS){
		IW->mp.scr_pos += (IW->key_state & KEY_A)? TEST_LINE_JUMP : 1;
		if(IW->mp.scr_pos > MAX_POS) IW->mp.scr_pos = MAX_POS;
	} else if(push & KEY_SELECT){
		IW->mp.disp_mode ^=  1;
	} else if(push & KEY_START){
		IW->mp.disp_mode ^=  2;
	} else if(push & KEY_A){
		// reload
	} else {
		return 0;
	}
	VoiceStop(); // テストに音声バッファを使うため音声DMA停止
	PlaySG1(SG1_SELECT);
	Putsf("%.mSECTOR DUMP%d %08x:%08x", IW->mp.disp_mode & 2, IW->mp.mp_flag >> 9, IW->mp.mp_flag + (IW->mp.scr_pos << 3));
	s32 ret = IW->cif->ReadData(IW->mp.mp_flag, (void*)VOICE_ADDRESS, SECT_SIZE, (IW->mp.disp_mode & 2)? 2 : 1); // コピー先は音声用バッファ
	if(ret){
		Putsf("%.2m<*** READ ERROR %d ***>  ", ret);
	} else {
		PutLastBuf((u8*)VOICE_ADDRESS, SECT_SIZE, IW->mp.disp_mode & 1);
	}
	return 0;
}

#define KEY_INITIALIZE	(KEY_L | KEY_R)

u32 MP_Cart(u16 push){
	if(push == 0xffff){
		MenuCls(SELMENU_BG);
		DrawTextCenter(1, "< カートリッジ >");
		DrawText(0,  6, " Config Area:   ");
		PutsHeadInfo(FI_CONFIG_OFFSET, FLBL_TASK_MAGIC);
		DrawText(0,  8, " Route/Wpt Area:");
		PutsHeadInfo(FI_ROUTE_OFFSET, FLBL_ROUTE_MAGIC);
		if(CART_IS_WRITEABLE()){
			Putsf("%.4m Generation:    %d", GEN_COUNTER);
			DrawText(0, 11, "L+Rボタンをおすと");
			DrawText(4, 13, "イニシャライズできます.");
		}
		if(CART_IS_DUPLICATE()){
			DrawText(0, 16, "STARTボタンをおすと");
			DrawText(4, 18, "べつのGBAにコピーできます.");
		}
		return 0;
	}
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	if((IW->key_state & KEY_INITIALIZE) == KEY_INITIALIZE && CART_IS_WRITEABLE()){
		return MP_Initialize(0xffff);
	}
	if((push & KEY_START) && CART_IS_DUPLICATE()){
		StopGPSMode();

		// 複製
		if(MBoot())	PlaySG1(SG1_CANCEL);
		else		PlaySG1(SG1_COMP2);

		StartGPSMode(IW->tc.bt_mode);
		MP_Cart(0xffff);
//		return SetKeyWaitCB(0xffff);
		return 0; // そのまま終了
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// コンフィグ初期化
///////////////////////////////////////////////////////////////////////////////
u32 MP_Reload(u16 push){
	if(push == 0xffff){
		PlaySG1(SG1_NEXT);
		MenuFillBox(0, 3, 29, 13);
		DrawText(1, 5,  "すべてのせっていを しょきか");
		DrawText(1, 7,  "して よろしいですか?");
		DrawText(4, 10, "STARTボタン: しょきか");
		return 0;
	}
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	if(!(push & KEY_START)) return 0;

	PlaySG1(SG1_CLEAR);
	extern const TaskConfig INIT_TASK_CONFIG;
	IW->tc = INIT_TASK_CONFIG;
	IW->mp.save_flag |= SAVEF_UPDATE_CFG; // 設定変更フラグ
	return 1;
}
///////////////////////////////////////////////////////////////////////////////
// セーブ
///////////////////////////////////////////////////////////////////////////////
void SetupHead(u32* head, u32 magic, u32 size){
	if(head[0] == magic) head[2] ++;
	else                 head[2] = 1;
	head[0] = magic;
	head[1] = size;
	head[3] = 0;
}

void SaveFlash(){
	MenuFillBox(3, 4, 25, 9);
	DrawText(5, 6, "セーブしています...");

	// 音声DMA停止
	VoiceStop();
	EnableSound(SOUND_CH_VARIO, -1); // バリオ音は止めておく
	SetupHead(&IW->tc.magic, FLBL_TASK_MAGIC, TDataSize(IW->tc));
	s32 ret = IW->cif->WriteData(FI_CONFIG_OFFSET, &IW->tc, sizeof(TaskConfig));
	if(!ret && (IW->mp.save_flag & SAVEF_SAVE_WPT)){
		SetupHead(&ROUTE->magic, FLBL_ROUTE_MAGIC, ROUTE_WPT_SIZE);
		ret = IW->cif->WriteData(FI_ROUTE_OFFSET, ROUTE, ROUTE_WPT_SIZE);
	}
	IW->mp.save_flag &= ~SAVEF_SAVE_WPT;

	PlaySG1(ret? SG1_CANCEL : SG1_CHANGE);
	PutFlashError(ret);
}

u32 MP_Save(u16 push){
	SaveFlash();
	return SetKeyWaitCB(0xffff);
}

///////////////////////////////////////////////////////////////////////////////
// 測位状態
///////////////////////////////////////////////////////////////////////////////
// スペース改行で出力
void PutsLongStr(const u8* str){
	while(*str){
		u8* p = strchr(str, ' ');
		u32 len = p? (p - str + 1) : PutsLen(str);
		if(IW->mp.gx + len > 30 && len <= 30) Putsf("%r");
		PutsName2(str, len, 0);
		str += len;
	}
}

const u8* const NMEA_LOG_NMAE[NMEA_LOG_ID_COUNT] = {
	"GGA", "GSA", "GSV", "RMC", "VTG", "ZDA", "GLL", "ALM",
	"E_U", "E_F", "E_T", "Sat",
};
// ユニット情報
void MP_GPSCheck1(){
	DrawTextCenter(0, "< GPS information 1/6 >");
	DrawTextUL(0, 4, "Unit information:");
	if(IW->gi.pid == NMEA_DEV_ID){
		s32 i;
		Puts(" (NMEA-0183)");
		Locate(2, 7);
		for(i = 0 ; i < NMEA_LOG_ID_COUNT ; ++i){
			Putsf("%s:%7d    ", NMEA_LOG_NMAE[i], IW->nmea_log[i]);
		}
	} else { 
		DrawText(2, 7, "Product ID = ");
		if(IW->gi.pid) Putsf("%d", IW->gi.pid);
		else           Puts("Unknown");

		DrawText(2,10, "FW Level   = ");
		if(IW->gi.version) Putsf("%d", IW->gi.version);
		else               Puts("Unknown");

		if(*IW->gi.name){
			Locate(0, 13);
			PutsLongStr(IW->gi.name);
		}
	}
}

s32 GetAngK(u32 ang64){
//	return (ang64 & 0xffff) * 36000 / 256 * 10 / 256;
	return ((ang64 & 0xffff) * 45000) >> 13;
}

void MP_GPSCheck2(){
	DrawTextCenter(0, "< GPS information 2/6 >");
	const PvtX* px = &IW->px;
	s32 lat = px->lat;
	if(lat == INVALID_LAT) lat = 0;
	s32 va = GetAngK(px->v_ang64);
	if(va > 180000) va -= 360000;

	Putsf("%.2m"
		"Lat:   %N%r"
		"Lon:   %E%r"
		"Alt: %5.3fm%r"
		"Error:%4.3fm (2 sigma)%r"
		"ErrHV:%4.3fm;H%4.3fm;V%r"
		"DirHV:%4.3f°;H%4.3f°;V%r"
		"VelHV:%4.3fm/s%4.3fm/s;V%r"
		"Date:  %T",
		lat, px->lon, px->alt_mm, px->epe_mm, px->eph_mm, px->epv_mm, GetAngK(px->h_ang_c),
		va, px->vh_mm, px->up_mm, px->dtime);

	if(px->dtime) Putsf(" (%s)", WEEK_NAME[px->week]);

	u32 m, h = BiosDiv(IW->tc.tzone_m, 60, &m);
	Putsf("%rTime:  %t (UTC%+d:%02d)", px->dtime, h, m);
	return;
}

// 通信状態チェック
typedef struct {
	const u8* msg;
	u32* val;
} ComInfo;

#define IW_OFFSET(v) ((u32*)(0x03000000 + offsetof(MyIWRAM, v)))
const ComInfo COM_INFO[] = {
	{"Error byte",		IW_OFFSET(dl_drop)},
	{"Packet Drop",		IW_OFFSET(dl_drop_pkt)},
	{"Packet Timeout",	IW_OFFSET(dl_timeout)},
	{"Packet Recv",		IW_OFFSET(px.counter)},
	{"  Unusable",		IW_OFFSET(px.fix_sum[0])},
	{"  Invalid",		IW_OFFSET(px.fix_sum[1])},
	{"  2D, 2D-diff",	IW_OFFSET(px.fix_sum[2])},
	{"  3D, 3D-diff",	IW_OFFSET(px.fix_sum[3])},
	{0, 0}
};
	
void MP_COMCheck(){
	u32 i;
	DrawTextCenter(0, "< GPS information 3/6 >");
	DrawText(0, 2, "Communication:");
	for(i = 0 ; COM_INFO[i].msg ; ++i){
		Putsf("%r%2P%s%16P%7d", COM_INFO[i].msg, COM_INFO[i].val[0]);
	}
	Putsf("%r%23P, %d", IW->px.fix_sum[4]);
	Putsf("%r%23P, %d", IW->px.fix_sum[5]);
}

void MP_GPSDebug1(){
	switch(IW->mp.last_pvt_len & DBGDMP_TYPE){
	case DBGDMP_SCSD:
		DrawTextCenter(0, "< SCSD FAT DUMP (4/6) >");
		break;
	case DBGDMP_COM:
		DrawTextCenter(0, "< Law NMEA DUMP (4/6) >");
		break;
	default:
		DrawTextCenter(0, "< GPS LAST PVT DUMP (4/6) >");
		break;
	}
	PutLastBuf(IW->mp.last_pvt, IW->mp.last_pvt_len, IW->mp.disp_mode);
}
void MP_GPSDebug2(){
	switch(IW->mp.last_wpt_len & DBGDMP_TYPE){
	case DBGDMP_SCSD:
		DrawTextCenter(0, "< SCSD MBR DUMP (5/6) >");
		break;
	case DBGDMP_COM:
		DrawTextCenter(0, "< Law Rx DUMP (5/6) >");
		break;
	default:
		DrawTextCenter(0, "< GPS LAST WPT DUMP (5/6) >");
		break;
	}
	PutLastBuf(IW->mp.last_wpt, IW->mp.last_wpt_len, IW->mp.disp_mode);
}
void MP_GPSDebug3(){
	switch(IW->mp.last_route_len & DBGDMP_TYPE){
	case DBGDMP_COM:
		DrawTextCenter(0, "< Raw Tx DUMP (6/6) >");
		break;
	default:
		DrawTextCenter(0, "< GPS LAST ROUTE DUMP (6/6) >");
		break;
	}
	PutLastBuf(IW->mp.last_route, IW->mp.last_route_len, IW->mp.disp_mode);
}

// GPSチェック
const TallyProc GPS_PROC[] = {
	MP_GPSCheck1,
	MP_GPSCheck2,
	MP_COMCheck,
	MP_GPSDebug1,
	MP_GPSDebug2,
	MP_GPSDebug3,
};

u32 MP_GPS(u16 push){
	if(push == 0xffff){
		IW->mp.mp_flag = 0;
		IW->mp.scr_pos = 0;
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 1;
	} else if(IW->mp.mp_flag == 0 && (push & KEY_UP)){
		PlaySG1(SG1_PREV);
		IW->mp.mp_flag = ARRAY_SIZE(GPS_PROC) - 1;
		IW->mp.scr_pos = MAX_POS;
	} else if(IW->mp.mp_flag > 2 && (push & KEY_UP)){
		if(IW->mp.scr_pos){
			PlaySG1(SG1_SELECT);
			IW->mp.scr_pos--;
			push = 0;
		} else {
			PlaySG1(SG1_PREV);
			IW->mp.scr_pos = MAX_POS;
			IW->mp.mp_flag--;
		}
	} else if(IW->mp.mp_flag == 2 && (push & KEY_DOWN)){
		PlaySG1(SG1_NEXT);
		IW->mp.mp_flag++;
		IW->mp.scr_pos = 0;
	} else if(IW->mp.mp_flag > 2 && (push & KEY_DOWN)){
		PlaySG1(SG1_SELECT);
		if(IW->mp.scr_pos < MAX_POS){
			PlaySG1(SG1_SELECT);
			IW->mp.scr_pos++;
			push = 0;
		} else {
			PlaySG1(SG1_NEXT);
			IW->mp.scr_pos = 0;
			if(++IW->mp.mp_flag >= ARRAY_SIZE(GPS_PROC)) IW->mp.mp_flag = 0;
		}
	} else if(push & KEY_LOG_NEXT){
		PlaySG1(SG1_NEXT);
		if(++IW->mp.mp_flag >= ARRAY_SIZE(GPS_PROC)) IW->mp.mp_flag = 0;
	} else if(push & KEY_LOG_PREV){
		PlaySG1(SG1_PREV);
		if(!IW->mp.mp_flag--) IW->mp.mp_flag = ARRAY_SIZE(GPS_PROC) - 1;
	} else if(push & (KEY_SELECT | KEY_START)){
		IW->mp.disp_mode ^= 1;
		
//	} else if(IW->mp.pvt_check == IW->px.counter){
	} else {
		push = 0;
//		return 0;
	}
	IW->mp.pvt_check = IW->px.counter;
	if(push) MenuCls(SELMENU_BG);
	(*GPS_PROC[IW->mp.mp_flag])();

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// アネモメータ
///////////////////////////////////////////////////////////////////////////////
u32 MP_Anemometer(u16 push){
	if(push == 0xffff){
		MenuCls(SELMENU_BG);
		DrawTextCenter(0, "< Anemometer Test >");
		IW->mp.mp_flag = 0;
	} else if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	CalcAnemometer();
	Putsf("%2.4m" "Pulse:%8d%r" "Dt:%11d%r" "RPM:%6.3f,%6.3f%r" "Speed:%4.3f m/s%r" "%6.12m%6.3f km/h",
		IW->anemo.pulse, IW->anemo.dif_tm, IW->anemo.rpm_avg, IW->anemo.rpm, IW->anemo.vel, RoundDiv(IW->anemo.vel * 36, 10));

	if(!IW->tc.anemo_coef || IW->tc.anemo_coef == -1){
		DrawText( 0, 17, "Please calibrate your device!!");
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// フライト分析
///////////////////////////////////////////////////////////////////////////////
// 分析メニュー用出力関数
void PutsPercent(s32 a, s32 b){
	Putsf("%+8S", a, b);
}
void PutPercent2(s32 num, s32 denom){
	Putsf("%3d%%", RoundDiv(num * 100, denom));
}
void PutsVario(s32 val, s32 f){
	if(f)	Putsf("%+4.3f", val);
	else	Puts("   --m/s");
}
void PutsVario2(s32 valK, s32 div){
	if(div) valK = RoundDiv(valK, div);
	PutsVario(valK, div);
}
static void PutsVario3(s32 valK, s32 div){
	if(div) PutsVario(RoundDiv(valK, div), 1);
	else    Puts("   -----");
	Puts("m/s");
}
void PutsTurn(s32 num, s32 turn, s32 f){
	if(f)	Putsf("%8d", RoundShift(turn + num * CETERING_DETECT, 16)); // 検出までの回転量を加算して計算
	else	Puts("   -----");
}
void PutsTurn2(s32 num, s32 turn){
	if(num)	Putsf("%6.1f", BiosDiv((turn + num * CETERING_DETECT), num * 6553, &num));
	else	Puts("   -----");
}
static void PutsRad(s32 len, s32 turn){
	if(turn){
		s32 t = RoundDiv(turn, 104); // 65536/ 2PI
		if(t) t = RoundDiv(len, t);
		Putsf("%5.1fm", t);
	} else {
		Puts("   ----m");
	}
}
static void PutsRPM(s32 turn, s32 count){
	if(count)	Putsf("%4.3f", RoundShift(RoundDiv(turn, count) * 60000, 16));
	else		Puts("   -----");
}
void PutsGain(s32 val, s32 f){
	if(f)	Putsf("%7dm", RoundDiv(val, 1000));
	else	Puts("   ----m");
}
void PutsGainP(s32 v, s32 sum){
	Putsf("%6dm", RoundDiv(v, 1000));
	if(sum){
		if(sum > 10000000){ // 桁あふれをちょっとでも防ぐ
			sum >>= 8;
			v   >>= 8;
		}
		Putsf(" (%3d%%)", RoundDiv(v * 100, sum));
	}
}

void PutsSpd8(s32 v, s32 c){
	if(!c){
		if(IW->tc.spd_unit) Puts("  --km/s");
		else                Puts("  ---m/s");
	} else {
		if(c != 1) v = RoundDiv(v, c);
		if(IW->tc.spd_unit){
			if(v > 10000000) v = RoundDiv(v, 10) * 36;
			else             v = RoundDiv(v * 36, 10);
		}
		Putsf("%4.3f", v);
	}
}

static void PutsPoint9(s32 valK, s32 div){
	if(div){
		if(div != 1) valK = BiosDiv(valK, div, &div);
		Putsf("%3.3fG", valK);
	}
	else {
		Puts("   ----G");
	}
}

// フライト分析
void PutClearMessage(u32 val){
	if(val == -1){
		DrawText(1, 16, "STARTボタンでルートこうしん");
		DrawText(0, 18, "(いまのログはさくじょされます)");
	} else if(val){
		Putsf("%0.18mL<=  %T  %t  =>R", val, val);
	} else {
		DrawText(0, 18, "L<=    (STARTでクリア)     =>R");
	}
}
void TallyMenu(){
	MenuCls(SELMENU_BG);
	const Tally* tl = GetTallyData();
	if(tl == &IW->mp.tally){
		PutClearMessage(0);
	} else {
		PutClearMessage(tl->log_time);
	}
	SetVideoMode(0); // グラフ専用モード解除
}

// 直線: Raise/Sink
// Vario/-.5 0 .0 1 1.5 4.5
// 最高値、平均値
void UpdateTally1(){
	const Tally* tl = GetTallyData();

	s32 avg = 0, max = 0;
	const u8* unit;
	if(IW->tc.spd_unit){
		unit = "km/h";
		if(tl->count){
			avg = (u32)(tl->sum_v /  tl->count * 18 / 5);
			max = RoundDiv(tl->max_v * 36, 10);
		}
	} else {
		unit = "m/s";
		if(tl->count){
			avg = (u32)(tl->sum_v /  tl->count);
			max = tl->max_v;
		}
	}

	Putsf("%2.3m"
			"Speed Avg =%F%s%r"
			"      Max =%F%s%r"
			"Lift  Max =%+Fm/s%r"
			"Sink  Max =%+Fm/s%r"
			"Alt.  Max =%Dm%r"
			"Accel.Max =%FG%r"
			"      Min =%FG",
			avg, tl->count, unit,
			max, tl->count, unit,
			tl->max_up, tl->count,
			tl->min_up, tl->count,
			RoundDiv(tl->max_alt, 1000), tl->count,
			tl->max_G, tl->max_G >= tl->min_G,
			tl->min_G, tl->max_G >= tl->min_G);
}

void UpdateTally2(){
	const Tally* tl = GetTallyData();
	DrawTextUL(1, 3, "Distance:");
	Putsf("%3.6m"
			"Trip meter =%8dm%r"
			"Sect meter =%8dm%r"
			"Total gain =%8dm",
			(u32)((tl->sum_v + 500) / 1000), (u32)((tl->trip_mm + 500) / 1000), RoundDiv(tl->sum_gain, 1000)); // 64bit値を使用している!
}

// ローリング、ピッチング
void UpdateTally3(){
	const Tally* tl = GetTallyData();
	DrawTextUL(1, 3, "Motion statistics:");

	Putsf("%3.6m"
			"Pitching   =%4d times%r"
			"C-Pitching =%4d times%r"
			"Rolling    =%4d times%r"
			"Centering  =%4d turn%r"
			"Spiral     =%4d turn%r"
			"Stall      =%4d second",
			tl->pitch_s, tl->pitch_r, tl->roll, RoundShift(tl->center_sum, 16), RoundShift(tl->spiral_sum, 16), tl->stall_sec);
}

// カウンタ
void UpdateTally4(){
	const Tally* tl = GetTallyData();
	DrawTextUL(1,  3, "Status statistics:");
	Putsf("%3.6m"
			"Straight   =%+8S%r" // 比率付特殊フォーマット！
			"Left turn  =%+8S%r"
			"Right turn =%+8S%r"
			"H Stop     =%+8S%r"
			"Pre flight =%+8S%r",
			tl->s_count, tl->count, // 2コ組
			tl->turn_l,  tl->count,
			tl->turn_r,  tl->count,
			tl->w_count, tl->count,
			tl->count - (tl->s_count + tl->w_count + tl->turn_l + tl->turn_r), tl->count);
}


// タイム
void UpdateTally5(){
	const Tally* tl = GetTallyData();

	DrawTextUL(1,  3, "Duration:");
	if(IsCurFlog()){
		Putsf("%3.6mTakeoff time = %t", tl->takeoff_time);
		Putsf("%3.8mFlight time  = %8S", tl->takeoff_time? (tl->last_sec - tl->takeoff_time) : -1);
		u32 t;
		Putsf("%3.11mBoot time    = %8S", BiosDiv(IW->vb_counter, 60, &t));
		Putsf("%3.13mResume time  = %8S", BiosDiv(IW->vb_counter - IW->mp.resume_vbc, 60, &t));
	} else {
		if(tl->takeoff_time){
			Putsf("%3.6mFlight time  = %8S%3r" "Takeoff date = %T%r" "Takeoff time = %t",
				tl->last_sec - tl->takeoff_time, tl->takeoff_time, tl->takeoff_time);
		} else {
			DrawText(3,  6, "Flight time  = No data");
		}
	}
}

void UpdateTally6x1(const s32* soar){
	s32 sum = soar[0] + soar[1] + soar[2] + soar[3];

	DrawText(3,  6, "Centering =");	PutsPercent(soar[3], sum);
	DrawText(3,  8, "Turn      =");	PutsPercent(soar[1], sum);
	DrawText(3, 10, "Straight  =");	PutsPercent(soar[0], sum);
	DrawText(3, 12, "H Stop    =");	PutsPercent(soar[2], sum);
	DrawText(3, 14, "Total     =");	Putsf("%8S", sum);
}
void UpdateTally6x2(const s32* soar){
	s32 sum = soar[0] + soar[1] + soar[2] + soar[3];

	DrawText(3,  6, "Centering =");	PutsGainP(soar[3], sum);
	DrawText(3,  8, "Turn      =");	PutsGainP(soar[1], sum);
	DrawText(3, 10, "Straight  =");	PutsGainP(soar[0], sum);
	DrawText(3, 12, "H Stop    =");	PutsGainP(soar[2], sum);
	DrawText(3, 14, "Total     =");	PutsGainP(sum, 0);
}
void UpdateTally6s(){
	const Tally* tl = GetTallyData();
	DrawTextUL(1,  3, "Soaring statistics summary:");
	s32 sum[3] = {0}, i;
	for(i = 0 ; i < 4 ; ++i){
		sum[0] += tl->soaring_cnt[i];
		sum[1] += tl->sinking_cnt[i];
		sum[2] += tl->keeping_cnt[i];
	}
	s32 sum2 = sum[0] + sum[1] + sum[2];
	DrawText(3,  6, "Lift    =");	PutsPercent(sum[0], sum2);
	DrawText(3,  8, "Lv.keep =");	PutsPercent(sum[2], sum2);
	DrawText(3, 10, "Sink    =");	PutsPercent(sum[1], sum2);

	Putsf("%2.13m(Sink < -%.3fm/s",  tl->keep_range);
	Putsf("%10.15m+%.3fm/s < Lift)", tl->keep_range);
}
void UpdateTally6a1(){
	const Tally* tl = GetTallyData();
	DrawTextUL(1,  3, "Soaring statistics: (Lift)");
	UpdateTally6x1(tl->soaring_cnt);
}
void UpdateTally6a2(){
	const Tally* tl = GetTallyData();
	DrawTextUL(1,  3, "Soaring statistics: (Gain)");
	UpdateTally6x2(tl->soaring_sum);
}

void UpdateTally6b1(){
	const Tally* tl = GetTallyData();
	DrawTextUL(1,  3, "Soaring statistics: (Sink)");
	UpdateTally6x1(tl->sinking_cnt);
}
void UpdateTally6b2(){
	const Tally* tl = GetTallyData();
	DrawTextUL(1,  3, "Soaring statistics: (Loss)");
	UpdateTally6x2(tl->sinking_sum);
}

void UpdateTally6c1(){
	const Tally* tl = GetTallyData();
	DrawTextUL(1,  3, "Soaring statistics: (LvKeep)");
	UpdateTally6x1(tl->keeping_cnt);
	DrawText(3, 16, "(Level keep: ");
	PutsPoint(tl->keep_range, 0, 3);
	Puts("m/s)");
}

void UpdateTally10(){
	const Tally* tl = GetTallyData();
	DrawTextUL(1,  3, "Soaring statistics: (Vario)");

	s32* v_s = tl->vario_sum;
	s32* v_c = tl->vario_cnt;
	s32 sum = 0, cnt = 0, i;
	for(i = 0 ; i < 4 ; ++i){
		sum += v_s[i];
		cnt += v_c[i];
	}

	DrawText(3,  6, "Centering =");	PutsVario3(v_s[3], v_c[3]);
	DrawText(3,  8, "Turn      =");	PutsVario3(v_s[1], v_c[1]);
	DrawText(3, 10, "Straight  =");	PutsVario3(v_s[0], v_c[0]);
	DrawText(3, 12, "H Stop    =");	PutsVario3(v_s[2], v_c[2]);
	DrawText(3, 14, "Total     =");	PutsVario3(sum, cnt);
}

void UpdateTally11(){
	const Tally* tl = GetTallyData();
	s32* cl = tl->centering[0];
	s32* cr = tl->centering[1];
	DrawTextUL(1,  3, "Centering statistics:");
	DrawText(9,  6, "Left   Right   Total");

	DrawText(1,  8, "Time");

	s32 cll = cl[TALLY_CENTER_COUNT];
	s32 clr = cr[TALLY_CENTER_COUNT];
	s32 cls = cll + clr;
	Putsf("%7S%7S%7S", cll, clr, cls);

	DrawText(1, 10, "Balance");
	if(cls){
		s32 t = BiosDiv(cl[TALLY_CENTER_COUNT] * 100, cls, &t);
		Putsf("%4d%%" "%7d%%" "%7d%%", t, 100 - t, 100);
	} else {
		Puts(" ---%" "    ---%" "    ---%");
	}

	s32 t_sum = cl[TALLY_CENTER_TIMES] + cr[TALLY_CENTER_TIMES];
	DrawText(1, 12, "Count");
	Putsf("%7d%8d%8d", cl[TALLY_CENTER_TIMES], cr[TALLY_CENTER_TIMES], t_sum);

	DrawText(1, 14, "Turn");
	PutsTurn(cl[TALLY_CENTER_TIMES], cl[TALLY_CENTER_TURN], cll);
	PutsTurn(cr[TALLY_CENTER_TIMES], cr[TALLY_CENTER_TURN], clr);
	PutsTurn(t_sum, cl[TALLY_CENTER_TURN] + cr[TALLY_CENTER_TURN], cls);
}

void UpdateTally12(){
	const Tally* tl = GetTallyData();
	s32* cl = tl->centering[0];
	s32* cr = tl->centering[1];
	DrawTextUL(1,  3, "Centering statistics:(Vario)");
	DrawText(9,  6, "Left   Right   Total");

	s32 cll = cl[TALLY_CENTER_COUNT];
	s32 clr = cr[TALLY_CENTER_COUNT];
	s32 cls = cll + clr;

	DrawText(1,  8, "vMax");
	PutsVario(cl[TALLY_CENTER_LIFT_MAX], cll);
	PutsVario(cr[TALLY_CENTER_LIFT_MAX], clr);
	PutsVario(MaxS32(cl[TALLY_CENTER_LIFT_MAX], cr[TALLY_CENTER_LIFT_MAX]), cls);

	DrawText(1, 10, "vMin");
	PutsVario(cl[TALLY_CENTER_SINK_MAX], cll);
	PutsVario(cr[TALLY_CENTER_SINK_MAX], clr);
	PutsVario(MinS32(cl[TALLY_CENTER_SINK_MAX], cr[TALLY_CENTER_SINK_MAX]), cls);

	DrawText(1, 12, "vAvg");
	s32 sum_l = cl[TALLY_CENTER_LIFT_SUM] + cl[TALLY_CENTER_SINK_SUM];
	s32 sum_r = cr[TALLY_CENTER_LIFT_SUM] + cr[TALLY_CENTER_SINK_SUM];
	PutsVario2(sum_l, cl[TALLY_CENTER_COUNT]);
	PutsVario2(sum_r, cr[TALLY_CENTER_COUNT]);
	PutsVario2(sum_l + sum_r, cl[TALLY_CENTER_COUNT] + cr[TALLY_CENTER_COUNT]);

	DrawText(1, 14, "Gain");
	PutsGain(cl[TALLY_CENTER_LIFT_SUM], cll);
	PutsGain(cr[TALLY_CENTER_LIFT_SUM], clr);
	PutsGain(cl[TALLY_CENTER_LIFT_SUM] + cr[TALLY_CENTER_LIFT_SUM], cls);

	DrawText(1, 16, "Loss");
	PutsGain(cl[TALLY_CENTER_SINK_SUM], cll);
	PutsGain(cr[TALLY_CENTER_SINK_SUM], clr);
	PutsGain(cl[TALLY_CENTER_SINK_SUM] + cr[TALLY_CENTER_SINK_SUM], cls);
}

void UpdateTally13(){
	const Tally* tl = GetTallyData();
	s32* cl = tl->centering[0];
	s32* cr = tl->centering[1];
	DrawTextUL(1,  3, "Centering statistics:(Power)");
	DrawText(9,  6, "Left   Right   Total");

	DrawText(1,  8, "Spd.");
	s32 cll = cl[TALLY_CENTER_COUNT];
	s32 clr = cr[TALLY_CENTER_COUNT];
	s32 cls = cll + clr;
	PutsSpd8(cl[TALLY_CENTER_SPEED], cll);
	PutsSpd8(cr[TALLY_CENTER_SPEED], clr);
	PutsSpd8(cl[TALLY_CENTER_SPEED] + cr[TALLY_CENTER_SPEED], cls);

	DrawText(2, 10, "Max");
	PutsSpd8(cl[TALLY_CENTER_SPEED_MAX], cll? 1 : 0);
	PutsSpd8(cr[TALLY_CENTER_SPEED_MAX], clr? 1 : 0);
	PutsSpd8(MaxS32(cl[TALLY_CENTER_SPEED_MAX], cr[TALLY_CENTER_SPEED_MAX]), cls? 1 : 0);

	DrawText(1, 12, "Gr. ");
	PutsPoint9(cl[TALLY_CENTER_G], cll);
	PutsPoint9(cr[TALLY_CENTER_G], clr);
	PutsPoint9(cl[TALLY_CENTER_G] + cr[TALLY_CENTER_G], cls);

	DrawText(2, 14, "Max");
	PutsPoint9(cl[TALLY_CENTER_G_MAX], cll? 1 : 0);
	PutsPoint9(cr[TALLY_CENTER_G_MAX], clr? 1 : 0);
	PutsPoint9(MaxS32(cl[TALLY_CENTER_G_MAX], cr[TALLY_CENTER_G_MAX]), cls? 1 : 0);
}

void UpdateTally14(){
	const Tally* tl = GetTallyData();
	s32* cl = tl->centering[0];
	s32* cr = tl->centering[1];
	DrawTextUL(1,  3, "Centering statistics:(etc)");
	DrawText(9,  6, "Left   Right   Total");

	DrawText(1,  8, "RPM ");
	PutsRPM(cl[TALLY_CENTER_TURN], cl[TALLY_CENTER_COUNT]);
	PutsRPM(cr[TALLY_CENTER_TURN], cr[TALLY_CENTER_COUNT]);
	PutsRPM(cl[TALLY_CENTER_TURN] + cr[TALLY_CENTER_TURN], cl[TALLY_CENTER_COUNT] + cr[TALLY_CENTER_COUNT]);

	DrawText(1, 10, "r.  ");
	PutsRad(cl[TALLY_CENTER_SPEED], cl[TALLY_CENTER_TURN]);
	PutsRad(cr[TALLY_CENTER_SPEED], cr[TALLY_CENTER_TURN]);
	PutsRad(cl[TALLY_CENTER_SPEED] + cr[TALLY_CENTER_SPEED], cl[TALLY_CENTER_TURN] + cr[TALLY_CENTER_TURN]);

	DrawText(1, 12, "Dr/C");
	s32 cll = cl[TALLY_CENTER_COUNT];
	s32 clr = cr[TALLY_CENTER_COUNT];
	s32 ctl = cl[TALLY_CENTER_TIMES];
	s32 ctr = cr[TALLY_CENTER_TIMES];
	PutsSec4(cll, ctl);
	PutsSec4(clr, ctr);
	PutsSec4(cll + clr, ctl + ctr);

	DrawText(1, 14, "Tr/C");
	s32 cxl = cl[TALLY_CENTER_TURN];
	s32 cxr = cr[TALLY_CENTER_TURN];
	PutsTurn2(ctl, cxl);
	PutsTurn2(ctr, cxr);
	PutsTurn2(ctl + ctr, cxl + cxr);
}


#define XP0 SELMENU_BG
#define XP1 SELMENU_GRP
#define XP2 SELMENU_GR2
const u16 PT_TABLE1[] ={XP2,XP0,XP0,XP0,XP0,XP2,XP0,XP0,XP0,XP0,XP2,XP0,XP0,XP0,XP0,XP2, 0};
const u16 PT_TABLE2[] ={XP0,XP2,XP0,XP0,XP0,XP0,XP0,XP0,XP1,XP0,XP0,XP0,XP0,XP0,XP2,XP0, 0};

s32 HistMaxSum(s32* hist, s32 len, s32* sum_ret){
	s32 sum = 0, max = 0;
	while(len--){
		if(hist[len] > max) max = hist[len];
		sum += hist[len];
	}
	if(sum_ret) *sum_ret = sum;
	if(sum) sum = BiosDiv(max * 100, sum, &sum);
	DrawText( 1, 6, "(%)");
	Putsf("%.8m%3d", sum);
	return max;
}

void UpdateTally15(){
	const Tally* tl = GetTallyData();
	DrawTextUL(1,  3, "Alt. histogram:");

	s32* hist = tl->alt_hist;
	s32  max = HistMaxSum(hist, ALT_HIST, 0), i;
	for(i = 0 ; i < ALT_HIST ; ++i){
		DrawGraph(hist[i], 0, max, i + 8, 15, 10, 2);
	}

	s32 s = BiosDiv(MaxS32(ALT_HIST_RANGE_MIN, tl->alt_hist_range), 1000, &s);
	Putsf("%18.4mStep %dm", s);
	Putsf("%4.16m0m%25.16m%4d", s * ALT_HIST);

	const u16* ptt = (s < 40)? PT_TABLE1 : PT_TABLE2;
	for(s = 8 ; *ptt ; ++s, ++ptt) SetChar_Base(MAP_BG1, s, 16, *ptt);

	SetVideoMode(1); // グラフ専用モード
}

void UpdateTally16(){
	const Tally* tl = GetTallyData();
	DrawTextUL(1,  3, "Vario histogram:");

	s32* hist = tl->vario_hist;
	s32  max = HistMaxSum(hist, VARIO_HIST, 0), i, x = 8;
	for(i = 0 ; i < VARIO_HIST ; ++i){
		DrawGraph(hist[i], 0, max, x, 15, 10, 2);
		if(i == VARIO_HIST_H - 1) x += 3;
		else                      x++;
	}

	s32 s = BiosDiv(MaxS32(VARIO_HIST_RANGE_MIN, tl->vario_hist_range), 100, &s);
	Putsf("%18.4mStep %dm/s", s);

	s *= 25;
	Putsf("%2.16m%+2.1f%14P-0%17P+0%25P%+3.1f", -s, s);

	for(s = 0 ; s < 4 ; ++s){
		s32 t = (s * 5) >> 1;
		SetChar_Base(MAP_BG1, 13 - t, 16, (s & 1)? SELMENU_GRP : SELMENU_GR2);
		SetChar_Base(MAP_BG1, 19 + t, 16, (s & 1)? SELMENU_GR2 : SELMENU_GRP);
	}

	SetVideoMode(1); // グラフ専用モード
}

void UpdateTally17(){
	const Tally* tl = GetTallyData();
	DrawTextUL(1,  3, "Speed histogram:");

	s32* hist = tl->speed_hist;
	s32 max = HistMaxSum(hist, SPEED_HIST, 0), i;
	for(i = 0 ; i < SPEED_HIST ; ++i){
		DrawGraph(hist[i], 0, max, i + 8, 15, 10, 2);
	}

	s32 s = BiosDiv(MaxS32(SPEED_HIST_RANGE_MIN, tl->speed_hist_range), 1000, &s);
	Putsf("%18.4mStep%2.1fkm/s", s);
	Putsf("%4.16m0km/s%26P%3d", s * (SPEED_HIST / 10));

	const u16* ptt = (s < 4)? PT_TABLE1 : PT_TABLE2;
	for(s = 8 ; *ptt ; ++s, ++ptt) SetChar_Base(MAP_BG1, s, 16, *ptt);

	SetVideoMode(1); // グラフ専用モード
}

void UpdateTally18(){
	const Tally* tl = GetTallyData();
	DrawTextUL(1,  3, "Direction histogram:");

	s32* hist = tl->angle_hist;
	s32 sum, max = HistMaxSum(hist, ANGLE_HIST, &sum), i, x = 8, j = 0, k = 20;
	s32 merge[4] = {0}, *mp = merge;
	for(i = 0 ; i < ANGLE_HIST ; ++i){
		if(++k >= ANGLE_HIST) k = 0;
		DrawGraph(hist[k], 0, max, x, 15, 10, 2);
		*mp += hist[k];
		if(++j < 6) x++;
		else {
			x += 3;
			j = 0;
			++mp;
		}
	}

	DrawText(21,  5, "Step 15°");
	DrawText( 5, 16, "N   E   S   W");
	if(!sum) sum = 1;
	DrawText(20,  8, "N:");	PutPercent2(merge[0], sum);
	DrawText(20, 10, "E:");	PutPercent2(merge[1], sum);
	DrawText(20, 12, "S:");	PutPercent2(merge[2], sum);
	DrawText(20, 14, "W:");	PutPercent2(merge[3], sum);

	SetVideoMode(1); // グラフ専用モード
}

const TallyProc TALLY_PROC[] = {
	UpdateTally1,
	UpdateTally2,
	UpdateTally3,
	UpdateTally4,
	UpdateTally5,
	UpdateTally6s,
	UpdateTally6a1,
	UpdateTally6c1,
	UpdateTally6b1,
	UpdateTally6a2,
	UpdateTally6b2,
	UpdateTally10,
	UpdateTally11,
	UpdateTally12,
	UpdateTally13,
	UpdateTally14,
	UpdateTally15,
	UpdateTally16,
	UpdateTally17,
	UpdateTally18,
};

u32 MP_Tally(u16 push){
	if(push == 0xffff){
		TallyMenu();
		IW->mp.mp_flag = 0;
		IW->mp.proc = MP_Tally; // コールバックを変える
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 1;
	} else if(push & KEY_LOG_NEXT){
		PlaySG1(SG1_NEXT);
		if(++IW->mp.mp_flag >= ARRAY_SIZE(TALLY_PROC)) IW->mp.mp_flag = 0;
		TallyMenu();
	} else if(push & KEY_LOG_PREV){
		PlaySG1(SG1_PREV);
		if(!IW->mp.mp_flag--) IW->mp.mp_flag = ARRAY_SIZE(TALLY_PROC) - 1;
		TallyMenu();
	} else if(push & KEY_START){
		if(IsCurFlog()){
			PlaySG1(SG1_CLEAR);
			InitTally();
			IW->px.gstate = 0;
			TallyMenu();
		}
//	} else if(IW->mp.pvt_check == IW->px.counter){
//		return 0;
	}
	Putsf("%5m< Flight log %d/20 >", IW->mp.mp_flag + 1);
	IW->mp.pvt_check = IW->px.counter;
	(*TALLY_PROC[IW->mp.mp_flag])();

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// フライトログ保存
///////////////////////////////////////////////////////////////////////////////
u32 CalcCheckSum32(u32* start, u32* end){
	u32 sum = 0;
	while(start < end) sum += *start++; // word sum
	return sum;
}

u32 SaveFLog(u32 pos, void* addr, u32 size, u32 msg_flag){
	if(pos == -1){ // 自動セーブ検索
		u32 min = -1, i = 4;
		while(i--){
			u32* p = GetFLogHead(i); // キャッシュを使う
			if(!p){ // ログデータなし
				min = 0;
				pos = i; // 最小値
			} else { // 古いデータあり
				if(p[1] <= min){ // 一番古いものなら更新
					min = p[1];
					pos = i;
				}
			}
		}
	}

	if(pos > 3 || size > FLOG_SIZE - 4){ // sum
		if(msg_flag){
			MenuFillBox(3, 4, 25, 9);
			DrawText(5, 6, "<INTERNAL ERROR>");
		}
		return -1;
	}

	u32* addr32 = (u32*)addr; // 必ず4byte境界
	if(!addr32[1]){
		if(msg_flag){
			MenuFillBox(2, 4, 27, 9);
			DrawTextCenter(6, "まだ,ログがありません!");
		}
		return -2;
	}
	if(msg_flag){
		MenuFillBox(3, 4, 25, 9);
		DrawText(5, 6, "セーブしています...");
	}

	// 音声DMA停止
	VoiceStop();
	EnableSound(SOUND_CH_VARIO, -1); // バリオ音は止めておく

	// Voiceエリアに書き込みデータを構築
	u32 offset = FI_FLOG_OFFSET;
	u32 dst    = VOICE_ADDRESS;
	if(CART_IS_SECTWRITE()){
		// 変更ログ部分のみの書き換えがOK
		DmaClear(3, 0, VOICE_ADDRESS, FLOG_SIZE, 32); // まず0クリア
		offset += FLOG_SIZE * pos; // 書き込み先アドレスをシフト
	} else {
		// 8KBブロック全体が消去されるため、部分書き換え処理が必要
		IW->cif->ReadData(FI_FLOG_OFFSET, (void*)VOICE_ADDRESS, FLOG_TOTAL, 0);
		dst += FLOG_SIZE * pos; // 書き込み元アドレスをシフト
	}

	// RAM上でチェックサム作成
	DmaCopy (3, addr, dst, size, 32); // ログをWRAMにコピー(CheckSum書き込み用)
	u32* end  = (u32*)(dst + FLOG_SIZE - sizeof(u32));
	*end = -CalcCheckSum32((u32*)dst, end);// チェックサムを保存

	// カートリッジ書き込み
	s32 ret = IW->cif->WriteData(offset, (void*)VOICE_ADDRESS, CART_IS_SECTWRITE()? FLOG_SIZE : FLOG_TOTAL);
	if(msg_flag){
		PlaySG1(ret? SG1_CANCEL : SG1_CHANGE);
		PutFlashError(ret);
	}

	// キャッシュを更新
	u32* cache = IW->mp.flog_cache[pos];
	cache[0] = addr32[0];
	cache[1] = addr32[1];

	// 次の自動セーブのエリアを記憶
	switch(*addr32){
	case FLOG_MAGIC_FLIGHT:
		IW->mp.flog_presave = pos;
		if(IW->mp.tlog_presave == pos) IW->mp.tlog_presave = -1;
		break;
	case FLOG_MAGIC_TASK:
		IW->mp.tlog_presave = pos;
		if(IW->mp.flog_presave == pos) IW->mp.flog_presave = -1;
		break;
	}

	return ret;
}

u32 MP_Task(u16 push);
u32 MP_TallySave(u16 push){
	SaveFLog(IW->mp.sel, &IW->mp.tally, sizeof(Tally), 1);
	return SetKeyWaitCB(0xffff);
}
u32 MP_TaskSave(u16 push){
	SaveFLog(IW->mp.sel, &IW->task_log, sizeof(TaskLogData), 1);
	return SetKeyWaitCB(0xffff);
}
u32 MP_TallyCur(u16 push){
	IW->mp.flog_addr = 0;
	return MP_Tally(0xffff);
}
u32 MP_TaskCur(u16 push){
	IW->mp.flog_addr = 0;
	return MP_Task(0xffff);
}
u32 MP_LogView(u16 push){
	if(push == 0xffff){
		// マジック確認
		if(!GetFLogHead(IW->mp.sel)){
			// 無効データ
			PlaySG1(SG1_CANCEL);
			MenuFillBox(1, 4, 27, 9);
			DrawText(3, 6, "ログデータがありません!");
			return SetKeyWaitCB(push);
		}
		// チェックサム確認
		if(IW->cif->ReadData(FI_FLOG_OFFSET + IW->mp.sel * FLOG_SIZE, FLOG_PTR, FLOG_SIZE, 0)){
			// 無効データ
			PlaySG1(SG1_CANCEL);
			MenuFillBox(1, 4, 27, 9);
			DrawText(3, 6, "ログデータよみこみエラー");
			return SetKeyWaitCB(push);
		}
		IW->mp.flog_addr = FLOG_PTR; // ログ表示ポインタを切り替え
		if(CalcCheckSum32(IW->mp.flog_addr, IW->mp.flog_addr + (FLOG_SIZE >> 2))){
			MenuFillBox(3, 2, 27, 12);
			DrawText(5, 4, "<チェックサム エラー>");
			DrawText(5, 7, "Aボタン: データをみる");
			DrawText(5, 9, "Bボタン: キャンセル");
			return 0;
		}
	}
	if(push & KEY_A){ // チェックサムOK時は直接ここに入る
		PlaySG1(SG1_OK);
		switch(*IW->mp.flog_addr){
		case FLOG_MAGIC_FLIGHT:	return MP_Tally(0xffff);
		case FLOG_MAGIC_TASK:	return MP_Task (0xffff);
		}
		return 1;
	}
	if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// グラフ
///////////////////////////////////////////////////////////////////////////////
#define GRAPH_RIGHT 59
#define GRAPH_COUNT 50
#define SCALE_LIMIT (MAX_LOCUS / GRAPH_COUNT)

void InitGraphScale(int f){
	if(f){
		IW->mp.min_alt = IW->mp.tally.min_alt;
		IW->mp.max_alt = IW->mp.tally.max_alt;
		IW->mp.graph_range = MaxS32(IW->mp.tally.max_up, -IW->mp.tally.min_up);
	} else {
		IW->mp.min_alt =  9999999;
		IW->mp.max_alt = -9999999;
		IW->mp.graph_range = 0;
	}

	s32 index = LOCUS->index, i;
	if(IW->mp.graph_scale > 1){
		index = BiosDiv(index, IW->mp.graph_scale, &i) * IW->mp.graph_scale;
	}
	for(i = 0 ; i < GRAPH_COUNT ; ++i){
		LocusVal* lv = &LOCUS->val[index];
		if((index -= IW->mp.graph_scale) < 0) index += MAX_LOCUS;
		if(*(u32*)&lv->lat_d == INVALID_LAT) break;
		s32 alt = (lv->alt_x >> 1) * 1000;
		CHANGE_MAX(IW->mp.max_alt, alt);
		CHANGE_MIN(IW->mp.min_alt, alt);
		CHANGE_MAX(IW->mp.graph_range, myAbs(lv->up));
	}
}

#define GRAPH_MODE 3
const u32 GRAPH_POS[GRAPH_MODE][10] = {
	{ 4, 1, 7,  14,11,17,   9, 10, 14, 5}, // Alt+Vario
	{ 9, 1,17,  20,20,20,  19, 20,  0, 0}, // Alt
	{20,20,20,   9, 1,17,   0,  0,  9,10}, // Vario
};

u32 MP_Graph(u16 push){
	if(push == 0xffff){
		if(IW->mp.graph_scale < 1) IW->mp.graph_scale = 1;
		MenuCls(SELMENU_BG);
		InitGraphScale(IW->mp.graph_minmax);
		SetVideoMode(1); // グラフ専用モード
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		SetVideoMode(0); // モード復帰
		return 1;
	} else if(push & KEY_A){
		PlaySG1(SG1_OK);
		InitGraphScale(IW->mp.graph_minmax ^= 1);
	} else if(push & KEY_LEFT){
		PlaySG1(SG1_PREV);
		if(IW->mp.graph_scale > 1) IW->mp.graph_scale--;
	} else if(push & KEY_RIGHT){
		PlaySG1(SG1_NEXT);
		if(IW->mp.graph_scale < SCALE_LIMIT) IW->mp.graph_scale++;
	} else if(push & KEY_L){
		PlaySG1(SG1_PREV);
		if(IW->mp.graph_scale > 1) IW->mp.graph_scale >>= 1;
	} else if(push & KEY_R){
		PlaySG1(SG1_NEXT);
		if((IW->mp.graph_scale <<= 1) > SCALE_LIMIT) IW->mp.graph_scale = SCALE_LIMIT;
	} else if(push & KEY_UP){
		PlaySG1(SG1_CHANGE);
		if(IW->mp.graph_mode++ >= GRAPH_MODE - 1) IW->mp.graph_mode = 0;
		MenuCls(SELMENU_BG);
	} else if(push & KEY_DOWN){
		PlaySG1(SG1_CHANGE);
		if(!IW->mp.graph_mode--) IW->mp.graph_mode = GRAPH_MODE - 1;
		MenuCls(SELMENU_BG);
	} else if(IW->mp.pvt_check == IW->px.counter){
		return 0;
	}
	IW->mp.pvt_check = IW->px.counter;

	// 2%マージン付きレンジ設定
	s32 vario_range = IW->mp.graph_range;
	if(vario_range < 980) vario_range = 1000;
	else if(vario_range < 1960) vario_range = 2000;
	else if(vario_range < 4900) vario_range = 5000;
	else vario_range = 10000;

	s32 index = LOCUS->index, i;
	if(IW->mp.graph_scale > 1){
		index = BiosDiv(index, IW->mp.graph_scale, &i) * IW->mp.graph_scale;
	}
	s32 min_alt, max_alt;
	if(IW->mp.min_alt > IW->mp.max_alt){
		min_alt = 0;
		max_alt = 100;
	} else {
		min_alt = BiosDiv(IW->mp.min_alt         , 100000, &i) * 100;
		max_alt = BiosDiv(IW->mp.max_alt + 100000, 100000, &i) * 100;
	}
	const u32* gp = GRAPH_POS[IW->mp.graph_mode];

	DrawText(1, gp[0], "Alt.");
	Putsf("%M%4dm", 0, gp[1], max_alt);
	Putsf("%M%4dm", 0, gp[2], min_alt);
	if(!IW->mp.graph_mode) FillTile(MAP_BG0, 0, 9, 4,  9, SELMENU_UDL | (1 << 11));

	DrawText(0, gp[3], "Vario");
	i = BiosDiv(vario_range, 100, &i);
	Putsf("%M%+3.1f", 0, gp[4],  i);
	Putsf("%M%+3.1f", 0, gp[5], -i);

	for(i = 0 ; i < GRAPH_COUNT ; ++i){
		LocusVal* lv = &LOCUS->val[index];
		if((index -= IW->mp.graph_scale) < 0) index += MAX_LOCUS;
		s32 alt = lv->alt_x >> 1;
		s32 up  = lv->up;
		if(*(u32*)&lv->lat_d == INVALID_LAT){
			alt = up = 0;
		}
		if(gp[6]){
			DrawGraph(alt,min_alt, max_alt, GRAPH_RIGHT - i, gp[6], gp[7], 2);
		}
		if(gp[8]){
			DrawGraph((up > 0)?  up : 0, 0, vario_range, GRAPH_RIGHT - i, gp[8],     gp[9], 1);
			DrawGraph((up < 0)? -up : 0, 0, vario_range, GRAPH_RIGHT - i, gp[8] + 1, gp[9], 0);
		}
	}

	if(push) IW->mp.mp_flag = 8;
	else if(IW->mp.mp_flag) IW->mp.mp_flag--;

	if(IW->mp.mp_flag){
		FillTile(1, 5, 18, 14, 19, SELMENU_BK3);
		DrawText(5, 18, (IW->mp.graph_scale < 10)? " x" : "x");
		Putsf("%d", IW->mp.graph_scale);
		i = RoundDiv(IW->tc.locus_smp * IW->mp.graph_scale * GRAPH_COUNT, 60);
		if(i < 100){
			Putsf("%3dmin ", i);
		} else {
			i = BiosDiv(i, 60, &i);
			if(i > 99) i = 99;
			Putsf("%3dhour", i);
		}
	}
	return 0;
}
	
///////////////////////////////////////////////////////////////////////////////
// タスクログ
///////////////////////////////////////////////////////////////////////////////
u32 MP_Task(u16 push){
	if(push == 0xffff){
		IW->mp.mp_flag = 0;
		IW->mp.proc = MP_Task; // コールバックを変える
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 1;
	} else if(push & KEY_LOG_NEXT){
		PlaySG1(SG1_NEXT);
		++IW->mp.mp_flag;
	} else if(push & KEY_LOG_PREV){
		PlaySG1(SG1_PREV);
		--IW->mp.mp_flag;
	} else if(push & KEY_START){
		if(IsCurFlog()){
			PlaySG1(SG1_CLEAR);
			IW->px.gstate = 0;
			InitTaskLog(0);
			if(IW->px.gstate) UpdateTaskLog(0); // テイクオフ済は、今をテイクオフに更新
		}
	} else {
		return 0;
	}

	// ページ送りチェック
	const TaskLogData* tld = GetTaskData();
//	const Route*       rt  = &IW->route_input;
	if(IW->mp.mp_flag  == -1)               IW->mp.mp_flag = tld->rt_count;
	else if(IW->mp.mp_flag > tld->rt_count) IW->mp.mp_flag = 0;
	const TaskLog1* tl1  = &tld->tl1[IW->mp.mp_flag];		// 必ずある
	const TaskLog2* tl2  = GetTL2Addr((TaskLogData*)tld, IW->mp.mp_flag); // ルートサイズによっては無い場合もある

	// 全ページの共通内容を出力
	MenuCls(SELMENU_BG);
	u32 dtime_val = IsCurFlog()? 0 : tld->log_time;// 最下段の日付表示用
	if(!dtime_val && (u32)IW->mp.pre_route == -1){
		// もしまだタスクログに記録がなければ、タスクログを自動で更新する
		if(tld->update_mark & 2){ // 変更あり? (takeOffは除く)
			dtime_val = -1; //更新メッセージ
		} else {
			InitTaskLog(1); // 自動更新
		}
	}
	// タスク数
	Putsf("%5m< Task log %d/%d >", IW->mp.mp_flag, tld->rt_count);

	// 到着/テイクオフ高度
	DrawText(2, 5, IW->mp.mp_flag? "Arrival Alt. = " : "Takeoff Alt. =");
	if(tl1->lat) Putsf("%8dm", tl1->alt);
	else         Puts(" -------m");
	// 到着/テイクオフ時間
	DrawText(2, 7, IW->mp.mp_flag? "Arrival time =  " : "Takeoff time = ");
	Putsf("%t", tl1->dtime);

	// ページ別の表示
	if(IW->mp.mp_flag){
		// ウェイポイント情報の表示
		Locate(0, 3);
		if(tl2){ // ウェイポイント情報あり
			PutsName(tl2->wpt_name);
			Putsf("/Alt.%dm", tl2->wpt_alt);
		} else { // ウェイポイント情報なし
			Putsf("Pylon#%d", IW->mp.mp_flag);
		}
		Puts(" Cyl.");
		PutsDistance2(tl1->cyl);

		// フライト距離
		if(tl1->lat){
			Putsf("%2.9mTrip meter   =%9dm", tl1->trip);

			const TaskLog1* tl1p = &tld->tl1[IW->mp.mp_flag - 1];
			s32 t = tl1->dtime - tl1p->dtime;
//			if(tl1p->seq + 1 == tl->seq){ // 厳密すぎる
			if(IW->mp.mp_flag == 1 && (tld->pre_pylonX & 0x80)){
				// テイクオフ
				DrawTextCenter(13, "- Takeoff Pylon -");
				PutClearMessage(0);
			} else if(tl1p->lat && (t = tl1->dtime - tl1p->dtime) >= 0){
				s32 len;
				CalcDist(tl1->lat, tl1->lon, tl1p->lat, tl1p->lon, &len, 0);
				Putsf("%2.11mSection dist.=%9dm", len);
				Putsf("%2.13mSection time =  %8S", t);
				if(t){
					DrawText(2, 15, "Section spd. =");
					if(IW->tc.spd_unit) Putsf("%4.1fkm/h", BiosDiv(len * 36, t, &len));
					else				Putsf("%5.1fm/s", BiosDiv(len * 10, t, &len));
				}

				// スタートパイロンからのトータル時間を表示
				DrawText(2, 17, "Total time   =");
				u32 ppy = tld->pre_pylonX & 0x7f;
				t = IW->mp.mp_flag - ppy;
				if(t < 1){
					Puts("(Pre-pylon)");
				} else if(t == 1 && !tld->start_time){
					Puts("(Start-pylon)");
				} else {
					tl1p = &tld->tl1[ppy + 1];
					if(!tl1p->lat || (t = tl1->dtime - tl1p->dtime) < 0) Puts("(Invalid)");
					else {
						if(tld->start_time){
							s32 t2;
							BiosDiv(tl1p->dtime, 86400, &t2);
							t2 -= tld->start_time * 60;
							if(t2 < 0)	Puts("(StartErr)");
							else		Putsf("  %8S", t + t2);
						} else {
							Putsf("  %8S", t);
						}
					}
				}
			} else {
				DrawTextCenter(14, "^^^ INVALID DATA ^^^");
				PutClearMessage(dtime_val);
			}
		} else {
			DrawTextCenter(12, "^^^ NOT YET ARRIVED ^^^");
			PutClearMessage(dtime_val);
		}
	} else {
		// 表紙
		DrawText(0, 3, "Takeoff:");
		DrawText(0, 10, "Route information:");
		DrawText(2, 12, "Route    = ");
		PutsNameB(tld->rt_name);
		DrawText(2, 14, "Distance = ");
		Putsf("%dm", tld->rt_dist);
		PutClearMessage(dtime_val);
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// ウェイポイント追加/追加削除
///////////////////////////////////////////////////////////////////////////////
enum {
	WPT_ADD_SUCCESS = 0,
	WPT_ADD_MAX,
	WPT_ADD_SAMENAME,
	WPT_ADD_BADNAME,
	WPT_ADD_CANCEL,

	WPT_DEL_SUCCESS = 0,
	WPT_DEL_USE,
	WPT_DEL_ERROR,

	ROUTE_ADD_SUCCESS = 0,
	ROUTE_ADD_MAX,
	ROUTE_ADD_SAMENAME,
	ROUTE_ADD_BADNAME,
	ROUTE_ADD_EMPTY,
	ROUTE_ADD_CANCEL,

	ROUTE_DEL_SUCCESS = 0,
	ROUTE_DEL_ERROR,
};

u32 WptAddCheck(WptInput* wi){
	if(!wi->name[0] && !wi->alt && !wi->lat && !wi->lon) return WPT_ADD_CANCEL;
	if(WPT->wpt_count == MAX_WPT) return WPT_ADD_MAX;
	if(!wi->name[0]) return WPT_ADD_BADNAME;
	u32 i;
	for(i = 0 ; i < WPT->wpt_count ; ++i){
		if(memcmp(WPT->wpt[i].name, wi->name, WPT_NAMESIZE) == 0) return WPT_ADD_SAMENAME;
	}
	memcpy(WPT->wpt[i].name, wi->name, WPT_NAMESIZE);
	WPT->wpt[i].alt = wi->alt;
	WPT->wpt[i].lat = wi->lat;
	WPT->wpt[i].lon = wi->lon;
	++WPT->wpt_count;
	memset(wi, 0, sizeof(WptInput));

	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ルート/WPT変更フラグをセット
	IW->wpt_sort_type = GetSortType(); // 再ソートのためのリセット
	return WPT_ADD_SUCCESS;
}

u32 WptDelCheck(u32 id){
	// 念のためチェック
	if(id >= WPT->wpt_count) return WPT_DEL_ERROR;

	// 選択シフト
	if(WPT->wpt_count > 2){
		u16* sort = IW->wpt_sort;
		u32 t = IW->mp.wpt_sel + 1; // デフォルトは次の値
		t = sort[(t >= WPT->wpt_count)? 0 : t]; // 最後の場合は先頭の値
		sort[0] = (t <= id)? t : (t - 1); // 0に直接次回の値を入れる
		IW->mp.wpt_sel = 0; // sort[0]を指す
	}
	IW->wpt_sort_type = GetSortType(); // 再ソートのためのリセット

	// ウェイポイント削除
	u32 i = id, j;
	for(--WPT->wpt_count ; i < WPT->wpt_count ; ++i){
		WPT->wpt[i] = WPT->wpt[i + 1];
	}

	// ルート全探索でIndex再計算(既にWptIsUsedでチェック済であることを前提にしている…)
	for(i = 0 ; i < ROUTE->route_count ; ++i){
		Route* rt = &ROUTE->route[i];
		for(j = 0 ; j < rt->count ; ++j) if(rt->py[j].wpt > id) --rt->py[j].wpt;
	}

	// ランディグもシフトしておく
	if(id < WPT->def_ld) WPT->def_ld--;

	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ルート/WPT変更フラグをセット
	return WPT_DEL_SUCCESS;
}

// ウェイポイントがルートに含まれているかチェック(ルート全探索)
s32 WptIsUsed(u32 id, u32 begin){
	for(; begin < ROUTE->route_count ; ++begin){
		u32 j;
		Route* rt = &ROUTE->route[begin];
		for(j = 0 ; j < rt->count ; ++j) if(rt->py[j].wpt == id) return begin; // ルートで使用中
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////
// ウェイポイントを手動で追加
///////////////////////////////////////////////////////////////////////////////
u32 MP_AddWptCancel(u16 push){
	IW->mp.menuid = MENU_ID_WAYPOINT;
	IW->mp.sel = 0;
	IW->mp.proc = 0;
	DispMenu(IW->mp.menuid);
	memset(&IW->wpt_input, 0, sizeof(WptInput)); // ウェイポイントを初期化しておく
	return 1;
}

u32 MP_AddWpt(u16 push){
	if(push != 0xffff) return push & (KEY_A | KEY_B);
	switch(WptAddCheck(&IW->wpt_input)){
	case WPT_ADD_MAX:
		PlaySG1(SG1_CANCEL);
		MenuFillBox(0, 4, 29, 9);
		DrawText(2, 6, "WPTは さいだい1000けんです");
		return 0;
	case WPT_ADD_SAMENAME:
		return PutSameNameMessage("WPT");
	case WPT_ADD_BADNAME:
		return PutBadNameMessage();
	case WPT_ADD_CANCEL:
		return MP_AddWptCancel(0xffff);
	}
	IW->mp.menuid = MENU_ID_WAYPOINT;
	IW->mp.sel = 0;
	PlaySG1(SG1_CHANGE);
	MenuFillBox(3, 4, 25, 9);
	DrawText(5, 6, "WPTをついかしました");
	return 0;
}


#define NO_SCROLL 7

void PutWptMenu(u32 id, u32 y){
	id = IW->wpt_sort[id];
	Putsf("%M#%03d:", 1, y, id);
	PutsNameB(WPT->wpt[id].name);

	s32 t = WPT->wpt[id].alt;
	if(t < -999) t = -999;
	else if(t > 9999) t = 9999;
	Putsf(" Alt.%4d%s", t, (id == WPT->def_ld)? "m LD" : "m   ");
}

u32 MP_ChangeWpt(u16 push);
u32 MP_DelWpt(u16 push);
u32 MP_CopyWpt(u16 push);
u32 MP_LDWpt(u16 push);
u32 MP_RtWptInput(u16 push);

#include <stdlib.h>
#define MAX_SORT_TYPE 5

int comp_lat(const void* lhs, const void* rhs){
	return WPT->wpt[*(u16*)lhs].lat - WPT->wpt[*(u16*)rhs].lat;
}
int comp_lon(const void* lhs, const void* rhs){
	return WPT->wpt[*(u16*)lhs].lon - WPT->wpt[*(u16*)rhs].lon;
}
int comp_alt(const void* lhs, const void* rhs){
	return WPT->wpt[*(u16*)lhs].alt - WPT->wpt[*(u16*)rhs].alt;
}
int comp_name(const void* lhs, const void* rhs){
	return strcmp(WPT->wpt[*(u16*)lhs].name, WPT->wpt[*(u16*)rhs].name);
}
typedef int (*qsort_comp)(const void*, const void*);
typedef struct {
	qsort_comp comp;
	const u8* name;
} SortTable;
const SortTable WPT_COMP[] = {
	{0,			"なまえソート   "},
	{comp_name,	"こうどソート   "},
	{comp_alt,	"いどソート     "},
	{comp_lat,	"けいどソート   "},
	{comp_lon,	"とうろくじゅん "},
};

const u32 HILIGHT_POS[5][2] = {
	{1,5}, {6,16}, {17,26}, {0,13}, {16,29}
};
const u32 HILIGHT_NOP[2] = {
	0,0
};

u32 SortWpt(){
	const SortTable* st = &WPT_COMP[GetSortType()];
	if(!GetSortMark()){
		PlaySG1(SG1_OK);
		IW->wpt_sort_type |= SORTED_MARK;
		DrawText(0, 18, "Sorting... ");

		// 数字を詰める
		u16* sort = IW->wpt_sort;
		u32 backup = sort[IW->mp.wpt_sel];
		u32 i;
		for(i = 0 ; i < WPT->wpt_count ; ++i) sort[i] = i; // オーダ順

		// ソート
		if(st->comp) qsort(sort, WPT->wpt_count, sizeof(u16), st->comp); // 1000ポイントでも1秒以内に完了

		// もとのselectを検索
		for(i = 0 ; sort[i] != backup && i < WPT->wpt_count - 1 ; ++i);
		IW->mp.wpt_sel = i;
	}
	FillTile(MAP_BG1, 0, 18, 29, 19, SELMENU_BK2);

	// アイテム表示
	u32 sort_type = GetSortType();
	const u32* hpos = (sort_type < 3)? HILIGHT_POS[sort_type] : HILIGHT_NOP;
	if(WPT->wpt_count > NO_SCROLL){
		DrawBox(0, 2, 29, 17);
		MoveSelBg2(3, hpos[0], hpos[1], SELMENU_0, SELMENU_BK3); // カーソルは中心固定
	} else {
		DrawBox(0, 2, 29, WPT->wpt_count * 2 + 3);
		u32 i;
		for(i = 0 ; i < WPT->wpt_count ; ++i) PutWptMenu(i, i * 2 + 3);
	}
	if(WPT->wpt_count < 2) return 0;
	DrawText(0, 18, "Selectボタン \\ ");
	Puts(st->name);
	return 1;
}

u32 SelWpt(u16 push){
	u32 sort_flag = 0;
	if(push == 0xffff){
		// 初期のWPT有無チェック
		IW->mp.mp_flag = 0;
		if(!WPT->wpt_count){
			PlaySG1(SG1_CANCEL);
			MenuFillBox(5, 4, 25, 9);
			DrawText(7, 6, "WPTが ありません!");
			return SetKeyWaitCB(0xffff);
		}

		// メニュー作成
		MenuCls(SELMENU_BK2);
		if(     IW->mp.proc == MP_ChangeWpt)	Puts("どのWPTを へんこう しますか?");
		else if(IW->mp.proc == MP_DelWpt)		Puts("どのWPTを さくじょ しますか?");
		else if(IW->mp.proc == MP_CopyWpt)		Puts("どのWPTを コピー しますか?");
		else if(IW->mp.proc == MP_LDWpt)		Puts("ランディングをえらんでください");
		else if(IW->mp.proc == MP_RtWptInput)	Puts("どのWPTを ついか しますか?");
		if(IW->mp.wpt_sel >= WPT->wpt_count) IW->mp.wpt_sel = WPT->wpt_count - 1;
		sort_flag = SortWpt();
	} else if(push & KEY_B){
		// 選択キャンセル
		PlaySG1(SG1_CANCEL);
		return 2;
	}
	else if(push & KEY_DOWN)	RangeAdd(&IW->mp.wpt_sel,   1, WPT->wpt_count);
	else if(push & KEY_UP)		RangeAdd(&IW->mp.wpt_sel,  -1, WPT->wpt_count);
	else if(push & KEY_RIGHT)	RangeAdd(&IW->mp.wpt_sel,   7, WPT->wpt_count);
	else if(push & KEY_LEFT)	RangeAdd(&IW->mp.wpt_sel,  -7, WPT->wpt_count);
	else if(push & KEY_R)		RangeAdd(&IW->mp.wpt_sel,  50, WPT->wpt_count);
	else if(push & KEY_L)		RangeAdd(&IW->mp.wpt_sel, -50, WPT->wpt_count);
	else if(push & KEY_SELECT){
		if((IW->wpt_sort_type = GetSortType() + 1) >= MAX_SORT_TYPE) IW->wpt_sort_type = 0;
		sort_flag = SortWpt();
	}
	else {
#define BLANK_INTERVAL	0x0010
		if(WPT->wpt_count > NO_SCROLL) DispMenuArrow(1, 1);
		return 0;
	}

	// スクロール表示
	u32 sort_type = GetSortType();
	PlaySG1(SG1_SELECT);
	if(WPT->wpt_count > NO_SCROLL){
		MoveObj(OBJ_MENU_CURSOR, 1, 3 * 16 + 27);
		s32 i, id = IW->mp.wpt_sel - 3;
		if(id < 0) id += WPT->wpt_count;
		for(i = 0 ; i < NO_SCROLL ; ++i, ++id){
			if(id >= WPT->wpt_count) id -= WPT->wpt_count;
			PutWptMenu(id, i * 2 + 3);
		}
	} else {
		u16 y = IW->mp.wpt_sel * 16;
		MoveObj(OBJ_MENU_CURSOR, 1, y + 27);
		const u32* hpos = (sort_type < 3)? HILIGHT_POS[sort_type] : HILIGHT_NOP;
		MoveSelBg2(IW->mp.wpt_sel, hpos[0], hpos[1], SELMENU_0, SELMENU_BK3);
	}
	if(!sort_flag){
		if(sort_type >= 3){
			const u32* hpos = HILIGHT_POS[sort_type];
			FillTile(MAP_BG1, hpos[0], 18, hpos[1], 19, SELMENU_BK3);
		}
		Locate(0, 18);
		Wpt* sel = &WPT->wpt[IW->wpt_sort[IW->mp.wpt_sel]];
		PutsLat(sel->lat);
		PutsSpace(2);
		PutsLon(sel->lon);
	}
	return 0;
}

u32 MP_CopyWpt(u16 push){
	if(push == 0xffff || !(push & KEY_A)) return SelWpt(push);
	if(IW->mp.wpt_sel == -1) return 2;

	PlaySG1(SG1_OK);
	Wpt* sel = &WPT->wpt[IW->wpt_sort[IW->mp.wpt_sel]];
	IW->wpt_input.lat = sel->lat;
	IW->wpt_input.lon = sel->lon;
	IW->wpt_input.alt = sel->alt;
	IW->wpt_input.name[0] = 0;

	// 新規追加へジャンプ
	IW->mp.menuid = MENU_ID_NEW_WPT;
	IW->mp.sel = 0;
	return 1;
}

u32 MP_LDWpt(u16 push){
	if(push == 0xffff || !(push & KEY_A)) return SelWpt(push);
	if(IW->mp.wpt_sel == -1) return 2;

	PlaySG1(SG1_OK);
	WPT->def_ld = IW->wpt_sort[IW->mp.wpt_sel];
	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ルート/WPT変更フラグをセット
	return 1;
}

u32 MP_ChangeWpt(u16 push){
	if(push == 0xffff || !(push & KEY_A)) return SelWpt(push);
	if(IW->mp.wpt_sel == -1) return 2;

	// コピーを作成
	Wpt* sel = &WPT->wpt[IW->wpt_sort[IW->mp.wpt_sel]];
	PlaySG1(SG1_OK);
	IW->wpt_input.lat = sel->lat;
	IW->wpt_input.lon = sel->lon;
	IW->wpt_input.alt = sel->alt;
	memcpy(IW->wpt_input.name, sel->name, WPT_NAMESIZE);

	// 新規追加へジャンプ
	IW->mp.menuid = MENU_ID_CHANGE_WPT;
	IW->mp.sel = 0;
	return 1;
}
u32 MP_ChangeWptCancel(u16 push){
	// wpt_inputを初期化しておく
	IW->wpt_input.lat = 0;
	IW->wpt_input.lon = 0;
	IW->wpt_input.alt = 0;
	memset(IW->wpt_input.name, 0, WPT_NAMESIZE);

	IW->mp.menuid = MENU_ID_WAYPOINT;
	IW->mp.proc = MP_ChangeWpt;
	IW->mp.sel = 1; // 変更の位置!
	MP_ChangeWpt(0xffff);
	return 0; // 0で良い
}
u32 MP_ChangeWptSubmit(u16 push){
	if(push == 0xffff){
		if(!*IW->wpt_input.name) return PutBadNameMessage();
		if(IW->mp.wpt_sel < WPT->wpt_count){
			u32 i, sel_id = IW->wpt_sort[IW->mp.wpt_sel];
			for(i = 0 ; i < WPT->wpt_count ; ++i){
				if(i == sel_id) continue;
				if(!memcmp(WPT->wpt[i].name, IW->wpt_input.name, WPT_NAMESIZE)){
					return PutSameNameMessage("WPT");
				}
			}
			Wpt* sel = &WPT->wpt[sel_id];
			if( sel->lat != IW->wpt_input.lat ||
				sel->lon != IW->wpt_input.lon ||
				sel->alt != IW->wpt_input.alt ||
				memcmp(sel->name, IW->wpt_input.name, WPT_NAMESIZE)){
				// 変更したときだけ
				sel->lat = IW->wpt_input.lat;
				sel->lon = IW->wpt_input.lon;
				sel->alt = IW->wpt_input.alt;
				memcpy(sel->name, IW->wpt_input.name, WPT_NAMESIZE);
				IW->wpt_sort_type = GetSortType(); // 再ソートのためのリセット
				i = -1;
				while((i = WptIsUsed(sel_id, i + 1)) != -1) CalcRouteDistance(i);
				IW->mp.navi_update |= NAVI_UPDATE_WPT;
				IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ルート/WPT変更フラグをセット
			}
		} else {
			return 1;
		}
		return MP_ChangeWptCancel(push);
	}
	return push & (KEY_A | KEY_B); // 復帰ボタンチェック
}

u32 MP_ClearWpt(u16 push){
	if(push == 0xffff){
		PlaySG1(SG1_NEXT);
		MenuFillBox(0, 3, 29, 13);
		DrawText(1, 5,  "ルートに ふくまれない WPT を");
		DrawText(1, 7,  "さくじょして よろしいですか?");
		DrawText(4, 10, "STARTボタン: さくじょ");
		return 0;
	}
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	if(!(push & KEY_START)) return 0;

	PlaySG1(SG1_CLEAR);
	MenuFillBox(3, 4, 26, 9);
	DrawText(5, 6,  "クリアしています...");
	u32 i = WPT->wpt_count;
	while(i--) if(WptIsUsed(i, 0) == -1) WptDelCheck(i);
	IW->wpt_sort_type = GetSortType(); // 再ソートのためのリセット
	return 1;
}
u32 MP_CurWpt(u16 push){
	if(IW->px.fix <= FIX_INVALID){
		PlaySG1(SG1_CANCEL);
		MenuFillBox(3, 4, 26, 9);
		DrawText(5, 6, "つうしんエラーです!");
		return SetKeyWaitCB(0xffff);
	}

	// 現在地を設定
	IW->wpt_input.lat = IW->px.lat;
	IW->wpt_input.lon = IW->px.lon;
	IW->wpt_input.alt = BiosDiv(IW->px.alt_mm, 1000, &IW->wpt_input.alt);
	IW->wpt_input.name[0] = 0;

	// 新規追加へジャンプ
	IW->mp.menuid = MENU_ID_NEW_WPT;
	IW->mp.sel = 0;
	return 1;
}
u32 MP_DelWpt(u16 push){
	if(push == 0xffff) return SelWpt(push);

	if(IW->mp.mp_flag){
		if(IW->mp.mp_flag == 1 && (push & KEY_START)){
			PlaySG1(SG1_OK);
			WptDelCheck(IW->wpt_sort[IW->mp.wpt_sel]);
			MoveOut(OBJ_MENU_UP);
			MoveOut(OBJ_MENU_DOWN);
			if(!WPT->wpt_count) return 1;
			return SelWpt(0xffff);
		}
		if(push & (KEY_A | KEY_B)) SelWpt(0xffff);
		return 0;
	}
	
	if(push & KEY_A){
		if(IW->mp.wpt_sel == -1) return 2;
		s32 used = WptIsUsed(IW->wpt_sort[IW->mp.wpt_sel], 0);
		if(used < 0){
			PutDelConf();
			IW->mp.mp_flag = 1;
		} else {
			PlaySG1(SG1_CANCEL);
			MenuFillBox(2, 4, 27, 11);
			DrawText(4, 6, "ルート\"");
			PutsName(ROUTE->route[used].name);
			Putc('"');
			DrawText(4, 8, "で つかわれています!");
			IW->mp.mp_flag = 2;
		}
		MoveOut(OBJ_MENU_CURSOR);
		return 0;
	}
	if(IW->mp.wpt_sel == -1) return 2;
	return SelWpt(push);
}


///////////////////////////////////////////////////////////////////////////////
// ウェイポイントのダウンロード
///////////////////////////////////////////////////////////////////////////////
u32 MP_DownloadWpt(u16 push){
	return SetDownloadCB(0xffff);
}

///////////////////////////////////////////////////////////////////////////////
// ルート制御
///////////////////////////////////////////////////////////////////////////////
u32 ChangeTaskRoute(u32 n, const u8* msg){
	if(n >= ROUTE->route_count) return 1;//error

	if(msg){
		MenuFillBox(1, 4, 29, 15);
		DrawTextCenter(6, msg);
		Locate(4, 9);
		Puts("きょり: ");
		PutsDistance(CalcRouteDistance(n));
		DrawTextCenter(12, "このルートをタスクにします");
		IW->mp.save_flag |= SAVEF_CHANGE_WPT; // 設定変更フラグ
	}

	IW->tc.route = n;
	IW->mp.save_flag |= SAVEF_UPDATE_CFG; // 設定変更フラグ
	IW->mp.navi_update |= NAVI_UPDATE_ROUTE;
	IW->mp.pre_route = (Route*)-1;
	return 0;
}

// ルート追加
u32 RouteAddCheck(Route* r){
	if(!r->name[0] && !r->count) return ROUTE_ADD_CANCEL; 
	if(ROUTE->route_count == MAX_ROUTE) return ROUTE_ADD_MAX;
	if(!r->name[0]) return ROUTE_ADD_BADNAME;
	if(!r->count) return ROUTE_ADD_EMPTY;
	u32 i;
	for(i = 0 ; i < ROUTE->route_count ; ++i){
		if(memcmp(ROUTE->route[i].name, r->name, ROUTE_NAMESIZE) == 0) return ROUTE_ADD_SAMENAME;
	}
	ROUTE->route[i] = *r;
	++ROUTE->route_count;
	r->count = 0;
	r->name[0] = 0;

	ChangeTaskRoute(i, "ルートをついかしました");
	return ROUTE_ADD_SUCCESS;
}

// ルート削除
u32 RouteDelCheck(u32 id){
	if(id >= ROUTE->route_count) return ROUTE_DEL_ERROR;
	for(--ROUTE->route_count ; id < ROUTE->route_count ; ++id){
		ROUTE->route[id] = ROUTE->route[id + 1];
	}

	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ルート/WPT変更フラグをセット
	return ROUTE_DEL_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// ルート選択
///////////////////////////////////////////////////////////////////////////////
void PutRouteMenu(u32 id, u32 y){
	Putsf("%M#%02d: ", 1, y, id);
	PutsNameB(ROUTE->route[id].name);
	Putsf(" (%2dpt)", ROUTE->route[id].count);
	Putc((id == IW->tc.route)? 'T' : ' ');
}

u32 MP_ChangeRoute(u16 push);
u32 MP_DelRoute(u16 push);
u32 MP_CopyRoute(u16 push);
u32 MP_SendRoute(u16 push);
u32 MP_TaskInput(u16 push);

u32 SelRoute(u16 push){
	if(push == 0xffff){
		IW->mp.mp_flag = 0;
		if(!ROUTE->route_count){
			PlaySG1(SG1_CANCEL);
			MenuFillBox(4, 4, 26, 9);
			DrawText(6, 6, "ルートがありません!");
			return SetKeyWaitCB(0xffff);
		}
		MenuCls(SELMENU_BK2);
		if(IW->mp.proc == MP_ChangeRoute)		Puts("どのルートを へんこうしますか?");
		else if(IW->mp.proc == MP_DelRoute)		Puts("どのルートを さくじょしますか?");
		else if(IW->mp.proc == MP_CopyRoute)	Puts("どのルートを コピー しますか?");
		else if(IW->mp.proc == MP_SendRoute)	Puts("どのルートを てんそうしますか?");
		else if(IW->mp.proc == MP_TaskInput)	Puts("どのルートを タスクにしますか?");

		if(IW->mp.route_sel >= ROUTE->route_count) IW->mp.route_sel = ROUTE->route_count - 1;
		if(ROUTE->route_count > NO_SCROLL){
			DrawBox(0, 2, 29, 17);
		} else {
			DrawBox(0, 2, 29, ROUTE->route_count * 2 + 3);
			u32 i;
			for(i = 0 ; i < ROUTE->route_count ; ++i) PutRouteMenu(i, i * 2 + 3);
		}
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 2;
	}
	else if(push & KEY_DOWN)	RangeAdd(&IW->mp.route_sel,    1, ROUTE->route_count);
	else if(push & KEY_UP)		RangeAdd(&IW->mp.route_sel,   -1, ROUTE->route_count);
	else if(push & KEY_RIGHT)	RangeAdd(&IW->mp.route_sel,    7, ROUTE->route_count);
	else if(push & KEY_LEFT)	RangeAdd(&IW->mp.route_sel,   -7, ROUTE->route_count);
	else if(push & KEY_R)		RangeAdd(&IW->mp.route_sel,   21, ROUTE->route_count);
	else if(push & KEY_L)		RangeAdd(&IW->mp.route_sel,  -21, ROUTE->route_count);
	else {
		if(ROUTE->route_count > NO_SCROLL) DispMenuArrow(1, 1);
		return 0;
	}

	PlaySG1(SG1_SELECT);
	if(ROUTE->route_count > NO_SCROLL){
		MoveObj(OBJ_MENU_CURSOR, 1, 3 * 16 + 27);
		MoveSelBg(3, SELMENU_0);
		s32 i, id = IW->mp.route_sel - 3;
		if(id < 0) id += ROUTE->route_count;
		for(i = 0 ; i < NO_SCROLL ; ++i, ++id){
			if(id >= ROUTE->route_count) id -= ROUTE->route_count;
			PutRouteMenu(id, i * 2 + 3);
		}
	} else {
		u16 y = IW->mp.route_sel * 16;
		MoveObj(OBJ_MENU_CURSOR, 1, y + 27);
		MoveSelBg(IW->mp.route_sel, SELMENU_0);
	}

	Locate(0, 18);
	Puts("きょり: ");
	PutsDistance(ROUTE->route[IW->mp.route_sel].dist);
	PutsSpace(9);
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// ルートの追加
///////////////////////////////////////////////////////////////////////////////
u32 MP_AddRouteCancel(u16 push){
	IW->mp.menuid = MENU_ID_ROUTE;
	IW->mp.sel = 0;
	IW->mp.proc = 0;
	DispMenu(IW->mp.menuid);

	Route* r = &IW->route_input;
	r->count = 0;
	r->name[0] = 0;
	return 0;
}

u32 MP_AddRoute(u16 push){
	if(push != 0xffff) return push & (KEY_A | KEY_B);
	switch(RouteAddCheck(&IW->route_input)){
	case ROUTE_ADD_MAX:
		PlaySG1(SG1_CANCEL);
		MenuFillBox(0, 4, 29, 9);
		DrawText(1, 6, "ルートは さいだい20けんです");
		return 0;
	case ROUTE_ADD_SAMENAME:
		return PutSameNameMessage("ルート");
	case ROUTE_ADD_BADNAME:
		return PutBadNameMessage();
	case ROUTE_ADD_EMPTY:
		MenuFillBox(0, 4, 29, 9);
		DrawText(2, 6, "WPTをせっていしてください!");
		return 0;
	case ROUTE_ADD_CANCEL:
		return MP_AddRouteCancel(0xffff);
	}
	// 成功時
	IW->mp.menuid = MENU_ID_ROUTE;
	IW->mp.sel = 0;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// ルートの編集
///////////////////////////////////////////////////////////////////////////////
u32 MP_CopyRoute(u16 push){
	if(push == 0xffff || !(push & KEY_A)) return SelRoute(push);
	if(IW->mp.route_sel == -1) return 2;

	PlaySG1(SG1_OK);
	IW->route_input = ROUTE->route[IW->mp.route_sel];
	IW->route_input.name[0] = 0; // 名前だけは初期化

	IW->mp.menuid = MENU_ID_NEW_ROUTE;
	IW->mp.sel = 0;
	return 1;
}
u32 MP_SendRoute(u16 push){
	// 初回チェック
	if(push == 0xffff){
		if(IW->gi.state < GPS_EMUL_INFO || GPS_EMUL_FINISH <= IW->gi.state) return SelRoute(0xffff);
		PlaySG1(SG1_CANCEL);
		MenuFillBox(2, 3, 27, 8);
		DrawTextCenter(5, "つうしんちゅう!");
		return SetKeyWaitCB(0xffff);
	}

	// ルート選択中
	if(!IW->mp.mp_flag){
		if(!(push & KEY_A)) return SelRoute(push);
		if(IW->mp.route_sel == -1) return 2;

		// ルート確定
		void GpsEmuMode(u32 mode, const u16* snd);
		GpsEmuMode(GPS_EMUL_ROUTE, SG1_COMP1);
		MenuFillBox(5, 3, 24, 11);
		DrawTextCenter(5, "てんそうちゅう");
		DrawTextCenter(8, "Bボタン:もどる");
		IW->mp.mp_flag = 1;
	}

	// 転送中
	if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 1; // モードは戻さず転送は継続
	}
	if(IW->gi.state == GPS_EMUL_FINISH || IW->gi.state == GPS_EMUL_FINISH_WAIT){
		return 1; // 正常完了
	}
	if(IW->gi.state < GPS_EMUL_ROUTE || GPS_EMUL_ROUTE_WAIT < IW->gi.state){
		// 転送エラー時。SELECTが押されても強制中断してココにくる
		PlaySG1(SG1_CANCEL);
		MenuFillBox(4, 4, 25, 9);
		DrawTextCenter(6, "てんそうエラー!");
		return SetKeyWaitCB(0xffff);
	}
	return 0;
}

u32 MP_ChangeRoute(u16 push){
	if(push == 0xffff || !(push & KEY_A)) return SelRoute(push);
	if((push & KEY_A) && IW->mp.route_sel == -1) return 2;

	PlaySG1(SG1_OK);
	IW->route_input = ROUTE->route[IW->mp.route_sel];

	IW->mp.menuid = MENU_ID_CHANGE_ROUTE;
	IW->mp.sel = 0; // WPT選択から
	return 1;
}
u32 MP_ChangeRouteCancel(u16 push){
	// route_inputを初期化しておく
	IW->route_input.count = 0;
	IW->route_input.dist  = 0;
	memset(IW->route_input.name, 0, ROUTE_NAMESIZE);

	IW->mp.menuid = MENU_ID_ROUTE;
	IW->mp.proc = MP_ChangeRoute;
	IW->mp.sel = 1;
	return MP_ChangeRoute(0xffff);
}
u32 MP_ChangeRouteWait(u16 push){
	if(push == 0xffff){
		IW->mp.proc = MP_ChangeRouteWait;
		return 0;
	}
	if(push & (KEY_A | KEY_B)) return MP_ChangeRouteCancel(push);
	return 0;
}
u32 MP_ChangeRouteSubmit(u16 push){
	if(push != 0xffff) return push & (KEY_A | KEY_B); // 復帰ボタンチェック

	if(!IW->route_input.count){
		MenuFillBox(0, 4, 29, 9);
		DrawText(2, 6, "WPTをせっていしてください!");
		return 0;
	}

	if(IW->mp.route_sel >= ROUTE->route_count) return 0; // !?
	if(!*IW->route_input.name) return PutBadNameMessage();
	u32 i;
	for(i = 0 ; i < ROUTE->route_count ; ++i){
		if(i == IW->mp.route_sel) continue;
		if(!memcmp(ROUTE->route[i].name, IW->route_input.name, ROUTE_NAMESIZE)){
			return PutSameNameMessage("ルート");
		}
	}

	Route* cur = &ROUTE->route[IW->mp.route_sel];
	if(memcmp(cur, &IW->route_input, sizeof(Route))){
		*cur = IW->route_input;
		ChangeTaskRoute(IW->mp.route_sel, "ルートをへんこうしました");
		return MP_ChangeRouteWait(0xffff);
	}
	return MP_ChangeRouteCancel(push);
}


///////////////////////////////////////////////////////////////////////////////
// ルートの削除
///////////////////////////////////////////////////////////////////////////////
u32 MP_DelRoute(u16 push){
	if(push == 0xffff) return SelRoute(push);

	if(IW->mp.mp_flag){
		if(IW->mp.mp_flag == 2 && (push & KEY_START)){
			PlaySG1(SG1_CHANGE);
			RouteDelCheck(IW->mp.route_sel);
			if(IW->mp.route_sel < IW->tc.route){
				--IW->tc.route;
				IW->mp.pre_route = GetTaskRoute();// 新しい場所にシフト(タスクルートが削除されることはない)
			}
			if(!ROUTE->route_count) return 1;
			return SelRoute(0xffff);
		}
		if(push & (KEY_A | KEY_B)) SelRoute(0xffff);
		return 0;
	}
	
	if(push & KEY_A){
		if(IW->mp.route_sel == -1) return 2;
		if(IW->mp.route_sel == IW->tc.route){
			PlaySG1(SG1_CANCEL);
			MenuFillBox(1, 4, 28, 11);
			DrawText(3, 6, "このルートはタスクのため");
			DrawText(3, 8, "さくじょ できません!");
			IW->mp.mp_flag = 1;
			return 0;
		}
		PutDelConf();
		IW->mp.mp_flag = 2;
		return 0;
	}
	if(IW->mp.route_sel == -1) return 2;
	return SelRoute(push);
}

///////////////////////////////////////////////////////////////////////////////
// ルートのダウンロード
///////////////////////////////////////////////////////////////////////////////
const WptInput WPT_INIT[] = {
	{"A-TO",		1003, IDO(35, 22, 21, 200), IDO(138, 32, 48, 200)},
	{"B-LD",		 712, IDO(35, 22, 13, 500), IDO(138, 33, 15, 500)},
	{"C-NisiKoya",	 713, IDO(35, 22,  8, 500), IDO(138, 33, 11, 200)},
	{"D-Nishi TO",	1175, IDO(35, 22, 32, 300), IDO(138, 32, 21, 200)},
	{"E-Tetto",		1377, IDO(35, 22,  7, 600), IDO(138, 31, 49, 800)},
	{"F-Kitaone",	 951, IDO(35, 21, 44, 100), IDO(138, 33,  7, 900)},
	{"G-Yomo",		1578, IDO(35, 23, 18, 100), IDO(138, 32,  7, 100)},
	{"H-Jujiro",	 705, IDO(35, 21, 59, 300), IDO(138, 33, 31, 500)},
	{"", 0, 0, 0}
};
const u16 SAMPLE_ROUTE[] = { // データを減らすため Route 構造体ではなく、u16配列で定義
	0x6153, 0x706d, 0x656c, 0x3130, 0, 0, 0, // Sample01
	11, 0, 0, // count, dist_lo, dist_hi
	2,-1,  3,-1,  4,-1,  5,-1,  6,-1,  4,-1,  5,-1,  6,-1,  3,-1,  7,-1,
	1,-1,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
};

u32 AddAsagiriRoute(){
	if(ROUTE->route_count){
		// 2回目以降はデバッグ用大量データ
		WptInput wi;
		int i;
		for(i = 0 ; i < 100 ; ++i){
			wi.alt = (WPT->wpt_count + 100);
			wi.lat = (WPT->wpt_count << 14) + IDO( 35, 22, 00, 000) + (MyRand() & 0x3fff);
			wi.lon = (WPT->wpt_count << 14) + IDO(138, 32, 00, 000) + (MyRand() & 0x3fff);
			strcpy(wi.name, "Test");
			u32 n = WPT->wpt_count;
			wi.name[7] = 0;
			wi.name[6] = '0' + (n & 0x3f);
			wi.name[5] = '0' + ((n >>= 6) & 0x3f);
			wi.name[4] = '0' + ((n >>= 6) & 0x3f);
			WptAddCheck(&wi);
		}
		Route r;
		r.count = 99;
		r.dist  = 0;
		for(i = 0 ; i < 100 ; ++i){
			r.py[i].wpt = i;
			r.py[i].cyl = i + 100;
		}
		strcpy(r.name, "Test");
		r.name[4] = 'A' + ROUTE->route_count;
		r.name[5] = 0;
		RouteAddCheck(&r);
		return SetKeyWaitCB(0xffff);
	}

	// 最初の1回だけF1ルートを登録
	const WptInput* p = WPT_INIT;
	u32 i;
	for(i = 0 ; i < 8 ; ++p, ++i){
		WptInput wi = *p;
		WptAddCheck(&wi);
	}
	Route r;
	memcpy(r.name, SAMPLE_ROUTE, sizeof(SAMPLE_ROUTE));
	RouteAddCheck(&r);

	IW->mp.navi_update |= NAVI_UPDATE_ROUTE;

	// 日付越えテスト用
	IW->px.counter = 3600 * 14 + 60 * 55;
	return 0;
}

u32 MP_DownloadRoute(u16 push){
	return SetDownloadCB(0xffff);
}

u32 MP_ClearRoute(u16 push){
	if(push == 0xffff){
		PlaySG1(SG1_NEXT);
		MenuFillBox(0, 3, 29, 13);
		DrawText(1, 5,  "すべてのルートをさくじょして");
		DrawText(1, 7,  "よろしいですか?");
		DrawText(4, 10, "STARTボタン: さくじょ");
		return 0;
	}
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	if(!(push & KEY_START)) return 0;

	PlaySG1(SG1_CLEAR);
	ROUTE->route_count = 0;
	IW->mp.navi_update |= NAVI_UPDATE_ROUTE;
	IW->mp.cur_view = -1;	// 再描画

	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ルート/WPT変更フラグをセット
	IW->mp.pre_route = (Route*)-1;// タスクログも初期化
	IW->wpt_sort_type = GetSortType(); // 再ソートのためのリセット
	return 1;
}

u32 MP_AllClear(u16 push){
	if(push == 0xffff){
		PlaySG1(SG1_NEXT);
		MenuFillBox(0, 3, 29, 13);
		DrawText(1, 5,  "ルートとウェイポインをすべて");
		DrawText(1, 7,  "さくじょして よろしいですか?");
		DrawText(4, 10, "STARTボタン: さくじょ");
		return 0;
	}
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	if(!(push & KEY_START)) return 0;

	PlaySG1(SG1_CLEAR);
	ROUTE->route_count = 0;
	WPT->wpt_count = 0;
	IW->mp.navi_update |= NAVI_UPDATE_ROUTE;
	IW->mp.cur_view = -1;	// 再描画

	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ルート/WPT変更フラグをセット
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// メニュー数値入力
///////////////////////////////////////////////////////////////////////////////
u32 MP_IntInput(u16 push){
	IntInput* ii = &IW->aip.i_int;
	if(push == 0xffff){
		// 入力名
		MenuFillBox (5, 4, 24, 13);
		DrawTextCenter(5, ii->t->name);
		DrawText(20, 8, ii->t->unit);
		//範囲
		Locate(7, 11);
		PutsPointSGN(ii->t->min, 0, ii->t->prec);
		Puts(" - ");
		PutsPointSGN(ii->t->max, 0, ii->t->prec);
	} else if(push & KEY_A){
		PlaySG1(SG1_OK);
		*ii->t->val = ii->val;
		IW->mp.save_flag |= SAVEF_UPDATE_CFG; // 設定変更フラグ
		return 1; // MP終了 (確定)
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 2; // MP終了 (キャンセル)
	} else if(push & KEY_UP){
		PlaySG1(SG1_NEXT);
		if(ii->val < 0 && -ii->val < INT_POS_TABLE[ii->pos]) ii->val = -ii->val;
		else ii->val += INT_POS_TABLE[ii->pos];
	} else if(push & KEY_DOWN){
		PlaySG1(SG1_PREV);
		if(ii->val > 0 && ii->val < INT_POS_TABLE[ii->pos]) ii->val = -ii->val;
		else ii->val -= INT_POS_TABLE[ii->pos];
	} else if(push & (KEY_LEFT | KEY_L)){
		PlaySG1(SG1_SELECT);
		ii->pos++;
	} else if(push & (KEY_RIGHT | KEY_R)){
		PlaySG1(SG1_SELECT);
		ii->pos--;
	} else {
		if(IW->vb_counter & BLANK_INTERVAL){
			MoveOut(OBJ_MENU_KETA);
		} else {
			u32 i = 18 - ii->pos;
			if(ii->pos < ii->t->prec) i++;
			MoveObj(OBJ_MENU_KETA, i * 8, 79);
		}
		return 0;
	}

	ii->pos = Range32(0, ii->t->keta, ii->pos);
	ii->val = Range32(ii->t->min, ii->t->max, ii->val);
	Locate(18 - ii->t->keta, 8);
	PutsPoint0SGN(ii->val, ii->t->keta - ii->t->prec + 1, ii->t->prec);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// メニュー時刻入力
///////////////////////////////////////////////////////////////////////////////
#define MAX_TIME (60 * 24 - 1)
u32 MP_TimeInput(u16 push){
	TimeInput* ii = &IW->aip.i_time;
	if(push == 0xffff){
		// 入力名
		MenuFillBox (6, 4, 23, 12);
		DrawTextCenter(6, ii->t->name);
	} else if(push & KEY_A){
		PlaySG1(SG1_OK);
		*ii->t->val = ii->val;
		IW->mp.save_flag |= SAVEF_UPDATE_CFG; // 設定変更フラグ
		return 1; // MP終了 (確定)
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 2; // MP終了 (キャンセル)
	} else if(push & KEY_UP){
		PlaySG1(SG1_NEXT);
		ii->val += TIME_POS_TABLE[ii->pos];
		if(ii->val > MAX_TIME) ii->val = MAX_TIME;
	} else if(push & KEY_DOWN){
		PlaySG1(SG1_PREV);
		ii->val -= TIME_POS_TABLE[ii->pos];
		if(ii->val < 0) ii->val = 0;
	} else if(push & (KEY_LEFT | KEY_L)){
		if(ii->pos < 3){
			PlaySG1(SG1_SELECT);
			ii->pos++;
		} else {
			PlaySG1(SG1_CANCEL);
		}
		return 0;
	} else if(push & (KEY_RIGHT | KEY_R)){
		if(ii->pos){
			PlaySG1(SG1_SELECT);
			ii->pos--;
		} else {
			PlaySG1(SG1_CANCEL);
		}
		return 0;
	} else {
		if(IW->vb_counter & BLANK_INTERVAL){
			MoveOut(OBJ_MENU_KETA);
		} else {
			u32 i = 16 - ii->pos;
			if(ii->pos < 2) i++;
			MoveObj(OBJ_MENU_KETA, i * 8, 87);
		}
		return 0;
	}
	s32 m, h = BiosDiv(ii->val, 60, &m);
	Putsf("%13.9m%02d:%02d", h, m);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// メニュー名前入力
///////////////////////////////////////////////////////////////////////////////
const u8 NAME_INPUT_TABLE[3][40] = {
	"0123456789" "ABCDEFGHIJ" "KLMNOPQRST" "UVWXYZ -+ ",
	"0123456789" "abcdefghij" "klmnopqrst" "uvwxyz -+ ",
	"0123456789" "!\"#$%&'()*""+,-./:;<=>" "?@_[]     "
};

void PutsNameTable(u32 n){
	if(n > 2) n = 0;
	u32 x = 0, y = 0, i;
	for(i = 0 ; i < 40 ; ++i){
		Locate(x * 2 + 5, y * 3 + 5);
		Putc(NAME_INPUT_TABLE[n][i]);
		if(++x == 10){
			x = 0;
			y++;
		}
	}
	DrawText(23, 14, "\\\\");
	IW->mp.name_table = n;
}

static inline u32 IsBlank(u8 ch){ return ch == 0 || ch == ' '; }

u32 MP_NameInput(u16 push){
	NameInput* ii = &IW->aip.i_name;
	if(push == 0xffff){
		MenuCls(SELMENU_BK2);
		SelectMap(MAP_BG1);
		DrawBox(1, 0, 2 + ii->t->max, 3);
		SelectMap(MAP_BG0);
		Putsf("%M%dもじ", 3 + ii->t->max, 1, ii->t->max);
		Locate(2, 2);
		u32 i;
		for(i= 0 ; i < ii->t->max ; ++i) Putc('=');

		// 入力枠
		DrawBox(2, 4, 26, 19);
		PutsNameTable(0);
		DrawText(5, 17, "けってい  キャンセル");
		//初期化
		ii->x = 0;
		ii->y = 0;
		ii->pos = strlen(ii->val);
	} else if(push & KEY_SELECT){
		PlaySG1(SG1_NEXT);
		PutsNameTable(IW->mp.name_table + 1);
	} else if(push & KEY_START){
		PlaySG1(SG1_SELECT);
		ii->x = (ii->x == 0 && ii->y == 4)? 5 : 0;
		ii->y = 4;
	} else if(push & KEY_UP){
		PlaySG1(SG1_SELECT);
		if(!ii->y--) ii->y = 4;
	} else if(push & KEY_DOWN){
		PlaySG1(SG1_SELECT);
		if(ii->y++ == 4) ii->y = 0;
	} else if(push & KEY_LEFT){
		PlaySG1(SG1_SELECT);
		if(ii->y == 4) {
			ii->x = (ii->x < 5)? 5 : 0;
		} else {
			if(!ii->x--) ii->x = 9;
		}
	} else if(push & KEY_RIGHT){
		PlaySG1(SG1_SELECT);
		if(ii->y == 4) {
			ii->x = (ii->x < 5)? 5 : 0;
		} else {
			if(ii->x++ == 9) ii->x = 0;
		}
	} else if(push & KEY_L){
		if(ii->pos){
			PlaySG1(SG1_SELECT);
			--ii->pos;
		} else {
			PlaySG1(SG1_CANCEL);
		}
	} else if(push & KEY_R){
		if(ii->val[ii->pos] && ii->pos < ii->t->max){
			PlaySG1(SG1_SELECT);
			++ii->pos;
		} else {
			PlaySG1(SG1_CANCEL);
		}
	} else if(push & KEY_A){ // 入力
		if(ii->y == 4){
			if(ii->x < 5){
				// 確定
				PlaySG1(SG1_OK);
				memcpy(ii->t->val, ii->val, ii->t->max);
				IW->mp.save_flag |= SAVEF_UPDATE_CFG; // 設定変更フラグ
			} else {
				// キャンセル
				PlaySG1(SG1_CANCEL);
			}
			return 1; // 終了
		}
		if(ii->y == 3 && ii->x == 9){
			PlaySG1(SG1_NEXT);
			PutsNameTable(IW->mp.name_table + 1);
		} else {
			s32 i = strlen(ii->val);
			if(i < ii->t->max){
				PlaySG1(SG1_CHANGE);
				for(; i >= ii->pos ; --i) ii->val[i + 1] = ii->val[i];
				ii->val[ii->pos++] = NAME_INPUT_TABLE[IW->mp.name_table][10 * ii->y + ii->x];
			} else {
				PlaySG1(SG1_CANCEL);
			}
		}
	} else if(push & KEY_B){ // 削除
		if(ii->val[0]){
			PlaySG1(SG1_PREV);
			if(ii->pos) --ii->pos;
			u32 i = ii->pos;
			for(; !!(ii->val[i] = ii->val[i + 1]) ; ++i);
		} else {
			PlaySG1(SG1_CANCEL);
		}
	} else {
		u32 x = (ii->pos == ii->t->max || (IW->vb_counter & BLANK_INTERVAL))? 240 : (ii->pos * 8 + 16);
		MoveObj(OBJ_MENU_KETA, x, 23);
		return 0;
	}

	Locate(2, 1);
	PutsName2(ii->val, ii->t->max, 1);
	if(ii->pos == ii->t->max) MoveOut(OBJ_MENU_KETA); // out
	u32 x = ii->x;
	if(ii->y == 4) x = (x < 5)? 0 : 5;
	MoveObj(OBJ_MENU_CURSOR, x * 16 + 35, ii->y * 24 + 42);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// メニュー緯度経度入力
///////////////////////////////////////////////////////////////////////////////
u32 MP_LatInput(u16 push){
	LatInput* ii = &IW->aip.i_lat;
	if(push == 0xffff){
		MenuFillBox(6, 5, 23, 12);
		DrawText(8, 7, ii->t->latlon? "いど:" : "けいど:");
	} else if(push & KEY_A){
		PlaySG1(SG1_OK);
		*ii->t->val = ii->val;
		IW->mp.save_flag |= SAVEF_UPDATE_CFG; // 設定変更フラグ
		return 1; // MP終了 (確定)
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 2; // MP終了 (キャンセル)
	} else if(push & KEY_UP){
		PlaySG1(SG1_NEXT);
		if(ii->pos == 10 - ii->t->latlon) ii->val = -ii->val;
		else if(ii->val < 0 && -ii->val < LAT_POS_TABLE[ii->pos]) ii->val = -ii->val;
		else ii->val += LAT_POS_TABLE[ii->pos];
		ii->val = MinS32(ii->val,  LAT_MAX[ii->t->latlon]);
	} else if(push & KEY_DOWN){
		PlaySG1(SG1_PREV);
		if(ii->pos == 10 - ii->t->latlon) ii->val = -ii->val;
		else if(ii->val > 0 && ii->val < LAT_POS_TABLE[ii->pos]) ii->val = -ii->val;
		else ii->val -= LAT_POS_TABLE[ii->pos];
		ii->val = MaxS32(ii->val, -LAT_MAX[ii->t->latlon]);
	} else if(push & (KEY_LEFT | KEY_L)){
		PlaySG1(SG1_SELECT);
		if(ii->pos < 10 - ii->t->latlon) ii->pos++;
	} else if(push & (KEY_RIGHT | KEY_R)){
		PlaySG1(SG1_SELECT);
		if(ii->pos) ii->pos--;
	} else {
		if(IW->vb_counter & BLANK_INTERVAL){
			MoveOut(OBJ_MENU_KETA);
		} else {
			MoveObj(OBJ_MENU_KETA, 168 - LAT_VAL_POS[ii->t->latlon][ii->pos] * 8, 87);
		}
		return 0;
	}

	Locate(8, 9);
	if(ii->t->latlon) PutsLat(ii->val);
	else              PutsLon(ii->val);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// RtWpt入力
///////////////////////////////////////////////////////////////////////////////
#define RTWPT_NO_SCROLL 7
void AddRtWptSel(s32 v){
	IW->mp.rtwpt_sel += v;
	if(v < 0){
		if(IW->mp.rtwpt_sel < 0) IW->mp.rtwpt_sel = 0;
		v = IW->mp.rtwpt_sel / 2;
		if (IW->mp.rtwpt_dsp > v) IW->mp.rtwpt_dsp = v;
	} else {
		Route* rt = &IW->route_input;
		if(IW->mp.rtwpt_sel > rt->count * 2) IW->mp.rtwpt_sel = rt->count * 2;
		if(rt->count > RTWPT_NO_SCROLL){
			v = (IW->mp.rtwpt_sel + 1) / 2 - RTWPT_NO_SCROLL;
			if(IW->mp.rtwpt_dsp < v) IW->mp.rtwpt_dsp = v;
		}
	}
}

// RtWpt追加
u32 AddRtWpt(){
	Route* rt = &IW->route_input;
	u32 i = rt->count;
	if(i >= MAX_ROUTE_PT) return 1;
	u32 end = IW->mp.rtwpt_sel / 2;
	for(; i > end ; --i) rt->py[i] = rt->py[i - 1];
	rt->py[i].wpt = IW->wpt_sort[IW->mp.wpt_sel];
	rt->py[i].cyl = -1; // デフォルト
	rt->count++;
	return 0;
}

// RtWptメニューの1行を表示
void PutRtWptMenu(u32 id, u32 y){
	Putsf("%M%d.", 1, y, id + 1);
	u32 i = 3;
	while(--i && INT_POS_TABLE[i] > id + 1) Putc(' ');
	Wpt* w = &WPT->wpt[IW->route_input.py[id].wpt];
	PutsNameB(w->name);
	Putsf("%5dm ", w->alt);

	u16 v = IW->route_input.py[id].cyl;
	switch(v){
	case 0:
		Puts("  なし ");
		break;
	case 0xffff:
		Putsf("%5dm ", IW->tc.cylinder);
		break;
	default:
		Putsf("%5dm!", v);
	}
}

// RtWptメニューの初期化
void InitRtWptMenu(){
	Route* rt = &IW->route_input;
	MenuCls(SELMENU_BK2);
	Puts("ルート:\"");
	u32 i;
	for(i = 0 ; i < ROUTE_NAMESIZE && rt->name[i] ; ++i) Putc(rt->name[i]);
	Putc('"');
	if(rt->count > RTWPT_NO_SCROLL){
		DrawBox(0, 2, 29, 17);
		DrawText(18, 18, "Alt.   Cyl.");
	} else {
		i = rt->count * 2 + 3;
		DrawBox(0, 2, 29, i);
		if(rt->count) DrawText(18, i + 1, "Alt.   Cyl.");
		for(i = 0 ; i < rt->count ; ++i) PutRtWptMenu(i, i * 2 + 3);
	}
	IW->mp.rtwpt_sum = 0;
	IW->mp.rtwpt_calc = 1; // 計算開始
}

const IntInputTemplate INDV_CYL = {
	5, 1, 0, -1, 65000, 
	"シリンダはんけい",
	"m",
	IW_OFFSET(mp.cyl_input)
};

u32 MP_RtWptInput(u16 push){
	Route* rt = &IW->route_input;
	if(push == 0xffff){					// 初期設定
		IW->mp.mp_flag = 0;
		IW->mp.rtwpt_state = 0;
		IW->mp.rtwpt_sel = 0;
		IW->mp.rtwpt_dsp = 0;
		if(!WPT->wpt_count){
			PlaySG1(SG1_CANCEL);
			MenuFillBox(5, 4, 24, 9);
			DrawText(7, 6, "WPTがありません!");
			return SetKeyWaitCB(0xffff);
		}
		if(!IW->route_input.count){
			IW->mp.rtwpt_state = 1;
			return SelWpt(0xffff);
		}
		InitRtWptMenu();
	} else if(IW->mp.rtwpt_state == 1){ // 追加
		if(push & KEY_A){
			IW->mp.rtwpt_state = 0;
			AddRtWpt();
			AddRtWptSel(2);
			InitRtWptMenu();
		} else {
			if(!SelWpt(push)) return 0;
			IW->mp.rtwpt_state = 0;
			InitRtWptMenu();
		}
	} else if(IW->mp.rtwpt_state == 2){ // 削除 or 
		if(push & KEY_START){
			// 削除
			u32 i;
			rt->count--;
			for(i = IW->mp.rtwpt_sel / 2 ; i < rt->count ; ++i){
				rt->py[i] = rt->py[i + 1];
			}
			AddRtWptSel(-2);
			if(IW->mp.rtwpt_dsp) --IW->mp.rtwpt_dsp;
			PlaySG1(SG1_CLEAR);
		} else if(push & KEY_A){
			// シリンダ変更
			PlaySG1(SG1_OK);
			IW->mp.rtwpt_state = 3;
			const IntInputTemplate* t = &INDV_CYL;
			IntInput* ii = &IW->aip.i_int;
			ii->t = t;
			ii->val	= rt->py[IW->mp.rtwpt_sel / 2].cyl;
			if(ii->val == 0xffff) ii->val = -1;
			ii->pos = 0;
			MP_IntInput(0xffff);
			MenuFillBox(1, 13, 28, 18);
			DrawTextCenter(15, "-1はデフォルトサイズです");
			return 0;
		} else if(!(push & KEY_B)) return 0;

		// キャンセル復帰
		IW->mp.rtwpt_state = 0;
		InitRtWptMenu();
	} else if(IW->mp.rtwpt_state == 3){ // シリンダ変更
		switch(MP_IntInput(push)){
		case 1: // 確定
			rt->py[IW->mp.rtwpt_sel / 2].cyl = IW->aip.i_int.val;
			break;
		case 2: // キャンセル
			break;
		default:
			return 0;
		}
		// 決定処理
		IW->mp.rtwpt_state = 0;
		InitRtWptMenu();
	} else if(push & KEY_A){
		if(IW->mp.rtwpt_sel & 1){
			// 削除
			IW->mp.rtwpt_state = 2;
//			PutDelConf();
			PlaySG1(SG1_CHANGE);
			MenuFillBox(1, 4, 28, 12);
			DrawText(3, 6, "Aボタン:シリンダへんこう");
			DrawText(3, 9, "START  :パイロンさくじょ");
			return 0;
		} else {
			// 追加
			if(IW->route_input.count < MAX_ROUTE_PT){
				IW->mp.rtwpt_state = 1;
				SelWpt(0xffff);
			} else {
				PlaySG1(SG1_CANCEL);
			}
		}
		return 0;
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 2;
	}
	else if(push & KEY_DOWN)	AddRtWptSel(  1);
	else if(push & KEY_UP)		AddRtWptSel( -1);
	else if(push & KEY_RIGHT)	AddRtWptSel( 14);
	else if(push & KEY_LEFT)	AddRtWptSel(-14);
	else if(push & KEY_R)		AddRtWptSel( 42);
	else if(push & KEY_L)		AddRtWptSel(-42);
	else {
		// 総距離表示
		if(IW->mp.rtwpt_calc){
			if(IW->mp.rtwpt_calc < IW->route_input.count){
				Wpt* w1 = &WPT->wpt[IW->route_input.py[IW->mp.rtwpt_calc - 1].wpt];
				Wpt* w2 = &WPT->wpt[IW->route_input.py[IW->mp.rtwpt_calc++  ].wpt];
				s32 len;

				CalcDist(w1->lat, w1->lon, w2->lat, w2->lon, &len, 0);
				IW->mp.rtwpt_sum += len;
			} else {
				// 完了
				Locate(24, 0);
				PutsDistance(IW->mp.rtwpt_sum);
				IW->mp.rtwpt_calc = 0;
			}
		}
		return DispMenuArrow(IW->mp.rtwpt_dsp, IW->mp.rtwpt_dsp < rt->count - RTWPT_NO_SCROLL);
	}

	// スクロール
	PlaySG1(SG1_SELECT);
	if(rt->count > RTWPT_NO_SCROLL){
		u32 dif = IW->mp.rtwpt_sel - IW->mp.rtwpt_dsp * 2;
		MoveObj(OBJ_MENU_CURSOR, 1, dif * 8 + 19);
		MoveSelBg(dif / 2, (IW->mp.rtwpt_sel & 1)? SELMENU_0 : SELMENU_1);
		u32 i;
		for(i = 0 ; i < RTWPT_NO_SCROLL ; ++i) PutRtWptMenu(i + IW->mp.rtwpt_dsp, i * 2 + 3);
	} else {
		MoveObj(OBJ_MENU_CURSOR, 1, IW->mp.rtwpt_sel * 8 + 19);
		MoveSelBg(IW->mp.rtwpt_sel / 2, (IW->mp.rtwpt_sel & 1)? SELMENU_0 : SELMENU_1);
	}

	// ヘルプ行表示
	Locate(0, 18);
	if(IW->mp.rtwpt_sel & 1){
		Putsf("%d.をへんこう    ", IW->mp.rtwpt_sel / 2 + 1);
	} else {
		// 中間
		if(IW->route_input.count < MAX_ROUTE_PT){
			Putsf("%d.にWPTをついか ", IW->mp.rtwpt_sel / 2 + 1);
		} else {
			Puts("ついかできません  ");
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// タスク入力
///////////////////////////////////////////////////////////////////////////////
u32 MP_TaskInput(u16 push){
	if(push == 0xffff){
		IW->mp.route_sel = IW->tc.route;
		return SelRoute(push);
	} else if(push & KEY_A){
		if(IW->mp.route_sel == -1) return 2;
	} else {
		return SelRoute(push);
	}
	PlaySG1(SG1_OK);
	if(IW->tc.route != IW->mp.route_sel) ChangeTaskRoute(IW->mp.route_sel, 0);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// 風速計補正
///////////////////////////////////////////////////////////////////////////////
const IntInputTemplate CALIB_REF = {
	3, 0, 1, 1, 999,
	"リファレンス",
	"m/s",
	IW_OFFSET(anemo.calib_input)
};

void PutsCalibStatus(){
	if(IW->anemo.calib_flag == 2){
		s32 dif = CALIB_END_TIME - (IW->anemo.tm - IW->anemo.calib_tm);
		if(dif < 0) dif = 0;
		Putsf("%23.6m%2d", dif >> 14);

		UnderLine(23, 8, 2);
	}

	CalcAnemometer();
	Putsf("%5.5mPulse:%8d%r" "Dt:%11d%r" "RPM:%6.3f", IW->anemo.pulse, IW->anemo.dif_tm, IW->anemo.rpm);
}

#define TIMEOUT (10 * 60)
u32 MP_WCALIB2(u16 push){
	if(push == 0xffff){
		MenuCls(SELMENU_BG);
		MenuFillBox(0, 3, 29, 19);
		DrawText(1, 0, "Anemometer Calibration (Ref)");
		DrawText(2, 13, "そくていをはじめてください");
		DrawText(2, 16, "Bボタンでキャンセルします");
		IW->anemo.calib_flag = 1;
		IW->mp.mp_flag = 0;
	} else if(!IW->anemo.calib_flag){
		if(IW->mp.mp_flag){
			switch(MP_IntInput(push)){
			case 1: // 確定
				{
					s32 t = IW->anemo.calib_tm * IW->aip.i_int.val;
					while(t > 21474836){
						t >>= 1;
						IW->anemo.calib_pulse >>= 1;
					}
					if(IW->anemo.calib_pulse){
						PlaySG1(SG1_OK);
						IW->tc.anemo_coef = RoundDiv(t * 100, IW->anemo.calib_pulse);
						return 1;
					}
				}
				PlaySG1(SG1_CANCEL);
				MenuFillBox(3, 4, 26, 9);
				DrawTextCenter(6, "ほせいエラーです!");
				return SetKeyWaitCB(0xffff);
			case 2: // キャンセル
				return 1;
			}
		} else if(!IW->anemo.calib_pulse){
			PlaySG1(SG1_CANCEL); // NG!? 念のためチェック
			return 1;
		} else {
			IW->mp.mp_flag = 1;
			const IntInputTemplate* t = &CALIB_REF;
			IntInput* ii = &IW->aip.i_int;
			ii->t = t;
			ii->val	= *t->val? *t->val : 10;
			ii->pos = 0;
			MP_IntInput(0xffff);
		}
		return 0;
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 1;
	}

	PutsCalibStatus();
	return 0;

}

u32 MP_WCALIB3(u16 push){
	if(push == 0xffff){
		if(IW->px.fix <= FIX_INVALID){
			PlaySG1(SG1_CANCEL);
			MenuFillBox(3, 4, 26, 9);
			DrawTextCenter(6, "GPSエラーです!");
			return SetKeyWaitCB(0xffff);
		}
		MenuCls(SELMENU_BG);
		MenuFillBox(0, 3, 29, 19);
		DrawText(1, 0, "Anemometer Calibration (GPS)");
		DrawText(2, 14, "そくていをはじめてください");
		DrawText(2, 16, "Bボタンでキャンセルします");
		IW->anemo.calib_flag = 1;
		IW->anemo.calib_vsum = IW->px.vh_mm;
		IW->anemo.calib_vcnt = 1;
		IW->mp.mp_flag = IW->px.counter;
	} else if(!IW->anemo.calib_flag){
		s32 t = RoundDiv(IW->anemo.calib_vsum, IW->anemo.calib_vcnt);
		while(t > 8192){ // 8192 * 163840 < 2^31
			t >>= 1;
			IW->anemo.calib_pulse >>= 1;
		}
		if(IW->anemo.calib_pulse){
			PlaySG1(SG1_OK);
			IW->tc.anemo_coef = RoundDiv(IW->anemo.calib_tm * t, IW->anemo.calib_pulse);
			return 1;
		}
		PlaySG1(SG1_CANCEL);
		MenuFillBox(3, 4, 26, 9);
		DrawTextCenter(6, "ほせいエラーです!");
		return SetKeyWaitCB(0xffff);
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 1;
	}

	PutsCalibStatus();
	if(IW->mp.mp_flag != IW->px.counter){
		IW->mp.mp_flag = IW->px.counter;
		IW->anemo.calib_vsum += IW->px.vh_mm;
		IW->anemo.calib_vcnt++;
		Putsf("%5.11mGPS:%6.3fm/s", IW->px.vh_mm);
	}
	return 0;

}

///////////////////////////////////////////////////////////////////////////////
// Bluetooth
///////////////////////////////////////////////////////////////////////////////
// Bluetoothモードが"なし"ならエラーとする
s32 BtModeCheck(){
	if(IW->tc.bt_mode) return 1;
	PlaySG1(SG1_CANCEL);
	MenuFillBox(4, 4, 25, 9);
	DrawTextCenter(6, "モードエラー!");
	return 0;
}

// 12桁正しくセットされていなければエラーとする
s32 BtAddrCheck(){
	if(IsBluetoothAddr(IW->tc.bt_addr)) return 1;
	PlaySG1(SG1_CANCEL);
	MenuFillBox(1, 4, 28, 9);
	DrawTextCenter(6, "Bluetoothアドレスエラー!");
	return 0;
}

// データリンク層のモード切替
static inline void SetATCMode(s32 mode){
	if(mode){
		// ATコマンド制御モードに移る
		IW->gi.state = GPS_BT_BR_CHECK1;// まずはボーレートチェックから
		IW->dl_fsm   = DL_FSM_ATC_WAIT;
		
		// デバッグ用に切り替え&クリア
		u32* log = &IW->mp.last_wpt_len;
		if(!(*log & DBGDMP_COM)) *log = DBGDMP_COM;
	} else {
		// GPSデータ受信モードに戻る
		IW->gi.state = GPS_GET_PINFO_WAIT;
		IW->dl_fsm   = DL_FSM_WAIT_DLE;
	}
}

// デバイスサーチ＆選択
u32 MP_BtInq(u16 push){
	if(push == 0xffff){
		if(!BtModeCheck()) return SetKeyWaitCB(0xffff);
		MenuCls(SELMENU_BK1);
		FillBox(0, 0, 29, 19);
		DrawText(1, 17, "サーチちゅう...");
		DrawText(5, 9, "Bボタンでキャンセル");

		ATC_PTR->inq_count = 0;
		IW->mp.scr_pos = 0;
		IW->mp.mp_flag = 0;
		SetATCMode(1);
	} else if(push & KEY_B){
		// INQ処理中断
		PlaySG1(SG1_CANCEL);
		SetATCMode(0);
		return 1;
	} else if((push & KEY_A) && IW->mp.scr_pos < ATC_PTR->inq_count){
		// カーソル位置のデバイスを選択
		PlaySG1(SG1_CHANGE);
		memcpy(IW->tc.bt_addr, ATC_PTR->inq_list[IW->mp.scr_pos], 12);
		SetATCMode(0);
		return 1;
	} else if((push & (KEY_UP | KEY_DOWN)) && ATC_PTR->inq_count){
		// カーソル移動
		if(push & KEY_UP){
			PlaySG1(SG1_PREV);
			if(IW->mp.scr_pos) IW->mp.scr_pos--;
			else               IW->mp.scr_pos = ATC_PTR->inq_count - 1;
		}
		if(push & KEY_DOWN){
			PlaySG1(SG1_NEXT);
			if(IW->mp.scr_pos + 1 < ATC_PTR->inq_count) IW->mp.scr_pos++;
			else                                        IW->mp.scr_pos = 0;
		}
	}
	
	// 経過表示用
	switch(IW->gi.state){
	case GPS_BT_BR_OK:
		// 特にメッセージは出さずに継続
		IW->gi.state = GPS_BT_INQ_START;
		break;

	case GPS_BT_INQ_DONE:
		// INQ完了したら"サーチちゅう..."を消去する
		if(ATC_PTR->inq_count < MAX_INQ_DEV){
			Locate(1, 17);
			PutsSpace(28);
		}
		break;

	case GPS_BT_ERROR:
		DrawText(1, 17, "サーチエラーです.");
		break;
	}

	// INQリストにアップデートがあれば即画面に反映
	for(; IW->mp.mp_flag < ATC_PTR->inq_count ; IW->mp.mp_flag++){ // 追加分だけ
		Locate(1, IW->mp.mp_flag * 2 + 1);
		PutsName2(ATC_PTR->inq_list[IW->mp.mp_flag], 28, 1); // 最大28文字まで
	}

	// カーソル位置表示
	if(ATC_PTR->inq_count){
		MoveObj(OBJ_MENU_CURSOR, 1, IW->mp.scr_pos * 16 + 9);
		MoveSelBg(IW->mp.scr_pos, SELMENU_0);
	}
	return 0;
}

// 接続/切断の共通フック
u32 MP_BtCmd(u16 push){
	if(push == 0xffff){
		if(!BtModeCheck()) return SetKeyWaitCB(0xffff);
		FillBox(2, 4, 27, 12);
		DrawText(4, 9, "(Bボタンでもどる)");
		SetATCMode(1);
		IW->mp.proc = MP_BtCmd; // コールバックを変える
		ATC_PTR->inq_count = 0;
	} else if(push & KEY_B){
		// 接続処理中断
		PlaySG1(SG1_CANCEL);
		SetATCMode(0);
		return 1;
	}

	// 経過表示用
	const u8* msg = 0;
	switch(IW->gi.state){
	case GPS_BT_BR_CHECK1:				msg = "ボーレートかくにん..."; break;
	case GPS_BT_BR_RESET_WAIT:
	case GPS_BT_CON_RESET_WAIT:
	case GPS_BT_SCAN_RESET_WAIT:		msg = "リセットちゅう...    "; break;
	
	case GPS_BT_BR_CHANGE_WAIT:
	case GPS_BT_BR_MODE0_WAIT:
	case GPS_BT_CON_MODECHANGE_WAIT:	msg = "モードへんこう...    "; break;
	
	case GPS_BT_CON_SETKEY_WAIT:		msg = "PINコードとうろく... "; break;
	case GPS_BT_CON_DIAL_WAIT:			msg = "ペアリング...        "; break;
	case GPS_BT_CON_STANDBY_WAIT:		msg = "スタンバイきりかえ..."; break;
	case GPS_BT_CON_ATH_WAIT:			msg = "せつだんちゅう...    "; break;
	case GPS_BT_SCAN_START_WAIT:		msg = "まちうけきりかえ...  "; break;

	case GPS_BT_CON_COMPLETE:
	case GPS_BT_SCAN_COMPLETE:
	case GPS_BT_IDLE_COMPLETE:			msg = "かんりょうしました.  "; break;
	case GPS_BT_ERROR:					msg = "つうしんエラーです!  "; break;
	case GPS_BT_BR_OK:
		// 特にメッセージは出さずに継続
		IW->gi.state = IW->mp.mp_flag;
		break;
	}
	if(msg) DrawText(4, 6, msg);

	// 完了 or エラーまで行ったら、Aボタンで復帰できるようkeyWaitに切り替える
	switch(IW->gi.state){
	case GPS_BT_CON_COMPLETE:
	case GPS_BT_SCAN_COMPLETE:
	case GPS_BT_IDLE_COMPLETE:
	case GPS_BT_ERROR:
		PlaySG1(SG1_CHANGE);
		SetATCMode(0);
		return SetKeyWaitCB(0xffff);
	}
	return 0;
}

// 設定内容で接続開始＆モード切替
u32 MP_BtConnect(u16 push){
	if(!BtAddrCheck()) return SetKeyWaitCB(0xffff);
	IW->mp.mp_flag = GPS_BT_CON_SETKEY;
	return MP_BtCmd(0xffff);
}

// 設定内容で接続開始＆モード切替
u32 MP_BtScan(u16 push){
	IW->mp.mp_flag = GPS_BT_SCAN_START;
	return MP_BtCmd(0xffff);
}

// 設定内容で接続開始＆モード切替
u32 MP_BtDisconnect(u16 push){
	IW->mp.mp_flag = GPS_BT_IDLE_START;
	return MP_BtCmd(0xffff);
}


///////////////////////////////////////////////////////////////////////////////
// メインメニュー

#define HELP_CONFIG_DETAL1	"せっていをセーブします. " \
							"ルートやウェイポイントがへんこうされているときには, それらもセーブします.  <>"

const MenuItem MENU_MAIN[] = {
	{"じょうほう",			"じょうほうを みます",				MENU_ID_INFO},
	{"タスク",				"タスクを せっていします",			MENU_ID_TASK},
	{"ルート",				"ルートを せっていします",			MENU_ID_ROUTE},
	{"ウェイポイント",		"ウェイポイントをせっていします",	MENU_ID_WAYPOINT},
	{"トラックログ",		"トラックログを かくにんします",	MENU_ID_CONFIG_LOG},
	{"コンフィグ",			"パラメータをへんこうします",		MENU_ID_CONFIG},
	{"セーブ",				HELP_CONFIG_DETAL1,					MENU_FCALL,		MP_Save},
	{"サスペンド",			"ナビの でんげんを きります",		MENU_FCALL,		MP_Suspend},
	{0, 0,														MENU_FCALL,		MP_SaveCheck} // ナビに戻る
};

///////////////////////////////////////////////////////////////////////////////
// タスクメニュー
const u8* const TASK_MODE_LIST[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.task_mode),
	"フリーフライト",
	"スピードラン",
	"ゴールレース",
};
const IntInputTemplate TASK_CY_R = {
	5, 0, 0, 0, 99999, 
	"シリンダはんけい",
	"m",
	IW_OFFSET(tc.cylinder)
};
const IntInputTemplate TASK_SECTOR = {
	5, 0, 0, 0, 99999, 
	"セクタはんけい",
	"m",
	IW_OFFSET(tc.sector)
};
const TimeInputTemplate TASK_CLOSE_TIME = {
	"クローズタイム",
	IW_OFFSET(tc.close_time)
};
const u8* const TASK_START_LIST[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.start_type),
	"フリースタート",
	"イン スタート",
	"アウトスタート",
};
const TimeInputTemplate TASK_START_TIME = {
	"スタートタイム",
	IW_OFFSET(tc.start_time)
};
const u8* const TASK_GOAL_TYPE[] = {
	(u8*)4,
	(u8*)IW_OFFSET(tc.goal_type),
	"ライン",
	"シリンダ",
	"セクタ180°",
	"シリンダセクタ",
};
const IntInputTemplate TASK_PRE_PYLON = {
	2, 0, 0, 0, 99, 
	"プレパイロン",
	"",
	IW_OFFSET(tc.pre_pylon)
};

#define HELP_TASK1	"タスクのタイプを えらびます.  " \
					"\"フリーフライト\":レースをおこないません. " \
						"このモードでカーソルキーをおすと,ちかくのウェイポイントにナビゲートします.  " \
					"\"スピードラン\":スタートからゴールまでのタイムをきそいます.  " \
					"\"ゴールレース\":スタートタイムに どうじスタートして,ゴールじゅんじょをきそいます. " \
						"(かならずスタートタイムをせっていしてください)  <>"
#define HELP_TASK2	"シリンダのデフォルトはんけいを せっていします. シリンダをつかわないときには 0m にします.  " \
					"シリンダサイズは ルートメニューから,パイロンごとにへんこうできます.  <>"
//#define HELP_TASK3	"スタートシリンダの はんけいを せっていします. -1のときにはシリンダ(r)をつかいます.  <>"
//#define HELP_TASK4	"ゴールシリンダの はんけいを せっていします. -1のときにはシリンダ(r)をつかいます.  <>"
#define HELP_TASK5	"セクタはんけいをせっていします. セクタをつかわないときには 0m にします.  " \
					"セクタがせっていされていると,セクタかくにんようのグライダひょうじを おこないます.  <>"
#define HELP_TASK6	"レースのスタートモードの せっていメニューにすすみます.  <>"
#define HELP_TASK7	"ゴールはんていのタイプをえらびます.  " \
					"\"ライン\":ゴールラインをつうかしたときをゴールとみなします.  " \
						"ゴールラインのながさはシリンダサイズとおなじです.  " \
					"\"シリンダ\":ゴールシリンダへの しんにゅうをゴールとします.  " \
					"\"セクタ180°\":ゴールセクタ(180°はんえん)への しんにゅうをゴールとします.  " \
					"\"シリンダセクタ\":つうじょうパイロンとおなじように,シリンダまたはセクタ(90°)への " \
						"しんにゅうをゴールとします.  <>"
#define MENU_TASK8	"タスクのルートを えらびます. " \
					"(タスクせんたくじにPC/GBAからルートそうしんリクエストがくると, カーソルでせんたくちゅうのルートのみを そうしんします)  <>"



#define MENU_TASK_START_DETAIL3	"レースのクローズタイムをせっていします. " \
								"(24Hタイムです. PM1:00は13:00を せっていします.) " \
								"クローズタイムになると,じどうでフリーフライトモードになり," \
								"ゴールパイロンにナビゲートします. " \
								"00:00のときは,クローズタイムをつかいません.  <>"

#define MENU_TASK_START_DETAIL	"スタートタイプを えらびます.  " \
								"\"フリースタート\":スタートシリンダにはいると,タスクスタートします.  " \
								"\"インスタート\":スタートタイムのあとに スタートシリンダにはいると," \
									"タスクスタートします.  " \
								"\"アウトスタート\":スタートタイムのあとに スタートシリンダからでると," \
									"タスクスタートします.  <>"
#define MENU_TASK_START_DETAIL2	"スタートタイムをせっていします. (24Hタイムです. PM1:00は13:00を せっていします.)  " \
								"このせっていは,フリースタートのときには つかいません.  <>"

const MenuItem MENU_TASK[] = {
	{"タスクタイプ",		HELP_TASK1,							MENU_SEL_ENUM,	&TASK_MODE_LIST},
	{"ルート",				MENU_TASK8,							MENU_SEL_TASK,	0},
	{"スタート",			HELP_TASK6,							MENU_SEL_START, (void*)MENU_ID_TASK_START},
	{"クローズ",			MENU_TASK_START_DETAIL3,			MENU_SEL_TIME,	&TASK_CLOSE_TIME},
	{"ゴールタイプ",		HELP_TASK7,							MENU_SEL_ENUM,	&TASK_GOAL_TYPE},
	{"シリンダ(r)"	,		HELP_TASK2,							MENU_SEL_VAL,	&TASK_CY_R},
//	{"-Sシリンダ(r)",		HELP_TASK3,							MENU_SEL_VAL,	&TASK_START_CY},
//	{"-Gシリンダ(r)",		HELP_TASK4,							MENU_SEL_VAL,	&TASK_GOAL_CY},
	{"セクタ(r)",			HELP_TASK5,							MENU_SEL_VAL,	&TASK_SECTOR},
	{0, 0,														MENU_ID_MAIN}
};

///////////////////////////////////////////////////////////////////////////////
// タスク - スタート

const u8* const TASK_START_ALARM[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.start_alarm),
	"なし",
	"あり",
};
const u8* const TASK_START_TO[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.skip_TO),
	"タスクパイロン",
	"テイクオフ",
};

#define MENU_TASK_START_DETAIL4	"プレパイロンの かずを せっていします. " \
								"プレパイロンが1つのとき(ルートの2ばんめがスタートパイロンのとき)は, プレパイロンに\"1\"をせっていします.  <>"
#define MENU_TASK_START_DETAIL5	"スタートアラームをせっていします.  " \
								"\"なし\":アラームをならしません. " \
								"\"あり\":アラームをならします. (30min. 10min. 5min. 1min. スタート)  <>"
#define MENU_TASK_START_DETAIL6	"ルートのせんとうのパイロンのモードをせんたくします. " \
								"\"タスクパイロン\":1stパイロンをタスクにふくめます. " \
								"\"テイクオフ\":1stパイロンはタスクにふくめません. (スキップします)  <>"

const MenuItem MENU_TASK_START[] = {
	{"タイプ",				MENU_TASK_START_DETAIL,				MENU_SEL_ENUM,	&TASK_START_LIST},
	{"スタート",			MENU_TASK_START_DETAIL2,			MENU_SEL_TIME,	&TASK_START_TIME},
	{"1stパイロン",			MENU_TASK_START_DETAIL6,			MENU_SEL_ENUM,	&TASK_START_TO},
	{"プレパイロン",		MENU_TASK_START_DETAIL4,			MENU_SEL_VAL,	&TASK_PRE_PYLON},
	{"アラーム",			MENU_TASK_START_DETAIL5,			MENU_SEL_ENUM,	&TASK_START_ALARM},
	{0, 0,														MENU_ID_TASK}
};

///////////////////////////////////////////////////////////////////////////////
// ルートメニュー

#define MENU_ALL_CLEAR "すべてのルートとウェイポイントをさくじょします. <>"

#define HELP_ROUTE_TX	"クロスケーブルでせつぞくされたパラナビに,ルートをてんそうします. <>"

const MenuItem MENU_ROUTE[] = {
	{"さくせい",			"あたらしいルートを つくります",	MENU_ID_NEW_ROUTE},
	{"へんこう",			"ルートを へんこうします",			MENU_FCALL,		MP_ChangeRoute},
	{"さくじょ",			"ルートを さくじょします",			MENU_FCALL,		MP_DelRoute},
	{"コピー",				"ルートを コピーします",			MENU_FCALL,		MP_CopyRoute},
	{"ダウンロード",		"GPSからダウンロードします",		MENU_FCALL,		MP_DownloadRoute},
	{"てんそう",			HELP_ROUTE_TX,						MENU_FCALL,		MP_SendRoute},
	{"クリア",				"すべてのルートをさくじょします",	MENU_FCALL,		MP_ClearRoute},
	{"フルクリア",			MENU_ALL_CLEAR,						MENU_FCALL,		MP_AllClear},
	{0, 0,														MENU_ID_MAIN}
};

///////////////////////////////////////////////////////////////////////////////
// ルート作成、変更
const NameInputTemplate ROUTE_NAME = {
	ROUTE_NAMESIZE, "ルートのなまえ",
	(u8*)IW_OFFSET(route_input.name[0])
};

#define HELP_ADD_ROUTE	"このルートをとうろくします. " \
						"とうろくしないでBボタンをおすと,ルートのついかはキャンセルされます.  <>"

const MenuItem MENU_NEW_ROUTE[] = {
	{"なまえ",				"ルートの なまえを きめます",		MENU_SEL_NAME,	&ROUTE_NAME},
	{"WPTリスト",			"WPTリストを さくせいします",		MENU_SEL_RTWPT,	0},
//	{"とうろく",			HELP_ADD_ROUTE,						MENU_FCALL,		MP_AddRoute},
	{"とりけし",			"ルートついかをキャンセルします",	MENU_FCALL,		MP_AddRouteCancel},
	{0, 0,														MENU_FCALL,		MP_AddRoute},
};
#define HELP_CHANGE_ROUTE "このあたいでルートをへんこうします. " \
							"けっていしないでBボタンをおすと,へんこうはキャンセルされます.  <>"

const MenuItem MENU_CHANGE_ROUTE[] = {
	{"なまえ",				"なまえを へんこう します",			MENU_SEL_NAME,	&ROUTE_NAME},
	{"WPTリスト",			"WPTリストを へんこう します",		MENU_SEL_RTWPT,	0},
//	{"けってい",			HELP_CHANGE_ROUTE,					MENU_FCALL,		MP_ChangeRouteSubmit},
	{"とりけし",			"へんこうをキャンセルします",		MENU_FCALL,		MP_ChangeRouteCancel},
	{0, 0,														MENU_FCALL,		MP_ChangeRouteSubmit},
};

///////////////////////////////////////////////////////////////////////////////

#define HELP_WPT1	"ランディグとなるウェイポイントを せんたくします. " \
					"ランディングとして せっていしたウェイポイントは,フリーフライトモードのゴールパイロンになります.  <>"

// ウェイポイントメニュー
const MenuItem MENU_WAYPOINT[] = {
	{"さくせい",			"あたらしいWPTを つくります",		MENU_ID_NEW_WPT},
	{"へんこう",			"WPTを へんこうします",				MENU_FCALL,		MP_ChangeWpt},
	{"さくじょ",			"WPTを さくじょします",				MENU_FCALL,		MP_DelWpt},
	{"コピー",				"WPTを コピーします",				MENU_FCALL,		MP_CopyWpt},
	{"ランディング",		HELP_WPT1,							MENU_FCALL,		MP_LDWpt},
	{"ダウンロード",		"GPSからWPTをダウンロードします",	MENU_FCALL,		MP_DownloadWpt},
	{"げんざいち",			"げんざいちをWPTについかします",	MENU_FCALL,		MP_CurWpt},
	{"クリア",				"ふようなWPTを さくじょします",		MENU_FCALL,		MP_ClearWpt},
//	{"フルクリア",			MENU_ALL_CLEAR,						MENU_FCALL,		MP_AllClear},
	{0, 0,														MENU_ID_MAIN}
};

///////////////////////////////////////////////////////////////////////////////
// ウェイポイント作成メニュー
const NameInputTemplate WPT_NAME = {
	WPT_NAMESIZE, "ウェイポイントのなまえ",
	(u8*)IW_OFFSET(wpt_input.name[0])
};
// 緯度経度
const LatInputTemplate WPT_LAT = {
	1,
	IW_OFFSET(wpt_input.lat)
};
const LatInputTemplate WPT_LON = {
	0,
	IW_OFFSET(wpt_input.lon)
};
const IntInputTemplate WPT_ALT = {
	4, 0, 0, 0, 9999,
	"WPTこうど",
	"m",
	IW_OFFSET(wpt_input.alt)
};
#define HELP_ADD_WPT "このウェイポイントをとうろくします. " \
					"とうろくしないでBボタンをおすと,ウェイポイントのついかはキャンセルされます.  <>"

const MenuItem MENU_NEW_WPT[] = {
	{"なまえ",				"WPTの なまえを きめます",			MENU_SEL_NAME,	&WPT_NAME},
	{"いど",				"いどを にゅうりょくします",		MENU_SEL_LAT,	&WPT_LAT},
	{"けいど",				"けいどを にゅうりょくします",		MENU_SEL_LAT,	&WPT_LON},
	{"こうど",				"こうどを にゅうりょくします",		MENU_SEL_VAL,	&WPT_ALT},
//	{"とうろく",			HELP_ADD_WPT,						MENU_FCALL,		MP_AddWpt},
	{"とりけし",			"WPTとうろくをキャンセルします",	MENU_FCALL,		MP_AddWptCancel},
	{0, 0,														MENU_FCALL,		MP_AddWpt}
};

#define HELP_CHANGE_WPT "このあたいでウェイポイントをへんこうします. " \
						"けっていしないでBボタンをおすと,へんこうはキャンセルされます.  <>"

const MenuItem MENU_CHANGE_WPT[] = {
	{"なまえ",				"なまえを へんこうします",			MENU_SEL_NAME,	&WPT_NAME},
	{"いど",				"いどを へんこうします",			MENU_SEL_LAT,	&WPT_LAT},
	{"けいど",				"けいどを へんこうします",			MENU_SEL_LAT,	&WPT_LON},
	{"こうど",				"こうどを へんこうします",			MENU_SEL_VAL,	&WPT_ALT},
//	{"けってい",			HELP_CHANGE_WPT,					MENU_FCALL,		MP_ChangeWptSubmit},
	{"とりけし",			"WPTへんこうをキャンセルします",	MENU_FCALL,		MP_ChangeWptCancel},
	{0, 0,														MENU_FCALL,		MP_ChangeWptSubmit},
};

///////////////////////////////////////////////////////////////////////////////
// 設定メニュー

const MenuItem MENU_CONFIG[] = {
	{"GPSコンフィグ",		"GPSパラメータをせっていします",	MENU_ID_CONFIG_GPS},
	{"ナビコンフィグ",		"ナビモードのせっていをします",		MENU_ID_CONFIG_NAVI},
	{"Ext.デバイス",		"Ext.デバイスをせっていをします",	MENU_ID_CONFIG_EXTDEVICE},
	{"がめん",				"がめんの せっていをします",		MENU_ID_DISPLAY},
	{"おんせい",			"ナビのおんせいをせっていします",	MENU_ID_VOICE},
	{"バリオモドキ",		"バリオモドキを せっていします",	MENU_ID_CONFIG_VARIO},
	{"アシストモード",		"アシストモードをせっていします",	MENU_ID_CONFIG_ETC},
	{"しょきか",			"せっていを しょきかします",		MENU_FCALL,		MP_Reload},
	{0, 0,														MENU_ID_MAIN}
};

///////////////////////////////////////////////////////////////////////////////
// GPS設定

const IntInputTemplate GPS_N_CALIB = {
	3, 1, 0, -180, 180,
	"ほせい かくど",
	"°",
	IW_OFFSET(tc.n_calib)
};
const IntInputTemplate GPS_TZONE = {
	3, 1, 0, -720, 720,
	"タイムゾーン",
	"min",
	IW_OFFSET(tc.tzone_m)
};
const IntInputTemplate GPS_STOP_DIR = {
	4, 0, 3, 0, 9999,
	"ゆうこう そくど",
	"m/s",
	IW_OFFSET(tc.stop_dir)
};
const u8* const GPS_ALT[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.alt_type),
	"かいばつ",
	"WGS84だえん",
};
const u8* const GPS_CALC[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.calc_type),
	"じどう",
	"こうそく",
	"こうせいど",
};
const u8* const GPS_TALLY_CLEAR[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.tally_clr),
	"マニュアル",
	"サスペンド",
	"テイクオフ",
};
const IntInputTemplate ETC_AUTOOFF = {
	3, 0, 0, 0, 999,
	"オートオフ",
	"min",
	IW_OFFSET(tc.auto_off)
};

const u8* const GPS_NMEA_UP[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.nmea_up),
	"こうどさ",
	"ハーフ",
	"クォータ",
};
const u8* const NAVI_VIEW_SBAS[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.waas_flag),
	"むこう",
	"ゆうこう",
};

#define HELP_CONFIG_GPS1	"Nほうこうを ほせいします. じしゃくNは W007(-7°)です.  <>"
#define HELP_CONFIG_GPS2	"タイムゾーンを せっていします. JSTは +540min (+9 Hour) です.  " \
							"サマータイムも,ここでせっていしてください.  <>"
#define HELP_CONFIG_GPS3	"せっていしたスピードをこえると,コンパスけいさんが ゆうこうになります.  <>"
#define HELP_CONFIG_GPS4	"こうどの さんしゅつほうほうを えらびます.  <>"
#define HELP_CONFIG_GPS5	"きょり/ほういアルゴリズムを えらびます.  "\
							"\"じどう\":アルゴリズムを じどうで せんたくします.  "\
							"\"こうそく\":どくじアルゴリズムをつかいます.  "\
							"\"こうせいど\":ヒューベニかくちょうしきをつかいます.(おそい!)  <>"
#define HELP_CONFIG_GPS6	"フライトログのクリアほうほうを えらびます.  " \
							"\"マニュアル\":フライトログビューでSTARTボタンをおすと,ログをクリアします.  " \
							"\"サスペンド\":サスペンドすると,じどうでログをクリアします.  " \
							"\"テイクオフ\":テイクオフすると,じどうでログをクリアします.  <>"
#define HELP_CONFIG_GPS7	"GPSつうしんやボタンそうさがないときに,せっていしたじかんでサスペンドします. " \
							"0minをせっていすると,じどうサスペンドしません. " \
							"リジュームするときには,L+Rボタンをおします.  <>"
#define HELP_CONFIG_GPS8	"NMEAのバリオけいさんほうほうを えらびます. " \
							"\"こうどさ\":こうどさを そのままバリオにつかいます. " \
							"\"ハーフ\":こうどさの さぶんを 1/2にならして けいさんします. " \
							"\"クォータ\":こうどさの さぶんを 1/4にならして けいさんします.  <>"

#define HELP_CONFIG_GPS9	"WAASやMSASなどのSBASをしようするか えらびます. " \
							"(このせっていは,せいどマーカーのひょうじをきりかえるために つかいます.)  " \
							"\"むこう\":ごさにおうじて,せいどマーカーが へんかします.  " \
							"\"ゆうこう\":SBASのじゅしんにおうじて,せいどマーカーが へんかします.  <>"

const MenuItem MENU_CONFIG_GPS[] = {
	{"Nほせい",				HELP_CONFIG_GPS1,					MENU_SEL_VAL,	&GPS_N_CALIB},
	{"タイムゾーン",		HELP_CONFIG_GPS2,					MENU_SEL_VAL,	&GPS_TZONE},
	{"ぎじコンパス",		HELP_CONFIG_GPS3,					MENU_SEL_VAL,	&GPS_STOP_DIR},
	{"こうど",				HELP_CONFIG_GPS4,					MENU_SEL_ENUM,	&GPS_ALT},
	{"けいさん",			HELP_CONFIG_GPS5,					MENU_SEL_ENUM,	&GPS_CALC},
	{"NMEAバリオ",			HELP_CONFIG_GPS8,					MENU_SEL_ENUM,	&GPS_NMEA_UP},
	{"オートオフ",			HELP_CONFIG_GPS7,					MENU_SEL_VAL,	&ETC_AUTOOFF},
	{0, 0,														MENU_ID_CONFIG}
};

///////////////////////////////////////////////////////////////////////////////
// ナビ設定メニュー

const IntInputTemplate NAVI_NEAR = {
	4, 0, 0, 0, 9999,
	"ニアサウンド",
	"m",
	IW_OFFSET(tc.near_beep)
};
const u8* const NAVI_VIEW_LOCK[] = {
	(u8*)4,
	(u8*)IW_OFFSET(tc.auto_lock),
	"マニュアル/Msg",
	"テイクオフ/Msg",
	"マニュアル",
	"テイクオフ",
};
const IntInputTemplate NAVI_LD = {
	5, 0, 3, 1, 99999,
	"グライダL/D",
	"",
	IW_OFFSET(tc.my_ldK)
};
const IntInputTemplate NAVI_DOWNRATION = {
	4, 0, 3, 1, 9999,
	"シンクレート",
	"m/s",
	IW_OFFSET(tc.my_down)
};
const u8* const NAVI_AVG_TYPE[] = {
	(u8*)6,
	(u8*)IW_OFFSET(tc.avg_type),
	"じどう",
	"グライダこてい",
	"リアルタイム",
	"3s へいきん",
	"10s へいきん",
	"30s へいきん",
};
const u8* const NAVI_VOLUME[] = {
	(u8*)8,
	(u8*)IW_OFFSET(tc.vol_key),
	"1:\\",
	"2:\\\\",
	"3:\\\\\\",
	"4:\\\\\\\\",
	"5:\\\\\\\\\\",
	"6:\\\\\\\\\\\\",
	"7:\\\\\\\\\\\\\\",
	"8:\\\\\\\\\\\\\\\\",
};

const u8* const NAVI_VIEW_ALT[] = {
	(u8*)6,
	(u8*)IW_OFFSET(tc.view_alt),
	"ArrivalAlt(a)",
	"ArrivalDiff(@)",
	"Diff(d)",
	"GoalDiff(g)",
	"Next L/D(NL)",
	"Goal L/D(GL)",
};

const u8* const NAVI_VIEW_WARN[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.gps_warn),
	"なし",
	"あり",
};

#define HELP_CONFIG_NAVI1	"シリンダせっきんじの,ニアビープをならすきょりを せっていします.  <>"
#define HELP_CONFIG_NAVI2	"とうちゃくけいさん(よそうじこく,よそうこうど)の,アルゴリズムをえらびます.  " \
							"\"じどう\":じどうでアルゴリズムをせんたくします." \
								"(センタリングじに <グライダこてい> をつかい, ほかは <nへいきん> をつかいます)  " \
							"\"グライダこてい\":あらかじめインプットされた, <L/D> と <Sink rate> から " \
								"みつもりをおこないます.  " \
							"\"リアルタイム\":つねに さいしんデータをつかって,みつもりをおこないます.  " \
							"\"nへいきん\":かこnかいのデータをへいきんして,みつもりします.  <>"
#define HELP_CONFIG_NAVI3	"あなたのグライダの L/D(レースセッティング)を せっていします. " \
							"みつもりが <じどう> または <グライダこてい> のときに しようします.  <>"
#define HELP_CONFIG_NAVI4	"あなたのグライダの Sink rate(レースセッティング)を せっていします. " \
							"みつもりが <じどう> または <グライダこてい> のときに しようします.  <>"
#define HELP_CONFIG_NAVI5	"L/DとSinkRateのチェックのために,グライダの たいきそくどを ひょうじしています. " \
							"< Airspeed(km/h) = 3.6 * SinkRate(m/s) * L/D >.  <>"
#define HELP_CONFIG_NAVI6	"キーロックのほうほうをえらびます. " \
							"\"マニュアル/Msg\":ナビがめんでSTART+SELECTをおすと,キーロックします. " \
								"ロックちゅうにキーをおすと, アンロックのほうほうをひょうじします.  " \
							"\"テイクオフ/Msg\":ナビがめんでテイクオフすると,じどうでキーロックします. " \
								"ロックちゅうにキーをおすと ,アンロックのほうほうをひょうじします.  " \
							"\"マニュアル\":ナビがめんでテイクオフすると,じどうでキーロックします. " \
								"ロックちゅうにキーをおしても, なにもしません.  " \
							"\"テイクオフ\":ナビがめんでテイクオフすると,じどうでキーロックします. " \
								"ロックちゅうにキーをおしても, なにもしません.  " \
							"(キーロックをかいじょするときには,START+SELECTをおします)  <>"
#define HELP_CONFIG_NAVI7	"ニアサウンドやキークリックなどの,おんりょうをせっていします.   <>"
#define HELP_CONFIG_NAVI8	"つぎパイロンのこうどじょうほうの,ひょうじないようを えらびます.  " \
							"\"ArrivalAlt(a)\":つぎパイロンのよそうとうちゃくこうどを ひょうじします.  " \
							"\"ArrivalDiff(@)\":つぎパイロンのよそうとうちゃくこうどと, " \
								"げんざいこうどの こうどさを ひょうじします.  " \
							"\"Diff(d)\":つぎパイロンのこうどと, げんざいこうどの こうどさを ひょうじします.  " \
							"\"Goal(g)\":ゴールパイロンの よそうとうちゃく こうどさを ひょうじします. " \
							"\"Next L/D\":つぎパイロンまでのL/Dを ひょうじします. " \
							"\"Goal L/D\":ゴールパイロンまでのL/Dを ひょうじします. " \
							"(ようそうこうどのアルゴリズムは,\"みつもり\"で せっていできます.)  <>"
#define HELP_CONFIG_NAVI9	"GPSエラーのけいこくひょうじをえらびます.  " \
							"\"なし\":ポップアップエラーをひょうじしません. " \
							"\"あり\":ポップアップでエラーをひょうじします.  <>"

const MenuItem MENU_CONFIG_NAVI[] = {
	{"キーロック",			HELP_CONFIG_NAVI6,					MENU_SEL_ENUM,	&NAVI_VIEW_LOCK},
	{"ニアサウンド",		HELP_CONFIG_NAVI1,					MENU_SEL_VAL,	&NAVI_NEAR},
	{"ボリューム",			HELP_CONFIG_NAVI7,					MENU_SEL_ENUM,	&NAVI_VOLUME},
	{"みつもり",			HELP_CONFIG_NAVI2,					MENU_SEL_ENUM,	&NAVI_AVG_TYPE},
	{"ひょうじ2",			HELP_CONFIG_NAVI8,					MENU_SEL_ENUM,	&NAVI_VIEW_ALT},
	{"-L/D",				HELP_CONFIG_NAVI3,					MENU_SEL_VAL,	&NAVI_LD},
	{"-Sink rate",			HELP_CONFIG_NAVI4,					MENU_SEL_VAL,	&NAVI_DOWNRATION},
//	{" (Airspeed)",			HELP_CONFIG_NAVI5,					MENU_SEL_SPEED},
	{"GPSけいこく",			HELP_CONFIG_NAVI9,					MENU_SEL_ENUM,	&NAVI_VIEW_WARN},
	{0, 0,														MENU_ID_CONFIG}
};

///////////////////////////////////////////////////////////////////////////////
// 外部デバイスメニュー

const MenuItem MENU_CONFIG_EXTDEVICE[] = {
	{"Bluetooth",			"Bluetoothユニットのせってい",		MENU_ID_BLUETOOTH},
	{"アネモメータ",		"アネモメータのせってい",			MENU_ID_CONFIG_ANEMOMETER},
	{0, 0,														MENU_ID_CONFIG}
};

///////////////////////////////////////////////////////////////////////////////
// Bluetooth設定メニュー
const u8* const BT_MODE[] = {
	(u8*)5,
	(u8*)IW_OFFSET(tc.bt_mode),

	// 0: Bluetoothなし。GPS直結
	"なし",

	// 1-4: Parani-ESD 38.4/57.6/115/9.6のみ選択可(bit演算するため順番重要)
	//      実際は、Parani-ESDに2KBバッファがあるのでNMEAデータ量程度なら
	//      どのボーレートでもOK。(消費電流を考えたら9.6Kbps、PCと通信するなら115k)
	"P-ESD 38.4K",
	"P-ESD 57.6K",
	"P-ESD 115K",
	"P-ESD 9.6K",

	// 5-: RFU
};

const NameInputTemplate BT_ADDR = {
	BT_ADDR_LEN, "せつぞくアドレス",
	(u8*)IW_OFFSET(tc.bt_addr)
};
const NameInputTemplate BT_PIN = {
	BT_PIN_LEN, "せつぞくPIN",
	(u8*)IW_OFFSET(tc.bt_pin)
};

#define HELP_BLUETOOTH1	"Bluetoothユニットとの つうしんモードをえらびます. Bluetoothをつかわないときには,\"なし\"をえらびます.  <>"
#define HELP_BLUETOOTH2	"ペアリングするBluetoothデバイスのアドレスをしていします.  " \
						"\"BTサーチ\"をつかうと,ちかくのデバイスをサーチして リストからGPSやPCをせんたくできます.  <>"
#define HELP_BLUETOOTH3	"BTアドレスとPINコードをしようして, GPSやPCにせつぞくします. " \
						"せつぞくに せいこうすると, じどうペアリングモードにきりかわります.  <>"
#define HELP_BLUETOOTH4	"GPSとのペアリングやPCまちうけモードをかいじょし, オフラインモードにします. (しょうエネ)  <>"

const MenuItem MENU_BLUETOOTH[] = {
	{"モード",				HELP_BLUETOOTH1,					MENU_SEL_ENUM,	&BT_MODE},
	{"BTアドレス",			HELP_BLUETOOTH2,					MENU_SEL_NAME,	&BT_ADDR},
	{"PINコード",			"せつぞくPINをせっていします",		MENU_SEL_NAME,	&BT_PIN},
	{"BTサーチ",			"ちかくのデバイスをさがします",		MENU_FCALL,		MP_BtInq},
	{"ペアリング",			HELP_BLUETOOTH3,					MENU_FCALL,		MP_BtConnect},
	{"PCまちうけ",			"まちうけモードにします",			MENU_FCALL,		MP_BtScan},
	{"オフライン",			HELP_BLUETOOTH4,					MENU_FCALL,		MP_BtDisconnect},
	{0, 0,														MENU_ID_CONFIG_EXTDEVICE}
};

///////////////////////////////////////////////////////////////////////////////
// アネモメータ設定メニュー

const IntInputTemplate ANEMO_PARAM = {
	9, 0, 0, 0, 999999999,
	"けいすう",
	"",
	IW_OFFSET(tc.anemo_coef)
};

const u8* const ANEMO_SPDUNIT[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.anemo_unit),
	"m/s",
	"km/h",
};

#define HELP_CONFIG_ANEMO1	"アネモメータのスピードけいすうを,マニュアルでせっていします. " \
							"アネモメータが, ふうそく V m/s のときに まいびょう N パルス をカウントするときには,つぎのけいすうを せっていします. " \
							"けいすう = N * 1638400 / V  <>"
#define HELP_CONFIG_ANEMO2	"アネモメータのスピードけいすうを,リファレンスデバイスをつかって せっていします. " \
							"Aボタンをおすと ほせいがスタートし, さいごににリファレンスふうそくを にゅうりょくします. " \
							"リファレンスほせいちゅうは, ていじょうふうを ながしてください.  <>"
#define HELP_CONFIG_ANEMO3	"アネモメータのスピードけいすうを,GPSをつかって せっていします. " \
							"GPSをせつぞくしてAボタンをおすと,ほせいが はじまります. " \
							"GPSほせいは, むふうじょうたいのときに いっていそくどでいどうしながら おこないます.  <>"
#define HELP_CONFIG_ANEMO4	"エアスピードの そくどたんいをえらびます.  " \
							"\"m/s\":びょうそく N メートルを つかいます.  " \
							"\"km/h\":じそく N メートルを つかいます.  <>"

const MenuItem MENU_CONFIG_ANEMOMETER[] = {
	{"そくどたんい",	HELP_CONFIG_ANEMO4,					MENU_SEL_ENUM,	&ANEMO_SPDUNIT},
	{"ほせい",			HELP_CONFIG_ANEMO1,					MENU_SEL_VAL,	&ANEMO_PARAM},
	{"ほせい(2)     (Reference)",	HELP_CONFIG_ANEMO2,					MENU_FCALL,		MP_WCALIB2},
	{"ほせい(3)     (GPS)",	HELP_CONFIG_ANEMO3,					MENU_FCALL,		MP_WCALIB3},
	{"テスト",			"アネモメータをテストします",		MENU_FCALL,		MP_Anemometer},
	{0, 0,													MENU_ID_CONFIG_EXTDEVICE}
};

///////////////////////////////////////////////////////////////////////////////
// ディスプレイメニュー
const u8* const DISP_PYLON[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.pylon_type),
	"フルネーム",
	"リスト",
	"トグル",
};
const IntInputTemplate DISP_INITIAL_POS = {
	2, 0, 0, 1, 10,
	"もじ いち",
	"ばん",
	IW_OFFSET(tc.initial_pos)
};
const IntInputTemplate DISP_PMAX = {
	4, 1, 0, -9999, 9999,
	"パイロンアローMax",
	"m",
	IW_OFFSET(tc.ar_max)
};
const IntInputTemplate DISP_PMIN = {
	4, 1, 0, -9999, 9999,
	"パイロンアローMin",
	"m",
	IW_OFFSET(tc.ar_min)
};
const IntInputTemplate DISP_SELF_R = {
	4, 0, 0, 0, 9999,
	"ポジション(r)",
	"m",
	IW_OFFSET(tc.self_r)
};

/* ナビメニューのB+カーソルで切り替えられるので、カット
 * TIPSにも表示しているし…
const u8* const NAVI_VIEW_MODE[] = { // MAX_VIEW_MODE
	(u8*)(MAX_VIEW_MODE - 4),
	(u8*)IW_OFFSET(tc.view_mode),
	"スタンダード",
	"スタンダードR1",
	"スタンダードR2",
	"スタンダードR3",
	"シンプル",
	"シンプル    R1",
	"シンプル    R2",
	"シンプル    R3",
	"ターゲット",
	"ターゲット  R1",
	"ターゲット  R2",
	"ターゲット  R3",
	"パイロンx2",
	"パイロンx2  R1",
	"パイロンx2  R2",
	"パイロンx2  R3",
	"ウインド",
	"ウインド    R1",
	"ウインド    R2",
	"ウインド    R3",
	"*CylinderMap",
	"*CylinderMapR1",
	"*CylinderMapR2",
	"*CylinderMapR3",
	"*Locus",
	"*Locus      R1",
	"*Locus      R2",
	"*Locus      R3",
	"テキスト",
	"テキスト    R1",
	"テキスト    R2",
	"テキスト    R3",
};
*/
const u8* const NAVI_SPDUNIT[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.spd_unit),
	"m/s",
	"km/h",
};

#define HELP_DISP1	"タスクちゅうのパイロンめいの ひょうじほうほうをえらびます.  " \
					"\"フルネーム\":つぎパイロンのなまえを フルネームで ひょうじします.  " \
					"\"リスト\":つぎパイロンからの なまえの1もじを れっきょします. " \
					"\"トグル\":フルネームとリストを こうごにひょうじします.  <>"
#define HELP_DISP6	"パイロンをリストひょうじするばあいの, ひょうじする もじいちを せっていします. " \
					"\"1\"をせっていするとウェイポイントめいのイニシャルをつかい, \"10\"をせっていすると なまえのさいごのもじを つかいます. " \
					"このせっていは \"パイロン\"が\"フルネーム\"のときには つかいません.  <>"
#define HELP_DISP2	"がめんの あかるさを せっていします.  デフォルトはレベル2です.  <>"
#define HELP_DISP3	"つぎパイロンへの よそうとうちゃくこうどさが \"アローMax\" より おおきいとき, " \
					"パイロンアローのながさが さいだいに なります.  <>"
#define HELP_DISP4	"つぎパイロンへの よそうとうちゃくこうどさが \"アローMin\" より ちいさいとき, " \
					"パイロンアローのながさが さいしょうに なります.  <>"
#define HELP_DISP5	"げんざいちの ひょうじスケール(はんけい)を せっていします. " \
					"0mのときには,タスクせっていのセクタ(r)をつかいます.  <>"
#define HELP_CONFIG_NAVI0	"ビュータイプをきりかえます. なまえのさいごに R がついているものは, かいてんします. " \
							"(ナビがめんで B+カーソル をおすと,ちょくせつビュータイプをきりかえられます)  <>"
#define HELP_CONFIG_NAVI8	"グラウンドスピードの そくどたんいをえらびます.  " \
							"\"m/s\":びょうそく N メートルを つかいます.  " \
							"\"km/h\":じそく N キロメートルを つかいます.  " \
							"(せっていは, ボイスナビにも はんえいします)  <>"

const MenuItem MENU_DISPLAY[] = {
//	{"ビュータイプ",	HELP_CONFIG_NAVI0,					MENU_SEL_ENUM,	&NAVI_VIEW_MODE},
	{"そくどたんい",	HELP_CONFIG_NAVI8,					MENU_SEL_ENUM,	&NAVI_SPDUNIT},
	{"パイロン",		HELP_DISP1,							MENU_SEL_ENUM,	&DISP_PYLON},
	{"もじ いち",		HELP_DISP6,							MENU_SEL_VAL,	&DISP_INITIAL_POS},
	{"SBAS",			HELP_CONFIG_GPS9,					MENU_SEL_ENUM,	&NAVI_VIEW_SBAS}, // マーカ表示設定なので"がめん"に移動
	{"アローMax",		HELP_DISP3,							MENU_SEL_VAL,	&DISP_PMAX},
	{"アローMin",		HELP_DISP4,							MENU_SEL_VAL,	&DISP_PMIN},
	{"ポジション(r)",	HELP_DISP5,							MENU_SEL_VAL,	&DISP_SELF_R},
	{"ガンマほせい",	HELP_DISP2,							MENU_SEL_PALETTE},
	{0, 0,													MENU_ID_CONFIG}
};

///////////////////////////////////////////////////////////////////////////////
// 音声設定メニュー
#define VOICE_SIZE	11
#define VOICE_SET	"なし", "ときどきアルチ", "アルチ&リフト",  "パイロンのみ",  "ノーマルA",  "シンプルA",  \
							"アルチ+",		  "アルチ&リフト+", "パイロンのみ+", "ノーマルA+", "シンプルA+"
							

const u8* const VOICE_ENABLE[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.vm_enable),
	"むこう",
	"ゆうこう",
};
const u8* const VOICE_WAIT[] = {
	(u8*)VOICE_SIZE,
	(u8*)IW_OFFSET(tc.vm_wait),
	VOICE_SET
};
const u8* const VOICE_NORMAL[] = {
	(u8*)VOICE_SIZE,
	(u8*)IW_OFFSET(tc.vm_normal),
	VOICE_SET
};
const u8* const VOICE_STOP[] = {
	(u8*)VOICE_SIZE,
	(u8*)IW_OFFSET(tc.vm_stop),
	VOICE_SET
};
const u8* const VOICE_CENTER[] = {
	(u8*)VOICE_SIZE,
	(u8*)IW_OFFSET(tc.vm_center),
	VOICE_SET
};
const u8* const VOICE_NEAR[] = {
	(u8*)VOICE_SIZE,
	(u8*)IW_OFFSET(tc.vm_near),
	VOICE_SET
};

const IntInputTemplate VOICE_INTERVAL = {
	4, 0, 0, 0, 9999,
	"インターバル",
	"s",
	IW_OFFSET(tc.vm_interval)
};

const IntInputTemplate ETC_CYL_NEAR = {
	5, 0, 0, 0, 99999,
	"ニアシリンダ",
	"m",
	IW_OFFSET(tc.cyl_near)
};

#define HELP_VOICE1	"おんせいナビのモードをえらびます.  " \
					"\"むこう\":おんせいナビをつかいません.   " \
					"\"ゆうこう\":おんせいナビをつかいます.  <>"
#define HELP_VOICE2	"おんせいナビのインターバルを せっていします. " \
					"このせっていは, おんせいタイプのさいごに'+'マークがついているものに てきようされます.  <>"
#define HELP_VOICE4	"ニアシリンダのきょりをせっていします. このせっていはCylinder modeのニアシリンダと きょうつうです. " \
					"シリンダが100mのときに ニアシリンダを200mにせっていすると, パイロンから300mいないをニアシリンダとはんだんします.  <>"

const MenuItem MENU_VOICE[] = {
	{"おんせいナビ",	HELP_VOICE1,						MENU_SEL_ENUM,	&VOICE_ENABLE},
	{"インターバル",	HELP_VOICE2,						MENU_SEL_VAL,	&VOICE_INTERVAL},
	{"プレフライト",	"テイクオフまえの おんせいナビ",	MENU_SEL_ENUM,	&VOICE_WAIT},
	{"ノーマル",		"つうじょうじの おんせいナビ",		MENU_SEL_ENUM,	&VOICE_NORMAL},
	{"ストップ",		"ていしちゅうの おんせいナビ",		MENU_SEL_ENUM,	&VOICE_STOP},
	{"センタリング",	"センタリングちゅうの おんせい",	MENU_SEL_ENUM,	&VOICE_CENTER},
	{"ニアシリンダ",	"シリンダせっきんじの おんせい",	MENU_SEL_ENUM,	&VOICE_NEAR},
	{"ニアきょり",		HELP_VOICE4,						MENU_SEL_VAL,	&ETC_CYL_NEAR},
	{0, 0,													MENU_ID_CONFIG}
};

///////////////////////////////////////////////////////////////////////////////
// etc設定メニュー
#define HELP_ETC1 "Locus(きせき)モードをせっていします. じょうしょうりつを,ドットひょうじします. " \
					"サーマルコアをかくにんするのに,つかえるかもしれません...  <>"
#define HELP_ETC2 "ウインドチェック(ぎじふうそくけい)のせっていをします. " \
					"ふうそくと かざむきを そくていするためのモードです.  <>"
#define HELP_ETC3 "グライダのベンチマークモードをせっていします. " \
					"あなたのグライダの L/D, Sink Rate, Speed をそくていします.  <>"
#define HELP_ETC4 "ステートけんしゅつようパラメータをせっていします.  <>"
#define HELP_ETC5 "アシストモードでつかうパラメータをせっていします.  <>"
#define HELP_ETC6 "シリンダモードをせっていします. シリンダをマップでひょうじします.  <>"
#define HELP_ETC7 "オートターゲットモードをせっていします.  <>"

const MenuItem MENU_CONFIG_ETC[] = {
	{"Locus mode",			HELP_ETC1,							MENU_ID_CONFIG_THERMAL},
	{"Cylinder mode",		HELP_ETC6,							MENU_ID_CONFIG_CYL},
	{"WindCheck mode",		HELP_ETC2,							MENU_ID_CONFIG_WINDCHECK},
	{"Glider Benchmark",	HELP_ETC3,							MENU_ID_CONFIG_BENCHMARK},
	{"Auto target mode",	HELP_ETC7,							MENU_ID_CONFIG_AUTOTARGET},
	{"パラメータ1",			HELP_ETC4,							MENU_ID_CONFIG_PARAM},
	{"パラメータ2",			HELP_ETC5,							MENU_ID_CONFIG_PARAM2},
	{0, 0,														MENU_ID_CONFIG}
};

///////////////////////////////////////////////////////////////////////////////
// パラメータメニュー

const IntInputTemplate NAVI_PARAM_SPIRAL = {
	5, 0, 3, -99999, 0,
	"スパイラルスピード",
	"m/s",
	IW_OFFSET(tc.spiral_spd)
};
const IntInputTemplate NAVI_PARAM_STALL = {
	5, 0, 3, -99999, 0,
	"ストールスピード",
	"m/s",
	IW_OFFSET(tc.stall_spd)
};
const IntInputTemplate NAVI_PARAM_START = {
	5, 0, 3, 0, 99999,
	"テイクオフスピード",
	"m/s",
	IW_OFFSET(tc.start_spd)
};
const IntInputTemplate NAVI_PARAM_PITCH_DIFF = {
	5, 0, 3, 0, 99999,
	"ピッチDiff",
	"m/s",
	IW_OFFSET(tc.pitch_diff)
};
const IntInputTemplate NAVI_PARAM_PITCH_FREQ = {
	2, 0, 0, 1, 30,
	"ピッチングMAX",
	"s",
	IW_OFFSET(tc.pitch_freq)
};
const IntInputTemplate NAVI_PARAM_ROLL_DIFF = {
	3, 0, 0, 0, 360,
	"ロールDiff",
	"°",
	IW_OFFSET(tc.roll_diff)
};
const IntInputTemplate NAVI_PARAM_ROLL_FREQ = {
	2, 0, 0, 1, 30,
	"ローリングMAX",
	"s",
	IW_OFFSET(tc.roll_freq)
};

#define HELP_PARAM1 "スパイラルのシンクスピードをせっていします. " \
					"せんかいじに,このスピードでシンクすると,スパイラルとにんしきします. " \
					"(シンクでのセンタリングも,スパイラルとにんしきします!)  <>"
#define HELP_PARAM2 "ストールのシンクスピードをせっていします. " \
					"このスピードでおりると,ストールとにんしきします. " \
					"(つよいシンクも,ストールとにんしきします!)  <>"
#define HELP_PARAM3 "テイクオフスピードをせっていします. このスピードでうごくと,テイクオフとにんしきします.  <>"
#define HELP_PARAM4 "ピッチングチェックパラメータ. ピッチングけんしゅつする,たいちそくどの へんかりょう. " \
					"(ちゅうい:すいちょく そくどの へんかでは ありません!) <>"
#define HELP_PARAM5 "ピッチングしゅうきの さいだいじかん. " \
					"これよりおそくピッチをきりかえしても,ただのペースチェンジとにんしきして,ピッチングとみなしません.  <>"
#define HELP_PARAM6 "ローリングチェックパラメータ. ローリングけんしゅつする,へんか かくど.  " \
					"このかくどの れんぞくきりかえしを ローリングとみなします.  <>"
#define HELP_PARAM7 "ローリングしゅうきの さいだいじかん. " \
					"これよりおそくロールをきりかえしても,ただのターンと にんしきして,ローリングとみなしません.  <>"
const MenuItem MENU_CONFIG_PARAM[] = {
	{"スパイラル",			HELP_PARAM1,						MENU_SEL_VAL,	&NAVI_PARAM_SPIRAL},
	{"ストール",			HELP_PARAM2,						MENU_SEL_VAL,	&NAVI_PARAM_STALL},
	{"テイクオフ",			HELP_PARAM3,						MENU_SEL_VAL,	&NAVI_PARAM_START},
	{"ピッチDiff",			HELP_PARAM4,						MENU_SEL_VAL,	&NAVI_PARAM_PITCH_DIFF},
	{"ピッチFreq",			HELP_PARAM5,						MENU_SEL_VAL,	&NAVI_PARAM_PITCH_FREQ},
	{"ロールDiff",			HELP_PARAM6,						MENU_SEL_VAL,	&NAVI_PARAM_ROLL_DIFF},
	{"ロールFreq",			HELP_PARAM7,						MENU_SEL_VAL,	&NAVI_PARAM_ROLL_FREQ},
	{0, 0,														MENU_ID_CONFIG_ETC}
};

///////////////////////////////////////////////////////////////////////////////
// パラメータメニュー
const IntInputTemplate ETC_STABLE_SPEED = {
	5, 0, 3, 0, 99999,
	"そくどマージン",
	"m/s",
	IW_OFFSET(tc.stable_speed)
};
const IntInputTemplate ETC_STABLE_ANGLE = {
	3, 0, 0, 1, 360,
	"かくどマージン",
	"°",
	IW_OFFSET(tc.stable_angle)
};
const IntInputTemplate ETC_INIT_WAIT = {
	3, 0, 0, 0, 999,
	"しょきウェイト",
	"s",
	IW_OFFSET(tc.init_wait)
};
const IntInputTemplate ETC_WAIT_TIMEOUT = {
	3, 0, 0, 0, 999,
	"タイムアウト",
	"s",
	IW_OFFSET(tc.wait_timeout)
};
const IntInputTemplate ETC_COMP_TIMEOUT = {
	3, 0, 0, 0, 999,
	"ひょうじ じかん",
	"s",
	IW_OFFSET(tc.comp_timeout)
};
const IntInputTemplate ETC_KEEP_RANGE = {
	4, 0, 3, 0, 9999,
	"キープレンジ",
	"m/s",
	IW_OFFSET(tc.keep_range)
};

#define HELP_PARAM2_1 "WindCheck/Benchmarkでつかう,あんていかくどの パラメータをせっていします.  " \
						"しんこうほういが<Stable angle>よりもブレていると," \
						"WindCheck/Benchmarkのそくていをスタートしません.  <>"
#define HELP_PARAM2_2 "WindCheck/Benchmarkでつかう,あんていそくどの パラメータをせっていします.  " \
						"グラウンドスピードが<Stable speed>よりもブレていると," \
						"WindCheck/Benchmarkのそくていをスタートしません.  <>"
#define HELP_PARAM2_3 "WindCheck/Benchmarkモードへの きりかわりから,そくていかいしまでのウェイトをせっていします.  <>"
#define HELP_PARAM2_4 "WindCheck/Benchmarkの,あんていまちタイムアウトをせっていします. " \
						"タイムアウトじかんをこえても グライダがあんていしないときは, " \
						"じどうでノーマルビューにもどります.  <>"
#define HELP_PARAM2_5 "WindCheck/Benchmarkの,ひょうじタイムアウトをせっていします. " \
						"そくていコンプリートごの,けっかひょうじじかんを きめます. " \
						"タイムアウトすると,じどうでノーマルビューにもどります.  <>"
#define HELP_PARAM2_6 "リフトでもシンクでもない, レベルキープの じょうしょうりつレンジをせっていします. " \
						"このパラメータはフライトログのSoaring Statisticsにつかわれます.  <> "

const MenuItem MENU_CONFIG_PARAM2[] = {
	{"Stable angle",		HELP_PARAM2_1,						MENU_SEL_VAL,	&ETC_STABLE_ANGLE},
	{"Stable speed",		HELP_PARAM2_2,						MENU_SEL_VAL,	&ETC_STABLE_SPEED},
	{"Init wait",			HELP_PARAM2_3,						MENU_SEL_VAL,	&ETC_INIT_WAIT},
	{"Mode timeout",		HELP_PARAM2_4,						MENU_SEL_VAL,	&ETC_WAIT_TIMEOUT},
	{"Display time",		HELP_PARAM2_5,						MENU_SEL_VAL,	&ETC_COMP_TIMEOUT},
	{"Level keep",			HELP_PARAM2_6,						MENU_SEL_VAL,	&ETC_KEEP_RANGE},
	{0, 0,														MENU_ID_CONFIG_ETC}
};

///////////////////////////////////////////////////////////////////////////////
//オートターゲット

const u8* const ETC_AUTOTARGET_MODE[] = {
	(u8*)4,
	(u8*)IW_OFFSET(tc.at_mode),
	"オフ",
	"タイプ1",
	"タイプ2",
	"タイプ3",
};

const IntInputTemplate ETC_AUTOTARGET_MIN = {
	6, 0, 0, 0, 999999,
	"さいしょうち",
	"m",
	IW_OFFSET(tc.at_min)
};
const IntInputTemplate ETC_AUTOTARGET_MAX = {
	6, 0, 0, 0, 999999,
	"さいだいち",
	"m",
	IW_OFFSET(tc.at_max)
};
const IntInputTemplate ETC_AUTOTARGET_RECHECK = {
	6, 0, 0, 0, 999999,
	"さいけんさく",
	"m",
	IW_OFFSET(tc.at_recheck)
};

#define HELP_AUTOTARGET1 "フリーフライトモードの ターゲットせっていほうほうを えらびます. " \
						"\"オフ\":オートターゲットをつかいません.  " \
						"\"タイプ1\": ターゲットシリンダにはいると, じどうで つぎのターゲットをせっていします. " \
						"\"タイプ2\": せっていレンジにウェイポイントがないときに, レンジにちかいウェイポイントをターゲットにします. " \
						"\"タイプ3\": タイプ2にくわえて,ローリングx3 をおこなうと あたらしいターゲットをせっていします.  <>"

#define HELP_AUTOTARGET2 "オートターゲットは \"シリンダサイズ\"+\"さいしょうち\" よりも とおいウェイポイントからえらびます.  <>"
#define HELP_AUTOTARGET3 "オートターゲットは \"さいだいち\" よりも ちかいウェイポイントからえらびます.  <>"
#define HELP_AUTOTARGET4 "ターゲットが\"さいけんさく\"よりとおくなると,さいどサーチをおこないます. 0をセットするとさいけんさくしません. (60sごとにチェック) <>"

const MenuItem MENU_CONFIG_AUTOTARGET[] = {
	{"モード",				HELP_AUTOTARGET1,					MENU_SEL_ENUM,	&ETC_AUTOTARGET_MODE},
	{"さいしょうち",		HELP_AUTOTARGET2,					MENU_SEL_VAL,	&ETC_AUTOTARGET_MIN},
	{"さいだいち",			HELP_AUTOTARGET3,					MENU_SEL_VAL,	&ETC_AUTOTARGET_MAX},
	{"さいけんさく",		HELP_AUTOTARGET4,					MENU_SEL_VAL,	&ETC_AUTOTARGET_RECHECK},
	{0, 0,														MENU_ID_CONFIG_ETC}
};

///////////////////////////////////////////////////////////////////////////////
// リフトサーチモード
const u8* const ETC_THERMAL_CMD[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.thermal_cmd),
	"B+Aボタン",
	"センタリング",
};
const IntInputTemplate NAVI_LOCUS_R = {
	4, 0, 0, 1, 9999,
	"ひょうじスケール",
	"m",
	IW_OFFSET(tc.locus_r)
};
const IntInputTemplate NAVI_SAMPLE = {
	3, 0, 0, 1, 999,
	"サンプリング",
	"s",
	IW_OFFSET(tc.locus_smp)
};
const IntInputTemplate NAVI_LOCUS_CNT = {
	4, 0, 0, 1, 1800,
	"ポイント",
	"pt",
	IW_OFFSET(tc.locus_cnt)
};
const IntInputTemplate NAVI_LOCUS_UP = {
	4, 1, 0, -9999, 9999,
	"うえレンジ",
	"m",
	IW_OFFSET(tc.locus_up)
};
const IntInputTemplate NAVI_LOCUS_DOWN = {
	4, 1, 0, -9999, 9999,
	"したレンジ",
	"m",
	IW_OFFSET(tc.locus_down)
};
const IntInputTemplate NAVI_LOCUS_RANGE = {
	4, 0, 3, 10, 9999,
	"カラーレンジ",
	"m/s",
	IW_OFFSET(tc.locus_range)
};

const u8* const NAVI_LOCUS_PAL[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.locus_pal),
	"しろ\\あか\\あお",
	"あか\\しろ\\あお",
	"レインボー",
};


#define HELP_THERMAL1	"Locusモードのコマンドを えらびます.  " \
						"\"B+Aボタン\":ナビがめんでB+Aボタンをおすと,Locusモードになります.  " \
						"\"センタリング\":センタリングすると,じどうでLocusモードになります.  " \
						"(B+AボタンでLocusモードにきりかえたときには, Bボタンでキャンセルするまで" \
						"Locusモードをけいぞくします. " \
						"センタリングでLocusモードにきりかえたときには, センタリングをやめると " \
						"じどうでLocusモードから ふっきします.)  <>"
#define HELP_THERMAL2	"ポイントのひょうじスケールをしていします.  " \
						"がめんちゅうおう(グライダマーク)からコンパスサークル(N/E/S/W)までの," \
						"はんけいを せっていします.  <>"
#define HELP_THERMAL3	"ポイントのサンプリングかんかくを していします. " \
						"<ポイント>が120ポイントのときに 5secでサンプリングすると, " \
						"10min のきせきが ひょうじされます.  <>"
#define HELP_THERMAL4	"ひょうじするポイントすうを していします. " \
						"(120よりおおきいと,ポイントがチラつくことがあります.)  <>"
#define HELP_THERMAL5	"そうたいひょうじレンジ(じょうげん)をせっていします.  " \
						"うえレンジより たかいポイントは ひょうじしません.  <>"
#define HELP_THERMAL6	"そうたいひょうじレンジ(かげん)をせっていします.  " \
						"したレンジより ひくいポイントは ひょうじしません.  <>"
#define HELP_THERMAL7	"バリオにおうじてへんかするカラーレンジのせっていをします.  " \
						"カラーレンジを10しょくでぬりわけます.  <>"
#define HELP_THERMAL8	"カラーパレットをせんたくします"

const MenuItem MENU_CONFIG_THERMAL[] = {
	{"コマンド",			HELP_THERMAL1,						MENU_SEL_ENUM,	&ETC_THERMAL_CMD},
	{"スケール",			HELP_THERMAL2,						MENU_SEL_VAL,	&NAVI_LOCUS_R},
	{"サンプリング",		HELP_THERMAL3,						MENU_SEL_VAL,	&NAVI_SAMPLE},
	{"ポイント",			HELP_THERMAL4,						MENU_SEL_VAL,	&NAVI_LOCUS_CNT},
	{"うえレンジ",			HELP_THERMAL5,						MENU_SEL_VAL,	&NAVI_LOCUS_UP},
	{"したレンジ",			HELP_THERMAL6,						MENU_SEL_VAL,	&NAVI_LOCUS_DOWN},
	{"カラーレンジ",		HELP_THERMAL7,						MENU_SEL_VAL,	&NAVI_LOCUS_RANGE},
	{"パレット",			HELP_THERMAL8,						MENU_SEL_ENUM,	&NAVI_LOCUS_PAL},
	{0, 0,														MENU_ID_CONFIG_ETC}
};


///////////////////////////////////////////////////////////////////////////////
// シリンダマップモード
const u8* const ETC_CYL_CMD[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.cyl_cmd),
	"B+Rボタン",
	"ニアシリンダ",
};

const u8* const ETC_CYL_START[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.cyl_end),
	"しゅうりょう",
	"けいぞく",
};

const u8* const ETC_SP_PRIO[] = {
	(u8*)4,
	(u8*)IW_OFFSET(tc.sp_prio),
	"せんこう",
	"Cylinder mode",
	"Locus mode",
	"トグル",
};

#define HELP_CYL1	"シリンダモードのコマンドを えらびます.  " \
						"\"B+Rボタン\":ナビがめんでB+Rボタンをおすと,シリンダモードになります.  " \
						"\"ニアシリンダ\":シリンダにちかづくと,じどうでシリンダモードになります.  " \
						"(B+Rボタンでシリンダモードにきりかえたときには, Bボタンでキャンセルするまで" \
						"シリンダモードをけいぞくします. " \
						"ニアシリンダけんしゅつでシリンダモードにきりかえたときには, シリンダからはなれると " \
						"じどうでシリンダモードから ふっきします.)  <>"
#define HELP_CYL2		"ニアシリンダのきょりをせっていします. このせっていは おんせいナビのニアシリンダと きょうつうです. " \
						"シリンダが100mのときに ニアシリンダを200mにせっていすると, パイロンから300mいないをニアシリンダとはんだんします.  <>"

#define HELP_CYL3		"タスクスタートまえにスタートシリンダへしんにゅうしたときのモードをせんたくします. " \
						"\"けいぞく\":シリンダモードをけいぞくします. " \
						"\"しゅうりょう\":シリンダモードをしゅうりょうし,ノーマルモードにもどります. " \
						"(いちどシリンダからでると,ふたたびシリンダモードになります)  <>"

#define HELP_SP1		"CylinderモードとLocusモードの ゆうせんどを せんたくします. " \
						"\"せんこう\":さきにはいったモードをけいぞくします. " \
						"\"Cylinder\":Cylinderモードをゆうせんします. " \
						"\"Locus\":Locusモードをゆうせんします. " \
						"\"トグル\":CylinderとLocusをトグルします.  <>"

const MenuItem MENU_CONFIG_CYL[] = {
	{"コマンド",			HELP_CYL1,							MENU_SEL_ENUM,	&ETC_CYL_CMD},
	{"ニアシリンダ",		HELP_CYL2,							MENU_SEL_VAL,	&ETC_CYL_NEAR},
	{"スタートまち",		HELP_CYL3,							MENU_SEL_ENUM,	&ETC_CYL_START},
	{"ゆうせんど",			HELP_SP1,							MENU_SEL_ENUM,	&ETC_SP_PRIO},
	{0, 0,														MENU_ID_CONFIG_ETC}
};

///////////////////////////////////////////////////////////////////////////////
// 風速計コマンド
const u8* const ETC_WIND_CMD[] = {
	(u8*)5,
	(u8*)IW_OFFSET(tc.wind_cmd),
	"B+STARTボタン",
	"ピッチング x3",
	"ローリング x3",
	"スパイラル",
	"ストール",
};
const u8* const ETC_WIND_UPDATE[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.wind_update),
	"しない",
	"する",
};
const IntInputTemplate NAVI_SPEED = {
	5, 0, 3, 0, 99999,
	"ニュートラル",
	"m/s",
	IW_OFFSET(tc.my_speed)
};

#define HELP_WINDCHECK1	"ウインドチェックモード(ぎじふうそくけい)のコマンドを えらびます.  " \
							"\"B+STARTボタン\":ナビがめんでB+STARTボタンをおすと,ウインドチェックモードになります.  " \
							"\"ピッチング x3\":ちょくしんじに れんぞく3かいピッチングすると," \
							"じどうでウインドチェックモードになります.  " \
							"\"ローリング x3\":れんぞく3かいローリングすると,じどうでウインドチェックモードになります.  " \
							"\"スパイラル\":スパイラルすると,じどうでウインドチェックモードになります.  " \
							"\"ストール\":ストールすると,じどうでウインドチェックモードになります.  " \
							"(ウインドチェックモードは,Bボタンでキャンセルするか,タイムアウトすると,ナビにふっきします.)  <>"
#define HELP_WINDCHECK2	"ウインドチェックモードで,そくていコンプリートしたときに," \
							"<Nスピード>をじどうでアップデートするか,えらびます. " \
							"\"しない\":じどうでグライダパラメータをアップデートしません.  " \
							"\"する\":じどうでグライダパラメータをアップデートします.  <>"
#define HELP_WINDCHECK3	"あなたのグライダの ふうそくそくていようのニュートラルスピードを せっていします.  " \
							"ウインドチェックモードのフェーズ1で つかいます.  (km/h ではなく,m/s で していします!)  <>"

const MenuItem MENU_CONFIG_WINDCHECK[] = {
	{"コマンド",			HELP_WINDCHECK1,					MENU_SEL_ENUM,	&ETC_WIND_CMD},
	{"オートUpdate",		HELP_WINDCHECK2,					MENU_SEL_ENUM,	&ETC_WIND_UPDATE},
	{"Nスピード",			HELP_WINDCHECK3,					MENU_SEL_VAL,	&NAVI_SPEED},
	{0, 0,														MENU_ID_CONFIG_ETC}
};


///////////////////////////////////////////////////////////////////////////////
// グライダベンチマーク
const u8* const ETC_GLIDER_CMD[] = {
	(u8*)5,
	(u8*)IW_OFFSET(tc.glider_cmd),
	"B+SELECTボタン",
	"ピッチング x3",
	"ローリング x3",
	"スパイラル",
	"ストール",
};
const u8* const ETC_GLIDER_UPDATE[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.bench_update),
	"しない",
	"する",
};

#define HELP_BENCHMARK1	"ベンチマークモードのコマンドを えらびます.  " \
						"\"B+SELECTボタン\":ナビがめんでB+SELECTボタンをおすと,ベンチマークモードになります.  " \
						"\"ピッチング x3\":ちょくしんじに れんぞく3かいピッチングすると " \
							"じどうでベンチマークモードになります.  " \
						"\"ローリング x3\":れんぞく3かいローリングすると じどうでベンチマークモードになります.  " \
						"\"スパイラル\":スパイラルすると,じどうでベンチマークモードになります.  " \
						"\"ストール\":ストールすると,じどうでベンチマークモードになります.  " \
						"(ベンチマークモードは,Bボタンでキャンセルするか,タイムアウトすると,ナビにふっきします.)  <>"
#define HELP_BENCHMARK2	"ベンチマークモードで,そくていコンプリートしたときに," \
							"グライダじょうほう(L/D, Sink rate)をじどうでアップデートするか,えらびます. " \
							"\"しない\":じどうでグライダパラメータをアップデートしません.  " \
							"\"する\":じどうでグライダパラメータをアップデートします.  <>"
#define HELP_BENCHMARK3	"さいごにアップデートされた L/D をひょうじ しています. " \
						"このパラメータは,ナビコンフィグメニューの<みつもり>のパラメータと,きょうゆうしています.  <>"
#define HELP_BENCHMARK4	"さいごにアップデートされた Sink rate をひょうじ しています. " \
						"このパラメータは,ナビコンフィグメニューの<みつもり>のパラメータと,きょうゆうしています.  <>"
#define HELP_BENCHMARK5	"さいごにアップデートされた Airspeed をひょうじ しています.  <>"

const MenuItem MENU_CONFIG_BENCHMARK[] = {
	{"コマンド",			HELP_BENCHMARK1,					MENU_SEL_ENUM,	&ETC_GLIDER_CMD},
	{"オートUpdate",		HELP_BENCHMARK2,					MENU_SEL_ENUM,	&ETC_GLIDER_UPDATE},
	{"*L/D",				HELP_BENCHMARK3,					MENU_SEL_VAL,	&NAVI_LD},
	{"*Sink rate",			HELP_BENCHMARK4,					MENU_SEL_VAL,	&NAVI_DOWNRATION},
	{" (Airspeed)",			HELP_BENCHMARK5,					MENU_SEL_SPEED},
	{0, 0,														MENU_ID_CONFIG_ETC}
};

///////////////////////////////////////////////////////////////////////////////

#define HELP_INFO1	"かくだいテキストで げんざいのざひょうを ひょうじします.  <>"

// 情報メニュー
const MenuItem MENU_INFO[] = {
//	{"フライトログ",		"フライトログを かくにんします",	MENU_FCALL,		MP_Tally},
//	{"タスクログ",			"タスクログを かくにんします",		MENU_FCALL,		MP_Task},
	{"げんざいち",			HELP_INFO1,							MENU_FCALL,		MP_LatLon},
	{"ログデータ",			"ログを かくにんします",			MENU_ID_LOG},
	{"グラフ",				"グラフを かくにんします",			MENU_FCALL,		MP_Graph},
	{"GPSユニット",			"GPSユニットを かくにんします",		MENU_FCALL,		MP_GPS},
	{"カートリッジ",		"カートリッジを かくにんします",	MENU_FCALL,		MP_Cart},
	{"バージョン",			"バージョンを かくにんします",		MENU_FCALL,		MP_Version},
	{"Tips",				"Tipsをひょうじします",				MENU_FCALL,		MP_Tips},
	{0, 0,														MENU_ID_MAIN}
};

///////////////////////////////////////////////////////////////////////////////
// バリオモドキ設定メニュー
const u8* const VARIO_MODE_LIST[] = {
	(u8*)4,
	(u8*)IW_OFFSET(tc.vario_mode),
	"なし",
	"スムーズ",
	"おんかい1",
	"おんかい2",
};
const IntInputTemplate VARIO_UP_CONFIG = {
	4, 1, 3, -9999, 9999, 
	"リフトかいし",
	"m",
	IW_OFFSET(tc.vario_up)
};
const IntInputTemplate VARIO_DOWN_CONFIG = {
	4, 1, 3, -9999, 9999, 
	"シンクかいし",
	"m",
	IW_OFFSET(tc.vario_down)
};
// タスクメニュー
const u8* const VARIO_TO[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.vario_to),
	"ならす",
	"ならさない",
};
const u8* const VARIO_VOLUME[] = {
	(u8*)8,
	(u8*)IW_OFFSET(tc.vol_vario),
	"1:\\",
	"2:\\\\",
	"3:\\\\\\",
	"4:\\\\\\\\",
	"5:\\\\\\\\\\",
	"6:\\\\\\\\\\\\",
	"7:\\\\\\\\\\\\\\",
	"8:\\\\\\\\\\\\\\\\",
};

#define HELP_VARIO1	"バリオのスタートタイミングを えらびます.  " \
					"\"ならす\":Takeoffまえも バリオをならします.  " \
					"\"ならさない\":ウェイティングしているときには バリオをならしません.  <>"
#define HELP_VARIO2	"バリオサウンドの おんりょうをせっていします.  <>"

const MenuItem MENU_CONFIG_VARIO[] = {
	{"モード",				"サウンドタイプを えらびます",		MENU_SEL_ENUM,	&VARIO_MODE_LIST},
	{"リフト",				"リフトかいしを せっていします",	MENU_SEL_VAL,	&VARIO_UP_CONFIG},
	{"シンク",				"シンクかいしを せっていします",	MENU_SEL_VAL,	&VARIO_DOWN_CONFIG},
	{"Takeoffまえ",			HELP_VARIO1,						MENU_SEL_ENUM,	&VARIO_TO},
	{"ボリューム",			HELP_VARIO2,						MENU_SEL_ENUM,	&VARIO_VOLUME},
	{"テスト",				"バリオのサウンドテストをします",	MENU_FCALL,		MP_VarioTest},
	{0, 0,														MENU_ID_CONFIG}
};

///////////////////////////////////////////////////////////////////////////////
// トラックログ設定メニュー
const u8* const LOG_ENABLE[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.log_enable),
	"マニュアル",
	"リアルタイム",
};
const u8* const LOG_PREC[] = {
	(u8*)4,
	(u8*)IW_OFFSET(tc.log_prec),
	"1ms:Aprx.3cm",
	"8ms:Aprx.24cm",
	"32ms:Aprx.96cm",
	"128ms:Aprx.4m",
};
const IntInputTemplate LOG_INTVL = {
	4, 0, 0, 1, 9999,
	"ログかんかく",
	"s",
	IW_OFFSET(tc.log_intvl)
};
const u8* const LOG_OVERWRITE[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.log_overwrite),
	"ログていし",
	"うわがき",
};
const u8* const LOG_DEBUG[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.log_debug),
	"----",
	"Full PVT debug",
};

#define HELP_LOG1	"トラックログのきろくモードをえらびます.  " \
					"\"マニュアル\":GPSにほぞんされているトラックログを,マニュアルでダウンロードしてきろくします.  " \
					"\"リアルタイム\":GPSのそくいデータをリアルタイムにきろくします.  <>"
#define HELP_LOG2	"すいへいざひょうの ほぞんせいどをえらびます.  " \
					"\"1ms\":1ミリびょう(やく3cm)で きろくします. データサイズは,いちばんおおきくなります.  " \
					"\"8ms\":8ミリびょう(やく24cm)で きろくします.  " \
					"\"32ms\":32ミリびょう(やく1m)で きろくします.(デフォルト)  " \
					"\"128ms\":128ミリびょう(やく4m)で きろくします.  データサイズは,いちばんちいさくなります.  <>"
#define HELP_LOG3	"GPSからトラックログをダウンロードします.  " \
					"トラックログのダウンロードちゅうは, リアルタイムほぞんを ちゅうだんします.  <>"
#define HELP_LOG4	"トラックログをマニュアルで ぶんかつします.  (つぎのポイントから, あたらしいセグメントになります)  <>"
#define HELP_LOG5	"トラックログをカートリッジの どのブロックにセーブするか せっていします.  <>"
#define HELP_LOG6	"トラックログのセーブブロックすうを せっていします.  <>"
#define HELP_LOG7	"トラックログをリアルタイムほぞんするばあいの, ほぞんかんかくをせっていします.  <>"
#define HELP_LOG8	"トラックログの あきスペースがなくなったときの どうさをえらびます.  " \
					"\"ログていし\":ログがいっぱいになると,きろくをやめて エラー(ID=12)をマークします.  " \
					"\"うわがき\":ログがいっぱいになると ふるいログをさくじょしてログをきろくします.  <>"
#define HELP_LOG9	"                              <DEBUG ONLY>"
#define HELP_LOG10	"PCへトラックログをアップロードします. " \
					"トラックログをアップロードするためには,PCせつぞくケーブルが ひつようです. 　<>"
#define HELP_LOG11	"トラックログをアップロードするさいの, ボーレートをえらびます. " \
					"ボーレートをあげるとスピードがあがりますが, ボーレートをあげすぎると かんきょうによってはエラーになるばあいがあります.  <>"

const MenuItem MENU_CONFIG_LOG[] = {
	{"ステータス",			"ログステータスをかくにんします",	MENU_FCALL,		MP_LogCheck},
	{"きろくモード",		"ログパラメータをせっていします",	MENU_ID_CONFIG_LOG2},
	{"ぶんかつ",			HELP_LOG4,							MENU_FCALL,		MP_SegmentLog},
	{"ログクリア",			"トラックログをクリアします",		MENU_FCALL,		MP_ClearLog},
	{"ダウンロード",		HELP_LOG3,							MENU_FCALL,		MP_DownloadLogPre},
	{"アップロード",		HELP_LOG10,							MENU_FCALL,		MP_UploadLogPre},
	{0, 0,														MENU_ID_MAIN}
};

const MenuItem MENU_CONFIG_LOG2[] = {
	{"ログほぞん",			HELP_LOG1,							MENU_SEL_ENUM,	&LOG_ENABLE},
	{"かいぞうど",			HELP_LOG2,							MENU_SEL_ENUM,	&LOG_PREC},
	{"ログかんかく",		HELP_LOG7,							MENU_SEL_VAL,	&LOG_INTVL},
	{"ログエンド",			HELP_LOG8,							MENU_SEL_ENUM,	&LOG_OVERWRITE},
	{"(debug)",				HELP_LOG9,							MENU_SEL_ENUM,	&LOG_DEBUG},
	{0, 0,														MENU_ID_CONFIG_LOG}
};

#define HELP_LOG21	"げんざいのフライトログをカートリッジにマニュアルでセーブします.  <>"
#define HELP_LOG22	"げんざいのタスクログをカートリッジにマニュアルでセーブします.  <>"

// 情報メニュー
const MenuItem MENU_LOG[] = {
	{"フライトログ",		"げんざいのフライトログをみます",	MENU_FCALL,		MP_TallyCur},
	{"タスクログ",			"げんざいのタスクログをみます",		MENU_FCALL,		MP_TaskCur},
	{"ログのロード",		"セーブしたログをみます",			MENU_ID_LOG_LIST},
	{"フライトログのセーブ",HELP_LOG21,							MENU_ID_LOG_FL_SAVE},
	{"タスクログのセーブ",	HELP_LOG22,							MENU_ID_LOG_TL_SAVE},
	{"ログせってい",		"ログのせっていをします",			MENU_ID_LOG_AUTO_SAVE},
	{0, 0,														MENU_ID_INFO}
};
const MenuItem MENU_LOG_LIST[] = {
	{"ログ#1をみる",		0,									MENU_SEL_FLOG,	MP_LogView},
	{"ログ#2をみる",		0,									MENU_SEL_FLOG,	MP_LogView},
	{"ログ#3をみる",		0,									MENU_SEL_FLOG,	MP_LogView},
	{"ログ#4をみる",		0,									MENU_SEL_FLOG,	MP_LogView},
	{0, 0,														MENU_ID_LOG}
};
const MenuItem MENU_LOG_FL_SAVE[] = {
	{"#1にセーブ",			0,									MENU_SEL_FLOG,	MP_TallySave},
	{"#2にセーブ",			0,									MENU_SEL_FLOG,	MP_TallySave},
	{"#3にセーブ",			0,									MENU_SEL_FLOG,	MP_TallySave},
	{"#4にセーブ",			0,									MENU_SEL_FLOG,	MP_TallySave},
	{0, 0,														MENU_ID_LOG}
};
const MenuItem MENU_LOG_TL_SAVE[] = {
	{"#1にセーブ",			0,									MENU_SEL_FLOG,	MP_TaskSave},
	{"#2にセーブ",			0,									MENU_SEL_FLOG,	MP_TaskSave},
	{"#3にセーブ",			0,									MENU_SEL_FLOG,	MP_TaskSave},
	{"#4にセーブ",			0,									MENU_SEL_FLOG,	MP_TaskSave},
	{0, 0,														MENU_ID_LOG}
};

const u8* const LOG_FL_AUTOSAVE[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.flog_as),
	"マニュアルS",
	"サスペンドAS",
};
const u8* const LOG_TL_AUTOSAVE[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.tlog_as),
	"マニュアルS",
	"ゴールAS",
	"パイロンAS",
};

#define LOG_AS1	"フライトログのオートセーブのせっていをえらびます. " \
				"\"マニュアルS\":オートセーブしません.  " \
				"\"サスペンドAS\":サスペンドのときにオートセーブします.  <>"
#define LOG_AS2	"タスクログのオートセーブのせっていをえらびます. " \
				"\"マニュアルS\":オートセーブしません.  " \
				"\"ゴールAS\":ゴールしたときにオートセーブします. " \
				"\"パイロンAS\":パイロンをとったときにオートセーブします.  <>"

const MenuItem MENU_LOG_AUTO_SAVE[] = {
	{"フライトログ",		LOG_AS1,							MENU_SEL_ENUM,	&LOG_FL_AUTOSAVE},
	{"タスクログ",			LOG_AS2,							MENU_SEL_ENUM,	&LOG_TL_AUTOSAVE},
	{"ログクリア",			HELP_CONFIG_GPS6,					MENU_SEL_ENUM,	&GPS_TALLY_CLEAR},
	{0, 0,														MENU_ID_LOG}
};

///////////////////////////////////////////////////////////////////////////////
// etc
///////////////////////////////////////////////////////////////////////////////
const u8* const TESTMENU_MODE[] = {
	(u8*)4,
	(u8*)IW_OFFSET(mp.test_mode),
	"オフ",
	"オン",
	"ブーストx5",
	"フルスピード",
};

const IntInputTemplate TESTMENU_LOAD = {
	4, 0, 0, 0, 9999,
	"Load Average",
	"vbc",
	IW_OFFSET(mp.load_test)
};

const MenuItem MENU_TEST[] = {
	{"GPS Emulation",		"GPSエミュレーション",				MENU_SEL_ENUM,	&TESTMENU_MODE},
	{"Add TestRoute",		"テストルートのついか",				MENU_FCALL,		MP_TestRoute},
	{"Load Average",		"ロードアベレージチェック",			MENU_SEL_VAL,	&TESTMENU_LOAD},
	{"Cart Perf.",			"カートリッジせいのうテスト",		MENU_FCALL,		MP_CartTest},
	{"Sector Dump",			"カートリッジセクタダンプ",			MENU_FCALL,		MP_CartTest2},
	{"Wpt Upload",			"ウェイポイントアップロード",		MENU_FCALL,		MP_UploadWpt},
	{"Route Upload",		"ルートアップロード",				MENU_FCALL,		MP_UploadRoute},
	{0, 0,														MENU_ID_INFO}
};


///////////////////////////////////////////////////////////////////////////////
// メニューリスト
///////////////////////////////////////////////////////////////////////////////
const MenuPage MENU_LIST[] = {
	MENU_MAIN,
	MENU_TASK,
	MENU_TASK_START,
	MENU_ROUTE,
	MENU_NEW_ROUTE,
	MENU_CHANGE_ROUTE,
	MENU_WAYPOINT,
	MENU_NEW_WPT,
	MENU_CHANGE_WPT,
	MENU_CONFIG,
	MENU_VOICE,
	MENU_DISPLAY,
	MENU_INFO,
	MENU_CONFIG_GPS,
	MENU_CONFIG_NAVI,
	MENU_CONFIG_EXTDEVICE,
	MENU_CONFIG_ANEMOMETER,
	MENU_CONFIG_VARIO,
	MENU_CONFIG_LOG,
	MENU_CONFIG_LOG2,
	MENU_BLUETOOTH,
	MENU_LOG,
	MENU_LOG_LIST,
	MENU_LOG_FL_SAVE,
	MENU_LOG_TL_SAVE,
	MENU_LOG_AUTO_SAVE,
	MENU_CONFIG_ETC,
	MENU_CONFIG_PARAM,
	MENU_CONFIG_PARAM2,
	MENU_CONFIG_AUTOTARGET,
	MENU_CONFIG_THERMAL,
	MENU_CONFIG_CYL,
	MENU_CONFIG_WINDCHECK,
	MENU_CONFIG_BENCHMARK,
	MENU_TEST,
};


///////////////////////////////////////////////////////////////////////////////
// オートリピート & キー変化
///////////////////////////////////////////////////////////////////////////////
u16 GetPushKey(u16* pre){
	// キーチェック1
	u16 key = IW->key_state; // key_stateはここで取り込む
	u16 push = key & ~*pre;
	*pre = key;

	// タイパマチック
	if(push != 0){
		IW->mp.ar_count = AUTO_REPEATE_INTERVAL - AUTO_REPEATE_START;
		IW->mp.ar_vbc   = IW->vb_counter;
		IW->mp.ar_key   = push & AUTO_REPEATE_ENABLE; // 有効キーを取り出す
		IW->mp.ar_key  &= ~(IW->mp.ar_key - 1);       // 同時押しの場合は最下位のキーを使う
	} else if(key & IW->mp.ar_key){
		if((IW->mp.ar_count += UpdateVBC(&IW->mp.ar_vbc)) >= AUTO_REPEATE_INTERVAL){
			IW->mp.ar_count = 0;
			push = IW->mp.ar_key;
		}
	}
	return push;
}

///////////////////////////////////////////////////////////////////////////////
// メニュータスク
///////////////////////////////////////////////////////////////////////////////
u16 gMenuPreKey = 0;
void DoMenuAction(MenuPage mi){
	u32 action = mi[IW->mp.sel].action;
	if(action < MENU_ID_MAX){
		IW->mp.menuid = action;
		IW->mp.sel = 0;
		DispMenu(IW->mp.menuid);
		return;
	}
//	PlaySG1(SG1_CHANGE);
	switch(action){
	case MENU_FCALL: // 関数呼び出し
	case MENU_SEL_FLOG:// ログ表示専用
		IW->mp.proc = (MenuProc)mi[IW->mp.sel].data;
		(*IW->mp.proc)(0xffff);
		break;

	case MENU_SEL_VAL: // 数値入力
		{
			const IntInputTemplate* t = (IntInputTemplate*)mi[IW->mp.sel].data;
			IntInput* ii = &IW->aip.i_int;
			ii->t = t;
			ii->val	= *t->val;
			ii->pos = 0;
			IW->mp.proc = MP_IntInput;
			(*IW->mp.proc)(0xffff);
		}
		break;
	case MENU_SEL_TIME: // 時刻選択
		{
			const TimeInputTemplate* t = (TimeInputTemplate*)mi[IW->mp.sel].data;
			TimeInput* ii = &IW->aip.i_time;
			ii->t = t;
			ii->val	= *t->val;
			ii->pos = 0;
			IW->mp.proc = MP_TimeInput;
			(*IW->mp.proc)(0xffff);
		}
		break;

	case MENU_SEL_NAME: // 名前入力
		{
			const NameInputTemplate* t = (NameInputTemplate*)mi[IW->mp.sel].data;
			NameInput* ii = &IW->aip.i_name;
			ii->t = t;
			memcpy(ii->val, t->val, t->max);
			ii->val[t->max] = 0; // ii->valはSZ
			IW->mp.proc = MP_NameInput;
			(*IW->mp.proc)(0xffff);
		}
		break;
		
	case MENU_SEL_LAT: // 名前入力
		{
			const LatInputTemplate* t = (LatInputTemplate*)mi[IW->mp.sel].data;
			LatInput* ii = &IW->aip.i_lat;
			ii->t = t;
			ii->val	= *t->val;
			ii->pos = 0;
			IW->mp.proc = MP_LatInput;
			(*IW->mp.proc)(0xffff);
		}
		break;
	
	case MENU_SEL_ENUM: // 列挙選択
		{
			const EnumInputTemplate* t = (EnumInputTemplate*)mi[IW->mp.sel].data;
			if(++*t->val >= t->max) *t->val = 0;
			IW->mp.save_flag |= SAVEF_UPDATE_CFG; // 設定変更フラグ
			// 表示の更新
			PutsSpace2(VAL_POS, IW->mp.sel * 2 + 1, 14);

			LocateX(VAL_POS);
			Puts(t->names[*t->val]);
		}
		break;

	case MENU_SEL_RTWPT: // ルートウェイポイント入力
		{
			// システムに1つしかない
			IW->mp.proc = MP_RtWptInput;
			(*IW->mp.proc)(0xffff);
		}
		break;

	case MENU_SEL_TASK: // タスク入力
		{
			// システムに1つしかない
			IW->mp.proc = MP_TaskInput;
			(*IW->mp.proc)(0xffff);
		}
		break;

	case MENU_SEL_START:// スタートタイム設定
		{
			IW->mp.menuid = MENU_ID_TASK_START;
			IW->mp.sel = 0;
			DispMenu(IW->mp.menuid);
		}
		break;

	case MENU_SEL_SPEED: // グライダスピード表示専用
		MenuFillBox(4, 5, 25, 10);
		DrawText(6, 7, "(じどう けいさん)");
		SetKeyWaitCB(0xffff);
		break;

	case MENU_SEL_PALETTE:// パレット設定専用
		if(++IW->tc.palette_conf > 4) IW->tc.palette_conf = 0;
		IW->mp.save_flag |= SAVEF_UPDATE_CFG; // 設定変更フラグ
		InitPalette(IW->tc.palette_conf);
		ChangeNaviBasePal(IW->tc.palette_conf, IW->mp.cur_sect);
		Locate(VAL_POS, IW->mp.sel * 2 + 1);
		PutsGammaLevel(IW->tc.palette_conf);
		break;
	}
}

void RollbackMenu(u32 pre){
	// カーソルを戻す
	IW->mp.sel = 0;

	if(pre == -1) return;
	MenuPage mi = MENU_LIST[IW->mp.menuid];
	while(mi[++IW->mp.sel].action != pre && (u32)mi[IW->mp.sel].data != pre){
		if(!mi[IW->mp.sel].menu){
			IW->mp.sel = 0;// 見つからなかった…
			break;
		}
	}
	DispMenu(IW->mp.menuid);
}
s32 MT_Menu(){
	// メニューはMAP0に出力する(MAP1は背景で使う)
	SelectMap(MAP_BG0);

	// 指定色ブランク
	vu16* p = (u16*)BGpal;
	u32 pal = IW->vb_counter & 0x3f;
	if(pal > 0x16) pal = 0x16;
	p[0x0f0] = pal << 5; // カーソル選択

	pal = IW->vb_counter & 0x1f;
	p[0x1f0] = (pal << 5) | (pal << 10); //自機

	p[0x1f1] = ((IW->vb_counter & 0x40) && (IW->px.pvt_timeout))?
		RGB(0x08, 0, 0) : RGB(0x3f, 0x38, 0x38); // 測位状態"×"

	// ヘルプスクロール表示
	DispHelp(0);

	// 押下キーの変化チェック
	u16 push = GetPushKey(&gMenuPreKey);
	if(push) IW->mp.auto_off_cnt = IW->vb_counter;

	// 関数コール処理
	if(IW->mp.proc){
		if((*IW->mp.proc)(push)){
			// 関数CBモード終了
			IW->mp.proc = 0;
			DispMenu(IW->mp.menuid);
		}
		return 0;
	}

	// メニュー処理
	if(!push){
		// ルート/ウェイポイントはPCから裏で送信される場合があるため、暇なときにアップデートしておく
		if(IW->mp.menuid == MENU_ID_WAYPOINT || IW->mp.menuid == MENU_ID_ROUTE){
			PutRouteWptCount();
		}
		return 0;
	}
	MenuPage mi = MENU_LIST[IW->mp.menuid];
	if(push & KEY_A){
		PlaySG1(SG1_OK);
		DoMenuAction(mi);
		return 0;
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		while(mi[++IW->mp.sel].menu);
		u32 pre = (mi[IW->mp.sel].action < MENU_ID_MAX)? IW->mp.menuid : -1;
		DoMenuAction(mi);
		RollbackMenu(pre);
		return 0;
	} else if(push & KEY_DOWN){
		PlaySG1(SG1_SELECT);
		if(!mi[++IW->mp.sel].menu) IW->mp.sel = 0;
	} else if(push & KEY_UP){
		PlaySG1(SG1_SELECT);
		if(!IW->mp.sel--){
			while(mi[++IW->mp.sel + 1].menu);
		}
	} else {
		return 0; // 対象外のキー
	}

	// 位置変更あり。カーソルとヘルプを更新
	DispCursor();
	return 0;
}

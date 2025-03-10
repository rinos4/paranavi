///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// 初期化処理やタスクのコントロールをこのファイルにまとめた。
// タスク制御は独自OSの呼び出しをやめ、タスクテーブルで自己管理するようにした。


///////////////////////////////////////////////////////////////////////////////
// 定数
///////////////////////////////////////////////////////////////////////////////

#define KEY_RESUME		(KEY_L | KEY_R)
#define KEY_LOAD_GUARD	(KEY_L | KEY_R)

MyIWRAM*		const IW    = (MyIWRAM*)CPU_WRAM;
RouteList*		const ROUTE = (RouteList*)ROUTE_ADDRESS;
WptList*		const WPT   = (WptList*)WPT_ADDRESS;
Locus*			const LOCUS = (Locus*)LOCUS_ADDRESS;
SpliteTable*	const SPLITE= (SpliteTable*)SPLITE_ADDRESS;

// コンフィグの初期値定義
const TaskConfig INIT_TASK_CONFIG = {
	// ヘッダ情報
	FLBL_TASK_MAGIC,		//u32 magic;
	TDataSize(TaskConfig),	//u32 size;
	0,						//u32 w_count;
	0,						//u32 rfu;
	
	// GPS設定
	9 * 60,	// tzone_m		JST(+9:00)
	-7,		// n_calib		(W007°)
	300,	// stop_dir     (0.3m/s)
	0,		// alt_type		(平均海面高度)
	0,		// calc_type	(オート)
	2,		// tally_clr;	Takeoff Reset
	1,		// vario_mode	バリオモード スムーズ
	1,		// vario_to		テイクオフ前はバリオを鳴らさない
	+0,		// vario_up		+0.0以上
	-2000,	// vario_down	-2.0以下
	0,		// waas_flag	WAASモード
	1,		// u32 gps_warn GPS警告あり
	1,		// u32 nmea_up	デフォルトはハーフ
	{0, 0, 0, 0, 0, 0}, // RFU

	// タスク設定
	0,		// task_mode	フリー
	0,		// route		0番
	1000,	// sector		1000m
	200,	// cylinder		200m
	0,		// rfu1			0
	0,		// goal_type	0
	0,		// start_type	フリー
	0,		// start_time	使わない
	0,		// close_time	使わない
	0,		// pre_pylon    0個
	0,		// cyl_end
	0,		// skip_TO;	// 最初のTOをスキップ
	{0}, // RFU

	0,		// at_mode;	// オートターゲットモード
	500,	// at_min;		// 最小距離
	5000,	// at_max;		// 最大距離
	6000,	// at_recheck;	// 再検索距離

	// ナビ設定
	0,		// view_mode;	標準横
	50,		// near_beep	距離ビープ(シリンダサイズ+α)
	0,		// auto_lock;	マニュアル
	0,		// avg_type;	じどう
	 200,	// ar_max;		// パイロンアローMAX[m]
	-200,	// ar_min;		// パイロンアローMIN[m]
	0,		// self_r;		// 自機表示半径(セクタと同じ)
	1,		// spd_unit;	// 対地速度単位 km/h
	7000,	// my_ldK;		7.0
	1000,	// my_down;		1.0m/s
	7000,	// my_speed;	// 機体の速度 (風速用)
	-4000,	// spiral_spd;	// スパイラル速度
	-8000,	// stall_spd;	// ストール速度
	3000,	// pitch_diff;	// ピッチング速度差
	5,		// pitch_freq;	// ピッチング最大周期
	120,	// roll_diff;	// ロール角度差(120°)
	5,		// roll_freq;	// ロール最大周期
	1000,	// start_spd;	// スタートスピード

	0,		// pylon_type;	// パイロン表示タイプ
	1,		// initial_pos; // イニシャル位置
	7,		// vol_key;		// キーボリューム
	1,		// vol_vario;	// バリオボリューム
	1,		// vm_enable;	// 音声の有効/無効
	0,		// vm_wait;		// 音声なし
	9,		// vm_normal;	// 直進音声
	1,		// vm_stop;		// 停止中音声なし
	2,		// vm_center;	// センタリング中
	4,		// view_alt		// Next L/D
	1,		// anemo_unit;	// 大気速度単位 km/h
	4,		// vm_near;		// ニアシリンダの音声
	10,		// vm_interval;	// 音声の間隔
	0,		// start_alarm;	// スタートアラーム
	0,		// sp_prio;		// SPモード優先度
	{0, 0, 0}, // RFU

	10,		// auto_off		自動電源OFF 10分間
	0,		// time_alarm	時報なし
	0,//1,	// wind_cmd;	ローリングx3
	0,//2,	// glider_cmd;	ピッチングx3
	1,		// thermal_cmd; センタリング
	1,		// wind_update; 自動書き換えOn
	1,		// bench_update;自動書き換えOn
	20,		// stable_angle;// 角度変化マージン 20°
	1000,	// stable_speed;// 速度変化マージン 1 m/s
	5,		// init_wait;	// 5秒
	30,		// wait_timeout;// タイムアウト待ち
	5,		// comp_timeout;// コンプリート後の表示時間
	2,		// palette_conf;// パレット設定
	100,	// keep_range:	//リフトでもシンクでもない上昇率

	0,		// anemo_coef:	// 風速係数　(標準値として16384000 * 2とかでも良いが)
	1,		// cyl_cmd;		// シリンダマップコマンド ニア
	500,	// cyl_near;	// ニアシリンダ
	{0, 0, 0, 0, 0},// RFU

	2,		// locus_smp;	2秒サンプリング(4分)
	300,	// locus_r;		300m
	1800,	// locus_cnt;	// 軌跡数(MAX 1800)
	+3000,	// locus_up;	// 上レンジ
	-3000,	// locus_down;	// 下レンジ
	2000,	// locus_range; // 2m/s
	1,		// locus_pal;	// パレットモード

	// Bluetooth
	0,		// bt_mode;					Bluetoothなし
	{0},	// bt_addr[BT_ADDR_LEN];	接続先アドレスは空
	{"0000"},// bt_pin [BT_PIN_LEN];	大抵"0000"か"1234"なので、0000をデフォルトに設定

	0,		// log_enable;	// トラックログ有効/無効
	{0, 0},	// rsv
	2,		// log_prec;	// 保存精度
	1,		// log_intvl;	// 保存間隔
	1,		// log_overwrite// 上書きモード
	0,		// log_debug;	// デバッグオフ

	0,		// flog_as;		//フライトログのオートセーブ設定
	0,		// tlog_as;		//トラックログのオートセーブ設定
	{0, 0, 0, 0, 0, 0}, // RFU
};

///////////////////////////////////////////////////////////////////////////////
// BIOSコール等はここで定義
///////////////////////////////////////////////////////////////////////////////

#ifdef __thumb__
//#pragma optimize(0, "off")
// ARMモードにするのが面倒…
u32 InterlockedExchange(vu32* p, u32 val){
	s32 r;
	INTR_OFF(); // REG_IMEクリアのみ
	r = *p;
	*p = val;
	INTR_ON(); // REG_IMEセットのみ
	return r;
}
//#pragma optimize(0, "on")

#else
// ARMのみ
u32 InterlockedExchange(vu32*p, u32 val){
  u32 ret;
  __asm (
	"swp %0, %2, [%1]\n"
	: "=&r"(ret)
	: "r"(p), "r"(val)
	: "memory");
  return ret;
}
u8 InterlockedExchange8(vu8*p, u8 val){
  u8 ret;
  __asm (
	"swpb %0, %2, [%1]\n"
	: "=&r"(ret)
	: "r"(p), "r"(val)
	: "memory");
  return ret;
}
#endif

//算術関連
s32 BiosDiv(s32 num, s32 denom, s32* mod){
//	*mod  = num % denom;
//	return  num / denom;
  __asm (
	"\n"
#ifdef __thumb__
	"push	{r1-r3}\n"
	"swi	0x6\n"
	"str	r1, [%2]\n"
	"pop	{r1-r3}\n"
#else
	"stmfd	sp!, {r1-r3}\n"
	"swi	0x60000\n"
	"str	r1, [%2]\n"
	"ldmea	sp!, {r1-r3}\n"
#endif

	: "=&r"(num)	// %0

	: "r"(denom),	// %1
	  "r"(mod)		// %2

	: "memory");
  return num;
}

u32 BiosSqrt(u32 n){
  __asm (
	"\n"
#ifdef __thumb__
	"push	{r1,r3}\n"
	"swi	0x8\n"
	"pop	{r1,r3}\n"
#else
	"stmfd	sp!, {r1, r3}\n"
	"swi	0x80000\n"
	"ldmea	sp!, {r1, r3}\n"
#endif

	: "=&r"(n)	// %0
	:
	: "memory");
  return n;
}
s32 BiosArcTan2(s32 x, s32 y){
  __asm (
	"\n"
#ifdef __thumb__
	"push	{r1,r3}\n"
	"swi	0xA\n"
	"pop	{r1,r3}\n"
#else
	"stmfd	sp!, {r1, r3}\n"
	"swi	0xA0000\n"
	"ldmea	sp!, {r1, r3}\n"
#endif

	: "=&r"(x)	// %0
	:
	: "memory");
  return x;
}

// 初期化
void BiosRegisterRamReset(u32 n){
  __asm (
	"\n"
#ifdef __thumb__
	"push	{r1,r3}\n"
	"swi	0x1\n"
	"pop	{r1,r3}\n"
#else
	"stmfd	sp!, {r1, r3}\n"
	"swi	0x10000\n"
	"ldmea	sp!, {r1, r3}\n"
#endif

	: "=&r"(n)	// %0
	:
	: "memory");
}

// おまけ関数
u32 BiosHypot(s32 a, s32 b){
	if(a == 0x80000000 || b == 0x80000000) return -1; // 最大値を返しておく…
 	if(a < 0) a = -a;
	if(b < 0) b = -b;
	s32 i = 0;
	for(i = 0 ; a + b >= 0x10000 ; ++i, a >>= 1, b >>= 1);
	return BiosSqrt(a * a + b * b) << i;
}
// sqrt(a^2 + b^2 + c^2)
u32 BiosTripot(s32 a, s32 b, s32 c){
	if(a == 0x80000000 || b == 0x80000000 || c == 0x80000000) return -1; // 最大値を返しておく…
	if(a < 0) a = -a;
	if(b < 0) b = -b;
	if(c < 0) c = -c;
	s32 i = 0;
	for(i = 0 ; a + b + c >= 0x10000 ; ++i, a >>= 1, b >>= 1, c >>= 1);
	return BiosSqrt(a * a + b * b + c * c) << i;
}

// mathと同じ引数で 64Kで返す
s32 BiosAtan64K(s32 yK, s32 xK){
	// 16bitへ正規化してBiosArcTan2を呼び出す(実機で試したら、なぜか正規化が必要なかったが…)
	for(; xK < -0x7fff || 0x7fff < xK || yK < -0x7fff || 0x7fff < yK ; xK >>= 1, yK >>= 1);
	return BiosArcTan2(xK, yK) & 0xffff;
}

s32 CountBit(s32 a){
	if(a == 0x80000000) return 32;
	if(a < 0) a = -a;
	s32 c = 0;
	for(; a ; a >>= 1) ++c;
	return c;
}

s32 BiosMBoot(MBootParam* p){
  __asm (
	"\n"
#ifdef __thumb__
	"push	{r1,r3, r7}\n"
	"mov	r7, r0\n"
	"mov	r1, #1\n"
	"swi	0x25\n"
	"pop	{r1,r3,r7}\n"
#else
	"stmfd	sp!, {r1, r3, r7}\n"
	"mov	r7, r0\n"
	"mov	r1, #0\n"
	"swi	0x250000\n"
	"ldmea	sp!, {r1, r3, r7}\n"
#endif

	: "=&r"(p)	// %0
	:
	: "memory");
	return (s32)p;
}

u32 BiosLZ77UnCompWram(const void* src, const void* dst){
	u32 num;
  __asm (
	"\n"
#ifdef __thumb__
	"push	{r1-r3}\n"
	"ldr	r2, [r0]\n"
	"swi	0x11\n"
	"lsr	%0, r2, #8\n"
	"pop	{r1-r3}\n"
#else
	"stmfd	sp!, {r1-r3}\n"
	"ldr	r2, [r0]\n"
	"swi	0x110000\n"
	"lsr	%0, r2, #8\n"
	"ldmea	sp!, {r1-r3}\n"
#endif
	: "=&r"(num)	// %0
	:
	: "memory");
	return num;
}
u32 BiosLZ77UnCompVram(const void* src, const void* dst){
	u32 num;
  __asm (
	"\n"
#ifdef __thumb__
	"push	{r1-r3}\n"
	"ldr	r2, [r0]\n"
	"swi	0x12\n"
	"lsr	%0, r2, #8\n"
	"pop	{r1-r3}\n"
#else
	"stmfd	sp!, {r1-r3}\n"
	"ldr	r2, [r0]\n"
	"swi	0x120000\n"
	"lsr	%0, r2, #8\n"
	"ldmea	sp!, {r1-r3}\n"
#endif
	: "=&r"(num)	// %0
	:
	: "memory");
	return num;
}


u32 UpdateVBC(u32* pre){
	u32 cur = IW->vb_counter;
	u32 ret = cur - *pre;
	*pre = cur;
	return ret;
}

s32 RoundDiv(s32 num, s32 denom){
	return BiosDiv(num + ((num > 0)? (denom/2) : (-denom/2)), denom, &num);
}

// 乱数は線形合同法を使用。乱数としてはイマイチだが、オートターゲット用ならこれで十分。
void MySrand(u32 v){
	IW->mp.rand_val = v;
}
u32 MyRand(){
	IW->mp.rand_val = IW->mp.rand_val * 1103515245 + 12345;
	return IW->mp.rand_val >> 8; // LSBの周期は256
}

///////////////////////////////////////////////////////////////////////////////
// ルート/ウェイポイント操作系関数
///////////////////////////////////////////////////////////////////////////////
void UpdateWptInfo(TaskLogData* tld, Route* rt, u32 n){
	if(!n) return;
	TaskLog1* tl1 = &tld->tl1[n];
	tl1->cyl = GetCurCyl(rt, n - 1);
	TaskLog2* tl2 = GetTL2Addr(tld, n);
	if(tl2){
		Wpt* w = GetWptInfo(rt, n - 1);
		tl2->wpt_alt = w->alt;
		memcpy(tl2->wpt_name, w->name, sizeof(tl2->wpt_name));
	}
}

s8 GetPrePylon(){
//	return IW->tc.pre_pylon;
	if(IW->tc.skip_TO) return IW->tc.pre_pylon + 0x81;
	return IW->tc.pre_pylon;
}

// タスクログ
void InitTaskLog(u32 keepTO){ // ログ初期化
	TaskLogData* tld = &IW->task_log;

	TaskLog1 takeoff;
	if(keepTO) takeoff = tld->tl1[0]; // テイクオフ情報をバックアップ
	DmaClear(3, 0, tld, sizeof(TaskLogData), 32);

	// コンフィグ情報セット
	tld->magic = FLOG_MAGIC_TASK;
	tld->log_time = IW->px.dtime;
	tld->pre_pylonX	= GetPrePylon();
	tld->start_time = (u16)(IW->tc.start_type? IW->tc.start_time : 0);
	tld->update_mark= (keepTO && takeoff.dtime)? 1: 0;

	// ルート情報セット
	Route* rt = GetTaskRoute();
	IW->mp.pre_route = rt;
	if(rt){
		tld->rt_dist	= rt->dist;
		tld->rt_count	= rt->count;
		tld->pre_pylonX	= GetPrePylon();
		memcpy(tld->rt_name, rt->name, sizeof(tld->rt_name));
		// ウェイポイント情報セット
		u32 i;
		for(i = 1 ; i <= rt->count ; ++i) UpdateWptInfo(tld, rt, i);
	} else {
		strcpy(tld->rt_name, "## NONE ##");
	}

	// トリップメータの初期化
	IW->mp.tally.trip_mm = 0;

	// テイクオフ情報の設定
	if(keepTO) tld->tl1[0] = takeoff; // テイクオフ情報を戻す
	else       IW->mp.tlog_presave = -1; // 完全初期化
}

void SetTlog1(TaskLog1* tl1){
	tl1->dtime= IW->px.dtime;
	tl1->trip = IW->mp.tally.trip_mm / 1000; // u64!
	tl1->lat  = IW->px.lat;
	tl1->lon  = IW->px.lon;
	tl1->alt  = RoundDiv(IW->px.alt_mm, 1000);
	if(!tl1->lat) tl1->lat = 1; // 0を特殊処理していた。万が一、経度0°で使っても3mm位誤差のうち？
}

void UpdateTaskLog(s32 n){ // ログデータ追加
	Route* rt = GetTaskRoute();
	if(IW->mp.pre_route != rt) InitTaskLog(1);

	// ルート変更せずに値が変わる変数をアップデート
	TaskLogData* tld = &IW->task_log;
	tld->pre_pylonX	 = GetPrePylon();
	tld->start_time  = (u16)(IW->tc.start_type? IW->tc.start_time : 0);
	tld->update_mark |= n? 2 : 1;

	// ログエリア取得
	if(n > tld->rt_count) return; // !?
	SetTlog1(&tld->tl1[n]);

	// ログ格納
	if(n) UpdateWptInfo(tld, rt, n); // 念のため更新

	IW->mp.tally.trip_mm = 0;
}

#define AUTO_TARGET_ID (-2)
#define MAX_AT_LOG 60 // これ以上は名前が消える
void UpdateAutoTargetLog(){
	// 一応ターゲットがあるかチェック
	Wpt* wpt = GetNearTarget();
	if(!wpt) return; // !?

	TaskLogData* tld = &IW->task_log;
	if((u32)IW->mp.pre_route != AUTO_TARGET_ID){
		 IW->mp.pre_route = (Route*)AUTO_TARGET_ID;
		 // オートターゲット用初期化

		TaskLog1 takeoff = tld->tl1[0]; // テイクオフ情報をバックアップ
		DmaClear(3, 0, tld, sizeof(TaskLogData), 32);

		// コンフィグ情報セット
		tld->magic = FLOG_MAGIC_TASK;
		tld->log_time = IW->px.dtime;
		tld->update_mark= (takeoff.dtime)? 1: 0;
		tld->pre_pylonX	= 0; // テイクオフをスタートパイロンにする
		// ルート情報
		strcpy(tld->rt_name, "[Auto Target]");

		// テイクオフ情報の設定
		tld->tl1[0] = takeoff; // テイクオフ情報を戻す
		IW->mp.tlog_presave = -1; // 保存場所を初期化
	}

	if(tld->rt_count >= MAX_AT_LOG) return; // これ以上ログできない
	TaskLog1* tl0 = &tld->tl1[   tld->rt_count ];
	TaskLog1* tl1 = &tld->tl1[++(tld->rt_count)];
	SetTlog1(tl1);
	tl1->cyl = IW->tc.cylinder;
	TaskLog2* tl2 = GetTL2Addr(tld, tld->rt_count);
	if(tl2){
		tl2->wpt_alt = wpt->alt;
		memcpy(tl2->wpt_name, wpt->name, sizeof(tl2->wpt_name));
	}

	u32 len;
	CalcDist(tl0->lat, tl0->lon, tl1->lat, tl1->lon, &len, 0);
	tld->rt_dist += len;

	if(IW->tc.tlog_as){
		SaveFLog(IW->mp.tlog_presave, &IW->task_log, sizeof(TaskLogData), 0); // 未ゴール時は表示後に保存
	}
	IW->mp.tally.trip_mm = 0;
}

// 
TaskLog2* GetTL2Addr(TaskLogData* tld, u32 n){
	if(!n || (u32)&tld->tl1[tld->rt_count + 1] > (u32)&tld->tl2[TL2MAX - n]) return 0; // 書き込めない
	return &tld->tl2[TL2MAX - n]; // 書き込みバッファあり
}

Route* GetTaskRoute(){
	return (IW->tc.route < ROUTE->route_count)? &ROUTE->route[IW->tc.route] : 0;
}
Wpt* GetWptInfo(const Route* rt, s32 offset){
	if(!rt || offset < 0 || rt->count <= offset) return 0;
	return &WPT->wpt[rt->py[offset].wpt];
}
Wpt* GetCurWpt(const Route* rt, s32 offset){
	return GetWptInfo(rt, offset + IW->mp.cur_point);
}
u16 GetCurCyl(const Route* rt, u32 offset){ // offset = IW->mp.cur_point
	u16 ret = 0xffff;
	if(rt && offset < rt->count) ret = rt->py[offset].cyl;
	return (ret == 0xffff)? IW->tc.cylinder : ret;
}
Wpt* GetDefaultLD(){
	return ((u32)WPT->def_ld < (u32)WPT->wpt_count)? &WPT->wpt[WPT->def_ld] : 0;
}
Wpt* GetNearTarget(){
	return (IW->mp.nw_target < (u32)WPT->wpt_count)? &WPT->wpt[IW->mp.nw_target] : 0;
}


///////////////////////////////////////////////////////////////////////////////
// 初期化処理
///////////////////////////////////////////////////////////////////////////////
void Isr();
void Isr_end();
void InitNavi();

// プログラムエラーのチェック
void InitError(){
	// チェックパラメータを出力して停止
	Putsf("%mInitError%r" "IWSZ:%08x=03007ffc%r" "PGMS:%08x<%08x%r" "ISRS:%08x<%08x",
		&IW->irq_handler, &__iwram_overlay_lma, PROGRAM_LIMIT, (u32)Isr_end - (u32)Isr, sizeof(IW->isr_cpy));
	for(;;);// 停止
}

// 全I/Oの初期化
void Init(){
	// まずメモリを初期化。IWRAMは0クリア
	DmaClear(3, 0, IW->global_reserve, IW->isr_cpy - IW->global_reserve, 32); // クリアするのはisr_cpy直前まで
//	BiosRegisterRamReset(BIOSRESET_IWRAM); // 残したい領域もあるので、これは使わない

	// 軌跡バッファは0x80000000で埋める
	DmaClear(3, INVALID_LAT, LOCUS->val, sizeof(LOCUS->val), 32);
	LOCUS->index     = LOCUS->sample = 0;
	IW->mp.cur_lat   = INVALID_LAT;
	IW->mp.nmea_xalt = INVALID_VAL;

	// Route/Wpt/Spliteはヘッダ情報のクリアだけで十分
	ROUTE->route_count = 0;
	WPT->wpt_count = 0;
	SPLITE->flag = 0; // 特殊スプライトモード解除

	// 音声バッファは初期化の必要なし
	
	// IWRAM領域の非0初期値を設定。面倒なので、できるだけ0初期値で使えるようにする…
	IW->mp.cur_view		= -1;
	IW->mp.pre_task		= -1;
	IW->mp.nw_target	= -1;
	IW->mp.goal_dist.pos= -1;
	IW->mp.boot_msg		= 1;
	IW->mp.proc			= MP_Navi; // セットだけでコールはしない
	IW->mp.nw_start.lat	= INVALID_LAT;
	strcpy(IW->mp.nw_start.name, "SearchPos"); // 9文字+1=10byte
	IW->px.lat			= INVALID_LAT;


	// 次にI/O初期化。最初はビデオ。ビデオが使えないとエラー表示も出せないので…
	InitVideo();

	// 以降、エラーメッセージ表示OK

	//サウンド基本設定
	REG16(REG_SGCNT0_L) = 0x0000; // S1-4はこれでOnOff制御する
	REG16(REG_SGCNT0_H) = 0x0006;
	REG16(REG_SGCNT1)   = 0x0080;
	REG16(REG_SGBIAS)	= 0x0200;
	IW->vm.vario_test = VARIO_TEST_DISABLE;

	// UART初期化
	StartGPSMode(0); //とりあえず9600bpsに設定
	REG16(REG_IE) |= SIO_INTR_FLAG;

	// V BLANK初期化
	REG32(REG_DISPSTAT) = DISPSTAT_VBIRQ | DISPSTAT_VCIRQ;
	REG16(REG_IE) |= V_BLANK_INTR_FLAG;

	// タイマ初期化(Anemometer用)
	REG16(REG_TM3CNT)   = 0x83;

	REG16(REG_TM2CNT)   = 0x00; // 送信FIFO Emptyの検出に使う。デフォルトは無効。
	REG16(REG_IE) |= TIMER2_INTR_FLAG;

	// 割り込み開始
	s32 i = (u32)Isr_end - (u32)Isr;// OK?
	if(i > sizeof(IW->isr_cpy))						InitError(); // IWRAMのISR領域に収まらない!
	if((u32)&IW->irq_handler != 0x03007ffc)			InitError(); // MyIWRAMサイズテスト用
	if((u32)&__iwram_overlay_lma >= PROGRAM_LIMIT)	InitError(); // プログラムサイズテスト
	memcpy(IW->isr_cpy, (void*)Isr, i);
	IW->irq_handler = (u32)IW->isr_cpy;	// ISRは高速動作させるためにIWRAMでARM実行する
	INTR_ON();


	// タスク設定を初期化
	IW->tc = INIT_TASK_CONFIG;

	// MultiBootチェック
	if(IS_MBOOT()){
		FillBox(8, 15, 21, 18);
		DrawTextCenter(16, "Multiboot");
	}

	DrawTextCenter(10, "Loading config..."); // 17文字

	// カートリッジを初期化
	if((IW->cw.init_error = IW->cif->InitCart()) != 0){
		// 初期化エラー時はカートリッジなし
		IW->cif = &CIF_NONE;
	}
		
	// タスク設定をカートリッジからロード
	if(((REG16(REG_KEYINPUT) ^ 0x3ff) & KEY_LOAD_GUARD) != KEY_LOAD_GUARD){ // 割り込み後か曖昧なので、ガードキーは自前でレジスタアクセス
		s32 ret = IW->cif->ReadData(FI_CONFIG_OFFSET, &IW->tc, sizeof(IW->tc), FLBL_TASK_MAGIC);
		if(ret){
			FillTile(MAP_BG1, 0, 16, 29, 19, SELMENU_BK3); // エラー時は赤Fillで強調
			Putsf("%1.16mThe config was reset because%r" "of the E%d", ret);
			if(ret == FLASH_E_MAGIC) Puts(" (software upgrade)"); // FLASH_E_MAGICは初期化時や改版時に発生しうるのでコメントを付けてみた。
		}

		// 続けてルートウェイポイントもロード
		IW->cif->ReadData(FI_ROUTE_OFFSET, ROUTE, ROUTE_WPT_SIZE, FLBL_ROUTE_MAGIC); // ルートが無くてもエラーにはしない
	} else {
		// タスクデータ領域が壊れて起動しなくなった場合に備えて、デフォルト値で起動する
		FillBox(8, 15, 21, 18);
		DrawTextCenter(16, "LoadGuard!");
	}
	
	// 読み込んだデータの整合性チェックと修正
	if((u32)ROUTE->route_count > MAX_ROUTE)		ROUTE->route_count= 0;
	if((u32)WPT->wpt_count     > MAX_WPT  )		WPT->wpt_count = 0;
	if((u32)WPT->def_ld >= (u32)WPT->wpt_count)	WPT->def_ld = 0;
	if(IW->tc.route >= ROUTE->route_count)		IW->tc.route = 0;

	// 統計情報の初期化
	InitTally();

	// タスクログの初期化
	InitTaskLog(0);

	// パレットの再設定
	InitPalette(IW->tc.palette_conf); // ロードしたタスク設定のガンマ補正を反映

	// ナビ初期化
	InitNavi();

	// トラックログ変数初期化
	DrawTextCenter(10, "Checking TrackLog..."); // 20文字
	InitLogParams();  // これは遅いSDカードを使うと数秒かかる場合がある

	// 初期化完了。あとはGPSの接続待ち
	DrawTextCenter(10, "Connecting to GPS..."); // 20文字
}

///////////////////////////////////////////////////////////////////////////////
// サスペンド/リジューム制御
///////////////////////////////////////////////////////////////////////////////
void Suspend(){
	// GPS停止
	LpSendCmd(Cmnd_Turn_Off_Pwr);
	s32 t = IW->vb_counter;

	// 音停止
	EnableSound(SOUND_CH_VARIO, -1);
	EnableSound(SOUND_CH_KEY,   -1);

	// フライトログのサスペンド自動セーブ
	if(IW->tc.flog_as && IW->mp.tally.log_time) SaveFLog(IW->mp.flog_presave, &IW->mp.tally, sizeof(Tally), 0);
	IW->cif->Flush(); // 未保存のデータをフラッシュさせる

	// GPS停止待ち
	while(IW->vb_counter - t < VBC_TIMEOUT(2)); // 画面メッセージを見せるため、2秒ほどウェイトする

	//サスペンド開始
	REG16(REG_DISPCNT)	= FORCE_BLANK;
	REG16(REG_IE)	   |= KEY_INTR_FLAG;
	REG16(REG_P1CNT)	= 0xc000 | KEY_RESUME; // L/R同時押しで再起動
	BiosStop();


	// ... L+Rキー待ち ...


	// リジューム処理
	REG16(REG_DISPCNT) = DISP_MODE_1 | DISP_BG0_ON | DISP_BG1_ON | DISP_BG2_ON | DISP_OBJ_ON | DISP_OBJ_CHAR_1D_MAP;
	REG16(REG_IE) &= ~KEY_INTR_FLAG;
	IW->mp.sel = 0;
	IW->mp.freeflight = 0;
	IW->px.gstate = 0; // グライダ停止ステート
	if(IW->gi.state == GPS_PVT_WAIT) IW->gi.state = GPS_PVT;
	IW->mp.auto_off_cnt = IW->vb_counter;
	IW->mp.resume_vbc   = IW->vb_counter;
	if(IW->tc.tally_clr == 1) InitTally();
	IW->mp.tlog.abs_flag |= ABSF_SEPARETE; // トラックログ セパレータ挿入
	IW->mp.tally.log_time = 0;
}

///////////////////////////////////////////////////////////////////////////////
// アプリ初期化、タスク制御
///////////////////////////////////////////////////////////////////////////////
typedef s32 (*MyTask)();
const MyTask MY_TASK[] = {
	MT_GPSCtrl,
	MT_DataLink,
	MT_AppLayer,
	MT_Navi,
	MT_Vario,
	MT_Menu,
	MT_Voice,
	MT_TrackLog,
	0
};

u32 UpdateCPULoad(){
	u32 cur = IW->anemo.tm;
	u32 pre = InterlockedExchange(&IW->mp.last_load, cur);
	return cur - pre;
}
u32 GetLoad99(u32 val, s32 sum){
	if(!sum) return 0;
	val = RoundDiv(val * 100, sum);
	if(val > 99) val = 99;
	return val;
}

s32 DoMTask(){
	const MyTask* p;
#define LOADLOG_TASKSTART 3
#define LOADLOG_TASKEND   (3 + 8)
	u32 load_num = LOADLOG_TASKSTART; // タスクは#3から
	u32 ret = 0;
	for(p = MY_TASK ; *p && !ret; ++p){
		ret = (*p)();
		IW->mp.load[load_num++] += UpdateCPULoad();
	}
	return ret;
}

// crtからAgbMainまでは相対ジャンプで必ず到達
void AgbMain(){
	// まず最初にカートリッジ判別とタイプ別処理
	if(GetPC() > (u32)AgbMain + 0x100){ // SuperCardとか?
		// 08000000番地でROM起動のカートリッジ使用時も、RAMへジャンプして動作させる
		DmaCopy(3, ROM_ADDRESS, EX_WRAM, ROM_SIZE, 32);

		// SCSD/エミュレータ判別
		StartGPSMode(0);
		if(REG16(REG_SIOCNT) & UART_DATA8){// VBAは何故かUART_DATA8が立たないのを利用…
			IW->cif = &CIF_SUPERCARD_SD;
		} else {
			IW->cif = &CIF_EMULATOR;
		}
		JumpR0(AgbMain); // リンカで指定したEX_WRAMオフセットへのジャンプとなる
	}
	if(IS_MBOOT())	IW->cif = &CIF_NONE; // MultiBootのRAM実行
	if(!IW->cif)	IW->cif = &CIF_JOYCARRY;
	// ここまでにカートリッジタイプ確定しておく

	// メモリ、I/O初期化
	Init();

	//Load 計測を開始
	UpdateCPULoad();

	// タスクループ
	for(;;){
		IW->intr_flag = 0;
		if(DoMTask()) continue; // 再処理要求があれば高優先度のタスクへ制御を戻す(NMEA連続取り込み用)

		// CPU Load確認用
		if(IW->mp.load_test && IW->vb_counter - IW->mp.load_vbc > IW->mp.load_test){
			IW->mp.load_vbc = IW->vb_counter;

			u32 i, sum = 0;
			u32 diff[MAX_LOAD_LIST];
			for(i = 0 ; i < LOADLOG_TASKEND ; ++i){
				sum += diff[i] = InterlockedExchange(&IW->mp.load[i], 0); // 実際にInterlockが必要なのは i=0 のみだが…
			}

			FillTile(MAP_BG1, 24, 2, 29, 17, SELMENU_BK3);
			SelectMap(MAP_BG0);
			Putsf("%24.2m" "Intr%2d%r" "Main%2d%r" "GPS %2d%r" "Navi%2d%r" "Sund%2d%r" "Menu%2d%r" "TLog%2d%r" "Idle%2d",
#define LOAD_AVG(n) GetLoad99((n), sum)
				LOAD_AVG(diff[0]),						// intr
				LOAD_AVG(diff[2]),						// main
				LOAD_AVG(diff[3] + diff[4] + diff[5]),	// gps
				LOAD_AVG(diff[6]),						// navi
				LOAD_AVG(diff[7] + diff[9]),			// vario + Voice
				LOAD_AVG(diff[8]),						// menu
				LOAD_AVG(diff[10]),						// TrackLog
				LOAD_AVG(diff[1]));						// idle
		}
		IW->mp.load[2] += UpdateCPULoad(); // Mainは#2 (とは言ってもLoad測定しか負荷のかかる仕事は無いが…)

		// 消費電流を減らすため、できるだけスリープに入れる。
		while(!IW->intr_flag){
			// アイドル中
#define VBC_1min	3600 // 60frame*60sec
			if(IW->tc.auto_off && (IW->vb_counter - IW->mp.auto_off_cnt) > IW->tc.auto_off * VBC_1min){
				Suspend();	// GPSオフで無操作が続いたら、オートサスペンドする
			} else {
				BiosHalt(); // 電池を持たせるためにできるだけスリープにしておく
			}
			IW->mp.load[1] += UpdateCPULoad(); // アイドルは#1
		}
	}
}

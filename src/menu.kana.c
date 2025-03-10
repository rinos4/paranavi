///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005-2007 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// ���j���[��ʂ̐�������̃t�@�C���ɂ܂Ƃ߂��B

///////////////////////////////////////////////////////////////////////////////
// �萔
///////////////////////////////////////////////////////////////////////////////
#define KEY_LOCUS		(KEY_B | KEY_A)
#define KEY_CYL			(KEY_B | KEY_R)
//#define KEY_TEXT		(KEY_B | KEY_L)
#define KEY_WINDCHECK	(KEY_B | KEY_START)
#define KEY_BENCHMARK	(KEY_B | KEY_SELECT)
#define KEY_LOG_NEXT	(KEY_A | KEY_R | KEY_RIGHT | KEY_DOWN)
#define KEY_LOG_PREV	(KEY_L | KEY_LEFT | KEY_UP)
#define KEY_SP_NEXT		(KEY_B | KEY_A)

const u16 AUTO_REPEATE_START	= 800 / 16; // �I�[�g���s�[�g�J�n(0.8�b)
const u16 AUTO_REPEATE_INTERVAL	= 100 / 16; // �I�[�g���s�[�g�Ԋu(0.1�b)
const u16 AUTO_REPEATE_ENABLE = ~(KEY_START | KEY_SELECT); // START, SELECT��AutoRepeat����


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

#define GEN_COUNTER (*(u32*)(0x08000000 + 0xC8)) // ������I�t�Z�b�g

///////////////////////////////////////////////////////////////////////////////
// ���j���[�p�\����
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
// ���j���[���X�g
///////////////////////////////////////////////////////////////////////////////

enum {
	// ���j���[�ԍ�
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

	MENU_FCALL,		// �֐��Ăяo��
	MENU_SEL_ENUM,	// �񋓑I��
	MENU_SEL_NAME,	// ���O����
	MENU_SEL_LAT,	// �ܓx�o�x����
	MENU_SEL_VAL,	// ���l����
	MENU_SEL_TIME,	// ��������
	MENU_SEL_RTWPT,	// ���[�g�E�F�C�|�C���g����
	MENU_SEL_TASK,  // �^�X�N�I��
	MENU_SEL_START,	// �X�^�[�g�ݒ�
	MENU_SEL_SPEED,	// �X�s�[�h�\����p
	MENU_SEL_PALETTE,// �p���b�g�ݒ��p
	MENU_SEL_FLOG,	// ���O�����\��
};

extern const MenuPage MENU_LIST[];

///////////////////////////////////////////////////////////////////////////////
// ���ʏ���
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
	Putsf("%1.6m���Ȃ� �Ȃ܂��� %s��%r���ł� �Ƃ��낭 ����Ă��܂�", str);
	return 0;
}
u32 PutBadNameMessage(){
	PlaySG1(SG1_CANCEL);
	MenuFillBox(1, 4, 28, 9);
	Putsf("%3.6m�Ȃ܂��� ����Ă�������!");
	return 0;
}

u32 PutDelConf(){
	PlaySG1(SG1_CHANGE);
	MenuFillBox(0, 4, 29, 12);
	Putsf("%1.6m�ق�Ƃ��� �������債�܂���?%3rSTART�{�^��: ��������");
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
		msg ="�Z�[�u���܂���";
		break;
	case FLASH_E_NO_CART:
	case FLASH_E_ID:
		msg ="�J�[�g���b�W������܂���!";
		break;
	case FLASH_E_NO_STORAGE:
		msg ="�������J�[�h������܂���!";
		break;
	case FLASH_E_NO_FILE:
		msg ="�t�@�C�����݂���܂���!";
		break;
	case FLASH_E_SR_PROTECT:
		msg ="���C�g�v���e�N�g�G���[!";
		break;
	case FLASH_E_SR_WRITE:
		msg ="�������݃G���[!";
		break;
	case FLASH_E_NOT_FFFF:
	case FLASH_E_SR_ERASE:
		msg ="���傤����G���[!";
		break;
	case FLASH_E_COMPARE:
		msg ="�R���y�A�G���[!";
		break;
	case FLASH_E_MBR_ERROR:
	case FLASH_E_SECT_ERROR:
	case FLASH_E_CLST_ERROR:
	case FLASH_E_DAT_SIZE:
	case FLASH_E_CNUM_RANGE:
		msg ="�t�H�[�}�b�g�G���[!";
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
	if(neg == 2 && val < 1) val = 1; // �x�[�X���C��
	if(val >= range8) val = range8 - 1; // ��؂�X�y�[�X

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

// GPS�ʐM�̒�~�ƍĊJ
void StopGPSMode(){
	// UART���荞�݃X�L�b�v
	IW->mp.multiboot = 1;

	// ����DMA��~
	VoiceStop();
	EnableSound(SOUND_CH_VARIO, -1); // �o���I������~
}
void StartGPSMode(u32 baudrate){
	// UART���A
	REG16(REG_RCNT)		= 0; // SIO Enable
	REG16(REG_SIOCNT)	= UART_MODE | UART_FIFO_DISABLE; // Clear FIFO
	REG16(REG_SIOCNT)	= (baudrate & 3) | UART_DATA8 | UART_FIFO_ENABLE | UART_PARITY_NONE |
						  UART_SEND_ENABLE | UART_RECV_ENABLE | UART_MODE | UART_IRQ_ENABLE; // Garmin�p�ݒ�
	
	// UART���荞�ݕ��A
	IW->mp.multiboot = 0;
}

//FLog�̐擪2�o�C�g�̃A�h���X��Ԃ�
u32* GetFLogHead(u32 pos){
	if(pos < FLOG_COUNT){
		// �L���b�V���`�F�b�N
		u32* cache = IW->mp.flog_cache[pos];
		if(!*cache){
			// �L���b�V���Ƀf�[�^�i�[
			const u32* addr = IW->cif->ReadDirect(FI_FLOG_OFFSET + pos * FLOG_SIZE);
			if(addr){
				cache[0] = addr[0];
				cache[1] = addr[1];
			} else {
				cache[0] = 1;//����Magic��ݒ�
			}
		}
		if(*cache == FLOG_MAGIC_FLIGHT || *cache== FLOG_MAGIC_TASK) return cache; // �L���b�V���̃A�h���X��Ԃ�
	}
	return 0; // �f�[�^�Ȃ�
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

// ���[�g�����v�Z
u32 CalcRouteDistance(u32 i){
//	MenuFillBox(2, 4, 27, 12);
//	DrawText(4, 6, "�������������񂿂イ");
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
// ���j���[�\��
///////////////////////////////////////////////////////////////////////////////
// �w���v�s�̃X�N���[���\��
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

	// �X�N���[���\��
	IW->mp.help_vbc = IW->vb_counter + ((IW->mp.help_scr)? 8 : 60);
	p += IW->mp.help_scr / 2;
	if(!*p) IW->mp.help_scr = 0;
	else if(IsHalf(*p)) IW->mp.help_scr += 2;
	else if(!((IW->mp.help_scr += 1) & 1)) Putc2nd(*p++); // ���L�������炵

	// �w���v�\��
	while(IW->mp.gy < 20){
		if(!*p) p = IW->mp.help_msg; // �����߂�
		Putc(*p++);
	}
}

// �J�[�\���\��
void DispCursor(){
	// �ʒu�ύX����B�J�[�\���ƃw���v���X�V
	DispHelp(MENU_LIST[IW->mp.menuid][IW->mp.sel].help);
	MoveObj(OBJ_MENU_CURSOR, 1, IW->mp.sel * 16 + 10);
	MoveSelBg(IW->mp.sel, SELMENU_0);
}

// ���ԕ\����p
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
			case MENU_SEL_VAL: // ���l�I��
				{
					const IntInputTemplate* t = (IntInputTemplate*)mi[i].data;
					if(t->disp) PutsPointSGN(*t->val, 0, t->prec);
					else        PutsPoint   (*t->val, 0, t->prec);
					Puts(t->unit);
				}
				break;

			case MENU_SEL_TIME: // �����I��
				{
					const TimeInputTemplate* t = (TimeInputTemplate*)mi[i].data;
					PutsMin(*t->val);
				}
				break;
			
			case MENU_SEL_NAME: // ���O�I��
				{
					const NameInputTemplate* t = (NameInputTemplate*)mi[i].data;
					u32 j = 0;
					while(j < t->max && IsHalf(t->val[j])) Putc(t->val[j++]);
				}
				break;
			
			case MENU_SEL_LAT: // �ܓx�o�x
				{
					const LatInputTemplate* t = (LatInputTemplate*)mi[i].data;
					if(t->latlon) PutsLat(*t->val);
					else          PutsLon(*t->val);
				}
				break;
			
			case MENU_SEL_ENUM: // �񋓑I��
				{
					const EnumInputTemplate* t = (EnumInputTemplate*)mi[i].data;
					u32 sel = *t->val;
					if(sel >= t->max) sel = 0; // �O�̂���
					Puts(t->names[sel]);
				}
				break;

			case MENU_SEL_RTWPT: // ���[�g�E�F�C�|�C���g����
				{
					// �V�X�e����1�����Ȃ�
					Putsf("%d�|�C���g", IW->route_input.count);
				}
				break;
			
			case MENU_SEL_TASK: // �^�X�N(���[�g�\��)
				{
					// �V�X�e����1�����Ȃ�
					const Route* rt = GetTaskRoute();
					if(rt) PutsName(rt->name);
					else   Puts("�Ȃ�");
				}
				break;
			
			case MENU_SEL_START:// �X�^�[�g�ݒ�
				{
					// �V�X�e����1�����Ȃ�
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
			
			case MENU_SEL_SPEED: // �O���C�_�X�s�[�h�\����p
				if(IW->tc.spd_unit){
					Putsf("%1.1fkm/h", BiosDiv(IW->tc.my_ldK * IW->tc.my_down, 27777, &t));
				} else {
					Putsf("%1.1fm/s", BiosDiv(IW->tc.my_ldK * IW->tc.my_down, 100000, &t));
				}
				break;
			
			case MENU_SEL_PALETTE: // �p���b�g�ݒ��p
				PutsGammaLevel(IW->tc.palette_conf);
				break;

			case MENU_SEL_FLOG: // ���O�ݒ��p
				{
					u32* addr = GetFLogHead(i); // ���ۂɂ̓L���b�V����ǂ�
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
						Puts("�f�[�^�Ȃ�");
					}
				}
				break;
			}
		}
	}
	if(sel) max = 29;
	DrawBox(0, 0, max, i * 2 + 1);
	DispCursor();

	// ���ʃ��j���[�ɂ̓v�`����\��
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
		Putsf("%1.12m�J�[�g���b�W�����݂��ރ��O%r" "�������ł�������.");
		break;
	case MENU_ID_LOG_FL_SAVE:
		Putsf("%1.12m�t���C�g���O�̂������݃G���A%r" "�������ł�������.");
		break;
	case MENU_ID_LOG_TL_SAVE:
		Putsf("%1.12m�^�X�N���O�̂������݃G���A��%r" "�����ł�������.");
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////
// �u�[�g���b�Z�[�W����
///////////////////////////////////////////////////////////////////////////////
void SetBootMsgTimeout(u32 n){
	IW->mp.boot_msg = IW->vb_counter + n;
	if(IW->mp.boot_msg < 2) IW->mp.boot_msg = 2;
}
// �u�[�g��ʂ̏���
u32 CheckBootMsg(){
	if(IW->mp.boot_msg == 1){// �ڑ������҂�
		if(IW->gi.state > GPS_GET_PINFO_WAIT){
			PlaySG1(SG1_CONNECT);
			DrawTextCenter(10, "     Connected.     ");
			SetBootMsgTimeout(60 * 2);
		}
	} else {
		// ��ʏ����^�C���A�E�g�҂�
		if((s32)(IW->vb_counter - IW->mp.boot_msg) > 0) return 1;
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// ���ʃR�[���o�b�N�ɕύX����
///////////////////////////////////////////////////////////////////////////////
// �L�[�҂�
u32 SetKeyWaitCB(u16 push){
	if(push == 0xffff){
		IW->mp.proc = SetKeyWaitCB; // �R�[���o�b�N��ς���
		return 0;
	}
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	return 0;
}

// �_�E�����[�h�҂�
u32 MP_DownloadWpt(u16 push);
u32 MP_DownloadRoute(u16 push);
u32 SetDownloadCB(u16 push){
	if(push == 0xffff){
		if(IW->gi.state <= GPS_STOP_DOWNLOAD){
			PlaySG1(SG1_CANCEL);
			MenuFillBox(2, 4, 28, 9);
			DrawText(4, 6, "�_�E�����[�h�ł��܂���!");
			return SetKeyWaitCB(0xffff);
		}

		PlaySG1(SG1_COMP1);
		MenuFillBox(0, 3, 29, 19);
		Locate(2, 5);
		if(IW->mp.proc == MP_DownloadRoute){
			IW->gi.state = GPS_ROUTE;
			Puts("���[�g");
		} else if(IW->mp.proc == MP_DownloadWpt){
			IW->gi.state = GPS_WPT;
			Puts("WPT");
		}
		Puts("���_�E�����[�h���܂�");
		DrawText(4, 8, "(Select: ���イ����)");
		IW->gi.dl_accept = 1;
		MT_GPSCtrl();
		IW->mp.proc = SetDownloadCB;
		return 0;
	}

	if(push & KEY_SELECT){ // ���f
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
// �Ŋ�WPT�̌���
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
	IW->mp.cur_view = -1;	// �ĕ`��
	IW->mp.freeflight = 1;
}

void SetNaviStartPos(){
	// �t���[�t���C�g���[�h�̊J�n�ꏊ�̃Z�b�g
	IW->mp.nw_start.alt = RoundDiv(IW->px.alt_mm, 1000);
	IW->mp.nw_start.lat = IW->px.lat;
	IW->mp.nw_start.lon = IW->px.lon;
	IW->mp.nw_cnd_len = -1;
	IW->mp.nw_search  = 0;
}

u32 MP_SearchNearWpt(u16 push){
	if(IW->key_state == KEY_DOWN){// &���Z�͎g�킸�A�P�̃`�F�b�N
		PlaySG1(SG1_CHANGE);
		FillBox(2, 6, 26, 13);
		DrawTextCenter(9, "�t���[�t���C�g");
		IW->mp.ar_key = 0;		// �I�[�g���s�[�g��~
		InitNearNavi();
		SetNaviStartPos();
		return BacktoNavi();
	}

	s32 lat = IW->px.lat;
	s32 lon = IW->px.lon;

	if(push & 0x8000){
		push &= ~0x8000;
		if(push != KEY_LEFT && push != KEY_RIGHT && push != KEY_UP){
			IW->mp.ar_key = 0;		// �I�[�g���s�[�g��~
			return 0;
		}
		FillBox(2, 6, 26, 13);
		DrawTextCenter( 8, "�^�[�Q�b�g���񂳂�...");
		DrawTextCenter(10, "(Down�Ńt���[�t���C�g)");
		if(push != KEY_UP && IW->mp.nw_target < (u32)WPT->wpt_count){
			const Wpt* w = &WPT->wpt[IW->mp.nw_target];
			CalcDist(w->lat, w->lon, lat, lon, &IW->mp.nw_tgt_len, 0);
			IW->mp.nw_cnd     = IW->mp.nw_target;
			IW->mp.nw_s_dir   = (push == KEY_RIGHT)? 1 : 0;
		} else {
			// ��ԋ߂�WPT��T��
			if(IW->tc.at_mode){
				IW->mp.nw_tgt_len = IW->tc.at_min + IW->tc.cylinder;
				IW->mp.nw_s_dir   =  2; // �I�[�g�^�[�Q�b�g���[�h
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
			IW->mp.ar_key	  =  0;		// �I�[�g���s�[�g��~
		}
		SetNaviStartPos(); // �����J�n�ꏊ�̃Z�b�g
		IW->mp.proc = MP_SearchNearWpt; // �R�[���o�b�N��ς���
	}

	// �I�[�g�^�[�Q�b�g�̏���\��
	if(!IW->mp.nw_search && IW->mp.nw_s_dir > 1){
		FillBox(2, 6, 26, 13);
		Putsf("%4.8mRange:%dm-%dm%r" "�I�[�g�^�[�Q�b�g...", IW->mp.nw_tgt_len, IW->tc.at_max);
	}

	s32 end = IW->vb_counter + 1; // �^�C���A�E�g�ݒ�
	const Wpt* w = &WPT->wpt[IW->mp.nw_search];
	for(; IW->mp.nw_search < WPT->wpt_count ; ++IW->mp.nw_search, ++w){
		if(IW->mp.nw_s_dir != 3 && IW->mp.nw_search == IW->mp.nw_target) continue; // ���݂̃^�[�Q�b�g�͎���₩��O��
		u32 len;
		CalcDist(w->lat, w->lon, lat, lon, &len, 0);
		switch(IW->mp.nw_s_dir){
		case 0:
			// 1�߂�WPT��T��
			if((len < IW->mp.nw_tgt_len && (s32)len > (s32)IW->mp.nw_cnd_len) ||
               (len < IW->mp.nw_tgt_len && len == IW->mp.nw_cnd_len) ||
			   (len == IW->mp.nw_tgt_len && IW->mp.nw_search < IW->mp.nw_target)){
				IW->mp.nw_cnd     = IW->mp.nw_search;
				IW->mp.nw_cnd_len = len;
			}
			break;

		case 1:
			// 1����WPT��T��
			if(((s32)len > (s32)IW->mp.nw_tgt_len && len < IW->mp.nw_cnd_len) || 
			   (len == IW->mp.nw_tgt_len && len != IW->mp.nw_cnd_len && IW->mp.nw_search > IW->mp.nw_target)){
				IW->mp.nw_cnd     = IW->mp.nw_search;
				IW->mp.nw_cnd_len = len;
			}
			break;

		default:
			// �I�[�g�^�[�Q�b�g���[�h
			if((len >= IW->mp.nw_tgt_len && len <= IW->tc.at_max)){
				if(IW->mp.atcand_count < MAX_AUTOTARGET_CAND){
					IW->mp.atcand[IW->mp.atcand_count++] = (u16)IW->mp.nw_search;
				} else {
					// ������o�b�t�@����ꂽ��
					IW->mp.atcand[MyRand() & (MAX_AUTOTARGET_CAND - 1)] = (u16)IW->mp.nw_search; // ����ł͈�l�ɂȂ�Ȃ����A���܂茵������Ȃ��Ă��ǂ�
				}
			} else if(IW->tc.at_mode > 1 && !IW->mp.atcand_count){
				// �L�������W�ɖ����ꍇ�ɔ����āA�ł������W�ɋ߂��E�F�C�|�C���g���L�^���Ă���
				if(len < IW->tc.cylinder) len  = 0x7fffffff; // �V�����_���͒�D��
				else if(len < IW->mp.nw_tgt_len) len  = IW->mp.nw_tgt_len - len;
				else                        len -= IW->tc.at_max;
				if(IW->mp.nw_cnd == -1 || len < IW->mp.nw_cnd_len){
					IW->mp.nw_cnd = IW->mp.nw_search;
					IW->mp.nw_cnd_len = len;
				}
			}
			break;
		}

		// ���Ԑ؂ꒆ�f
		if(end - *(vs32*)&IW->vb_counter < 0){
			IW->intr_flag = 1;
			IW->mp.nw_search++;
			return 0;
		}
	}

	// �I�[�g�^�[�Q�b�g����
	if(IW->mp.nw_s_dir > 1){
		if(IW->mp.atcand_count){
			u32 mod;
			BiosDiv(MyRand(), IW->mp.atcand_count, &mod);
			IW->mp.nw_cnd = IW->mp.atcand[mod];
		} else if(IW->mp.nw_cnd == -1){
			// ��△���̏ꍇ�́A�^�[�Q�b�g�w�薳��(LD�Ɍ�����)
			Putsf("%4.10m�^�[�Q�b�g�Ȃ�!     ");
		}
	}

	// �^�[�Q�b�g�ύX
	if(IW->mp.nw_target == IW->mp.nw_cnd){
		PlaySG1(SG1_CANCEL);
	} else {
		switch(IW->mp.nw_s_dir){
		case 0:	PlaySG1 (SG1_PREV);		break;
		case 1:	PlaySG1 (SG1_NEXT);		break;
		default:PlaySG1X(SG1_CHANGE);	break; // �p�C��������D��
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
// �f�o�b�O����
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

	// �}�j���A�����c
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

	// �������c
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
		// �Z���^�����O
		if(*turn < 0)	*turn -= 3;
		else			*turn += 3;
		// �Z���^�����O��]���~�b�^
		if(*turn < -150)		*turn = -150;
		else if(*turn > 150)	*turn =  150;

		if(myAbs(*turn) > 100) *turn += (MyRand() & 0x3f) - 32; // �h��
	} else {
		// �^�[�Q�b�g
//		ang += 0x8000; // ZZZ �t���p
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
	if((t = MyRand() & 7) < 7) *turn += t - 3; // �h��

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
// �i�r���j���[
///////////////////////////////////////////////////////////////////////////////
#define LOCK_KEY (KEY_START | KEY_SELECT)
#define SUSPEND_KEY (KEY_START | KEY_SELECT | KEY_L | KEY_R)

u8* const VIEW_NAME[MAX_VIEW_MODE] = {
	"#1:�X�^���_�[�h",
	"#1:�X�^���_�[�h R1",
	"#1:�X�^���_�[�h R2",
	"#1:�X�^���_�[�h R3",
	"#2:�V���v��",
	"#2:�V���v�� R1",
	"#2:�V���v�� R2",
	"#2:�V���v�� R3",
	"#3:�^�[�Q�b�g",
	"#3:�^�[�Q�b�g R1",
	"#3:�^�[�Q�b�g R2",
	"#3:�^�[�Q�b�g R3",
	"#4:�p�C����x2",
	"#4:�p�C����x2 R1",
	"#4:�p�C����x2 R2",
	"#4:�p�C����x2 R3",
	"#5:�E�C���h",
	"#5:�E�C���h R1",
	"#5:�E�C���h R2",
	"#5:�E�C���h R3",
	"#6:*Cylinder",
	"#6:*Cylinder R1",
	"#6:*Cylinder R2",
	"#6:*Cylinder R3",
	"#7:*Locus",
	"#7:*Locus R1",
	"#7:*Locus R2",
	"#7:*Locus R3",
	"#8:�e�L�X�g",
	"#8:�e�L�X�g R1",
	"#8:�e�L�X�g R2",
	"#8:�e�L�X�g R3",
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
		// �ǂ̃��[�h��B�{�^���ŕ��A����
		if(push == KEY_B){
			PlaySG1(SG1_CANCEL);
			IW->mp.sp_enter |= s_mode; // �K�[�h�t���O
			IW->mp.sp_view = SP_VIEW_NONE;
			return 1;
		}
		// ���������`�F�b�N
		if((IW->mp.sp_view == mode && !cmd && (IW->mp.sp_enter & (1 << mode))) || IW->mp.sp_state == -1){
			PlaySG1X(SG1_CANCEL);
			IW->mp.sp_view = SP_VIEW_NONE;
			IW->mp.sp_enter &= ~s_mode;
			return 1;
		}
	} else {
		s32 s_mode = 1 << mode;
		// �R�}���h�{�^���`�F�b�N
		if((push & key) && IW->key_state == key){
			PlayModeSound(mode);
			// ��p���[�h�ؑ�
			IW->mp.ar_key = 0; // �I�[�g���s�[�g��~
			IW->mp.sp_view = mode;
			IW->mp.sp_enter &= ~s_mode;
			IW->mp.sp_state = 0;
			return 1;
		}
		// �R�}���h���̓`�F�b�N
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
			// ���b�Z�[�W�Ȃ��̃L�[���b�N����A+B��������Locus�ƃV�����_�����ɂ܂킷
			IW->mp.ar_key = 0; // �I�[�g���s�[�g�X�g�b�v
			IW->mp.sp_enter |= 1 << IW->mp.sp_view;
			IW->mp.sp_state = 0;
			if(IW->mp.sp_view++ >= SP_VIEW_CYL) IW->mp.sp_view = 0;
			PlayModeSound(IW->mp.sp_state);
			return 1; // �ύX
		}
		push = 0;
	}

	// �O�ՂƃV�����_�̗D��x�ݒ���ŏ��ɂ��Ă���
	s32 state[5] = {0};
	s32 f_locus = (IW->px.gstate & GSTATE_CENTERING);
	s32 f_cyl   = (IW->mp.cyl_dist && IW->mp.cyl_dist < IW->tc.cyl_near);

	if(f_locus && IW->tc.thermal_cmd && f_cyl && IW->tc.cyl_cmd && IW->tc.sp_prio){ // �v�D��x�`�F�b�N
		s32 prio = IW->tc.sp_prio;
		if(prio == 3) prio = (IW->vb_counter & (1 << 8))? 1 : 2; // �g�O��(1/60*2^8=4�b�g�O��)
		if((prio == 1 && IW->mp.sp_view == SP_VIEW_LOCUS && !(IW->mp.sp_enter & (1 << SP_VIEW_CYL))) ||
		   (prio == 2 && IW->mp.sp_view == SP_VIEW_CYL   && !(IW->mp.sp_enter & (1 << SP_VIEW_LOCUS)))){
			IW->mp.sp_enter &= ~(1 << IW->mp.sp_view);
			IW->mp.sp_view = SP_VIEW_NONE;
			if(prio == 1) f_locus = 0;
			else          f_cyl = 0;
		}
	}

	// �R�}���h�ɉ����ĊeSP���[�h�ɓ���
	state[1] = f_locus;
	if(CheckSpMode(SP_VIEW_LOCUS, push, KEY_LOCUS, state[IW->tc.thermal_cmd])) return 1;
	state[1] = f_cyl;
	if(CheckSpMode(SP_VIEW_CYL,			push, KEY_CYL,		 state[IW->tc.cyl_cmd])) return 4; 
//	if(CheckSpMode(SP_VIEW_TEXT,		push, KEY_TEXT,		 0)) return 5; // B+L�Ńe�L�X�g���[�h�ɓ��� �� ���j���[���ŕ\���BB+L��RFU�Ƃ��ĉ����B
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

	// �܂��A�X�y�V�������[�h�R�}���h���`�F�b�N
	if(CheckSpModeCommand(push)) return 0;

	// �L�[���b�N�`�F�b�N
	if(push & LOCK_KEY){
		if((IW->key_state & SUSPEND_KEY) == SUSPEND_KEY){
			MenuFillBox(3, 4, 26, 9);
			DrawTextCenter(6, "�T�X�y���h���܂�...");
			Suspend();
			MenuCls(0);
			return 0;
		}
		if((IW->key_state & LOCK_KEY) == LOCK_KEY){
			IW->mp.key_lock = !IW->mp.key_lock;
			FillBox(6, 6, 23, 11);
			DrawTextCenter(8, IW->mp.key_lock? "�L�[���b�N" : "���b�N��������");
			SetBootMsgTimeout(30);
		}
		return 0;
	}
	if(IW->mp.key_lock && push){
		if(!(IW->tc.auto_lock & 2)){
			FillBox(2, 6, 27, 11);
			DrawTextCenter(8, "START+SELECT�ŃA�����b�N");
			SetBootMsgTimeout(40);
		}
		push = 0;
		IW->mp.ar_key = 0; // �I�[�g���s�[�g��~
	}

	// �i�r��ʐؑ�
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

	// ���֐i�߂�
	if(push & KEY_CURSOR){
		push = IW->key_state & KEY_CURSOR;
		if(!WPT->wpt_count) return 0; // WPT������
		if(!IW->tc.task_mode) return MP_SearchNearWpt(push | 0x8000);
		const Route* rt = GetTaskRoute();
		if(push == KEY_RIGHT){ // &���Z�͎g���A�P�̃`�F�b�N
			PlaySG1(SG1_NEXT);
			if(rt->count <= ++IW->mp.cur_point) IW->mp.cur_point = 0;
		} else if(push == KEY_LEFT){// &���Z�͎g���A�P�̃`�F�b�N
			PlaySG1(SG1_PREV);
			if(rt->count <= IW->mp.cur_point || !IW->mp.cur_point--) IW->mp.cur_point = rt->count - 1;
		} else {
			return 0; // �P�̉����ȊO�͉������Ȃ�
		}
		IW->mp.navi_update |= NAVI_UPDATE_WPT;
		IW->mp.cur_view = -1;
		return 0;
	}

	// ���j���[�`�F�b�N
	if(push & KEY_A){
		PlaySG1(SG1_OK);
		IW->mp.boot_msg = 0;
		return 1;
	}

	// �u�[�g���b�Z�[�W����
	if(IW->mp.boot_msg){
		if(push || CheckBootMsg()){
			IW->mp.boot_msg = 0;
			MenuCls(0);
		}
	}

	// �x���|�b�v�A�b�v
	if(IW->tc.gps_warn && !IW->mp.boot_msg && IW->mp.sp_view != SP_VIEW_TEXT){
		const u8* warn = 0;
		if(IW->gi.state >= GPS_EMUL_FINISH)		warn = "PC�����񃂁[�h";
		else if(IW->gi.state >= GPS_EMUL_TRACK)	warn = "Track��������...";
		else if(IW->gi.state >= GPS_EMUL_ROUTE)	warn = "Route��������...";
		else if(IW->gi.state >= GPS_EMUL_WPT)	warn = "WPT��������...";
		else if(IW->gi.state >= GPS_EMUL_INFO)	warn = "PC�����񂩂���";
		else if(IW->px.fix == FIX_UNUSABLE)		warn = "GPS�^�C���A�E�g!";
		else if(IW->px.fix == FIX_INVALID)		warn = "�T�e���C�g���X�g";

		if(warn){ // �x������
			FillBox(5, 6, 24, 11); // ���ʃT�C�Y
			DrawTextCenter(8, warn);
			SetBootMsgTimeout(90); // 1.5�b�Ԃ͌x���\������
		}
	}
	return 0;
}

s32 CheckConfigDiff(){
	if(!CART_IS_WRITEABLE()) return 0; // �������ݕs�̃J�[�g���b�W
	if(IW->mp.save_flag & SAVEF_UPDATE_WPT) return 1; // ���[�g�ύX���͋���
	if(IW->mp.save_flag & SAVEF_UPDATE_CFG){// �R���t�B�O�ύX���͐ݒ��߂��Ă��Ȃ����`�F�b�N���Ă���
		// DirectMap���g���Ȃ�SCSD���̏ꍇ�ɂ́A�����`�F�b�N��葀�쐫�����D��c
		if(!CART_IS_DIRECTMAP()) return 1; // �t���O���f�݂̂ŁA�f�[�^�R���y�A�͏ȗ�

		// JoyCarry���̒��ǂ݉\�ȃJ�[�g���b�W�́A���ɐݒ��߂����ꍇ�Ƀ��b�Z�[�W���o���Ȃ��悤�`�F�b�N
		return memcmp(&IW->tc, IW->cif->ReadDirect(FI_CONFIG_OFFSET), sizeof(IW->tc));
	}
	return 0;
}

u32 MP_SaveCheck(u16 push){
	if(push == 0xffff){
		if(CheckConfigDiff()){
			FillBox(1, 2, 28, 14);
			Putsf("%3.4m�����Ă����ւ񂱂������%r"
				        "���܂�.  �Z�[�u���܂���?%3r"
						"A�{�^��: �Z�[�u%r"
						"B�{�^��: �Z�[�u���Ȃ�");
			IW->mp.mp_flag = 0;
			IW->mp.save_flag &= ~SAVEF_UPDATE; //�t���O�͗��Ƃ��Ă���
			return 0;
		}
		IW->mp.save_flag &= ~SAVEF_UPDATE;
		return MP_Navi(0xffff);
	}

	if(IW->mp.mp_flag == 1){ // �ۑ���
		if(push & (KEY_A | KEY_B)) return MP_Navi(0xffff);
		return 0;
	}

	// �ۑ��m�F��
	if(push & KEY_B) return MP_Navi(0xffff);
	if(!(push & KEY_A)) return 0;

	// �Z�[�u
	void SaveFlash();
	SaveFlash();
	IW->mp.mp_flag = 1;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// �T�X�y���h
///////////////////////////////////////////////////////////////////////////////
u32 MP_Suspend(u16 push){
	if(push == 0xffff){
		PlaySG1(SG1_CHANGE);
		MenuFillBox(1, 3, 28, 11);
		Putsf("%5.5m�T�X�y���h���܂���?%3r" "START�{�^��:�T�X�y���h");
		IW->mp.mp_flag = 0;
		return 0;
	}

	// �L�����Z���`�F�b�N
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}

	if(!(push & KEY_START)) return 0;

	// �T�X�y���h�J�n
	MenuFillBox(0, 3, 29, 10);
	DrawText(1,  6, "L+R�{�^���� ���W���[�����܂�");
	Suspend();
	return 1;
}


///////////////////////////////////////////////////////////////////////////////
// �o���I�e�X�g
///////////////////////////////////////////////////////////////////////////////
u32 MP_VarioTest(u16 push){
	// �ŏ��̌Ăяo��
	if(push == 0xffff){
		MenuFillBox(9, 4, 20, 12);
		DrawText(11, 6, "�e�X�g");
		Putc('��');
		IW->vm.vario_test = 0;
	} else if(push & (KEY_A | KEY_B)){ // ���A�{�^���`�F�b�N
		IW->vm.vario_test = VARIO_TEST_DISABLE;
		return 1; // MP�I��
	} else if(push & KEY_UP)	IW->vm.vario_test += 100;
	else if(push & KEY_DOWN)	IW->vm.vario_test -= 100;
	else if(push & KEY_LEFT)	IW->vm.vario_test += 1000;
	else if(push & KEY_RIGHT)	IW->vm.vario_test -= 1000;
	else if(push & KEY_L)		IW->vm.vario_test += 3000;
	else if(push & KEY_R)		IW->vm.vario_test -= 3000;
	else return 0;

	if(IW->vm.vario_test >  9900) IW->vm.vario_test =  9900;
	if(IW->vm.vario_test < -9900) IW->vm.vario_test = -9900;
	// �����ύX
	Putsf("%12.9m%+.3f%16Pm/s", IW->vm.vario_test);
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// �o�[�W�����\��
///////////////////////////////////////////////////////////////////////////////

u32 MP_Version(u16 push){
	MenuCls(SELMENU_BK1);
	FillBox(1, 4, 28, 12);
	DrawText( 7,  1, "--- �p���i�r ---");
	DrawText( 3,  6, "�o�[�W����: " NAVI_VERSION);
	DrawText( 3,  9, "����������: " __DATE__);
	DrawText( 0, 15, "COPYRIGHT(C) 2005-2008 Rinos.");
	DrawText(10, 17, "All rights reserved.");
	return SetKeyWaitCB(0xffff);
}

///////////////////////////////////////////////////////////////////////////////
// ���ݒn���g��\��(�e�L�X�g���[�h�̕\���𗘗p)
///////////////////////////////////////////////////////////////////////////////
u32 MP_LatLon(u16 push){
	if(push == 0xffff){
		// �r���[���[�h�������I�ɐ؂�ւ���
		IW->mp.mp_flag = (IW->mp.sp_view << 8) | (IW->tc.view_mode & 0xff); // �o�b�N�A�b�v
		IW->mp.sp_view = SP_VIEW_TEXT;
		MenuCls(0);
		DrawBox(0, 0, 29, 19);
		SetVideoMode(2);
	} else if(push == KEY_A){
		// ���݂̃i�r��ʊp�x�Ɖ���ʂ��g�O��
		PlaySG1(SG1_CHANGE);
		if(IW->tc.view_mode & 3) IW->tc.view_mode &= ~3;
		else                     IW->tc.view_mode |= IW->mp.mp_flag & 3;
	} else if(push == KEY_B){
		PlaySG1(SG1_CANCEL);
		// �\�����A
		IW->mp.sp_view   = IW->mp.mp_flag >> 8;
		IW->tc.view_mode = IW->mp.mp_flag & 0xff;
		SetVideoMode(0);
		return 1; // �I��
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Tips
///////////////////////////////////////////////////////////////////////////////
const u8* const TIPS[] = {
//	"SLELEC�{�^���������Ȃ��� TIPS���Ђ傤�������,�e�X�g���[�h�ɂȂ�܂�.",
	"�i�r���߂��START+SELECT��������,�L�[���b�N���܂�.",
	"�i�r���߂��B+�J�[�\����������,�r���[�^�C�v���ւ񂱂��ł��܂�.",
	"L+R�{�^���������Ȃ���u�[�g�����,�R���t�B�O�̃��[�h�����܂���.",
	"���[�X���[�h��Left�{�^����������,�܂��̃p�C�����ɂ��ǂ�܂�.",
	"���[�X���[�h��Right�{�^����������,���̃p�C�����ɂ����݂܂�.",
	"�t���[�t���C�g���[�h��Up/Left/Right�{�^����������,Goto���[�h�ɂȂ�܂�.",
	"Goto���[�h��Left�{�^����������,1��������WPT�Ƀi�r�Q�[�g���܂�.",
	"Goto���[�h��Right�{�^����������,1�Ƃ�����WPT�Ƀi�r�Q�[�g���܂�.",
	"Goto���[�h��Up�{�^����������,�����΂񂿂�����WPT�Ƀi�r�Q�[�g���܂�.",
	"Goto���[�h��Down�{�^����������,�t���[�t���C�g���[�h�ɂȂ�܂�.",
	"Goto���[�h�ł�WPT�T�[�`�������, �E�F�C�|�C���g������ �Ђꂢ���܂�.",
	"�p�C�����A���[�̂Ȃ�����,�Ƃ��������ǂ� �߂₷������킵�Ă��܂�.",
	"�i�r���߂��B+A�{�^����������, Locus���[�h�ɂȂ�܂�.",
	"�i�r���߂��B+R�{�^����������, �V�����_���[�h�ɂȂ�܂�.",
	"�i�r���߂��B+SELECT�{�^����������,�O���C�_�x���`�}�[�N���[�h�ɂȂ�܂�.",
	"�i�r���߂��B+START�{�^����������,�E�C���h�`�F�b�N���[�h�ɂȂ�܂�.",
	"�O���t�̃X�P�[����,Locus���j���[��\"�T���v�����O\"��,���ǂ����Ă��܂�.",
	0
};

u32 MP_Tips(u16 push){
	// �e�X�g���[�h�̃`�F�b�N
	if(IW->key_state & KEY_SELECT){
		IW->mp.menuid = MENU_ID_TEST;
		IW->mp.sel = 0;
		IW->mp.proc = 0;
		DispMenu(IW->mp.menuid);
		return 1;
	}

	// �ȍ~�A�{����Tips
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

//  �T���v�����[�g�f�o�b�O�o�^
u32 MP_TestRoute(u16 push){
	u32 AddAsagiriRoute();
	AddAsagiriRoute();

	Route* rt = GetTaskRoute();
	Wpt* w0 = GetCurWpt(rt, 0);
	IW->px.lat		= w0->lat + 30000; // 100�� 3m
	IW->px.lon		= w0->lon + 30000;
	IW->px.alt_mm	= w0->alt * 1000;
	IW->px.dtime	= 25 * 365 * 86400 + 23 * 3600; // �K��

	IW->mp.tlog.opt |= TLOG_OPT_MANUAL; // �Z�O�����g����
	IW->mp.tally.last_lat = IW->px.lat;
	IW->mp.tally.last_lon = IW->px.lon;
	return SetKeyWaitCB(0xffff);
}

///////////////////////////////////////////////////////////////////////////////
// �g���b�N���O
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
		// �f�o�b�O�p�����N���A
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

	DrawTextCenter(18, "(START�� �Ƃ��������N���A)");
	return 0;
}

u32 MP_DownloadLog(u16 push){
	if(push == 0xffff){
		if(IW->gi.state <= GPS_STOP_DOWNLOAD){
			PlaySG1(SG1_CANCEL);
			MenuFillBox(2, 4, 28, 9);
			DrawText(4, 6, "�_�E�����[�h�ł��܂���!");
			return SetKeyWaitCB(0xffff);
		}

		PlaySG1(SG1_COMP1);
		MenuFillBox(0, 3, 29, 17);
		Locate(2, 5);
		IW->gi.state = GPS_TRACK;
		IW->gi.dl_accept = 1;
		Puts("�g���b�N���O�̃_�E�����[�h");
		DrawText(4, 8, "(Select: ���イ����)");
		MT_GPSCtrl();
		return 0;
	}

	if(push & KEY_SELECT){ // ���f
		PlaySG1(SG1_CANCEL);
		IW->mp.tlog.abs_flag |= ABSF_SEPARETE; // �Z�p���[�^�}��
		IW->gi.state = GPS_STOP_DOWNLOAD;
		IW->gi.dl_accept = 0;
		IW->mp.navi_update |= NAVI_UPDATE_WPT;
		return 1;
	}

	if(IW->gi.state <= GPS_STOP_DOWNLOAD) return 0;
//	IW->gi.dl_accept = 1;

	IW->mp.tlog.abs_flag |= ABSF_SEPARETE; // �Z�p���[�^�}��
	PlaySG1(SG1_COMP2);
	return 1;
}

u32 MP_DownloadLogPre(u16 push){
	if(IW->tc.log_enable){
		if(push == 0xffff){
			PlaySG1(SG1_NEXT);
			MenuFillBox(0, 3, 29, 13);
			DrawText(1, 5,  "�_�E�����[�h���イ�� ���A��");
			DrawText(1, 7,  "�^�C���ق���� �Ă������܂�.");
			DrawText(3, 10, "START�{�^��: �_�E�����[�h");
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
		DrawText(1, 5,  "���̃|�C���g���� ���O��");
		DrawText(1, 7,  "�Ԃ񂩂��� ��낵���ł���?");
		DrawText(4, 10, "START�{�^��: �Ԃ񂩂�");
		return 0;
	}
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	if(!(push & KEY_START)) return 0;

	PlaySG1(SG1_COMP2);
	IW->mp.tlog.opt |= TLOG_OPT_MANUAL; // �Z�O�����g����
	return 1;
}

u32 MP_ClearLog(u16 push){
	if(push == 0xffff){
		PlaySG1(SG1_NEXT);
		MenuFillBox(0, 3, 29, 13);
		DrawText(1, 5,  "���ׂẴg���b�N���O��");
		DrawText(1, 7,  "�������債�� ��낵���ł���?");
		DrawText(4, 10, "START�{�^��: ��������");
		return 0;
	}
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	if(!(push & KEY_START)) return 0;

	PlaySG1(SG1_CLEAR);
	MenuFillBox(3, 4, 26, 9);
	DrawText(5, 6,  "�N���A���Ă��܂�...");
	s32 ret = TrackLogClear();
	if(ret){
		PutFlashError(ret);
		return SetKeyWaitCB(0xffff);
	}
	return 1;
}

// ���x�A�b�v�̂��߁A���荞�݂��g�킸���ړ]���������s��
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
		if(IW->key_state & KEY_SELECT) return 1; // ���f
	} while(REG16(REG_SIOCNT) & UART_SEND_FULL);
	REG16(REG_SIODATA8) = ch;
	if(sum) *sum -= ch;
	return 0; // ����
}
s32 RxPurge(){
	while(!(REG16(REG_SIOCNT) & UART_RECV_EMPTY)){
		if(IW->key_state & KEY_SELECT) return 1; // ���f
		REG16(REG_SIODATA8);
	}
	return 0;
}
s32 SendPacketHead(u8 pid, u16 len){
	u8 sum = 0;
	if(CheckSend(pid,      &sum)) return 1;
	if(CheckSend(len >> 8, &sum)) return 1;
	if(CheckSend(len,      &sum)) return 1;

	if(RxPurge()) return 1;// �����Ŏ�M���N���A���Ă���
	if(CheckSend(sum, 0)) return 1; // 1byte sum
	return 0; // ����
}
// offset��512�̔{���œn�����ƁI
s32 SendTrackData(u32 offset, u16 len){
	u8 sum = 0;
	if(CART_IS_DIRECTMAP()){
		// JoyCarry�͍����Ȓ�ROM�ǂ݂ŏ���
		const u8* phy = (u8*)IW->cif->ReadDirect(offset);
		while(len--) if(CheckSend(*phy++, &sum)) return 1;
	} else {
		// SCSD�̓o�b�t�@�ɓǂ݂Ȃ��瑗�M�Boffset/len��512�̔{���ł��邱��!
		for(; len ; len -= SECT_SIZE, offset += SECT_SIZE){
			const u8* p = (u8*)IW->cif->ReadDirect(offset); // 512byte�����L���łȂ�
			if(!p) return 1; // ���[�h�G���[
			u32 i;
			for(i = 0 ; i < SECT_SIZE ; ++i) if(CheckSend(*p++, &sum)) return 1;
		}
	}
	// sum���M
	if(CheckSend(sum, 0)) return 1;
	return 0; // ����
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
	if(!full_mode && tlp->pkt_counter < 0x100000) blk = MinS32(blk, tlp->blk_counter); // �L���u���b�N�̂ݑ��M
	
	MenuCls(SELMENU_BG);
	//				123456789012345678901234567890
	DrawTextCenter(0, "< Track Log Uploader >");
	DrawTextCenter(18, "(SELECT�{�^���ŃL�����Z��)");
	
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

	// �X�^�[�g�u���b�N�̌���
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
			if(SendPacketHead(PID_POLL | toggle, 0)) return 1; // ���f
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
				state = UPS_POLL; // �đ�
			}
			break;

		case UPS_BAUD:
			if(SendPacketHead(PID_BAUD | toggle, 0)) return 1; // ���f
			vbc = IW->vb_counter;
			state = UPS_BAUD_VAL;
			break;

		case UPS_BAUD_VAL:
			if(ch != -1){
				if(ch & ~3){ // NAK����
					PlaySG1(SG1_CANCEL);
					state = UPS_BAUD; // �đ�
					break;
				}
				if(ch && !IW->tc.bt_mode) { // �{�[���[�g�ύX��Bluetooth���g�p���̂�
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
				state = UPS_BAUD; // �đ�
			}
			break;

		case UPS_BAUD_WAIT:
			if(ch == PID_ACK){
				state = UPS_DATA;
				Putsf("OK%r" "Uploading...");
			} else if(ch == PID_NAK || IW->vb_counter - vbc > VBC_TIMEOUT(3)){
				PlaySG1(SG1_CANCEL);
//				state = UPS_BAUD; // �đ�
				state = UPS_DATA;
				Putsf("Skip%r" "Uploading...");
			}
			break;

		case UPS_DATA:
			if(SendPacketHead(PID_DATA | toggle, send_size + 1) || 
				SendTrackData(send_pos, send_size)) return 1; // ���f
			vbc   = IW->vb_counter;
			state = UPS_DATA_WAIT;
			break;

		case UPS_DATA_WAIT:
			if(ch == PID_ACK){
				toggle = 1 - toggle;
				send_pos += send_size;

				// �I�[�`�F�b�N(DirectMap�\�ȃJ�[�g���b�W�̂ݎ��s)
				// DirectMap�ł��Ȃ��ꍇ�́A�I�[�`�F�b�N����ق����x���Ȃ�c
				if(!full_mode && send_pos + BLOCK_SIZE > send_end && CART_IS_DIRECTMAP()){
					s32 check = 20; // �f�o�b�O�p���[�h�ł�OK?
					const u8* phy = (const u8*)IW->cif->ReadDirect(send_pos);
					while(check-- && phy[check] == 0xff);
					if(check < 0) send_end = send_pos; // �����ɏI�[�ɂ���
				}

				if(send_pos < send_end){
					state = UPS_DATA; // ����
					Putsf("%13.7m%d bytes", send_pos - send_start);
					break;
				} else {
					PlaySG1(SG1_CHANGE);
					state = UPS_END; // �I��
					Putsf(" OK%r");
				}
			} else if(ch == PID_NAK || IW->vb_counter - vbc > VBC_TIMEOUT(1)){
#define MIN_SEND_SIZE 512 // �Z�N�^�T�C�Y��菬�������Ȃ�
				if(send_size > MIN_SEND_SIZE) send_size >>= 1; // �T�C�Y�����������čđ�
				state = UPS_DATA; // �đ�
				PlaySG1(SG1_CANCEL);
			}
			break;

		case UPS_END:
			if(SendPacketHead(PID_END | toggle, 0)) return 1; // ���f
			vbc = IW->vb_counter;
			state = UPS_END_WAIT;
			break;

		case UPS_END_WAIT:
			if(ch == PID_ACK){
				toggle = 1 - toggle;
				Puts("Done.");
				return 1; // ����
			} else if(ch == PID_NAK || IW->vb_counter - vbc > VBC_TIMEOUT(1)){
				state = UPS_END; // �đ�
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
			DrawText(3, 5,  "���O�f�[�^������܂���!");
			return SetKeyWaitCB(0xffff);
		} else {
			MenuFillBox(0, 3, 29, 11);
			DrawText(3, 5,  "PC�փA�b�v���[�h���܂���?");
			DrawText(3, 8,  "START�{�^��: �A�b�v���[�h");
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

		// �A�b�v���[�h
		UploadLog();

		StartGPSMode(IW->tc.bt_mode);
		return 1;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// ���[�g/�E�F�C�|�C���g�A�b�v���[�h(�e�X�g�p�ɍ쐬����etrex�n��p�A�b�v���[�_)
///////////////////////////////////////////////////////////////////////////////
u32 UploadTest(u32 mode){
	MenuFillBox(2, 3, 27, 8);
	if(IW->gi.state < GPS_EMUL_INFO || GPS_EMUL_FINISH <= IW->gi.state){
		void GpsEmuMode(u32 mode, const u16* snd);
		// �����Ȃ�J�n
		GpsEmuMode(mode, SG1_COMP1);
		DrawTextCenter(5, "�A�b�v���[�h������");
	} else {
		PlaySG1(SG1_CANCEL);
		DrawTextCenter(5, "�������񂿂イ!");
	}
	return SetKeyWaitCB(0xffff);
}
u32 MP_UploadWpt  (u16 push) { return UploadTest(GPS_EMUL_WPT);  }
u32 MP_UploadRoute(u16 push) { return UploadTest(GPS_EMUL_ROUTE);}

///////////////////////////////////////////////////////////////////////////////
// �J�[�g���b�W
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

// RAM�u�[�g�p
const RomArea RA_RAM[] = {
	{0x00000001,	0x02000000, 	1024 * 256},
	{0, 0, 0}
};

// �W���C�L�����[�ւ̕����p
const RomArea RA_CART1[] = {
	// �v���O����
	{0x00000001,	0x09f00000, 	1024 * 64},
	{0x00010001,	0x09f10000, 	1024 * 64},
	{0x00020001,	0x09f20000, 	1024 * 64},
	{0x00030001,	0x09f30000, 	1024 * 64},

	// ����
	{0x00000002,	0x09f40000, 	1024 * 64},
	{0x00010002,	0x09f50000, 	1024 * 64},
	{0x00020002,	0x09f60000, 	1024 * 64},
	{0x00030002,	0x09f70000, 	1024 * 64},
	{0, 0, 0}
};

// �W���C�L�����[�����p(�B�����[�h)
const RomArea RA_CART2[] = {
	// �v���O����
	{0x00000001,	0x09f00000, 	1024 * 64},
	{0x00010001,	0x09f10000, 	1024 * 64},
	{0x00020001,	0x09f20000, 	1024 * 64},
	{0x00030001,	0x09f30000, 	1024 * 64},

	// ����
	{0x00000002,	0x09f40000, 	1024 * 64},
	{0x00010002,	0x09f50000, 	1024 * 64},
	{0x00020002,	0x09f60000, 	1024 * 64},
	{0x00030002,	0x09f70000, 	1024 * 64},

	// �^�X�N�ݒ�
	{0x00000004,	0x09ff0000, 	1024 * 8},

	// ���[�g/�E�F�C�|�C���g
	{0x00000006,	0x09ff4000, 	1024 * 8},
	{0x00002003,	0x09ff6000, 	1024 * 8},
	{0x00004003,	0x09ff8000, 	1024 * 8},
	{0x00006003,	0x09ffa000, 	1024 * 8},

	// SCSD�̏ꍇ�̓R�R�܂ł����R�s�[���Ȃ�

	// �t���C�g���O
	{0x00000005,	0x09ff2000, 	1024 * 8},

	// �g���b�N
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
		if(val == -1) return -1;	// �L�����Z��
		if(val == 0xaaaa) return -2;// ��������
		if((val & 0xf000) == 0x1000) return val & 0xfff; // ACK��M
	}
}

u32 SendRecv32(u32 val){
	REG32(0x4000120) = val;
	REG16(REG_SIOCNT) |= 0x80;
	while(REG16(REG_SIOCNT) & 0x80){
		if(IW->key_state & KEY_SELECT) return -1;
	}
	return REG32(0x4000120); // -1�͕Ԃ��Ȃ�!
}

void WaitMS(u32 ms){
	ms = (ms >> 4) + 1; // ����ŏ\��
	u32 cur = IW->vb_counter;
	while(IW->vb_counter - cur <= ms) BiosHalt();
}

const u32 BOOTBIN_ADDR[] = {
	0x09ffc000, // JoyCarry
	((u32)&__iwram_overlay_lma) + (0x08000000 - 0x02000000) // SuperCard����
};
u32 MBoot(){
	SetCommMode(SIO_MPLAY_MASTER);

	MenuCls(SELMENU_BG);
	//				123456789012345678901234567890
	DrawText(0, 0, "1.GBA���P�[�u�����������܂�.");
	DrawText(0, 2, "  (�R�s�[����: �O���C�R�l�N�^)");
	DrawText(0, 4, "2.START+SELECT�{�^���������Ȃ�");
	DrawText(0, 6, "  ��,�R�s�[�������u�[�g���܂�.");
	DrawTextCenter(18, "SELECT�{�^���ŃL�����Z��");

	// �ŏ��Ƀu�[�g�Z�N�^�̃u�[�g�R�[�h�𑗂荞�� /////////////////////////////
	PlaySG1(SG1_OK);
	FillBox(2, 9, 27, 16);
	DrawTextCenter(11, "  Connecting...  ");
	u32 id, val;
	u32 vbc = IW->vb_counter;
	while(((id = SendRecv(0x6200)) & 0xfff0) != 0x7200){
		if(id == -1) return 1; // �L�����Z���{�^��

		// ��莞�ԑ҂��Ă��ʐM�ł��Ȃ��Ƃ��ɂ́AUART�����������ă��g���C����
		if(IW->vb_counter - vbc > VBC_500ms){
			WaitMS(100);
			SetCommMode(SIO_MPLAY_MASTER);// ���Z�b�g
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


	// �f�[�^���M /////////////////////////////////////////////////////////////
	DrawTextCenter(11, "Checking target...");
	WaitMS(1);
	SetCommMode(SIO_MPLAY_MASTER); // �O�̂��ߍĐݒ�

	// �ڑ�����擾(�J�[�g���b�W�L��)
	val = WaitAck(0);
	const RomArea* rap = (val & 0xf)? RA_CART1 : RA_RAM;
	if(IW->key_state & KEY_UP) rap = RA_CART2; // �������S�R�s�[

	// �v���O���X�\���p
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

	// �f�[�^���M
	for(;;){
		// ����
		val = SendRecv(0x5555);
		if(val == -1) return 1;
		if(val != 0xaaaa) continue;

		// �T�C�Y���擾
		s32 size = rap->size;
		s32 dst  = rap->dst_addr;
		u16* src = (u16*)(IW->cif->GetCodeAddr(rap->src_dat & 0xf));
		if(!src) size = dst = 0; // �����I��
		SendRecvX2(size); // 0�T�C�Y�ʒm�őΌ����u���������o���Ď������u�[�g����B
		SendRecvX2(~size);
		SendRecvX2(dst);
		SendRecvX2(~dst);
		val = WaitAck(2);
		if(val == -2) continue; // �ē���
		if(!size) break;

		// �f�[�^���M
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

		// BCC�`�F�b�N
		SendRecvX2(bcc);
		val = WaitAck(4);
		if(val != 1) continue;

		// �������ݑ҂�
		val = WaitAck(6);
		if(val != 2) continue;

		// �u���b�N�]������
		cur_size += rap->size;
		rap++; // ���̃f�[�^��
	}

	return 0; // success
}

///////////////////////////////////////////////////////////////////////////////
// �l���΍�̊��S����
///////////////////////////////////////////////////////////////////////////////
const u32 CLEAR_8192[] = {
//	FI_CONFIG_OFFSET, // Config�ɂ͌l���͊܂܂�Ȃ��̂ō폜���Ȃ��B
	FI_FLOG_OFFSET,
	FI_ROUTE_OFFSET + BLOCK_TASK_SIZE * 0,
	FI_ROUTE_OFFSET + BLOCK_TASK_SIZE * 1,
	FI_ROUTE_OFFSET + BLOCK_TASK_SIZE * 2,
	FI_ROUTE_OFFSET + BLOCK_TASK_SIZE * 3,
	-1
};

// �����f�[�^�ł̏㏑�����K�v?
u32 FullClear(){
	// 8KB�u���b�N����
	const u32* p = CLEAR_8192;
	while(*p != -1){
		s32 ret = IW->cif->EraseBlock(*p++, BLOCK_TASK_SIZE, ERASE_CLEAR);
		if(ret) return ret;
	}

	// 64KB�g���b�N���O����
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
		DrawTextUL(1, 1, "�����񂶂傤�ق��� ���傫��");
		DrawText(2, 6, "Route/Waypoint/Track/Log��");
		DrawText(2, 8, "���񂺂�� ���傫�����܂�.");
		DrawText(4,11, "START�{�^��: ���傫��");
		DrawText(4,13, "B�{�^��:     �L�����Z��");
		IW->mp.proc = MP_Initialize; // �R�[���o�b�N��ς���
		return 0;
	}
	if(push & KEY_START){
		PlaySG1(SG1_CLEAR);
		MenuFillBox(2, 6, 27, 13);
		DrawTextCenter(8, "���傫�� ���Ă��܂�...");

		// ���S����
		s32 ret = FullClear();
		if(ret){
			PutFlashError(ret);
			return SetKeyWaitCB(0xffff);
		}

		// �����̃f�[�^�����������Ă���
		ROUTE->route_count = 0;
		WPT->wpt_count = 0;
		InitLogParams();
		push = KEY_A;
	}
	if(push & (KEY_A | KEY_B)){
		u32 MP_Cart(u16 push);
		IW->mp.proc = MP_Cart; // �R�[���o�b�N��߂�
		PlaySG1(SG1_CANCEL);
		return MP_Cart(0xffff);
	}
	return 0;
}

#define CARTTEST_OFFSET FI_CONFIG_OFFSET	// �R���t�B�O�u���b�N
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
		DrawTextUL(6, 4, "�J�[�g���b�W�e�X�g");
		DrawText  (6, 7, "START�{�^��:������");
		return 0;
	}
	if(push & KEY_START){
		VoiceStop(); // �e�X�g�ɉ����o�b�t�@���g�����߉���DMA��~
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

		DmaClear(3, 0x55aaaa55, VOICE_ADDRESS, VOICE_BUF_SIZE, 32); // �e�X�g�p�^�[����55aaaa55
		Putsf("%9.12mWrite: "); // JoyC:301ms, SCSD-UMAX:251ms
		t = IW->anemo.tm; // 16.78KHz
		for(i = 0 ; i < TEST_NUM ; ++i){
			ret = IW->cif->WriteData(CARTTEST_OFFSET, (void*)VOICE_ADDRESS, CARTTEST_SIZE); // �R�s�[���͉����p�o�b�t�@
			if(ret) break;
			ret = IW->cif->Flush();
			if(ret) break;
		}
		PutsPerf(ret, t);

		Putsf("%9.14mRead:  "); // JoyC:0ms, SCSD-UMAX:33ms
		t = IW->anemo.tm; // 16.78KHz
		for(i = 0 ; i < TEST_NUM ; ++i){
			ret = IW->cif->ReadData(CARTTEST_OFFSET, (void*)VOICE_ADDRESS, CARTTEST_SIZE, 0); // �R�s�[��͉����p�o�b�t�@
			if(ret) break;
		}
		PutsPerf(ret, t);

		// �ݒ�������߂��Ă���
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
	VoiceStop(); // �e�X�g�ɉ����o�b�t�@���g�����߉���DMA��~
	PlaySG1(SG1_SELECT);
	Putsf("%.mSECTOR DUMP%d %08x:%08x", IW->mp.disp_mode & 2, IW->mp.mp_flag >> 9, IW->mp.mp_flag + (IW->mp.scr_pos << 3));
	s32 ret = IW->cif->ReadData(IW->mp.mp_flag, (void*)VOICE_ADDRESS, SECT_SIZE, (IW->mp.disp_mode & 2)? 2 : 1); // �R�s�[��͉����p�o�b�t�@
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
		DrawTextCenter(1, "< �J�[�g���b�W >");
		DrawText(0,  6, " Config Area:   ");
		PutsHeadInfo(FI_CONFIG_OFFSET, FLBL_TASK_MAGIC);
		DrawText(0,  8, " Route/Wpt Area:");
		PutsHeadInfo(FI_ROUTE_OFFSET, FLBL_ROUTE_MAGIC);
		if(CART_IS_WRITEABLE()){
			Putsf("%.4m Generation:    %d", GEN_COUNTER);
			DrawText(0, 11, "L+R�{�^����������");
			DrawText(4, 13, "�C�j�V�����C�Y�ł��܂�.");
		}
		if(CART_IS_DUPLICATE()){
			DrawText(0, 16, "START�{�^����������");
			DrawText(4, 18, "�ׂ�GBA�ɃR�s�[�ł��܂�.");
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

		// ����
		if(MBoot())	PlaySG1(SG1_CANCEL);
		else		PlaySG1(SG1_COMP2);

		StartGPSMode(IW->tc.bt_mode);
		MP_Cart(0xffff);
//		return SetKeyWaitCB(0xffff);
		return 0; // ���̂܂܏I��
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// �R���t�B�O������
///////////////////////////////////////////////////////////////////////////////
u32 MP_Reload(u16 push){
	if(push == 0xffff){
		PlaySG1(SG1_NEXT);
		MenuFillBox(0, 3, 29, 13);
		DrawText(1, 5,  "���ׂĂ̂����Ă��� ���傫��");
		DrawText(1, 7,  "���� ��낵���ł���?");
		DrawText(4, 10, "START�{�^��: ���傫��");
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
	IW->mp.save_flag |= SAVEF_UPDATE_CFG; // �ݒ�ύX�t���O
	return 1;
}
///////////////////////////////////////////////////////////////////////////////
// �Z�[�u
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
	DrawText(5, 6, "�Z�[�u���Ă��܂�...");

	// ����DMA��~
	VoiceStop();
	EnableSound(SOUND_CH_VARIO, -1); // �o���I���͎~�߂Ă���
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
// ���ʏ��
///////////////////////////////////////////////////////////////////////////////
// �X�y�[�X���s�ŏo��
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
// ���j�b�g���
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
		"DirHV:%4.3f��;H%4.3f��;V%r"
		"VelHV:%4.3fm/s%4.3fm/s;V%r"
		"Date:  %T",
		lat, px->lon, px->alt_mm, px->epe_mm, px->eph_mm, px->epv_mm, GetAngK(px->h_ang_c),
		va, px->vh_mm, px->up_mm, px->dtime);

	if(px->dtime) Putsf(" (%s)", WEEK_NAME[px->week]);

	u32 m, h = BiosDiv(IW->tc.tzone_m, 60, &m);
	Putsf("%rTime:  %t (UTC%+d:%02d)", px->dtime, h, m);
	return;
}

// �ʐM��ԃ`�F�b�N
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

// GPS�`�F�b�N
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
// �A�l�����[�^
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
// �t���C�g����
///////////////////////////////////////////////////////////////////////////////
// ���̓��j���[�p�o�͊֐�
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
	if(f)	Putsf("%8d", RoundShift(turn + num * CETERING_DETECT, 16)); // ���o�܂ł̉�]�ʂ����Z���Čv�Z
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
		if(sum > 10000000){ // �����ӂ��������Ƃł��h��
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

// �t���C�g����
void PutClearMessage(u32 val){
	if(val == -1){
		DrawText(1, 16, "START�{�^���Ń��[�g��������");
		DrawText(0, 18, "(���܂̃��O�͂������傳��܂�)");
	} else if(val){
		Putsf("%0.18mL<=  %T  %t  =>R", val, val);
	} else {
		DrawText(0, 18, "L<=    (START�ŃN���A)     =>R");
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
	SetVideoMode(0); // �O���t��p���[�h����
}

// ����: Raise/Sink
// Vario/-.5 0 .0 1 1.5 4.5
// �ō��l�A���ϒl
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
			(u32)((tl->sum_v + 500) / 1000), (u32)((tl->trip_mm + 500) / 1000), RoundDiv(tl->sum_gain, 1000)); // 64bit�l���g�p���Ă���!
}

// ���[�����O�A�s�b�`���O
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

// �J�E���^
void UpdateTally4(){
	const Tally* tl = GetTallyData();
	DrawTextUL(1,  3, "Status statistics:");
	Putsf("%3.6m"
			"Straight   =%+8S%r" // �䗦�t����t�H�[�}�b�g�I
			"Left turn  =%+8S%r"
			"Right turn =%+8S%r"
			"H Stop     =%+8S%r"
			"Pre flight =%+8S%r",
			tl->s_count, tl->count, // 2�R�g
			tl->turn_l,  tl->count,
			tl->turn_r,  tl->count,
			tl->w_count, tl->count,
			tl->count - (tl->s_count + tl->w_count + tl->turn_l + tl->turn_r), tl->count);
}


// �^�C��
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

	SetVideoMode(1); // �O���t��p���[�h
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

	SetVideoMode(1); // �O���t��p���[�h
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

	SetVideoMode(1); // �O���t��p���[�h
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

	DrawText(21,  5, "Step 15��");
	DrawText( 5, 16, "N   E   S   W");
	if(!sum) sum = 1;
	DrawText(20,  8, "N:");	PutPercent2(merge[0], sum);
	DrawText(20, 10, "E:");	PutPercent2(merge[1], sum);
	DrawText(20, 12, "S:");	PutPercent2(merge[2], sum);
	DrawText(20, 14, "W:");	PutPercent2(merge[3], sum);

	SetVideoMode(1); // �O���t��p���[�h
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
		IW->mp.proc = MP_Tally; // �R�[���o�b�N��ς���
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
// �t���C�g���O�ۑ�
///////////////////////////////////////////////////////////////////////////////
u32 CalcCheckSum32(u32* start, u32* end){
	u32 sum = 0;
	while(start < end) sum += *start++; // word sum
	return sum;
}

u32 SaveFLog(u32 pos, void* addr, u32 size, u32 msg_flag){
	if(pos == -1){ // �����Z�[�u����
		u32 min = -1, i = 4;
		while(i--){
			u32* p = GetFLogHead(i); // �L���b�V�����g��
			if(!p){ // ���O�f�[�^�Ȃ�
				min = 0;
				pos = i; // �ŏ��l
			} else { // �Â��f�[�^����
				if(p[1] <= min){ // ��ԌÂ����̂Ȃ�X�V
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

	u32* addr32 = (u32*)addr; // �K��4byte���E
	if(!addr32[1]){
		if(msg_flag){
			MenuFillBox(2, 4, 27, 9);
			DrawTextCenter(6, "�܂�,���O������܂���!");
		}
		return -2;
	}
	if(msg_flag){
		MenuFillBox(3, 4, 25, 9);
		DrawText(5, 6, "�Z�[�u���Ă��܂�...");
	}

	// ����DMA��~
	VoiceStop();
	EnableSound(SOUND_CH_VARIO, -1); // �o���I���͎~�߂Ă���

	// Voice�G���A�ɏ������݃f�[�^���\�z
	u32 offset = FI_FLOG_OFFSET;
	u32 dst    = VOICE_ADDRESS;
	if(CART_IS_SECTWRITE()){
		// �ύX���O�����݂̂̏���������OK
		DmaClear(3, 0, VOICE_ADDRESS, FLOG_SIZE, 32); // �܂�0�N���A
		offset += FLOG_SIZE * pos; // �������ݐ�A�h���X���V�t�g
	} else {
		// 8KB�u���b�N�S�̂���������邽�߁A�������������������K�v
		IW->cif->ReadData(FI_FLOG_OFFSET, (void*)VOICE_ADDRESS, FLOG_TOTAL, 0);
		dst += FLOG_SIZE * pos; // �������݌��A�h���X���V�t�g
	}

	// RAM��Ń`�F�b�N�T���쐬
	DmaCopy (3, addr, dst, size, 32); // ���O��WRAM�ɃR�s�[(CheckSum�������ݗp)
	u32* end  = (u32*)(dst + FLOG_SIZE - sizeof(u32));
	*end = -CalcCheckSum32((u32*)dst, end);// �`�F�b�N�T����ۑ�

	// �J�[�g���b�W��������
	s32 ret = IW->cif->WriteData(offset, (void*)VOICE_ADDRESS, CART_IS_SECTWRITE()? FLOG_SIZE : FLOG_TOTAL);
	if(msg_flag){
		PlaySG1(ret? SG1_CANCEL : SG1_CHANGE);
		PutFlashError(ret);
	}

	// �L���b�V�����X�V
	u32* cache = IW->mp.flog_cache[pos];
	cache[0] = addr32[0];
	cache[1] = addr32[1];

	// ���̎����Z�[�u�̃G���A���L��
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
		// �}�W�b�N�m�F
		if(!GetFLogHead(IW->mp.sel)){
			// �����f�[�^
			PlaySG1(SG1_CANCEL);
			MenuFillBox(1, 4, 27, 9);
			DrawText(3, 6, "���O�f�[�^������܂���!");
			return SetKeyWaitCB(push);
		}
		// �`�F�b�N�T���m�F
		if(IW->cif->ReadData(FI_FLOG_OFFSET + IW->mp.sel * FLOG_SIZE, FLOG_PTR, FLOG_SIZE, 0)){
			// �����f�[�^
			PlaySG1(SG1_CANCEL);
			MenuFillBox(1, 4, 27, 9);
			DrawText(3, 6, "���O�f�[�^��݂��݃G���[");
			return SetKeyWaitCB(push);
		}
		IW->mp.flog_addr = FLOG_PTR; // ���O�\���|�C���^��؂�ւ�
		if(CalcCheckSum32(IW->mp.flog_addr, IW->mp.flog_addr + (FLOG_SIZE >> 2))){
			MenuFillBox(3, 2, 27, 12);
			DrawText(5, 4, "<�`�F�b�N�T�� �G���[>");
			DrawText(5, 7, "A�{�^��: �f�[�^���݂�");
			DrawText(5, 9, "B�{�^��: �L�����Z��");
			return 0;
		}
	}
	if(push & KEY_A){ // �`�F�b�N�T��OK���͒��ڂ����ɓ���
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
// �O���t
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
		SetVideoMode(1); // �O���t��p���[�h
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		SetVideoMode(0); // ���[�h���A
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

	// 2%�}�[�W���t�������W�ݒ�
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
// �^�X�N���O
///////////////////////////////////////////////////////////////////////////////
u32 MP_Task(u16 push){
	if(push == 0xffff){
		IW->mp.mp_flag = 0;
		IW->mp.proc = MP_Task; // �R�[���o�b�N��ς���
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
			if(IW->px.gstate) UpdateTaskLog(0); // �e�C�N�I�t�ς́A�����e�C�N�I�t�ɍX�V
		}
	} else {
		return 0;
	}

	// �y�[�W����`�F�b�N
	const TaskLogData* tld = GetTaskData();
//	const Route*       rt  = &IW->route_input;
	if(IW->mp.mp_flag  == -1)               IW->mp.mp_flag = tld->rt_count;
	else if(IW->mp.mp_flag > tld->rt_count) IW->mp.mp_flag = 0;
	const TaskLog1* tl1  = &tld->tl1[IW->mp.mp_flag];		// �K������
	const TaskLog2* tl2  = GetTL2Addr((TaskLogData*)tld, IW->mp.mp_flag); // ���[�g�T�C�Y�ɂ���Ă͖����ꍇ������

	// �S�y�[�W�̋��ʓ��e���o��
	MenuCls(SELMENU_BG);
	u32 dtime_val = IsCurFlog()? 0 : tld->log_time;// �ŉ��i�̓��t�\���p
	if(!dtime_val && (u32)IW->mp.pre_route == -1){
		// �����܂��^�X�N���O�ɋL�^���Ȃ���΁A�^�X�N���O�������ōX�V����
		if(tld->update_mark & 2){ // �ύX����? (takeOff�͏���)
			dtime_val = -1; //�X�V���b�Z�[�W
		} else {
			InitTaskLog(1); // �����X�V
		}
	}
	// �^�X�N��
	Putsf("%5m< Task log %d/%d >", IW->mp.mp_flag, tld->rt_count);

	// ����/�e�C�N�I�t���x
	DrawText(2, 5, IW->mp.mp_flag? "Arrival Alt. = " : "Takeoff Alt. =");
	if(tl1->lat) Putsf("%8dm", tl1->alt);
	else         Puts(" -------m");
	// ����/�e�C�N�I�t����
	DrawText(2, 7, IW->mp.mp_flag? "Arrival time =  " : "Takeoff time = ");
	Putsf("%t", tl1->dtime);

	// �y�[�W�ʂ̕\��
	if(IW->mp.mp_flag){
		// �E�F�C�|�C���g���̕\��
		Locate(0, 3);
		if(tl2){ // �E�F�C�|�C���g��񂠂�
			PutsName(tl2->wpt_name);
			Putsf("/Alt.%dm", tl2->wpt_alt);
		} else { // �E�F�C�|�C���g���Ȃ�
			Putsf("Pylon#%d", IW->mp.mp_flag);
		}
		Puts(" Cyl.");
		PutsDistance2(tl1->cyl);

		// �t���C�g����
		if(tl1->lat){
			Putsf("%2.9mTrip meter   =%9dm", tl1->trip);

			const TaskLog1* tl1p = &tld->tl1[IW->mp.mp_flag - 1];
			s32 t = tl1->dtime - tl1p->dtime;
//			if(tl1p->seq + 1 == tl->seq){ // ����������
			if(IW->mp.mp_flag == 1 && (tld->pre_pylonX & 0x80)){
				// �e�C�N�I�t
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

				// �X�^�[�g�p�C��������̃g�[�^�����Ԃ�\��
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
		// �\��
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
// �E�F�C�|�C���g�ǉ�/�ǉ��폜
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

	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ���[�g/WPT�ύX�t���O���Z�b�g
	IW->wpt_sort_type = GetSortType(); // �ă\�[�g�̂��߂̃��Z�b�g
	return WPT_ADD_SUCCESS;
}

u32 WptDelCheck(u32 id){
	// �O�̂��߃`�F�b�N
	if(id >= WPT->wpt_count) return WPT_DEL_ERROR;

	// �I���V�t�g
	if(WPT->wpt_count > 2){
		u16* sort = IW->wpt_sort;
		u32 t = IW->mp.wpt_sel + 1; // �f�t�H���g�͎��̒l
		t = sort[(t >= WPT->wpt_count)? 0 : t]; // �Ō�̏ꍇ�͐擪�̒l
		sort[0] = (t <= id)? t : (t - 1); // 0�ɒ��ڎ���̒l������
		IW->mp.wpt_sel = 0; // sort[0]���w��
	}
	IW->wpt_sort_type = GetSortType(); // �ă\�[�g�̂��߂̃��Z�b�g

	// �E�F�C�|�C���g�폜
	u32 i = id, j;
	for(--WPT->wpt_count ; i < WPT->wpt_count ; ++i){
		WPT->wpt[i] = WPT->wpt[i + 1];
	}

	// ���[�g�S�T����Index�Čv�Z(����WptIsUsed�Ń`�F�b�N�ςł��邱�Ƃ�O��ɂ��Ă���c)
	for(i = 0 ; i < ROUTE->route_count ; ++i){
		Route* rt = &ROUTE->route[i];
		for(j = 0 ; j < rt->count ; ++j) if(rt->py[j].wpt > id) --rt->py[j].wpt;
	}

	// �����f�B�O���V�t�g���Ă���
	if(id < WPT->def_ld) WPT->def_ld--;

	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ���[�g/WPT�ύX�t���O���Z�b�g
	return WPT_DEL_SUCCESS;
}

// �E�F�C�|�C���g�����[�g�Ɋ܂܂�Ă��邩�`�F�b�N(���[�g�S�T��)
s32 WptIsUsed(u32 id, u32 begin){
	for(; begin < ROUTE->route_count ; ++begin){
		u32 j;
		Route* rt = &ROUTE->route[begin];
		for(j = 0 ; j < rt->count ; ++j) if(rt->py[j].wpt == id) return begin; // ���[�g�Ŏg�p��
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////
// �E�F�C�|�C���g���蓮�Œǉ�
///////////////////////////////////////////////////////////////////////////////
u32 MP_AddWptCancel(u16 push){
	IW->mp.menuid = MENU_ID_WAYPOINT;
	IW->mp.sel = 0;
	IW->mp.proc = 0;
	DispMenu(IW->mp.menuid);
	memset(&IW->wpt_input, 0, sizeof(WptInput)); // �E�F�C�|�C���g�����������Ă���
	return 1;
}

u32 MP_AddWpt(u16 push){
	if(push != 0xffff) return push & (KEY_A | KEY_B);
	switch(WptAddCheck(&IW->wpt_input)){
	case WPT_ADD_MAX:
		PlaySG1(SG1_CANCEL);
		MenuFillBox(0, 4, 29, 9);
		DrawText(2, 6, "WPT�� ��������1000����ł�");
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
	DrawText(5, 6, "WPT���������܂���");
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
	{0,			"�Ȃ܂��\�[�g   "},
	{comp_name,	"�����ǃ\�[�g   "},
	{comp_alt,	"���ǃ\�[�g     "},
	{comp_lat,	"�����ǃ\�[�g   "},
	{comp_lon,	"�Ƃ��낭����� "},
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

		// �������l�߂�
		u16* sort = IW->wpt_sort;
		u32 backup = sort[IW->mp.wpt_sel];
		u32 i;
		for(i = 0 ; i < WPT->wpt_count ; ++i) sort[i] = i; // �I�[�_��

		// �\�[�g
		if(st->comp) qsort(sort, WPT->wpt_count, sizeof(u16), st->comp); // 1000�|�C���g�ł�1�b�ȓ��Ɋ���

		// ���Ƃ�select������
		for(i = 0 ; sort[i] != backup && i < WPT->wpt_count - 1 ; ++i);
		IW->mp.wpt_sel = i;
	}
	FillTile(MAP_BG1, 0, 18, 29, 19, SELMENU_BK2);

	// �A�C�e���\��
	u32 sort_type = GetSortType();
	const u32* hpos = (sort_type < 3)? HILIGHT_POS[sort_type] : HILIGHT_NOP;
	if(WPT->wpt_count > NO_SCROLL){
		DrawBox(0, 2, 29, 17);
		MoveSelBg2(3, hpos[0], hpos[1], SELMENU_0, SELMENU_BK3); // �J�[�\���͒��S�Œ�
	} else {
		DrawBox(0, 2, 29, WPT->wpt_count * 2 + 3);
		u32 i;
		for(i = 0 ; i < WPT->wpt_count ; ++i) PutWptMenu(i, i * 2 + 3);
	}
	if(WPT->wpt_count < 2) return 0;
	DrawText(0, 18, "Select�{�^�� \\ ");
	Puts(st->name);
	return 1;
}

u32 SelWpt(u16 push){
	u32 sort_flag = 0;
	if(push == 0xffff){
		// ������WPT�L���`�F�b�N
		IW->mp.mp_flag = 0;
		if(!WPT->wpt_count){
			PlaySG1(SG1_CANCEL);
			MenuFillBox(5, 4, 25, 9);
			DrawText(7, 6, "WPT�� ����܂���!");
			return SetKeyWaitCB(0xffff);
		}

		// ���j���[�쐬
		MenuCls(SELMENU_BK2);
		if(     IW->mp.proc == MP_ChangeWpt)	Puts("�ǂ�WPT�� �ւ񂱂� ���܂���?");
		else if(IW->mp.proc == MP_DelWpt)		Puts("�ǂ�WPT�� �������� ���܂���?");
		else if(IW->mp.proc == MP_CopyWpt)		Puts("�ǂ�WPT�� �R�s�[ ���܂���?");
		else if(IW->mp.proc == MP_LDWpt)		Puts("�����f�B���O�������ł�������");
		else if(IW->mp.proc == MP_RtWptInput)	Puts("�ǂ�WPT�� ���� ���܂���?");
		if(IW->mp.wpt_sel >= WPT->wpt_count) IW->mp.wpt_sel = WPT->wpt_count - 1;
		sort_flag = SortWpt();
	} else if(push & KEY_B){
		// �I���L�����Z��
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

	// �X�N���[���\��
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

	// �V�K�ǉ��փW�����v
	IW->mp.menuid = MENU_ID_NEW_WPT;
	IW->mp.sel = 0;
	return 1;
}

u32 MP_LDWpt(u16 push){
	if(push == 0xffff || !(push & KEY_A)) return SelWpt(push);
	if(IW->mp.wpt_sel == -1) return 2;

	PlaySG1(SG1_OK);
	WPT->def_ld = IW->wpt_sort[IW->mp.wpt_sel];
	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ���[�g/WPT�ύX�t���O���Z�b�g
	return 1;
}

u32 MP_ChangeWpt(u16 push){
	if(push == 0xffff || !(push & KEY_A)) return SelWpt(push);
	if(IW->mp.wpt_sel == -1) return 2;

	// �R�s�[���쐬
	Wpt* sel = &WPT->wpt[IW->wpt_sort[IW->mp.wpt_sel]];
	PlaySG1(SG1_OK);
	IW->wpt_input.lat = sel->lat;
	IW->wpt_input.lon = sel->lon;
	IW->wpt_input.alt = sel->alt;
	memcpy(IW->wpt_input.name, sel->name, WPT_NAMESIZE);

	// �V�K�ǉ��փW�����v
	IW->mp.menuid = MENU_ID_CHANGE_WPT;
	IW->mp.sel = 0;
	return 1;
}
u32 MP_ChangeWptCancel(u16 push){
	// wpt_input�����������Ă���
	IW->wpt_input.lat = 0;
	IW->wpt_input.lon = 0;
	IW->wpt_input.alt = 0;
	memset(IW->wpt_input.name, 0, WPT_NAMESIZE);

	IW->mp.menuid = MENU_ID_WAYPOINT;
	IW->mp.proc = MP_ChangeWpt;
	IW->mp.sel = 1; // �ύX�̈ʒu!
	MP_ChangeWpt(0xffff);
	return 0; // 0�ŗǂ�
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
				// �ύX�����Ƃ�����
				sel->lat = IW->wpt_input.lat;
				sel->lon = IW->wpt_input.lon;
				sel->alt = IW->wpt_input.alt;
				memcpy(sel->name, IW->wpt_input.name, WPT_NAMESIZE);
				IW->wpt_sort_type = GetSortType(); // �ă\�[�g�̂��߂̃��Z�b�g
				i = -1;
				while((i = WptIsUsed(sel_id, i + 1)) != -1) CalcRouteDistance(i);
				IW->mp.navi_update |= NAVI_UPDATE_WPT;
				IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ���[�g/WPT�ύX�t���O���Z�b�g
			}
		} else {
			return 1;
		}
		return MP_ChangeWptCancel(push);
	}
	return push & (KEY_A | KEY_B); // ���A�{�^���`�F�b�N
}

u32 MP_ClearWpt(u16 push){
	if(push == 0xffff){
		PlaySG1(SG1_NEXT);
		MenuFillBox(0, 3, 29, 13);
		DrawText(1, 5,  "���[�g�� �ӂ��܂�Ȃ� WPT ��");
		DrawText(1, 7,  "�������債�� ��낵���ł���?");
		DrawText(4, 10, "START�{�^��: ��������");
		return 0;
	}
	if(push & (KEY_A | KEY_B)){
		PlaySG1(SG1_CANCEL);
		return 1;
	}
	if(!(push & KEY_START)) return 0;

	PlaySG1(SG1_CLEAR);
	MenuFillBox(3, 4, 26, 9);
	DrawText(5, 6,  "�N���A���Ă��܂�...");
	u32 i = WPT->wpt_count;
	while(i--) if(WptIsUsed(i, 0) == -1) WptDelCheck(i);
	IW->wpt_sort_type = GetSortType(); // �ă\�[�g�̂��߂̃��Z�b�g
	return 1;
}
u32 MP_CurWpt(u16 push){
	if(IW->px.fix <= FIX_INVALID){
		PlaySG1(SG1_CANCEL);
		MenuFillBox(3, 4, 26, 9);
		DrawText(5, 6, "������G���[�ł�!");
		return SetKeyWaitCB(0xffff);
	}

	// ���ݒn��ݒ�
	IW->wpt_input.lat = IW->px.lat;
	IW->wpt_input.lon = IW->px.lon;
	IW->wpt_input.alt = BiosDiv(IW->px.alt_mm, 1000, &IW->wpt_input.alt);
	IW->wpt_input.name[0] = 0;

	// �V�K�ǉ��փW�����v
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
			DrawText(4, 6, "���[�g\"");
			PutsName(ROUTE->route[used].name);
			Putc('"');
			DrawText(4, 8, "�� �����Ă��܂�!");
			IW->mp.mp_flag = 2;
		}
		MoveOut(OBJ_MENU_CURSOR);
		return 0;
	}
	if(IW->mp.wpt_sel == -1) return 2;
	return SelWpt(push);
}


///////////////////////////////////////////////////////////////////////////////
// �E�F�C�|�C���g�̃_�E�����[�h
///////////////////////////////////////////////////////////////////////////////
u32 MP_DownloadWpt(u16 push){
	return SetDownloadCB(0xffff);
}

///////////////////////////////////////////////////////////////////////////////
// ���[�g����
///////////////////////////////////////////////////////////////////////////////
u32 ChangeTaskRoute(u32 n, const u8* msg){
	if(n >= ROUTE->route_count) return 1;//error

	if(msg){
		MenuFillBox(1, 4, 29, 15);
		DrawTextCenter(6, msg);
		Locate(4, 9);
		Puts("�����: ");
		PutsDistance(CalcRouteDistance(n));
		DrawTextCenter(12, "���̃��[�g���^�X�N�ɂ��܂�");
		IW->mp.save_flag |= SAVEF_CHANGE_WPT; // �ݒ�ύX�t���O
	}

	IW->tc.route = n;
	IW->mp.save_flag |= SAVEF_UPDATE_CFG; // �ݒ�ύX�t���O
	IW->mp.navi_update |= NAVI_UPDATE_ROUTE;
	IW->mp.pre_route = (Route*)-1;
	return 0;
}

// ���[�g�ǉ�
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

	ChangeTaskRoute(i, "���[�g���������܂���");
	return ROUTE_ADD_SUCCESS;
}

// ���[�g�폜
u32 RouteDelCheck(u32 id){
	if(id >= ROUTE->route_count) return ROUTE_DEL_ERROR;
	for(--ROUTE->route_count ; id < ROUTE->route_count ; ++id){
		ROUTE->route[id] = ROUTE->route[id + 1];
	}

	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ���[�g/WPT�ύX�t���O���Z�b�g
	return ROUTE_DEL_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// ���[�g�I��
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
			DrawText(6, 6, "���[�g������܂���!");
			return SetKeyWaitCB(0xffff);
		}
		MenuCls(SELMENU_BK2);
		if(IW->mp.proc == MP_ChangeRoute)		Puts("�ǂ̃��[�g�� �ւ񂱂����܂���?");
		else if(IW->mp.proc == MP_DelRoute)		Puts("�ǂ̃��[�g�� �������債�܂���?");
		else if(IW->mp.proc == MP_CopyRoute)	Puts("�ǂ̃��[�g�� �R�s�[ ���܂���?");
		else if(IW->mp.proc == MP_SendRoute)	Puts("�ǂ̃��[�g�� �Ă񂻂����܂���?");
		else if(IW->mp.proc == MP_TaskInput)	Puts("�ǂ̃��[�g�� �^�X�N�ɂ��܂���?");

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
	Puts("�����: ");
	PutsDistance(ROUTE->route[IW->mp.route_sel].dist);
	PutsSpace(9);
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// ���[�g�̒ǉ�
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
		DrawText(1, 6, "���[�g�� ��������20����ł�");
		return 0;
	case ROUTE_ADD_SAMENAME:
		return PutSameNameMessage("���[�g");
	case ROUTE_ADD_BADNAME:
		return PutBadNameMessage();
	case ROUTE_ADD_EMPTY:
		MenuFillBox(0, 4, 29, 9);
		DrawText(2, 6, "WPT�������Ă����Ă�������!");
		return 0;
	case ROUTE_ADD_CANCEL:
		return MP_AddRouteCancel(0xffff);
	}
	// ������
	IW->mp.menuid = MENU_ID_ROUTE;
	IW->mp.sel = 0;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// ���[�g�̕ҏW
///////////////////////////////////////////////////////////////////////////////
u32 MP_CopyRoute(u16 push){
	if(push == 0xffff || !(push & KEY_A)) return SelRoute(push);
	if(IW->mp.route_sel == -1) return 2;

	PlaySG1(SG1_OK);
	IW->route_input = ROUTE->route[IW->mp.route_sel];
	IW->route_input.name[0] = 0; // ���O�����͏�����

	IW->mp.menuid = MENU_ID_NEW_ROUTE;
	IW->mp.sel = 0;
	return 1;
}
u32 MP_SendRoute(u16 push){
	// ����`�F�b�N
	if(push == 0xffff){
		if(IW->gi.state < GPS_EMUL_INFO || GPS_EMUL_FINISH <= IW->gi.state) return SelRoute(0xffff);
		PlaySG1(SG1_CANCEL);
		MenuFillBox(2, 3, 27, 8);
		DrawTextCenter(5, "�����񂿂イ!");
		return SetKeyWaitCB(0xffff);
	}

	// ���[�g�I��
	if(!IW->mp.mp_flag){
		if(!(push & KEY_A)) return SelRoute(push);
		if(IW->mp.route_sel == -1) return 2;

		// ���[�g�m��
		void GpsEmuMode(u32 mode, const u16* snd);
		GpsEmuMode(GPS_EMUL_ROUTE, SG1_COMP1);
		MenuFillBox(5, 3, 24, 11);
		DrawTextCenter(5, "�Ă񂻂����イ");
		DrawTextCenter(8, "B�{�^��:���ǂ�");
		IW->mp.mp_flag = 1;
	}

	// �]����
	if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 1; // ���[�h�͖߂����]���͌p��
	}
	if(IW->gi.state == GPS_EMUL_FINISH || IW->gi.state == GPS_EMUL_FINISH_WAIT){
		return 1; // ���튮��
	}
	if(IW->gi.state < GPS_EMUL_ROUTE || GPS_EMUL_ROUTE_WAIT < IW->gi.state){
		// �]���G���[���BSELECT��������Ă��������f���ăR�R�ɂ���
		PlaySG1(SG1_CANCEL);
		MenuFillBox(4, 4, 25, 9);
		DrawTextCenter(6, "�Ă񂻂��G���[!");
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
	IW->mp.sel = 0; // WPT�I������
	return 1;
}
u32 MP_ChangeRouteCancel(u16 push){
	// route_input�����������Ă���
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
	if(push != 0xffff) return push & (KEY_A | KEY_B); // ���A�{�^���`�F�b�N

	if(!IW->route_input.count){
		MenuFillBox(0, 4, 29, 9);
		DrawText(2, 6, "WPT�������Ă����Ă�������!");
		return 0;
	}

	if(IW->mp.route_sel >= ROUTE->route_count) return 0; // !?
	if(!*IW->route_input.name) return PutBadNameMessage();
	u32 i;
	for(i = 0 ; i < ROUTE->route_count ; ++i){
		if(i == IW->mp.route_sel) continue;
		if(!memcmp(ROUTE->route[i].name, IW->route_input.name, ROUTE_NAMESIZE)){
			return PutSameNameMessage("���[�g");
		}
	}

	Route* cur = &ROUTE->route[IW->mp.route_sel];
	if(memcmp(cur, &IW->route_input, sizeof(Route))){
		*cur = IW->route_input;
		ChangeTaskRoute(IW->mp.route_sel, "���[�g���ւ񂱂����܂���");
		return MP_ChangeRouteWait(0xffff);
	}
	return MP_ChangeRouteCancel(push);
}


///////////////////////////////////////////////////////////////////////////////
// ���[�g�̍폜
///////////////////////////////////////////////////////////////////////////////
u32 MP_DelRoute(u16 push){
	if(push == 0xffff) return SelRoute(push);

	if(IW->mp.mp_flag){
		if(IW->mp.mp_flag == 2 && (push & KEY_START)){
			PlaySG1(SG1_CHANGE);
			RouteDelCheck(IW->mp.route_sel);
			if(IW->mp.route_sel < IW->tc.route){
				--IW->tc.route;
				IW->mp.pre_route = GetTaskRoute();// �V�����ꏊ�ɃV�t�g(�^�X�N���[�g���폜����邱�Ƃ͂Ȃ�)
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
			DrawText(3, 6, "���̃��[�g�̓^�X�N�̂���");
			DrawText(3, 8, "�������� �ł��܂���!");
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
// ���[�g�̃_�E�����[�h
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
const u16 SAMPLE_ROUTE[] = { // �f�[�^�����炷���� Route �\���̂ł͂Ȃ��Au16�z��Œ�`
	0x6153, 0x706d, 0x656c, 0x3130, 0, 0, 0, // Sample01
	11, 0, 0, // count, dist_lo, dist_hi
	2,-1,  3,-1,  4,-1,  5,-1,  6,-1,  4,-1,  5,-1,  6,-1,  3,-1,  7,-1,
	1,-1,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
};

u32 AddAsagiriRoute(){
	if(ROUTE->route_count){
		// 2��ڈȍ~�̓f�o�b�O�p��ʃf�[�^
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

	// �ŏ���1�񂾂�F1���[�g��o�^
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

	// ���t�z���e�X�g�p
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
		DrawText(1, 5,  "���ׂẴ��[�g���������債��");
		DrawText(1, 7,  "��낵���ł���?");
		DrawText(4, 10, "START�{�^��: ��������");
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
	IW->mp.cur_view = -1;	// �ĕ`��

	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ���[�g/WPT�ύX�t���O���Z�b�g
	IW->mp.pre_route = (Route*)-1;// �^�X�N���O��������
	IW->wpt_sort_type = GetSortType(); // �ă\�[�g�̂��߂̃��Z�b�g
	return 1;
}

u32 MP_AllClear(u16 push){
	if(push == 0xffff){
		PlaySG1(SG1_NEXT);
		MenuFillBox(0, 3, 29, 13);
		DrawText(1, 5,  "���[�g�ƃE�F�C�|�C�������ׂ�");
		DrawText(1, 7,  "�������債�� ��낵���ł���?");
		DrawText(4, 10, "START�{�^��: ��������");
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
	IW->mp.cur_view = -1;	// �ĕ`��

	IW->mp.save_flag |= SAVEF_CHANGE_WPT; // ���[�g/WPT�ύX�t���O���Z�b�g
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// ���j���[���l����
///////////////////////////////////////////////////////////////////////////////
u32 MP_IntInput(u16 push){
	IntInput* ii = &IW->aip.i_int;
	if(push == 0xffff){
		// ���͖�
		MenuFillBox (5, 4, 24, 13);
		DrawTextCenter(5, ii->t->name);
		DrawText(20, 8, ii->t->unit);
		//�͈�
		Locate(7, 11);
		PutsPointSGN(ii->t->min, 0, ii->t->prec);
		Puts(" - ");
		PutsPointSGN(ii->t->max, 0, ii->t->prec);
	} else if(push & KEY_A){
		PlaySG1(SG1_OK);
		*ii->t->val = ii->val;
		IW->mp.save_flag |= SAVEF_UPDATE_CFG; // �ݒ�ύX�t���O
		return 1; // MP�I�� (�m��)
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 2; // MP�I�� (�L�����Z��)
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
// ���j���[��������
///////////////////////////////////////////////////////////////////////////////
#define MAX_TIME (60 * 24 - 1)
u32 MP_TimeInput(u16 push){
	TimeInput* ii = &IW->aip.i_time;
	if(push == 0xffff){
		// ���͖�
		MenuFillBox (6, 4, 23, 12);
		DrawTextCenter(6, ii->t->name);
	} else if(push & KEY_A){
		PlaySG1(SG1_OK);
		*ii->t->val = ii->val;
		IW->mp.save_flag |= SAVEF_UPDATE_CFG; // �ݒ�ύX�t���O
		return 1; // MP�I�� (�m��)
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 2; // MP�I�� (�L�����Z��)
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
// ���j���[���O����
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
		Putsf("%M%d����", 3 + ii->t->max, 1, ii->t->max);
		Locate(2, 2);
		u32 i;
		for(i= 0 ; i < ii->t->max ; ++i) Putc('=');

		// ���͘g
		DrawBox(2, 4, 26, 19);
		PutsNameTable(0);
		DrawText(5, 17, "�����Ă�  �L�����Z��");
		//������
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
	} else if(push & KEY_A){ // ����
		if(ii->y == 4){
			if(ii->x < 5){
				// �m��
				PlaySG1(SG1_OK);
				memcpy(ii->t->val, ii->val, ii->t->max);
				IW->mp.save_flag |= SAVEF_UPDATE_CFG; // �ݒ�ύX�t���O
			} else {
				// �L�����Z��
				PlaySG1(SG1_CANCEL);
			}
			return 1; // �I��
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
	} else if(push & KEY_B){ // �폜
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
// ���j���[�ܓx�o�x����
///////////////////////////////////////////////////////////////////////////////
u32 MP_LatInput(u16 push){
	LatInput* ii = &IW->aip.i_lat;
	if(push == 0xffff){
		MenuFillBox(6, 5, 23, 12);
		DrawText(8, 7, ii->t->latlon? "����:" : "������:");
	} else if(push & KEY_A){
		PlaySG1(SG1_OK);
		*ii->t->val = ii->val;
		IW->mp.save_flag |= SAVEF_UPDATE_CFG; // �ݒ�ύX�t���O
		return 1; // MP�I�� (�m��)
	} else if(push & KEY_B){
		PlaySG1(SG1_CANCEL);
		return 2; // MP�I�� (�L�����Z��)
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
// RtWpt����
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

// RtWpt�ǉ�
u32 AddRtWpt(){
	Route* rt = &IW->route_input;
	u32 i = rt->count;
	if(i >= MAX_ROUTE_PT) return 1;
	u32 end = IW->mp.rtwpt_sel / 2;
	for(; i > end ; --i) rt->py[i] = rt->py[i - 1];
	rt->py[i].wpt = IW->wpt_sort[IW->mp.wpt_sel];
	rt->py[i].cyl = -1; // �f�t�H���g
	rt->count++;
	return 0;
}

// RtWpt���j���[��1�s��\��
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
		Puts("  �Ȃ� ");
		break;
	case 0xffff:
		Putsf("%5dm ", IW->tc.cylinder);
		break;
	default:
		Putsf("%5dm!", v);
	}
}

// RtWpt���j���[�̏�����
void InitRtWptMenu(){
	Route* rt = &IW->route_input;
	MenuCls(SELMENU_BK2);
	Puts("���[�g:\"");
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
	IW->mp.rtwpt_calc = 1; // �v�Z�J�n
}

const IntInputTemplate INDV_CYL = {
	5, 1, 0, -1, 65000, 
	"�V�����_�͂񂯂�",
	"m",
	IW_OFFSET(mp.cyl_input)
};

u32 MP_RtWptInput(u16 push){
	Route* rt = &IW->route_input;
	if(push == 0xffff){					// �����ݒ�
		IW->mp.mp_flag = 0;
		IW->mp.rtwpt_state = 0;
		IW->mp.rtwpt_sel = 0;
		IW->mp.rtwpt_dsp = 0;
		if(!WPT->wpt_count){
			PlaySG1(SG1_CANCEL);
			MenuFillBox(5, 4, 24, 9);
			DrawText(7, 6, "WPT������܂���!");
			return SetKeyWaitCB(0xffff);
		}
		if(!IW->route_input.count){
			IW->mp.rtwpt_state = 1;
			return SelWpt(0xffff);
		}
		InitRtWptMenu();
	} else if(IW->mp.rtwpt_state == 1){ // �ǉ�
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
	} else if(IW->mp.rtwpt_state == 2){ // �폜 or 
		if(push & KEY_START){
			// �폜
			u32 i;
			rt->count--;
			for(i = IW->mp.rtwpt_sel / 2 ; i < rt->count ; ++i){
				rt->py[i] = rt->py[i + 1];
			}
			AddRtWptSel(-2);
			if(IW->mp.rtwpt_dsp) --IW->mp.rtwpt_dsp;
			PlaySG1(SG1_CLEAR);
		} else if(push & KEY_A){
			// �V�����_�ύX
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
			DrawTextCenter(15, "-1�̓f�t�H���g�T�C�Y�ł�");
			return 0;
		} else if(!(push & KEY_B)) return 0;

		// �L�����Z�����A
		IW->mp.rtwpt_state = 0;
		InitRtWptMenu();
	} else if(IW->mp.rtwpt_state == 3){ // �V�����_�ύX
		switch(MP_IntInput(push)){
		case 1: // �m��
			rt->py[IW->mp.rtwpt_sel / 2].cyl = IW->aip.i_int.val;
			break;
		case 2: // �L�����Z��
			break;
		default:
			return 0;
		}
		// ���菈��
		IW->mp.rtwpt_state = 0;
		InitRtWptMenu();
	} else if(push & KEY_A){
		if(IW->mp.rtwpt_sel & 1){
			// �폜
			IW->mp.rtwpt_state = 2;
//			PutDelConf();
			PlaySG1(SG1_CHANGE);
			MenuFillBox(1, 4, 28, 12);
			DrawText(3, 6, "A�{�^��:�V�����_�ւ񂱂�");
			DrawText(3, 9, "START  :�p�C������������");
			return 0;
		} else {
			// �ǉ�
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
		// �������\��
		if(IW->mp.rtwpt_calc){
			if(IW->mp.rtwpt_calc < IW->route_input.count){
				Wpt* w1 = &WPT->wpt[IW->route_input.py[IW->mp.rtwpt_calc - 1].wpt];
				Wpt* w2 = &WPT->wpt[IW->route_input.py[IW->mp.rtwpt_calc++  ].wpt];
				s32 len;

				CalcDist(w1->lat, w1->lon, w2->lat, w2->lon, &len, 0);
				IW->mp.rtwpt_sum += len;
			} else {
				// ����
				Locate(24, 0);
				PutsDistance(IW->mp.rtwpt_sum);
				IW->mp.rtwpt_calc = 0;
			}
		}
		return DispMenuArrow(IW->mp.rtwpt_dsp, IW->mp.rtwpt_dsp < rt->count - RTWPT_NO_SCROLL);
	}

	// �X�N���[��
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

	// �w���v�s�\��
	Locate(0, 18);
	if(IW->mp.rtwpt_sel & 1){
		Putsf("%d.���ւ񂱂�    ", IW->mp.rtwpt_sel / 2 + 1);
	} else {
		// ����
		if(IW->route_input.count < MAX_ROUTE_PT){
			Putsf("%d.��WPT������ ", IW->mp.rtwpt_sel / 2 + 1);
		} else {
			Puts("�����ł��܂���  ");
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// �^�X�N����
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
// �����v�␳
///////////////////////////////////////////////////////////////////////////////
const IntInputTemplate CALIB_REF = {
	3, 0, 1, 1, 999,
	"���t�@�����X",
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
		DrawText(2, 13, "�����Ă����͂��߂Ă�������");
		DrawText(2, 16, "B�{�^���ŃL�����Z�����܂�");
		IW->anemo.calib_flag = 1;
		IW->mp.mp_flag = 0;
	} else if(!IW->anemo.calib_flag){
		if(IW->mp.mp_flag){
			switch(MP_IntInput(push)){
			case 1: // �m��
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
				DrawTextCenter(6, "�ق����G���[�ł�!");
				return SetKeyWaitCB(0xffff);
			case 2: // �L�����Z��
				return 1;
			}
		} else if(!IW->anemo.calib_pulse){
			PlaySG1(SG1_CANCEL); // NG!? �O�̂��߃`�F�b�N
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
			DrawTextCenter(6, "GPS�G���[�ł�!");
			return SetKeyWaitCB(0xffff);
		}
		MenuCls(SELMENU_BG);
		MenuFillBox(0, 3, 29, 19);
		DrawText(1, 0, "Anemometer Calibration (GPS)");
		DrawText(2, 14, "�����Ă����͂��߂Ă�������");
		DrawText(2, 16, "B�{�^���ŃL�����Z�����܂�");
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
		DrawTextCenter(6, "�ق����G���[�ł�!");
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
// Bluetooth���[�h��"�Ȃ�"�Ȃ�G���[�Ƃ���
s32 BtModeCheck(){
	if(IW->tc.bt_mode) return 1;
	PlaySG1(SG1_CANCEL);
	MenuFillBox(4, 4, 25, 9);
	DrawTextCenter(6, "���[�h�G���[!");
	return 0;
}

// 12���������Z�b�g����Ă��Ȃ���΃G���[�Ƃ���
s32 BtAddrCheck(){
	if(IsBluetoothAddr(IW->tc.bt_addr)) return 1;
	PlaySG1(SG1_CANCEL);
	MenuFillBox(1, 4, 28, 9);
	DrawTextCenter(6, "Bluetooth�A�h���X�G���[!");
	return 0;
}

// �f�[�^�����N�w�̃��[�h�ؑ�
static inline void SetATCMode(s32 mode){
	if(mode){
		// AT�R�}���h���䃂�[�h�Ɉڂ�
		IW->gi.state = GPS_BT_BR_CHECK1;// �܂��̓{�[���[�g�`�F�b�N����
		IW->dl_fsm   = DL_FSM_ATC_WAIT;
		
		// �f�o�b�O�p�ɐ؂�ւ�&�N���A
		u32* log = &IW->mp.last_wpt_len;
		if(!(*log & DBGDMP_COM)) *log = DBGDMP_COM;
	} else {
		// GPS�f�[�^��M���[�h�ɖ߂�
		IW->gi.state = GPS_GET_PINFO_WAIT;
		IW->dl_fsm   = DL_FSM_WAIT_DLE;
	}
}

// �f�o�C�X�T�[�`���I��
u32 MP_BtInq(u16 push){
	if(push == 0xffff){
		if(!BtModeCheck()) return SetKeyWaitCB(0xffff);
		MenuCls(SELMENU_BK1);
		FillBox(0, 0, 29, 19);
		DrawText(1, 17, "�T�[�`���イ...");
		DrawText(5, 9, "B�{�^���ŃL�����Z��");

		ATC_PTR->inq_count = 0;
		IW->mp.scr_pos = 0;
		IW->mp.mp_flag = 0;
		SetATCMode(1);
	} else if(push & KEY_B){
		// INQ�������f
		PlaySG1(SG1_CANCEL);
		SetATCMode(0);
		return 1;
	} else if((push & KEY_A) && IW->mp.scr_pos < ATC_PTR->inq_count){
		// �J�[�\���ʒu�̃f�o�C�X��I��
		PlaySG1(SG1_CHANGE);
		memcpy(IW->tc.bt_addr, ATC_PTR->inq_list[IW->mp.scr_pos], 12);
		SetATCMode(0);
		return 1;
	} else if((push & (KEY_UP | KEY_DOWN)) && ATC_PTR->inq_count){
		// �J�[�\���ړ�
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
	
	// �o�ߕ\���p
	switch(IW->gi.state){
	case GPS_BT_BR_OK:
		// ���Ƀ��b�Z�[�W�͏o�����Ɍp��
		IW->gi.state = GPS_BT_INQ_START;
		break;

	case GPS_BT_INQ_DONE:
		// INQ����������"�T�[�`���イ..."����������
		if(ATC_PTR->inq_count < MAX_INQ_DEV){
			Locate(1, 17);
			PutsSpace(28);
		}
		break;

	case GPS_BT_ERROR:
		DrawText(1, 17, "�T�[�`�G���[�ł�.");
		break;
	}

	// INQ���X�g�ɃA�b�v�f�[�g������Α���ʂɔ��f
	for(; IW->mp.mp_flag < ATC_PTR->inq_count ; IW->mp.mp_flag++){ // �ǉ�������
		Locate(1, IW->mp.mp_flag * 2 + 1);
		PutsName2(ATC_PTR->inq_list[IW->mp.mp_flag], 28, 1); // �ő�28�����܂�
	}

	// �J�[�\���ʒu�\��
	if(ATC_PTR->inq_count){
		MoveObj(OBJ_MENU_CURSOR, 1, IW->mp.scr_pos * 16 + 9);
		MoveSelBg(IW->mp.scr_pos, SELMENU_0);
	}
	return 0;
}

// �ڑ�/�ؒf�̋��ʃt�b�N
u32 MP_BtCmd(u16 push){
	if(push == 0xffff){
		if(!BtModeCheck()) return SetKeyWaitCB(0xffff);
		FillBox(2, 4, 27, 12);
		DrawText(4, 9, "(B�{�^���ł��ǂ�)");
		SetATCMode(1);
		IW->mp.proc = MP_BtCmd; // �R�[���o�b�N��ς���
		ATC_PTR->inq_count = 0;
	} else if(push & KEY_B){
		// �ڑ��������f
		PlaySG1(SG1_CANCEL);
		SetATCMode(0);
		return 1;
	}

	// �o�ߕ\���p
	const u8* msg = 0;
	switch(IW->gi.state){
	case GPS_BT_BR_CHECK1:				msg = "�{�[���[�g�����ɂ�..."; break;
	case GPS_BT_BR_RESET_WAIT:
	case GPS_BT_CON_RESET_WAIT:
	case GPS_BT_SCAN_RESET_WAIT:		msg = "���Z�b�g���イ...    "; break;
	
	case GPS_BT_BR_CHANGE_WAIT:
	case GPS_BT_BR_MODE0_WAIT:
	case GPS_BT_CON_MODECHANGE_WAIT:	msg = "���[�h�ւ񂱂�...    "; break;
	
	case GPS_BT_CON_SETKEY_WAIT:		msg = "PIN�R�[�h�Ƃ��낭... "; break;
	case GPS_BT_CON_DIAL_WAIT:			msg = "�y�A�����O...        "; break;
	case GPS_BT_CON_STANDBY_WAIT:		msg = "�X�^���o�C���肩��..."; break;
	case GPS_BT_CON_ATH_WAIT:			msg = "�����񂿂イ...    "; break;
	case GPS_BT_SCAN_START_WAIT:		msg = "�܂��������肩��...  "; break;

	case GPS_BT_CON_COMPLETE:
	case GPS_BT_SCAN_COMPLETE:
	case GPS_BT_IDLE_COMPLETE:			msg = "�����傤���܂���.  "; break;
	case GPS_BT_ERROR:					msg = "������G���[�ł�!  "; break;
	case GPS_BT_BR_OK:
		// ���Ƀ��b�Z�[�W�͏o�����Ɍp��
		IW->gi.state = IW->mp.mp_flag;
		break;
	}
	if(msg) DrawText(4, 6, msg);

	// ���� or �G���[�܂ōs������AA�{�^���ŕ��A�ł���悤keyWait�ɐ؂�ւ���
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

// �ݒ���e�Őڑ��J�n�����[�h�ؑ�
u32 MP_BtConnect(u16 push){
	if(!BtAddrCheck()) return SetKeyWaitCB(0xffff);
	IW->mp.mp_flag = GPS_BT_CON_SETKEY;
	return MP_BtCmd(0xffff);
}

// �ݒ���e�Őڑ��J�n�����[�h�ؑ�
u32 MP_BtScan(u16 push){
	IW->mp.mp_flag = GPS_BT_SCAN_START;
	return MP_BtCmd(0xffff);
}

// �ݒ���e�Őڑ��J�n�����[�h�ؑ�
u32 MP_BtDisconnect(u16 push){
	IW->mp.mp_flag = GPS_BT_IDLE_START;
	return MP_BtCmd(0xffff);
}


///////////////////////////////////////////////////////////////////////////////
// ���C�����j���[

#define HELP_CONFIG_DETAL1	"�����Ă����Z�[�u���܂�. " \
							"���[�g��E�F�C�|�C���g���ւ񂱂�����Ă���Ƃ��ɂ�, �������Z�[�u���܂�.  <>"

const MenuItem MENU_MAIN[] = {
	{"���傤�ق�",			"���傤�ق��� �݂܂�",				MENU_ID_INFO},
	{"�^�X�N",				"�^�X�N�� �����Ă����܂�",			MENU_ID_TASK},
	{"���[�g",				"���[�g�� �����Ă����܂�",			MENU_ID_ROUTE},
	{"�E�F�C�|�C���g",		"�E�F�C�|�C���g�������Ă����܂�",	MENU_ID_WAYPOINT},
	{"�g���b�N���O",		"�g���b�N���O�� �����ɂ񂵂܂�",	MENU_ID_CONFIG_LOG},
	{"�R���t�B�O",			"�p�����[�^���ւ񂱂����܂�",		MENU_ID_CONFIG},
	{"�Z�[�u",				HELP_CONFIG_DETAL1,					MENU_FCALL,		MP_Save},
	{"�T�X�y���h",			"�i�r�� �ł񂰂�� ����܂�",		MENU_FCALL,		MP_Suspend},
	{0, 0,														MENU_FCALL,		MP_SaveCheck} // �i�r�ɖ߂�
};

///////////////////////////////////////////////////////////////////////////////
// �^�X�N���j���[
const u8* const TASK_MODE_LIST[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.task_mode),
	"�t���[�t���C�g",
	"�X�s�[�h����",
	"�S�[�����[�X",
};
const IntInputTemplate TASK_CY_R = {
	5, 0, 0, 0, 99999, 
	"�V�����_�͂񂯂�",
	"m",
	IW_OFFSET(tc.cylinder)
};
const IntInputTemplate TASK_SECTOR = {
	5, 0, 0, 0, 99999, 
	"�Z�N�^�͂񂯂�",
	"m",
	IW_OFFSET(tc.sector)
};
const TimeInputTemplate TASK_CLOSE_TIME = {
	"�N���[�Y�^�C��",
	IW_OFFSET(tc.close_time)
};
const u8* const TASK_START_LIST[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.start_type),
	"�t���[�X�^�[�g",
	"�C�� �X�^�[�g",
	"�A�E�g�X�^�[�g",
};
const TimeInputTemplate TASK_START_TIME = {
	"�X�^�[�g�^�C��",
	IW_OFFSET(tc.start_time)
};
const u8* const TASK_GOAL_TYPE[] = {
	(u8*)4,
	(u8*)IW_OFFSET(tc.goal_type),
	"���C��",
	"�V�����_",
	"�Z�N�^180��",
	"�V�����_�Z�N�^",
};
const IntInputTemplate TASK_PRE_PYLON = {
	2, 0, 0, 0, 99, 
	"�v���p�C����",
	"",
	IW_OFFSET(tc.pre_pylon)
};

#define HELP_TASK1	"�^�X�N�̃^�C�v�� ����т܂�.  " \
					"\"�t���[�t���C�g\":���[�X�������Ȃ��܂���. " \
						"���̃��[�h�ŃJ�[�\���L�[��������,�������̃E�F�C�|�C���g�Ƀi�r�Q�[�g���܂�.  " \
					"\"�X�s�[�h����\":�X�^�[�g����S�[���܂ł̃^�C�����������܂�.  " \
					"\"�S�[�����[�X\":�X�^�[�g�^�C���� �ǂ����X�^�[�g����,�S�[������񂶂���������܂�. " \
						"(���Ȃ炸�X�^�[�g�^�C���������Ă����Ă�������)  <>"
#define HELP_TASK2	"�V�����_�̃f�t�H���g�͂񂯂��� �����Ă����܂�. �V�����_������Ȃ��Ƃ��ɂ� 0m �ɂ��܂�.  " \
					"�V�����_�T�C�Y�� ���[�g���j���[����,�p�C�������Ƃɂւ񂱂��ł��܂�.  <>"
//#define HELP_TASK3	"�X�^�[�g�V�����_�� �͂񂯂��� �����Ă����܂�. -1�̂Ƃ��ɂ̓V�����_(r)�������܂�.  <>"
//#define HELP_TASK4	"�S�[���V�����_�� �͂񂯂��� �����Ă����܂�. -1�̂Ƃ��ɂ̓V�����_(r)�������܂�.  <>"
#define HELP_TASK5	"�Z�N�^�͂񂯂��������Ă����܂�. �Z�N�^������Ȃ��Ƃ��ɂ� 0m �ɂ��܂�.  " \
					"�Z�N�^�������Ă�����Ă����,�Z�N�^�����ɂ�悤�̃O���C�_�Ђ傤���� �����Ȃ��܂�.  <>"
#define HELP_TASK6	"���[�X�̃X�^�[�g���[�h�� �����Ă����j���[�ɂ����݂܂�.  <>"
#define HELP_TASK7	"�S�[���͂�Ă��̃^�C�v������т܂�.  " \
					"\"���C��\":�S�[�����C�������������Ƃ����S�[���Ƃ݂Ȃ��܂�.  " \
						"�S�[�����C���̂Ȃ����̓V�����_�T�C�Y�Ƃ��Ȃ��ł�.  " \
					"\"�V�����_\":�S�[���V�����_�ւ� ����ɂイ���S�[���Ƃ��܂�.  " \
					"\"�Z�N�^180��\":�S�[���Z�N�^(180���͂񂦂�)�ւ� ����ɂイ���S�[���Ƃ��܂�.  " \
					"\"�V�����_�Z�N�^\":�����傤�p�C�����Ƃ��Ȃ��悤��,�V�����_�܂��̓Z�N�^(90��)�ւ� " \
						"����ɂイ���S�[���Ƃ��܂�.  <>"
#define MENU_TASK8	"�^�X�N�̃��[�g�� ����т܂�. " \
					"(�^�X�N���񂽂�����PC/GBA���烋�[�g�������񃊃N�G�X�g�������, �J�[�\���ł��񂽂����イ�̃��[�g�݂̂� �������񂵂܂�)  <>"



#define MENU_TASK_START_DETAIL3	"���[�X�̃N���[�Y�^�C���������Ă����܂�. " \
								"(24H�^�C���ł�. PM1:00��13:00�� �����Ă����܂�.) " \
								"�N���[�Y�^�C���ɂȂ��,���ǂ��Ńt���[�t���C�g���[�h�ɂȂ�," \
								"�S�[���p�C�����Ƀi�r�Q�[�g���܂�. " \
								"00:00�̂Ƃ���,�N���[�Y�^�C���������܂���.  <>"

#define MENU_TASK_START_DETAIL	"�X�^�[�g�^�C�v�� ����т܂�.  " \
								"\"�t���[�X�^�[�g\":�X�^�[�g�V�����_�ɂ͂����,�^�X�N�X�^�[�g���܂�.  " \
								"\"�C���X�^�[�g\":�X�^�[�g�^�C���̂��Ƃ� �X�^�[�g�V�����_�ɂ͂����," \
									"�^�X�N�X�^�[�g���܂�.  " \
								"\"�A�E�g�X�^�[�g\":�X�^�[�g�^�C���̂��Ƃ� �X�^�[�g�V�����_����ł��," \
									"�^�X�N�X�^�[�g���܂�.  <>"
#define MENU_TASK_START_DETAIL2	"�X�^�[�g�^�C���������Ă����܂�. (24H�^�C���ł�. PM1:00��13:00�� �����Ă����܂�.)  " \
								"���̂����Ă���,�t���[�X�^�[�g�̂Ƃ��ɂ� �����܂���.  <>"

const MenuItem MENU_TASK[] = {
	{"�^�X�N�^�C�v",		HELP_TASK1,							MENU_SEL_ENUM,	&TASK_MODE_LIST},
	{"���[�g",				MENU_TASK8,							MENU_SEL_TASK,	0},
	{"�X�^�[�g",			HELP_TASK6,							MENU_SEL_START, (void*)MENU_ID_TASK_START},
	{"�N���[�Y",			MENU_TASK_START_DETAIL3,			MENU_SEL_TIME,	&TASK_CLOSE_TIME},
	{"�S�[���^�C�v",		HELP_TASK7,							MENU_SEL_ENUM,	&TASK_GOAL_TYPE},
	{"�V�����_(r)"	,		HELP_TASK2,							MENU_SEL_VAL,	&TASK_CY_R},
//	{"-S�V�����_(r)",		HELP_TASK3,							MENU_SEL_VAL,	&TASK_START_CY},
//	{"-G�V�����_(r)",		HELP_TASK4,							MENU_SEL_VAL,	&TASK_GOAL_CY},
	{"�Z�N�^(r)",			HELP_TASK5,							MENU_SEL_VAL,	&TASK_SECTOR},
	{0, 0,														MENU_ID_MAIN}
};

///////////////////////////////////////////////////////////////////////////////
// �^�X�N - �X�^�[�g

const u8* const TASK_START_ALARM[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.start_alarm),
	"�Ȃ�",
	"����",
};
const u8* const TASK_START_TO[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.skip_TO),
	"�^�X�N�p�C����",
	"�e�C�N�I�t",
};

#define MENU_TASK_START_DETAIL4	"�v���p�C������ ������ �����Ă����܂�. " \
								"�v���p�C������1�̂Ƃ�(���[�g��2�΂�߂��X�^�[�g�p�C�����̂Ƃ�)��, �v���p�C������\"1\"�������Ă����܂�.  <>"
#define MENU_TASK_START_DETAIL5	"�X�^�[�g�A���[���������Ă����܂�.  " \
								"\"�Ȃ�\":�A���[�����Ȃ炵�܂���. " \
								"\"����\":�A���[�����Ȃ炵�܂�. (30min. 10min. 5min. 1min. �X�^�[�g)  <>"
#define MENU_TASK_START_DETAIL6	"���[�g�̂���Ƃ��̃p�C�����̃��[�h�����񂽂����܂�. " \
								"\"�^�X�N�p�C����\":1st�p�C�������^�X�N�ɂӂ��߂܂�. " \
								"\"�e�C�N�I�t\":1st�p�C�����̓^�X�N�ɂӂ��߂܂���. (�X�L�b�v���܂�)  <>"

const MenuItem MENU_TASK_START[] = {
	{"�^�C�v",				MENU_TASK_START_DETAIL,				MENU_SEL_ENUM,	&TASK_START_LIST},
	{"�X�^�[�g",			MENU_TASK_START_DETAIL2,			MENU_SEL_TIME,	&TASK_START_TIME},
	{"1st�p�C����",			MENU_TASK_START_DETAIL6,			MENU_SEL_ENUM,	&TASK_START_TO},
	{"�v���p�C����",		MENU_TASK_START_DETAIL4,			MENU_SEL_VAL,	&TASK_PRE_PYLON},
	{"�A���[��",			MENU_TASK_START_DETAIL5,			MENU_SEL_ENUM,	&TASK_START_ALARM},
	{0, 0,														MENU_ID_TASK}
};

///////////////////////////////////////////////////////////////////////////////
// ���[�g���j���[

#define MENU_ALL_CLEAR "���ׂẴ��[�g�ƃE�F�C�|�C���g���������債�܂�. <>"

#define HELP_ROUTE_TX	"�N���X�P�[�u���ł��������ꂽ�p���i�r��,���[�g���Ă񂻂����܂�. <>"

const MenuItem MENU_ROUTE[] = {
	{"��������",			"�����炵�����[�g�� ����܂�",	MENU_ID_NEW_ROUTE},
	{"�ւ񂱂�",			"���[�g�� �ւ񂱂����܂�",			MENU_FCALL,		MP_ChangeRoute},
	{"��������",			"���[�g�� �������債�܂�",			MENU_FCALL,		MP_DelRoute},
	{"�R�s�[",				"���[�g�� �R�s�[���܂�",			MENU_FCALL,		MP_CopyRoute},
	{"�_�E�����[�h",		"GPS����_�E�����[�h���܂�",		MENU_FCALL,		MP_DownloadRoute},
	{"�Ă񂻂�",			HELP_ROUTE_TX,						MENU_FCALL,		MP_SendRoute},
	{"�N���A",				"���ׂẴ��[�g���������債�܂�",	MENU_FCALL,		MP_ClearRoute},
	{"�t���N���A",			MENU_ALL_CLEAR,						MENU_FCALL,		MP_AllClear},
	{0, 0,														MENU_ID_MAIN}
};

///////////////////////////////////////////////////////////////////////////////
// ���[�g�쐬�A�ύX
const NameInputTemplate ROUTE_NAME = {
	ROUTE_NAMESIZE, "���[�g�̂Ȃ܂�",
	(u8*)IW_OFFSET(route_input.name[0])
};

#define HELP_ADD_ROUTE	"���̃��[�g���Ƃ��낭���܂�. " \
						"�Ƃ��낭���Ȃ���B�{�^����������,���[�g�̂����̓L�����Z������܂�.  <>"

const MenuItem MENU_NEW_ROUTE[] = {
	{"�Ȃ܂�",				"���[�g�� �Ȃ܂��� ���߂܂�",		MENU_SEL_NAME,	&ROUTE_NAME},
	{"WPT���X�g",			"WPT���X�g�� �����������܂�",		MENU_SEL_RTWPT,	0},
//	{"�Ƃ��낭",			HELP_ADD_ROUTE,						MENU_FCALL,		MP_AddRoute},
	{"�Ƃ肯��",			"���[�g�������L�����Z�����܂�",	MENU_FCALL,		MP_AddRouteCancel},
	{0, 0,														MENU_FCALL,		MP_AddRoute},
};
#define HELP_CHANGE_ROUTE "���̂������Ń��[�g���ւ񂱂����܂�. " \
							"�����Ă����Ȃ���B�{�^����������,�ւ񂱂��̓L�����Z������܂�.  <>"

const MenuItem MENU_CHANGE_ROUTE[] = {
	{"�Ȃ܂�",				"�Ȃ܂��� �ւ񂱂� ���܂�",			MENU_SEL_NAME,	&ROUTE_NAME},
	{"WPT���X�g",			"WPT���X�g�� �ւ񂱂� ���܂�",		MENU_SEL_RTWPT,	0},
//	{"�����Ă�",			HELP_CHANGE_ROUTE,					MENU_FCALL,		MP_ChangeRouteSubmit},
	{"�Ƃ肯��",			"�ւ񂱂����L�����Z�����܂�",		MENU_FCALL,		MP_ChangeRouteCancel},
	{0, 0,														MENU_FCALL,		MP_ChangeRouteSubmit},
};

///////////////////////////////////////////////////////////////////////////////

#define HELP_WPT1	"�����f�B�O�ƂȂ�E�F�C�|�C���g�� ���񂽂����܂�. " \
					"�����f�B���O�Ƃ��� �����Ă������E�F�C�|�C���g��,�t���[�t���C�g���[�h�̃S�[���p�C�����ɂȂ�܂�.  <>"

// �E�F�C�|�C���g���j���[
const MenuItem MENU_WAYPOINT[] = {
	{"��������",			"�����炵��WPT�� ����܂�",		MENU_ID_NEW_WPT},
	{"�ւ񂱂�",			"WPT�� �ւ񂱂����܂�",				MENU_FCALL,		MP_ChangeWpt},
	{"��������",			"WPT�� �������債�܂�",				MENU_FCALL,		MP_DelWpt},
	{"�R�s�[",				"WPT�� �R�s�[���܂�",				MENU_FCALL,		MP_CopyWpt},
	{"�����f�B���O",		HELP_WPT1,							MENU_FCALL,		MP_LDWpt},
	{"�_�E�����[�h",		"GPS����WPT���_�E�����[�h���܂�",	MENU_FCALL,		MP_DownloadWpt},
	{"���񂴂���",			"���񂴂�����WPT�ɂ������܂�",	MENU_FCALL,		MP_CurWpt},
	{"�N���A",				"�ӂ悤��WPT�� �������債�܂�",		MENU_FCALL,		MP_ClearWpt},
//	{"�t���N���A",			MENU_ALL_CLEAR,						MENU_FCALL,		MP_AllClear},
	{0, 0,														MENU_ID_MAIN}
};

///////////////////////////////////////////////////////////////////////////////
// �E�F�C�|�C���g�쐬���j���[
const NameInputTemplate WPT_NAME = {
	WPT_NAMESIZE, "�E�F�C�|�C���g�̂Ȃ܂�",
	(u8*)IW_OFFSET(wpt_input.name[0])
};
// �ܓx�o�x
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
	"WPT������",
	"m",
	IW_OFFSET(wpt_input.alt)
};
#define HELP_ADD_WPT "���̃E�F�C�|�C���g���Ƃ��낭���܂�. " \
					"�Ƃ��낭���Ȃ���B�{�^����������,�E�F�C�|�C���g�̂����̓L�����Z������܂�.  <>"

const MenuItem MENU_NEW_WPT[] = {
	{"�Ȃ܂�",				"WPT�� �Ȃ܂��� ���߂܂�",			MENU_SEL_NAME,	&WPT_NAME},
	{"����",				"���ǂ� �ɂイ��傭���܂�",		MENU_SEL_LAT,	&WPT_LAT},
	{"������",				"�����ǂ� �ɂイ��傭���܂�",		MENU_SEL_LAT,	&WPT_LON},
	{"������",				"�����ǂ� �ɂイ��傭���܂�",		MENU_SEL_VAL,	&WPT_ALT},
//	{"�Ƃ��낭",			HELP_ADD_WPT,						MENU_FCALL,		MP_AddWpt},
	{"�Ƃ肯��",			"WPT�Ƃ��낭���L�����Z�����܂�",	MENU_FCALL,		MP_AddWptCancel},
	{0, 0,														MENU_FCALL,		MP_AddWpt}
};

#define HELP_CHANGE_WPT "���̂������ŃE�F�C�|�C���g���ւ񂱂����܂�. " \
						"�����Ă����Ȃ���B�{�^����������,�ւ񂱂��̓L�����Z������܂�.  <>"

const MenuItem MENU_CHANGE_WPT[] = {
	{"�Ȃ܂�",				"�Ȃ܂��� �ւ񂱂����܂�",			MENU_SEL_NAME,	&WPT_NAME},
	{"����",				"���ǂ� �ւ񂱂����܂�",			MENU_SEL_LAT,	&WPT_LAT},
	{"������",				"�����ǂ� �ւ񂱂����܂�",			MENU_SEL_LAT,	&WPT_LON},
	{"������",				"�����ǂ� �ւ񂱂����܂�",			MENU_SEL_VAL,	&WPT_ALT},
//	{"�����Ă�",			HELP_CHANGE_WPT,					MENU_FCALL,		MP_ChangeWptSubmit},
	{"�Ƃ肯��",			"WPT�ւ񂱂����L�����Z�����܂�",	MENU_FCALL,		MP_ChangeWptCancel},
	{0, 0,														MENU_FCALL,		MP_ChangeWptSubmit},
};

///////////////////////////////////////////////////////////////////////////////
// �ݒ胁�j���[

const MenuItem MENU_CONFIG[] = {
	{"GPS�R���t�B�O",		"GPS�p�����[�^�������Ă����܂�",	MENU_ID_CONFIG_GPS},
	{"�i�r�R���t�B�O",		"�i�r���[�h�̂����Ă������܂�",		MENU_ID_CONFIG_NAVI},
	{"Ext.�f�o�C�X",		"Ext.�f�o�C�X�������Ă������܂�",	MENU_ID_CONFIG_EXTDEVICE},
	{"���߂�",				"���߂�� �����Ă������܂�",		MENU_ID_DISPLAY},
	{"���񂹂�",			"�i�r�̂��񂹂��������Ă����܂�",	MENU_ID_VOICE},
	{"�o���I���h�L",		"�o���I���h�L�� �����Ă����܂�",	MENU_ID_CONFIG_VARIO},
	{"�A�V�X�g���[�h",		"�A�V�X�g���[�h�������Ă����܂�",	MENU_ID_CONFIG_ETC},
	{"���傫��",			"�����Ă��� ���傫�����܂�",		MENU_FCALL,		MP_Reload},
	{0, 0,														MENU_ID_MAIN}
};

///////////////////////////////////////////////////////////////////////////////
// GPS�ݒ�

const IntInputTemplate GPS_N_CALIB = {
	3, 1, 0, -180, 180,
	"�ق��� ������",
	"��",
	IW_OFFSET(tc.n_calib)
};
const IntInputTemplate GPS_TZONE = {
	3, 1, 0, -720, 720,
	"�^�C���]�[��",
	"min",
	IW_OFFSET(tc.tzone_m)
};
const IntInputTemplate GPS_STOP_DIR = {
	4, 0, 3, 0, 9999,
	"�䂤���� ������",
	"m/s",
	IW_OFFSET(tc.stop_dir)
};
const u8* const GPS_ALT[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.alt_type),
	"�����΂�",
	"WGS84������",
};
const u8* const GPS_CALC[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.calc_type),
	"���ǂ�",
	"��������",
	"����������",
};
const u8* const GPS_TALLY_CLEAR[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.tally_clr),
	"�}�j���A��",
	"�T�X�y���h",
	"�e�C�N�I�t",
};
const IntInputTemplate ETC_AUTOOFF = {
	3, 0, 0, 0, 999,
	"�I�[�g�I�t",
	"min",
	IW_OFFSET(tc.auto_off)
};

const u8* const GPS_NMEA_UP[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.nmea_up),
	"�����ǂ�",
	"�n�[�t",
	"�N�H�[�^",
};
const u8* const NAVI_VIEW_SBAS[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.waas_flag),
	"�ނ���",
	"�䂤����",
};

#define HELP_CONFIG_GPS1	"N�ق������� �ق������܂�. �����ႭN�� W007(-7��)�ł�.  <>"
#define HELP_CONFIG_GPS2	"�^�C���]�[���� �����Ă����܂�. JST�� +540min (+9 Hour) �ł�.  " \
							"�T�}�[�^�C����,�����ł����Ă����Ă�������.  <>"
#define HELP_CONFIG_GPS3	"�����Ă������X�s�[�h���������,�R���p�X�������� �䂤�����ɂȂ�܂�.  <>"
#define HELP_CONFIG_GPS4	"�����ǂ� ���񂵂�ق��ق��� ����т܂�.  <>"
#define HELP_CONFIG_GPS5	"�����/�ق����A���S���Y���� ����т܂�.  "\
							"\"���ǂ�\":�A���S���Y���� ���ǂ��� ���񂽂����܂�.  "\
							"\"��������\":�ǂ����A���S���Y���������܂�.  "\
							"\"����������\":�q���[�x�j�������傤�����������܂�.(������!)  <>"
#define HELP_CONFIG_GPS6	"�t���C�g���O�̃N���A�ق��ق��� ����т܂�.  " \
							"\"�}�j���A��\":�t���C�g���O�r���[��START�{�^����������,���O���N���A���܂�.  " \
							"\"�T�X�y���h\":�T�X�y���h�����,���ǂ��Ń��O���N���A���܂�.  " \
							"\"�e�C�N�I�t\":�e�C�N�I�t�����,���ǂ��Ń��O���N���A���܂�.  <>"
#define HELP_CONFIG_GPS7	"GPS�������{�^�����������Ȃ��Ƃ���,�����Ă�����������ŃT�X�y���h���܂�. " \
							"0min�������Ă������,���ǂ��T�X�y���h���܂���. " \
							"���W���[������Ƃ��ɂ�,L+R�{�^���������܂�.  <>"
#define HELP_CONFIG_GPS8	"NMEA�̃o���I��������ق��ق��� ����т܂�. " \
							"\"�����ǂ�\":�����ǂ��� ���̂܂܃o���I�ɂ����܂�. " \
							"\"�n�[�t\":�����ǂ��� ���Ԃ�� 1/2�ɂȂ炵�� �������񂵂܂�. " \
							"\"�N�H�[�^\":�����ǂ��� ���Ԃ�� 1/4�ɂȂ炵�� �������񂵂܂�.  <>"

#define HELP_CONFIG_GPS9	"WAAS��MSAS�Ȃǂ�SBAS�����悤���邩 ����т܂�. " \
							"(���̂����Ă���,�����ǃ}�[�J�[�̂Ђ傤�������肩���邽�߂� �����܂�.)  " \
							"\"�ނ���\":�����ɂ�������,�����ǃ}�[�J�[�� �ւ񂩂��܂�.  " \
							"\"�䂤����\":SBAS�̂��サ��ɂ�������,�����ǃ}�[�J�[�� �ւ񂩂��܂�.  <>"

const MenuItem MENU_CONFIG_GPS[] = {
	{"N�ق���",				HELP_CONFIG_GPS1,					MENU_SEL_VAL,	&GPS_N_CALIB},
	{"�^�C���]�[��",		HELP_CONFIG_GPS2,					MENU_SEL_VAL,	&GPS_TZONE},
	{"�����R���p�X",		HELP_CONFIG_GPS3,					MENU_SEL_VAL,	&GPS_STOP_DIR},
	{"������",				HELP_CONFIG_GPS4,					MENU_SEL_ENUM,	&GPS_ALT},
	{"��������",			HELP_CONFIG_GPS5,					MENU_SEL_ENUM,	&GPS_CALC},
	{"NMEA�o���I",			HELP_CONFIG_GPS8,					MENU_SEL_ENUM,	&GPS_NMEA_UP},
	{"�I�[�g�I�t",			HELP_CONFIG_GPS7,					MENU_SEL_VAL,	&ETC_AUTOOFF},
	{0, 0,														MENU_ID_CONFIG}
};

///////////////////////////////////////////////////////////////////////////////
// �i�r�ݒ胁�j���[

const IntInputTemplate NAVI_NEAR = {
	4, 0, 0, 0, 9999,
	"�j�A�T�E���h",
	"m",
	IW_OFFSET(tc.near_beep)
};
const u8* const NAVI_VIEW_LOCK[] = {
	(u8*)4,
	(u8*)IW_OFFSET(tc.auto_lock),
	"�}�j���A��/Msg",
	"�e�C�N�I�t/Msg",
	"�}�j���A��",
	"�e�C�N�I�t",
};
const IntInputTemplate NAVI_LD = {
	5, 0, 3, 1, 99999,
	"�O���C�_L/D",
	"",
	IW_OFFSET(tc.my_ldK)
};
const IntInputTemplate NAVI_DOWNRATION = {
	4, 0, 3, 1, 9999,
	"�V���N���[�g",
	"m/s",
	IW_OFFSET(tc.my_down)
};
const u8* const NAVI_AVG_TYPE[] = {
	(u8*)6,
	(u8*)IW_OFFSET(tc.avg_type),
	"���ǂ�",
	"�O���C�_���Ă�",
	"���A���^�C��",
	"3s �ւ�����",
	"10s �ւ�����",
	"30s �ւ�����",
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
	"�Ȃ�",
	"����",
};

#define HELP_CONFIG_NAVI1	"�V�����_�������񂶂�,�j�A�r�[�v���Ȃ炷������ �����Ă����܂�.  <>"
#define HELP_CONFIG_NAVI2	"�Ƃ����Ⴍ��������(�悻��������,�悻��������)��,�A���S���Y��������т܂�.  " \
							"\"���ǂ�\":���ǂ��ŃA���S���Y�������񂽂����܂�." \
								"(�Z���^�����O���� <�O���C�_���Ă�> ������, �ق��� <n�ւ�����> �������܂�)  " \
							"\"�O���C�_���Ă�\":���炩���߃C���v�b�g���ꂽ, <L/D> �� <Sink rate> ���� " \
								"�݂���������Ȃ��܂�.  " \
							"\"���A���^�C��\":�˂� ��������f�[�^��������,�݂���������Ȃ��܂�.  " \
							"\"n�ւ�����\":����n�����̃f�[�^���ւ����񂵂�,�݂��肵�܂�.  <>"
#define HELP_CONFIG_NAVI3	"���Ȃ��̃O���C�_�� L/D(���[�X�Z�b�e�B���O)�� �����Ă����܂�. " \
							"�݂��肪 <���ǂ�> �܂��� <�O���C�_���Ă�> �̂Ƃ��� ���悤���܂�.  <>"
#define HELP_CONFIG_NAVI4	"���Ȃ��̃O���C�_�� Sink rate(���[�X�Z�b�e�B���O)�� �����Ă����܂�. " \
							"�݂��肪 <���ǂ�> �܂��� <�O���C�_���Ă�> �̂Ƃ��� ���悤���܂�.  <>"
#define HELP_CONFIG_NAVI5	"L/D��SinkRate�̃`�F�b�N�̂��߂�,�O���C�_�� �����������ǂ� �Ђ傤�����Ă��܂�. " \
							"< Airspeed(km/h) = 3.6 * SinkRate(m/s) * L/D >.  <>"
#define HELP_CONFIG_NAVI6	"�L�[���b�N�̂ق��ق�������т܂�. " \
							"\"�}�j���A��/Msg\":�i�r���߂��START+SELECT��������,�L�[���b�N���܂�. " \
								"���b�N���イ�ɃL�[��������, �A�����b�N�̂ق��ق����Ђ傤�����܂�.  " \
							"\"�e�C�N�I�t/Msg\":�i�r���߂�Ńe�C�N�I�t�����,���ǂ��ŃL�[���b�N���܂�. " \
								"���b�N���イ�ɃL�[�������� ,�A�����b�N�̂ق��ق����Ђ傤�����܂�.  " \
							"\"�}�j���A��\":�i�r���߂�Ńe�C�N�I�t�����,���ǂ��ŃL�[���b�N���܂�. " \
								"���b�N���イ�ɃL�[�������Ă�, �Ȃɂ����܂���.  " \
							"\"�e�C�N�I�t\":�i�r���߂�Ńe�C�N�I�t�����,���ǂ��ŃL�[���b�N���܂�. " \
								"���b�N���イ�ɃL�[�������Ă�, �Ȃɂ����܂���.  " \
							"(�L�[���b�N���������傷��Ƃ��ɂ�,START+SELECT�������܂�)  <>"
#define HELP_CONFIG_NAVI7	"�j�A�T�E���h��L�[�N���b�N�Ȃǂ�,�����傤�������Ă����܂�.   <>"
#define HELP_CONFIG_NAVI8	"���p�C�����̂����ǂ��傤�ق���,�Ђ傤���Ȃ��悤�� ����т܂�.  " \
							"\"ArrivalAlt(a)\":���p�C�����̂悻���Ƃ����Ⴍ�����ǂ� �Ђ傤�����܂�.  " \
							"\"ArrivalDiff(@)\":���p�C�����̂悻���Ƃ����Ⴍ�����ǂ�, " \
								"���񂴂������ǂ� �����ǂ��� �Ђ傤�����܂�.  " \
							"\"Diff(d)\":���p�C�����̂����ǂ�, ���񂴂������ǂ� �����ǂ��� �Ђ傤�����܂�.  " \
							"\"Goal(g)\":�S�[���p�C������ �悻���Ƃ����Ⴍ �����ǂ��� �Ђ傤�����܂�. " \
							"\"Next L/D\":���p�C�����܂ł�L/D�� �Ђ傤�����܂�. " \
							"\"Goal L/D\":�S�[���p�C�����܂ł�L/D�� �Ђ傤�����܂�. " \
							"(�悤���������ǂ̃A���S���Y����,\"�݂���\"�� �����Ă��ł��܂�.)  <>"
#define HELP_CONFIG_NAVI9	"GPS�G���[�̂��������Ђ傤��������т܂�.  " \
							"\"�Ȃ�\":�|�b�v�A�b�v�G���[���Ђ傤�����܂���. " \
							"\"����\":�|�b�v�A�b�v�ŃG���[���Ђ傤�����܂�.  <>"

const MenuItem MENU_CONFIG_NAVI[] = {
	{"�L�[���b�N",			HELP_CONFIG_NAVI6,					MENU_SEL_ENUM,	&NAVI_VIEW_LOCK},
	{"�j�A�T�E���h",		HELP_CONFIG_NAVI1,					MENU_SEL_VAL,	&NAVI_NEAR},
	{"�{�����[��",			HELP_CONFIG_NAVI7,					MENU_SEL_ENUM,	&NAVI_VOLUME},
	{"�݂���",			HELP_CONFIG_NAVI2,					MENU_SEL_ENUM,	&NAVI_AVG_TYPE},
	{"�Ђ傤��2",			HELP_CONFIG_NAVI8,					MENU_SEL_ENUM,	&NAVI_VIEW_ALT},
	{"-L/D",				HELP_CONFIG_NAVI3,					MENU_SEL_VAL,	&NAVI_LD},
	{"-Sink rate",			HELP_CONFIG_NAVI4,					MENU_SEL_VAL,	&NAVI_DOWNRATION},
//	{" (Airspeed)",			HELP_CONFIG_NAVI5,					MENU_SEL_SPEED},
	{"GPS��������",			HELP_CONFIG_NAVI9,					MENU_SEL_ENUM,	&NAVI_VIEW_WARN},
	{0, 0,														MENU_ID_CONFIG}
};

///////////////////////////////////////////////////////////////////////////////
// �O���f�o�C�X���j���[

const MenuItem MENU_CONFIG_EXTDEVICE[] = {
	{"Bluetooth",			"Bluetooth���j�b�g�̂����Ă�",		MENU_ID_BLUETOOTH},
	{"�A�l�����[�^",		"�A�l�����[�^�̂����Ă�",			MENU_ID_CONFIG_ANEMOMETER},
	{0, 0,														MENU_ID_CONFIG}
};

///////////////////////////////////////////////////////////////////////////////
// Bluetooth�ݒ胁�j���[
const u8* const BT_MODE[] = {
	(u8*)5,
	(u8*)IW_OFFSET(tc.bt_mode),

	// 0: Bluetooth�Ȃ��BGPS����
	"�Ȃ�",

	// 1-4: Parani-ESD 38.4/57.6/115/9.6�̂ݑI����(bit���Z���邽�ߏ��ԏd�v)
	//      ���ۂ́AParani-ESD��2KB�o�b�t�@������̂�NMEA�f�[�^�ʒ��x�Ȃ�
	//      �ǂ̃{�[���[�g�ł�OK�B(����d�����l������9.6Kbps�APC�ƒʐM����Ȃ�115k)
	"P-ESD 38.4K",
	"P-ESD 57.6K",
	"P-ESD 115K",
	"P-ESD 9.6K",

	// 5-: RFU
};

const NameInputTemplate BT_ADDR = {
	BT_ADDR_LEN, "�������A�h���X",
	(u8*)IW_OFFSET(tc.bt_addr)
};
const NameInputTemplate BT_PIN = {
	BT_PIN_LEN, "������PIN",
	(u8*)IW_OFFSET(tc.bt_pin)
};

#define HELP_BLUETOOTH1	"Bluetooth���j�b�g�Ƃ� �����񃂁[�h������т܂�. Bluetooth������Ȃ��Ƃ��ɂ�,\"�Ȃ�\"������т܂�.  <>"
#define HELP_BLUETOOTH2	"�y�A�����O����Bluetooth�f�o�C�X�̃A�h���X�����Ă����܂�.  " \
						"\"BT�T�[�`\"��������,�������̃f�o�C�X���T�[�`���� ���X�g����GPS��PC�����񂽂��ł��܂�.  <>"
#define HELP_BLUETOOTH3	"BT�A�h���X��PIN�R�[�h�����悤����, GPS��PC�ɂ��������܂�. " \
						"�������� �������������, ���ǂ��y�A�����O���[�h�ɂ��肩���܂�.  <>"
#define HELP_BLUETOOTH4	"GPS�Ƃ̃y�A�����O��PC�܂��������[�h���������債, �I�t���C�����[�h�ɂ��܂�. (���傤�G�l)  <>"

const MenuItem MENU_BLUETOOTH[] = {
	{"���[�h",				HELP_BLUETOOTH1,					MENU_SEL_ENUM,	&BT_MODE},
	{"BT�A�h���X",			HELP_BLUETOOTH2,					MENU_SEL_NAME,	&BT_ADDR},
	{"PIN�R�[�h",			"������PIN�������Ă����܂�",		MENU_SEL_NAME,	&BT_PIN},
	{"BT�T�[�`",			"�������̃f�o�C�X���������܂�",		MENU_FCALL,		MP_BtInq},
	{"�y�A�����O",			HELP_BLUETOOTH3,					MENU_FCALL,		MP_BtConnect},
	{"PC�܂�����",			"�܂��������[�h�ɂ��܂�",			MENU_FCALL,		MP_BtScan},
	{"�I�t���C��",			HELP_BLUETOOTH4,					MENU_FCALL,		MP_BtDisconnect},
	{0, 0,														MENU_ID_CONFIG_EXTDEVICE}
};

///////////////////////////////////////////////////////////////////////////////
// �A�l�����[�^�ݒ胁�j���[

const IntInputTemplate ANEMO_PARAM = {
	9, 0, 0, 0, 999999999,
	"��������",
	"",
	IW_OFFSET(tc.anemo_coef)
};

const u8* const ANEMO_SPDUNIT[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.anemo_unit),
	"m/s",
	"km/h",
};

#define HELP_CONFIG_ANEMO1	"�A�l�����[�^�̃X�s�[�h����������,�}�j���A���ł����Ă����܂�. " \
							"�A�l�����[�^��, �ӂ����� V m/s �̂Ƃ��� �܂��т傤 N �p���X ���J�E���g����Ƃ��ɂ�,���̂��������� �����Ă����܂�. " \
							"�������� = N * 1638400 / V  <>"
#define HELP_CONFIG_ANEMO2	"�A�l�����[�^�̃X�s�[�h����������,���t�@�����X�f�o�C�X�������� �����Ă����܂�. " \
							"A�{�^���������� �ق������X�^�[�g��, �������ɂɃ��t�@�����X�ӂ������� �ɂイ��傭���܂�. " \
							"���t�@�����X�ق������イ��, �Ă����傤�ӂ��� �Ȃ����Ă�������.  <>"
#define HELP_CONFIG_ANEMO3	"�A�l�����[�^�̃X�s�[�h����������,GPS�������� �����Ă����܂�. " \
							"GPS������������A�{�^����������,�ق����� �͂��܂�܂�. " \
							"GPS�ق�����, �ނӂ����傤�����̂Ƃ��� �����Ă������ǂł��ǂ����Ȃ��� �����Ȃ��܂�.  <>"
#define HELP_CONFIG_ANEMO4	"�G�A�X�s�[�h�� �����ǂ��񂢂�����т܂�.  " \
							"\"m/s\":�т傤���� N ���[�g���� �����܂�.  " \
							"\"km/h\":������ N ���[�g���� �����܂�.  <>"

const MenuItem MENU_CONFIG_ANEMOMETER[] = {
	{"�����ǂ���",	HELP_CONFIG_ANEMO4,					MENU_SEL_ENUM,	&ANEMO_SPDUNIT},
	{"�ق���",			HELP_CONFIG_ANEMO1,					MENU_SEL_VAL,	&ANEMO_PARAM},
	{"�ق���(2)     (Reference)",	HELP_CONFIG_ANEMO2,					MENU_FCALL,		MP_WCALIB2},
	{"�ق���(3)     (GPS)",	HELP_CONFIG_ANEMO3,					MENU_FCALL,		MP_WCALIB3},
	{"�e�X�g",			"�A�l�����[�^���e�X�g���܂�",		MENU_FCALL,		MP_Anemometer},
	{0, 0,													MENU_ID_CONFIG_EXTDEVICE}
};

///////////////////////////////////////////////////////////////////////////////
// �f�B�X�v���C���j���[
const u8* const DISP_PYLON[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.pylon_type),
	"�t���l�[��",
	"���X�g",
	"�g�O��",
};
const IntInputTemplate DISP_INITIAL_POS = {
	2, 0, 0, 1, 10,
	"���� ����",
	"�΂�",
	IW_OFFSET(tc.initial_pos)
};
const IntInputTemplate DISP_PMAX = {
	4, 1, 0, -9999, 9999,
	"�p�C�����A���[Max",
	"m",
	IW_OFFSET(tc.ar_max)
};
const IntInputTemplate DISP_PMIN = {
	4, 1, 0, -9999, 9999,
	"�p�C�����A���[Min",
	"m",
	IW_OFFSET(tc.ar_min)
};
const IntInputTemplate DISP_SELF_R = {
	4, 0, 0, 0, 9999,
	"�|�W�V����(r)",
	"m",
	IW_OFFSET(tc.self_r)
};

/* �i�r���j���[��B+�J�[�\���Ő؂�ւ�����̂ŁA�J�b�g
 * TIPS�ɂ��\�����Ă��邵�c
const u8* const NAVI_VIEW_MODE[] = { // MAX_VIEW_MODE
	(u8*)(MAX_VIEW_MODE - 4),
	(u8*)IW_OFFSET(tc.view_mode),
	"�X�^���_�[�h",
	"�X�^���_�[�hR1",
	"�X�^���_�[�hR2",
	"�X�^���_�[�hR3",
	"�V���v��",
	"�V���v��    R1",
	"�V���v��    R2",
	"�V���v��    R3",
	"�^�[�Q�b�g",
	"�^�[�Q�b�g  R1",
	"�^�[�Q�b�g  R2",
	"�^�[�Q�b�g  R3",
	"�p�C����x2",
	"�p�C����x2  R1",
	"�p�C����x2  R2",
	"�p�C����x2  R3",
	"�E�C���h",
	"�E�C���h    R1",
	"�E�C���h    R2",
	"�E�C���h    R3",
	"*CylinderMap",
	"*CylinderMapR1",
	"*CylinderMapR2",
	"*CylinderMapR3",
	"*Locus",
	"*Locus      R1",
	"*Locus      R2",
	"*Locus      R3",
	"�e�L�X�g",
	"�e�L�X�g    R1",
	"�e�L�X�g    R2",
	"�e�L�X�g    R3",
};
*/
const u8* const NAVI_SPDUNIT[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.spd_unit),
	"m/s",
	"km/h",
};

#define HELP_DISP1	"�^�X�N���イ�̃p�C�����߂��� �Ђ傤���ق��ق�������т܂�.  " \
					"\"�t���l�[��\":���p�C�����̂Ȃ܂��� �t���l�[���� �Ђ傤�����܂�.  " \
					"\"���X�g\":���p�C��������� �Ȃ܂���1������ ������債�܂�. " \
					"\"�g�O��\":�t���l�[���ƃ��X�g�� �������ɂЂ傤�����܂�.  <>"
#define HELP_DISP6	"�p�C���������X�g�Ђ傤������΂�����, �Ђ傤������ ���������� �����Ă����܂�. " \
					"\"1\"�������Ă�����ƃE�F�C�|�C���g�߂��̃C�j�V����������, \"10\"�������Ă������ �Ȃ܂��̂������̂����� �����܂�. " \
					"���̂����Ă��� \"�p�C����\"��\"�t���l�[��\"�̂Ƃ��ɂ� �����܂���.  <>"
#define HELP_DISP2	"���߂�� �����邳�� �����Ă����܂�.  �f�t�H���g�̓��x��2�ł�.  <>"
#define HELP_DISP3	"���p�C�����ւ� �悻���Ƃ����Ⴍ�����ǂ��� \"�A���[Max\" ��� ���������Ƃ�, " \
					"�p�C�����A���[�̂Ȃ����� ���������� �Ȃ�܂�.  <>"
#define HELP_DISP4	"���p�C�����ւ� �悻���Ƃ����Ⴍ�����ǂ��� \"�A���[Min\" ��� ���������Ƃ�, " \
					"�p�C�����A���[�̂Ȃ����� �������傤�� �Ȃ�܂�.  <>"
#define HELP_DISP5	"���񂴂����� �Ђ傤���X�P�[��(�͂񂯂�)�� �����Ă����܂�. " \
					"0m�̂Ƃ��ɂ�,�^�X�N�����Ă��̃Z�N�^(r)�������܂�.  <>"
#define HELP_CONFIG_NAVI0	"�r���[�^�C�v�����肩���܂�. �Ȃ܂��̂������� R �����Ă�����̂�, �����Ă񂵂܂�. " \
							"(�i�r���߂�� B+�J�[�\�� ��������,���傭���r���[�^�C�v�����肩�����܂�)  <>"
#define HELP_CONFIG_NAVI8	"�O���E���h�X�s�[�h�� �����ǂ��񂢂�����т܂�.  " \
							"\"m/s\":�т傤���� N ���[�g���� �����܂�.  " \
							"\"km/h\":������ N �L�����[�g���� �����܂�.  " \
							"(�����Ă���, �{�C�X�i�r�ɂ� �͂񂦂����܂�)  <>"

const MenuItem MENU_DISPLAY[] = {
//	{"�r���[�^�C�v",	HELP_CONFIG_NAVI0,					MENU_SEL_ENUM,	&NAVI_VIEW_MODE},
	{"�����ǂ���",	HELP_CONFIG_NAVI8,					MENU_SEL_ENUM,	&NAVI_SPDUNIT},
	{"�p�C����",		HELP_DISP1,							MENU_SEL_ENUM,	&DISP_PYLON},
	{"���� ����",		HELP_DISP6,							MENU_SEL_VAL,	&DISP_INITIAL_POS},
	{"SBAS",			HELP_CONFIG_GPS9,					MENU_SEL_ENUM,	&NAVI_VIEW_SBAS}, // �}�[�J�\���ݒ�Ȃ̂�"���߂�"�Ɉړ�
	{"�A���[Max",		HELP_DISP3,							MENU_SEL_VAL,	&DISP_PMAX},
	{"�A���[Min",		HELP_DISP4,							MENU_SEL_VAL,	&DISP_PMIN},
	{"�|�W�V����(r)",	HELP_DISP5,							MENU_SEL_VAL,	&DISP_SELF_R},
	{"�K���}�ق���",	HELP_DISP2,							MENU_SEL_PALETTE},
	{0, 0,													MENU_ID_CONFIG}
};

///////////////////////////////////////////////////////////////////////////////
// �����ݒ胁�j���[
#define VOICE_SIZE	11
#define VOICE_SET	"�Ȃ�", "�Ƃ��ǂ��A���`", "�A���`&���t�g",  "�p�C�����̂�",  "�m�[�}��A",  "�V���v��A",  \
							"�A���`+",		  "�A���`&���t�g+", "�p�C�����̂�+", "�m�[�}��A+", "�V���v��A+"
							

const u8* const VOICE_ENABLE[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.vm_enable),
	"�ނ���",
	"�䂤����",
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
	"�C���^�[�o��",
	"s",
	IW_OFFSET(tc.vm_interval)
};

const IntInputTemplate ETC_CYL_NEAR = {
	5, 0, 0, 0, 99999,
	"�j�A�V�����_",
	"m",
	IW_OFFSET(tc.cyl_near)
};

#define HELP_VOICE1	"���񂹂��i�r�̃��[�h������т܂�.  " \
					"\"�ނ���\":���񂹂��i�r�������܂���.   " \
					"\"�䂤����\":���񂹂��i�r�������܂�.  <>"
#define HELP_VOICE2	"���񂹂��i�r�̃C���^�[�o���� �����Ă����܂�. " \
					"���̂����Ă���, ���񂹂��^�C�v�̂�������'+'�}�[�N�����Ă�����̂� �Ă��悤����܂�.  <>"
#define HELP_VOICE4	"�j�A�V�����_�̂����������Ă����܂�. ���̂����Ă���Cylinder mode�̃j�A�V�����_�� ���傤���ł�. " \
					"�V�����_��100m�̂Ƃ��� �j�A�V�����_��200m�ɂ����Ă������, �p�C��������300m���Ȃ����j�A�V�����_�Ƃ͂񂾂񂵂܂�.  <>"

const MenuItem MENU_VOICE[] = {
	{"���񂹂��i�r",	HELP_VOICE1,						MENU_SEL_ENUM,	&VOICE_ENABLE},
	{"�C���^�[�o��",	HELP_VOICE2,						MENU_SEL_VAL,	&VOICE_INTERVAL},
	{"�v���t���C�g",	"�e�C�N�I�t�܂��� ���񂹂��i�r",	MENU_SEL_ENUM,	&VOICE_WAIT},
	{"�m�[�}��",		"�����傤���� ���񂹂��i�r",		MENU_SEL_ENUM,	&VOICE_NORMAL},
	{"�X�g�b�v",		"�Ă������イ�� ���񂹂��i�r",		MENU_SEL_ENUM,	&VOICE_STOP},
	{"�Z���^�����O",	"�Z���^�����O���イ�� ���񂹂�",	MENU_SEL_ENUM,	&VOICE_CENTER},
	{"�j�A�V�����_",	"�V�����_�������񂶂� ���񂹂�",	MENU_SEL_ENUM,	&VOICE_NEAR},
	{"�j�A�����",		HELP_VOICE4,						MENU_SEL_VAL,	&ETC_CYL_NEAR},
	{0, 0,													MENU_ID_CONFIG}
};

///////////////////////////////////////////////////////////////////////////////
// etc�ݒ胁�j���[
#define HELP_ETC1 "Locus(������)���[�h�������Ă����܂�. ���傤���傤���,�h�b�g�Ђ傤�����܂�. " \
					"�T�[�}���R�A�������ɂ񂷂�̂�,�����邩������܂���...  <>"
#define HELP_ETC2 "�E�C���h�`�F�b�N(�����ӂ���������)�̂����Ă������܂�. " \
					"�ӂ������� �����ނ��� �����Ă����邽�߂̃��[�h�ł�.  <>"
#define HELP_ETC3 "�O���C�_�̃x���`�}�[�N���[�h�������Ă����܂�. " \
					"���Ȃ��̃O���C�_�� L/D, Sink Rate, Speed �������Ă����܂�.  <>"
#define HELP_ETC4 "�X�e�[�g���񂵂�悤�p�����[�^�������Ă����܂�.  <>"
#define HELP_ETC5 "�A�V�X�g���[�h�ł����p�����[�^�������Ă����܂�.  <>"
#define HELP_ETC6 "�V�����_���[�h�������Ă����܂�. �V�����_���}�b�v�łЂ傤�����܂�.  <>"
#define HELP_ETC7 "�I�[�g�^�[�Q�b�g���[�h�������Ă����܂�.  <>"

const MenuItem MENU_CONFIG_ETC[] = {
	{"Locus mode",			HELP_ETC1,							MENU_ID_CONFIG_THERMAL},
	{"Cylinder mode",		HELP_ETC6,							MENU_ID_CONFIG_CYL},
	{"WindCheck mode",		HELP_ETC2,							MENU_ID_CONFIG_WINDCHECK},
	{"Glider Benchmark",	HELP_ETC3,							MENU_ID_CONFIG_BENCHMARK},
	{"Auto target mode",	HELP_ETC7,							MENU_ID_CONFIG_AUTOTARGET},
	{"�p�����[�^1",			HELP_ETC4,							MENU_ID_CONFIG_PARAM},
	{"�p�����[�^2",			HELP_ETC5,							MENU_ID_CONFIG_PARAM2},
	{0, 0,														MENU_ID_CONFIG}
};

///////////////////////////////////////////////////////////////////////////////
// �p�����[�^���j���[

const IntInputTemplate NAVI_PARAM_SPIRAL = {
	5, 0, 3, -99999, 0,
	"�X�p�C�����X�s�[�h",
	"m/s",
	IW_OFFSET(tc.spiral_spd)
};
const IntInputTemplate NAVI_PARAM_STALL = {
	5, 0, 3, -99999, 0,
	"�X�g�[���X�s�[�h",
	"m/s",
	IW_OFFSET(tc.stall_spd)
};
const IntInputTemplate NAVI_PARAM_START = {
	5, 0, 3, 0, 99999,
	"�e�C�N�I�t�X�s�[�h",
	"m/s",
	IW_OFFSET(tc.start_spd)
};
const IntInputTemplate NAVI_PARAM_PITCH_DIFF = {
	5, 0, 3, 0, 99999,
	"�s�b�`Diff",
	"m/s",
	IW_OFFSET(tc.pitch_diff)
};
const IntInputTemplate NAVI_PARAM_PITCH_FREQ = {
	2, 0, 0, 1, 30,
	"�s�b�`���OMAX",
	"s",
	IW_OFFSET(tc.pitch_freq)
};
const IntInputTemplate NAVI_PARAM_ROLL_DIFF = {
	3, 0, 0, 0, 360,
	"���[��Diff",
	"��",
	IW_OFFSET(tc.roll_diff)
};
const IntInputTemplate NAVI_PARAM_ROLL_FREQ = {
	2, 0, 0, 1, 30,
	"���[�����OMAX",
	"s",
	IW_OFFSET(tc.roll_freq)
};

#define HELP_PARAM1 "�X�p�C�����̃V���N�X�s�[�h�������Ă����܂�. " \
					"���񂩂�����,���̃X�s�[�h�ŃV���N�����,�X�p�C�����Ƃɂ񂵂����܂�. " \
					"(�V���N�ł̃Z���^�����O��,�X�p�C�����Ƃɂ񂵂����܂�!)  <>"
#define HELP_PARAM2 "�X�g�[���̃V���N�X�s�[�h�������Ă����܂�. " \
					"���̃X�s�[�h�ł�����,�X�g�[���Ƃɂ񂵂����܂�. " \
					"(�悢�V���N��,�X�g�[���Ƃɂ񂵂����܂�!)  <>"
#define HELP_PARAM3 "�e�C�N�I�t�X�s�[�h�������Ă����܂�. ���̃X�s�[�h�ł�������,�e�C�N�I�t�Ƃɂ񂵂����܂�.  <>"
#define HELP_PARAM4 "�s�b�`���O�`�F�b�N�p�����[�^. �s�b�`���O���񂵂����,�����������ǂ� �ւ񂩂�傤. " \
					"(���イ��:�������傭 �����ǂ� �ւ񂩂ł� ����܂���!) <>"
#define HELP_PARAM5 "�s�b�`���O���イ���� ��������������. " \
					"�����肨�����s�b�`�����肩�����Ă�,�����̃y�[�X�`�F���W�Ƃɂ񂵂�����,�s�b�`���O�Ƃ݂Ȃ��܂���.  <>"
#define HELP_PARAM6 "���[�����O�`�F�b�N�p�����[�^. ���[�����O���񂵂����,�ւ� ������.  " \
					"���̂����ǂ� ��񂼂����肩������ ���[�����O�Ƃ݂Ȃ��܂�.  <>"
#define HELP_PARAM7 "���[�����O���イ���� ��������������. " \
					"�����肨�������[�������肩�����Ă�,�����̃^�[���� �ɂ񂵂�����,���[�����O�Ƃ݂Ȃ��܂���.  <>"
const MenuItem MENU_CONFIG_PARAM[] = {
	{"�X�p�C����",			HELP_PARAM1,						MENU_SEL_VAL,	&NAVI_PARAM_SPIRAL},
	{"�X�g�[��",			HELP_PARAM2,						MENU_SEL_VAL,	&NAVI_PARAM_STALL},
	{"�e�C�N�I�t",			HELP_PARAM3,						MENU_SEL_VAL,	&NAVI_PARAM_START},
	{"�s�b�`Diff",			HELP_PARAM4,						MENU_SEL_VAL,	&NAVI_PARAM_PITCH_DIFF},
	{"�s�b�`Freq",			HELP_PARAM5,						MENU_SEL_VAL,	&NAVI_PARAM_PITCH_FREQ},
	{"���[��Diff",			HELP_PARAM6,						MENU_SEL_VAL,	&NAVI_PARAM_ROLL_DIFF},
	{"���[��Freq",			HELP_PARAM7,						MENU_SEL_VAL,	&NAVI_PARAM_ROLL_FREQ},
	{0, 0,														MENU_ID_CONFIG_ETC}
};

///////////////////////////////////////////////////////////////////////////////
// �p�����[�^���j���[
const IntInputTemplate ETC_STABLE_SPEED = {
	5, 0, 3, 0, 99999,
	"�����ǃ}�[�W��",
	"m/s",
	IW_OFFSET(tc.stable_speed)
};
const IntInputTemplate ETC_STABLE_ANGLE = {
	3, 0, 0, 1, 360,
	"�����ǃ}�[�W��",
	"��",
	IW_OFFSET(tc.stable_angle)
};
const IntInputTemplate ETC_INIT_WAIT = {
	3, 0, 0, 0, 999,
	"���傫�E�F�C�g",
	"s",
	IW_OFFSET(tc.init_wait)
};
const IntInputTemplate ETC_WAIT_TIMEOUT = {
	3, 0, 0, 0, 999,
	"�^�C���A�E�g",
	"s",
	IW_OFFSET(tc.wait_timeout)
};
const IntInputTemplate ETC_COMP_TIMEOUT = {
	3, 0, 0, 0, 999,
	"�Ђ傤�� ������",
	"s",
	IW_OFFSET(tc.comp_timeout)
};
const IntInputTemplate ETC_KEEP_RANGE = {
	4, 0, 3, 0, 9999,
	"�L�[�v�����W",
	"m/s",
	IW_OFFSET(tc.keep_range)
};

#define HELP_PARAM2_1 "WindCheck/Benchmark�ł���,����Ă������ǂ� �p�����[�^�������Ă����܂�.  " \
						"���񂱂��ق�����<Stable angle>�����u���Ă����," \
						"WindCheck/Benchmark�̂����Ă����X�^�[�g���܂���.  <>"
#define HELP_PARAM2_2 "WindCheck/Benchmark�ł���,����Ă������ǂ� �p�����[�^�������Ă����܂�.  " \
						"�O���E���h�X�s�[�h��<Stable speed>�����u���Ă����," \
						"WindCheck/Benchmark�̂����Ă����X�^�[�g���܂���.  <>"
#define HELP_PARAM2_3 "WindCheck/Benchmark���[�h�ւ� ���肩��肩��,�����Ă��������܂ł̃E�F�C�g�������Ă����܂�.  <>"
#define HELP_PARAM2_4 "WindCheck/Benchmark��,����Ă��܂��^�C���A�E�g�������Ă����܂�. " \
						"�^�C���A�E�g������������Ă� �O���C�_������Ă����Ȃ��Ƃ���, " \
						"���ǂ��Ńm�[�}���r���[�ɂ��ǂ�܂�.  <>"
#define HELP_PARAM2_5 "WindCheck/Benchmark��,�Ђ傤���^�C���A�E�g�������Ă����܂�. " \
						"�����Ă��R���v���[�g����,�������Ђ傤��������� ���߂܂�. " \
						"�^�C���A�E�g�����,���ǂ��Ńm�[�}���r���[�ɂ��ǂ�܂�.  <>"
#define HELP_PARAM2_6 "���t�g�ł��V���N�ł��Ȃ�, ���x���L�[�v�� ���傤���傤������W�������Ă����܂�. " \
						"���̃p�����[�^�̓t���C�g���O��Soaring Statistics�ɂ����܂�.  <> "

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
//�I�[�g�^�[�Q�b�g

const u8* const ETC_AUTOTARGET_MODE[] = {
	(u8*)4,
	(u8*)IW_OFFSET(tc.at_mode),
	"�I�t",
	"�^�C�v1",
	"�^�C�v2",
	"�^�C�v3",
};

const IntInputTemplate ETC_AUTOTARGET_MIN = {
	6, 0, 0, 0, 999999,
	"�������傤��",
	"m",
	IW_OFFSET(tc.at_min)
};
const IntInputTemplate ETC_AUTOTARGET_MAX = {
	6, 0, 0, 0, 999999,
	"����������",
	"m",
	IW_OFFSET(tc.at_max)
};
const IntInputTemplate ETC_AUTOTARGET_RECHECK = {
	6, 0, 0, 0, 999999,
	"�������񂳂�",
	"m",
	IW_OFFSET(tc.at_recheck)
};

#define HELP_AUTOTARGET1 "�t���[�t���C�g���[�h�� �^�[�Q�b�g�����Ă��ق��ق��� ����т܂�. " \
						"\"�I�t\":�I�[�g�^�[�Q�b�g�������܂���.  " \
						"\"�^�C�v1\": �^�[�Q�b�g�V�����_�ɂ͂����, ���ǂ��� ���̃^�[�Q�b�g�������Ă����܂�. " \
						"\"�^�C�v2\": �����Ă������W�ɃE�F�C�|�C���g���Ȃ��Ƃ���, �����W�ɂ������E�F�C�|�C���g���^�[�Q�b�g�ɂ��܂�. " \
						"\"�^�C�v3\": �^�C�v2�ɂ��킦��,���[�����Ox3 �������Ȃ��� �����炵���^�[�Q�b�g�������Ă����܂�.  <>"

#define HELP_AUTOTARGET2 "�I�[�g�^�[�Q�b�g�� \"�V�����_�T�C�Y\"+\"�������傤��\" ���� �Ƃ����E�F�C�|�C���g���炦��т܂�.  <>"
#define HELP_AUTOTARGET3 "�I�[�g�^�[�Q�b�g�� \"����������\" ���� �������E�F�C�|�C���g���炦��т܂�.  <>"
#define HELP_AUTOTARGET4 "�^�[�Q�b�g��\"�������񂳂�\"���Ƃ����Ȃ��,�����ǃT�[�`�������Ȃ��܂�. 0���Z�b�g����Ƃ������񂳂����܂���. (60s���ƂɃ`�F�b�N) <>"

const MenuItem MENU_CONFIG_AUTOTARGET[] = {
	{"���[�h",				HELP_AUTOTARGET1,					MENU_SEL_ENUM,	&ETC_AUTOTARGET_MODE},
	{"�������傤��",		HELP_AUTOTARGET2,					MENU_SEL_VAL,	&ETC_AUTOTARGET_MIN},
	{"����������",			HELP_AUTOTARGET3,					MENU_SEL_VAL,	&ETC_AUTOTARGET_MAX},
	{"�������񂳂�",		HELP_AUTOTARGET4,					MENU_SEL_VAL,	&ETC_AUTOTARGET_RECHECK},
	{0, 0,														MENU_ID_CONFIG_ETC}
};

///////////////////////////////////////////////////////////////////////////////
// ���t�g�T�[�`���[�h
const u8* const ETC_THERMAL_CMD[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.thermal_cmd),
	"B+A�{�^��",
	"�Z���^�����O",
};
const IntInputTemplate NAVI_LOCUS_R = {
	4, 0, 0, 1, 9999,
	"�Ђ傤���X�P�[��",
	"m",
	IW_OFFSET(tc.locus_r)
};
const IntInputTemplate NAVI_SAMPLE = {
	3, 0, 0, 1, 999,
	"�T���v�����O",
	"s",
	IW_OFFSET(tc.locus_smp)
};
const IntInputTemplate NAVI_LOCUS_CNT = {
	4, 0, 0, 1, 1800,
	"�|�C���g",
	"pt",
	IW_OFFSET(tc.locus_cnt)
};
const IntInputTemplate NAVI_LOCUS_UP = {
	4, 1, 0, -9999, 9999,
	"���������W",
	"m",
	IW_OFFSET(tc.locus_up)
};
const IntInputTemplate NAVI_LOCUS_DOWN = {
	4, 1, 0, -9999, 9999,
	"���������W",
	"m",
	IW_OFFSET(tc.locus_down)
};
const IntInputTemplate NAVI_LOCUS_RANGE = {
	4, 0, 3, 10, 9999,
	"�J���[�����W",
	"m/s",
	IW_OFFSET(tc.locus_range)
};

const u8* const NAVI_LOCUS_PAL[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.locus_pal),
	"����\\����\\����",
	"����\\����\\����",
	"���C���{�[",
};


#define HELP_THERMAL1	"Locus���[�h�̃R�}���h�� ����т܂�.  " \
						"\"B+A�{�^��\":�i�r���߂��B+A�{�^����������,Locus���[�h�ɂȂ�܂�.  " \
						"\"�Z���^�����O\":�Z���^�����O�����,���ǂ���Locus���[�h�ɂȂ�܂�.  " \
						"(B+A�{�^����Locus���[�h�ɂ��肩�����Ƃ��ɂ�, B�{�^���ŃL�����Z������܂�" \
						"Locus���[�h�������������܂�. " \
						"�Z���^�����O��Locus���[�h�ɂ��肩�����Ƃ��ɂ�, �Z���^�����O����߂�� " \
						"���ǂ���Locus���[�h���� �ӂ������܂�.)  <>"
#define HELP_THERMAL2	"�|�C���g�̂Ђ傤���X�P�[�������Ă����܂�.  " \
						"���߂񂿂イ����(�O���C�_�}�[�N)����R���p�X�T�[�N��(N/E/S/W)�܂ł�," \
						"�͂񂯂��� �����Ă����܂�.  <>"
#define HELP_THERMAL3	"�|�C���g�̃T���v�����O���񂩂��� ���Ă����܂�. " \
						"<�|�C���g>��120�|�C���g�̂Ƃ��� 5sec�ŃT���v�����O�����, " \
						"10min �̂������� �Ђ傤������܂�.  <>"
#define HELP_THERMAL4	"�Ђ傤������|�C���g������ ���Ă����܂�. " \
						"(120��肨��������,�|�C���g���`�������Ƃ�����܂�.)  <>"
#define HELP_THERMAL5	"���������Ђ傤�������W(���傤����)�������Ă����܂�.  " \
						"���������W��� �������|�C���g�� �Ђ傤�����܂���.  <>"
#define HELP_THERMAL6	"���������Ђ傤�������W(������)�������Ă����܂�.  " \
						"���������W��� �Ђ����|�C���g�� �Ђ傤�����܂���.  <>"
#define HELP_THERMAL7	"�o���I�ɂ������Ăւ񂩂���J���[�����W�̂����Ă������܂�.  " \
						"�J���[�����W��10���傭�łʂ�킯�܂�.  <>"
#define HELP_THERMAL8	"�J���[�p���b�g�����񂽂����܂�"

const MenuItem MENU_CONFIG_THERMAL[] = {
	{"�R�}���h",			HELP_THERMAL1,						MENU_SEL_ENUM,	&ETC_THERMAL_CMD},
	{"�X�P�[��",			HELP_THERMAL2,						MENU_SEL_VAL,	&NAVI_LOCUS_R},
	{"�T���v�����O",		HELP_THERMAL3,						MENU_SEL_VAL,	&NAVI_SAMPLE},
	{"�|�C���g",			HELP_THERMAL4,						MENU_SEL_VAL,	&NAVI_LOCUS_CNT},
	{"���������W",			HELP_THERMAL5,						MENU_SEL_VAL,	&NAVI_LOCUS_UP},
	{"���������W",			HELP_THERMAL6,						MENU_SEL_VAL,	&NAVI_LOCUS_DOWN},
	{"�J���[�����W",		HELP_THERMAL7,						MENU_SEL_VAL,	&NAVI_LOCUS_RANGE},
	{"�p���b�g",			HELP_THERMAL8,						MENU_SEL_ENUM,	&NAVI_LOCUS_PAL},
	{0, 0,														MENU_ID_CONFIG_ETC}
};


///////////////////////////////////////////////////////////////////////////////
// �V�����_�}�b�v���[�h
const u8* const ETC_CYL_CMD[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.cyl_cmd),
	"B+R�{�^��",
	"�j�A�V�����_",
};

const u8* const ETC_CYL_START[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.cyl_end),
	"���イ��傤",
	"��������",
};

const u8* const ETC_SP_PRIO[] = {
	(u8*)4,
	(u8*)IW_OFFSET(tc.sp_prio),
	"���񂱂�",
	"Cylinder mode",
	"Locus mode",
	"�g�O��",
};

#define HELP_CYL1	"�V�����_���[�h�̃R�}���h�� ����т܂�.  " \
						"\"B+R�{�^��\":�i�r���߂��B+R�{�^����������,�V�����_���[�h�ɂȂ�܂�.  " \
						"\"�j�A�V�����_\":�V�����_�ɂ����Â���,���ǂ��ŃV�����_���[�h�ɂȂ�܂�.  " \
						"(B+R�{�^���ŃV�����_���[�h�ɂ��肩�����Ƃ��ɂ�, B�{�^���ŃL�����Z������܂�" \
						"�V�����_���[�h�������������܂�. " \
						"�j�A�V�����_���񂵂�ŃV�����_���[�h�ɂ��肩�����Ƃ��ɂ�, �V�����_����͂Ȃ��� " \
						"���ǂ��ŃV�����_���[�h���� �ӂ������܂�.)  <>"
#define HELP_CYL2		"�j�A�V�����_�̂����������Ă����܂�. ���̂����Ă��� ���񂹂��i�r�̃j�A�V�����_�� ���傤���ł�. " \
						"�V�����_��100m�̂Ƃ��� �j�A�V�����_��200m�ɂ����Ă������, �p�C��������300m���Ȃ����j�A�V�����_�Ƃ͂񂾂񂵂܂�.  <>"

#define HELP_CYL3		"�^�X�N�X�^�[�g�܂��ɃX�^�[�g�V�����_�ւ���ɂイ�����Ƃ��̃��[�h�����񂽂����܂�. " \
						"\"��������\":�V�����_���[�h�������������܂�. " \
						"\"���イ��傤\":�V�����_���[�h�����イ��傤��,�m�[�}�����[�h�ɂ��ǂ�܂�. " \
						"(�����ǃV�����_����ł��,�ӂ����уV�����_���[�h�ɂȂ�܂�)  <>"

#define HELP_SP1		"Cylinder���[�h��Locus���[�h�� �䂤����ǂ� ���񂽂����܂�. " \
						"\"���񂱂�\":�����ɂ͂��������[�h�������������܂�. " \
						"\"Cylinder\":Cylinder���[�h���䂤���񂵂܂�. " \
						"\"Locus\":Locus���[�h���䂤���񂵂܂�. " \
						"\"�g�O��\":Cylinder��Locus���g�O�����܂�.  <>"

const MenuItem MENU_CONFIG_CYL[] = {
	{"�R�}���h",			HELP_CYL1,							MENU_SEL_ENUM,	&ETC_CYL_CMD},
	{"�j�A�V�����_",		HELP_CYL2,							MENU_SEL_VAL,	&ETC_CYL_NEAR},
	{"�X�^�[�g�܂�",		HELP_CYL3,							MENU_SEL_ENUM,	&ETC_CYL_START},
	{"�䂤�����",			HELP_SP1,							MENU_SEL_ENUM,	&ETC_SP_PRIO},
	{0, 0,														MENU_ID_CONFIG_ETC}
};

///////////////////////////////////////////////////////////////////////////////
// �����v�R�}���h
const u8* const ETC_WIND_CMD[] = {
	(u8*)5,
	(u8*)IW_OFFSET(tc.wind_cmd),
	"B+START�{�^��",
	"�s�b�`���O x3",
	"���[�����O x3",
	"�X�p�C����",
	"�X�g�[��",
};
const u8* const ETC_WIND_UPDATE[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.wind_update),
	"���Ȃ�",
	"����",
};
const IntInputTemplate NAVI_SPEED = {
	5, 0, 3, 0, 99999,
	"�j���[�g����",
	"m/s",
	IW_OFFSET(tc.my_speed)
};

#define HELP_WINDCHECK1	"�E�C���h�`�F�b�N���[�h(�����ӂ���������)�̃R�}���h�� ����т܂�.  " \
							"\"B+START�{�^��\":�i�r���߂��B+START�{�^����������,�E�C���h�`�F�b�N���[�h�ɂȂ�܂�.  " \
							"\"�s�b�`���O x3\":���傭���񂶂� ��񂼂�3�����s�b�`���O�����," \
							"���ǂ��ŃE�C���h�`�F�b�N���[�h�ɂȂ�܂�.  " \
							"\"���[�����O x3\":��񂼂�3�������[�����O�����,���ǂ��ŃE�C���h�`�F�b�N���[�h�ɂȂ�܂�.  " \
							"\"�X�p�C����\":�X�p�C���������,���ǂ��ŃE�C���h�`�F�b�N���[�h�ɂȂ�܂�.  " \
							"\"�X�g�[��\":�X�g�[�������,���ǂ��ŃE�C���h�`�F�b�N���[�h�ɂȂ�܂�.  " \
							"(�E�C���h�`�F�b�N���[�h��,B�{�^���ŃL�����Z�����邩,�^�C���A�E�g�����,�i�r�ɂӂ������܂�.)  <>"
#define HELP_WINDCHECK2	"�E�C���h�`�F�b�N���[�h��,�����Ă��R���v���[�g�����Ƃ���," \
							"<N�X�s�[�h>�����ǂ��ŃA�b�v�f�[�g���邩,����т܂�. " \
							"\"���Ȃ�\":���ǂ��ŃO���C�_�p�����[�^���A�b�v�f�[�g���܂���.  " \
							"\"����\":���ǂ��ŃO���C�_�p�����[�^���A�b�v�f�[�g���܂�.  <>"
#define HELP_WINDCHECK3	"���Ȃ��̃O���C�_�� �ӂ����������Ă��悤�̃j���[�g�����X�s�[�h�� �����Ă����܂�.  " \
							"�E�C���h�`�F�b�N���[�h�̃t�F�[�Y1�� �����܂�.  (km/h �ł͂Ȃ�,m/s �� ���Ă����܂�!)  <>"

const MenuItem MENU_CONFIG_WINDCHECK[] = {
	{"�R�}���h",			HELP_WINDCHECK1,					MENU_SEL_ENUM,	&ETC_WIND_CMD},
	{"�I�[�gUpdate",		HELP_WINDCHECK2,					MENU_SEL_ENUM,	&ETC_WIND_UPDATE},
	{"N�X�s�[�h",			HELP_WINDCHECK3,					MENU_SEL_VAL,	&NAVI_SPEED},
	{0, 0,														MENU_ID_CONFIG_ETC}
};


///////////////////////////////////////////////////////////////////////////////
// �O���C�_�x���`�}�[�N
const u8* const ETC_GLIDER_CMD[] = {
	(u8*)5,
	(u8*)IW_OFFSET(tc.glider_cmd),
	"B+SELECT�{�^��",
	"�s�b�`���O x3",
	"���[�����O x3",
	"�X�p�C����",
	"�X�g�[��",
};
const u8* const ETC_GLIDER_UPDATE[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.bench_update),
	"���Ȃ�",
	"����",
};

#define HELP_BENCHMARK1	"�x���`�}�[�N���[�h�̃R�}���h�� ����т܂�.  " \
						"\"B+SELECT�{�^��\":�i�r���߂��B+SELECT�{�^����������,�x���`�}�[�N���[�h�ɂȂ�܂�.  " \
						"\"�s�b�`���O x3\":���傭���񂶂� ��񂼂�3�����s�b�`���O����� " \
							"���ǂ��Ńx���`�}�[�N���[�h�ɂȂ�܂�.  " \
						"\"���[�����O x3\":��񂼂�3�������[�����O����� ���ǂ��Ńx���`�}�[�N���[�h�ɂȂ�܂�.  " \
						"\"�X�p�C����\":�X�p�C���������,���ǂ��Ńx���`�}�[�N���[�h�ɂȂ�܂�.  " \
						"\"�X�g�[��\":�X�g�[�������,���ǂ��Ńx���`�}�[�N���[�h�ɂȂ�܂�.  " \
						"(�x���`�}�[�N���[�h��,B�{�^���ŃL�����Z�����邩,�^�C���A�E�g�����,�i�r�ɂӂ������܂�.)  <>"
#define HELP_BENCHMARK2	"�x���`�}�[�N���[�h��,�����Ă��R���v���[�g�����Ƃ���," \
							"�O���C�_���傤�ق�(L/D, Sink rate)�����ǂ��ŃA�b�v�f�[�g���邩,����т܂�. " \
							"\"���Ȃ�\":���ǂ��ŃO���C�_�p�����[�^���A�b�v�f�[�g���܂���.  " \
							"\"����\":���ǂ��ŃO���C�_�p�����[�^���A�b�v�f�[�g���܂�.  <>"
#define HELP_BENCHMARK3	"�������ɃA�b�v�f�[�g���ꂽ L/D ���Ђ傤�� ���Ă��܂�. " \
						"���̃p�����[�^��,�i�r�R���t�B�O���j���[��<�݂���>�̃p�����[�^��,���傤�䂤���Ă��܂�.  <>"
#define HELP_BENCHMARK4	"�������ɃA�b�v�f�[�g���ꂽ Sink rate ���Ђ傤�� ���Ă��܂�. " \
						"���̃p�����[�^��,�i�r�R���t�B�O���j���[��<�݂���>�̃p�����[�^��,���傤�䂤���Ă��܂�.  <>"
#define HELP_BENCHMARK5	"�������ɃA�b�v�f�[�g���ꂽ Airspeed ���Ђ傤�� ���Ă��܂�.  <>"

const MenuItem MENU_CONFIG_BENCHMARK[] = {
	{"�R�}���h",			HELP_BENCHMARK1,					MENU_SEL_ENUM,	&ETC_GLIDER_CMD},
	{"�I�[�gUpdate",		HELP_BENCHMARK2,					MENU_SEL_ENUM,	&ETC_GLIDER_UPDATE},
	{"*L/D",				HELP_BENCHMARK3,					MENU_SEL_VAL,	&NAVI_LD},
	{"*Sink rate",			HELP_BENCHMARK4,					MENU_SEL_VAL,	&NAVI_DOWNRATION},
	{" (Airspeed)",			HELP_BENCHMARK5,					MENU_SEL_SPEED},
	{0, 0,														MENU_ID_CONFIG_ETC}
};

///////////////////////////////////////////////////////////////////////////////

#define HELP_INFO1	"���������e�L�X�g�� ���񂴂��̂��Ђ傤�� �Ђ傤�����܂�.  <>"

// ��񃁃j���[
const MenuItem MENU_INFO[] = {
//	{"�t���C�g���O",		"�t���C�g���O�� �����ɂ񂵂܂�",	MENU_FCALL,		MP_Tally},
//	{"�^�X�N���O",			"�^�X�N���O�� �����ɂ񂵂܂�",		MENU_FCALL,		MP_Task},
	{"���񂴂���",			HELP_INFO1,							MENU_FCALL,		MP_LatLon},
	{"���O�f�[�^",			"���O�� �����ɂ񂵂܂�",			MENU_ID_LOG},
	{"�O���t",				"�O���t�� �����ɂ񂵂܂�",			MENU_FCALL,		MP_Graph},
	{"GPS���j�b�g",			"GPS���j�b�g�� �����ɂ񂵂܂�",		MENU_FCALL,		MP_GPS},
	{"�J�[�g���b�W",		"�J�[�g���b�W�� �����ɂ񂵂܂�",	MENU_FCALL,		MP_Cart},
	{"�o�[�W����",			"�o�[�W������ �����ɂ񂵂܂�",		MENU_FCALL,		MP_Version},
	{"Tips",				"Tips���Ђ傤�����܂�",				MENU_FCALL,		MP_Tips},
	{0, 0,														MENU_ID_MAIN}
};

///////////////////////////////////////////////////////////////////////////////
// �o���I���h�L�ݒ胁�j���[
const u8* const VARIO_MODE_LIST[] = {
	(u8*)4,
	(u8*)IW_OFFSET(tc.vario_mode),
	"�Ȃ�",
	"�X���[�Y",
	"���񂩂�1",
	"���񂩂�2",
};
const IntInputTemplate VARIO_UP_CONFIG = {
	4, 1, 3, -9999, 9999, 
	"���t�g������",
	"m",
	IW_OFFSET(tc.vario_up)
};
const IntInputTemplate VARIO_DOWN_CONFIG = {
	4, 1, 3, -9999, 9999, 
	"�V���N������",
	"m",
	IW_OFFSET(tc.vario_down)
};
// �^�X�N���j���[
const u8* const VARIO_TO[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.vario_to),
	"�Ȃ炷",
	"�Ȃ炳�Ȃ�",
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

#define HELP_VARIO1	"�o���I�̃X�^�[�g�^�C�~���O�� ����т܂�.  " \
					"\"�Ȃ炷\":Takeoff�܂��� �o���I���Ȃ炵�܂�.  " \
					"\"�Ȃ炳�Ȃ�\":�E�F�C�e�B���O���Ă���Ƃ��ɂ� �o���I���Ȃ炵�܂���.  <>"
#define HELP_VARIO2	"�o���I�T�E���h�� �����傤�������Ă����܂�.  <>"

const MenuItem MENU_CONFIG_VARIO[] = {
	{"���[�h",				"�T�E���h�^�C�v�� ����т܂�",		MENU_SEL_ENUM,	&VARIO_MODE_LIST},
	{"���t�g",				"���t�g�������� �����Ă����܂�",	MENU_SEL_VAL,	&VARIO_UP_CONFIG},
	{"�V���N",				"�V���N�������� �����Ă����܂�",	MENU_SEL_VAL,	&VARIO_DOWN_CONFIG},
	{"Takeoff�܂�",			HELP_VARIO1,						MENU_SEL_ENUM,	&VARIO_TO},
	{"�{�����[��",			HELP_VARIO2,						MENU_SEL_ENUM,	&VARIO_VOLUME},
	{"�e�X�g",				"�o���I�̃T�E���h�e�X�g�����܂�",	MENU_FCALL,		MP_VarioTest},
	{0, 0,														MENU_ID_CONFIG}
};

///////////////////////////////////////////////////////////////////////////////
// �g���b�N���O�ݒ胁�j���[
const u8* const LOG_ENABLE[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.log_enable),
	"�}�j���A��",
	"���A���^�C��",
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
	"���O���񂩂�",
	"s",
	IW_OFFSET(tc.log_intvl)
};
const u8* const LOG_OVERWRITE[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.log_overwrite),
	"���O�Ă���",
	"���킪��",
};
const u8* const LOG_DEBUG[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.log_debug),
	"----",
	"Full PVT debug",
};

#define HELP_LOG1	"�g���b�N���O�̂��낭���[�h������т܂�.  " \
					"\"�}�j���A��\":GPS�ɂق��񂳂�Ă���g���b�N���O��,�}�j���A���Ń_�E�����[�h���Ă��낭���܂�.  " \
					"\"���A���^�C��\":GPS�̂������f�[�^�����A���^�C���ɂ��낭���܂�.  <>"
#define HELP_LOG2	"�����ւ����Ђ傤�� �ق��񂹂��ǂ�����т܂�.  " \
					"\"1ms\":1�~���т傤(�₭3cm)�� ���낭���܂�. �f�[�^�T�C�Y��,�����΂񂨂������Ȃ�܂�.  " \
					"\"8ms\":8�~���т傤(�₭24cm)�� ���낭���܂�.  " \
					"\"32ms\":32�~���т傤(�₭1m)�� ���낭���܂�.(�f�t�H���g)  " \
					"\"128ms\":128�~���т傤(�₭4m)�� ���낭���܂�.  �f�[�^�T�C�Y��,�����΂񂿂������Ȃ�܂�.  <>"
#define HELP_LOG3	"GPS����g���b�N���O���_�E�����[�h���܂�.  " \
					"�g���b�N���O�̃_�E�����[�h���イ��, ���A���^�C���ق���� ���イ���񂵂܂�.  <>"
#define HELP_LOG4	"�g���b�N���O���}�j���A���� �Ԃ񂩂��܂�.  (���̃|�C���g����, �����炵���Z�O�����g�ɂȂ�܂�)  <>"
#define HELP_LOG5	"�g���b�N���O���J�[�g���b�W�� �ǂ̃u���b�N�ɃZ�[�u���邩 �����Ă����܂�.  <>"
#define HELP_LOG6	"�g���b�N���O�̃Z�[�u�u���b�N������ �����Ă����܂�.  <>"
#define HELP_LOG7	"�g���b�N���O�����A���^�C���ق��񂷂�΂�����, �ق��񂩂񂩂��������Ă����܂�.  <>"
#define HELP_LOG8	"�g���b�N���O�� �����X�y�[�X���Ȃ��Ȃ����Ƃ��� �ǂ���������т܂�.  " \
					"\"���O�Ă���\":���O�������ς��ɂȂ��,���낭����߂� �G���[(ID=12)���}�[�N���܂�.  " \
					"\"���킪��\":���O�������ς��ɂȂ�� �ӂ邢���O���������債�ă��O�����낭���܂�.  <>"
#define HELP_LOG9	"                              <DEBUG ONLY>"
#define HELP_LOG10	"PC�փg���b�N���O���A�b�v���[�h���܂�. " \
					"�g���b�N���O���A�b�v���[�h���邽�߂ɂ�,PC�������P�[�u���� �Ђ悤�ł�. �@<>"
#define HELP_LOG11	"�g���b�N���O���A�b�v���[�h���邳����, �{�[���[�g������т܂�. " \
					"�{�[���[�g��������ƃX�s�[�h��������܂���, �{�[���[�g������������� ���񂫂傤�ɂ���Ă̓G���[�ɂȂ�΂���������܂�.  <>"

const MenuItem MENU_CONFIG_LOG[] = {
	{"�X�e�[�^�X",			"���O�X�e�[�^�X�������ɂ񂵂܂�",	MENU_FCALL,		MP_LogCheck},
	{"���낭���[�h",		"���O�p�����[�^�������Ă����܂�",	MENU_ID_CONFIG_LOG2},
	{"�Ԃ񂩂�",			HELP_LOG4,							MENU_FCALL,		MP_SegmentLog},
	{"���O�N���A",			"�g���b�N���O���N���A���܂�",		MENU_FCALL,		MP_ClearLog},
	{"�_�E�����[�h",		HELP_LOG3,							MENU_FCALL,		MP_DownloadLogPre},
	{"�A�b�v���[�h",		HELP_LOG10,							MENU_FCALL,		MP_UploadLogPre},
	{0, 0,														MENU_ID_MAIN}
};

const MenuItem MENU_CONFIG_LOG2[] = {
	{"���O�ق���",			HELP_LOG1,							MENU_SEL_ENUM,	&LOG_ENABLE},
	{"����������",			HELP_LOG2,							MENU_SEL_ENUM,	&LOG_PREC},
	{"���O���񂩂�",		HELP_LOG7,							MENU_SEL_VAL,	&LOG_INTVL},
	{"���O�G���h",			HELP_LOG8,							MENU_SEL_ENUM,	&LOG_OVERWRITE},
	{"(debug)",				HELP_LOG9,							MENU_SEL_ENUM,	&LOG_DEBUG},
	{0, 0,														MENU_ID_CONFIG_LOG}
};

#define HELP_LOG21	"���񂴂��̃t���C�g���O���J�[�g���b�W�Ƀ}�j���A���ŃZ�[�u���܂�.  <>"
#define HELP_LOG22	"���񂴂��̃^�X�N���O���J�[�g���b�W�Ƀ}�j���A���ŃZ�[�u���܂�.  <>"

// ��񃁃j���[
const MenuItem MENU_LOG[] = {
	{"�t���C�g���O",		"���񂴂��̃t���C�g���O���݂܂�",	MENU_FCALL,		MP_TallyCur},
	{"�^�X�N���O",			"���񂴂��̃^�X�N���O���݂܂�",		MENU_FCALL,		MP_TaskCur},
	{"���O�̃��[�h",		"�Z�[�u�������O���݂܂�",			MENU_ID_LOG_LIST},
	{"�t���C�g���O�̃Z�[�u",HELP_LOG21,							MENU_ID_LOG_FL_SAVE},
	{"�^�X�N���O�̃Z�[�u",	HELP_LOG22,							MENU_ID_LOG_TL_SAVE},
	{"���O�����Ă�",		"���O�̂����Ă������܂�",			MENU_ID_LOG_AUTO_SAVE},
	{0, 0,														MENU_ID_INFO}
};
const MenuItem MENU_LOG_LIST[] = {
	{"���O#1���݂�",		0,									MENU_SEL_FLOG,	MP_LogView},
	{"���O#2���݂�",		0,									MENU_SEL_FLOG,	MP_LogView},
	{"���O#3���݂�",		0,									MENU_SEL_FLOG,	MP_LogView},
	{"���O#4���݂�",		0,									MENU_SEL_FLOG,	MP_LogView},
	{0, 0,														MENU_ID_LOG}
};
const MenuItem MENU_LOG_FL_SAVE[] = {
	{"#1�ɃZ�[�u",			0,									MENU_SEL_FLOG,	MP_TallySave},
	{"#2�ɃZ�[�u",			0,									MENU_SEL_FLOG,	MP_TallySave},
	{"#3�ɃZ�[�u",			0,									MENU_SEL_FLOG,	MP_TallySave},
	{"#4�ɃZ�[�u",			0,									MENU_SEL_FLOG,	MP_TallySave},
	{0, 0,														MENU_ID_LOG}
};
const MenuItem MENU_LOG_TL_SAVE[] = {
	{"#1�ɃZ�[�u",			0,									MENU_SEL_FLOG,	MP_TaskSave},
	{"#2�ɃZ�[�u",			0,									MENU_SEL_FLOG,	MP_TaskSave},
	{"#3�ɃZ�[�u",			0,									MENU_SEL_FLOG,	MP_TaskSave},
	{"#4�ɃZ�[�u",			0,									MENU_SEL_FLOG,	MP_TaskSave},
	{0, 0,														MENU_ID_LOG}
};

const u8* const LOG_FL_AUTOSAVE[] = {
	(u8*)2,
	(u8*)IW_OFFSET(tc.flog_as),
	"�}�j���A��S",
	"�T�X�y���hAS",
};
const u8* const LOG_TL_AUTOSAVE[] = {
	(u8*)3,
	(u8*)IW_OFFSET(tc.tlog_as),
	"�}�j���A��S",
	"�S�[��AS",
	"�p�C����AS",
};

#define LOG_AS1	"�t���C�g���O�̃I�[�g�Z�[�u�̂����Ă�������т܂�. " \
				"\"�}�j���A��S\":�I�[�g�Z�[�u���܂���.  " \
				"\"�T�X�y���hAS\":�T�X�y���h�̂Ƃ��ɃI�[�g�Z�[�u���܂�.  <>"
#define LOG_AS2	"�^�X�N���O�̃I�[�g�Z�[�u�̂����Ă�������т܂�. " \
				"\"�}�j���A��S\":�I�[�g�Z�[�u���܂���.  " \
				"\"�S�[��AS\":�S�[�������Ƃ��ɃI�[�g�Z�[�u���܂�. " \
				"\"�p�C����AS\":�p�C�������Ƃ����Ƃ��ɃI�[�g�Z�[�u���܂�.  <>"

const MenuItem MENU_LOG_AUTO_SAVE[] = {
	{"�t���C�g���O",		LOG_AS1,							MENU_SEL_ENUM,	&LOG_FL_AUTOSAVE},
	{"�^�X�N���O",			LOG_AS2,							MENU_SEL_ENUM,	&LOG_TL_AUTOSAVE},
	{"���O�N���A",			HELP_CONFIG_GPS6,					MENU_SEL_ENUM,	&GPS_TALLY_CLEAR},
	{0, 0,														MENU_ID_LOG}
};

///////////////////////////////////////////////////////////////////////////////
// etc
///////////////////////////////////////////////////////////////////////////////
const u8* const TESTMENU_MODE[] = {
	(u8*)4,
	(u8*)IW_OFFSET(mp.test_mode),
	"�I�t",
	"�I��",
	"�u�[�X�gx5",
	"�t���X�s�[�h",
};

const IntInputTemplate TESTMENU_LOAD = {
	4, 0, 0, 0, 9999,
	"Load Average",
	"vbc",
	IW_OFFSET(mp.load_test)
};

const MenuItem MENU_TEST[] = {
	{"GPS Emulation",		"GPS�G�~�����[�V����",				MENU_SEL_ENUM,	&TESTMENU_MODE},
	{"Add TestRoute",		"�e�X�g���[�g�̂���",				MENU_FCALL,		MP_TestRoute},
	{"Load Average",		"���[�h�A�x���[�W�`�F�b�N",			MENU_SEL_VAL,	&TESTMENU_LOAD},
	{"Cart Perf.",			"�J�[�g���b�W�����̂��e�X�g",		MENU_FCALL,		MP_CartTest},
	{"Sector Dump",			"�J�[�g���b�W�Z�N�^�_���v",			MENU_FCALL,		MP_CartTest2},
	{"Wpt Upload",			"�E�F�C�|�C���g�A�b�v���[�h",		MENU_FCALL,		MP_UploadWpt},
	{"Route Upload",		"���[�g�A�b�v���[�h",				MENU_FCALL,		MP_UploadRoute},
	{0, 0,														MENU_ID_INFO}
};


///////////////////////////////////////////////////////////////////////////////
// ���j���[���X�g
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
// �I�[�g���s�[�g & �L�[�ω�
///////////////////////////////////////////////////////////////////////////////
u16 GetPushKey(u16* pre){
	// �L�[�`�F�b�N1
	u16 key = IW->key_state; // key_state�͂����Ŏ�荞��
	u16 push = key & ~*pre;
	*pre = key;

	// �^�C�p�}�`�b�N
	if(push != 0){
		IW->mp.ar_count = AUTO_REPEATE_INTERVAL - AUTO_REPEATE_START;
		IW->mp.ar_vbc   = IW->vb_counter;
		IW->mp.ar_key   = push & AUTO_REPEATE_ENABLE; // �L���L�[�����o��
		IW->mp.ar_key  &= ~(IW->mp.ar_key - 1);       // ���������̏ꍇ�͍ŉ��ʂ̃L�[���g��
	} else if(key & IW->mp.ar_key){
		if((IW->mp.ar_count += UpdateVBC(&IW->mp.ar_vbc)) >= AUTO_REPEATE_INTERVAL){
			IW->mp.ar_count = 0;
			push = IW->mp.ar_key;
		}
	}
	return push;
}

///////////////////////////////////////////////////////////////////////////////
// ���j���[�^�X�N
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
	case MENU_FCALL: // �֐��Ăяo��
	case MENU_SEL_FLOG:// ���O�\����p
		IW->mp.proc = (MenuProc)mi[IW->mp.sel].data;
		(*IW->mp.proc)(0xffff);
		break;

	case MENU_SEL_VAL: // ���l����
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
	case MENU_SEL_TIME: // �����I��
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

	case MENU_SEL_NAME: // ���O����
		{
			const NameInputTemplate* t = (NameInputTemplate*)mi[IW->mp.sel].data;
			NameInput* ii = &IW->aip.i_name;
			ii->t = t;
			memcpy(ii->val, t->val, t->max);
			ii->val[t->max] = 0; // ii->val��SZ
			IW->mp.proc = MP_NameInput;
			(*IW->mp.proc)(0xffff);
		}
		break;
		
	case MENU_SEL_LAT: // ���O����
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
	
	case MENU_SEL_ENUM: // �񋓑I��
		{
			const EnumInputTemplate* t = (EnumInputTemplate*)mi[IW->mp.sel].data;
			if(++*t->val >= t->max) *t->val = 0;
			IW->mp.save_flag |= SAVEF_UPDATE_CFG; // �ݒ�ύX�t���O
			// �\���̍X�V
			PutsSpace2(VAL_POS, IW->mp.sel * 2 + 1, 14);

			LocateX(VAL_POS);
			Puts(t->names[*t->val]);
		}
		break;

	case MENU_SEL_RTWPT: // ���[�g�E�F�C�|�C���g����
		{
			// �V�X�e����1�����Ȃ�
			IW->mp.proc = MP_RtWptInput;
			(*IW->mp.proc)(0xffff);
		}
		break;

	case MENU_SEL_TASK: // �^�X�N����
		{
			// �V�X�e����1�����Ȃ�
			IW->mp.proc = MP_TaskInput;
			(*IW->mp.proc)(0xffff);
		}
		break;

	case MENU_SEL_START:// �X�^�[�g�^�C���ݒ�
		{
			IW->mp.menuid = MENU_ID_TASK_START;
			IW->mp.sel = 0;
			DispMenu(IW->mp.menuid);
		}
		break;

	case MENU_SEL_SPEED: // �O���C�_�X�s�[�h�\����p
		MenuFillBox(4, 5, 25, 10);
		DrawText(6, 7, "(���ǂ� ��������)");
		SetKeyWaitCB(0xffff);
		break;

	case MENU_SEL_PALETTE:// �p���b�g�ݒ��p
		if(++IW->tc.palette_conf > 4) IW->tc.palette_conf = 0;
		IW->mp.save_flag |= SAVEF_UPDATE_CFG; // �ݒ�ύX�t���O
		InitPalette(IW->tc.palette_conf);
		ChangeNaviBasePal(IW->tc.palette_conf, IW->mp.cur_sect);
		Locate(VAL_POS, IW->mp.sel * 2 + 1);
		PutsGammaLevel(IW->tc.palette_conf);
		break;
	}
}

void RollbackMenu(u32 pre){
	// �J�[�\����߂�
	IW->mp.sel = 0;

	if(pre == -1) return;
	MenuPage mi = MENU_LIST[IW->mp.menuid];
	while(mi[++IW->mp.sel].action != pre && (u32)mi[IW->mp.sel].data != pre){
		if(!mi[IW->mp.sel].menu){
			IW->mp.sel = 0;// ������Ȃ������c
			break;
		}
	}
	DispMenu(IW->mp.menuid);
}
s32 MT_Menu(){
	// ���j���[��MAP0�ɏo�͂���(MAP1�͔w�i�Ŏg��)
	SelectMap(MAP_BG0);

	// �w��F�u�����N
	vu16* p = (u16*)BGpal;
	u32 pal = IW->vb_counter & 0x3f;
	if(pal > 0x16) pal = 0x16;
	p[0x0f0] = pal << 5; // �J�[�\���I��

	pal = IW->vb_counter & 0x1f;
	p[0x1f0] = (pal << 5) | (pal << 10); //���@

	p[0x1f1] = ((IW->vb_counter & 0x40) && (IW->px.pvt_timeout))?
		RGB(0x08, 0, 0) : RGB(0x3f, 0x38, 0x38); // ���ʏ��"�~"

	// �w���v�X�N���[���\��
	DispHelp(0);

	// �����L�[�̕ω��`�F�b�N
	u16 push = GetPushKey(&gMenuPreKey);
	if(push) IW->mp.auto_off_cnt = IW->vb_counter;

	// �֐��R�[������
	if(IW->mp.proc){
		if((*IW->mp.proc)(push)){
			// �֐�CB���[�h�I��
			IW->mp.proc = 0;
			DispMenu(IW->mp.menuid);
		}
		return 0;
	}

	// ���j���[����
	if(!push){
		// ���[�g/�E�F�C�|�C���g��PC���痠�ő��M�����ꍇ�����邽�߁A�ɂȂƂ��ɃA�b�v�f�[�g���Ă���
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
		return 0; // �ΏۊO�̃L�[
	}

	// �ʒu�ύX����B�J�[�\���ƃw���v���X�V
	DispCursor();
	return 0;
}

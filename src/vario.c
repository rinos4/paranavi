///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// バリオモドキの処理をこのファイルにまとめた。


///////////////////////////////////////////////////////////////////////////////
// バリオモドキ
///////////////////////////////////////////////////////////////////////////////
// 音階モード
// +0.0  C C# D D# E F F# G G# A A# B
#define FREQ(x) (2048 - 131072/(x))
const u16 ONKAI_TABLE[] = {
	// C    C#     D    D#     E     F    F#     G    G#     A    A#     B
	2004, 1891, 1785, 1685, 1591, 1501, 1417, 1337, 1262, 1192, 1125, 1061
};
#define MAX_ONKAI ARRAY_SIZE(ONKAI_TABLE)

//code:0:C, 1:C#, … 11:B
u16 GetOnakiTone(u32 code, u32 oct){
	u16 v = ONKAI_TABLE[code] >> oct;
	if(v) return 2048 - v;
	return 2047; // MAX
}

// 画面をみなくても上昇値が分かる絶対音感用の音階バリオ
// 0.1m/s単位で1音階アップし、1m/sで1オクターブアップするので暗算し易い。
void SetOnkaiTone1(s32 up){
	if(up >= IW->tc.vario_up){
		up += 2000 - IW->tc.vario_up;
	} else if(up <= IW->tc.vario_down){
		up += 2000 - IW->tc.vario_down;
	} else {
		return;
	}
	if(up < 0){
		SetCh2Tone(0);
		return;
	}
	s32 mod;
	up  = BiosDiv(up, 1000, &mod);	// 1m/sで1オクターブアップ
	mod = BiosDiv(mod,  84, &mod);
	up = GetOnakiTone(mod, up);
	if(up > 2000) up = 2000;
	SetCh2Tone(up);
}

// 画面をみなくても上昇値が分かる絶対音感用の音階バリオ その２
// 分解能は"その１"より低いが、こちらの方が風切音のある空でも聞き取り易かった
const u8 ONKAI_C[] = { 0, 2, 4, 5, 7, 9, 11}; // ドレミファソラシド のみ
void SetOnkaiTone2(s32 up){
	if(up >= IW->tc.vario_up){
		up += 4000 - IW->tc.vario_up;
	} else if(up <= IW->tc.vario_down){
		up += 4000 - IW->tc.vario_down;
	} else {
		return;
	}
	if(up < 0){
		SetCh2Tone(0);
		return;
	}
	s32 mod;
	up  = BiosDiv(up, 2000, &mod);	// 2m/sで1オクターブアップ
	mod = BiosDiv(mod, 286, &mod);
	up = GetOnakiTone(ONKAI_C[mod], up);
	if(up > 2000) up = 2000;
	SetCh2Tone(up);
}


s32 MT_Vario(){
	// 上昇値を選択
	if(!IW->tc.vario_mode){
		EnableSound(SOUND_CH_VARIO, -1);
		return 0;
	}
	Vario* vm = &IW->vm;
	s32 up = vm->vario_test;
	if(up == VARIO_TEST_DISABLE){
		if(IW->px.fix < FIX_2D || (IW->px.gstate == GSTATE_STOP && IW->tc.vario_to)){
			EnableSound(SOUND_CH_VARIO, -1);
			return 0;
		}
		up = IW->px.up_mm; // 測定値を使う
	}

	// 上昇値が変化した時、またはモード変更時には周波数を再計算する
	if(up != vm->cur_value || IW->tc.vario_mode != vm->cur_mode){
		vm->cur_value = up;
		vm->cur_mode  = IW->tc.vario_mode;
		if(up == VARIO_TEST_DISABLE){
			vm->vario_blink = 0; // 止め続ける
			EnableSound(SOUND_CH_VARIO, -1);
		} else if(up >= IW->tc.vario_up){
			vm->vario_blink = MaxS32((3840 - up / 2) >> 7, 1);
			switch(IW->tc.vario_mode){
			case 1: // スムーズ
				SetCh2Tone(MinS32(1600 + (up >> 4), 2000));
				break;
			case 2: // 音階1
				SetOnkaiTone1(up);
				break;
			case 3: // 音階2
				SetOnkaiTone2(up);
				break;
			}
		} else if(up <= IW->tc.vario_down){
			vm->vario_blink = 0; // 鳴り続ける
			switch(IW->tc.vario_mode){
			case 1: // スムーズ
//				SetCh2Tone((u16)MaxS32(1500 + ((up - IW->tc.vario_down) >> 3), 0));
				SetCh2Tone((u16)MaxS32(1500 + (up >> 3), 0));
				break;
			case 2: // 音階1
				SetOnkaiTone1(up);
				break;
			case 3: // 音階2
				SetOnkaiTone2(up);
				break;
			}
			EnableSound(SOUND_CH_VARIO, 7);
		} else {
			vm->vario_blink = 0; // 止め続ける
			EnableSound(SOUND_CH_VARIO, -1);
		}
	}

	// 音のON/OFFを設定
	if(vm->vario_blink && (vm->vario_count += UpdateVBC(&vm->vario_vbc)) > vm->vario_blink){
		vm->vario_count = 0;
		EnableSound(SOUND_CH_VARIO, EnableSoundCheck(SOUND_CH_VARIO)? -1 : 7);
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// 割り込み処理を定義。このファイルの関数だけはIWRAMでARM実行する。サイズ制限あり。
// 割り込みネストはしない。16ms以内に割り込み処理を完了すること！

///////////////////////////////////////////////////////////////////////////////
// 割り込みルーチン。手抜きでCで書いてる…
// ！注意！　スタックは160byte
///////////////////////////////////////////////////////////////////////////////
void Isr(){
	INTR_OFF(); // 割り込みネストはしない

	// CPU Load測定用
	u16 start_time = REG16(REG_TM3D);

	// 要因チェック & クリア
	u16 intr = REG16(REG_IE) & REG16(REG_IF);
	REG16(REG_IF) = intr;

	// 風速測定用カウンタ(2秒に1回は割り込みコールされること!)
	u16 tmp = start_time;
	if(!(tmp & 0x8000) && (IW->anemo.tm & 0x8000)) IW->anemo.tm += 0x10000;
	*(u16*)&IW->anemo.tm = tmp; // 下位ビットのみアップデート

	// 風速パルスカウント(UARTモードでのRCNTリードは仕様外?だが動いたので使ってみる)
	tmp = IsEmulator()? ((IW->key_state & KEY_SELECT)? 0 : 1) : (REG16(REG_RCNT) & 1);
	if(IW->anemo.pre_level != tmp){
		IW->anemo.pre_level = tmp;
		if(!tmp){
			IW->anemo.pulse++;
			IW->anemo.dif_tm = IW->anemo.tm - IW->anemo.pre_tm;
			IW->anemo.pre_tm = IW->anemo.tm;
			IW->anemo.calc_flag = 0;

			// 補正精度を上げるため、割り込みで処理
			switch(IW->anemo.calib_flag){
			case 1:
				IW->anemo.calib_tm    = IW->anemo.tm;
				IW->anemo.calib_pulse = 0;
				IW->anemo.calib_flag  = 2;
				break;
			case 2:
				IW->anemo.calib_pulse++;
				//終了判定
				u32 dif = IW->anemo.tm - IW->anemo.calib_tm;
				if(dif > CALIB_END_TIME){
					IW->anemo.calib_tm   = dif;
					IW->anemo.calib_flag = 0;
				}
			}
		}
	}

	// SIOエラー or 受信
	if(!IW->mp.multiboot && !IsEmulator()){ // RAM実行?(エミュレータでのテスト用チェック)
		// エラーチェック(最初にやらないとフラグがクリアされる)
		if(REG16(REG_SIOCNT) & UART_ERROR){ // オーバーラン?通信エラー?
			IW->uart_error = 1;
			IW->intr_flag = 1;
		}

		// 受信
		while(!(REG16(REG_SIOCNT) & UART_RECV_EMPTY)){
			// 受信バッファに追加する(ISRから変更していいのはrx_tailのみ)
			u8 ch = (u8)REG16(REG_SIODATA8);
			u16 next = NextIndex(IW->rx_tail, sizeof(IW->rx_buf));
			if(next == IW->rx_head) IW->rx_drop++; // バッファ不足!
			else{
				IW->rx_buf[IW->rx_tail] = ch;
				IW->rx_tail = next;
			}
			IW->intr_flag = 1;
		}

		// SIO送信(残りデータの有無で処理を行なう)
		while(IW->tx_head != IW->tx_tail && !(REG16(REG_SIOCNT) & UART_SEND_FULL)){
			// 送信バッファから取り込む(ISRから変更していいのはtx_headのみ)
			REG16(REG_SIODATA8) = IW->tx_buf[IW->tx_head];
			IW->tx_head = NextIndex(IW->tx_head, sizeof(IW->tx_buf));
			IW->intr_flag = 1;
		}
		if(IW->tx_head != IW->tx_tail){
			// 3/4 Empty割り込み
			// 4 byte FIFO: 9.6K=3125us, 38k=781us, 57k=521us, 115k=260us
			u32 v = 0;
			switch(IW->tc.bt_mode & 3){
			case 0: v = 0x00c03333;	break; // 9.6k (n = -3125 * 16777216 / 1000 / 1000 + 65536 = 0x3333) 
			case 1: v = 0x00c0CCD0;	break; // 38k  (n =  -781 * 16777216 / 1000 / 1000 + 65536 = 0xCCD0)
			case 2: v = 0x00c0DDDB;	break; // 57k  (n =  -521 * 16777216 / 1000 / 1000 + 65536 = 0xDDDB)
			case 3: v = 0x00c0EEF5;	break; // 115k (n =  -260 * 16777216 / 1000 / 1000 + 65536 = 0xEEF5)
			}
			REG32(REG_TM2D) = v; // FIFO送信割り込み開始
		} else {
			REG16(REG_TM2CNT) = 0;
		}

	}

	// V_BLANK キーチェック
	if(intr & V_BLANK_INTR_FLAG){
		// キーチェックは V_BLANK のタイミングでしかしない
		u16 pre = IW->key_chatter;
		IW->key_chatter = REG16(REG_KEYINPUT) ^ 0x3ff;
		IW->key_state = IW->key_chatter & pre;
		IW->intr_flag = 1;
		IW->vb_counter++;

		// 軌跡スプライトチェック
		if(SPLITE->flag & 1){
			REG16(REG_IE) |= V_COUNT_INTR_FLAG; //必要な時だけ設定するのが良い
		}
	}

	// H_BLANKでLocusのスプライト切り替え。このモードに入ると寝る時間が減って消費電流が増える…
	if(intr & V_COUNT_INTR_FLAG){
//if(IW->key_state & KEY_SELECT) SPLITE->flag = 0;
		if(SPLITE->flag & 1){
			s32 idx = REG16(REG_VCOUNT) >> 3;
			if(idx >= SPLITE_VCOUNT) idx = 0;
			s32 idx2 = idx;

			vu16* p = (u16*)(OAM + ((SPLITE_BASE) << 3));
			SpliteHBuf* sh = SPLITE->hBuf + idx;
			s32 left = SPLITE_END;
			while(left){
				u32 size = sh->pre_count;
				if(size){
					if(size > left){
						DmaCopy(0, sh->buf, p, left << 3, 16);
						break;
					}
					DmaCopy(0, sh->buf, p, size << 3, 16);
					left -= size;
					p += size << 2;
				}
				// 次ラインへ移る
				if(++idx >= SPLITE_VCOUNT){
					if(idx2 == 0){ // 1画面分を全て表示しきれた
						SPLITE->flag = 2;// 一旦解除
						REG16(REG_IE) &= ~V_COUNT_INTR_FLAG;
					}
					break;
				}
				++sh;
			}

			if(SPLITE->flag & 1){
				// 次のVCOUNTをセット
				u16 f = REG16(REG_DISPSTAT) & 0xff;
				if(idx2 == idx) ++idx;
				if(idx < SPLITE_VCOUNT) f |= idx << 11;
				REG16(REG_DISPSTAT) = f;
			} else {
				// 表示完了。残りを消す
				while(left--){
					p[0] = 160;
					p += 4;
				}
			}
		} else {
			REG16(REG_IE) &= ~V_COUNT_INTR_FLAG; //必要なくなった
		}
	}

	// DMA 音声ナビ
	if((intr & DMA1_INTR_FLAG) && IW->voice_ctr && !--IW->voice_ctr){
		REG16(REG_TM0CNT)   = 0;
		REG16(REG_DM1CNT_H) = 0;
		REG16(REG_IE) &= ~DMA1_INTR_FLAG; // 排他制御が無いためクリアされない可能性もあるが、次回クリアされるのでOK
		IW->intr_flag = 1; // クリアしたときだけフラグをセットする
	}

	// CPU Load測定用
	start_time = REG16(REG_TM3D) - start_time; // 差分取得。必ず1秒以下なので16bitでオーバーフローは発生しない。
	IW->mp.load[0]	 += start_time;	// 0番のISR Loadに加算
	IW->mp.last_load += start_time;	// ISR中はタスクLoadを減算させるために開始をシフト

	INTR_ON();
}
void Isr_end(){
}

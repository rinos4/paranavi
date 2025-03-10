///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2005 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// ���荞�ݏ������`�B���̃t�@�C���̊֐�������IWRAM��ARM���s����B�T�C�Y��������B
// ���荞�݃l�X�g�͂��Ȃ��B16ms�ȓ��Ɋ��荞�ݏ������������邱�ƁI

///////////////////////////////////////////////////////////////////////////////
// ���荞�݃��[�`���B�蔲����C�ŏ����Ă�c
// �I���ӁI�@�X�^�b�N��160byte
///////////////////////////////////////////////////////////////////////////////
void Isr(){
	INTR_OFF(); // ���荞�݃l�X�g�͂��Ȃ�

	// CPU Load����p
	u16 start_time = REG16(REG_TM3D);

	// �v���`�F�b�N & �N���A
	u16 intr = REG16(REG_IE) & REG16(REG_IF);
	REG16(REG_IF) = intr;

	// ��������p�J�E���^(2�b��1��͊��荞�݃R�[������邱��!)
	u16 tmp = start_time;
	if(!(tmp & 0x8000) && (IW->anemo.tm & 0x8000)) IW->anemo.tm += 0x10000;
	*(u16*)&IW->anemo.tm = tmp; // ���ʃr�b�g�̂݃A�b�v�f�[�g

	// �����p���X�J�E���g(UART���[�h�ł�RCNT���[�h�͎d�l�O?�����������̂Ŏg���Ă݂�)
	tmp = IsEmulator()? ((IW->key_state & KEY_SELECT)? 0 : 1) : (REG16(REG_RCNT) & 1);
	if(IW->anemo.pre_level != tmp){
		IW->anemo.pre_level = tmp;
		if(!tmp){
			IW->anemo.pulse++;
			IW->anemo.dif_tm = IW->anemo.tm - IW->anemo.pre_tm;
			IW->anemo.pre_tm = IW->anemo.tm;
			IW->anemo.calc_flag = 0;

			// �␳���x���グ�邽�߁A���荞�݂ŏ���
			switch(IW->anemo.calib_flag){
			case 1:
				IW->anemo.calib_tm    = IW->anemo.tm;
				IW->anemo.calib_pulse = 0;
				IW->anemo.calib_flag  = 2;
				break;
			case 2:
				IW->anemo.calib_pulse++;
				//�I������
				u32 dif = IW->anemo.tm - IW->anemo.calib_tm;
				if(dif > CALIB_END_TIME){
					IW->anemo.calib_tm   = dif;
					IW->anemo.calib_flag = 0;
				}
			}
		}
	}

	// SIO�G���[ or ��M
	if(!IW->mp.multiboot && !IsEmulator()){ // RAM���s?(�G�~�����[�^�ł̃e�X�g�p�`�F�b�N)
		// �G���[�`�F�b�N(�ŏ��ɂ��Ȃ��ƃt���O���N���A�����)
		if(REG16(REG_SIOCNT) & UART_ERROR){ // �I�[�o�[����?�ʐM�G���[?
			IW->uart_error = 1;
			IW->intr_flag = 1;
		}

		// ��M
		while(!(REG16(REG_SIOCNT) & UART_RECV_EMPTY)){
			// ��M�o�b�t�@�ɒǉ�����(ISR����ύX���Ă����̂�rx_tail�̂�)
			u8 ch = (u8)REG16(REG_SIODATA8);
			u16 next = NextIndex(IW->rx_tail, sizeof(IW->rx_buf));
			if(next == IW->rx_head) IW->rx_drop++; // �o�b�t�@�s��!
			else{
				IW->rx_buf[IW->rx_tail] = ch;
				IW->rx_tail = next;
			}
			IW->intr_flag = 1;
		}

		// SIO���M(�c��f�[�^�̗L���ŏ������s�Ȃ�)
		while(IW->tx_head != IW->tx_tail && !(REG16(REG_SIOCNT) & UART_SEND_FULL)){
			// ���M�o�b�t�@�����荞��(ISR����ύX���Ă����̂�tx_head�̂�)
			REG16(REG_SIODATA8) = IW->tx_buf[IW->tx_head];
			IW->tx_head = NextIndex(IW->tx_head, sizeof(IW->tx_buf));
			IW->intr_flag = 1;
		}
		if(IW->tx_head != IW->tx_tail){
			// 3/4 Empty���荞��
			// 4 byte FIFO: 9.6K=3125us, 38k=781us, 57k=521us, 115k=260us
			u32 v = 0;
			switch(IW->tc.bt_mode & 3){
			case 0: v = 0x00c03333;	break; // 9.6k (n = -3125 * 16777216 / 1000 / 1000 + 65536 = 0x3333) 
			case 1: v = 0x00c0CCD0;	break; // 38k  (n =  -781 * 16777216 / 1000 / 1000 + 65536 = 0xCCD0)
			case 2: v = 0x00c0DDDB;	break; // 57k  (n =  -521 * 16777216 / 1000 / 1000 + 65536 = 0xDDDB)
			case 3: v = 0x00c0EEF5;	break; // 115k (n =  -260 * 16777216 / 1000 / 1000 + 65536 = 0xEEF5)
			}
			REG32(REG_TM2D) = v; // FIFO���M���荞�݊J�n
		} else {
			REG16(REG_TM2CNT) = 0;
		}

	}

	// V_BLANK �L�[�`�F�b�N
	if(intr & V_BLANK_INTR_FLAG){
		// �L�[�`�F�b�N�� V_BLANK �̃^�C�~���O�ł������Ȃ�
		u16 pre = IW->key_chatter;
		IW->key_chatter = REG16(REG_KEYINPUT) ^ 0x3ff;
		IW->key_state = IW->key_chatter & pre;
		IW->intr_flag = 1;
		IW->vb_counter++;

		// �O�ՃX�v���C�g�`�F�b�N
		if(SPLITE->flag & 1){
			REG16(REG_IE) |= V_COUNT_INTR_FLAG; //�K�v�Ȏ������ݒ肷��̂��ǂ�
		}
	}

	// H_BLANK��Locus�̃X�v���C�g�؂�ւ��B���̃��[�h�ɓ���ƐQ�鎞�Ԃ������ď���d����������c
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
				// �����C���ֈڂ�
				if(++idx >= SPLITE_VCOUNT){
					if(idx2 == 0){ // 1��ʕ���S�ĕ\�������ꂽ
						SPLITE->flag = 2;// ��U����
						REG16(REG_IE) &= ~V_COUNT_INTR_FLAG;
					}
					break;
				}
				++sh;
			}

			if(SPLITE->flag & 1){
				// ����VCOUNT���Z�b�g
				u16 f = REG16(REG_DISPSTAT) & 0xff;
				if(idx2 == idx) ++idx;
				if(idx < SPLITE_VCOUNT) f |= idx << 11;
				REG16(REG_DISPSTAT) = f;
			} else {
				// �\�������B�c�������
				while(left--){
					p[0] = 160;
					p += 4;
				}
			}
		} else {
			REG16(REG_IE) &= ~V_COUNT_INTR_FLAG; //�K�v�Ȃ��Ȃ���
		}
	}

	// DMA �����i�r
	if((intr & DMA1_INTR_FLAG) && IW->voice_ctr && !--IW->voice_ctr){
		REG16(REG_TM0CNT)   = 0;
		REG16(REG_DM1CNT_H) = 0;
		REG16(REG_IE) &= ~DMA1_INTR_FLAG; // �r�����䂪�������߃N���A����Ȃ��\�������邪�A����N���A�����̂�OK
		IW->intr_flag = 1; // �N���A�����Ƃ������t���O���Z�b�g����
	}

	// CPU Load����p
	start_time = REG16(REG_TM3D) - start_time; // �����擾�B�K��1�b�ȉ��Ȃ̂�16bit�ŃI�[�o�[�t���[�͔������Ȃ��B
	IW->mp.load[0]	 += start_time;	// 0�Ԃ�ISR Load�ɉ��Z
	IW->mp.last_load += start_time;	// ISR���̓^�X�NLoad�����Z�����邽�߂ɊJ�n���V�t�g

	INTR_ON();
}
void Isr_end(){
}

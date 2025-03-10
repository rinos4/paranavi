///////////////////////////////////////////////////////////////////////////////
// ParaNavi
// Copyright (C) 2006 Rinos, All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ParaNavi.h"

// �g���b�N���O�̈��k/�W�J���W�b�N�A�t�@�C���������ݐ�������̃t�@�C���Œ�`

///////////////////////////////////////////////////////////////////////////////
// �g���b�N���O
///////////////////////////////////////////////////////////////////////////////
//�y���k�g���b�N���O�t�H�[�}�b�g Version 0x01�z
//PID Size:  Detail= L(Lat) o(Lon) A(Alt) S(Sec) R(Repeat) P(Precision) B(BCC)
// 0  1byte:                            RRRRRR00 (Repeat:R+2 [R:0-58])// ���S����(�ő�1��Rep)
// 1  1byte:                            LLooAA01 (Lat:   2,A: 2,S: 0) // �����ړ����A���Ԋu
// 2  2byte:                   LLLLLLoo ooooAA10 (Lat:  32,A: 2,S: 0) // ����ړ����A���Ԋu
// 3  3byte:          LLLLLLLL Looooooo ooAAA011 (Lat: 256,A: 4,S: 0) // �s����ړ��A���Ԋu
// 4  4byte: LLLLLLLL LLLLoooo oooooooo AAA00111 (Lat:2048,A: 4,S: 0) // ��������A  ���Ԋu
// 5  4byte: LLLLLLLL LLLooooo ooooooAA AAA01111 (Lat:1024,A:16,S: 0) // ���T�[�}���A���Ԋu
// 6  4byte: LLLLLLLL Looooooo ooAAAASS SSS10111 (Lat: 256,A: 8,S:16) // ���O�Ԋu�ύX
// 7  5byte: L*13 o*13 A* 7 S*0          0011111 (Lat:4096,A:64,S: 0) // �A�N���A    ���Ԋu
// 8  5byte: L*11 o*11 A* 5 S*6          0111111 (Lat:1024,A:16,S:32) // �ϊԊu���O
// 9  6byte: L*15 o*15 A*11 S*0          1011111 (Lat: 16K,A:1K,S: 0) // �������A    ���Ԋu
//10  7byte: L*15 o*15 A*11 S*7         01111111 (Lat: 16K,A:1K,S:64) // UnidentifiedFlyingObj
//11 15byte: L*30 o*31 A*16 S*32    PPP 11101100 ��Βl�p�P�b�g<�p��> (P:-4�`3)   [R59]
//12 15byte: L*30 o*31 A*16 S*32    PPP 11110000 ��Βl�p�P�b�g<�V�K> (�Z�p���[�^)[R60]
//13  2byte:                   BBBBBBBB 11110100 �����p�P�b�g(BCC: 0-255)         [R61]
//14  2byte: Option*8                   11111000 1byte �I�v�V����(Total:2byte)    [R62]
//15  ?byte: Option*n*8 Type*8 nnnnnnnn 11111100 nbyte �I�v�V����(Total=n+3 byte) [R63] RFU
//16  1byte:                            11111111 ����������(FlashROM�������)
//    8byte:Magic[24] + Version[8] + Blk#[8] + Pkt#[24]) 'Trk?pppb'  // Flash�u���b�N�w�b�_

// �L�^���x(Lat/Lon): P:0 1ms(��3cm)�AP:1 8ms(24cm) P:2 32ms(96cm) P:3 128ms(4m) P:�� RFU
// 1byte�I�v�V����:   [RFU*4 ManualSeg*1 CommTimeout*1 LostFlag*1] (�Z�O�����g�����p)
// �ϐ������l:        ��K����: lat/lon/alt:0, sec:+1  (��Βl�p�P�b�g��M���Ƀ��Z�b�g)
//                    BCC(1byte): 0 (�w�b�_�܂��͌����p�P�b�g��M���Ƀ��Z�b�g)
// Flash�u���b�N:     BLOCK_HEAD [PID#14-15]* PID#11-12 [PID#0-15]* [PID#16] (�ő�64KB)
//                    Flash�u���b�N(64KB)�̐擪�́A�p�����ł��K���u���b�N�w�b�_��}������(�����p)
//                    �e�u���b�N�̍ŏ��̃g���b�N�f�[�^�͏㏑�����[�h�ɔ����Đ�Βl�p�P�b�g���g�p
// �����p�P�b�g:      Flash�u���b�N�����A�܂��̓T�X�y���h���Ɍ����p�P�b�g��}��(�I�v�V����)
//                    ���Ȃ��Ƃ�3600�p�P�b�g��1��͌����p�P�b�g��}�����ׂ����C���i�C?

// ���ӎ���:
// Repeat����Flash�ւ̏������݂͍s�Ȃ�Ȃ����߁A�����������ɓˑR�d��OFF����ƍő�1���ԃ��X�g����


typedef struct{
	u32 packet_size;
	u32 bit[4];		// [lat/lon/alt/sec]
	s32 range[4];	// [lat/lon/alt/sec]
	u8  flag;
	u8  mask;
	u8  mask_size;
	u32 pid;
} PacketType2;

#define BitFill(n)  ((1 <<  (n)) - 1) // n=32�ŃI�[�o�[�t���[
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

	// �f�R�[�h�p(�I�[����)
	DEF_CMP(30, 31, 16, 32,  0xf0, 8, 12),
	DEF_CMP( 0,  0,  0,  0,  0xf4, 8, 13),
	DEF_CMP( 0,  0,  0,  0,  0xf8, 8, 14),
	DEF_CMP( 0,  0,  0,  0,  0xfc, 8, 15),
	DEF_CMP( 0,  0,  0,  0,  0xff, 8, 16),
	DEF_CMP( 0,  0,  0,  0,  0x00, 2, 0),
	DEF_CMP( 0,  0,  0,  0,  0,    0, 16), // END(�����ɓ��B���邱�Ƃ͂Ȃ����c)
};

#define END_INDEX 11

const PacketType2 ABS_TABLE[] = {
	DEF_CMP(30, 31, 16, 32,  0xec, 8, 11), // �ʎw��p
};
const PacketType2 SEP_TABLE[] = {
	DEF_CMP(30, 31, 16, 32,  0xf0, 8, 12), // �ʎw��p
};

const s32 PRECISION_TABLE[8] = { // �ۑ����x
	0, 3, 5, 7, 0, 0, 0, 0 // 1ms/8ms/32ms/128ms, ���̓f�t�H���g��1ms(3cm)�Ƃ���
};

#define MAX_REPEAT 58
const u32 PACKET_SIZE[] = {1, 1, 2, 3, 4, 4, 4, 5, 5, 6, 7, 15, 15, 2, 2, 3, 1};

#define OPT_1byte 0xf8
#define OPT_Nbyte 0xfc


static s32 BldRepeat(TrackLogParam* tlp, u8* buf);


///////////////////////////////////////////////////////////////////////////////
// ���O�������ݏ���
///////////////////////////////////////////////////////////////////////////////
// 1�o�C�g�ÂV�t�g������
void ByteShift(u8* buf, u32 size){
	while(size--) buf[size + 1] = buf[size];// memmov�̑���
}

// ���̃u���b�N�֐i��(�`�F�b�N�t��)
static s32 GotoNextBlock(TrackLogParam* tlp){
	if(tlp->index){
		tlp->index = 0;
		tlp->block++;
	}
	if(tlp->block >= IW->cw.tlog_block) tlp->block = 0; // �����߂�
	tlp->abs_flag |= ABSF_CONTINUE; // ���̃f�[�^�p�P�b�g�͐�Βl�t�H�[�}�b�g���K�v
	if(!IW->tc.log_overwrite){
		if(*IW->cif->ReadDirect(CUR_BLOCK_OFFSET(tlp)) == FLBL_TRACK_MAGIC){
			tlp->full = 1;
			return FLASH_E_LOG_FULL;
		}
	}
	tlp->full = 0;
	return FLASH_SUCCESS;
}

// �g���b�N���O�̒ǋL�Bbuf��2byte�A���C�����g�B2byte�̗V�т����邱��!
u32 AppendTrackLog(TrackLogParam* tlp, u8* buf, u32 buf_size){
	if(!buf_size) return FLASH_SUCCESS; // ���������`�F�b�N�c

	// ��~���[�h����㏑�����[�h�ւ̕ύX�p�`�F�b�N
	s32 ret;
	if(tlp->index >= BLOCK_SIZE || tlp->full){ // �G���[���̃��J�o��
		ret = GotoNextBlock(tlp);
		if(ret) return ret; // FULL
	
		// ���O��~���Ƀ��X�g����repeat�͂�����߂�
		tlp->repeat = 0;
	}

	// ���s�[�g�`�F�b�N
	if(tlp->repeat){
		tlp->pkt_counter++; // �������݃J�E���^
		if(tlp->index + buf_size < BLOCK_SIZE){
			// ���݂̃o�b�t�@�ɕ֏揑�����݂��Ă������
			ByteShift(buf, buf_size++);
			BldRepeat(tlp, buf); // repeat�t���O�͂��̒��ŗ�����
		} else {
			// ���s�[�g�p�P�b�g�ŏI�[(tlp->index=0�͂��肦�Ȃ�)
			u8 rep[2]; // +1�̓A���C�����g�p�}�[�W��
			BldRepeat(tlp, rep); // repeat�t���O�͂��̒��ŗ�����
			ret = IW->cif->AppendTLog(rep, 1); // ���s�[�g�݂̂ŏ�������(1byte�͐�΋󂫂�����)
			if(ret) return ret;
		}
	}

	// �������݃u���b�N�v�Z
	if(tlp->index + buf_size > BLOCK_SIZE){
		ret = GotoNextBlock(tlp); // ���̃u���b�N�͋󂫂�����Ȃ��̂Ŏ��̃u���b�N�֐i��(TODO BCC�ǉ�)
		if(ret) return ret; // �t��
	}

	// �u���b�N�ŏ��̏�������?
	if(!tlp->index){
		// �u���b�N�̏���
		ret = IW->cif->EraseBlock(CUR_BLOCK_OFFSET(tlp), BLOCK_SIZE, ERASE_TRACK);
		if(ret) return ret;

		// �w�b�_��������
		TrackLogHeader tlh;
		tlh.magic = FLBL_TRACK_MAGIC;
		tlh.index = (tlp->pkt_counter & 0x00ffffff) | (++tlp->blk_counter << 24);
		ret = IW->cif->AppendTLog((u8*)&tlh, sizeof(tlh));
		if(ret) return ret; // �G���[���o

		tlp->abs_flag |= ABSF_CONTINUE; // ���̃f�[�^�p�P�b�g�͐�Βl�t�H�[�}�b�g���K�v
		return FLASH_E_LOG_NEWBLOCK; // �V�����u���b�N�ɐ؂�ւ�������Ƃ�ʒm(��Βl�p�P�b�g�̗v��)
	} 

	// ��������
	tlp->pkt_counter++; // �������݃J�E���^
	ret = IW->cif->AppendTLog(buf, buf_size);
	if(tlp->index >= BLOCK_SIZE) GotoNextBlock(tlp); // ��t�ɂȂ����玟��
	return ret;
}


///////////////////////////////////////////////////////////////////////////////
// ���k���W�b�N
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

// ���s�[�g�p�P�b�g�쐬
static s32 BldRepeat(TrackLogParam* tlp, u8* buf){
	switch(tlp->repeat){
	case 0:
		return 0; //!?
	case 1:
		*buf = 0x1;// PID#1 �[������x1
		tlp->t_count[1]++; // PID#1�ǉ�
		break;
	default: // ���s�[�g
		*buf = (tlp->repeat - 2) << 2;
		tlp->t_count[0]++; // PID#0�ǉ�
	}
	tlp->repeat = 0;
	return 1;
}

// ��Βl�p�P�b�g�쐬
static s32 BldAbsolute(TrackLogParam* tlp, u8* buf, const PacketType2* p, const TrackLog* tl){
	u8* start_buf = buf;
	memset(buf, 0, 15);

	u32 bit = 0, i;
	PackBit(&buf, &bit, p->mask_size + 3, p->flag | ((tlp->pre_prec & 7) << 8));
	for(i = 4 ; i-- ;) PackBit(&buf, &bit, p->bit[i], tl->val[i]);
	tlp->dif[0] = tlp->dif[1] = tlp->dif[2] = 0;
	tlp->dif[3] = 1;// sec����1���f�t�H���g
	tlp->t_count[p->pid]++; // PID�ǉ�
	tlp->abs_flag = 0; // �ȍ~�A�����p�P�b�gOK
	return buf - start_buf;
}

// ��K�����p�P�b�g�쐬
static s32 BldDiff(TrackLogParam* tlp, u8* buf, const PacketType2* p, const TrackLog* tl, s32* dif2){
	u8* start_buf = buf;
	// ��K�����p�P�b�g
	memset(buf, 0, p->packet_size);
	s32 bit = 0, i;
	PackBit(&buf, &bit, p->mask_size, p->flag);
	for(i = 4 ; i-- ;){
		PackBit(&buf, &bit, p->bit[i], dif2[i]);
		tlp->dif[i] = tl->val[i] - tlp->pre[i];
	}
	tlp->t_count[p->pid]++; // PID�ǉ�
	return buf - start_buf;
}

static u32 EncodeTrackLog(TrackLogParam* tlp, TrackLog* tl, u8* buf){
	// �����v�Z
	s32 dif2[4], i, same_flag = 1;
	for(i = 4 ; i-- ;){
		dif2[i] = tl->val[i] - tlp->pre[i] - tlp->dif[i];
		if(dif2[i]) same_flag = 0;
	}

	// ���s�[�g�`�F�b�N
	if(same_flag){
		if(tlp->repeat++ < MAX_REPEAT) return 0;
		return BldRepeat(tlp, buf); // ���s�[�g�p�P�b�g�m��A����
	}

	// ���k�p�P�b�g�r���h
	const PacketType2* p = PACKET_TABLE;
	for(; p->pid < END_INDEX ; ++p){
		for(i = 4 ; i-- ;){
			if(dif2[i] && (dif2[i] < -p->range[i] || p->range[i] <= dif2[i])) break;
		}
		if(i < 0) return BldDiff(tlp, buf, p, tl, dif2); // ����PID�ň��k��
	}

	// ��Βl�p�P�b�g
	return BldAbsolute(tlp, buf, p, tl);
}

s32 ErrorLog(TrackLogParam* tlp, s32 err){
	tlp->err = err; // �ꉞ�L�^
	tlp->drop++;
	tlp->abs_flag |= ABSF_CONTINUE; //�G���[��͐�Βl�p�P�b�g�ŕ���������
	tlp->repeat = 0; // ���s�[�g�����X�g
	return err;
}

//���k���O��������
u32 TrackLogAdd(TrackLog* tl){
	TrackLogParam* tlp = &IW->mp.tlog;
	if(!IW->cw.tlog_block) return 1; // �̈斳��

	tlp->trk_counter++; // �f�o�b�O�p�J�E���^
	IW->mp.tlog.tc_state = 0; // �Čv�Z�v��
	IW->mp.tlog.tc_lastadd = IW->vb_counter;

	// �������ݐ��x�̕ύX���o
	if(tlp->pre_prec != IW->tc.log_prec){
		tlp->pre_prec = IW->tc.log_prec;
		tlp->abs_flag |= ABSF_CONTINUE; // ��Βl�p�P�b�g�Ő��x�ύX��ʒm
	}
	s32 t= PRECISION_TABLE[tlp->pre_prec & 7];
	if(t){
		tl->val[0] = RoundShift(tl->val[0], t); // lat
		tl->val[1] = RoundShift(tl->val[1], t); // lon
	}

	u32 buf32[80 / 3]; // PVT=64byte �A���C�����g�p�}�[�W���t��
	u8* buf = (u8*)buf32;
	u32 buf_size = 0;

	// �I�v�V������������
	if(tlp->opt & TLOG_OPT_ENABLE){
		buf[0] = OPT_1byte;
		buf[1] = tlp->opt & TLOG_OPT_MASK;
		tlp->t_count[14]++; // PID#14(1byte�I�v�V����)�ǉ�
		t = AppendTrackLog(tlp, buf, 2); // 2byte�I�v�V����
		if(t == FLASH_E_LOG_NEWBLOCK) t = AppendTrackLog(tlp, buf, 2); // �I�v�V�����͂��̂܂܎��u���b�N�֏�������
		if(t) return ErrorLog(tlp, t);
		tlp->opt = 0; // �I�v�V�����N���A
	}

	if(IW->tc.log_debug == 1){ // PVT�f�o�b�O
#define PVTRAW_SIZE (sizeof(D800_Pvt_Data_Type) + 3)
		buf[0] = OPT_Nbyte;
		buf[1] = sizeof(D800_Pvt_Data_Type);
		buf[2] = 0;
		memcpy(buf + 3, &IW->pvt_raw, sizeof(D800_Pvt_Data_Type));
		tlp->t_count[15] += PVTRAW_SIZE;
		t = AppendTrackLog(tlp, buf, PVTRAW_SIZE);
		if(t == FLASH_E_LOG_NEWBLOCK) t = AppendTrackLog(tlp, buf, PVTRAW_SIZE); // �I�v�V�����͂��̂܂܎��u���b�N�֏�������
		if(t) return ErrorLog(tlp, t);

		static s32 sPreTime = 0;
		if(sPreTime && tl->val[3] - sPreTime < 1){
			tlp->t_count[13] ++; // �ُ�^�C�����o
		}
		sPreTime = tl->val[3];
	}


	// �g���b�N�f�[�^��������
	if(tlp->abs_flag) buf_size = BldAbsolute(tlp, buf, (tlp->abs_flag & ABSF_SEPARETE)? SEP_TABLE : ABS_TABLE, tl);
	else 			  buf_size = EncodeTrackLog(tlp, tl, buf);

	if(buf_size){
		t = AppendTrackLog(tlp, buf, buf_size);
		if(t == FLASH_E_LOG_NEWBLOCK){
			// ��Βl�p�P�b�g�ɕϊ�
			if(buf_size != 15) buf_size = BldAbsolute(tlp, buf, ABS_TABLE, tl);
			else               tlp->abs_flag = 0; // ���ɐ�Βl�p�P�b�g���g�p
			t = AppendTrackLog(tlp, buf, buf_size);
		}
		if(t) return ErrorLog(tlp, t);
	}

	// ���ݒl���X�V���Ă���
	s32 i;
	for(i = 4 ; i-- ;) tlp->pre[i] = tl->val[i];
	return 0;
}

// ���݂�PVT���g���b�N�ɒǉ�
u32 TrackLogAddCurrentPVT(){
	// �g���b�N���O�̃_�E�����[�h���́A���݂�PVT��������Ȃ��悤�ɃK�[�h����
	if(IW->tc.log_enable && IW->gi.state != GPS_TRACK && IW->gi.state != GPS_TRACK_WAIT){
		if(IW->px.fix > FIX_INVALID){
			if(++IW->mp.tlog.intvl_ctr >= IW->tc.log_intvl){
				IW->mp.tlog.intvl_ctr = 0;
				TrackLog tl;
				tl.val[0] = IW->px.lat;
				tl.val[1] = IW->px.lon;
				tl.val[2] = RoundDiv(IW->px.alt_mm, 1000); // ���x��1m���x�ɗ��Ƃ�
				tl.val[3] = IW->px.dtime;
				return TrackLogAdd(&tl);
			}
		} else {
			IW->mp.tlog.opt |= TLOG_OPT_LOST; // INVALID��M
		}
	}
	return 0; // �ǉ�������̂Ȃ�
}

// �S�Ẵg���b�N���O����������(�}�W�b�N�̂ݏ����̍�����)
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

// �������ݍς݂̃g���b�N���O�̏I�[�I�t�Z�b�g��T���B(tlp->block�Ɍ����u���b�N���Z�b�g)
// �߂�l�̓g���b�N���O��(���OPC�]���p�B�p�P�b�g���ł͂Ȃ��̂Œ��ӁI)
u32 SearchTrackEnd(TrackLogParam* tlp){
	tlp->index = 0;
	if(IW->cw.tlog_block <= tlp->block) return 0;//�̈�O

	u32 start = CUR_BLOCK_OFFSET(tlp);
	u32 pre_sect = -1;
	u8* p;
	if(*IW->cif->ReadDirect(start) != FLBL_TRACK_MAGIC) return 0;// ����������Ă��Ȃ�

	u32 i = BLOCK_HEAD_SIZE;
	u32 ret = 0;
	while(i < BLOCK_SIZE){
		// PID����(�Ƃ肠�������[�v�ŁB256type�����e�[�u�����ǂ�����)
		const PacketType2* pt = PACKET_TABLE;
		u8 code = GetLogByte(i + start, &pre_sect, &p);
		while((pt->mask & code) != pt->flag) ++pt;
		if(pt->pid == 16) break;
		if(pt->pid == 15){
			i += GetLogByte(i + start + 1, &pre_sect, &p); // �σT�C�Y�I�v�V����
		} else if(!pt->pid){
			ret += (GetLogByte(i + start + 1, &pre_sect, &p) >> 2) + 2;
		} else if(pt->pid == 12){
			ret += 2;
		} else if(pt->pid < 13){
			ret++;
		}
		i += PACKET_SIZE[pt->pid];
		tlp->pkt_counter++; // �����ŃJ�E���g�A�b�v���Ă���
	}
	tlp->index = i;
	return ret;
}

// ���O�������ݐ��������
void InitLogParams(){
	TrackLogParam* tlp = &IW->mp.tlog;
	DmaClear(3, 0, tlp, sizeof(TrackLogParam), 32);
	tlp->abs_flag = ABSF_SEPARETE; //�Z�p���[�^

	if(!IW->cw.tlog_block) return;

	// �X�^�[�g�u���b�N�̌���
	const u32* tb = 0;
	u32 end = IW->cw.tlog_block;
	for(tlp->block = 0 ; tlp->block < end ; tlp->block++){
		tb = IW->cif->ReadDirect(CUR_BLOCK_OFFSET(tlp));
		if(!tb) return; // �G���[!?
		if(tb[0] == FLBL_TRACK_MAGIC) break;
	}
	if(tlp->block == end){
		tlp->block = 0;
		return; // ���O�Ȃ�
	}

	s32 pre_index = tb[1];
	for(end-- ; tlp->block < end ; tlp->block++){
		tb = IW->cif->ReadDirect(CUR_BLOCK_OFFSET(tlp) + BLOCK_SIZE);
		if(!tb) return; // �G���[!?
		if(tb[0] != FLBL_TRACK_MAGIC || pre_index - (s32)tb[1] > 0) break;
		pre_index = tb[1];
	}
	tlp->blk_counter = ((u32)pre_index) >> 24;
	tlp->pkt_counter = ((u32)pre_index) & 0xffffff;

	// �I�[��T��
	SearchTrackEnd(tlp);
	if(tlp->index >= BLOCK_SIZE) GotoNextBlock(tlp);//�����ł̓G���[���O�s�v
}


///////////////////////////////////////////////////////////////////////////////
// NMEA�Z���e���X���f�o�b�O�p�ɕۑ�
///////////////////////////////////////////////////////////////////////////////
u32 AppendNMEALog(){
	if(IW->cw.tlog_block) return 1; //�̈�Ȃ�
	
	// �σT�C�Y�I�v�V�����w�b�_�쐬
	TrackLogParam* tlp = &IW->mp.tlog;
	u32 len = IW->dl_size;
#define MAX_NMEA_LOG (256 - 4)
	if(len > MAX_NMEA_LOG) len = MAX_NMEA_LOG;
	u8* head = IW->dl_loghead + 1;
	head[0] = OPT_Nbyte;
	head[1] = len;
	head[2] = 0;

	// ��������
	len += 3;
	tlp->t_count[15] += len;
	s32 t = AppendTrackLog(tlp, head, len);
	if(t == FLASH_E_LOG_NEWBLOCK) t = AppendTrackLog(tlp, head, len);
	return t;
}


///////////////////////////////////////////////////////////////////////////////
// ���OPC�]���p
///////////////////////////////////////////////////////////////////////////////
// �r�b�g���o��
s32 UnpackBit(u8** buf, s32* cur_bit, s32 bit_size){
	if(!bit_size) return 0; // ���o���s�v
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
	if(ret & BitRange(bit_size)) ret |= ~BitFill(bit_size); // �����g��
	return ret;
}

// �擪�u���b�N���T�[�`
s32 FindFirstBlock(){
	TrackLogParam* tlp = &IW->mp.tlog;
	u32 block = tlp->block;
	if(IW->cw.tlog_block <= block) return 0; // �Ƃ肠���������l�̏ꍇ�͐擪��Ԃ��Ă���

	while(*IW->cif->ReadDirect(FI_TRACK_OFFSET + block * BLOCK_SIZE) == FLBL_TRACK_MAGIC){
		if(!block--) block = IW->cw.tlog_block - 1;
		if(block == tlp->block) break; // �Ō�̃u���b�N�ɖ߂���
	}
	if(++block < IW->cw.tlog_block) return block; // �����g���b�N��1�悪�ŏ��̗L���u���b�N
	return 0; // �擪���ŏ��̃u���b�N
}

// ���̃g���b�N�ɐi�߂�
s32 NextTrack(s32 countmode){
	TrackInfo* ti = &IW->gi.tinfo;
	ti->seg = 0; // �Z�O�����g�}�[�N��������
	s32 i, j;
	char dbuf[20];
	int timeout = 0;
	for(;;){
		if(++timeout > 0x10000) break; // �O�̂��߁c
		// ���s�[�g�p�P�b�g�p��
		if(ti->rep){
			ti->rep--;
			for(i = 4 ; i-- ;) ti->val[i] += ti->dif[i];
			return 1;
		}

		// �u���b�N�؂�ւ�����
		if((ti->tpos & (BLOCK_SIZE - 1)) < BLOCK_HEAD_SIZE){
			if((ti->tpos >> 16) >= IW->cw.tlog_block) ti->tpos = 0; // �����߂�
			else ti->tpos &= ~(BLOCK_SIZE - 1); // �O�̂��ߐ擪�ɍăZ�b�g

			// ���O�Ȃ��u���b�N�Ȃ炱���Œ�~����
			if(*IW->cif->ReadDirect(FI_TRACK_OFFSET + ti->tpos) != FLBL_TRACK_MAGIC) return 0;

			ti->tpos += BLOCK_HEAD_SIZE; // �J�n�ʒu����
		}

		// PID����(�Ƃ肠�������[�v�ŁB256type�����e�[�u�����ǂ�����)
		*dbuf = GetLogByte(FI_TRACK_OFFSET + ti->tpos, &ti->pre_sect, &ti->sect_ptr);
		const PacketType2* pt = PACKET_TABLE;
		while((pt->mask & *dbuf) != pt->flag) ++pt;

		// �I�[�p�P�b�g�`�F�b�N
		if(pt->pid == 16){
			if((ti->tpos >> 16) == IW->mp.tlog.block) break; // �ŏI�u���b�N

			//���u���b�N��
			ti->tpos = (ti->tpos + BLOCK_SIZE) & ~(BLOCK_SIZE - 1);
			continue;
		}

		// �p�P�b�g�T�C�Y���̃f�[�^��荞��
		u32 size = PACKET_SIZE[pt->pid];
		for(i = 1 ; i < size && i < sizeof(dbuf) ; ++i){
			dbuf[i] = GetLogByte(FI_TRACK_OFFSET + ti->tpos + i, &ti->pre_sect, &ti->sect_ptr);
		}
		ti->tpos += size;

		// PID�ʏ���
		u8* buf = dbuf;
		switch(pt->pid){
		case 0: // ���s�[�g�p�P�b�g
			i = (*buf++ >> 2) + 2;
			if(countmode) return i; // �J�E���g���[�h���͌��𒼐ڕԂ�
			ti->rep = i;
			continue;

		case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10: // ��K�����p�P�b�g
			if(countmode) return 1; // �J�E���g���[�h��
			j = pt->mask_size;
			for(i = 4 ; i-- ;) ti->val[i] += ti->dif[i] += UnpackBit(&buf, &j, pt->bit[i]);
			return 1;

		case 12: // ��Βl�p�P�b�g<�V�K>
			ti->seg = -1;
			// no break�@(�Z�p���[�g�����ȊO��<�p��>�Ɠ���)
		case 11: // ��Βl�p�P�b�g<�p��>
			if(countmode) return 1; // �J�E���g���[�h��
			j = 0;
			++buf;
			ti->prc = PRECISION_TABLE[UnpackBit(&buf, &j, 3) & 7]; // �ۑ��𑜓x�擾
			for(i = 4 ; i-- ;) ti->val[i] = UnpackBit(&buf, &j, pt->bit[i]);
			ti->dif[0] = ti->dif[1] = ti->dif[2] = 0;	// �����l��������
			ti->dif[3] = 1;						// sec����1�������l
			return 1;

		case 13: // BCC(�Ƃ肠����BCC�G���[�͖������āA�ł��邾���f�R�[�h���Ă݂�)
			continue;

		case 14: // 1byte �R�}���h
			ti->seg |= buf[1]; // ���p�P�b�g�ŃZ�O�����g����(�I�v�V�����I��)
			continue;

		case 15: // n byte �R�}���h
			ti->tpos += (u32)buf[1];
			continue;
		}
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// �g���b�N���O�ۑ��^�X�N
///////////////////////////////////////////////////////////////////////////////

// ���Ԃ̂�����Flash�������ݏ����͍Ō�Ɏ��s
s32 MT_TrackLog(){
	if(IW->mp.tlog.pre_pvtc != IW->px.counter){
		IW->mp.tlog.pre_pvtc = IW->px.counter;
		TrackLogAddCurrentPVT(); // �g���b�N���O�ۑ�
		if(IW->tc.log_enable) IW->mp.tlog.tc_lastadd = IW->vb_counter; // ���A���^�C�����O���̓��O�Ԋu�Ɋ֌W�Ȃ��J�E���g�`�F�b�N���K�[�h����
	} else if(IW->vb_counter - IW->mp.tlog.tc_lastadd > VBC_TIMEOUT(3)){ // 3�b��PVT�ʐM���Ȃ���Όv�Z���J�n
		// �󂢂Ă��鎞�Ԃɑ��g���b�N�����J�E���g���Ă���(����ڑ����̓J�E���g���Ȃ�)
		TrackLogParam* tlp = &IW->mp.tlog;
		TrackInfo* ti = &IW->gi.tinfo;
		s32 end = IW->vb_counter + 1; // �^�C���A�E�g�ݒ�
		switch(tlp->tc_state){
		case 0:
			memset(ti, 0, sizeof(TrackInfo));
			ti->pre_sect = -1; // �����Z�N�^���w���Ă���
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
					// ����
					tlp->tc_state = 2;
					break;
				}
				if(ti->seg == -1){
					ti->seg = 0;
					++count;
				}
				tlp->tc_total += count;
//#define MAX_TX_LOG 0xffff // �ő�]���g���b�N���B�v���g�R���ケ���葽���̃g���b�N��]���ł��Ȃ��̂ŁA�����Ōv�����~���Ă����B
#define MAX_TX_LOG 0x7fff // �J�V�~�[����32768�ȏ��n���ƃ������s���G���[���\�������c�B�Ƃ肠�����G���[�̏o�Ȃ��ő�l��ݒ肵�Ă����B
				if(tlp->tc_total > MAX_TX_LOG){
					tlp->tc_total = MAX_TX_LOG;
					// ����
					tlp->tc_state = 2;
					break;
				}
				// ���Ԑ؂ꒆ�f
				if(end - *(vs32*)&IW->vb_counter < 0){
					IW->intr_flag = 1;
					break;
				}
			}
		}
	}
	return 0;
}

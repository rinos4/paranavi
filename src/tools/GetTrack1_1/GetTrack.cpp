#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////
// 定数
///////////////////////////////////////////////////////////////////////////////
const char REV_INFO[] = "GetTrack 1.0";

// 要求パケット
const BYTE PID_POLL = '0';
const BYTE PID_BAUD = '2';
const BYTE PID_DATA = '4';
const BYTE PID_FIN  = '8';
const BYTE PID_MASK = 0xfe;

// 応答パケット(2種類だけ)
const BYTE PID_ACK = 'A';
const BYTE PID_NAK = ~PID_ACK;

//#define DBG_CHAR(a) putchar(a)
#define DBG_CHAR(a)


BYTE gBuf[64 * 1024]; // MAX 64KB (面倒なのでグローバルに…)

///////////////////////////////////////////////////////////////////////////////
// COM制御
///////////////////////////////////////////////////////////////////////////////
DWORD BAUDRATE_LIST[4] = {
	CBR_9600, CBR_38400, CBR_57600, CBR_115200
};

DWORD SetPort(HANDLE hPort, int baudrate){
	DCB dcb;
	dcb.DCBlength = sizeof(dcb);
	GetCommState(hPort, &dcb);
	dcb.BaudRate    = BAUDRATE_LIST[baudrate & 3];
	dcb.fParity     = 0;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.ByteSize    = 8;
	dcb.StopBits    = ONESTOPBIT;
	if(!SetCommState(hPort, &dcb)) return 1;

	COMMTIMEOUTS ct;
	ct.ReadIntervalTimeout         = 100;
	ct.ReadTotalTimeoutMultiplier  = 1000;
	ct.ReadTotalTimeoutConstant    = 5000;
	ct.WriteTotalTimeoutMultiplier = 100;
	ct.WriteTotalTimeoutConstant   = 1000;
	if(!SetCommTimeouts(hPort, &ct)) return 2;

	return 0; // success
}

DWORD RxData(HANDLE h, BYTE* buf, DWORD len){
	DWORD n = 0;
	while(n < len){
		DWORD nR;
		if(!ReadFile(h, buf + n, len - n, &nR, 0) || !nR) break;
		n += nR;
	}
	return n;
}

DWORD TxByte(HANDLE h, BYTE ch){
	DWORD nW;
	if(!WriteFile(h, &ch, 1, &nW, 0) || !nW) return 0;
	return 1;
}

BYTE CheckSum(BYTE* p, int len){
	BYTE sum = 0;
	while(len--) sum += *p++;
	return sum;
}

///////////////////////////////////////////////////////////////////////////////
// トラックログ吸い上げ
///////////////////////////////////////////////////////////////////////////////
// HEAD: PID*7 TOGGLE*1 LEN*16 PSUM*8
DWORD GetPacketHead(HANDLE hPort, BYTE pre_pid, DWORD& len){
	DWORD need = 4, head;
	BYTE* buf = (BYTE*)&head;
	while(RxData(hPort, buf + (4 - need), need) == need){
		if(CheckSum(buf, 4) == 0){ // ヘッダチェック
			len = ((DWORD)buf[1] << 8) | buf[2];// big endian
			// 再送チェック
			if(buf[0] == pre_pid){
				if(RxData(hPort, gBuf, len) != len) break;
				// 再送時はデータを読み捨て
				DBG_CHAR('m'); // 再送
				TxByte(hPort, PID_ACK); // ACKを返す
				need = 4;// 次のパケットを受信
				continue;
			}
			// PIDチェック
			switch(buf[0] & ~1){
			case PID_POLL:
			case PID_BAUD:
			case PID_DATA:
			case PID_FIN:
				return head; // 有効PIDを検出
			}
		}

		// 無効パケット
		head >>= 8; // 1つシフトして再同期
		need = 1;
		DBG_CHAR('H'); // ヘッダエラー
		TxByte(hPort, PID_NAK);
	}
	DBG_CHAR('T');
	return -1; // タイムアウト
}

int GetTrack(HANDLE hPort, HANDLE hFile, DWORD baurdate){
	DWORD pre_pid = -1, len = 0;
	DWORD read_len = 0;
	for(;;){
		DWORD pid = GetPacketHead(hPort, (BYTE)pre_pid, len);
		if(pid == -1){
			// return 1;// タイムアウト
			continue; // やっぱりタイムアウトはなし。永遠に待機して、終わるときはCtrl-Cで…。
		}
		if(RxData(hPort, gBuf, len) != len){
			DBG_CHAR('L');
			TxByte(hPort, PID_NAK);
			continue;
		}

		// パケット処理
		switch(pid & PID_MASK){
		case PID_POLL: // ポーリングパケット
			DBG_CHAR('p');
			printf("GBA was connected.\n");
			pre_pid = pid;
			TxByte(hPort, PID_ACK);
			continue;

		case PID_BAUD: // ボーレート要求
			// ボーレート変更シーケンスはかなり手抜き。応答でのドロップはリトライなし…。
			DBG_CHAR('b');
			pre_pid = pid;
			baurdate &= 3;
			TxByte(hPort, (BYTE)baurdate);
			if(baurdate){
				Sleep(100); // 9600bpsなら1msで送信される(WaitCommEvent?)
				SetPort(hPort, baurdate);
				printf("Baudrate %d\n", BAUDRATE_LIST[baurdate]);
				Sleep(100);//ParaNaviは100ms以内にBaudrateを切り替える
			}
			TxByte(hPort, PID_ACK); // 変更ボーレートで応答
			continue;

		case PID_FIN: // 完了パケット
			DBG_CHAR('f');
			printf("TrackLog transfer was completed.\n");
			TxByte(hPort, PID_ACK);
			return 0; // 完了

		case PID_DATA: // データパケット
			if(len > 0){
				// データサムチェック(CRCにすべき?)
				if(CheckSum(gBuf, len) == 0){
					DWORD nW;
					if(!WriteFile(hFile, gBuf, --len, &nW, 0) || nW != len){
						fprintf(stderr, "\nWriteFile failed %d\n", GetLastError());
						return 1; // ファイル書き込みできないときには即エラー
					}
					DBG_CHAR('r');
					read_len += len;
					printf("Read %d bytes\r", read_len);
					pre_pid = pid;
					TxByte(hPort, PID_ACK);
					continue;
				}
			}
			break;
		}
		DBG_CHAR('D'); // データ異常
		TxByte(hPort, PID_NAK);
	}
	return 0; // success
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]){
	printf("%s\n", REV_INFO);

	// 引数処理
	DWORD baudrate = 0; //デフォルト9600
	switch(argc){
	case 3:
		break;

	case 4:
		switch(atoi(argv[3])){
		case 9600:   baudrate = 0; break;
		case 38400:  baudrate = 1; break;
		case 57600:  baudrate = 2; break;
		case 115200: baudrate = 3; break;
		default:
			fprintf(stderr, "Invalid baurdate\n");
			return 1;
		}
		break;

	default:
		fprintf(stderr, "usage: %s port_name file_name [baudrate]\n", *argv);
		fprintf(stderr, "       baudrate: [9600|38400|57600|115200]\n");
		return 2;
	}

	// 入力ポートのチェック
	HANDLE hPort = CreateFile(argv[1], GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if(hPort == INVALID_HANDLE_VALUE){
		fprintf(stderr, "CreateFile failed %d '%s'\n", GetLastError(), argv[1]);
		return 1;
	}
	if(SetPort(hPort, 0)){ // デフォルトは9600で接続
		fprintf(stderr, "Can not set COM port\n");
		CloseHandle(hPort);
		return 2;
	}

	// 出力ファイルのチェック
	HANDLE hFile = CreateFile(argv[2], GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(hFile == INVALID_HANDLE_VALUE){
		fprintf(stderr, "CreateFile failed %d '%s'\n", GetLastError(), argv[2]);
		CloseHandle(hPort);
		return 3; // 出力ファイルオープンエラー
	}

	// データ取得
	GetTrack(hPort, hFile, baudrate);
	CloseHandle(hFile);
	CloseHandle(hPort);
	return 0;
}

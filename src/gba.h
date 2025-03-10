#ifndef GBA_HEADER
#define GBA_HEADER

typedef unsigned char     u8;
typedef unsigned short    u16;
typedef unsigned long     u32;
typedef unsigned long int u64;

typedef volatile unsigned char     vu8;
typedef volatile unsigned short    vu16;
typedef volatile unsigned long     vu32;
typedef volatile   signed long     vs32;
typedef volatile unsigned long int vu64;

typedef signed char     s8;
typedef signed short    s16;
typedef signed long     s32;
typedef signed long int s64;

#define REG8(a)  (*(vu8 *)(a))
#define REG16(a) (*(vu16*)(a))
#define REG32(a) (*(vu32*)(a))

#define  OAMmem		(u32*)0x7000000
#define  VideoBuffer	(u16*)0x6000000

#define  OAMdata	(u16*)0x6010000
#define  BGpal		(u16*)0x5000000

#define  OBJpal		(u16*)0x5000200

#define REG_INTERUPT   0x3007FFC
#define REG_DISPCNT    0x4000000
#define REG_DISPCNT_L  0x4000000
#define REG_DISPCNT_H  0x4000002
#define REG_DISPSTAT   0x4000004
#define REG_STAT       0x4000004
#define REG_VCOUNT     0x4000006
#define REG_BG0CNT     0x4000008
#define REG_BG1CNT     0x400000A
#define REG_BG2CNT     0x400000C
#define REG_BG3CNT     0x400000E
#define REG_BG0HOFS    0x4000010
#define REG_BG0VOFS    0x4000012
#define REG_BG1HOFS    0x4000014
#define REG_BG1VOFS    0x4000016
#define REG_BG2HOFS    0x4000018
#define REG_BG2VOFS    0x400001A
#define REG_BG3HOFS    0x400001C
#define REG_BG3VOFS    0x400001E
#define REG_BG2PA      0x4000020
#define REG_BG2PB      0x4000022
#define REG_BG2PC      0x4000024
#define REG_BG2PD      0x4000026
#define REG_BG2X       0x4000028
#define REG_BG2X_L     0x4000028
#define REG_BG2X_H     0x400002A
#define REG_BG2Y       0x400002C
#define REG_BG2Y_L     0x400002C
#define REG_BG2Y_H     0x400002E
#define REG_BG3PA      0x4000030
#define REG_BG3PB      0x4000032
#define REG_BG3PC      0x4000034
#define REG_BG3PD      0x4000036
#define REG_BG3X       0x4000038
#define REG_BG3X_L     0x4000038
#define REG_BG3X_H     0x400003A
#define REG_BG3Y       0x400003C
#define REG_BG3Y_L     0x400003C
#define REG_BG3Y_H     0x400003E
#define REG_WIN0H      0x4000040
#define REG_WIN1H      0x4000042
#define REG_WIN0V      0x4000044
#define REG_WIN1V      0x4000046
#define REG_WININ      0x4000048
#define REG_WINOUT     0x400004A
#define REG_MOSAIC     0x400004C
#define REG_MOSAIC_L   0x400004C
#define REG_MOSAIC_H   0x400004E
#define REG_BLDMOD     0x4000050
#define REG_COLEV      0x4000052
#define REG_COLEY      0x4000054
#define REG_SG10       0x4000060
#define REG_SG10_L     0x4000060
#define REG_SG10_H     0x4000062
#define REG_SG11       0x4000064
#define REG_SG20       0x4000068
#define REG_SG21       0x400006C
#define REG_SG30       0x4000070
#define REG_SG30_L     0x4000070
#define REG_SG30_H     0x4000072
#define REG_SG31       0x4000074
#define REG_SG40       0x4000078
#define REG_SG41       0x400007C
#define REG_SGCNT0     0x4000080
#define REG_SGCNT0_L   0x4000080
#define REG_SGCNT0_H   0x4000082
#define REG_SGCNT1     0x4000084
#define REG_SGBIAS     0x4000088
#define REG_SGWR0      0x4000090
#define REG_SGWR0_L    0x4000090
#define REG_SGWR0_H    0x4000092
#define REG_SGWR1      0x4000094
#define REG_SGWR1_L    0x4000094
#define REG_SGWR1_H    0x4000096
#define REG_SGWR2      0x4000098
#define REG_SGWR2_L    0x4000098
#define REG_SGWR2_H    0x400009A
#define REG_SGWR3      0x400009C
#define REG_SGWR3_L    0x400009C
#define REG_SGWR3_H    0x400009E
#define REG_SGFIF0A    0x40000A0
#define REG_SGFIFOA_L  0x40000A0
#define REG_SGFIFOA_H  0x40000A2
#define REG_SGFIFOB    0x40000A4
#define REG_SGFIFOB_L  0x40000A4
#define REG_SGFIFOB_H  0x40000A6
#define REG_DMA0       0x40000B0
#define REG_DM0SAD     0x40000B0
#define REG_DM0SAD_L   0x40000B0
#define REG_DM0SAD_H   0x40000B2
#define REG_DM0DAD     0x40000B4
#define REG_DM0DAD_L   0x40000B4
#define REG_DM0DAD_H   0x40000B6
#define REG_DM0CNT     0x40000B8
#define REG_DM0CNT_L   0x40000B8
#define REG_DM0CNT_H   0x40000BA
#define REG_DMA1       0x40000BC
#define REG_DM1SAD     0x40000BC
#define REG_DM1SAD_L   0x40000BC
#define REG_DM1SAD_H   0x40000BE
#define REG_DM1DAD     0x40000C0
#define REG_DM1DAD_L   0x40000C0
#define REG_DM1DAD_H   0x40000C2
#define REG_DM1CNT     0x40000C4
#define REG_DM1CNT_L   0x40000C4
#define REG_DM1CNT_H   0x40000C6
#define REG_DMA2       0x40000C8
#define REG_DM2SAD     0x40000C8
#define REG_DM2SAD_L   0x40000C8
#define REG_DM2SAD_H   0x40000CA
#define REG_DM2DAD     0x40000CC
#define REG_DM2DAD_L   0x40000CC
#define REG_DM2DAD_H   0x40000CE
#define REG_DM2CNT     0x40000D0
#define REG_DM2CNT_L   0x40000D0
#define REG_DM2CNT_H   0x40000D2
#define REG_DMA3       0x40000D4
#define REG_DM3SAD     0x40000D4
#define REG_DM3SAD_L   0x40000D4
#define REG_DM3SAD_H   0x40000D6
#define REG_DM3DAD     0x40000D8
#define REG_DM3DAD_L   0x40000D8
#define REG_DM3DAD_H   0x40000DA
#define REG_DM3CNT     0x40000DC
#define REG_DM3CNT_L   0x40000DC
#define REG_DM3CNT_H   0x40000DE
#define REG_TM0D       0x4000100
#define REG_TM0CNT     0x4000102
#define REG_TM1D       0x4000104
#define REG_TM1CNT     0x4000106
#define REG_TM2D       0x4000108
#define REG_TM2CNT     0x400010A
#define REG_TM3D       0x400010C
#define REG_TM3CNT     0x400010E
#define REG_SCD0       0x4000120
#define REG_SCD1       0x4000122
#define REG_SCD2       0x4000124
#define REG_SCD3       0x4000126
#define REG_SCCNT      0x4000128
#define REG_SCCNT_L    0x4000128
#define REG_SCCNT_H    0x400012A
#define REG_SIODATA8   0x400012A
#define REG_P1         0x4000130
#define REG_KEYINPUT   0x4000130
#define REG_P1CNT      0x4000132
#define REG_R          0x4000134
#define REG_HS_CTRL    0x4000140
#define REG_JOYRE      0x4000150
#define REG_JOYRE_L    0x4000150
#define REG_JOYRE_H    0x4000152
#define REG_JOYTR      0x4000154
#define REG_JOYTR_L    0x4000154
#define REG_JOYTR_H    0x4000156
#define REG_JSTAT      0x4000158
#define REG_JSTAT_L    0x4000158
#define REG_JSTAT_H    0x400015A
#define REG_IE         0x4000200
#define REG_IF         0x4000202
#define REG_WSCNT      0x4000204
#define REG_IME        0x4000208
#define REG_PAUSE      0x4000300

///// REG_DISPCNT defines
#define MODE_0 0x0
#define MODE_1 0x1
#define MODE_2 0x2
#define MODE_3 0x3
#define MODE_4 0x4
#define MODE_5 0x5

#define BACKBUFFER 0x10
#define H_BLANK_OAM 0x20 

#define OBJ_MAP_2D 0x0
#define OBJ_MAP_1D 0x40

#define FORCE_BLANK 0x80

#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200 
#define BG2_ENABLE 0x400
#define BG3_ENABLE 0x800
#define OBJ_ENABLE 0x1000 

#define WIN1_ENABLE 0x2000 
#define WIN2_ENABLE 0x4000
#define WINOBJ_ENABLE 0x8000

#define DISP_MODE_0             0x0000
#define DISP_MODE_1             0x0001
#define DISP_MODE_2             0x0002
#define DISP_MODE_3             0x0003
#define DISP_MODE_4             0x0004
#define DISP_MODE_5             0x0005

#define DISP_OBJ_CHAR_2D_MAP    0x0000
#define DISP_OBJ_CHAR_1D_MAP    0x0040
#define DISP_BG0_ON             0x0100
#define DISP_BG1_ON             0x0200
#define DISP_BG2_ON             0x0400
#define DISP_BG3_ON             0x0800
#define DISP_OBJ_ON             0x1000
#define DISP_WIN0_ON            0x2000
#define DISP_WIN1_ON            0x4000
#define DISP_WIN01_ON           0x6000
#define DISP_OBJWIN_ON          0x8000

#define VRAM_SIZE               0x18000
#define PLTT_SIZE               (2*256*2)
#define BG_PLTT_SIZE            (2*256)
#define OBJ_PLTT_SIZE           (2*256)
#define OAM_SIZE                (8*128)

#define PLTT                    0x05000000
#define PLTT_END                (PLTT +      0x400)
#define BG_PLTT                 (PLTT +        0x0)
#define BG_PLTT_END             (PLTT +      0x200)
#define OBJ_PLTT                (PLTT +      0x200)
#define OBJ_PLTT_END            (PLTT +      0x400)

#define BG_PRIORITY_0           0x0000
#define BG_PRIORITY_1           0x0001
#define BG_PRIORITY_2           0x0002
#define BG_PRIORITY_3           0x0003
#define BG_MOS_ON               0x0040
#define BG_MOS_OFF              0x0000
#define BG_COLOR_16             0x0000
#define BG_COLOR_256            0x0080
#define BG_LOOP_ON              0x2000
#define BG_LOOP_OFF             0x0000
#define BG_SCREEN_SIZE_0        0x0000
#define BG_SCREEN_SIZE_1        0x4000
#define BG_SCREEN_SIZE_2        0x8000
#define BG_SCREEN_SIZE_3        0xc000

#define BG_PRIORITY_SHIFT       0
#define BG_CHAR_BASE_SHIFT      2
#define BG_SCREEN_BASE_SHIFT    8
#define BG_SCREEN_SIZE_SHIFT    14

#define KEY_A 		1
#define KEY_B 		2
#define KEY_SELECT	4
#define KEY_START 	8
#define KEY_RIGHT 	16
#define KEY_LEFT 	32
#define KEY_UP 		64
#define KEY_DOWN 	128
#define KEY_R		256
#define KEY_L 		512

// ÉLÅ[ì¸óÕóp
#define KEYS (vu32*)0x04000130

#define RGB(r,g,b)	(((b) << 10) | ((g) << 5) | (r))

#define SCREEN_SIZE_X	240
#define SCREEN_SIZE_Y	160

#define VRAM_ADDRESS	0x6000000
#define EX_WRAM			0x02000000
#define EX_WRAM_END		(EX_WRAM + 0x40000)
#define CPU_WRAM		0x03000000
#define CPU_WRAM_END	(CPU_WRAM + 0x8000)
#define VRAM			0x06000000
#define VRAM_END		(VRAM +    0x18000)
#define BG_VRAM			(VRAM +        0x0)
#define BG_BITMAP0_VRAM	(VRAM +        0x0)
#define BG_BITMAP1_VRAM	(VRAM +     0xa000)
#define OBJ_MODE0_VRAM	(VRAM +    0x10000)
#define OBJ_MODE1_VRAM	(VRAM +    0x10000)
#define OBJ_MODE2_VRAM	(VRAM +    0x10000)
#define OBJ_MODE3_VRAM	(VRAM +    0x14000)
#define OBJ_MODE4_VRAM	(VRAM +    0x14000)
#define OBJ_MODE5_VRAM	(VRAM +    0x14000)
#define OBJ_VRAM_END	(VRAM +    0x18000)
#define OAM				0x07000000
#define OAM_END			(OAM +       0x400)
#define ROM_ADDRESS		0x08000000


//Interrupt FLAG
#define V_BLANK_INTR_FLAG       0x0001
#define H_BLANK_INTR_FLAG       0x0002
#define V_COUNT_INTR_FLAG       0x0004
#define TIMER0_INTR_FLAG        0x0008
#define TIMER1_INTR_FLAG        0x0010
#define TIMER2_INTR_FLAG        0x0020
#define TIMER3_INTR_FLAG        0x0040
#define SIO_INTR_FLAG           0x0080
#define DMA0_INTR_FLAG          0x0100
#define DMA1_INTR_FLAG          0x0200
#define DMA2_INTR_FLAG          0x0400
#define DMA3_INTR_FLAG          0x0800
#define KEY_INTR_FLAG           0x1000

#define INTR_OFF() (REG16(REG_IME) = 0)
#define INTR_ON()  (REG16(REG_IME) = 1)

//DISP_STAR
#define DISPSTAT_VBIRQ			(1 << 3)
#define DISPSTAT_HBIRQ			(1 << 4)
#define DISPSTAT_VCIRQ			(1 << 5)

//SIO
#define REG_RCNT                0x4000134
#define REG_SIOCNT              0x4000128
#define REG_SIODATA32           0x4000120

// UART Mode
#define UART_9600			0
#define UART_38400			1
#define UART_57600			2
#define UART_115200			3
#define UART_CTS_ENABLE		(1 << 2)
#define UART_PARITY_EVEN	0
#define UART_PARITY_ODD		(1 << 3)
#define UART_SEND_FULL		(1 << 4)
#define UART_RECV_EMPTY		(1 << 5)
#define UART_ERROR 			(1 << 6)
#define UART_DATA7			0
#define UART_DATA8			(1 << 7)
#define UART_FIFO_DISABLE 	0
#define UART_FIFO_ENABLE 	(1 << 8)
#define UART_PARITY_NONE	0
#define UART_PARITY_ENABLE	(1 << 9)
#define UART_SEND_ENABLE  	(1 << 10)
#define UART_RECV_ENABLE   	(1 << 11)
#define UART_MODE		  	(3 << 12)
#define UART_IRQ_ENABLE  	(1 << 14)

//DMA MACRO
#define DMA_ENABLE              0x80000000
#define DMA_TIMMING_IMM         0x00000000
#define DMA_SRC_INC             0x00000000
#define DMA_DEST_INC            0x00000000
#define DMA_SRC_FIX             0x01000000
#define DMA_16BIT_BUS           0x00000000
#define DMA_32BIT_BUS           0x04000000

#define DmaSet(DmaNo, Srcp, Destp, DmaCntData)          \
{                                                       \
    vu32 *(DmaCntp) = (vu32 *)REG_DMA##DmaNo;           \
    DmaCntp[0] = (vu32 )(Srcp);                         \
    DmaCntp[1] = (vu32 )(Destp);                        \
    DmaCntp[2] = (vu32 )(DmaCntData);                   \
    DmaCntp[2];                                         \
}

#define DmaCopy(DmaNo, Srcp, Destp, Size, Bit)              \
                                                            \
    DmaSet(DmaNo, Srcp, Destp,  (                           \
        DMA_ENABLE         | DMA_TIMMING_IMM |              \
        DMA_SRC_INC        | DMA_DEST_INC    |              \
        DMA_##Bit##BIT_BUS | ((Size)/((Bit)/8))))

#define DmaClear(DmaNo, Data, Destp, Size, Bit)             \
{                                                           \
    vu##Bit Tmp = (vu##Bit )(Data);                         \
    DmaSet(DmaNo, &Tmp, Destp, (                            \
        DMA_ENABLE         | DMA_TIMMING_IMM |              \
        DMA_SRC_FIX        | DMA_DEST_INC    |              \
        DMA_##Bit##BIT_BUS | ((Size)/(Bit/8))));            \
}

#define DmaArrayCopy(  DmaNo, Srcp, Destp, Bit)             \
        DmaCopy(       DmaNo, Srcp, Destp, sizeof(Srcp), Bit)


#define INTR_VECTOR_BUF	(CPU_WRAM_END - 0x4)


#endif

###############################################################################
# ParaNavi
# Copyright (C) 2005 Rinos, All rights reserved.
###############################################################################

# パラナビ本体
TARGET	= ParaNavi

T_DEP	= $(TARGET).depend
T_ELF	= $(TARGET).elf
T_BIN	= $(TARGET).bin

.T_ASM	= crt0.s
.T_CPP	= main.c sound.c garmin.c vario.c arith.c tracklog.c \
			menu.kana.c navi.kana.c video.kana.c \
			Isr.arm.c \
			f_joycarry.c f_scsd.c f_emul.c \
			font/comp.c
.T_OBJ	= $(.T_ASM:.s=.o) $(.T_CPP:.c=.o)


# ブートローダ
BOOT	= Boot

B_DEP	= $(BOOT).depend
B_ELF	= $(BOOT).elf
B_BIN	= $(BOOT).bin

.B_ASM	= crt0.s
.B_CPP	= bootloader.c
.B_OBJ	= $(.B_ASM:.s=.o) $(.B_CPP:.c=.o)


# マージファイル
MARGE	= ParaNaviM.gba


###############################################################################
# コンパイラオプション
CC		= gcc
FASFLAGS= -I$(AGBINC) -mthumb-interwork
SFLAGS	= 
CFLAGS	= -Wall -O -mthumb -nostdlib
CFLAGS2	= -Wall -O -nostdlib
LDFLAGS	= -mthumb -nostartfiles -mthumb-interwork -lm -Wl

# 仮名変換ルール
%.kana.o: %.kana.c
	tools/tr_kana.exe < $< > $<_t.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $<_t.c -o $@
	rm $<_t.c

%.arm.o: %.arm.c
	$(CC) $(INCLUDES) $(CFLAGS2) -c $< -o $@

%.o: %.s
	$(CC) $(SFLAGS) -c $< -o $@


###############################################################################
# マージファイル作成
$(MARGE): $(T_BIN) $(B_BIN)
	cmd /c copy /b ParaNavi.bin + boot.bin + Voice.bin $@

all: $(MARGE)


# パラナビ本体
$(T_BIN): $(T_ELF)
	objcopy -v -O binary $< $@

$(T_ELF): $(.T_OBJ) Makefile $(T_DEP)
	$(CC) -g -o $@ $(.T_OBJ) $(LDFLAGS) -Ttext 0x02000000 -Tbss 0x03000000

$(T_DEP): 
	$(CC) $(CFLAGS) -M $(.T_CPP) > $(T_DEP)


# ブートローダ
$(B_BIN): $(B_ELF)
	objcopy -v -O binary $< $@
	tools/add8kb.exe $@

$(B_ELF): $(.B_OBJ) Makefile $(B_DEP)
	$(CC) -g -o $@ $(.B_OBJ) $(LDFLAGS) -Ttext 0x03000000 -Tbss 0x03000000

$(B_DEP): 
	$(CC) $(CFLAGS) -M $(.B_CPP) > $(B_DEP)


# 圧縮フォント ###############################################################
compfont:
	tools/mkfont.exe 1 < font/kanafont.bmp > font/kanafont.c

	echo "typedef unsigned long u32;" > font/comp.c
	tools/c2b dat1 < font/bignum.c
	tools/gbacomp.exe -v -c dat1 dat2
	tools/b2c dat2 BIGNUM_COMP >> font/comp.c
	
	tools/c2b dat1 < font/kanafont.c
	tools/gbacomp.exe -v -c dat1 dat2
	tools/b2c dat2 FONT_COMP >> font/comp.c
	
	tools/c2b dat1 < font/obj_tile.c
	tools/gbacomp.exe -v -c dat1 dat2
	tools/b2c dat2 OBJ_TILE_COMP >> font/comp.c
	
	tools/c2b dat1 < font/bg_tile.c
	tools/gbacomp.exe -v -c dat1 dat2
	tools/b2c dat2 BG_TILE_COMP >> font/comp.c

	rm dat1 dat2


# etc ########################################################################
clean:
	rm -f *.o *.elf */*.o *.sav *.bak *.map *.depend

# SDカードにコピーしてメディア取り外し
sd: $(MARGE)
	cmd /c copy /b $< f:
#	"C:\Program Files\Software Factory\D-Unplugger\dunpluggerc.exe" g:

.PHONY: all compfont clean sd depend

include $(T_DEP)
include $(B_DEP)

#
# FM-7 EMULATOR "XM7"
#
# Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
# [ makefile for Borland C++ ]
#

#
# ツール定義
#
AS		= nasmw
AFLAGS		= -d _OMF -f obj
CC		= bcc32
CFLAGS		= -5 -c -DNDEBUG -O2
LD		= ilink32
LFLAGS		= -aa -x -Tpe
RC   		= brcc32
RFLAGS		= -c932

#
# ファイル定義
#
TARGET		= XM7.exe

ASM_SRCS	= \
		cpu_x86.asm \
		w32_rend.asm
ASM_OBJS	= ${ASM_SRCS:.asm=.obj}

C_SRCS		= \
		aluline.c \
		apalet.c \
		disasm.c \
		display.c \
		fdc.c \
		history.c \
		kanji.c \
		keyboard.c \
		maincpu.c \
		mainetc.c \
		mainmem.c \
		mmr.c \
		multipag.c \
		opn.c \
		rtc.c \
		schedule.c \
		subcpu.c \
		subctrl.c \
		submem.c \
		system.c \
		tapelp.c \
		tools.c \
		ttlpalet.c \
		whg.c \
		w32_bar.c \
		w32_cfg.c \
		w32_cmd.c \
		w32_draw.c \
		w32_file.c \
		w32_gdi.c \
		w32_main.c \
		w32_sch.c \
		w32_sub1.c \
		w32_sub2.c \
		w32_sub3.c \
		w32_ver.c
C_OBJS		= ${C_SRCS:.c=.obj}

CPP_SRCS	= \
		fmgen.cpp \
		fmtimer.cpp \
		opna.cpp \
		psg.cpp \
		w32_dd.cpp \
		w32_kbd.cpp \
		w32_snd.cpp
CPP_OBJS	= ${CPP_SRCS:.cpp=.obj}

RC_SRCS		= \
		w32_res.rc
RC_OBJS		= ${RC_SRCS:.rc=.res}

MAPFILES	=

LIBFILES	= \
		c0w32.obj \
		cw32mt.lib \
		kernel32.lib \
		user32.lib \
		gdi32.lib \
		ddraw.lib \
		dinput.lib \
		dsound.lib \
		dxextra.lib \
		comctl32.lib \
		comdlg32.lib \
		imm32.lib \
		import32.lib \
		winmm.lib

DEFFILES	=

#
# デフォルト生成ルール
#
.asm.obj:
		$(AS) $(AFLAGS) $&.asm

.c.obj:
		$(CC) $(CFLAGS) $&.c

.cpp.obj:
		$(CC) $(CFLAGS) $&.cpp

.rc.res:
		$(RC) $(RFLAGS) $&.rc

#
# ソースファイル→ターゲット
#
$(TARGET) : $(ASM_OBJS) $(C_OBJS) $(CPP_OBJS) $(RC_OBJS)
		$(LD) $(LFLAGS) @&&|
		$(ASM_OBJS) \
		$(C_OBJS) \
		$(CPP_OBJS) \
		,$(TARGET) \
		,$(MAPFILES) \
		,$(LIBFILES) \
		,$(DEFFILES) \
		,$(RC_OBJS)
|

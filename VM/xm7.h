/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ 共通定義 ]
 */

#ifndef _xm7_h_
#define _xm7_h_

#include <stdio.h>

/*
 *	定数、型定義
 */

/* 汎用定数 */
#ifndef FALSE
#define FALSE			0
#define TRUE			(!FALSE)
#endif
#ifndef NULL
#define NULL			((void)0)
#endif

/* 診断 */
#ifndef ASSERT
#ifdef _DEBUG
#define ASSERT(exp)		assert(exp)
#else
#define ASSERT(exp)		((void)0)
#endif
#endif

/* 最適化 */
#if defined(_WIN32) && defined(__BORLANDC__)
#define FASTCALL		__fastcall
#else
#define FASTCALL
#endif

/* 基本型定義 */
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef int BOOL;

/* CPUレジスタ定義 */
#ifdef _WIN32
#pragma pack(push, 1)
#endif
typedef struct {
	BYTE cc;
	BYTE dp;
	union {
		struct {
#ifdef _WIN32
			BYTE b;
			BYTE a;
#endif
#ifdef __MSDOS__
			BYTE b;
			BYTE a;
#endif
#ifdef HUMAN68K
			BYTE a;
			BYTE b;
#endif
#ifdef _XWIN
			BYTE b __attribute__((aligned(1)));
			BYTE a __attribute__((aligned(1)));
#endif
		} h;
		WORD d;
	} acc;
	WORD x;
	WORD y;
	WORD u;
	WORD s;
	WORD pc;
	WORD intr;
	WORD cycle;
	WORD total;
	BYTE FASTCALL (*readmem)(WORD);
	void FASTCALL (*writemem)(WORD, BYTE);
} cpu6809_t;
#ifdef _WIN32
#pragma pack(pop)
#endif

/* 割り込み定義 */
#define INTR_NMI		0x0001			/* NMI割り込み */
#define INTR_FIRQ		0x0002			/* FIRQ割り込み */
#define INTR_IRQ		0x0004			/* IRQ割り込み */

#define INTR_SLOAD		0x0010			/* リセット後Sを設定 */
#define INTR_SYNC_IN	0x0020			/* SYNC実行中 */
#define INTR_SYNC_OUT	0x0040			/* SYNC終了可能 */
#define INTR_CWAI_IN	0x0080			/* CWAI実行中
#define INTR_CWAI_OUT	0x0100			/* CWAI終了可能 */

/* ブレークポイント定義 */
#define BREAKP_NOTUSE	0				/* 未使用 */
#define BREAKP_ENABLED	1				/* 使用中 */
#define BREAKP_DISABLED	2				/* 禁止中 */
#define BREAKP_STOPPED	3				/* 停止中 */
#define BREAKP_MAXNUM	8				/* ブレークポイントの個数 */
typedef struct {
	int flag;							/* 上のフラグ */
	int cpu;							/* CPU種別 */
	WORD addr;							/* ブレークポイントアドレス */
} breakp_t;

/* イベント定義 */
#define EVENT_NOTUSE	0				/* 未使用 */
#define EVENT_ENABLED	1				/* 使用中 */
#define EVENT_DISABLED	2				/* 禁止中 */
#define EVENT_MAXNUM	16				/* イベントの個数 */
typedef struct {
	int flag;							/* 上のフラグ */
	DWORD current;						/* カレント時間カウンタ */
	DWORD reload;						/* リロード時間カウンタ */
	BOOL FASTCALL (*callback)(void);	/* コールバック関数 */
} event_t;

/* その他定数 */
#define MAINCPU			0				/* メインCPU */
#define SUBCPU			1				/* サブCPU */

#define BOOT_BASIC		0				/* BASICモード */
#define BOOT_DOS		1				/* DOSモード */

/*
 * ROMファイル名定義
 */
#define FBASIC_ROM		"FBASIC30.ROM"
#define SUBSYSC_ROM		"SUBSYS_C.ROM"
#define KANJI_ROM		"KANJI.ROM"
#define INITIATE_ROM	"INITIATE.ROM"
#define SUBSYSA_ROM		"SUBSYS_A.ROM"
#define SUBSYSB_ROM		"SUBSYS_B.ROM"
#define SUBSYSCG_ROM	"SUBSYSCG.ROM"

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */

/* システム(system.c) */
BOOL FASTCALL system_init(void);
										/* システム 初期化 */
void FASTCALL system_cleanup(void);
										/* システム クリーンアップ */
void FASTCALL system_reset(void);
										/* システム リセット */
BOOL FASTCALL system_save(char *filename);
										/* システム セーブ */
BOOL FASTCALL system_load(char *filename);
										/* システム ロード */

/* スケジューラ(schedule.c) */
BOOL FASTCALL schedule_init(void);
										/* スケジューラ 初期化 */
void FASTCALL schedule_cleanup(void);
										/* スケジューラ クリーンアップ */
void FASTCALL schedule_reset(void);
										/* スケジューラ リセット */
DWORD FASTCALL schedule_exec(DWORD microsec);
										/* 実行 */
void FASTCALL schedule_trace(void);
										/* トレース */
BOOL FASTCALL schedule_setbreak(int cpu, WORD addr);
										/* ブレークポイント設定 */
BOOL FASTCALL schedule_setevent(int id, DWORD microsec, BOOL FASTCALL (*func)(void));
										/* イベント追加 */
BOOL FASTCALL schedule_delevent(int id);
										/* イベント削除 */
void FASTCALL schedule_handle(int id, BOOL FASTCALL (*func)(void));
										/* イベントハンドラ設定 */
BOOL FASTCALL schedule_save(int fileh);
										/* スケジューラ セーブ */
BOOL FASTCALL schedule_load(int fileh, int ver);
										/* スケジューラ ロード */

/* 逆アセンブラ(disasm.c) */
int FASTCALL disline(int cputype, WORD pc, char *buffer);
										/* １行逆アセンブル */

/* メインCPUメモリ(mainmem.c) */
BOOL FASTCALL mainmem_init(void);
										/* メインCPUメモリ 初期化 */
void FASTCALL mainmem_cleanup(void);
										/* メインCPUメモリ クリーンアップ */
void FASTCALL mainmem_reset(void);
										/* メインCPUメモリ リセット */
BYTE FASTCALL mainmem_readb(WORD addr);
										/* メインCPUメモリ 読み出し */
BYTE FASTCALL mainmem_readbnio(WORD addr);
										/* メインCPUメモリ 読み出し(I/Oなし) */
void FASTCALL mainmem_writeb(WORD addr, BYTE dat);
										/* メインCPUメモリ 書き込み */
BOOL FASTCALL mainmem_save(int fileh);
										/* メインCPUメモリ セーブ */
BOOL FASTCALL mainmem_load(int fileh, int ver);
										/* メインCPUメモリ ロード */

/* サブCPUメモリ(submem.c) */
BOOL FASTCALL submem_init(void);
										/* サブCPUメモリ 初期化 */
void FASTCALL submem_cleanup(void);
										/* サブCPUメモリ クリーンアップ */
void FASTCALL submem_reset(void);
										/* サブCPUメモリ リセット */
BYTE FASTCALL submem_readb(WORD addr);
										/* サブCPUメモリ 読み出し */
BYTE FASTCALL submem_readbnio(WORD addr);
										/* サブCPUメモリ 読み出し(I/Oなし) */
void FASTCALL submem_writeb(WORD addr, BYTE dat);
										/* サブCPUメモリ 書き込み */
BOOL FASTCALL submem_save(int fileh);
										/* サブCPUメモリ セーブ */
BOOL FASTCALL submem_load(int fileh, int ver);
										/* サブCPUメモリ ロード */

/* メインCPU(maincpu.c) */
BOOL FASTCALL maincpu_init(void);
										/* メインCPU 初期化 */
void FASTCALL maincpu_cleanup(void);
										/* メインCPU クリーンアップ */
void FASTCALL maincpu_reset(void);
										/* メインCPU リセット */
void FASTCALL maincpu_execline(void);
										/* メインCPU １行実行 */
void FASTCALL maincpu_exec(void);
										/* メインCPU 実行 */
void FASTCALL maincpu_firq(void);
										/* メインCPU FIRQ割り込み要求 */
void FASTCALL maincpu_irq(void);
										/* メインCPU IRQ割り込み要求 */
BOOL FASTCALL maincpu_save(int fileh);
										/* メインCPU セーブ */
BOOL FASTCALL maincpu_load(int fileh, int ver);
										/* メインCPU ロード */

/* サブCPU(subcpu.c) */
BOOL FASTCALL subcpu_init(void);
										/* サブCPU 初期化 */
void FASTCALL subcpu_cleanup(void);
										/* サブCPU クリーンアップ */
void FASTCALL subcpu_reset(void);
										/* サブCPU リセット */
void FASTCALL subcpu_execline(void);
										/* サブCPU １行実行 */
void FASTCALL subcpu_exec(void);
										/* サブCPU 実行 */
void FASTCALL subcpu_nmi(void);
										/* サブCPU NMI割り込み要求 */
void FASTCALL subcpu_firq(void);
										/* サブCPU FIRQ割り込み要求 */
void FASTCALL subcpu_irq(void);
										/* サブCPU IRQ割り込み要求 */
BOOL FASTCALL subcpu_save(int fileh);
										/* サブCPU セーブ */
BOOL FASTCALL subcpu_load(int fileh, int ver);
										/* サブCPU ロード */

/*
 *	CPU、その他主要ワークエリア
 */
extern cpu6809_t maincpu;
										/* メインCPU */
extern cpu6809_t subcpu;
										/* サブCPU */
extern int fm7_ver;
										/* 動作バージョン */
extern breakp_t breakp[BREAKP_MAXNUM];
										/* ブレークポイント */
extern event_t event[EVENT_MAXNUM];
										/* イベント */
extern BOOL run_flag;
										/* 動作フラグ */
extern BOOL stopreq_flag;
										/* 停止要求フラグ */
extern DWORD main_speed;
										/* メインCPUスピード */
extern DWORD mmr_speed;
										/* メインCPU(MMR)スピード */
extern BOOL cycle_steel;
										/* サイクルスチールフラグ */

/*
 *	メモリ
 */
extern BYTE *mainram_a;
										/* RAM (表RAM)      $8000 */
extern BYTE *mainram_b;
										/* RAM (裏RAM)      $7C80 */
extern BYTE *basic_rom;
										/* ROM (F-BASIC)    $7C00 */
extern BYTE *main_io;
										/* メインCPU I/O    $0100 */
extern BYTE *vram_c;
										/* VRAM(タイプC)    $C000 */
extern BYTE *subrom_c;
										/* ROM (タイプC)    $2800 */
extern BYTE *sub_ram;
										/* コンソールRAM    $1680 */
extern BYTE *sub_io;
										/* サブCPU I/O      $0100 */

extern BYTE *extram_a;
										/* RAM (FM77AV)    $10000 */
extern BYTE *boot_ram;
										/* BOOT (RAM)        $200 */
extern BYTE *init_rom;
										/* イニシエートROM  $2000 */

extern BYTE *subrom_a;
										/* ROM (タイプA)    $2000 */
extern BYTE *subrom_b;
										/* ROM (タイプB)    $2000 */
extern BYTE *subromcg;
										/* ROM (CG)         $2000 */
extern BYTE *vram_b;
										/* VRAM (タイプA,B) $C000 */

extern BOOL boot_mode;
										/* 起動モード */
extern BOOL basicrom_en;
										/* F-BASIC 3.0 ROM イネーブル */
extern BOOL initrom_en;
										/* イニシエータROM イネーブル */
extern BOOL bootram_rw;
										/* ブートRAM 書き込み可能 */
extern BYTE subrom_bank;
										/* サブシステムROMバンク */
extern BYTE cgrom_bank;
										/* CGROMバンク */

#ifdef __cplusplus
}
#endif

#endif	/* _xm7_h_ */

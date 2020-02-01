/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API サブウインドウ ]
 */

#ifdef _WIN32

#ifndef _w32_sub_h_
#define _w32_sub_h_

/*
 *	サブウインドウ定義
 */
#define SWND_BREAKPOINT			0		/* ブレークポイント */
#define SWND_SCHEDULER			1		/* スケジューラ */
#define SWND_CPUREG_MAIN		2		/* CPUレジスタ メイン */
#define SWND_CPUREG_SUB			3		/* CPUレジスタ サブ */
#define SWND_DISASM_MAIN		4		/* 逆アセンブル メイン */
#define SWND_DISASM_SUB			5		/* 逆アセンブル サブ */
#define SWND_MEMORY_MAIN		6		/* メモリダンプ メイン */
#define SWND_MEMORY_SUB			7		/* メモリダンプ サブ */
#define SWND_FDC				8		/* FDC */
#define SWND_OPNREG				9		/* OPNレジスタ */
#define SWND_OPNDISP			10		/* OPNディスプレイ */
#define SWND_SUBCTRL			11		/* サブCPUコントロール */
#define SWND_KEYBOARD			12		/* キーボード */
#define SWND_MMR				13		/* MMR */
#define SWND_MAXNUM				14		/* 最大サブウインドウ数 */

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
HWND FASTCALL CreateBreakPoint(HWND hParent, int index);
										/* ブレークポイントウインドウ 作成 */
void FASTCALL RefreshBreakPoint(void);
										/* ブレークポイントウインドウ リフレッシュ */
HWND FASTCALL CreateScheduler(HWND hParent, int index);
										/* スケジューラウインドウ 作成 */
void FASTCALL RefreshScheduler(void);
										/* スケジューラウインドウ リフレッシュ */
HWND FASTCALL CreateCPURegister(HWND hParent, BOOL bMain, int index);
										/* CPUレジスタウインドウ 作成 */
void FASTCALL RefreshCPURegister(void);
										/* CPUレジスタウインドウ リフレッシュ */
HWND FASTCALL CreateDisAsm(HWND hParent, BOOL bMain, int index);
										/* 逆アセンブルウインドウ 作成 */
void FASTCALL RefreshDisAsm(void);
										/* 逆アセンブルウインドウ リフレッシュ */
void FASTCALL AddrDisAsm(BOOL bMain, WORD wAddr);
										/* 逆アセンブルウインドウ アドレス指定 */
HWND FASTCALL CreateMemory(HWND hParent, BOOL bMain, int index);
										/* メモリダンプウインドウ 作成 */
void FASTCALL RefreshMemory(void);
										/* メモリダンプウインドウ リフレッシュ */
void FASTCALL AddrMemory(BOOL bMain, WORD wAddr);
										/* メモリダンプウインドウ アドレス指定 */

HWND FASTCALL CreateFDC(HWND hParent, int index);
										/* FDCウインドウ 作成 */
void FASTCALL RefreshFDC(void);
										/* FDCウインドウ リフレッシュ */
HWND FASTCALL CreateOPNReg(HWND hParent, int index);
										/* OPNレジスタウインドウ 作成 */
void FASTCALL RefreshOPNReg(void);
										/* OPNレジスタウインドウ リフレッシュ */
HWND FASTCALL CreateSubCtrl(HWND hParent, int index);
										/* サブCPUコントロールウインドウ 作成 */
void FASTCALL RefreshSubCtrl(void);
										/* サブCPUコントロールウインドウ リフレッシュ */
HWND FASTCALL CreateOPNDisp(HWND hParent, int index);
										/* OPNディスプレイウインドウ 作成 */
void FASTCALL RefreshOPNDisp(void);
										/* OPNディスプレイウインドウ リフレッシュ */
HWND FASTCALL CreateKeyboard(HWND hParent, int index);
										/* キーボードウインドウ 作成 */
void FASTCALL RefreshKeyboard(void);
										/* キーボードウインドウ リフレッシュ */
HWND FASTCALL CreateMMR(HWND hParent, int index);
										/* MMRウインドウ 作成 */
void FASTCALL RefreshMMR(void);
										/* MMRウインドウ リフレッシュ */

/*
 *	主要ワーク
 */
extern HWND hSubWnd[SWND_MAXNUM];
										/* サブウインドウ */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_sub_h_ */
#endif	/* _WIN32 */


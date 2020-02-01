/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API ウインドウ表示 ]
 */

#ifdef _WIN32

#ifndef _w32_gdi_h_
#define _w32_gdi_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
void FASTCALL InitGDI(void);
										/* 初期化 */
void FASTCALL CleanGDI(void);
										/* クリーンアップ */
BOOL FASTCALL SelectGDI(HWND hWnd);
										/* セレクト */
void FASTCALL DrawGDI(HWND hWnd, HDC hDC);
										/* 描画 */
void FASTCALL VramGDI(WORD addr);
										/* VRAM書き込み通知 */
void FASTCALL DigitalGDI(void);
										/* TTLパレット通知 */
void FASTCALL AnalogGDI(void);
										/* アナログパレット通知 */
void FASTCALL ReDrawGDI(void);
										/* 再描画通知 */

/*
 *	主要ワーク
 */
extern WORD rgbTTLGDI[8];
										/* デジタルパレット */
extern WORD rgbAnalogGDI[4096];
										/* アナログパレット */
extern BYTE *pBitsGDI;
										/* ビットデータ */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_gdi_h_ */
#endif	/* _WIN32 */

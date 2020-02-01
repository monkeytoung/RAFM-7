/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API コントロールバー ]
 */

#ifdef _WIN32

#ifndef _w32_bar_h_
#define _w32_bar_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
HWND FASTCALL CreateStatus(HWND hWnd);
										/* ステータスバー作成 */
void FASTCALL DrawStatus(void);
										/* 描画 */
void FASTCALL PaintStatus(void);
										/* すべて再描画 */
void FASTCALL SizeStatus(LONG cx);
										/* サイズ変更 */
void FASTCALL OwnerDrawStatus(DRAWITEMSTRUCT *pDI);
										/* オーナードロー */
void FASTCALL OnMenuSelect(WPARAM wParam);
										/* WM_MENUSELECT */
void FASTCALL OnExitMenuLoop(void);
										/* WM_EXITMENULOOP */

/*
 *	主要ワーク
 */
extern HWND hStatusBar;
										/* ステータスバー */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_bar_h_ */
#endif	/* _WIN32 */


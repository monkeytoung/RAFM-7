/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API 表示 ]
 */

#ifdef _WIN32

#ifndef _w32_draw_h_
#define _w32_draw_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
void FASTCALL InitDraw(void);
										/* 初期化 */
void FASTCALL CleanDraw(void);
										/* クリーンアップ */
BOOL FASTCALL SelectDraw(HWND hWnd);
										/* セレクト */
void FASTCALL ModeDraw(HWND hWnd, BOOL bFullScreen);
										/* 描画モード変更 */
void FASTCALL OnPaint(HWND hWnd);
										/* 再描画 */
void FASTCALL OnDraw(HWND hWnd, HDC hDC);
										/* 部分描画 */

/*
 *	主要ワーク
 */
extern BOOL bFullScreen;
										/* フルスクリーン */
extern BOOL bFullScan;
										/* フルスキャン */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_draw_h_ */
#endif	/* _WIN32 */

/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API ]
 */

#ifdef _WIN32

#ifndef _w32_h_
#define _w32_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
HFONT FASTCALL CreateTextFont(void);
										/* テキストフォント作成 */
void FASTCALL LockVM(void);
										/* VMロック */
void FASTCALL UnlockVM(void);
										/* VMアンロック */
void FASTCALL OnCommand(HWND hWnd, WORD wID);
										/* WM_COMMAND */
void FASTCALL OnMenuPopup(HWND hWnd, HMENU hMenu, UINT uPos);
										/* WM_INITMENUPOPUP */
void FASTCALL OnDropFiles(HANDLE hDrop);
										/* WM_DROPFILES */
void FASTCALL OnCmdLine(LPTSTR lpCmdLine);
										/* コマンドライン */
void FASTCALL OnAbout(HWND hWnd);
										/* バージョン情報 */

/*
 *	主要ワーク
 */
extern HINSTANCE hAppInstance;
										/* アプリケーション インスタンス */
extern HWND hMainWnd;
										/* メインウインドウ */
extern HWND hDrawWnd;
										/* 描画ウインドウ */
extern int nErrorCode;
										/* エラーコード */
extern BOOL bMenuLoop;
										/* メニューループ中 */
extern BOOL bCloseReq;
										/* 終了要求フラグ */
extern LONG lCharWidth;
										/* キャラクタ横幅 */
extern LONG lCharHeight;
										/* キャラクタ縦幅 */
extern BOOL bSync;
										/* 実行に同期 */
extern BOOL bActivate;
										/* アクティベートフラグ */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_h_ */
#endif	/* _WIN32 */

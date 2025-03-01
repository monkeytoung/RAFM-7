/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API DirectDraw ]
 */

#ifdef _WIN32

#ifndef _w32_dd_h_
#define _w32_dd_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
void FASTCALL InitDD(void);
										/* 初期化 */
void FASTCALL CleanDD(void);
										/* クリーンアップ */
BOOL FASTCALL SelectDD(void);
										/* セレクト */
void FASTCALL DrawDD(void);
										/* 描画 */
void FASTCALL EnterMenuDD(HWND hWnd);
										/* メニュー開始 */
void FASTCALL ExitMenuDD(void);
										/* メニュー終了 */
void FASTCALL VramDD(WORD addr);
										/* VRAM書き込み通知 */
void FASTCALL DigitalDD(void);
										/* TTLパレット通知 */
void FASTCALL AnalogDD(void);
										/* アナログパレット通知 */
void FASTCALL ReDrawDD(void);
										/* 再描画通知 */

/*
 *	主要ワーク
 */
extern WORD rgbTTLDD[8];
										/* 640x200 パレット */
extern WORD rgbAnalogDD[4096];
										/* 320x200 パレット */
extern BOOL bDD480Line;
										/* 640x480 優先フラグ */
extern BOOL bDD480Status;
										/* 640x480 ステータスフラグ */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_dd_h_ */
#endif	/* _WIN32 */

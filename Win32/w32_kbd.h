/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API キーボード ]
 */

#ifdef _WIN32

#ifndef _w32_kbd_h_
#define _w32_kbd_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
void FASTCALL InitKbd(void);
										/* 初期化 */
void FASTCALL CleanKbd(void);
										/* クリーンアップ */
BOOL FASTCALL SelectKbd(HWND hWnd);
										/* セレクト */
void FASTCALL PollKbd(void);
										/* ポーリング */
void FASTCALL PollJoy(void);
										/* ポーリング */
void FASTCALL GetDefMapKbd(BYTE *pMap, int mode);
										/* デフォルトマップ取得 */
void FASTCALL SetMapKbd(BYTE *pMap);
										/* マップ設定 */
BOOL FASTCALL GetKbd(BYTE *pBuf);
										/* ポーリング＆キー情報取得 */
/*
 *	主要ワーク
 */
extern int nJoyType[2];
										/* ジョイスティックタイプ */
extern int nJoyRapid[2][2];
										/* 連射タイプ */
extern int nJoyCode[2][7];
										/* 生成コード */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_kbd_h_ */
#endif	/* _WIN32 */

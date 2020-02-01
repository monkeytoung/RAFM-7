/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API スケジューラ ]
 */

#ifdef _WIN32

#ifndef _w32_sch_h_
#define _w32_sch_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
void FASTCALL InitSch(void);
										/* 初期化 */
void FASTCALL CleanSch(void);
										/* クリーンアップ */
BOOL FASTCALL SelectSch(void);
										/* セレクト */
void FASTCALL ResetSch(void);
										/* 実行リセット */

/*
 *	主要ワーク
 */
extern DWORD dwExecTotal;
										/* 実行時間トータル */
extern BOOL bTapeFullSpeed;
										/* テープ高速モード */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_sch_h_ */
#endif	/* _WIN32 */

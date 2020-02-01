/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API コンフィギュレーション ]
 */

#ifdef _WIN32

#ifndef _w32_cfg_h_
#define _w32_cfg_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
void FASTCALL LoadCfg(void);
										/* 設定ロード */
void FASTCALL SaveCfg(void);
										/* 設定セーブ */
void FASTCALL ApplyCfg(void);
										/* 設定適用 */
void FASTCALL OnConfig(HWND hWnd);
										/* 設定ダイアログ */

/*
 *	主要ワーク
 */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_cfg_h_ */
#endif	/* _WIN32 */

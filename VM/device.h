/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ デバイス依存部 ]
 */

#ifndef _device_h_
#define _device_h_

/*
 *	定数定義
 */
#define OPEN_R		1					/* 読み込みモード */
#define OPEN_W		2					/* 書き込みモード */
#define OPEN_RW		3					/* 読み書きモード */

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */

/* 描画 */
void FASTCALL vram_notify(WORD addr, BYTE dat);
										/* VRAM書き込み通知 */
void FASTCALL ttlpalet_notify(void);
										/* デジタルパレット変更通知 */
void FASTCALL apalet_notify(void);
										/* アナログパレット変更通知 */
void FASTCALL display_notify(void);
										/* 画面無効通知 */
void FASTCALL digitize_notify(void);
										/* ディジタイズ通知 */
void FASTCALL vsync_notify(void);
										/* VSYNC通知 */

/* 音源、ジョイスティック */
void FASTCALL opn_notify(BYTE reg, BYTE dat);
										/* OPN出力通知 */
void FASTCALL whg_notify(BYTE reg, BYTE dat);
										/* WHG出力通知 */
BYTE FASTCALL joy_request(BYTE port);
										/* ジョイスティック要求 */

/* ファイル */
BOOL FASTCALL file_load(char *fname, BYTE *buf, int size);
										/* ファイルロード(ROM専用) */
int FASTCALL file_open(char *fname, int mode);
										/* ファイルオープン */
void FASTCALL file_close(int handle);
										/* ファイルクローズ */
DWORD FASTCALL file_getsize(int handle);
										/* ファイルレングス取得 */
BOOL FASTCALL file_seek(int handle, DWORD offset);
										/* ファイルシーク */
BOOL FASTCALL file_read(int handle, BYTE *ptr, DWORD size);
										/* ファイル読み込み */
BOOL FASTCALL file_write(int handle, BYTE *ptr, DWORD size);
										/* ファイル書き込み */

/* ファイルサブ(プラットフォーム非依存。実体はsystem.cにある) */
BOOL FASTCALL file_byte_read(int handle, BYTE *dat);
										/* バイト読み込み */
BOOL FASTCALL file_word_read(int handle, WORD *dat);
										/* ワード読み込み */
BOOL FASTCALL file_dword_read(int handle, DWORD *dat);
										/* ダブルワード読み込み */
BOOL FASTCALL file_bool_read(int handle, BOOL *dat);
										/* ブール読み込み */
BOOL FASTCALL file_byte_write(int handle, BYTE dat);
										/* バイト書き込み */
BOOL FASTCALL file_word_write(int handle, WORD dat);
										/* ワード書き込み */
BOOL FASTCALL file_dword_write(int handle, DWORD dat);
										/* ダブルワード書き込み */
BOOL FASTCALL file_bool_write(int handle, BOOL dat);
										/* ブール書き込み */
#ifdef __cplusplus
}
#endif

#endif	/* _device_h_ */

/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ ディスプレイ ]
 */

#ifndef _display_h_
#define _display_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
BOOL FASTCALL display_init(void);
										/* 初期化 */
void FASTCALL display_cleanup(void);
										/* クリーンアップ */
void FASTCALL display_reset(void);
										/* リセット */
BOOL FASTCALL display_readb(WORD addr, BYTE *dat);
										/* メモリ読み出し */
BOOL FASTCALL display_writeb(WORD addr, BYTE dat);
										/* メモリ書き込み */
BOOL FASTCALL display_save(int fileh);
										/* セーブ */
BOOL FASTCALL display_load(int fileh, int ver);
										/* ロード */

/*
 *	主要ワーク
 */
extern BOOL crt_flag;
										/* CRT表示フラグ */
extern BOOL vrama_flag;
										/* VRAMアクセスフラグ */
extern WORD vram_offset[2];
										/* VRAMオフセットレジスタ */
extern BOOL vram_offset_flag;
										/* 拡張VRAMオフセットフラグ */
extern BOOL subnmi_flag;
										/* サブNMIイネーブルフラグ */
extern BOOL vsync_flag;
										/* VSYNCフラグ */
extern BOOL blank_flag;
										/* ブランキングフラグ */
extern BYTE vram_active;
										/* アクティブページ */
extern BYTE *vram_aptr;
										/* VRAMアクティブポインタ */
extern BYTE vram_display;
										/* 表示ページ */
extern BYTE *vram_dptr;
										/* VRAM表示ポインタ */
#ifdef __cplusplus
}
#endif

#endif	/* _display_h_ */

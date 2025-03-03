/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ アナログパレット ]
 */

#ifndef _apalet_h_
#define _apalet_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
BOOL FASTCALL apalet_init(void);
										/* 初期化 */
void FASTCALL apalet_cleanup(void);
										/* クリーンアップ */
void FASTCALL apalet_reset(void);
										/* リセット */
BOOL FASTCALL apalet_readb(WORD addr, BYTE *dat);
										/* メモリ読み出し */
BOOL FASTCALL apalet_writeb(WORD addr, BYTE dat);
										/* メモリ書き込み */
BOOL FASTCALL apalet_save(int fileh);
										/* セーブ */
BOOL FASTCALL apalet_load(int fileh, int ver);
										/* ロード */

/*
 *	主要ワーク
 */
extern WORD apalet_no;
										/* パレットナンバ */
extern BYTE apalet_b[4096];
										/* Bレベル(0-15) */
extern BYTE apalet_r[4096];
										/* Rレベル(0-15) */
extern BYTE apalet_g[4096];
										/* Gレベル(0-15) */

#ifdef __cplusplus
}
#endif

#endif	/* _apalet_h_ */

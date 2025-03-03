/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ MMR ]
 */

#ifndef _mmr_h_
#define _mmr_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
BOOL FASTCALL mmr_init(void);
										/* 初期化 */
void FASTCALL mmr_cleanup(void);
										/* クリーンアップ */
void FASTCALL mmr_reset(void);
										/* リセット */
BOOL FASTCALL mmr_readb(WORD addr, BYTE *dat);
										/* メモリ読み出し */
BOOL FASTCALL mmr_writeb(WORD addr, BYTE dat);
										/* メモリ書き込み */
BOOL FASTCALL mmr_extrb(WORD *addr, BYTE *dat);
										/* MMR経由読み出し */
BOOL FASTCALL mmr_extbnio(WORD *addr, BYTE *dat);
										/* MMR経由読み出し */
BOOL FASTCALL mmr_extwb(WORD *addr, BYTE dat);
										/* MMR経由書き込み */
BOOL FASTCALL mmr_save(int fileh);
										/* セーブ */
BOOL FASTCALL mmr_load(int fileh, int ver);
										/* ロード */

/*
 *	主要ワーク
 */
extern BOOL mmr_flag;
										/* MMR有効フラグ */
extern BYTE mmr_seg;
										/* MMRセグメント */
extern BYTE mmr_reg[0x40];
										/* MMRレジスタ */
extern BOOL twr_flag;
										/* TWR有効フラグ */
extern BYTE twr_reg;
										/* TWRレジスタ */
#ifdef __cplusplus
}
#endif

#endif	/* _mmr_h_ */

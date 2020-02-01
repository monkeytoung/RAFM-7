/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ 漢字ROM ]
 */

#ifndef _kanji_h_
#define _kanji_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	主要エントリ
 */
BOOL FASTCALL kanji_init(void);
										/* 初期化 */
void FASTCALL kanji_cleanup(void);
										/* クリーンアップ */
void FASTCALL kanji_reset(void);
										/* リセット */
BOOL FASTCALL kanji_readb(WORD addr, BYTE *dat);
										/* メモリ読み出し */
BOOL FASTCALL kanji_writeb(WORD addr, BYTE dat);
										/* メモリ書き込み */
BOOL FASTCALL kanji_save(int fileh);
										/* セーブ */
BOOL FASTCALL kanji_load(int fileh, int ver);
										/* セーブ */

/*
 *	主要ワーク
 */
extern WORD kanji_addr;
										/* 第１水準 アドレス */
extern BYTE *kanji_rom;
										/* 第１水準ROM */
#ifdef __cplusplus
}
#endif

#endif	/* _kanji_h_ */

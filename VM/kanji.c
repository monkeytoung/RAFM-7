/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ 漢字ROM ]
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "xm7.h"
#include "kanji.h"
#include "device.h"

/*
 *	グローバル ワーク
 */
WORD kanji_addr;						/* 第１水準アドレス */
BYTE *kanji_rom;						/* 第１水準ROM */

/*
 *	漢字ROM
 *	初期化
 */
BOOL FASTCALL kanji_init(void)
{
	/* メモリ確保 */
	kanji_rom = (BYTE *)malloc(0x20000);
	if (!kanji_rom) {
		return FALSE;
	}

	/* ファイル読み込み */
	if (!file_load(KANJI_ROM, kanji_rom, 0x20000)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	漢字ROM
 *	クリーンアップ
 */
void FASTCALL kanji_cleanup(void)
{
	ASSERT(kanji_rom);
	if (kanji_rom) {
		free(kanji_rom);
		kanji_rom = NULL;
	}
}

/*
 *	漢字ROM
 *	リセット
 */
void FASTCALL kanji_reset(void)
{
	kanji_addr = 0;
}

/*
 *	漢字ROM
 *	１バイト読み出し
 */
BOOL FASTCALL kanji_readb(WORD addr, BYTE *dat)
{
	int offset;

	switch (addr) {
		/* アドレス上位 */
		case 0xfd20:
			*dat = (BYTE)(kanji_addr >> 8);
			return TRUE;

		/* アドレス下位 */
		case 0xfd21:
			*dat = (BYTE)(kanji_addr & 0xff);
			return TRUE;

		/* データLEFT */
		case 0xfd22:
			offset = kanji_addr << 1;
			*dat = kanji_rom[offset + 0];
			return TRUE;

		/* データRIGHT */
		case 0xfd23:
			offset = kanji_addr << 1;
			*dat = kanji_rom[offset + 1];
			return TRUE;
	}

	return FALSE;
}

/*
 *	漢字ROM
 *	１バイト書き込み
 */
BOOL FASTCALL kanji_writeb(WORD addr, BYTE dat)
{
	switch (addr) {
		/* アドレス上位 */
		case 0xfd20:
			kanji_addr &= 0x00ff;
			kanji_addr |= (WORD)(dat << 8);
			return TRUE;

		/* アドレス下位 */
		case 0xfd21:
			kanji_addr &= 0xff00;
			kanji_addr |= dat;
			return TRUE;

		/* データLEFT*/
		case 0xfd22:
			return TRUE;

		/* データRIGHT */
		case 0xfd23:
			return TRUE;
	}

	return FALSE;
}

/*
 *	漢字ROM
 *	セーブ
 */
BOOL FASTCALL kanji_save(int fileh)
{
	if (!file_word_write(fileh, kanji_addr)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	漢字ROM
 *	ロード
 */
BOOL FASTCALL kanji_load(int fileh, int ver)
{
	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_word_read(fileh, &kanji_addr)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ アナログパレット ]
 */

#include <string.h>
#include <assert.h>
#include "xm7.h"
#include "device.h"
#include "apalet.h"

/*
 *	グローバル ワーク
 */
WORD apalet_no;							/* 選択パレット番号 */
BYTE apalet_b[4096];					/* Bレベル(0-15) */
BYTE apalet_r[4096];					/* Rレベル(0-15) */
BYTE apalet_g[4096];					/* Gレベル(0-15) */

/*
 *	アナログパレット
 *	初期化
 */
BOOL FASTCALL apalet_init(void)
{
	return TRUE;
}

/*
 *	アナログパレット
 *	クリーンアップ
 */
void FASTCALL apalet_cleanup(void)
{
}

/*
 *	アナログパレット
 *	リセット
 */
void FASTCALL apalet_reset(void)
{
	apalet_no = 0;

	memset(apalet_b, 0, sizeof(apalet_b));
	memset(apalet_r, 0, sizeof(apalet_r));
	memset(apalet_g, 0, sizeof(apalet_g));
}

/*-[ メモリマップドI/O ]----------------------------------------------------*/

/*
 *	アナログパレット
 *	１バイト読み出し
 */
BOOL FASTCALL apalet_readb(WORD addr, BYTE *dat)
{
	/* バージョンチェック */
	if (fm7_ver < 2) {
		return FALSE;
	}

	/* アドレスチェック */
	if ((addr >= 0xfd30) && (addr <= 0xfd34)) {
		*dat = 0xff;
		return TRUE;
	}

	return FALSE;
}

/*
 *	アナログパレット
 *	１バイト書き込み
 */
BOOL FASTCALL apalet_writeb(WORD addr, BYTE dat)
{
	/* バージョンチェック */
	if (fm7_ver < 2) {
		return FALSE;
	}

	switch (addr) {
		/* パレット番号上位 */
		case 0xfd30:
			apalet_no &= (WORD)0xff;
			apalet_no |= (WORD)((dat & 0x0f) << 8);
			return TRUE;

		/* パレット番号下位 */
		case 0xfd31:
			apalet_no &= (WORD)(0xf00);
			apalet_no |= (WORD)(dat);
			return TRUE;

		/* Bレベル */
		case 0xfd32:
			apalet_b[apalet_no] = (BYTE)(dat & 0x0f);
			apalet_notify();
			return TRUE;

		/* Rレベル */
		case 0xfd33:
			apalet_r[apalet_no] = (BYTE)(dat & 0x0f);
			apalet_notify();
			return TRUE;

		/* Gレベル */
		case 0xfd34:
			apalet_g[apalet_no] = (BYTE)(dat & 0x0f);
			apalet_notify();
			return TRUE;
	}

	return FALSE;
}

/*-[ ファイルI/O ]----------------------------------------------------------*/

/*
 *	アナログパレット
 *	セーブ
 */
BOOL FASTCALL apalet_save(int fileh)
{
	if (!file_word_write(fileh, apalet_no)) {
		return FALSE;
	}
	if (!file_write(fileh, apalet_b, sizeof(apalet_b))) {
		return FALSE;
	}
	if (!file_write(fileh, apalet_r, sizeof(apalet_r))) {
		return FALSE;
	}
	if (!file_write(fileh, apalet_g, sizeof(apalet_g))) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	アナログパレット
 *	ロード
 */
BOOL FASTCALL apalet_load(int fileh, int ver)
{
	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_word_read(fileh, &apalet_no)) {
		return FALSE;
	}
	if (!file_read(fileh, apalet_b, sizeof(apalet_b))) {
		return FALSE;
	}
	if (!file_read(fileh, apalet_r, sizeof(apalet_r))) {
		return FALSE;
	}
	if (!file_read(fileh, apalet_g, sizeof(apalet_g))) {
		return FALSE;
	}

	return TRUE;
}

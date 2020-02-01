/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ マルチページ ]
 */

#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "multipag.h"
#include "display.h"
#include "ttlpalet.h"
#include "subctrl.h"
#include "apalet.h"
#include "device.h"

/*
 *	グローバル ワーク
 */
BYTE multi_page;						/* マルチページ ワーク */

/*
 *	マルチページ
 *	初期化
 */
BOOL FASTCALL multipag_init(void)
{
	return TRUE;
}

/*
 *	マルチページ
 *	クリーンアップ
 */
void FASTCALL multipag_cleanup(void)
{
}

/*
 *	マルチページ
 *	リセット
 */
void FASTCALL multipag_reset(void)
{
	multi_page = 0;
}

/*
 *	マルチページ
 *	１バイト読み出し
 */
BOOL FASTCALL multipag_readb(WORD addr, BYTE *dat)
{
	if (addr != 0xfd37) {
		return FALSE;
	}

	/* 常にFFが読み出される */
	*dat = 0xff;
	return TRUE;
}

/*
 *	マルチページ
 *	１バイト書き込み
 */
BOOL FASTCALL multipag_writeb(WORD addr, BYTE dat)
{
	if (addr != 0xfd37) {
		return FALSE;
	}

	/* データ記憶 */
	multi_page = dat;

	/* パレット再設定 */
	ttlpalet_notify();
	apalet_notify();

	return TRUE;
}

/*
 *	マルチページ
 *	セーブ
 */
BOOL FASTCALL multipag_save(int fileh)
{
	return file_byte_write(fileh, multi_page);
}

/*
 *	マルチページ
 *	ロード
 */
BOOL FASTCALL multipag_load(int fileh, int ver)
{
	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	return file_byte_read(fileh, &multi_page);
}

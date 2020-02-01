/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ TTLパレット ]
 */

#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "ttlpalet.h"
#include "device.h"

/*
 *	グローバル ワーク
 */
BYTE ttl_palet[8];			/* TTLパレットデータ */

/*
 *	TTLパレット
 *	初期化
 */
BOOL FASTCALL ttlpalet_init(void)
{
	return TRUE;
}

/*
 *	TTLパレット
 *	クリーンアップ
 */
void FASTCALL ttlpalet_cleanup(void)
{
}

/*
 *	TTLパレット
 *	リセット
 */
void FASTCALL ttlpalet_reset(void)
{
	int i;
	
	/* すべての色を初期化 */
	for (i=0; i<8; i++) {
		ttl_palet[i] = (BYTE)i;
	}

	/* 通知 */
	ttlpalet_notify();
}

/*
 *	TTLパレット
 *	１バイト読み出し
 */
BOOL FASTCALL ttlpalet_readb(WORD addr, BYTE *dat)
{
	/* 範囲チェック、読み出し */
	if ((addr >= 0xfd38) && (addr <= 0xfd3f)) {
		ASSERT((WORD)(addr - 0xfd38) <= 7);

		/* 上位ニブルは0xF0が入る */
		*dat = (BYTE)(ttl_palet[(WORD)(addr - 0xfd38)] | 0xf0);
		return TRUE;
	}

	return FALSE;
}

/*
 *	TTLパレット
 *	１バイト書き込み
 */
BOOL FASTCALL ttlpalet_writeb(WORD addr, BYTE dat)
{
	int no;

	/* 範囲チェック、書き込み */
	if ((addr >= 0xfd38) && (addr <= 0xfd3f)) {
		no = addr - 0xfd38;
		ttl_palet[no] = (BYTE)(dat & 0x07);

		/* 通知 */
		ttlpalet_notify();
		return TRUE;
	}

	return FALSE;
}

/*
 *	TTLパレット
 *	セーブ
 */
BOOL FASTCALL ttlpalet_save(int fileh)
{
	return file_write(fileh, ttl_palet, 8);
}

/*
 *	TTLパレット
 *	ロード
 */
BOOL FASTCALL ttlpalet_load(int fileh, int ver)
{
	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	return file_read(fileh, ttl_palet, 8);
}

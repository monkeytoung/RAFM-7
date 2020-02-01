/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ MMR ]
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "xm7.h"
#include "device.h"
#include "mmr.h"

/*
 *	グローバル ワーク
 */
BOOL mmr_flag;							/* MMR有効フラグ */
BYTE mmr_seg;							/* MMRセグメント */
BYTE mmr_reg[0x40];						/* MMRレジスタ */
BOOL twr_flag;							/* TWR有効フラグ */
BYTE twr_reg;							/* TWRレジスタ */

/*
 *	MMR
 *	初期化
 */
BOOL FASTCALL mmr_init(void)
{
	return TRUE;
}

/*
 *	MMR
 *	クリーンアップ
 */
void FASTCALL mmr_cleanup(void)
{
}

/*
 *	MMR
 *	リセット
 */
void FASTCALL mmr_reset(void)
{
	mmr_flag = FALSE;
	twr_flag = FALSE;
	memset(mmr_reg, 0, sizeof(mmr_reg));
	twr_reg = 0;
}

/*-[ メモリマネージャ ]-----------------------------------------------------*/

/*
 *	TWRアドレス変換
 */
static BOOL FASTCALL mmr_trans_twr(WORD addr, DWORD *taddr)
{
	ASSERT(fm7_ver >= 2);

	/* TWR有効か */
	if (!twr_flag) {
		return FALSE;
	}

	/* アドレス要件チェック */
	if ((addr < 0x7c00) || (addr > 0x7fff)) {
		return FALSE;
	}

	/* TWRレジスタより変換 */
	*taddr = (DWORD)twr_reg;
	*taddr *= 256;
	*taddr += addr;
	*taddr &= 0xffff;

	return TRUE;
}

/*
 *	MMRアドレス変換
 */
static DWORD FASTCALL mmr_trans_mmr(WORD addr)
{
	DWORD maddr;
	int offset;

	ASSERT(fm7_ver >= 2);

	/* MMR有効か */
	if (!mmr_flag) {
		return (DWORD)(0x30000 | addr);
	}

	/* MMRレジスタより取得 */
	offset = (int)addr;
	offset >>= 12;
	offset |= (mmr_seg * 0x10);
	maddr = (DWORD)mmr_reg[offset];
	maddr <<= 12;

	/* 下位12ビットと合成 */
	addr &= 0xfff;
	maddr |= addr;

	return maddr;
}

/*
 *	メインCPUバス
 *	１バイト読み出し
 */
BOOL FASTCALL mmr_extrb(WORD *addr, BYTE *dat)
{
	DWORD raddr;

	ASSERT(fm7_ver >= 2);

	/* $FC00～$FFFFは常駐空間 */
	if (*addr >= 0xfc00) {
		return FALSE;
	}

	/* TWR,MMRを通す */
	if (!mmr_trans_twr(*addr, &raddr)) {
		raddr = mmr_trans_mmr(*addr);
	}

	/* FM77AV 拡張RAM */
	if (raddr < 0x10000) {
		*dat = extram_a[raddr & 0xffff];
		return TRUE;
	}

	/* サブシステム */
	if (raddr < 0x20000) {
		*dat = submem_readb((WORD)(raddr & 0xffff));
		return TRUE;
	}

	/* リザーブ */
	if (raddr < 0x30000) {
		*dat = 0xff;
		return TRUE;
	}

	/* リザーブ */
	if (raddr >= 0x40000) {
		*dat = 0xff;
		return TRUE;
	}

	/* $30セグメント */
	*addr = (WORD)(raddr & 0xffff);
	return FALSE;
}

/*
 *	メインCPUバス
 *	１バイト読み出し(I/Oなし)
 */
BOOL FASTCALL mmr_extbnio(WORD *addr, BYTE *dat)
{
	DWORD raddr;

	ASSERT(fm7_ver >= 2);

	/* $FC00～$FFFFは常駐空間 */
	if (*addr >= 0xfc00) {
		return FALSE;
	}

	/* TWR,MMRを通す */
	if (!mmr_trans_twr(*addr, &raddr)) {
		raddr = mmr_trans_mmr(*addr);
	}

	/* FM77AV 拡張RAM */
	if (raddr < 0x10000) {
		*dat = extram_a[raddr & 0xffff];
		return TRUE;
	}

	/* サブシステム */
	if (raddr < 0x20000) {
		*dat = submem_readbnio((WORD)(raddr & 0xffff));
		return TRUE;
	}

	/* リザーブ */
	if (raddr < 0x30000) {
		*dat = 0xff;
		return TRUE;
	}

	/* リザーブ */
	if (raddr >= 0x40000) {
		*dat = 0xff;
		return TRUE;
	}

	/* $30セグメント */
	*addr = (WORD)(raddr & 0xffff);
	return FALSE;
}

/*
 *	メインCPUバス
 *	１バイト書き込み
 */
BOOL FASTCALL mmr_extwb(WORD *addr, BYTE dat)
{
	DWORD raddr;

	ASSERT(fm7_ver >= 2);

	/* $FC00～$FFFFは常駐空間 */
	if (*addr >= 0xfc00) {
		return FALSE;
	}

	/* TWR,MMRを通す */
	if (!mmr_trans_twr(*addr, &raddr)) {
		raddr = mmr_trans_mmr(*addr);
	}

	/* FM77AV 拡張RAM */
	if (raddr < 0x10000) {
		extram_a[raddr & 0xffff] = dat;
		return TRUE;
	}

	/* サブシステム */
	if (raddr < 0x20000) {
		submem_writeb((WORD)(raddr & 0xffff), dat);
		return TRUE;
	}

	/* リザーブ */
	if (raddr < 0x30000) {
		return TRUE;
	}

	/* リザーブ */
	if (raddr >= 0x40000) {
		return TRUE;
	}

	/* $30セグメント */
	*addr = (WORD)(raddr & 0xffff);
	return FALSE;
}

/*-[ メモリマップドI/O ]----------------------------------------------------*/

/*
 *	MMR
 *	１バイト読み出し
 */
BOOL FASTCALL mmr_readb(WORD addr, BYTE *dat)
{
	BYTE tmp;

	/* バージョンチェック */
	if (fm7_ver < 2) {
		return FALSE;
	}

	switch (addr) {
		/* ブートステータス */
		case 0xfd0b:
			if (boot_mode == BOOT_BASIC) {
				*dat = 0xfe;
			}
			else {
				*dat = 0xff;
			}
			return TRUE;

		/* イニシエータROM */
		case 0xfd10:
			*dat = 0xff;
			return TRUE;

		/* MMRセグメント */
		case 0xfd90:
			*dat = 0xff;
			return TRUE;

		/* TWRオフセット */
		case 0xfd92:
			*dat = 0xff;
			return TRUE;

		/* モードセレクト */
		case 0xfd93:
			tmp = 0xff;
			if (!mmr_flag) {
				tmp &= (BYTE)(~0x80);
			}
			if (!twr_flag) {
				tmp &= ~0x40;
			}
			if (!bootram_rw) {
				tmp &= ~1;
			}
			*dat = tmp;
			return TRUE;
	}

	/* MMRレジスタ */
	if ((addr >= 0xfd80) && (addr <= 0xfd8f)) {
		*dat = mmr_reg[mmr_seg * 0x10 + (addr - 0xfd80)];
		return TRUE;
	}

	return FALSE;
}

/*
 *	MMR
 *	１バイト書き込み
 */
BOOL FASTCALL mmr_writeb(WORD addr, BYTE dat)
{
	/* バージョンチェック */
	if (fm7_ver < 2) {
		return FALSE;
	}

	switch (addr) {
		/* ブートステータス */
		case 0xfd0b:
			return TRUE;

		/* イニシエータROM */
		case 0xfd10:
			if (dat & 0x02) {
				initrom_en = FALSE;
			}
			else {
				initrom_en = TRUE;
			}
			return TRUE;

		/* MMRセグメント */
		case 0xfd90:
			mmr_seg = (BYTE)(dat & 0x03);
			return TRUE;

		/* TWRオフセット */
		case 0xfd92:
			twr_reg = dat;
			return TRUE;

		/* モードセレクト */
		case 0xfd93:
			if (dat & 0x80) {
				mmr_flag = TRUE;
			}
			else {
				mmr_flag = FALSE;
			}
			if (dat & 0x40) {
				twr_flag = TRUE;
			}
			else {
				twr_flag = FALSE;
			}
			if (dat & 0x01) {
				bootram_rw = TRUE;
			}
			else {
				bootram_rw = FALSE;
			}
			return TRUE;
	}

	/* MMRレジスタ */
	if ((addr >= 0xfd80) && (addr <= 0xfd8f)) {
		/* データは$00～$3Fに制限(Seles対策) */
		mmr_reg[mmr_seg * 0x10 + (addr - 0xfd80)] = (BYTE)(dat & 0x3f);
		return TRUE;
	}

	return FALSE;
}

/*-[ ファイルI/O ]----------------------------------------------------------*/

/*
 *	MMR
 *	セーブ
 */
BOOL FASTCALL mmr_save(int fileh)
{
	if (!file_bool_write(fileh, mmr_flag)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, mmr_seg)) {
		return FALSE;
	}
	if (!file_write(fileh, mmr_reg, 0x40)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, twr_flag)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, twr_reg)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	MMR
 *	ロード
 */
BOOL FASTCALL mmr_load(int fileh, int ver)
{
	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &mmr_flag)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &mmr_seg)) {
		return FALSE;
	}
	if (!file_read(fileh, mmr_reg, 0x40)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &twr_flag)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &twr_reg)) {
		return FALSE;
	}

	return TRUE;
}

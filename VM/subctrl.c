/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ サブCPUコントロール ]
 */

#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "keyboard.h"
#include "subctrl.h"
#include "device.h"
#include "display.h"

/*
 *	グローバル ワーク
 */
BOOL subhalt_flag;						/* サブHALTフラグ */
BOOL subbusy_flag;						/* サブBUSYフラグ */
BOOL subcancel_flag;					/* サブキャンセルフラグ */
BOOL subattn_flag;						/* サブアテンションフラグ */
BYTE shared_ram[0x80];					/* 共有RAM */
BOOL subreset_flag;						/* サブ再起動フラグ */
BOOL mode320;							/* 320x200モード */

/*
 *	サブCPUコントロール
 *	初期化
 */
BOOL FASTCALL subctrl_init(void)
{
	return TRUE;
}

/*
 *	サブCPUコントロール
 *	クリーンアップ
 */
void FASTCALL subctrl_cleanup(void)
{
}

/*
 *	サブCPUコントロール
 *	リセット
 */
void FASTCALL subctrl_reset(void)
{
	subhalt_flag = FALSE;
	subbusy_flag = TRUE;
	subcancel_flag = FALSE;
	subattn_flag = FALSE;
	subreset_flag = FALSE;
	mode320 = FALSE;

	memset(shared_ram, 0xff, sizeof(shared_ram));
}

/*
 *	サブCPUコントロール
 *	１バイト読み出し
 */
BOOL FASTCALL subctrl_readb(WORD addr, BYTE *dat)
{
	BYTE ret;

	switch (addr) {
		/* サブCPU アテンション割り込み、Breakキー割り込み */
		case 0xfd04:
			ret = 0xff;
			/* アテンションフラグ */
			if (subattn_flag) {
				ret &= ~0x01;
				subattn_flag = FALSE;
			}
			/* Breakキーフラグ */
			if (break_flag) {
				ret &= ~0x02;
			}
			*dat = ret;
			maincpu_firq();
			return TRUE;

		/* サブインタフェース */
		case 0xfd05:
			if (subbusy_flag) {
				*dat = 0xfe;
				return TRUE;
			}
			else {
				*dat = 0x7e;
				return TRUE;
			}

		/* サブモードステータス */
		case 0xfd12:
			ret = 0xff;
			if (fm7_ver >= 2) {
				/* 320/640 */
				if (!mode320) {
					ret &= ~0x40;
				}

				/* ブランクステータス */
				if (blank_flag) {
					ret &= ~0x02;
				}
				/* VSYNCステータス */
				if (!vsync_flag) {
					ret &= ~0x01;
				}
			}
			*dat = ret;
			return TRUE;
	}

	return FALSE;
}

/*
 *	サブCPUコントロール
 *	１バイト書き込み
 */
BOOL FASTCALL subctrl_writeb(WORD addr, BYTE dat)
{
	switch (addr) {
		/* サブコントロール */
		case 0xfd05:
			if (dat & 0x80) {
				/* サブHALT */
				subhalt_flag = TRUE;
				subbusy_flag = TRUE;
			}
			else {
				/* サブRUN */
				subhalt_flag = FALSE;
			}
			if (dat & 0x40) {
				/* キャンセルIRQ */
				subcancel_flag = TRUE;
			}
			subcpu_irq();
			return TRUE;

		/* サブモード切り替え */
		case 0xfd12:
			if (fm7_ver >= 2) {
				if (dat & 0x40) {
					mode320 = TRUE;
				}
				else {
					mode320 = FALSE;
				}
			}
			return TRUE;

		/* サブバンク切り替え */
		case 0xfd13:
			if (fm7_ver >= 2) {
				/* バンク切り替え */
				subrom_bank = (BYTE)(dat & 0x03);
				if (subrom_bank == 3) {
					subrom_bank = 0;
					ASSERT(FALSE);
				}

				/* リセット */
				subcpu_reset();

				/* フラグ類セット */
				subreset_flag = TRUE;
				subbusy_flag = TRUE;
			}
			return TRUE;
	}

	return FALSE;
}

/*
 *	サブCPUコントロール
 *	セーブ
 */
BOOL FASTCALL subctrl_save(int fileh)
{
	if (!file_bool_write(fileh, subhalt_flag)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, subbusy_flag)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, subcancel_flag)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, subattn_flag)) {
		return FALSE;
	}

	if (!file_write(fileh, shared_ram, 0x80)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, subreset_flag)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, mode320)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	サブCPUコントロール
 *	ロード
 */
BOOL FASTCALL subctrl_load(int fileh, int ver)
{
	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &subhalt_flag)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &subbusy_flag)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &subcancel_flag)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &subattn_flag)) {
		return FALSE;
	}

	if (!file_read(fileh, shared_ram, 0x80)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &subreset_flag)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &mode320)) {
		return FALSE;
	}

	return TRUE;
}

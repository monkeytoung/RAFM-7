/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ システム管理 ]
 */

#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "display.h"
#include "ttlpalet.h"
#include "subctrl.h"
#include "keyboard.h"
#include "fdc.h"
#include "mainetc.h"
#include "multipag.h"
#include "kanji.h"
#include "tapelp.h"
#include "display.h"
#include "opn.h"
#include "mmr.h"
#include "aluline.h"
#include "apalet.h"
#include "rtc.h"
#include "whg.h"
#include "device.h"

/*
 *	グローバル ワーク
 */
int fm7_ver;							/* ハードウェアバージョン */
int boot_mode;							/* 起動モード BASIC/DOS */

/*
 *	システム
 *	初期化
 */
BOOL FASTCALL system_init(void)
{
	/* モード設定 */
	fm7_ver = 2;					/* FM77AV相当に設定 */
	boot_mode = BOOT_BASIC;			/* BASIC MODE */

	/* スケジューラ、メモリバス */
	if (!schedule_init()) {
		return FALSE;
	}
	if (!mmr_init()) {
		return FALSE;
	}

	/* メモリ、CPU */
	if (!mainmem_init()) {
		return FALSE;
	}
	if (!submem_init()) {
		return FALSE;
	}
	if (!maincpu_init()) {
		return FALSE;
	}
	if (!subcpu_init()) {
		return FALSE;
	}

	/* その他デバイス */
	if (!display_init()) {
		return FALSE;
	}
	if (!ttlpalet_init()) {
		return FALSE;
	}
	if (!subctrl_init()) {
		return FALSE;
	}
	if (!keyboard_init()) {
		return FALSE;
	}
	if (!fdc_init()) {
		return FALSE;
	}
	if (!mainetc_init()) {
		return FALSE;
	}
	if (!multipag_init()) {
		return FALSE;
	}
	if (!kanji_init()) {
		return FALSE;
	}
	if (!tapelp_init()) {
		return FALSE;
	}
	if (!opn_init()) {
		return FALSE;
	}
	if (!aluline_init()) {
		return FALSE;
	}
	if (!apalet_init()) {
		return FALSE;
	}
	if (!rtc_init()) {
		return FALSE;
	}
	if (!whg_init()) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	システム
 *	クリーンアップ
 */
void FASTCALL system_cleanup(void)
{
	/* その他デバイス */
	whg_cleanup();
	rtc_cleanup();
	apalet_cleanup();
	aluline_cleanup();
	opn_cleanup();
	tapelp_cleanup();
	kanji_cleanup();
	multipag_cleanup();
	mainetc_cleanup();
	fdc_cleanup();
	keyboard_cleanup();
	subctrl_cleanup();
	ttlpalet_cleanup();
	display_cleanup();

	/* メモリ、CPU */
	subcpu_cleanup();
	maincpu_cleanup();
	submem_cleanup();
	mainmem_cleanup();

	/* スケジューラ、メモリバス */
	mmr_cleanup();
	schedule_cleanup();
}

/*
 *	システム
 *	リセット
 */
void FASTCALL system_reset(void)
{
	/* スケジューラ、メモリバス */
	schedule_reset();
	mmr_reset();

	/* その他デバイス */
	display_reset();
	ttlpalet_reset();
	subctrl_reset();
	keyboard_reset();
	fdc_reset();
	mainetc_reset();
	multipag_reset();
	kanji_reset();
	tapelp_reset();
	opn_reset();
	aluline_reset();
	apalet_reset();
	rtc_reset();
	whg_reset();

	/* メモリ、CPU */
	mainmem_reset();
	submem_reset();
	maincpu_reset();
	subcpu_reset();

	/* 画面再描画 */
	display_notify();
}

/*
 *	システム
 *	ファイルセーブ
 */
BOOL FASTCALL system_save(char *filename)
{
	int fileh;
	char *header = "XM7 VM STATE   5";
	BOOL flag;
	ASSERT(filename);

	/* ファイルオープン */
	fileh = file_open(filename, OPEN_W);
	if (fileh == -1) {
		return FALSE;
	}

	/* フラグ初期化 */
	flag = TRUE;

	/* ヘッダをセーブ */
	if (!file_write(fileh, (BYTE *)header, 16)) {
		flag = FALSE;
	}

	/* システムワーク */
	if (!file_word_write(fileh, (WORD)fm7_ver)) {
		return FALSE;
	}
	if (!file_word_write(fileh, (WORD)boot_mode)) {
		return FALSE;
	}

	/* 順番に呼び出す */
	if (!mainmem_save(fileh)) {
		flag = FALSE;
	}
	if (!submem_save(fileh)) {
		flag = FALSE;
	}
	if (!maincpu_save(fileh)) {
		flag = FALSE;
	}
	if (!subcpu_save(fileh)) {
		flag = FALSE;
	}
	if (!schedule_save(fileh)) {
		flag = FALSE;
	}
	if (!display_save(fileh)) {
		flag = FALSE;
	}
	if (!ttlpalet_save(fileh)) {
		flag = FALSE;
	}
	if (!subctrl_save(fileh)) {
		flag = FALSE;
	}
	if (!keyboard_save(fileh)) {
		flag = FALSE;
	}
	if (!fdc_save(fileh)) {
		flag = FALSE;
	}
	if (!mainetc_save(fileh)) {
		flag = FALSE;
	}
	if (!multipag_save(fileh)) {
		flag = FALSE;
	}
	if (!kanji_save(fileh)) {
		flag = FALSE;
	}
	if (!tapelp_save(fileh)) {
		flag = FALSE;
	}
	if (!opn_save(fileh)) {
		flag = FALSE;
	}
	if (!mmr_save(fileh)) {
		flag = FALSE;
	}
	if (!aluline_save(fileh)) {
		flag = FALSE;
	}
	if (!rtc_save(fileh)) {
		flag = FALSE;
	}
	if (!apalet_save(fileh)) {
		flag = FALSE;
	}
	if (!whg_save(fileh)) {
		flag = FALSE;
	}

	file_close(fileh);
	return flag;
}

/*
 *	システム
 *	ファイルロード
 */
BOOL FASTCALL system_load(char *filename)
{
	int fileh;
	int ver;
	char header[16];
	BOOL flag;
	ASSERT(filename);

	/* ファイルオープン */
	fileh = file_open(filename, OPEN_R);
	if (fileh == -1) {
		return FALSE;
	}

	/* フラグ初期化 */
	flag = TRUE;

	/* ヘッダをロード */
	if (!file_read(fileh, (BYTE *)header, 16)) {
		flag = FALSE;
	}
	else {
		if (memcmp(header, "XM7 VM STATE   ", 15) != 0) {
			flag = FALSE;
		}
	}

	/* ヘッダチェック */
	if (!flag) {
		file_close(fileh);
		return FALSE;
	}

	/* ファイルバージョン取得、バージョン2以上が対象 */
	ver = (int)(BYTE)(header[15]);
	ver -= 0x30;
	if (ver < 2) {
		return FALSE;
	}

	/* システムワーク */
	if (!file_word_read(fileh, (WORD *)&fm7_ver)) {
		return FALSE;
	}
	if (!file_word_read(fileh, (WORD *)&boot_mode)) {
		return FALSE;
	}

	/* 順番に呼び出す */
	if (!mainmem_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!submem_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!maincpu_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!subcpu_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!schedule_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!display_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!ttlpalet_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!subctrl_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!keyboard_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!fdc_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!mainetc_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!multipag_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!kanji_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!tapelp_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!opn_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!mmr_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!aluline_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!rtc_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!apalet_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!whg_load(fileh, ver)) {
		flag = FALSE;
	}
	file_close(fileh);

	/* 画面再描画 */
	display_notify();

	return flag;
}

/*
 *	ファイル読み込み(BYTE)
 */
BOOL FASTCALL file_byte_read(int fileh, BYTE *dat)
{
	return file_read(fileh, dat, 1);
}

/*
 *	ファイル読み込み(WORD)
 */
BOOL FASTCALL file_word_read(int fileh, WORD *dat)
{
	BYTE tmp;

	if (!file_read(fileh, &tmp, 1)) {
		return FALSE;
	}
	*dat = tmp;

	if (!file_read(fileh, &tmp, 1)) {
		return FALSE;
	}
	*dat <<= 8;
	*dat |= tmp;

	return TRUE;
}

/*
 *	ファイル読み込み(DWORD)
 */
BOOL FASTCALL file_dword_read(int fileh, DWORD *dat)
{
	BYTE tmp;

	if (!file_read(fileh, &tmp, 1)) {
		return FALSE;
	}
	*dat = tmp;

	if (!file_read(fileh, &tmp, 1)) {
		return FALSE;
	}
	*dat *= 256;
	*dat |= tmp;

	if (!file_read(fileh, &tmp, 1)) {
		return FALSE;
	}
	*dat *= 256;
	*dat |= tmp;

	if (!file_read(fileh, &tmp, 1)) {
		return FALSE;
	}
	*dat *= 256;
	*dat |= tmp;

	return TRUE;
}

/*
 *	ファイル読み込み(BOOL)
 */
BOOL FASTCALL file_bool_read(int fileh, BOOL *dat)
{
	BYTE tmp;

	if (!file_read(fileh, &tmp, 1)) {
		return FALSE;
	}

	switch (tmp) {
		case 0:
			*dat = FALSE;
			return TRUE;
		case 0xff:
			*dat = TRUE;
			return TRUE;
	}

	return FALSE;
}

/*
 *	ファイル書き込み(BYTE)
 */
BOOL FASTCALL file_byte_write(int fileh, BYTE dat)
{
	return file_write(fileh, &dat, 1);
}

/*
 *	ファイル書き込み(WORD)
 */
BOOL FASTCALL file_word_write(int fileh, WORD dat)
{
	BYTE tmp;

	tmp = (BYTE)(dat >> 8);
	if (!file_write(fileh, &tmp, 1)) {
		return FALSE;
	}

	tmp = (BYTE)(dat & 0xff);
	if (!file_write(fileh, &tmp, 1)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	ファイル書き込み(DWORD)
 */
BOOL FASTCALL file_dword_write(int fileh, DWORD dat)
{
	BYTE tmp;

	tmp = (BYTE)(dat >> 24);
	if (!file_write(fileh, &tmp, 1)) {
		return FALSE;
	}

	tmp = (BYTE)(dat >> 16);
	if (!file_write(fileh, &tmp, 1)) {
		return FALSE;
	}

	tmp = (BYTE)(dat >> 8);
	if (!file_write(fileh, &tmp, 1)) {
		return FALSE;
	}

	tmp = (BYTE)(dat & 0xff);
	if (!file_write(fileh, &tmp, 1)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	ファイル書き込み(BOOL)
 */
BOOL FASTCALL file_bool_write(int fileh, BOOL dat)
{
	BYTE tmp;

	if (dat) {
		tmp = 0xff;
	}
	else {
		tmp = 0;
	}

	if (!file_write(fileh, &tmp, 1)) {
		return FALSE;
	}

	return TRUE;
}

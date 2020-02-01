/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ メインCPU ]
 */

#include <assert.h>
#include "xm7.h"
#include "subctrl.h"
#include "keyboard.h"
#include "mainetc.h"
#include "tapelp.h"
#include "device.h"

/*
 *	グローバル ワーク
 */
cpu6809_t maincpu;

/*
 *	プロトタイプ宣言
 */
void main_reset(void);
void main_line(void);
void main_exec(void);

/*
 *	メインCPU
 *	初期化
 */
BOOL FASTCALL maincpu_init(void)
{
	maincpu.readmem = mainmem_readb;
	maincpu.writemem = mainmem_writeb;

	return TRUE;
}

/*
 *	メインCPU
 *	クリーンアップ
 */
void FASTCALL maincpu_cleanup(void)
{
	return;
}

/*
 *	メインCPU
 *	リセット
 */
void FASTCALL maincpu_reset(void)
{
	main_reset();
}

/*
 *	メインCPU
 *	１行実行
 */
void FASTCALL maincpu_execline(void)
{
	main_line();

	/* テープカウンタ処理 */
	if (tape_motor) {
		tape_subcnt += (BYTE)maincpu.cycle;
		if (tape_subcnt >= 0x10) {
			tape_subcnt -= (BYTE)0x10;
			tape_count++;
			if (tape_count == 0) {
				tape_count = 0xffff;
			}
		}
	}
}

/*
 *	メインCPU
 *	実行
 */
void FASTCALL maincpu_exec(void)
{
	/* 実行 */
	main_exec();

	/* テープカウンタ処理 */
	if (tape_motor) {
		tape_subcnt += (BYTE)maincpu.cycle;
		if (tape_subcnt >= 0x10) {
			tape_subcnt -= (BYTE)0x10;
			tape_count++;
			if (tape_count == 0) {
				tape_count = 0xffff;
			}
		}
	}
}

/*
 *	メインCPU
 *	FIRQ割り込み設定
 */
void FASTCALL maincpu_firq(void)
{
	/* BREAKキー及び、サブCPUからのアテンション割り込み */
	if (break_flag || subattn_flag) {
		maincpu.intr |= INTR_FIRQ;
	}
	else {
		maincpu.intr &= ~INTR_FIRQ;
	}
}

/*
 *	メインCPU
 *	IRQ割り込み設定
 */
void FASTCALL maincpu_irq(void)
{
	/* IRQ割り込み設定 */
	if ((key_irq_flag && !(key_irq_mask)) ||
		timer_irq_flag ||
		lp_irq_flag ||
		mfd_irq_flag ||
		txrdy_irq_flag ||
		rxrdy_irq_flag ||
		syndet_irq_flag ||
		opn_irq_flag ||
		whg_irq_flag) {
		maincpu.intr |= INTR_IRQ;
	}
	else {
		maincpu.intr &= ~INTR_IRQ;
	}
}

/*
 *	メインCPU
 *	セーブ
 */
BOOL FASTCALL maincpu_save(int fileh)
{
	/* プラットフォームごとのパッキング差を回避するため、分割 */
	if (!file_byte_write(fileh, maincpu.cc)) {
		return FALSE;
	}

	if (!file_byte_write(fileh, maincpu.dp)) {
		return FALSE;
	}

	if (!file_word_write(fileh, maincpu.acc.d)) {
		return FALSE;
	}

	if (!file_word_write(fileh, maincpu.x)) {
		return FALSE;
	}

	if (!file_word_write(fileh, maincpu.y)) {
		return FALSE;
	}

	if (!file_word_write(fileh, maincpu.u)) {
		return FALSE;
	}

	if (!file_word_write(fileh, maincpu.s)) {
		return FALSE;
	}

	if (!file_word_write(fileh, maincpu.pc)) {
		return FALSE;
	}

	if (!file_word_write(fileh, maincpu.intr)) {
		return FALSE;
	}

	if (!file_word_write(fileh, maincpu.cycle)) {
		return FALSE;
	}

	if (!file_word_write(fileh, maincpu.total)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	メインCPU
 *	ロード
 */
BOOL FASTCALL maincpu_load(int fileh, int ver)
{
	/* バージョンチェック */
	if (ver < 2) {
		return FALSE;
	}

	/* プラットフォームごとのパッキング差を回避するため、分割 */
	if (!file_byte_read(fileh, &maincpu.cc)) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &maincpu.dp)) {
		return FALSE;
	}

	if (!file_word_read(fileh, &maincpu.acc.d)) {
		return FALSE;
	}

	if (!file_word_read(fileh, &maincpu.x)) {
		return FALSE;
	}

	if (!file_word_read(fileh, &maincpu.y)) {
		return FALSE;
	}

	if (!file_word_read(fileh, &maincpu.u)) {
		return FALSE;
	}

	if (!file_word_read(fileh, &maincpu.s)) {
		return FALSE;
	}

	if (!file_word_read(fileh, &maincpu.pc)) {
		return FALSE;
	}

	if (!file_word_read(fileh, &maincpu.intr)) {
		return FALSE;
	}

	if (!file_word_read(fileh, &maincpu.cycle)) {
		return FALSE;
	}

	if (!file_word_read(fileh, &maincpu.total)) {
		return FALSE;
	}

	return TRUE;
}

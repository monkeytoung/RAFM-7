/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ ���C��CPU ]
 */

#include <assert.h>
#include "xm7.h"
#include "subctrl.h"
#include "keyboard.h"
#include "mainetc.h"
#include "tapelp.h"
#include "device.h"

/*
 *	�O���[�o�� ���[�N
 */
cpu6809_t maincpu;

/*
 *	�v���g�^�C�v�錾
 */
void main_reset(void);
void main_line(void);
void main_exec(void);

/*
 *	���C��CPU
 *	������
 */
BOOL FASTCALL maincpu_init(void)
{
	maincpu.readmem = mainmem_readb;
	maincpu.writemem = mainmem_writeb;

	return TRUE;
}

/*
 *	���C��CPU
 *	�N���[���A�b�v
 */
void FASTCALL maincpu_cleanup(void)
{
	return;
}

/*
 *	���C��CPU
 *	���Z�b�g
 */
void FASTCALL maincpu_reset(void)
{
	main_reset();
}

/*
 *	���C��CPU
 *	�P�s���s
 */
void FASTCALL maincpu_execline(void)
{
	main_line();

	/* �e�[�v�J�E���^���� */
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
 *	���C��CPU
 *	���s
 */
void FASTCALL maincpu_exec(void)
{
	/* ���s */
	main_exec();

	/* �e�[�v�J�E���^���� */
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
 *	���C��CPU
 *	FIRQ���荞�ݐݒ�
 */
void FASTCALL maincpu_firq(void)
{
	/* BREAK�L�[�y�сA�T�uCPU����̃A�e���V�������荞�� */
	if (break_flag || subattn_flag) {
		maincpu.intr |= INTR_FIRQ;
	}
	else {
		maincpu.intr &= ~INTR_FIRQ;
	}
}

/*
 *	���C��CPU
 *	IRQ���荞�ݐݒ�
 */
void FASTCALL maincpu_irq(void)
{
	/* IRQ���荞�ݐݒ� */
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
 *	���C��CPU
 *	�Z�[�u
 */
BOOL FASTCALL maincpu_save(int fileh)
{
	/* �v���b�g�t�H�[�����Ƃ̃p�b�L���O����������邽�߁A���� */
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
 *	���C��CPU
 *	���[�h
 */
BOOL FASTCALL maincpu_load(int fileh, int ver)
{
	/* �o�[�W�����`�F�b�N */
	if (ver < 2) {
		return FALSE;
	}

	/* �v���b�g�t�H�[�����Ƃ̃p�b�L���O����������邽�߁A���� */
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

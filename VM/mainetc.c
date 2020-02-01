/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ ���C��CPU�e��I/O ]
 */

#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "mainetc.h"
#include "keyboard.h"
#include "opn.h"
#include "device.h"
#include "event.h"

/*
 *	�O���[�o�� ���[�N
 */
BOOL key_irq_flag;					/* �L�[�{�[�h���荞�� �v�� */
BOOL key_irq_mask;					/* �L�[�{�[�h���荞�� �}�X�N */
BOOL lp_irq_flag;					/* �v�����^���荞�� �v�� */
BOOL lp_irq_mask;					/* �v�����^���荞�� �}�X�N */
BOOL timer_irq_flag;				/* �^�C�}�[���荞�� �v�� */
BOOL timer_irq_mask;				/* �^�C�}�[���荞�� �}�X�N */

BOOL mfd_irq_flag;					/* FDC���荞�� �t���O */
BOOL mfd_irq_mask;					/* FDC���荞�� �}�X�N */
BOOL txrdy_irq_flag;				/* TxRDY���荞�� �t���O */
BOOL txrdy_irq_mask;				/* TxRDY���荞�� �}�X�N */
BOOL rxrdy_irq_flag;				/* RxRDY���荞�� �t���O */
BOOL rxrdy_irq_mask;				/* RxRDY���荞�� �}�X�N */
BOOL syndet_irq_flag;				/* SYNDET���荞�� �t���O */
BOOL syndet_irq_mask;				/* SYNDET���荞�� �}�X�N */
BOOL opn_irq_flag;					/* OPN���荞�� �t���O */
BOOL whg_irq_flag;					/* WHG���荞�� �t���O */

BOOL beep_flag;						/* BEEP�t���O */
BOOL speaker_flag;					/* �X�s�[�J�t���O */

/*
 *	�v���g�^�C�v�錾
 */
BOOL FASTCALL mainetc_event(void);	/* 2.03ms �^�C�}�[�C�x���g */

/*
 *	���C��CPU I/O
 *	������
 */
BOOL FASTCALL mainetc_init(void)
{
	return TRUE;
}

/*
 *	���C��CPU I/O
 *	�N���[���A�b�v
 */
void FASTCALL mainetc_cleanup(void)
{
}

/*
 *	���C��CPU I/O
 *	���Z�b�g
 */
void FASTCALL mainetc_reset(void)
{
	key_irq_flag = FALSE;
	key_irq_mask = TRUE;
	lp_irq_flag = FALSE;
	lp_irq_mask = TRUE;
	timer_irq_flag = FALSE;
	timer_irq_mask = TRUE;

	mfd_irq_flag = FALSE;
	mfd_irq_mask = TRUE;
	txrdy_irq_flag = FALSE;
	txrdy_irq_mask = TRUE;
	rxrdy_irq_flag = FALSE;
	rxrdy_irq_mask = TRUE;
	syndet_irq_flag = FALSE;
	syndet_irq_mask = TRUE;
	opn_irq_flag = FALSE;
	whg_irq_flag = FALSE;

	beep_flag = FALSE;

	/* �C�x���g��ǉ� */
	schedule_setevent(EVENT_MAINTIMER, 2034, mainetc_event);
}

/*
 *	BEEP�I���C�x���g
 */
BOOL FASTCALL mainetc_beep(void)
{
	beep_flag = FALSE;

	/* ���ȃC�x���g�폜���́ATRUE�ɂ��� */
	schedule_delevent(EVENT_BEEP);
	return TRUE;
}

/*
 *	�^�C�}�[���荞�݃C�x���g
 */
static BOOL FASTCALL mainetc_event(void)
{
	/* 2.03ms���Ƃ�CLK�ŁAmask�̔��]��DFF�œ��͂��� */
	timer_irq_flag = !timer_irq_mask;
	maincpu_irq();

	if (event[EVENT_MAINTIMER].reload == 2034) {
		schedule_setevent(EVENT_MAINTIMER, 2035, mainetc_event);
	}
	else {
		schedule_setevent(EVENT_MAINTIMER, 2034, mainetc_event);
	}

	return TRUE;
}

/*
 *	FDC���荞��
 *	(fdc.c���R�}���h����E�ُ�I�����A�t�H�[�X�C���^���v�g�ŌĂ΂��)
 */
void FASTCALL mainetc_fdc(void)
{
	/* �}�X�N����Ă���΁A�������Ȃ� */
	if (mfd_irq_mask) {
		return;
	}

	/* ���C��CPU��IRQ���荞�݂������� */
	mfd_irq_flag = TRUE;

	/* ���� */
	maincpu_irq();
}

/*
 *	LP���荞��
 *	(tapelp.c���v�����^�f�[�^�o�͎��ɌĂ΂��)
 */
void FASTCALL mainetc_lp(void)
{
	/* �}�X�N����Ă���΁A�������Ȃ� */
	if (lp_irq_mask) {
		return;
	}

	/* ���C��CPU��IRQ���荞�݂������� */
	lp_irq_flag = TRUE;

	/* ���� */
	maincpu_irq();
}

/*
 *	���C��CPU I/O
 *	�P�o�C�g�ǂݏo��
 */
BOOL FASTCALL mainetc_readb(WORD addr, BYTE *dat)
{
	BYTE ret;

	switch (addr) {
		/* �L�[�{�[�h ��� */
		case 0xfd00:
			if (key_fm7 & 0x0100) {
				*dat = 0xff;
			}
			else {
				*dat = 0x7f;
			}
			return TRUE;

		/* �L�[�{�[�h ���� */
		case 0xfd01:
			*dat = (BYTE)(key_fm7 & 0xff);
			key_irq_flag = FALSE;
			maincpu_irq();
			subcpu_firq();
			return TRUE;

		/* IRQ�v������ */
		case 0xfd03:
			ret = 0xff;
			if ((key_irq_flag) && !(key_irq_mask)) {
				ret &= ~0x01;
			}
			if (lp_irq_flag) {
				ret &= ~0x02;
				lp_irq_flag = FALSE;
			}
			if (timer_irq_flag) {
				ret &= ~0x04;
				timer_irq_flag = FALSE;
			}
			if (mfd_irq_flag ||
				txrdy_irq_flag ||
				rxrdy_irq_flag ||
				syndet_irq_flag ||
				opn_irq_flag ||
				whg_irq_flag) {
				ret &= ~0x08;
			}
			*dat = ret;
			maincpu_irq();
			return TRUE;

		/* BASIC ROM */
		case 0xfd0f:
			basicrom_en = TRUE;
			*dat = 0xff;
			return TRUE;
	}

	return FALSE;
}

/*
 *	���C��CPU I/O
 *	�P�o�C�g��������
 */
BOOL FASTCALL mainetc_writeb(WORD addr, BYTE dat)
{
	switch (addr) {
		/* ���荞�݃}�X�N */
		case 0xfd02:
			if (dat & 0x80) {
				syndet_irq_mask = FALSE;
			}
			else {
				syndet_irq_mask = TRUE;
			}
			if (dat & 0x40) {
				rxrdy_irq_mask = FALSE;
			}
			else {
				rxrdy_irq_mask = TRUE;
			}
			if (dat & 0x20) {
				txrdy_irq_mask = FALSE;
			}
			else {
				txrdy_irq_mask = TRUE;
			}
			if (dat & 0x10) {
				mfd_irq_mask = FALSE;
			}
			else {
				mfd_irq_mask = TRUE;
			}
			if (dat & 0x04) {
				timer_irq_mask = FALSE;
			}
			else {
				timer_irq_mask = TRUE;
			}
			if (dat & 0x02) {
				lp_irq_mask = FALSE;
			}
			else {
				lp_irq_mask = TRUE;
			}
			if (dat & 0x01) {
				key_irq_mask = FALSE;
			}
			else {
				key_irq_mask = TRUE;
			}
			maincpu_irq();
			subcpu_firq();
			return TRUE;

		/* BEEP */
		case 0xfd03:
			/* �X�s�[�J�t���O�̏��� */
			if (dat & 0x01) {
				speaker_flag = TRUE;
			}
			else {
				speaker_flag = FALSE;
			}
			if (dat & 0x40) {
				/* �P��BEEP */
				beep_flag = TRUE;
				schedule_setevent(EVENT_BEEP, 205000, mainetc_beep);
			}
			else {
				if (dat & 0x80) {
					/* �A��BEEP */
					beep_flag = TRUE;
				}
				else {
					/* BEEP OFF */
					beep_flag = FALSE;
				}
			}
			return TRUE;

		/* BASIC ROM */
		case 0xfd0f:
			basicrom_en = FALSE;
			return TRUE;
	}

	return FALSE;
}

/*
 *	���C��CPU I/O
 *	�Z�[�u
 */
BOOL FASTCALL mainetc_save(int fileh)
{
	if (!file_bool_write(fileh, key_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, key_irq_mask)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, lp_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, lp_irq_mask)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, timer_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, timer_irq_mask)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, mfd_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, mfd_irq_mask)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, txrdy_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, txrdy_irq_mask)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, rxrdy_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, rxrdy_irq_mask)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, syndet_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, syndet_irq_mask)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, opn_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, whg_irq_flag)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, beep_flag)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, speaker_flag)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	���C��CPU I/O
 *	���[�h
 */
BOOL FASTCALL mainetc_load(int fileh, int ver)
{
	/* �o�[�W�����`�F�b�N */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &key_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &key_irq_mask)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &lp_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &lp_irq_mask)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &timer_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &timer_irq_mask)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &mfd_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &mfd_irq_mask)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &txrdy_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &txrdy_irq_mask)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &rxrdy_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &rxrdy_irq_mask)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &syndet_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &syndet_irq_mask)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &opn_irq_flag)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &whg_irq_flag)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &beep_flag)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &speaker_flag)) {
		return FALSE;
	}

	/* �C�x���g */
	schedule_handle(EVENT_MAINTIMER, mainetc_event);
	schedule_handle(EVENT_BEEP, mainetc_beep);

	return TRUE;
}

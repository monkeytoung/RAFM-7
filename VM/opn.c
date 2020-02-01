/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ OPN(YM2203) ]
 */

#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "opn.h"
#include "device.h"
#include "mainetc.h"
#include "event.h"
#include "whg.h"

/*
 *	�O���[�o�� ���[�N
 */
BYTE opn_reg[256];						/* OPN���W�X�^ */
BOOL opn_key[4];						/* OPN�L�[�I���t���O */
BOOL opn_timera;						/* �^�C�}�[A����t���O */
BOOL opn_timerb;						/* �^�C�}�[B����t���O */
DWORD opn_timera_tick;					/* �^�C�}�[A�Ԋu */
DWORD opn_timerb_tick;					/* �^�C�}�[B�Ԋu */
BYTE opn_scale;							/* �v���X�P�[�� */

/*
 *	�X�^�e�B�b�N ���[�N
 */
static BYTE opn_pstate;					/* �|�[�g��� */
static BYTE opn_selreg;					/* �Z���N�g���W�X�^ */
static BYTE opn_seldat;					/* �Z���N�g�f�[�^ */
static BOOL opn_timera_int;				/* �^�C�}�[A�I�[�o�[�t���[ */
static BOOL opn_timerb_int;				/* �^�C�}�[B�I�[�o�[�t���[ */
static BOOL opn_timera_en;				/* �^�C�}�[A�C�l�[�u�� */
static BOOL opn_timerb_en;				/* �^�C�}�[B�C�l�[�u�� */

/*
 *	OPN
 *	������
 */
BOOL FASTCALL opn_init(void)
{
	memset(opn_reg, 0, sizeof(opn_reg));

	return TRUE;
}

/*
 *	OPN
 *	�N���[���A�b�v
 */
void FASTCALL opn_cleanup(void)
{
	BYTE i;

	/* PSG */
	for (i=0; i<6; i++) {
		opn_notify(i, 0);
	}
	opn_notify(7, 0xff);

	/* TL=$7F */
	for (i=0x40; i<0x50; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		opn_notify(i, 0x7f);
	}

	/* �L�[�I�t */
	for (i=0; i<3; i++) {
		opn_notify(0x28, i);
	}
}

/*
 *	OPN
 *	���Z�b�g
 */
void FASTCALL opn_reset(void)
{
	BYTE i;

	/* ���W�X�^�N���A�A�^�C�}�[OFF */
	memset(opn_reg, 0, sizeof(opn_reg));
	opn_timera = FALSE;
	opn_timerb = FALSE;

	/* I/O������ */
	opn_pstate = OPN_INACTIVE;
	opn_selreg = 0;
	opn_seldat = 0;

	/* �f�o�C�X */
	opn_timera_int = FALSE;
	opn_timerb_int = FALSE;
	opn_timera_tick = 0;
	opn_timerb_tick = 0;
	opn_timera_en = FALSE;
	opn_timerb_en = FALSE;
	opn_scale = 3;

	/* PSG������ */
	for (i=0; i<14;i++) {
		if (i == 7) {
			opn_notify(i, 0xff);
			opn_reg[i] = 0xff;
		}
		else {
			opn_notify(i, 0);
		}
	}

	/* MUL,DT */
	for (i=0x30; i<0x40; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		opn_notify(i, 0);
	}

	/* TL=$7F */
	for (i=0x40; i<0x50; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		opn_notify(i, 0x7f);
		opn_reg[i] = 0x7f;
	}

	/* AR=$1F */
	for (i=0x50; i<0x60; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		opn_notify(i, 0x1f);
		opn_reg[i] = 0x1f;
	}

	/* ���̑� */
	for (i=0x60; i<0xb4; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		opn_notify(i, 0);
	}

	/* SL,RR */
	for (i=0x80; i<0x90; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		opn_notify(i, 0xff);
		opn_reg[i] = 0xff;
	}

	/* �L�[�I�t */
	for (i=0; i<3; i++) {
		opn_notify(0x28, i);
	}

	/* ���[�h */
	opn_notify(0x27, 0);
}

/*
 *	OPN
 *	�^�C�}�[A�I�[�o�t���[
 */
static BOOL FASTCALL opn_timera_event(void)
{
	/* �C�l�[�u���� */
	if (opn_timera_en) {
		/* �I�[�o�[�t���[�A�N�V�������L���� */
		if (opn_timera) {
			opn_timera = FALSE;
			opn_timera_int = TRUE;

			/* ���荞�݂������� */
			opn_irq_flag = TRUE;
			maincpu_irq();
		}
	}

	/* CSM�����������[�h�ł̃L�[�I�� */
	opn_notify(0xff, 0);

	/* �^�C�}�[�͉񂵑����� */
	return TRUE;
}

/*
 *	OPN
 *	�^�C�}�[A�C���^�[�o���Z�o
 */
static void FASTCALL opn_timera_calc(void)
{
	DWORD t;
	BYTE temp;

	t = opn_reg[0x24];
	t *= 4;
	temp = (BYTE)(opn_reg[0x25] & 3);
	t |= temp;
	t &= 0x3ff;
	t = (1024 - t);
	t *= opn_scale;
	t *= 12;
	t *= 10000;
	t /= 12288;

	/* �^�C�}�[�l��ݒ� */
	if (opn_timera_tick != t) {
		opn_timera_tick = t;
		schedule_setevent(EVENT_OPN_A, opn_timera_tick, opn_timera_event);
	}
}

/*
 *	OPN
 *	�^�C�}�[B�I�[�o�t���[
 */
static BOOL FASTCALL opn_timerb_event(void)
{
	/* �C�l�[�u���� */
	if (opn_timerb_en) {
		/* �I�[�o�[�t���[�A�N�V�������L���� */
		if (opn_timerb) {
			/* �t���O�ύX */
			opn_timerb = FALSE;
			opn_timerb_int = TRUE;

			/* ���荞�݂������� */
			opn_irq_flag = TRUE;
			maincpu_irq();
		}
	}

	/* �^�C�}�[�͉񂵑����� */
	return TRUE;
}

/*
 *	OPN
 *	�^�C�}�[B�C���^�[�o���Z�o
 */
static void FASTCALL opn_timerb_calc(void)
{
	DWORD t;

	t = opn_reg[0x26];
	t = (256 - t);
	t *= 192;
	t *= opn_scale;
	t *= 10000;
	t /= 12288;

	/* �^�C�}�[�l��ݒ� */
	if (t != opn_timerb_tick) {
		opn_timerb_tick = t;
		schedule_setevent(EVENT_OPN_B, opn_timerb_tick, opn_timerb_event);
	}
}

/*
 *	OPN
 *	���W�X�^�A���C���ǂݏo��
 */
static BYTE FASTCALL opn_readreg(BYTE reg)
{
	/* FM�������͓ǂݏo���Ȃ� */
	if (reg >= 0x10) {
		return 0xff;
	}

	return opn_reg[reg];
}

/*
 *	OPN
 *	���W�X�^�A���C�֏�������
 */
static void FASTCALL opn_writereg(BYTE reg, BYTE dat)
{
	/* �^�C�}�[���� */
	/* ���̃��W�X�^�͔��ɓ���B�ǂ�������Ȃ��܂܈����Ă���l���唼�ł́H */
	if (reg == 0x27) {
		/* �I�[�o�[�t���[�t���O�̃N���A */
		if (dat & 0x10) {
			opn_timera_int = FALSE;
		}
		if (dat & 0x20) {
			opn_timerb_int = FALSE;
		}

		/* ������������A���荞�݂𗎂Ƃ� */
		if (!opn_timera_int && !opn_timerb_int) {
			opn_irq_flag = FALSE;
			maincpu_irq();
		}

		/* �^�C�}�[A */
		if (dat & 0x01) {
			/* 0��1�Ń^�C�}�[�l�����[�h�A����ȊO�ł��^�C�}�[on */
			if ((opn_reg[0x27] & 0x01) == 0) {
				opn_timera_calc();
			}
			opn_timera_en = TRUE;
		}
		else {
			opn_timera_en = FALSE;
		}
		if (dat & 0x04) {
			opn_timera = TRUE;
		}
		else {
			opn_timera = FALSE;
		}

		/* �^�C�}�[B */
		if (dat & 0x02) {
			/* 0��1�Ń^�C�}�[�l�����[�h�A����ȊO�ł��^�C�}�[on */
			if ((opn_reg[0x27] & 0x02) == 0) {
				opn_timerb_calc();
			}
			opn_timerb_en = TRUE;
		}
		else {
			opn_timerb_en = FALSE;
		}
		if (dat & 0x08) {
			opn_timerb = TRUE;
		}
		else {
			opn_timerb = FALSE;
		}

		/* �f�[�^�L�� */
		opn_reg[reg] = dat;

		/* ���[�h�̂ݏo�� */
		opn_notify(0x27, (BYTE)(dat & 0xc0));
		return;
	}

	/* �f�[�^�L�� */
	opn_reg[reg] = dat;

	switch (reg) {
		/* �v���X�P�[���P */
		case 0x2d:
			if (opn_scale != 3) {
				opn_scale = 6;
				opn_timerb_calc();
				opn_timerb_calc();
			}
			return;

		/* �v���X�P�[���Q */
		case 0x2e:
			opn_scale = 3;
			opn_timerb_calc();
			opn_timerb_calc();
			return;

		/* �v���X�P�[���R */
		case 0x2f:
			opn_scale = 2;
			opn_timerb_calc();
			opn_timerb_calc();
			return;

		/* �^�C�}�[A(us�P�ʂŌv�Z) */
		case 0x24:
			opn_timera_calc();
			return;

		case 0x25:
			opn_timera_calc();
			return;

		/* �^�C�}�[B(us�P�ʂŌv�Z) */
		case 0x26:
			opn_timerb_calc();
			return;
	}

	/* �o�͐���i�� */
	if ((reg >= 14) && (reg <= 0x26)) {
		return;
	}
	if ((reg >= 0x29) && (reg <= 0x2f)) {
		return;
	}

	/* �L�[�I�� */
	if (reg == 0x28) {
		if (dat >= 16) {
			opn_key[dat & 0x03] = TRUE;
		}
		else {
			opn_key[dat & 0x03] = FALSE;
		}
	}

	/* �o�� */
	opn_notify(reg, dat);
}

/*
 *	OPN
 *	�P�o�C�g�ǂݏo��
 */
BOOL FASTCALL opn_readb(WORD addr, BYTE *dat)
{
	switch (addr) {
		/* �R�}���h���W�X�^�͓ǂݏo���֎~ */
		case 0xfd0d:
		case 0xfd15:
			*dat = 0xff;
			return TRUE;

		/* �f�[�^���W�X�^ */
		case 0xfd0e:
		case 0xfd16:
			switch (opn_pstate) {
				/* �ʏ�R�}���h */
				case OPN_INACTIVE:
				case OPN_READDAT:
				case OPN_WRITEDAT:
				case OPN_ADDRESS:
					*dat = opn_seldat;
					break;

				/* �X�e�[�^�X�ǂݏo�� */
				case OPN_READSTAT:
					*dat = 0;
					if (opn_timera_int) {
						*dat |= 0x01;
					}
					if (opn_timerb_int) {
						*dat |= 0x02;
					}
					break;

				/* �W���C�X�e�B�b�N�ǂݎ�� */
				case OPN_JOYSTICK:
					if (opn_selreg == 14) {
						if ((opn_reg[15] & 0xf0) == 0x20) {
							/* �W���C�X�e�B�b�N�P */
							*dat = (BYTE)(~joy_request(0) | 0xc0);
							break;
						}
						if ((opn_reg[15] & 0xf0) == 0x50) {
							/* �W���C�X�e�B�b�N�Q */
							*dat = (BYTE)(~joy_request(1) | 0xc0);
							break;
						}
						/* ����ȊO */
						*dat = 0xff;
					}
					else {
						/* ���W�X�^��14�łȂ���΁AFF�ȊO��Ԃ� */
						/* HOW MANY ROBOT�΍� */
						*dat = 0;
					}
					break;
			}
			return TRUE;

		/* �g�����荞�݃X�e�[�^�X */
		case 0xfd17:
			if (opn_timera_int || opn_timerb_int) {
				*dat = 0xf7;
			}
			else {
				*dat = 0xff;
			}
			return TRUE;
	}

	return FALSE;
}

/*
 *	OPN
 *	�P�o�C�g��������
 */
BOOL FASTCALL opn_writeb(WORD addr, BYTE dat)
{
	switch (addr) {
		/* OPN�R�}���h���W�X�^ */
		case 0xfd0d:
		case 0xfd15:
			switch (dat & 0x0f) {
				/* �C���A�N�e�B�u(�����`�Ȃ��A�f�[�^���W�X�^����������) */
				case OPN_INACTIVE:
					opn_pstate = OPN_INACTIVE;
					break;
				/* �f�[�^�ǂݏo�� */
				case OPN_READDAT:
					opn_pstate = OPN_READDAT;
					opn_seldat = opn_readreg(opn_selreg);
					break;
				/* �f�[�^�������� */
				case OPN_WRITEDAT:
					opn_pstate = OPN_WRITEDAT;
					opn_writereg(opn_selreg, opn_seldat);
					break;
				/* ���b�`�A�h���X */
				case OPN_ADDRESS:
					opn_pstate = OPN_ADDRESS;
					opn_selreg = opn_seldat;
					break;
				/* ���[�h�X�e�[�^�X */
				case OPN_READSTAT:
					opn_pstate = OPN_READSTAT;
					break;
				/* �W���C�X�e�B�b�N�ǂݎ�� */
				case OPN_JOYSTICK:
					opn_pstate = OPN_JOYSTICK;
					break;
			}
			return TRUE;

		/* �f�[�^���W�X�^ */
		case 0xfd0e:
		case 0xfd16:
			opn_seldat = dat;
			/* �C���A�N�e�B�u�ȊO�̏ꍇ�́A����̓�����s�� */
			switch (opn_pstate){
				/* �f�[�^�������� */
				case OPN_WRITEDAT:
					opn_writereg(opn_selreg, opn_seldat);
					break;
				/* ���b�`�A�h���X */
				case OPN_ADDRESS:
					opn_selreg = opn_seldat;
					break;
			}
			return TRUE;
	}

	return FALSE;
}

/*
 *	OPN
 *	�Z�[�u
 */
BOOL FASTCALL opn_save(int fileh)
{
	if (!file_write(fileh, opn_reg, 256)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, opn_timera)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, opn_timerb)) {
		return FALSE;
	}
	if (!file_dword_write(fileh, opn_timera_tick)) {
		return FALSE;
	}
	if (!file_dword_write(fileh, opn_timerb_tick)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, opn_scale)) {
		return FALSE;
	}

	if (!file_byte_write(fileh, opn_pstate)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, opn_selreg)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, opn_seldat)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, opn_timera_int)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, opn_timerb_int)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, opn_timera_en)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, opn_timerb_en)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	OPN
 *	���[�h
 */
BOOL FASTCALL opn_load(int fileh, int ver)
{
	int i;

	/* �o�[�W�����`�F�b�N */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_read(fileh, opn_reg, 256)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &opn_timera)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &opn_timerb)) {
		return FALSE;
	}
	if (!file_dword_read(fileh, &opn_timera_tick)) {
		return FALSE;
	}
	if (!file_dword_read(fileh, &opn_timerb_tick)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &opn_scale)) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &opn_pstate)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &opn_selreg)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &opn_seldat)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &opn_timera_int)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &opn_timerb_int)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &opn_timera_en)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &opn_timerb_en)) {
		return FALSE;
	}

	/* OPN���W�X�^���� */
	opn_notify(0x27, opn_reg[0x27]);
	opn_notify(0x28, 0);
	opn_notify(0x28, 1);
	opn_notify(0x28, 2);

	opn_notify(8, 0);
	opn_notify(9, 0);
	opn_notify(10, 0);
	for (i=0; i<14; i++) {
		if ((i < 8) || (i > 10)) {
			opn_notify((BYTE)i, opn_reg[i]);
		}
	}

	for (i=0x30; i<0xb4; i++) {
		if ((i & 0x03) != 3) {
			opn_notify((BYTE)i, opn_reg[i]);
		}
	}

	/* �C�x���g */
	schedule_handle(EVENT_OPN_A, opn_timera_event);
	schedule_handle(EVENT_OPN_B, opn_timerb_event);

	return TRUE;
}

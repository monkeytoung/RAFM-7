/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ WHG(YM2203) ]
 */

#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "whg.h"
#include "device.h"
#include "mainetc.h"
#include "event.h"

/*
 *	�O���[�o�� ���[�N
 */
BOOL whg_enable;						/* WHG�L���E�����t���O */
BOOL whg_use;							/* WHG�g�p�t���O */
BYTE whg_reg[256];						/* WHG���W�X�^ */
BOOL whg_key[4];						/* WHG�L�[�I���t���O */
BOOL whg_timera;						/* �^�C�}�[A����t���O */
BOOL whg_timerb;						/* �^�C�}�[B����t���O */
DWORD whg_timera_tick;					/* �^�C�}�[A�Ԋu */
DWORD whg_timerb_tick;					/* �^�C�}�[B�Ԋu */
BYTE whg_scale;							/* �v���X�P�[�� */

/*
 *	�X�^�e�B�b�N ���[�N
 */
static BYTE whg_pstate;					/* �|�[�g��� */
static BYTE whg_selreg;					/* �Z���N�g���W�X�^ */
static BYTE whg_seldat;					/* �Z���N�g�f�[�^ */
static BOOL whg_timera_int;				/* �^�C�}�[A�I�[�o�[�t���[ */
static BOOL whg_timerb_int;				/* �^�C�}�[B�I�[�o�[�t���[ */
static BOOL whg_timera_en;				/* �^�C�}�[A�C�l�[�u�� */
static BOOL whg_timerb_en;				/* �^�C�}�[B�C�l�[�u�� */

/*
 *	WHG
 *	������
 */
BOOL FASTCALL whg_init(void)
{
	memset(whg_reg, 0, sizeof(whg_reg));

	/* ���݂́A���WHG�L�� */
	whg_enable = TRUE;
	whg_use = FALSE;

	return TRUE;
}

/*
 *	WHG
 *	�N���[���A�b�v
 */
void FASTCALL whg_cleanup(void)
{
	BYTE i;

	/* PSG */
	for (i=0; i<6; i++) {
		whg_notify(i, 0);
	}
	whg_notify(7, 0xff);

	/* TL=$7F */
	for (i=0x40; i<0x50; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		whg_notify(i, 0x7f);
	}

	/* �L�[�I�t */
	for (i=0; i<3; i++) {
		whg_notify(0x28, i);
	}
}

/*
 *	WHG
 *	���Z�b�g
 */
void FASTCALL whg_reset(void)
{
	BYTE i;

	/* ���W�X�^�N���A�A�^�C�}�[OFF */
	memset(whg_reg, 0, sizeof(whg_reg));
	whg_timera = FALSE;
	whg_timerb = FALSE;

	/* I/O������ */
	whg_pstate = WHG_INACTIVE;
	whg_selreg = 0;
	whg_seldat = 0;

	/* �f�o�C�X */
	whg_timera_int = FALSE;
	whg_timerb_int = FALSE;
	whg_timera_tick = 0;
	whg_timerb_tick = 0;
	whg_timera_en = FALSE;
	whg_timerb_en = FALSE;
	whg_scale = 3;

	/* PSG������ */
	for (i=0; i<14;i++) {
		if (i == 7) {
			whg_notify(i, 0xff);
			whg_reg[i] = 0xff;
		}
		else {
			whg_notify(i, 0);
		}
	}

	/* MUL,DT */
	for (i=0x30; i<0x40; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		whg_notify(i, 0);
	}

	/* TL=$7F */
	for (i=0x40; i<0x50; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		whg_notify(i, 0x7f);
		whg_reg[i] = 0x7f;
	}

	/* AR=$1F */
	for (i=0x50; i<0x60; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		whg_notify(i, 0x1f);
		whg_reg[i] = 0x1f;
	}

	/* ���̑� */
	for (i=0x60; i<0xb4; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		whg_notify(i, 0);
	}

	/* SL,RR */
	for (i=0x80; i<0x90; i++) {
		if ((i & 0x03) == 3) {
			continue;
		}
		whg_notify(i, 0xff);
		whg_reg[i] = 0xff;
	}

	/* �L�[�I�t */
	for (i=0; i<3; i++) {
		whg_notify(0x28, i);
	}

	/* ���[�h */
	whg_notify(0x27, 0);

	/* �g�p�t���O�������� */
	whg_use = FALSE;
}

/*
 *	WHG
 *	�^�C�}�[A�I�[�o�t���[
 */
static BOOL FASTCALL whg_timera_event(void)
{
	/* �C�l�[�u���� */
	if (whg_enable && whg_timera_en) {
		/* �I�[�o�[�t���[�A�N�V�������L���� */
		if (whg_timera) {
			whg_timera = FALSE;
			whg_timera_int = TRUE;

			/* ���荞�݂������� */
			whg_irq_flag = TRUE;
			maincpu_irq();
		}
	}

	/* CSM�����������[�h�ł̃L�[�I�� */
	if (whg_enable) {
		whg_notify(0xff, 0);
	}

	/* �^�C�}�[�͉񂵑����� */
	return TRUE;
}

/*
 *	WHG
 *	�^�C�}�[A�C���^�[�o���Z�o
 */
static void FASTCALL whg_timera_calc(void)
{
	DWORD t;
	BYTE temp;

	t = whg_reg[0x24];
	t *= 4;
	temp = (BYTE)(whg_reg[0x25] & 3);
	t |= temp;
	t &= 0x3ff;
	t = (1024 - t);
	t *= whg_scale;
	t *= 12;
	t *= 10000;
	t /= 12288;

	/* �^�C�}�[�l��ݒ� */
	if (whg_timera_tick != t) {
		whg_timera_tick = t;
		schedule_setevent(EVENT_WHG_A, whg_timera_tick, whg_timera_event);
	}
}

/*
 *	WHG
 *	�^�C�}�[B�I�[�o�t���[
 */
static BOOL FASTCALL whg_timerb_event(void)
{
	/* �C�l�[�u���� */
	if (whg_enable && whg_timerb_en) {
		/* �I�[�o�[�t���[�A�N�V�������L���� */
		if (whg_timerb) {
			/* �t���O�ύX */
			whg_timerb = FALSE;
			whg_timerb_int = TRUE;

			/* ���荞�݂������� */
			whg_irq_flag = TRUE;
			maincpu_irq();
		}
	}

	/* �^�C�}�[�͉񂵑����� */
	return TRUE;
}

/*
 *	WHG
 *	�^�C�}�[B�C���^�[�o���Z�o
 */
static void FASTCALL whg_timerb_calc(void)
{
	DWORD t;

	t = whg_reg[0x26];
	t = (256 - t);
	t *= 192;
	t *= whg_scale;
	t *= 10000;
	t /= 12288;

	/* �^�C�}�[�l��ݒ� */
	if (t != whg_timerb_tick) {
		whg_timerb_tick = t;
		schedule_setevent(EVENT_WHG_B, whg_timerb_tick, whg_timerb_event);
	}
}

/*
 *	WHG
 *	���W�X�^�A���C���ǂݏo��
 */
static BYTE FASTCALL whg_readreg(BYTE reg)
{
	/* FM�������͓ǂݏo���Ȃ� */
	if (reg >= 0x10) {
		return 0xff;
	}

	return whg_reg[reg];
}

/*
 *	WHG
 *	���W�X�^�A���C�֏�������
 */
static void FASTCALL whg_writereg(BYTE reg, BYTE dat)
{
	/* �t���O�I�� */
	whg_use = TRUE;

	/* �^�C�}�[���� */
	/* ���̃��W�X�^�͔��ɓ���B�ǂ�������Ȃ��܂܈����Ă���l���唼�ł́H */
	if (reg == 0x27) {
		/* �I�[�o�[�t���[�t���O�̃N���A */
		if (dat & 0x10) {
			whg_timera_int = FALSE;
		}
		if (dat & 0x20) {
			whg_timerb_int = FALSE;
		}

		/* ������������A���荞�݂𗎂Ƃ� */
		if (!whg_timera_int && !whg_timerb_int) {
			whg_irq_flag = FALSE;
			maincpu_irq();
		}

		/* �^�C�}�[A */
		if (dat & 0x01) {
			/* 0��1�Ń^�C�}�[�l�����[�h�A����ȊO�ł��^�C�}�[on */
			if ((whg_reg[0x27] & 0x01) == 0) {
				whg_timera_calc();
			}
			whg_timera_en = TRUE;
		}
		else {
			whg_timera_en = FALSE;
		}
		if (dat & 0x04) {
			whg_timera = TRUE;
		}
		else {
			whg_timera = FALSE;
		}

		/* �^�C�}�[B */
		if (dat & 0x02) {
			/* 0��1�Ń^�C�}�[�l�����[�h�A����ȊO�ł��^�C�}�[on */
			if ((whg_reg[0x27] & 0x02) == 0) {
				whg_timerb_calc();
			}
			whg_timerb_en = TRUE;
		}
		else {
			whg_timerb_en = FALSE;
		}
		if (dat & 0x08) {
			whg_timerb = TRUE;
		}
		else {
			whg_timerb = FALSE;
		}

		/* �f�[�^�L�� */
		whg_reg[reg] = dat;

		/* ���[�h�̂ݏo�� */
		whg_notify(0x27, (BYTE)(dat & 0xc0));
		return;
	}

	/* �f�[�^�L�� */
	whg_reg[reg] = dat;

	switch (reg) {
		/* �v���X�P�[���P */
		case 0x2d:
			if (whg_scale != 3) {
				whg_scale = 6;
				whg_timerb_calc();
				whg_timerb_calc();
			}
			return;

		/* �v���X�P�[���Q */
		case 0x2e:
			whg_scale = 3;
			whg_timerb_calc();
			whg_timerb_calc();
			return;

		/* �v���X�P�[���R */
		case 0x2f:
			whg_scale = 2;
			whg_timerb_calc();
			whg_timerb_calc();
			return;

		/* �^�C�}�[A(us�P�ʂŌv�Z) */
		case 0x24:
			whg_timera_calc();
			return;

		case 0x25:
			whg_timera_calc();
			return;

		/* �^�C�}�[B(us�P�ʂŌv�Z) */
		case 0x26:
			whg_timerb_calc();
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
			whg_key[dat & 0x03] = TRUE;
		}
		else {
			whg_key[dat & 0x03] = FALSE;
		}
	}

	/* �o�� */
	whg_notify(reg, dat);
}

/*
 *	WHG
 *	�P�o�C�g�ǂݏo��
 */
BOOL FASTCALL whg_readb(WORD addr, BYTE *dat)
{
	/* �o�[�W�����A�L���t���O���`�F�b�N�A�����Ȃ牽�����Ȃ� */
	if (!whg_enable || (fm7_ver < 2)) {
		return FALSE;
	}

	switch (addr) {
		/* �R�}���h���W�X�^�͓ǂݏo���֎~ */
		case 0xfd45:
			*dat = 0xff;
			return TRUE;

		/* �f�[�^���W�X�^ */
		case 0xfd46:
			switch (whg_pstate) {
				/* �ʏ�R�}���h */
				case WHG_INACTIVE:
				case WHG_READDAT:
				case WHG_WRITEDAT:
				case WHG_ADDRESS:
					*dat = whg_seldat;
					break;

				/* �X�e�[�^�X�ǂݏo�� */
				case WHG_READSTAT:
					*dat = 0;
					if (whg_timera_int) {
						*dat |= 0x01;
					}
					if (whg_timerb_int) {
						*dat |= 0x02;
					}
					break;

				/* �W���C�X�e�B�b�N�ǂݎ�� */
				case WHG_JOYSTICK:
					if (whg_selreg == 14) {
						/* �W���C�X�e�B�b�N�͖��ڑ��Ƃ��Ĉ��� */
						*dat = 0xff;
					}
					else {
						/* ���W�X�^��14�łȂ���΁AFF�ȊO��Ԃ� */
						*dat = 0;
					}
					break;
			}
			return TRUE;
	}

	return FALSE;
}

/*
 *	WHG
 *	�P�o�C�g��������
 */
BOOL FASTCALL whg_writeb(WORD addr, BYTE dat)
{
	/* �o�[�W�����A�L���t���O���`�F�b�N�A�����Ȃ牽�����Ȃ� */
	if (!whg_enable || (fm7_ver < 2)) {
		return FALSE;
	}

	switch (addr) {
		/* WHG�R�}���h���W�X�^ */
		case 0xfd45:
			switch (dat & 0x0f) {
				/* �C���A�N�e�B�u(�����`�Ȃ��A�f�[�^���W�X�^����������) */
				case WHG_INACTIVE:
					whg_pstate = WHG_INACTIVE;
					break;
				/* �f�[�^�ǂݏo�� */
				case WHG_READDAT:
					whg_pstate = WHG_READDAT;
					whg_seldat = whg_readreg(whg_selreg);
					break;
				/* �f�[�^�������� */
				case WHG_WRITEDAT:
					whg_pstate = WHG_WRITEDAT;
					whg_writereg(whg_selreg, whg_seldat);
					break;
				/* ���b�`�A�h���X */
				case WHG_ADDRESS:
					whg_pstate = WHG_ADDRESS;
					whg_selreg = whg_seldat;
					break;
				/* ���[�h�X�e�[�^�X */
				case WHG_READSTAT:
					whg_pstate = WHG_READSTAT;
					break;
				/* �W���C�X�e�B�b�N�ǂݎ�� */
				case WHG_JOYSTICK:
					whg_pstate = WHG_JOYSTICK;
					break;
			}
			return TRUE;

		/* �f�[�^���W�X�^ */
		case 0xfd46:
			whg_seldat = dat;
			/* �C���A�N�e�B�u�ȊO�̏ꍇ�́A����̓�����s�� */
			switch (whg_pstate){
				/* �f�[�^�������� */
				case WHG_WRITEDAT:
					whg_writereg(whg_selreg, whg_seldat);
					break;
				/* ���b�`�A�h���X */
				case WHG_ADDRESS:
					whg_selreg = whg_seldat;
					break;
			}
			return TRUE;
	}

	return FALSE;
}

/*
 *	WHG
 *	�Z�[�u
 */
BOOL FASTCALL whg_save(int fileh)
{
	if (!file_bool_write(fileh, whg_enable)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, whg_use)) {
		return FALSE;
	}

	if (!file_write(fileh, whg_reg, 256)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, whg_timera)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, whg_timerb)) {
		return FALSE;
	}
	if (!file_dword_write(fileh, whg_timera_tick)) {
		return FALSE;
	}
	if (!file_dword_write(fileh, whg_timerb_tick)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, whg_scale)) {
		return FALSE;
	}

	if (!file_byte_write(fileh, whg_pstate)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, whg_selreg)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, whg_seldat)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, whg_timera_int)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, whg_timerb_int)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, whg_timera_en)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, whg_timerb_en)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	WHG
 *	���[�h
 */
BOOL FASTCALL whg_load(int fileh, int ver)
{
	int i;

	/* �t�@�C���o�[�W����3�Œǉ� */
	if (ver < 3) {
		return TRUE;
	}

	if (!file_bool_read(fileh, &whg_enable)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &whg_use)) {
		return FALSE;
	}

	if (!file_read(fileh, whg_reg, 256)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &whg_timera)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &whg_timerb)) {
		return FALSE;
	}
	if (!file_dword_read(fileh, &whg_timera_tick)) {
		return FALSE;
	}
	if (!file_dword_read(fileh, &whg_timerb_tick)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &whg_scale)) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &whg_pstate)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &whg_selreg)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &whg_seldat)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &whg_timera_int)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &whg_timerb_int)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &whg_timera_en)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &whg_timerb_en)) {
		return FALSE;
	}

	/* WHG���W�X�^���� */
	whg_notify(0x27, whg_reg[0x27]);
	whg_notify(0x28, 0);
	whg_notify(0x28, 1);
	whg_notify(0x28, 2);

	whg_notify(8, 0);
	whg_notify(9, 0);
	whg_notify(10, 0);
	for (i=0; i<14; i++) {
		if ((i < 8) || (i > 10)) {
			whg_notify((BYTE)i, whg_reg[i]);
		}
	}

	for (i=0x30; i<0xb4; i++) {
		if ((i & 0x03) != 3) {
			whg_notify((BYTE)i, whg_reg[i]);
		}
	}

	/* �C�x���g */
	schedule_handle(EVENT_WHG_A, whg_timera_event);
	schedule_handle(EVENT_WHG_B, whg_timerb_event);

	return TRUE;
}

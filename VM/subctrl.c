/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �T�uCPU�R���g���[�� ]
 */

#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "keyboard.h"
#include "subctrl.h"
#include "device.h"
#include "display.h"

/*
 *	�O���[�o�� ���[�N
 */
BOOL subhalt_flag;						/* �T�uHALT�t���O */
BOOL subbusy_flag;						/* �T�uBUSY�t���O */
BOOL subcancel_flag;					/* �T�u�L�����Z���t���O */
BOOL subattn_flag;						/* �T�u�A�e���V�����t���O */
BYTE shared_ram[0x80];					/* ���LRAM */
BOOL subreset_flag;						/* �T�u�ċN���t���O */
BOOL mode320;							/* 320x200���[�h */

/*
 *	�T�uCPU�R���g���[��
 *	������
 */
BOOL FASTCALL subctrl_init(void)
{
	return TRUE;
}

/*
 *	�T�uCPU�R���g���[��
 *	�N���[���A�b�v
 */
void FASTCALL subctrl_cleanup(void)
{
}

/*
 *	�T�uCPU�R���g���[��
 *	���Z�b�g
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
 *	�T�uCPU�R���g���[��
 *	�P�o�C�g�ǂݏo��
 */
BOOL FASTCALL subctrl_readb(WORD addr, BYTE *dat)
{
	BYTE ret;

	switch (addr) {
		/* �T�uCPU �A�e���V�������荞�݁ABreak�L�[���荞�� */
		case 0xfd04:
			ret = 0xff;
			/* �A�e���V�����t���O */
			if (subattn_flag) {
				ret &= ~0x01;
				subattn_flag = FALSE;
			}
			/* Break�L�[�t���O */
			if (break_flag) {
				ret &= ~0x02;
			}
			*dat = ret;
			maincpu_firq();
			return TRUE;

		/* �T�u�C���^�t�F�[�X */
		case 0xfd05:
			if (subbusy_flag) {
				*dat = 0xfe;
				return TRUE;
			}
			else {
				*dat = 0x7e;
				return TRUE;
			}

		/* �T�u���[�h�X�e�[�^�X */
		case 0xfd12:
			ret = 0xff;
			if (fm7_ver >= 2) {
				/* 320/640 */
				if (!mode320) {
					ret &= ~0x40;
				}

				/* �u�����N�X�e�[�^�X */
				if (blank_flag) {
					ret &= ~0x02;
				}
				/* VSYNC�X�e�[�^�X */
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
 *	�T�uCPU�R���g���[��
 *	�P�o�C�g��������
 */
BOOL FASTCALL subctrl_writeb(WORD addr, BYTE dat)
{
	switch (addr) {
		/* �T�u�R���g���[�� */
		case 0xfd05:
			if (dat & 0x80) {
				/* �T�uHALT */
				subhalt_flag = TRUE;
				subbusy_flag = TRUE;
			}
			else {
				/* �T�uRUN */
				subhalt_flag = FALSE;
			}
			if (dat & 0x40) {
				/* �L�����Z��IRQ */
				subcancel_flag = TRUE;
			}
			subcpu_irq();
			return TRUE;

		/* �T�u���[�h�؂�ւ� */
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

		/* �T�u�o���N�؂�ւ� */
		case 0xfd13:
			if (fm7_ver >= 2) {
				/* �o���N�؂�ւ� */
				subrom_bank = (BYTE)(dat & 0x03);
				if (subrom_bank == 3) {
					subrom_bank = 0;
					ASSERT(FALSE);
				}

				/* ���Z�b�g */
				subcpu_reset();

				/* �t���O�ރZ�b�g */
				subreset_flag = TRUE;
				subbusy_flag = TRUE;
			}
			return TRUE;
	}

	return FALSE;
}

/*
 *	�T�uCPU�R���g���[��
 *	�Z�[�u
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
 *	�T�uCPU�R���g���[��
 *	���[�h
 */
BOOL FASTCALL subctrl_load(int fileh, int ver)
{
	/* �o�[�W�����`�F�b�N */
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

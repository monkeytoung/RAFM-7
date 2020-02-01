/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �}���`�y�[�W ]
 */

#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "multipag.h"
#include "display.h"
#include "ttlpalet.h"
#include "subctrl.h"
#include "apalet.h"
#include "device.h"

/*
 *	�O���[�o�� ���[�N
 */
BYTE multi_page;						/* �}���`�y�[�W ���[�N */

/*
 *	�}���`�y�[�W
 *	������
 */
BOOL FASTCALL multipag_init(void)
{
	return TRUE;
}

/*
 *	�}���`�y�[�W
 *	�N���[���A�b�v
 */
void FASTCALL multipag_cleanup(void)
{
}

/*
 *	�}���`�y�[�W
 *	���Z�b�g
 */
void FASTCALL multipag_reset(void)
{
	multi_page = 0;
}

/*
 *	�}���`�y�[�W
 *	�P�o�C�g�ǂݏo��
 */
BOOL FASTCALL multipag_readb(WORD addr, BYTE *dat)
{
	if (addr != 0xfd37) {
		return FALSE;
	}

	/* ���FF���ǂݏo����� */
	*dat = 0xff;
	return TRUE;
}

/*
 *	�}���`�y�[�W
 *	�P�o�C�g��������
 */
BOOL FASTCALL multipag_writeb(WORD addr, BYTE dat)
{
	if (addr != 0xfd37) {
		return FALSE;
	}

	/* �f�[�^�L�� */
	multi_page = dat;

	/* �p���b�g�Đݒ� */
	ttlpalet_notify();
	apalet_notify();

	return TRUE;
}

/*
 *	�}���`�y�[�W
 *	�Z�[�u
 */
BOOL FASTCALL multipag_save(int fileh)
{
	return file_byte_write(fileh, multi_page);
}

/*
 *	�}���`�y�[�W
 *	���[�h
 */
BOOL FASTCALL multipag_load(int fileh, int ver)
{
	/* �o�[�W�����`�F�b�N */
	if (ver < 2) {
		return FALSE;
	}

	return file_byte_read(fileh, &multi_page);
}

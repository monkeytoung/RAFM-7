/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ MMR ]
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "xm7.h"
#include "device.h"
#include "mmr.h"

/*
 *	�O���[�o�� ���[�N
 */
BOOL mmr_flag;							/* MMR�L���t���O */
BYTE mmr_seg;							/* MMR�Z�O�����g */
BYTE mmr_reg[0x40];						/* MMR���W�X�^ */
BOOL twr_flag;							/* TWR�L���t���O */
BYTE twr_reg;							/* TWR���W�X�^ */

/*
 *	MMR
 *	������
 */
BOOL FASTCALL mmr_init(void)
{
	return TRUE;
}

/*
 *	MMR
 *	�N���[���A�b�v
 */
void FASTCALL mmr_cleanup(void)
{
}

/*
 *	MMR
 *	���Z�b�g
 */
void FASTCALL mmr_reset(void)
{
	mmr_flag = FALSE;
	twr_flag = FALSE;
	memset(mmr_reg, 0, sizeof(mmr_reg));
	twr_reg = 0;
}

/*-[ �������}�l�[�W�� ]-----------------------------------------------------*/

/*
 *	TWR�A�h���X�ϊ�
 */
static BOOL FASTCALL mmr_trans_twr(WORD addr, DWORD *taddr)
{
	ASSERT(fm7_ver >= 2);

	/* TWR�L���� */
	if (!twr_flag) {
		return FALSE;
	}

	/* �A�h���X�v���`�F�b�N */
	if ((addr < 0x7c00) || (addr > 0x7fff)) {
		return FALSE;
	}

	/* TWR���W�X�^���ϊ� */
	*taddr = (DWORD)twr_reg;
	*taddr *= 256;
	*taddr += addr;
	*taddr &= 0xffff;

	return TRUE;
}

/*
 *	MMR�A�h���X�ϊ�
 */
static DWORD FASTCALL mmr_trans_mmr(WORD addr)
{
	DWORD maddr;
	int offset;

	ASSERT(fm7_ver >= 2);

	/* MMR�L���� */
	if (!mmr_flag) {
		return (DWORD)(0x30000 | addr);
	}

	/* MMR���W�X�^���擾 */
	offset = (int)addr;
	offset >>= 12;
	offset |= (mmr_seg * 0x10);
	maddr = (DWORD)mmr_reg[offset];
	maddr <<= 12;

	/* ����12�r�b�g�ƍ��� */
	addr &= 0xfff;
	maddr |= addr;

	return maddr;
}

/*
 *	���C��CPU�o�X
 *	�P�o�C�g�ǂݏo��
 */
BOOL FASTCALL mmr_extrb(WORD *addr, BYTE *dat)
{
	DWORD raddr;

	ASSERT(fm7_ver >= 2);

	/* $FC00�`$FFFF�͏풓��� */
	if (*addr >= 0xfc00) {
		return FALSE;
	}

	/* TWR,MMR��ʂ� */
	if (!mmr_trans_twr(*addr, &raddr)) {
		raddr = mmr_trans_mmr(*addr);
	}

	/* FM77AV �g��RAM */
	if (raddr < 0x10000) {
		*dat = extram_a[raddr & 0xffff];
		return TRUE;
	}

	/* �T�u�V�X�e�� */
	if (raddr < 0x20000) {
		*dat = submem_readb((WORD)(raddr & 0xffff));
		return TRUE;
	}

	/* ���U�[�u */
	if (raddr < 0x30000) {
		*dat = 0xff;
		return TRUE;
	}

	/* ���U�[�u */
	if (raddr >= 0x40000) {
		*dat = 0xff;
		return TRUE;
	}

	/* $30�Z�O�����g */
	*addr = (WORD)(raddr & 0xffff);
	return FALSE;
}

/*
 *	���C��CPU�o�X
 *	�P�o�C�g�ǂݏo��(I/O�Ȃ�)
 */
BOOL FASTCALL mmr_extbnio(WORD *addr, BYTE *dat)
{
	DWORD raddr;

	ASSERT(fm7_ver >= 2);

	/* $FC00�`$FFFF�͏풓��� */
	if (*addr >= 0xfc00) {
		return FALSE;
	}

	/* TWR,MMR��ʂ� */
	if (!mmr_trans_twr(*addr, &raddr)) {
		raddr = mmr_trans_mmr(*addr);
	}

	/* FM77AV �g��RAM */
	if (raddr < 0x10000) {
		*dat = extram_a[raddr & 0xffff];
		return TRUE;
	}

	/* �T�u�V�X�e�� */
	if (raddr < 0x20000) {
		*dat = submem_readbnio((WORD)(raddr & 0xffff));
		return TRUE;
	}

	/* ���U�[�u */
	if (raddr < 0x30000) {
		*dat = 0xff;
		return TRUE;
	}

	/* ���U�[�u */
	if (raddr >= 0x40000) {
		*dat = 0xff;
		return TRUE;
	}

	/* $30�Z�O�����g */
	*addr = (WORD)(raddr & 0xffff);
	return FALSE;
}

/*
 *	���C��CPU�o�X
 *	�P�o�C�g��������
 */
BOOL FASTCALL mmr_extwb(WORD *addr, BYTE dat)
{
	DWORD raddr;

	ASSERT(fm7_ver >= 2);

	/* $FC00�`$FFFF�͏풓��� */
	if (*addr >= 0xfc00) {
		return FALSE;
	}

	/* TWR,MMR��ʂ� */
	if (!mmr_trans_twr(*addr, &raddr)) {
		raddr = mmr_trans_mmr(*addr);
	}

	/* FM77AV �g��RAM */
	if (raddr < 0x10000) {
		extram_a[raddr & 0xffff] = dat;
		return TRUE;
	}

	/* �T�u�V�X�e�� */
	if (raddr < 0x20000) {
		submem_writeb((WORD)(raddr & 0xffff), dat);
		return TRUE;
	}

	/* ���U�[�u */
	if (raddr < 0x30000) {
		return TRUE;
	}

	/* ���U�[�u */
	if (raddr >= 0x40000) {
		return TRUE;
	}

	/* $30�Z�O�����g */
	*addr = (WORD)(raddr & 0xffff);
	return FALSE;
}

/*-[ �������}�b�v�hI/O ]----------------------------------------------------*/

/*
 *	MMR
 *	�P�o�C�g�ǂݏo��
 */
BOOL FASTCALL mmr_readb(WORD addr, BYTE *dat)
{
	BYTE tmp;

	/* �o�[�W�����`�F�b�N */
	if (fm7_ver < 2) {
		return FALSE;
	}

	switch (addr) {
		/* �u�[�g�X�e�[�^�X */
		case 0xfd0b:
			if (boot_mode == BOOT_BASIC) {
				*dat = 0xfe;
			}
			else {
				*dat = 0xff;
			}
			return TRUE;

		/* �C�j�V�G�[�^ROM */
		case 0xfd10:
			*dat = 0xff;
			return TRUE;

		/* MMR�Z�O�����g */
		case 0xfd90:
			*dat = 0xff;
			return TRUE;

		/* TWR�I�t�Z�b�g */
		case 0xfd92:
			*dat = 0xff;
			return TRUE;

		/* ���[�h�Z���N�g */
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

	/* MMR���W�X�^ */
	if ((addr >= 0xfd80) && (addr <= 0xfd8f)) {
		*dat = mmr_reg[mmr_seg * 0x10 + (addr - 0xfd80)];
		return TRUE;
	}

	return FALSE;
}

/*
 *	MMR
 *	�P�o�C�g��������
 */
BOOL FASTCALL mmr_writeb(WORD addr, BYTE dat)
{
	/* �o�[�W�����`�F�b�N */
	if (fm7_ver < 2) {
		return FALSE;
	}

	switch (addr) {
		/* �u�[�g�X�e�[�^�X */
		case 0xfd0b:
			return TRUE;

		/* �C�j�V�G�[�^ROM */
		case 0xfd10:
			if (dat & 0x02) {
				initrom_en = FALSE;
			}
			else {
				initrom_en = TRUE;
			}
			return TRUE;

		/* MMR�Z�O�����g */
		case 0xfd90:
			mmr_seg = (BYTE)(dat & 0x03);
			return TRUE;

		/* TWR�I�t�Z�b�g */
		case 0xfd92:
			twr_reg = dat;
			return TRUE;

		/* ���[�h�Z���N�g */
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

	/* MMR���W�X�^ */
	if ((addr >= 0xfd80) && (addr <= 0xfd8f)) {
		/* �f�[�^��$00�`$3F�ɐ���(Seles�΍�) */
		mmr_reg[mmr_seg * 0x10 + (addr - 0xfd80)] = (BYTE)(dat & 0x3f);
		return TRUE;
	}

	return FALSE;
}

/*-[ �t�@�C��I/O ]----------------------------------------------------------*/

/*
 *	MMR
 *	�Z�[�u
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
 *	���[�h
 */
BOOL FASTCALL mmr_load(int fileh, int ver)
{
	/* �o�[�W�����`�F�b�N */
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

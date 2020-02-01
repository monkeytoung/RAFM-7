/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ TTL�p���b�g ]
 */

#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "ttlpalet.h"
#include "device.h"

/*
 *	�O���[�o�� ���[�N
 */
BYTE ttl_palet[8];			/* TTL�p���b�g�f�[�^ */

/*
 *	TTL�p���b�g
 *	������
 */
BOOL FASTCALL ttlpalet_init(void)
{
	return TRUE;
}

/*
 *	TTL�p���b�g
 *	�N���[���A�b�v
 */
void FASTCALL ttlpalet_cleanup(void)
{
}

/*
 *	TTL�p���b�g
 *	���Z�b�g
 */
void FASTCALL ttlpalet_reset(void)
{
	int i;
	
	/* ���ׂĂ̐F�������� */
	for (i=0; i<8; i++) {
		ttl_palet[i] = (BYTE)i;
	}

	/* �ʒm */
	ttlpalet_notify();
}

/*
 *	TTL�p���b�g
 *	�P�o�C�g�ǂݏo��
 */
BOOL FASTCALL ttlpalet_readb(WORD addr, BYTE *dat)
{
	/* �͈̓`�F�b�N�A�ǂݏo�� */
	if ((addr >= 0xfd38) && (addr <= 0xfd3f)) {
		ASSERT((WORD)(addr - 0xfd38) <= 7);

		/* ��ʃj�u����0xF0������ */
		*dat = (BYTE)(ttl_palet[(WORD)(addr - 0xfd38)] | 0xf0);
		return TRUE;
	}

	return FALSE;
}

/*
 *	TTL�p���b�g
 *	�P�o�C�g��������
 */
BOOL FASTCALL ttlpalet_writeb(WORD addr, BYTE dat)
{
	int no;

	/* �͈̓`�F�b�N�A�������� */
	if ((addr >= 0xfd38) && (addr <= 0xfd3f)) {
		no = addr - 0xfd38;
		ttl_palet[no] = (BYTE)(dat & 0x07);

		/* �ʒm */
		ttlpalet_notify();
		return TRUE;
	}

	return FALSE;
}

/*
 *	TTL�p���b�g
 *	�Z�[�u
 */
BOOL FASTCALL ttlpalet_save(int fileh)
{
	return file_write(fileh, ttl_palet, 8);
}

/*
 *	TTL�p���b�g
 *	���[�h
 */
BOOL FASTCALL ttlpalet_load(int fileh, int ver)
{
	/* �o�[�W�����`�F�b�N */
	if (ver < 2) {
		return FALSE;
	}

	return file_read(fileh, ttl_palet, 8);
}

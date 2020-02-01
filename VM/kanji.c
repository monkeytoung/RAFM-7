/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ ����ROM ]
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "xm7.h"
#include "kanji.h"
#include "device.h"

/*
 *	�O���[�o�� ���[�N
 */
WORD kanji_addr;						/* ��P�����A�h���X */
BYTE *kanji_rom;						/* ��P����ROM */

/*
 *	����ROM
 *	������
 */
BOOL FASTCALL kanji_init(void)
{
	/* �������m�� */
	kanji_rom = (BYTE *)malloc(0x20000);
	if (!kanji_rom) {
		return FALSE;
	}

	/* �t�@�C���ǂݍ��� */
	if (!file_load(KANJI_ROM, kanji_rom, 0x20000)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	����ROM
 *	�N���[���A�b�v
 */
void FASTCALL kanji_cleanup(void)
{
	ASSERT(kanji_rom);
	if (kanji_rom) {
		free(kanji_rom);
		kanji_rom = NULL;
	}
}

/*
 *	����ROM
 *	���Z�b�g
 */
void FASTCALL kanji_reset(void)
{
	kanji_addr = 0;
}

/*
 *	����ROM
 *	�P�o�C�g�ǂݏo��
 */
BOOL FASTCALL kanji_readb(WORD addr, BYTE *dat)
{
	int offset;

	switch (addr) {
		/* �A�h���X��� */
		case 0xfd20:
			*dat = (BYTE)(kanji_addr >> 8);
			return TRUE;

		/* �A�h���X���� */
		case 0xfd21:
			*dat = (BYTE)(kanji_addr & 0xff);
			return TRUE;

		/* �f�[�^LEFT */
		case 0xfd22:
			offset = kanji_addr << 1;
			*dat = kanji_rom[offset + 0];
			return TRUE;

		/* �f�[�^RIGHT */
		case 0xfd23:
			offset = kanji_addr << 1;
			*dat = kanji_rom[offset + 1];
			return TRUE;
	}

	return FALSE;
}

/*
 *	����ROM
 *	�P�o�C�g��������
 */
BOOL FASTCALL kanji_writeb(WORD addr, BYTE dat)
{
	switch (addr) {
		/* �A�h���X��� */
		case 0xfd20:
			kanji_addr &= 0x00ff;
			kanji_addr |= (WORD)(dat << 8);
			return TRUE;

		/* �A�h���X���� */
		case 0xfd21:
			kanji_addr &= 0xff00;
			kanji_addr |= dat;
			return TRUE;

		/* �f�[�^LEFT*/
		case 0xfd22:
			return TRUE;

		/* �f�[�^RIGHT */
		case 0xfd23:
			return TRUE;
	}

	return FALSE;
}

/*
 *	����ROM
 *	�Z�[�u
 */
BOOL FASTCALL kanji_save(int fileh)
{
	if (!file_word_write(fileh, kanji_addr)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	����ROM
 *	���[�h
 */
BOOL FASTCALL kanji_load(int fileh, int ver)
{
	/* �o�[�W�����`�F�b�N */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_word_read(fileh, &kanji_addr)) {
		return FALSE;
	}

	return TRUE;
}

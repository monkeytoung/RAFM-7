/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �T�uCPU������ ]
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "display.h"
#include "subctrl.h"
#include "device.h"
#include "keyboard.h"
#include "multipag.h"
#include "aluline.h"

/*
 *	�O���[�o�� ���[�N
 */
BYTE *vram_c;						/* VRAM(�^�C�vC) $C000 */
BYTE *subrom_c;						/* ROM (�^�C�vC) $2800 */
BYTE *sub_ram;						/* �R���\�[��RAM $1680 */
BYTE *sub_io;						/* �T�uCPU I/O   $0100 */

BYTE *vram_b;						/* VRAM(�^�C�vB) $C000 */
BYTE *subrom_a;						/* ROM (�^�C�vA) $2000 */
BYTE *subrom_b;						/* ROM (�^�C�vB) $2000 */
BYTE *subromcg;						/* ROM (�t�H���g)$2000 */

BYTE subrom_bank;					/* �T�u�V�X�e��ROM�o���N */
BYTE cgrom_bank;					/* CGROM�o���N */

/*
 *	�T�uCPU������
 *	������
 */
BOOL FASTCALL submem_init(void)
{
	/* ��x�A�S�ăN���A */
	vram_c = NULL;
	subrom_c = NULL;
	vram_b = NULL;
	subrom_a = NULL;
	subrom_b = NULL;
	subromcg = NULL;
	sub_ram = NULL;
	sub_io = NULL;

	/* �������m��(�^�C�vC) */
	vram_c = (BYTE *)malloc(0x18000);
	if (vram_c == NULL) {
		return FALSE;
	}
	subrom_c = (BYTE *)malloc(0x2800);
	if (subrom_c == NULL) {
		return FALSE;
	}

	/* �������m��(�^�C�vA,B) */
	vram_b = vram_c + 0xc000;
	subrom_a = (BYTE *)malloc(0x2000);
	if (subrom_a == NULL) {
		return FALSE;
	}
	subrom_b = (BYTE *)malloc(0x2000);
	if (subrom_b == NULL) {
		return FALSE;
	}
	subromcg = (BYTE *)malloc(0x2000);
	if (subromcg == NULL) {
		return FALSE;
	}

	/* �������m��(����) */
	sub_ram = (BYTE *)malloc(0x1680);
	if (sub_ram == NULL) {
		return FALSE;
	}
	sub_io = (BYTE *)malloc(0x0100);
	if (sub_io == NULL) {
		return FALSE;
	}

	/* ROM�t�@�C���ǂݍ��� */
	if (!file_load(SUBSYSC_ROM, subrom_c, 0x2800)) {
		return FALSE;
	}
	if (!file_load(SUBSYSA_ROM, subrom_a, 0x2000)) {
		return FALSE;
	}
	if (!file_load(SUBSYSB_ROM, subrom_b, 0x2000)) {
		return FALSE;
	}
	if (!file_load(SUBSYSCG_ROM, subromcg, 0x2000)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	�T�uCPU������
 *	�N���[���A�b�v
 */
void FASTCALL submem_cleanup(void)
{
	ASSERT(vram_c);
	ASSERT(subrom_c);
	ASSERT(subrom_a);
	ASSERT(subrom_b);
	ASSERT(subromcg);
	ASSERT(sub_ram);
	ASSERT(sub_io);

	/* �������r���Ŏ��s�����ꍇ���l�� */
	if (vram_c) {
		free(vram_c);
	}
	if (subrom_c) {
		free(subrom_c);
	}

	if (subrom_a) {
		free(subrom_a);
	}
	if (subrom_b) {
		free(subrom_b);
	}
	if (subromcg) {
		free(subromcg);
	}

	if (sub_ram) {
		free(sub_ram);
	}
	if (sub_io) {
		free(sub_io);
	}
}

/*
 *	�T�uCPU������
 *	���Z�b�g
 */
void FASTCALL submem_reset(void)
{
	/* VRAM�A�N�e�B�u�y�[�W */
	vram_aptr = vram_c;
	vram_active = 0;

	/* �o���N�N���A */
	subrom_bank = 0;
	cgrom_bank = 0;

	/* I/O��� �N���A */
	memset(sub_io, 0xff, 0x0100);
}

/*
 *	�T�uCPU������
 *	�P�o�C�g�擾
 */
BYTE FASTCALL submem_readb(WORD addr)
{
	BYTE dat;

	/* VRAM */
	if (addr < 0x4000) {
		if (multi_page & 0x01) {
			return 0xff;
		}
		else {
			aluline_extrb(addr);
			return vram_aptr[addr];
		}
	}
	if (addr < 0x8000) {
		if (multi_page & 0x02) {
			return 0xff;
		}
		else {
			aluline_extrb(addr);
			return vram_aptr[addr];
		}
	}
	if (addr < 0xc000) {
		if (multi_page & 0x04) {
			return 0xff;
		}
		else {
			aluline_extrb(addr);
			return vram_aptr[addr];
		}
	}

	/* ���[�NRAM */
	if (addr < 0xd380) {
		return sub_ram[addr - 0xc000];
	}

	/* ���LRAM */
	if (addr < 0xd400) {
		ASSERT(!subhalt_flag);
		return shared_ram[(WORD)(addr - 0xd380)];
	}

	/* �T�uROM */
	if (addr >= 0xe000) {
		ASSERT(subrom_bank <= 2);
		switch (subrom_bank) {
			/* �^�C�vC */
			case 0:
				return subrom_c[addr - 0xd800];
			/* �^�C�vA */
			case 1:
				return subrom_a[addr - 0xe000];
			/* �^�C�vB */
			case 2:
				return subrom_b[addr - 0xe000];
		}
	}

	/* CGROM */
	if (addr >= 0xd800) {
		ASSERT(cgrom_bank <= 3);
		return subromcg[cgrom_bank * 0x0800 + (addr - 0xd800)];
	}

	/* ���[�NRAM */
	if (addr >= 0xd500){
		return sub_ram[(addr - 0xd500) + 0x1380];
	}

	/*
	 *	�T�uI/O
	 */

	/* �f�B�X�v���C */
	if (display_readb(addr, &dat)) {
		return dat;
	}
	/* �L�[�{�[�h */
	if (keyboard_readb(addr, &dat)) {
		return dat;
	}
	/* �_�����Z�E������� */
	if (aluline_readb(addr, &dat)) {
		return dat;
	}

	return 0xff;
}

/*
 *	�T�uCPU������
 *	�P�o�C�g�擾(I/O�Ȃ�)
 */
BYTE FASTCALL submem_readbnio(WORD addr)
{
	/* VRAM */
	if (addr < 0xc000) {
		return vram_aptr[addr];
	}

	/* ���[�NRAM */
	if (addr < 0xd380) {
		return sub_ram[addr - 0xc000];
	}

	/* ���LRAM */
	if (addr < 0xd400) {
		return shared_ram[(WORD)(addr - 0xd380)];
	}

	/* �T�uI/O */
	if (addr < 0xd500) {
		return sub_io[addr - 0xd400];
	}

	/* ���[�NRAM */
	if (addr < 0xd800){
		return sub_ram[(addr - 0xd500) + 0x1380];
	}

	/* �T�uROM */
	if (addr >= 0xe000) {
		ASSERT(subrom_bank <= 2);
		switch (subrom_bank) {
			/* �^�C�vC */
			case 0:
				return subrom_c[addr - 0xd800];
			/* �^�C�vA */
			case 1:
				return subrom_a[addr - 0xe000];
			/* �^�C�vB */
			case 2:
				return subrom_b[addr - 0xe000];
		}
	}

	/* CGROM */
	if (addr >= 0xd800) {
		ASSERT(cgrom_bank <= 3);
		return subromcg[cgrom_bank * 0x0800 + (addr - 0xd800)];
	}

	/* �����ɂ͗��Ȃ� */
	ASSERT(FALSE);
	return 0;
}

/*
 *	�T�uCPU������
 *	�P�o�C�g��������
 */
void FASTCALL submem_writeb(WORD addr, BYTE dat)
{
	/* VRAM(�^�C�vC) */
	if (addr < 0x4000) {
		if (!(multi_page & 0x01)) {
			/* ALU�̓������������݂ł��L���B(���N�}�C�N�̈�l��) */
			if (alu_command & 0x80) {
				aluline_extrb(addr);
				return;
			}
			vram_aptr[addr] = dat;
			/* �t�b�N�֐� */
			vram_notify(addr, dat);
		}
		return;
	}
	if (addr < 0x8000) {
		if (!(multi_page & 0x02)) {
			/* ALU�̓������������݂ł��L���B(���N�}�C�N�̈�l��) */
			if (alu_command & 0x80) {
				aluline_extrb(addr);
				return;
			}
			vram_aptr[addr] = dat;
			/* �t�b�N�֐� */
			vram_notify(addr, dat);
		}
		return;
	}
	if (addr < 0xc000) {
		if (!(multi_page & 0x04)) {
			/* ALU�̓������������݂ł��L���B(���N�}�C�N�̈�l��) */
			if (alu_command & 0x80) {
				aluline_extrb(addr);
				return;
			}
			vram_aptr[addr] = dat;
			/* �t�b�N�֐� */
			vram_notify(addr, dat);
		}
		return;
	}

	/* ���[�NRAM */
	if (addr < 0xd380) {
		sub_ram[addr - 0xc000] = dat;
		return;
	}

	/* ���LRAM */
	if (addr < 0xd400) {
		shared_ram[(WORD)(addr - 0xd380)] = dat;
		return;
	}

	/* ���[�NRAM */
	if ((addr >= 0xd500) && (addr < 0xd800)) {
		sub_ram[(addr - 0xd500) + 0x1380] = dat;
		return;
	}

	/* CGROM�A�T�uROM���������݂ł��Ȃ� */
	if (addr >= 0xd800) {
		/* YAMAUCHI�ŏ���������ANMI���荞�݃n���h���ŏ������� */
		/* Thunder Force�΍� */
		return;
	}

	/*
	 *	�T�uI/O
	 */
	sub_io[addr - 0xd400] = dat;

	/* �f�B�X�v���C */
	if (display_writeb(addr, dat)) {
		return;
	}
	/* �L�[�{�[�h */
	if (keyboard_writeb(addr, dat)) {
		return;
	}
	/* �_�����Z�E������� */
	if (aluline_writeb(addr, dat)) {
		return;
	}
}

/*
 *	�T�uCPU������
 *	�Z�[�u
 */
BOOL FASTCALL submem_save(int fileh)
{
	if (!file_write(fileh, vram_c, 0x6000)) {
		return FALSE;
	}
	if (!file_write(fileh, &vram_c[0x6000], 0x6000)) {
		return FALSE;
	}
	if (!file_write(fileh, &vram_c[0xc000], 0x6000)) {
		return FALSE;
	}
	if (!file_write(fileh, &vram_c[0x12000], 0x6000)) {
		return FALSE;
	}

	if (!file_write(fileh, sub_ram, 0x1680)) {
		return FALSE;
	}

	if (!file_write(fileh, sub_io, 0x100)) {
		return FALSE;
	}

	if (!file_byte_write(fileh, subrom_bank)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, cgrom_bank)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	�T�uCPU������
 *	���[�h
 */
BOOL FASTCALL submem_load(int fileh, int ver)
{
	/* �o�[�W�����`�F�b�N */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_read(fileh, vram_c, 0x6000)) {
		return FALSE;
	}
	if (!file_read(fileh, &vram_c[0x6000], 0x6000)) {
		return FALSE;
	}
	if (!file_read(fileh, &vram_c[0xc000], 0x6000)) {
		return FALSE;
	}
	if (!file_read(fileh, &vram_c[0x12000], 0x6000)) {
		return FALSE;
	}

	if (!file_read(fileh, sub_ram, 0x1680)) {
		return FALSE;
	}

	if (!file_read(fileh, sub_io, 0x100)) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &subrom_bank)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &cgrom_bank)) {
		return FALSE;
	}

	return TRUE;
}

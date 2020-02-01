/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �_�����Z�E������� ]
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "xm7.h"
#include "aluline.h"
#include "event.h"
#include "display.h"
#include "subctrl.h"
#include "multipag.h"
#include "device.h"

/*
 *	�O���[�o�� ���[�N
 */
BYTE alu_command;						/* �_�����Z �R�}���h */
BYTE alu_color;							/* �_�����Z �J���[ */
BYTE alu_mask;							/* �_�����Z �}�X�N�r�b�g */
BYTE alu_cmpstat;						/* �_�����Z ��r�X�e�[�^�X */
BYTE alu_cmpdat[8];						/* �_�����Z ��r�f�[�^ */
BYTE alu_disable;						/* �_�����Z �֎~�o���N */
BYTE alu_tiledat[3];					/* �_�����Z �^�C���p�^�[�� */
BOOL line_busy;							/* ������� BUSY */
WORD line_offset;						/* ������� �A�h���X�I�t�Z�b�g */
WORD line_style;						/* ������� ���C���X�^�C�� */
WORD line_x0;							/* ������� X0 */
WORD line_y0;							/* ������� Y0 */
WORD line_x1;							/* ������� X1 */
WORD line_y1;							/* ������� Y1 */
BYTE line_count;						/* ������� �J�E���^ */

/*
 *	�v���g�^�C�v�錾
 */
static void FASTCALL alu_cmp(WORD addr);	/* �R���y�A */

/*
 *	�_�����Z�E�������
 *	������
 */
BOOL FASTCALL aluline_init(void)
{
	return TRUE;
}

/*
 *	�_�����Z�E�������
 *	�N���[���A�b�v
 */
void FASTCALL aluline_cleanup(void)
{
}

/*
 *	�_�����Z�E�������
 *	���Z�b�g
 */
void FASTCALL aluline_reset(void)
{
	/* �S�Ẵ��W�X�^�������� */
	alu_command = 0;
	alu_color = 0;
	alu_mask = 0;
	alu_cmpstat = 0;
	memset(alu_cmpdat, 0x80, sizeof(alu_cmpdat));
	alu_disable = 0x08;
	memset(alu_tiledat, 0, sizeof(alu_tiledat));

	line_busy = FALSE;
	line_offset = 0;
	line_style = 0;
	line_x0 = 0;
	line_y0 = 0;
	line_x1 = 0;
	line_y1 = 0;
	line_count = 0;
}

/*-[ �_�����Z ]-------------------------------------------------------------*/

/*
 *	�_�����Z
 *	VRAM�ǂݏo��
 */
static BYTE FASTCALL alu_read(WORD addr)
{
	if (addr < 0x4000) {
		if (multi_page & 0x01) {
			return 0xff;
		}
		else {
			return vram_aptr[addr];
		}
	}
	if (addr < 0x8000) {
		if (multi_page & 0x02) {
			return 0xff;
		}
		else {
			return vram_aptr[addr];
		}
	}
	if (addr < 0xc000) {
		if (multi_page & 0x04) {
			return 0xff;
		}
		else {
			return vram_aptr[addr];
		}
	}

	/* VRAM�͈̓G���[ */
	ASSERT(FALSE);
	return 0xff;
}

/*
 *	�_�����Z
 *	VRAM��������
 */
static void FASTCALL alu_write(WORD addr, BYTE dat)
{
	if (addr < 0x4000) {
		if (multi_page & 0x01) {
			return;
		}
		else {
			vram_aptr[addr] = dat;
			/* �t�b�N�֐� */
			vram_notify(addr, dat);
		}
		return;
	}
	if (addr < 0x8000) {
		if (multi_page & 0x02) {
			return;
		}
		else {
			vram_aptr[addr] = dat;
			/* �t�b�N�֐� */
			vram_notify(addr, dat);
		}
		return;
	}
	if (addr < 0xc000) {
		if (multi_page & 0x04) {
			return;
		}
		else {
			vram_aptr[addr] = dat;
			/* �t�b�N�֐� */
			vram_notify(addr, dat);
		}
		return;
	}
}

/*
 *	�_�����Z
 *	�������݃T�u(��r�������ݕt)
 */
static void FASTCALL alu_writesub(WORD addr, BYTE dat)
{
	BYTE temp;

	/* ��ɏ������݉\�� */
	if ((alu_command & 0x40) == 0) {
		alu_write(addr, dat);
		return;
	}

	/* �C�R�[���������݂��ANOT�C�R�[���������݂� */
	if (alu_command & 0x20) {
		/* NOT�C�R�[���ŏ������� */
		temp = alu_read(addr);
		temp &= alu_cmpstat;
		dat &= (BYTE)(~alu_cmpstat);
		alu_write(addr, (BYTE)(temp | dat));
	}
	else {
		/* �C�R�[���ŏ������� */
		temp = alu_read(addr);
		temp &= (BYTE)(~alu_cmpstat);
		dat &= alu_cmpstat;
		alu_write(addr, (BYTE)(temp | dat));
	}
}

/*
 *	�_�����Z
 *	PSET
 */
static void FASTCALL alu_pset(WORD addr)
{
	BYTE dat;
	BYTE mask;
	BYTE bit;
	int i;

	/* VRAM�I�t�Z�b�g���擾�A������ */
	addr &= 0x3fff;
	bit = 0x01;

	/* ��r�������ݎ��́A��ɔ�r�f�[�^���擾 */
	if (alu_command & 0x40) {
		alu_cmp(addr);
	}

	for (i=0; i<3; i++) {
		/* �����o���N�̓X�L�b�v */
		if (alu_disable & bit) {
			addr += (WORD)0x4000;
			bit <<= 1;
			continue;
		}

		/* ���Z�J���[�f�[�^���A�f�[�^�쐬 */
		if (alu_color & bit) {
			dat = 0xff;
		}
		else {
			dat = 0;
		}

		/* ���Z�Ȃ�(PSET) */
		mask = alu_read(addr);

		/* �}�X�N�r�b�g�̏��� */
		dat &= (BYTE)(~alu_mask);
		mask &= alu_mask;
		dat |= mask;

		/* �������� */
		alu_writesub(addr, dat);

		/* ���̃o���N�� */
		addr += (WORD)0x4000;
		bit <<= 1;
	}
}

/*
 *	�_�����Z
 *	PRESET
 */
static void FASTCALL alu_preset(WORD addr)
{
	BYTE dat;
	BYTE mask;
	BYTE bit;
	int i;

	/* VRAM�I�t�Z�b�g���擾�A������ */
	addr &= 0x3fff;
	bit = 0x01;

	/* ��r�������ݎ��́A��ɔ�r�f�[�^���擾 */
	if (alu_command & 0x40) {
		alu_cmp(addr);
	}

	for (i=0; i<3; i++) {
		/* �����o���N�̓X�L�b�v */
		if (alu_disable & bit) {
			addr += (WORD)0x4000;
			bit <<= 1;
			continue;
		}

		/* ���Z�J���[�f�[�^���A�f�[�^�쐬 */
		if (alu_color & bit) {
			dat = 0;
		}
		else {
			dat = 0xff;
		}

		/* ���Z�Ȃ�(PRESET) */
		mask = alu_read(addr);

		/* �}�X�N�r�b�g�̏��� */
		dat &= (BYTE)(~alu_mask);
		mask &= alu_mask;
		dat |= mask;

		/* �������� */
		alu_writesub(addr, dat);

		/* ���̃o���N�� */
		addr += (WORD)0x4000;
		bit <<= 1;
	}
}

/*
 *	�_�����Z
 *	OR
 */
static void FASTCALL alu_or(WORD addr)
{
	BYTE dat;
	BYTE mask;
	BYTE bit;
	int i;

	/* VRAM�I�t�Z�b�g���擾�A������ */
	addr &= 0x3fff;
	bit = 0x01;

	/* ��r�������ݎ��́A��ɔ�r�f�[�^���擾 */
	if (alu_command & 0x40) {
		alu_cmp(addr);
	}

	for (i=0; i<3; i++) {
		/* �����o���N�̓X�L�b�v */
		if (alu_disable & bit) {
			addr += (WORD)0x4000;
			bit <<= 1;
			continue;
		}

		/* ���Z�J���[�f�[�^���A�f�[�^�쐬 */
		if (alu_color & bit) {
			dat = 0xff;
		}
		else {
			dat = 0;
		}

		/* ���Z */
		mask = alu_read(addr);
		dat |= mask;

		/* �}�X�N�r�b�g�̏��� */
		dat &= (BYTE)(~alu_mask);
		mask &= alu_mask;
		dat |= mask;

		/* �������� */
		alu_writesub(addr, dat);

		/* ���̃o���N�� */
		addr += (WORD)0x4000;
		bit <<= 1;
	}
}

/*
 *	�_�����Z
 *	AND
 */
static void FASTCALL alu_and(WORD addr)
{
	BYTE dat;
	BYTE mask;
	BYTE bit;
	int i;

	/* VRAM�I�t�Z�b�g���擾�A������ */
	addr &= 0x3fff;
	bit = 0x01;

	/* ��r�������ݎ��́A��ɔ�r�f�[�^���擾 */
	if (alu_command & 0x40) {
		alu_cmp(addr);
	}

	for (i=0; i<3; i++) {
		/* �����o���N�̓X�L�b�v */
		if (alu_disable & bit) {
			addr += (WORD)0x4000;
			bit <<= 1;
			continue;
		}

		/* ���Z�J���[�f�[�^���A�f�[�^�쐬 */
		if (alu_color & bit) {
			dat = 0xff;
		}
		else {
			dat = 0;
		}

		/* ���Z */
		mask = alu_read(addr);
		dat &= mask;

		/* �}�X�N�r�b�g�̏��� */
		dat &= (BYTE)(~alu_mask);
		mask &= alu_mask;
		dat |= mask;

		/* �������� */
		alu_writesub(addr, dat);

		/* ���̃o���N�� */
		addr += (WORD)0x4000;
		bit <<= 1;
	}
}

/*
 *	�_�����Z
 *	XOR
 */
static void FASTCALL alu_xor(WORD addr)
{
	BYTE dat;
	BYTE mask;
	BYTE bit;
	int i;

	/* VRAM�I�t�Z�b�g���擾�A������ */
	addr &= 0x3fff;
	bit = 0x01;

	/* ��r�������ݎ��́A��ɔ�r�f�[�^���擾 */
	if (alu_command & 0x40) {
		alu_cmp(addr);
	}

	for (i=0; i<3; i++) {
		/* �����o���N�̓X�L�b�v */
		if (alu_disable & bit) {
			addr += (WORD)0x4000;
			bit <<= 1;
			continue;
		}

		/* ���Z�J���[�f�[�^���A�f�[�^�쐬 */
		if (alu_color & bit) {
			dat = 0xff;
		}
		else {
			dat = 0;
		}

		/* ���Z */
		mask = alu_read(addr);
		dat ^= mask;

		/* �}�X�N�r�b�g�̏��� */
		dat &= (BYTE)(~alu_mask);
		mask &= alu_mask;
		dat |= mask;

		/* �������� */
		alu_writesub(addr, dat);

		/* ���̃o���N�� */
		addr += (WORD)0x4000;
		bit <<= 1;
	}
}

/*
 *	�_�����Z
 *	NOT
 */
static void FASTCALL alu_not(WORD addr)
{
	BYTE dat;
	BYTE mask;
	BYTE bit;
	int i;

	/* VRAM�I�t�Z�b�g���擾�A������ */
	addr &= 0x3fff;
	bit = 0x01;

	/* ��r�������ݎ��́A��ɔ�r�f�[�^���擾 */
	if (alu_command & 0x40) {
		alu_cmp(addr);
	}

	for (i=0; i<3; i++) {
		/* �����o���N�̓X�L�b�v */
		if (alu_disable & bit) {
			addr += (WORD)0x4000;
			bit <<= 1;
			continue;
		}

		/* ���Z(NOT) */
		mask = alu_read(addr);
		dat = (BYTE)(~mask);

		/* �}�X�N�r�b�g�̏��� */
		dat &= (BYTE)(~alu_mask);
		mask &= alu_mask;
		dat |= mask;

		/* �������� */
		alu_writesub(addr, dat);

		/* ���̃o���N�� */
		addr += (WORD)0x4000;
		bit <<= 1;
	}
}

/*
 *	�_�����Z
 *	�^�C���y�C���g
 */
static void FASTCALL alu_tile(WORD addr)
{
	BYTE bit;
	BYTE mask;
	BYTE dat;
	int i;

	/* VRAM�I�t�Z�b�g���擾�A������ */
	addr &= 0x3fff;
	bit = 0x01;

	/* ��r�������ݎ��́A��ɔ�r�f�[�^���擾 */
	if (alu_command & 0x40) {
		alu_cmp(addr);
	}

	for (i=0; i<3; i++) {
		/* �����o���N�̓X�L�b�v */
		if (alu_disable & bit) {
			addr += (WORD)0x4000;
			bit <<= 1;
			continue;
		}

		/* �f�[�^�쐬 */
		dat = alu_tiledat[i];

		/* �}�X�N�r�b�g�̏��� */
		dat &= (BYTE)(~alu_mask);
		mask = alu_read(addr);
		mask &= alu_mask;
		dat |= mask;

		/* �������� */
		alu_writesub(addr, dat);

		/* ���̃o���N�� */
		addr += (WORD)0x4000;
		bit <<= 1;
	}
}

/*
 *	�_�����Z
 *	�R���y�A
 */
static void FASTCALL alu_cmp(WORD addr)
{
	BYTE color;
	BYTE bit;
	int i, j;
	BOOL flag;
	BYTE dat;
	BYTE b, r, g;
	BYTE disflag;

	/* �A�h���X�}�X�N */
	addr &= 0x3fff;

	/* �J���[�f�[�^�擾 */
	b = alu_read((WORD)(addr + 0x0000));
	r = alu_read((WORD)(addr + 0x4000));
	g = alu_read((WORD)(addr + 0x8000));

	/* �o���N�f�B�Z�[�u�����l������(���_�]���΍�) */
	disflag = (BYTE)((~alu_disable) & 0x07);

	/* ��r���K�v */
	dat = 0;
	bit = 0x80;
	for (i=0; i<8; i++) {
		/* �F���쐬 */
		color = 0;
		if (b & bit) {
			color |= 0x01;
		}
		if (r & bit) {
			color |= 0x02;
		}
		if (g & bit) {
			color |= 0x04;
		}

		/* 8�̐F�X���b�g���܂���āA�ǂꂩ��v������̂����邩 */
		flag = FALSE;
		for (j=0; j<8; j++) {
			if ((alu_cmpdat[j] & 0x80) == 0) {
				if ((alu_cmpdat[j] & disflag) == (color & disflag)) {
					flag = TRUE;
					break;
				}
			}
		}

		/* �C�R�[����1��ݒ� */
		if (flag) {
			dat |= bit;
		}

		/* ���� */
		bit >>= 1;
	}

	/* �f�[�^�ݒ� */
	alu_cmpstat = dat;
}

/*-[ ������� ]-------------------------------------------------------------*/

/*
 *	�������
 *	�_�`��
 */
static void FASTCALL aluline_pset(int x, int y)
{
	BYTE temp;
	WORD addr;
	static BYTE table[] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
	BYTE mask;

	/* �_�����Z�̃f�[�^�o�X�ɓ���̂ŁA�_�����Zon���K�v */
	if ((alu_command & 0x80) == 0) {
		return;
	}

	/* �N���b�s���O */
	if ((x < 0) || (x >= 640)) {
		return;
	}
	if ((y < 0) || (y >= 200)) {
		return;
	}

	/* ��ʃ��[�h����A�A�h���X�Z�o */
	if (mode320) {
		addr = (WORD)(y * 40 + (x >> 3));
	}
	else {
		addr = (WORD)(y * 80 + (x >> 3));
	}

	/* �I�t�Z�b�g�������� */
	addr += line_offset;
	addr &= 0x3fff;

	/* ���C���X�^�C�� */
	temp = table[line_count & 0x07];
	if (line_count < 8) {
		temp &= (BYTE)(line_style >> 8);
	}
	else {
		temp &= (BYTE)(line_style & 0xff);
	}

	/* �X�^�C���r�b�g�������Ă���ꍇ�̂� */
	if (temp != 0) {
		/* �}�X�N�̃o�b�N�A�b�v���Ƃ� */
		mask = alu_mask;

		/* �}�X�N��ݒ� */
		alu_mask = (BYTE)(~table[x & 0x07]);

		/* �_�����Z�𓮂��� */
		switch (alu_command & 0x07) {
			/* PSET */
			case 0:
				alu_pset(addr);
				break;
			/* �֎~ */
			case 1:
				alu_preset(addr);
				break;
			/* OR */
			case 2:
				alu_or(addr);
				break;
			/* AND */
			case 3:
				alu_and(addr);
				break;
			/* XOR */
			case 4:
				alu_xor(addr);
				break;
			/* NOT */
			case 5:
				alu_not(addr);
				break;
			/* �^�C���y�C���g */
			case 6:
				alu_tile(addr);
				break;
			/* �R���y�A */
			case 7:
				alu_cmp(addr);
				break;
		}

		/* �}�X�N�𕜌� */
		alu_mask = mask;
	}

	/* �J�E���g�A�b�v */
	line_count++;
	line_count &= 0x0f;
}

/*
 *	�������
 *	���W�X���b�v
 */
static void FASTCALL aluline_swap(int *a, int *b)
{
	int temp;

	temp = *a;
	*a = *b;
	*b = temp;
}

/*
 *	�������
 *	Y�����ւ̂΂�
 */
static void FASTCALL aluline_nexty(int xp, int yp, int dx, int dy, int dir)
{
	int k1, k2;
	int err;

	/* �X�e�b�v�����߂� */
	k1 = dy * 2;
	k2 = k1 - (dx * 2);
	err = k1 - dx;

	/* �`�� */
	aluline_pset(xp, yp);

	/* ���[�v */
	while (dx > 0) {
		dx --;

		if (err >= 0) {
			yp++;
			err += k2;
		}
		else {
			err += k1;
		}
		xp += dir;
		aluline_pset(xp, yp);
	}
}

/*
 *	�������
 *	X�����ւ̂΂�
 */
static void FASTCALL aluline_nextx(int xp, int yp, int dx, int dy, int dir)
{
	int k1, k2;
	int err;

	/* �X�e�b�v�����߂� */
	k1 = dx * 2;
	k2 = k1 - (dy * 2);
	err = k1 - dy;

	/* �`�� */
	aluline_pset(xp, yp);

	/* ���[�v */
	while (dy > 0) {
		dy--;

		if (err >= 0) {
			xp += dir;
			err += k2;
		}
		else {
			err += k1;
		}
		yp++;
		aluline_pset(xp, yp);
	}
}

/*
 *	�����`��
 *	�Q�l����:�uWindows95�Q�[���v���O���~���O�vBresenham���C���A���S���Y��
 */
static void FASTCALL aluline_line(void)
{
	int x1, x2, y1, y2;
	int dx, dy;
	int step;
	int i;

	/* �f�[�^�擾 */
	x1 = (int)line_x0;
	x2 = (int)line_x1;
	y1 = (int)line_y0;
	y2 = (int)line_y1;

	/* �J�E���^������ */
	line_count = 0;

	/* �P��_�`��̃`�F�b�N�Ə��� */
	if ((x1 == x2) && (y1 == y2)) {
		aluline_pset(x1, y1);
		return;
	}

	/* X������̏ꍇ */
	if (x1 == x2) {
		if (y2 > y1) {
			for (i=y1; i<=y2; i++) {
				aluline_pset(x1, i);
			}
		}
		else {
			for (i=y1; i>=y2; i--) {
				aluline_pset(x1, i);
			}
		}
		return;
	}

	/* Y������̏ꍇ */
	if (y1 == y2) {
		if (x2 > x1) {
			for (i=x1; i<=x2; i++) {
				aluline_pset(i, y1);
			}
		}
		else {
			for (i=x1; i>=x2; i--) {
				aluline_pset(i, y1);
			}
		}
		return;
	}

	/* y���W�̏㉺�𑵂��� */
	if (y1 > y2) {
		aluline_swap(&y1, &y2);
		aluline_swap(&x1, &x2);
	}

	/* ���l���v�Z */
	dx = x2 - x1;
	dy = y2 - y1;

	/* �E�����A�������͍����� */
	if (dx > 0) {
		step = 1;
	}
	else {
		dx = -dx;
		step = -1;
	}

	/* X�܂���Y�ǂ��炩�̕������d�� */
	if (dx > dy) {
		aluline_nexty(x1, y1, dx, dy, step);
	}
	else {
		aluline_nextx(x1, y1, dx, dy, step);
	}
}

/*
 *	�������
 *	�C�x���g
 */
static BOOL FASTCALL aluline_event(void)
{
	/* ������Ԃ�READY */
	line_busy = FALSE;

	schedule_delevent(EVENT_LINE);
	return TRUE;
}

/*-[ �������}�b�v�hI/O ]----------------------------------------------------*/

/*
 *	�_�����Z�E�������
 *	�P�o�C�g�ǂݏo��
 */
BOOL FASTCALL aluline_readb(WORD addr, BYTE *dat)
{
	/* �o�[�W�����`�F�b�N */
	if (fm7_ver < 2) {
		return FALSE;
	}

	switch (addr) {
		/* �_�����Z �R�}���h */
		case 0xd410:
			*dat = alu_command;
			return TRUE;

		/* �_�����Z �J���[ */
		case 0xd411:
			*dat = alu_color;
			return TRUE;

		/* �_�����Z �}�X�N�r�b�g */
		case 0xd412:
			*dat = alu_mask;
			return TRUE;

		/* �_�����Z ��r�X�e�[�^�X */
		case 0xd413:
			*dat = alu_cmpstat;
			return TRUE;

		/* �_�����Z �����o���N */
		case 0xd41b:
			*dat = alu_disable;
			return TRUE;
	}

	/* �_�����Z ��r�f�[�^ */
	if ((addr >= 0xd413) && (addr <= 0xd41a)) {
		*dat = 0xff;
		return TRUE;
	}

	/* �_�����Z �^�C���p�^�[�� */
	if ((addr >= 0xd41c) && (addr <= 0xd41e)) {
		*dat = 0xff;
		return TRUE;
	}

	/* ������� */
	if ((addr >= 0xd420) && (addr <= 0xd42b)) {
		*dat = 0xff;
		return TRUE;
	}

	return FALSE;
}

/*
 *	�_�����Z�E�������
 *	�P�o�C�g��������
 */
BOOL FASTCALL aluline_writeb(WORD addr, BYTE dat)
{
	/* �o�[�W�����`�F�b�N */
	if (fm7_ver < 2) {
		return FALSE;
	}

	switch (addr) {
		/* �_�����Z �R�}���h */
		case 0xd410:
			alu_command = dat;
			return TRUE;

		/* �_�����Z �J���[ */
		case 0xd411:
			alu_color = dat;
			return TRUE;

		/* �_�����Z �}�X�N�r�b�g */
		case 0xd412:
			alu_mask = dat;
			return TRUE;

		/* �_�����Z �����o���N */
		case 0xd41b:
			alu_disable = dat;
			return TRUE;

		/* ������� �A�h���X�I�t�Z�b�g(A1���璍��) */
		case 0xd420:
			line_offset &= 0x01fe;
			line_offset |= (WORD)((dat * 512) & 0x3e00);
			return TRUE;
		case 0xd421:
			line_offset &= 0x3e00;
			line_offset |= (WORD)(dat * 2);
			return TRUE;

		/* ������� ���C���X�^�C�� */
		case 0xd422:
			line_style &= 0xff;
			line_style |= (WORD)(dat * 256);
			return TRUE;
		case 0xd423:
			line_style &= 0xff00;
			line_style |= (WORD)dat;
			return TRUE;

		/* ������� X0 */
		case 0xd424:
			line_x0 &= 0xff;
			line_x0 |= (WORD)(dat * 256);
			return TRUE;
		case 0xd425:
			line_x0 &= 0xff00;
			line_x0 |= (WORD)dat;
			return TRUE;

		/* ������� Y0 */
		case 0xd426:
			line_y0 &= 0xff;
			line_y0 |= (WORD)(dat * 256);
			return TRUE;
		case 0xd427:
			line_y0 &= 0xff00;
			line_y0 |= (WORD)dat;
			return TRUE;

		/* ������� X1 */
		case 0xd428:
			line_x1 &= 0xff;
			line_x1 |= (WORD)(dat * 256);
			return TRUE;
		case 0xd429:
			line_x1 &= 0xff00;
			line_x1 |= (WORD)dat;
			return TRUE;

		/* ������� Y1 */
		case 0xd42a:
			line_y1 &= 0xff;
			line_y1 |= (WORD)(dat * 256);
			return TRUE;
		case 0xd42b:
			line_y1 &= 0xff00;
			line_y1 |= (WORD)dat;

			/* �����Œ�����ԃX�^�[�g */
			aluline_line();

			/* ���͈��������A���΂炭BUSY�ɂ��Ă��� */
			line_busy = TRUE;
			schedule_setevent(EVENT_LINE, 20, aluline_event);
			return TRUE;
	}

	/* �_�����Z ��r�f�[�^ */
	if ((addr >= 0xd413) && (addr <= 0xd41a)) {
		alu_cmpdat[addr - 0xd413] = dat;
		return TRUE;
	}

	/* �_�����Z �^�C���p�^�[�� */
	if ((addr >= 0xd41c) && (addr <= 0xd41e)) {
		alu_tiledat[addr - 0xd41c] = dat;
		return TRUE;
	}

	return FALSE;
}

/*
 *	�_�����Z�E�������
 *	VRAM�_�~�[���[�h
 */
void FASTCALL aluline_extrb(WORD addr)
{
	/* �_�����Z���L���� */
	if (alu_command & 0x80) {

		/* �R�}���h�� */
		switch (alu_command & 0x07) {
			/* PSET */
			case 0:
				alu_pset(addr);
				break;
			/* �֎~ */
			case 1:
				alu_preset(addr);
				break;
			/* OR */
			case 2:
				alu_or(addr);
				break;
			/* AND */
			case 3:
				alu_and(addr);
				break;
			/* XOR */
			case 4:
				alu_xor(addr);
				break;
			/* NOT */
			case 5:
				alu_not(addr);
				break;
			/* �^�C���y�C���g */
			case 6:
				alu_tile(addr);
				break;
			/* �R���y�A */
			case 7:
				alu_cmp(addr);
				break;
		}
	}
}

/*-[ �t�@�C��I/O ]----------------------------------------------------------*/

/*
 *	�_�����Z�E�������
 *	�Z�[�u
 */
BOOL FASTCALL aluline_save(int fileh)
{
	if (!file_byte_write(fileh, alu_command)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, alu_color)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, alu_mask)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, alu_cmpstat)) {
		return FALSE;
	}
	if (!file_write(fileh, alu_cmpdat, sizeof(alu_cmpdat))) {
		return FALSE;
	}
	if (!file_byte_write(fileh, alu_disable)) {
		return FALSE;
	}
	if (!file_write(fileh, alu_tiledat, sizeof(alu_tiledat))) {
		return FALSE;
	}

	if (!file_bool_write(fileh, line_busy)) {
		return FALSE;
	}

	if (!file_word_write(fileh, line_offset)) {
		return FALSE;
	}
	if (!file_word_write(fileh, line_style)) {
		return FALSE;
	}
	if (!file_word_write(fileh, line_x0)) {
		return FALSE;
	}
	if (!file_word_write(fileh, line_y0)) {
		return FALSE;
	}
	if (!file_word_write(fileh, line_x1)) {
		return FALSE;
	}
	if (!file_word_write(fileh, line_y1)) {
		return FALSE;
	}

	if (!file_byte_write(fileh, line_count)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	�_�����Z�E�������
 *	���[�h
 */
BOOL FASTCALL aluline_load(int fileh, int ver)
{
	/* �o�[�W�����`�F�b�N */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &alu_command)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &alu_color)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &alu_mask)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &alu_cmpstat)) {
		return FALSE;
	}
	if (!file_read(fileh, alu_cmpdat, sizeof(alu_cmpdat))) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &alu_disable)) {
		return FALSE;
	}
	if (!file_read(fileh, alu_tiledat, sizeof(alu_tiledat))) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &line_busy)) {
		return FALSE;
	}

	if (!file_word_read(fileh, &line_offset)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &line_style)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &line_x0)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &line_y0)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &line_x1)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &line_y1)) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &line_count)) {
		return FALSE;
	}

	/* �C�x���g */
	schedule_handle(EVENT_LINE, aluline_event);

	return TRUE;
}

/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �⏕�c�[�� ]
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "xm7.h"
#include "multipag.h"
#include "ttlpalet.h"
#include "apalet.h"
#include "subctrl.h"
#include "display.h"
#include "device.h"
#include "tools.h"

/*
 *	�f�W�^���p���b�g�e�[�u��
 *	(RGBQUAD����)
 */
static BYTE bmp_palet_table[] = {
	0x00, 0x00, 0x00, 0x00,
	0xff, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xff, 0x00,
	0xff, 0x00, 0xff, 0x00,
	0x00, 0xff, 0x00, 0x00,
	0xff, 0xff, 0x00, 0x00,
	0x00, 0xff, 0xff, 0x00,
	0xff, 0xff, 0xff, 0x00,
};

/*
 *	�u�����N�f�B�X�N�쐬 �T�u
 */
BOOL make_d77_sub(int fileh, DWORD dat)
{
	BYTE buf[4];

	buf[0] = (BYTE)(dat & 0xff);
	buf[1] = (BYTE)((dat >> 8) & 0xff);
	buf[2] = (BYTE)((dat >> 16) & 0xff);
	buf[3] = (BYTE)((dat >> 24) & 0xff);

	return file_write(fileh, buf, 4);
}

/*
 *	�u�����N�f�B�X�N�쐬
 */
BOOL make_new_d77(char *fname, char *name)
{
	int fileh;
	BYTE header[0x2b0];
	DWORD offset;
	int i;
	int j;

	/* assert */
	ASSERT(fname);

	/* �t�@�C���I�[�v�� */
	fileh = file_open(fname, OPEN_W);
	if (fileh == -1) {
		return FALSE;
	}

	/* �w�b�_�쐬 */
	memset(header, 0, sizeof(header));
	if (name != NULL) {
		for (i=0; i<16; i++) {
			if (name[i] == '\0') {
				break;
			}
			header[i] = name[i];
		}
	}
	else {
		strcpy((char*)header, "Default");
	}

	/* �w�b�_�������� */
	if (!file_write(fileh, header, 0x1c)) {
		file_close(fileh);
		return FALSE;
	}

	/* �t�@�C���g�[�^���T�C�Y */
	offset = 0x073ab0;
	if (!make_d77_sub(fileh, offset)) {
		file_close(fileh);
		return FALSE;
	}

	/* �g���b�N�I�t�Z�b�g */
	offset = 0x2b0;
	for (i=0; i<84; i++) {
		if (!make_d77_sub(fileh, offset)) {
			file_close(fileh);
			return FALSE;
		}
		offset += 0x1600;
	}

	/* �w�b�_�������� */
	if (!file_write(fileh, &header[0x170], 0x2b0 - 0x170)) {
		file_close(fileh);
		return FALSE;
	}

	/* �k���f�[�^�������� */
	memset(header, 0, sizeof(header));
	for (i=0; i<84; i++) {
		for (j=0; j<11; j++) {
			if (!file_write(fileh, header, 0x200)) {
				file_close(fileh);
				return FALSE;
			}
		}
	}

	/* ok */
	file_close(fileh);
	return TRUE;
}

/*
 *	�u�����N�e�[�v�쐬
 */
BOOL make_new_t77(char *fname)
{
	int fileh;

	ASSERT(fname);

	/* �t�@�C���I�[�v�� */
	fileh = file_open(fname, OPEN_W);
	if (fileh == -1) {
		return FALSE;
	}

	/* �w�b�_�������� */
	if (!file_write(fileh, (BYTE*)"XM7 TAPE IMAGE 0", 16)) {
		file_close(fileh);
		return FALSE;
	}

	/* ���� */
	file_close(fileh);
	return TRUE;
}

/*
 *	VFD��D77�ϊ�
 */
BOOL conv_vfd_to_d77(char *src, char *dst, char *name)
{
	int files;
	int filed;
	BYTE vfd_h[480];
	BYTE d77_h[0x2b0];
	BYTE *buffer;
	int trk;
	int sec;
	int secs;
	int len;
	int trklen;
	DWORD offset;
	DWORD srclen;
	DWORD wrlen;
	BYTE *header;
	BYTE *ptr;

	/* assert */
	ASSERT(src);
	ASSERT(dst);
	ASSERT(name);

	/* ���[�N�������m�� */
	buffer = (BYTE *)malloc(8192);
	if (buffer == NULL) {
		return FALSE;
	}

	/* VFD�t�@�C���I�[�v�� */
	files = file_open(src, OPEN_R);
	if (files == -1) {
		free(buffer);
		return FALSE;
	}

	/* �����ŁA�t�@�C���T�C�Y���擾���Ă��� */
	srclen = file_getsize(files);

	/* VFD�w�b�_�ǂݍ��� */
	if (!file_read(files, vfd_h, sizeof(vfd_h))) {
		free(buffer);
		file_close(files);
		return FALSE;
	}

	/* D77�t�@�C���쐬 */
	filed = file_open(dst, OPEN_W);
	if (filed == -1) {
		free(buffer);
		file_close(files);
		return FALSE;
	}

	/* �w�b�_�쐬 */
	memset(d77_h, 0, sizeof(d77_h));
	if (strlen(name) <= 16) {
		strcpy((char*)d77_h, name);
	}
	else {
		memcpy(d77_h, name, 16);
	}

	/* ��U�A�w�b�_������ */
	if (!file_write(filed, d77_h, sizeof(d77_h))) {
		free(buffer);
		file_close(files);
		file_close(filed);
		return FALSE;
	}

	/* �������݃|�C���^�������� */
	wrlen = sizeof(d77_h);

	/* �g���b�N���[�v */
	header = vfd_h;
	for (trk=0; trk<80; trk++) {
		/* �w�b�_�擾 */
		offset = header[3];
		offset *= 256;
		offset |= header[2];
		offset *= 256;
		offset |= header[1];
		offset *= 256;
		offset |= header[0];
		header += 4;
		len = *header++;
		len &= 3;
		secs = *header++;

		/* secs=0�ւ̑Ή� */
		if (secs == 0) {
			continue;
		}
		else {
			/* �������݃|�C���^���L�� */
			d77_h[trk * 4 + 0x20 + 3] = (BYTE)(wrlen >> 24);
			d77_h[trk * 4 + 0x20 + 2] = (BYTE)((wrlen >> 16) & 255);
			d77_h[trk * 4 + 0x20 + 1] = (BYTE)((wrlen >> 8) & 255);
			d77_h[trk * 4 + 0x20 + 0] = (BYTE)(wrlen & 255);
		}

		/* �g���b�N�����v�Z */
		switch (len) {
			case 0:
				trklen = secs * (128 + 16);
				break;
			case 1:
				trklen = secs * (256 + 16);
				break;
			case 2:
				trklen = secs * (512 + 16);
				break;
			case 3:
				trklen = secs * (1024 + 16);
				break;
		}

		/* �w�b�_���� */
		if ((offset > srclen) | (trklen > 8192)) {
			free(buffer);
			file_close(files);
			file_close(filed);
			return FALSE;
		}

		/* �V�[�N */
		if (!file_seek(files, offset)) {
			free(buffer);
			file_close(files);
			file_close(filed);
			return FALSE;
		}

		/* �Z�N�^���[�v */
		ptr = buffer;
		for (sec=1; sec<=secs; sec++) {
			memset(ptr, 0, 0x10);
			/* C,H,R,N */
			ptr[0] = (BYTE)(trk >> 1);
			ptr[1] = (BYTE)(trk & 1);
			ptr[2] = (BYTE)sec;
			ptr[3] = (BYTE)len;
			/* �Z�N�^�� */
			ptr[4] = (BYTE)(secs);
			/* �Z�N�^�����f�[�^�ǂݍ��� */
			switch (len) {
				case 0:
					ptr[0x0e] = 0x80;
					ptr += 0x10;
					file_read(files, ptr, 128);
					ptr += 128;
					break;
				case 1:
					ptr[0x0f] = 0x01;
					ptr += 0x10;
					file_read(files, ptr, 256);
					ptr += 256;
					break;
				case 2:
					ptr[0x0f] = 0x02;
					ptr += 0x10;
					file_read(files, ptr, 512);
					ptr += 512;
					break;
				case 3:
					ptr[0x0f] = 0x04;
					ptr += 0x10;
					file_read(files, ptr, 1024);
					ptr += 1024;
					break;
			}
		}

		/* �ꊇ�������� */
		if (!file_write(filed, buffer, trklen)) {
			free(buffer);
			file_close(files);
			file_close(filed);
			return FALSE;
		}

		/* �������݃|�C���^��i�߂� */
		wrlen += trklen;
	}

	/* �t�@�C���T�C�Y�ݒ� */
	d77_h[0x1f] = (BYTE)((wrlen >> 24) & 0xff);
	d77_h[0x1e] = (BYTE)((wrlen >> 16) & 0xff);
	d77_h[0x1d] = (BYTE)((wrlen >> 8) & 0xff);
	d77_h[0x1c] = (BYTE)(wrlen & 0xff);

	/* �ēx�A�w�b�_����������� */
	if (!file_seek(filed, 0)) {
		free(buffer);
		file_close(files);
		file_close(filed);
		return FALSE;
	}
	if (!file_write(filed, d77_h, sizeof(d77_h))) {
		free(buffer);
		file_close(files);
		file_close(filed);
		return FALSE;
	}

	/* ���ׂďI�� */
	free(buffer);
	file_close(files);
	file_close(filed);
	return TRUE;
}

/*
 *	2D��D77�ϊ�
 */
BOOL conv_2d_to_d77(char *src, char *dst, char *name)
{
	int files;
	int filed;
	BYTE d77_h[0x2b0];
	BYTE *buffer;
	BYTE *ptr;
	DWORD offset;
	int trk;
	int sec;

	/* assert */
	ASSERT(src);
	ASSERT(dst);
	ASSERT(name);

	/* ���[�N�������m�� */
	buffer = (BYTE *)malloc(0x1100);
	if (buffer == NULL) {
		return FALSE;
	}

	/* 2D�t�@�C���I�[�v���A�t�@�C���T�C�Y�`�F�b�N */
	files = file_open(src, OPEN_R);
	if (files == -1) {
		free(buffer);
		return FALSE;
	}
	if (file_getsize(files) != 327680) {
		file_close(files);
		free(buffer);
		return FALSE;
	}

	/* D77�t�@�C���쐬 */
	filed = file_open(dst, OPEN_W);
	if (filed == -1) {
		free(buffer);
		file_close(files);
		return FALSE;
	}

	/* �w�b�_�쐬 */
	memset(d77_h, 0, sizeof(d77_h));
	if (strlen(name) <= 16) {
		strcpy((char*)d77_h, name);
	}
	else {
		memcpy(d77_h, name, 16);
	}

	/* �t�@�C���T�C�Y */
	d77_h[0x1c] = 0xb0;
	d77_h[0x1d] = 0x52;
	d77_h[0x1e] = 0x05;

	/* �g���b�N�I�t�Z�b�g */
	offset = 0x2b0;
	for (trk=0; trk<80; trk++) {
		d77_h[0x20 + trk * 4 + 0] = (BYTE)(offset & 0xff);
		d77_h[0x20 + trk * 4 + 1] = (BYTE)((offset >> 8) & 0xff);
		d77_h[0x20 + trk * 4 + 2] = (BYTE)((offset >> 16) & 0xff);
		d77_h[0x20 + trk * 4 + 3] = (BYTE)((offset >> 24) & 0xff);
		offset += 0x1100;
	}

	/* �w�b�_�������� */
	if (!file_write(filed, d77_h, sizeof(d77_h))) {
		free(buffer);
		file_close(files);
		file_close(filed);
		return FALSE;
	}

	/* �g���b�N���[�v */
	for (trk=0; trk<80; trk++) {
		ptr = buffer;
		/* �Z�N�^���[�v */
		for (sec=1; sec<=16; sec++) {
			memset(ptr, 0, 0x10);
			/* C,H,R,N */
			ptr[0] = (BYTE)(trk >> 1);
			ptr[1] = (BYTE)(trk & 1);
			ptr[2] = (BYTE)sec;
			ptr[3] = 1;

			/* �Z�N�^���A�����O�X */
			ptr[4] = 16;
			ptr[0x0f] = 0x01;
			ptr += 0x10;

			/* �f�[�^�ǂݍ��� */
			file_read(files, ptr, 256);
			ptr += 256;
		}

		/* �ꊇ�������� */
		if (!file_write(filed, buffer, 0x1100)) {
			free(buffer);
			file_close(files);
			file_close(filed);
			return FALSE;
		}
	}

	/* ���ׂďI�� */
	free(buffer);
	file_close(files);
	file_close(filed);
	return TRUE;
}

/*
 *	VTP��T77�ϊ�
 */
BOOL conv_vtp_to_t77(char *src, char *dst)
{
	int files;
	int filed;
	DWORD fsize;
	DWORD l;
	int i;
	BYTE buf[44];
	char *header = "XM7 TAPE IMAGE 0";
	BYTE dat;

	/* assert */
	ASSERT(src);
	ASSERT(dst);

	/* VTP�t�@�C���I�[�v�� */
	files = file_open(src, OPEN_R);
	if (files == -1) {
		return FALSE;
	}
	fsize = file_getsize(files);

	/* T77�t�@�C���쐬 */
	filed = file_open(dst, OPEN_W);
	if (filed == -1) {
		file_close(files);
		return FALSE;
	}

	/* �w�b�_�A�}�[�J�������� */
	if (!file_write(filed, (BYTE*)header, 16)) {
		file_close(filed);
		file_close(files);
		return FALSE;
	}
	buf[0] = 0;
	buf[1] = 0;
	if (!file_write(filed, buf, 2)) {
		file_close(filed);
		file_close(files);
		return FALSE;
	}
	buf[0] = 0x7f;
	buf[1] = 0xff;
	if (!file_write(filed, buf, 2)) {
		file_close(filed);
		file_close(files);
		return FALSE;
	}

	/* �X�^�[�g�r�b�g�ݒ� */
	buf[0] = 0x00;
	buf[1] = 0x34;
	buf[2] = 0x80;
	buf[3] = 0x1a;
	buf[4] = 0x00;
	buf[5] = 0x1a;

	/* �X�g�b�v�r�b�g�ݒ� */
	buf[38] = 0x80;
	buf[39] = 0x2f;
	buf[40] = 0x00;
	buf[41] = 0x37;
	buf[42] = 0x80;
	buf[43] = 0x2f;

	/* �o�C�g���[�v */
	for (l=0; l<fsize; l++) {
		/* �f�[�^�ǂݍ��� */
		if (!file_read(files, &dat, 1)) {
			file_close(filed);
			file_close(files);
			return FALSE;
		}

		/* 8�r�b�g���� */
		for (i=0; i<8; i++) {
			if (dat & 0x80) {
				buf[i * 4 + 6 + 0] = 0x80;
				buf[i * 4 + 6 + 1] = 0x18;
				buf[i * 4 + 6 + 2] = 0x00;
				buf[i * 4 + 6 + 3] = 0x1a;
			}
			else {
				buf[i * 4 + 6 + 0] = 0x80;
				buf[i * 4 + 6 + 1] = 0x30;
				buf[i * 4 + 6 + 2] = 0x00;
				buf[i * 4 + 6 + 3] = 0x30;
			}
			dat <<= 1;
		}

		/* 44�o�C�g�Ɋg�債�ď������� */
		if (!file_write(filed, buf, 44)) {
			file_close(filed);
			file_close(files);
			return FALSE;
		}
	}

	/* ���ׂďI�� */
	file_close(files);
	file_close(filed);
	return TRUE;
}

/*
 *	BMP�w�b�_��������
 */
static BOOL bmp_header_sub(int fileh)
{
	BYTE filehdr[14];
	BYTE infohdr[40];

	ASSERT(fileh >= 0);

	/* �\���̃N���A */
	memset(filehdr, 0, sizeof(filehdr));
	memset(infohdr, 0, sizeof(infohdr));

	/* BITMAPFILEHEADER */
	filehdr[0] = 'B';
	filehdr[1] = 'M';
	if (mode320) {
		/* �t�@�C���T�C�Y 14+40+512000 */
		filehdr[2] = 0x36;
		filehdr[3] = 0xd0;
		filehdr[4] = 0x07;
	}
	else {
		/* �t�@�C���T�C�Y 14+40+16*4+128000 */
		filehdr[2] = 0x76;
		filehdr[3] = 0xf4;
		filehdr[4] = 0x01;
	}
	/* �r�b�g�}�b�v�ւ̃I�t�Z�b�g */
	if (mode320) {
		filehdr[10] = 14 + 40;
	}
	else {
		filehdr[10] = 14 + 40 + (16 * 4);
	}

	/* BITMAPFILEHEADER �������� */
	if (!file_write(fileh, filehdr, sizeof(filehdr))) {
		return FALSE;
	}

	/* BITMAPINFOHEADER */
	infohdr[0] = 40;
	infohdr[4] = 0x80;
	infohdr[5] = 0x02;
	infohdr[8] = 0x90;
	infohdr[9] = 0x01;
	infohdr[12] = 0x01;
	/* BiBitCount */
	if (mode320) {
		infohdr[14] = 16;
	}
	else {
		infohdr[14] = 4;
	}

	/* BITMAPFILEHEADER �������� */
	if (!file_write(fileh, infohdr, sizeof(infohdr))) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	BMP�p���b�g��������
 */
static BOOL bmp_palette_sub(int fileh)
{
	int i;
	BYTE *p;
	int vpage;

	ASSERT(fileh >= 0);

	/* �\���y�[�W���l�� */
	vpage = (~(multi_page >> 4)) & 0x07;

	/* �p���b�g���8�F */
	for (i=0; i<8; i++) {
		if (crt_flag) {
			p = &bmp_palet_table[ ttl_palet[i & vpage] * 4 ];
		}
		else {
			p = bmp_palet_table;
		}
		if (!file_write(fileh, p, 4)) {
			return FALSE;
		}
	}

	/* ������8�F */
	p = bmp_palet_table;
	for (i=0; i<8; i++) {
		if (!file_write(fileh, p, 4)) {
			return FALSE;
		}
	}

	return TRUE;
}

/*
 *  BMP�f�[�^��������(320���[�h)
 */
static BOOL bmp_320_sub(int fileh)
{
	int x, y;
	int offset;
	int i;
	BYTE buffer[1280];
	int dat;
	BYTE bit;
	WORD color;
	int mask;

	ASSERT(fileh >= 0);

	/* �����I�t�Z�b�g�ݒ� */
	offset = 40 * 199;

	/* �}�X�N�擾 */
	mask = 0;
	if (!(multi_page & 0x10)) {
		mask |= 0x000f;
	}
	if (!(multi_page & 0x20)) {
		mask |= 0x00f0;
	}
	if (!(multi_page & 0x40)) {
		mask |= 0x0f00;
	}

	/* y���[�v */
	for (y=0;y<200; y++) {
		/* 0�ŏ������� */
		memset(buffer, 0, sizeof(buffer));
		if (!file_write(fileh, buffer, sizeof(buffer))) {
			return FALSE;
		}

		/* x���[�v */
		for (x=0; x<40; x++) {
			bit = 0x80;
			/* �r�b�g���[�v */
			for (i=0; i<8; i++) {
				dat = 0;

				/* G�]�� */
				if (vram_c[offset + 0x8000] & bit) {
					dat |= 0x800;
				}
				if (vram_c[offset + 0xa000] & bit) {
					dat |= 0x400;
				}
				if (vram_b[offset + 0x8000] & bit) {
					dat |= 0x200;
				}
				if (vram_b[offset + 0xa000] & bit) {
					dat |= 0x100;
				}

				/* R�]�� */
				if (vram_c[offset + 0x4000] & bit) {
					dat |= 0x080;
				}
				if (vram_c[offset + 0x6000] & bit) {
					dat |= 0x040;
				}
				if (vram_b[offset + 0x4000] & bit) {
					dat |= 0x020;
				}
				if (vram_b[offset + 0x6000] & bit) {
					dat |= 0x010;
				}

				/* B�]�� */
				if (vram_c[offset + 0x0000] & bit) {
					dat |= 0x008;
				}
				if (vram_c[offset + 0x2000] & bit) {
					dat |= 0x004;
				}
				if (vram_b[offset + 0x0000] & bit) {
					dat |= 0x002;
				}
				if (vram_b[offset + 0x2000] & bit) {
					dat |= 0x001;
				}

				/* �A�i���O�p���b�g���f�[�^�擾 */
				dat &= mask;
				color = apalet_r[dat];
				color <<= 1;
				if (apalet_r[dat] > 0) {
					color |= 1;
				}
				color <<= 4;

				color |= apalet_g[dat];
				color <<= 1;
				if (apalet_g[dat] > 0) {
					color |= 1;
				}
				color <<= 4;

				color |= apalet_b[dat];
				color <<= 1;
				if (apalet_b[dat] > 0) {
					color |= 1;
				}

				/* CRT�t���O */
				if (!crt_flag) {
					color = 0;
				}

				/* �Q�񑱂��ē������̂��������� */
				buffer[x * 32 + i * 4 + 0] = (BYTE)(color & 255);
				buffer[x * 32 + i * 4 + 1] = (BYTE)(color >> 8);
				buffer[x * 32 + i * 4 + 2] = (BYTE)(color & 255);
				buffer[x * 32 + i * 4 + 3] = (BYTE)(color >> 8);

				/* ���̃r�b�g�� */
				bit >>= 1;
			}
			offset++;
		}

		/* �������� */
		if (!file_write(fileh, buffer, sizeof(buffer))) {
			return FALSE;
		}

		/* ����y��(�߂�) */
		offset -= (40 * 2);
	}

	return TRUE;
}

 /*
 *  BMP�f�[�^��������(640���[�h)
 */
static BOOL bmp_640_sub(int fileh)
{
	int x, y;
	int i;
	int offset;
	BYTE bit;
	BYTE buffer[320];

	ASSERT(fileh >= 0);

	/* �����I�t�Z�b�g�ݒ� */
	offset = 80 * 199;

	/* y���[�v */
	for (y=0;y<200; y++) {
		/* �J���[9�ŏ������� */
		memset(buffer, 0x99, sizeof(buffer));
		if (!file_write(fileh, buffer, sizeof(buffer))) {
			return FALSE;
		}

		/* ��U�N���A */
		memset(buffer, 0, sizeof(buffer));

		/* x���[�v */
		for (x=0; x<80; x++) {
			bit = 0x80;
			/* bit���[�v */
			for (i=0; i<4; i++) {
				if (vram_dptr[offset + 0x0000] & bit) {
					buffer[x * 4 + i] |= 0x10;
				}
				if (vram_dptr[offset + 0x4000] & bit) {
					buffer[x * 4 + i] |= 0x20;
				}
				if (vram_dptr[offset + 0x8000] & bit) {
					buffer[x * 4 + i] |= 0x40;
				}
				bit >>= 1;

				if (vram_dptr[offset + 0x0000] & bit) {
					buffer[x * 4 + i] |= 0x01;
				}
				if (vram_dptr[offset + 0x4000] & bit) {
					buffer[x * 4 + i] |= 0x02;
				}
				if (vram_dptr[offset + 0x8000] & bit) {
					buffer[x * 4 + i] |= 0x04;
				}
				bit >>= 1;
			}
			offset++;
		}

		/* �������� */
		if (!file_write(fileh, buffer, sizeof(buffer))) {
			return FALSE;
		}

		/* ����y��(�߂�) */
		offset -= (80 * 2);
	}

	return TRUE;
}

/*
 *	��ʃL���v�`��(BMP)
 */
BOOL capture_to_bmp(char *fname)
{
	int fileh;

	ASSERT(fname);

	/* �t�@�C���I�[�v�� */
	fileh = file_open(fname, OPEN_W);
	if (fileh == -1) {
		return FALSE;
	}

	/* �w�b�_�������� */
	if (!bmp_header_sub(fileh)) {
		file_close(fileh);
		return FALSE;
	}

	/* �p���b�g�������� */
	if (!mode320) {
		if (!bmp_palette_sub(fileh)) {
			file_close(fileh);
			return FALSE;
		}
	}

	/* �{�̏������� */
	if (mode320) {
		if (!bmp_320_sub(fileh)) {
			file_close(fileh);
			return FALSE;
		}
	}
	else {
		if (!bmp_640_sub(fileh)) {
			file_close(fileh);
			return FALSE;
		}
	}

	/* ���� */
	file_close(fileh);
	return TRUE;
}


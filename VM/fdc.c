/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �t���b�s�[�f�B�X�N �R���g���[��(MB8877A) ]
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "xm7.h"
#include "device.h"
#include "fdc.h"
#include "mainetc.h"

/*
 *	�O���[�o�� ���[�N
 */
BYTE fdc_command;					/* FDC�R�}���h */
BYTE fdc_status;					/* FDC�X�e�[�^�X */
BYTE fdc_trkreg;					/* �g���b�N���W�X�^ */
BYTE fdc_secreg;					/* �Z�N�^���W�X�^ */
BYTE fdc_datareg;					/* �f�[�^���W�X�^ */
BYTE fdc_sidereg;					/* �T�C�h���W�X�^ */
BYTE fdc_drvreg;					/* �h���C�u */
BYTE fdc_motor;						/* ���[�^ */
BYTE fdc_drqirq;					/* DRQ�����IRQ */

BYTE fdc_cmdtype;					/* �R�}���h�^�C�v */
WORD fdc_totalcnt;					/* �g�[�^���J�E���^ */
WORD fdc_nowcnt;					/* �J�����g�J�E���^ */
BYTE fdc_ready[FDC_DRIVES];			/* ���f�B��� */
BOOL fdc_teject[FDC_DRIVES];		/* �ꎞ�C�W�F�N�g */
BOOL fdc_writep[FDC_DRIVES];		/* ���C�g�v���e�N�g��� */
BYTE fdc_track[FDC_DRIVES];			/* ���g���b�N */

char fdc_fname[FDC_DRIVES][128+1];	/* �t�@�C���� */
BOOL fdc_fwritep[FDC_DRIVES];		/* ���C�g�v���e�N�g���(�t�@�C�����x��) */
BYTE fdc_header[FDC_DRIVES][0x2b0];	/* D77�t�@�C���w�b�_ */
BYTE fdc_media[FDC_DRIVES];			/* ���f�B�A���� */
BYTE fdc_medias[FDC_DRIVES];		/* ���f�B�A�Z���N�g��� */
char fdc_name[FDC_DRIVES][FDC_MEDIAS][17];
BYTE fdc_access[FDC_DRIVES];		/* �A�N�Z�XLED */

/*
 *	�X�^�e�B�b�N ���[�N
 */
static BYTE fdc_buffer[0x2000];					/* �f�[�^�o�b�t�@ */
static BYTE *fdc_dataptr;						/* �f�[�^�|�C���^ */
static DWORD fdc_seekofs[FDC_DRIVES];			/* �V�[�N�I�t�Z�b�g */
static DWORD fdc_secofs[FDC_DRIVES];			/* �Z�N�^�I�t�Z�b�g */
static DWORD fdc_foffset[FDC_DRIVES][FDC_MEDIAS];
static WORD fdc_trklen[FDC_DRIVES];				/* �g���b�N�f�[�^���� */
static BOOL fdc_seekvct;						/* �V�[�N����(Trk0:TRUE) */
static BYTE fdc_indexcnt;						/* INDEX�z�[�� �J�E���^ */
static BOOL fdc_boot;							/* �u�[�g�t���O */

/*
 *	�v���g�^�C�v�錾
 */
void FASTCALL fdc_readbuf(int drive);	/* �P�g���b�N���ǂݍ��� */

/*
 *	FDC
 *	������
 */
BOOL FASTCALL fdc_init(void)
{
	int i;

	/* �t���b�s�[�t�@�C���֌W�����Z�b�g */
	for (i=0; i<FDC_DRIVES; i++) {
		fdc_ready[i] = FDC_TYPE_NOTREADY;
		fdc_teject[i] = FALSE;
		fdc_fwritep[i] = FALSE;
		fdc_medias[i] = 0;
	}
	/* �t�@�C���I�t�Z�b�g��S�ăN���A */
	memset(fdc_foffset, 0, sizeof(fdc_foffset));

	return TRUE;
}

/*
 *	FDC
 *	�N���[���A�b�v
 */
void FASTCALL fdc_cleanup(void)
{
	int i;

	/* �t���b�s�[�t�@�C���֌W�����Z�b�g */
	for (i=0; i<FDC_DRIVES; i++) {
		fdc_ready[i] = FDC_TYPE_NOTREADY;
	}
}

/*
 *	FDC
 *	���Z�b�g
 */
void FASTCALL fdc_reset(void)
{
	/* FDC�������W�X�^�����Z�b�g */
	fdc_command = 0xff;
	fdc_status = 0;
	fdc_trkreg = 0;
	fdc_secreg = 0;
	fdc_datareg = 0;
	fdc_sidereg = 0;
	fdc_drvreg = 0;
	fdc_motor = 0;

	fdc_cmdtype = 0;
	fdc_seekvct = 0;
	memset(fdc_track, 0, sizeof(fdc_track));
	fdc_dataptr = NULL;
	memset(fdc_access, 0, sizeof(fdc_access));
	fdc_boot = FALSE;

	/* �f�[�^�o�b�t�@�֓ǂݍ��� */
	fdc_readbuf(fdc_drvreg);
}

/*-[ CRC�v�Z ]--------------------------------------------------------------*/

/*
 *	CRC�v�Z�e�[�u��
 */
static WORD crc_table[256] = {
	0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
	0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
	0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
	0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
	0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
	0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
	0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
	0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
	0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
	0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
	0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
	0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
	0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
	0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
	0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
	0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
	0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
	0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
	0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
	0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
	0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
	0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
	0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
	0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
	0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
	0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
	0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
	0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
	0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
	0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
	0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
	0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
};

/*
 *	16bit CRC���v�Z���A�Z�b�g
 */
static void FASTCALL calc_crc(BYTE *addr, int size)
{
	WORD crc;

	/* ������ */
	crc = 0;

	/* �v�Z */
	while (size > 0) {
		crc = (WORD)((crc << 8) ^ crc_table[(BYTE)(crc >> 8) ^ (BYTE)*addr++]);
		size--;
	}

	/* �����Q�o�C�g�ɃZ�b�g(�r�b�O�G���f�B�A��) */
	*addr++ = (BYTE)(crc >> 8);
	*addr = (BYTE)(crc & 0xff);
}

/*
 *	�������v�Z
 */
static BYTE FASTCALL calc_rand(void)
{
	static WORD rand_s = 0x7f28;
	WORD tmp1, tmp2, tmp3;

	tmp1 = rand_s;
	tmp2 = (WORD)(tmp1 & 255);
	tmp1 = (WORD)((tmp1 << 1) + 1 + rand_s);
	tmp3 = (WORD)(((tmp1 >> 8) + tmp2) & 255);
	tmp1 &= 255;
	rand_s = (WORD)((tmp3 << 8) | tmp1);

	return (BYTE)tmp3;
}

/*-[ �t�@�C���Ǘ� ]---------------------------------------------------------*/

/*
 *	Read Track�f�[�^�쐬
 */
static void FASTCALL fdc_make_track(void)
{
	int i;
	int j;
	int gap3;
	WORD count;
	WORD secs;
	WORD size;
	BOOL flag;
	BYTE *p;
	BYTE *q;

	/* �A���t�H�[�}�b�g�`�F�b�N */
	flag = FALSE;
	if (fdc_ready[fdc_drvreg] == FDC_TYPE_2D) {
		if (fdc_track[fdc_drvreg] > 79) {
			/* �A���t�H�[�}�b�g */
			flag = TRUE;
		}
	}
	else {
		if ((fdc_buffer[4] == 0) && (fdc_buffer[5] == 0)) {
			/* �A���t�H�[�}�b�g */
			flag = TRUE;
		}
	}

	/* �A���t�H�[�}�b�g�Ȃ�A�����_���f�[�^�쐬 */
	if (flag) {
		p = fdc_buffer;
		for (i=0; i<0x1800; i++) {
			*p++ = calc_rand();
		}

		/* �f�[�^�|�C���^�A�J�E���^�ݒ� */
		fdc_dataptr = fdc_buffer;
		fdc_totalcnt = 0x1800;
		fdc_nowcnt = 0;
		return;
	}

	/* �Z�N�^���Z�o�A�f�[�^�ړ� */
	if (fdc_ready[fdc_drvreg] == FDC_TYPE_2D) {
		secs = 16;
		q = &fdc_buffer[0x1000];
		memcpy(q, fdc_buffer, 0x1000);
	}
	else {
		secs = (WORD)(fdc_buffer[0x0005] * 256);
		secs += (WORD)(fdc_buffer[0x0004]);
		count = 0;
		/* �S�Z�N�^�܂���āA�T�C�Y���v�� */
		for (j=0; j<secs; j++) {
			p = &fdc_buffer[count];
			count += (WORD)((p[0x000f] * 256 + p[0x000e]));
			count += (WORD)0x10;
		}
		/* �f�[�^�R�s�[ */
		q = &fdc_buffer[0x2000 - count];
		for (j=(count-1); j>0; j--) {
			q[j] = fdc_buffer[j];
		}
	}

	/* GAP3���� */
	if (secs <= 5) {
		gap3 = 0x74;
	}
	else {
		if (secs <= 10) {
			gap3 = 0x54;
		}
		else {
			if (secs <= 16) {
				gap3 = 0x33;
			}
			else {
				gap3 = 0x10;
			}
		}
	}

	/* �o�b�t�@������ */
	p = fdc_buffer;
	count = 0;

	/* GAP0 */
	for (i=0; i<80; i++) {
		*p++ = 0x4e;
	}
	count += (WORD)80;

	/* SYNC */
	for (i=0; i<12; i++) {
		*p++ = 0;
	}
	count += (WORD)12;

	/* INDEX MARK */
	*p++ = 0xc2;
	*p++ = 0xc2;
	*p++ = 0xc2;
	*p++ = 0xfc;
	count += (WORD)4;

	/* GAP1 */
	for (i=0; i<50; i++) {
		*p++ = 0x4e;
	}
	count += (WORD)50;

	/* �Z�N�^���[�v */
	for (j=0; j<secs; j++) {
		/* SYNC */
		for (i=0; i<12; i++) {
			*p++ = 0;
		}
		count += (WORD)12;

		/* ID ADDRESS MARK */
		*p++ = 0xa1;
		*p++ = 0xa1;
		*p++ = 0xa1;
		*p++ = 0xfe;
		count += (WORD)4;

		/* ID */
		if (fdc_ready[fdc_drvreg] == FDC_TYPE_2D) {
			p[0] = fdc_track[fdc_drvreg];
			p[1] = fdc_sidereg;
			p[2] = (BYTE)(j + 1);
			p[3] = 1;
			size = 0x100;
		}
		else {
			memcpy(p, q, 4);
			size = (WORD)(q[0x000f] * 256 + q[0x000e]);
			q += 0x0010;
		}
		calc_crc(p, 4);
		p += (4 + 2);
		count += (WORD)(4 + 2);

		/* GAP2 */
		for (i=0; i<22; i++) {
			*p++ = 0x4e;
		}
		count += (WORD)22;

		/* �f�[�^ */
		memcpy(p, q, size);
		q += size;
		calc_crc(p, size);
		p += (size + 2);
		count += (WORD)(size + 2);

		/* GAP3 */
		for (i=0; i<gap3; i++) {
			*p++ = 0x4e;
		}
		count += (WORD)gap3;
	}

	/* GAP4 */
	j = (0x1800 - count);
	if (j < 0x1800) {
		for (i=0; i<j; i++) {
			*p++ = 0x4e;
		}
		count += (WORD)j;
	}

	/* �f�[�^�|�C���^�A�J�E���^�ݒ� */
	fdc_dataptr = fdc_buffer;
	fdc_totalcnt = count;
	fdc_nowcnt = 0;
}

/*
 *	ID�t�B�[���h���o�b�t�@�ɍ��
 *	�J�E���^�A�f�[�^�|�C���^��ݒ�
 */
static void FASTCALL fdc_makeaddr(int index)
{
	int i;
	BYTE *p;
	WORD offset;
	WORD size;

	/* �]���̂��߂̃e���|�����o�b�t�@�́A�ʏ�o�b�t�@�� */
	/* �Ō����ׂ��Đ݂��� */
	p = &fdc_buffer[0x1ff0];

	if (fdc_ready[fdc_drvreg] == FDC_TYPE_2D) {
		/* 2D�̏ꍇ�AC,H,R,N�͊m�肷�� */
		p[0] = fdc_track[fdc_drvreg];
		p[1] = fdc_sidereg;
		p[2] = (BYTE)(index + 1);
		p[3] = 1;
	}
	else {
		/* �o�b�t�@��ړI�̃Z�N�^�܂Ői�߂� */
		offset = 0;
		i = 0;
		while (i < index) {
			/* �Z�N�^�T�C�Y���擾 */
			size = fdc_buffer[offset + 0x000f];
			size *= (WORD)256;
			size |= fdc_buffer[offset + 0x000e];

			/* ���̃Z�N�^�֐i�߂� */
			offset += size;
			offset += (WORD)0x10;

			i++;
		}

		/* C,H,R,N�R�s�[ */
		memcpy(p, &fdc_buffer[offset], 4);
	}

	/* CRC */
	calc_crc(p, 4);

	/* �f�[�^�|�C���^�A�J�E���^�ݒ� */
	fdc_dataptr = &fdc_buffer[0x1ff0];
	fdc_totalcnt = 6;
	fdc_nowcnt = 0;
}

/*
 *	�C���f�b�N�X�J�E���^�����̃Z�N�^�ֈڂ�
 */
static int FASTCALL fdc_next_index(void)
{
	int secs;

	ASSERT(fdc_ready[fdc_drvreg] != FDC_TYPE_NOTREADY);

	/* �f�B�X�N�^�C�v���`�F�b�N */
	if (fdc_ready[fdc_drvreg] == FDC_TYPE_2D) {
		/* 2D�Ȃ�16�Z�N�^�Œ� */
		fdc_indexcnt = (BYTE)((fdc_indexcnt + 1) & 0x0f);
		if (fdc_track[fdc_drvreg] >= 40) {
			return -1;
		}
		return fdc_indexcnt;
	}

	/* D77 */
	ASSERT(fdc_ready[fdc_drvreg] == FDC_TYPE_D77);
	secs = fdc_buffer[0x0005];
	secs *= 256;
	secs |= fdc_buffer[0x0004];
	if (secs == 0) {
		/* �A���t�H�[�}�b�g */
		fdc_indexcnt = (BYTE)((fdc_indexcnt + 1) & 0x0f);
		return -1;
	}
	else {
		fdc_indexcnt++;
		if (fdc_indexcnt >= secs) {
			fdc_indexcnt = 0;
		}
		return fdc_indexcnt;
	}
}

/*
 *	ID�}�[�N��T��
 */
static BOOL FASTCALL fdc_idmark(WORD *p)
{
	WORD offset;
	BYTE dat;

	/* A1 A1 A1 FE��������F5 F5 F5 FE */
	offset = *p;

	while (offset < fdc_totalcnt) {
		dat = fdc_buffer[offset++];
		if ((dat != 0xa1) && (dat != 0xf5)) {
			continue;
		}
		dat = fdc_buffer[offset++];
		if ((dat != 0xa1) && (dat != 0xf5)) {
			continue;
		}
		dat = fdc_buffer[offset++];
		if ((dat != 0xa1) && (dat != 0xf5)) {
			continue;
		}
		dat = fdc_buffer[offset++];
		if (dat == 0xfe) {
			*p = offset;
			return TRUE;
		}
	}

	*p = offset;
	return FALSE;
}

/*
 *	�f�[�^�}�[�N��T��
 */
static BOOL FASTCALL fdc_datamark(WORD *p)
{
	WORD offset;
	BYTE dat;

	/* A1 A1 A1 FB��������F5 F5 F5 FB */
	/* deleted data mark�ɂ͖��Ή� */
	offset = *p;

	while (offset < fdc_totalcnt) {
		dat = fdc_buffer[offset++];
		if ((dat != 0xa1) && (dat != 0xf5)) {
			continue;
		}
		dat = fdc_buffer[offset++];
		if ((dat != 0xa1) && (dat != 0xf5)) {
			continue;
		}
		dat = fdc_buffer[offset++];
		if ((dat != 0xa1) && (dat != 0xf5)) {
			continue;
		}
		dat = fdc_buffer[offset++];
		if (dat == 0xfb) {
			*p = offset;
			return TRUE;
		}
	}

	*p = offset;
	return FALSE;
}

/*
 *	�g���b�N�������ݏI��
 */
static BOOL FASTCALL fdc_writetrk(void)
{
	int total;
	int sectors;
	WORD offset;
	WORD seclen;
	WORD writep;
	int i;
	int handle;

	/* ������ */
	total = 0;
	sectors = 0;

	/* �Z�N�^���ƁA�f�[�^���̃g�[�^���T�C�Y�𐔂��� */
	offset = 0;
	while (offset < fdc_totalcnt) {
		/* ID�}�[�N��T�� */
		if (!fdc_idmark(&offset)) {
			break;
		}
		/* C,H,R,N�̎���$F7�� */
		offset += (WORD)4;
		if (offset >= fdc_totalcnt) {
			return FALSE;
		}
		if (fdc_buffer[offset] != 0xf7) {
			return FALSE;
		}
		offset++;
		/* �f�[�^�}�[�N��T�� */
		if (!fdc_datamark(&offset)) {
			return FALSE;
		}
		/* �����O�X���v�Z���A$F7��T�� */
		seclen = 0;
		while(offset < fdc_totalcnt) {
			if (fdc_buffer[offset] == 0xf7) {
				break;
			}
			offset++;
			seclen++;
		}
		if (offset >= fdc_totalcnt) {
			return FALSE;
		}
		/* �Z�N�^ok */
		total += seclen;
		sectors++;
		offset++;
	}

	/* 2D���f�B�A�̏ꍇ�Atotal=0x1000, sectors=0x10���K�{ */
	if (fdc_ready[fdc_drvreg] == FDC_TYPE_2D) {
		/* total, sectors�̌��� */
		if (total != 0x1000) {
			return FALSE;
		}
		if (sectors != 16) {
			return FALSE;
		}

		/* �{����C,H,N�Ȃǃ`�F�b�N���ׂ� */
		return TRUE;
	}

	/* �Z�N�^���Ƃ�0x10�Ԃ�A�]�v�ɂ����� */
	if ((total + (sectors * 0x10)) > fdc_trklen[fdc_drvreg]) {
		return FALSE;
	}

	/* ���܂邱�Ƃ��킩�����̂ŁA�f�[�^���쐬���� */
	writep = 0;
	offset = 0;
	for (i=0; i<sectors; i++) {
		/* ID�}�[�N������ */
		fdc_idmark(&offset);

		/* 0x10�w�b�_�̍쐬 */
		fdc_buffer[writep++] = fdc_buffer[offset++];
		fdc_buffer[writep++] = fdc_buffer[offset++];
		fdc_buffer[writep++] = fdc_buffer[offset++];
		fdc_buffer[writep++] = fdc_buffer[offset++];
		fdc_buffer[writep++] = (BYTE)(sectors & 0xff);
		fdc_buffer[writep++] = (BYTE)(sectors >> 8);
		memset(&fdc_buffer[writep], 0, 8);
		writep += (WORD)8;
		offset++;

		/* �Z�N�^�����O�X�͌�ŏ������� */
		writep += (WORD)2;

		/* �f�[�^�}�[�N������ */
		fdc_datamark(&offset);

		/* �����O�X�𐔂��R�s�[ */
		seclen = 0;
		while (fdc_buffer[offset] != 0xf7) {
			fdc_buffer[writep++] = fdc_buffer[offset++];
			seclen++;
		}
		offset++;

		/* �Z�N�^�����O�X��ݒ� */
		fdc_buffer[writep - seclen - 2] = (BYTE)(seclen & 0xff);
		fdc_buffer[writep - seclen - 1] = (BYTE)(seclen >> 8);
	}

	/* �t�@�C���Ƀf�[�^���������� */
	handle = file_open(fdc_fname[fdc_drvreg], OPEN_RW);
	if (handle == -1) {
		return FALSE;
	}
	if (!file_seek(handle, fdc_seekofs[fdc_drvreg])) {
		file_close(handle);
		return FALSE;
	}
	if (!file_write(handle, fdc_buffer, (sectors * 0x10) + total)) {
		file_close(handle);
		return FALSE;
	}
	file_close(handle);

	/* ����I�� */
	return TRUE;
}

/*
 *	�Z�N�^�������ݏI��
 */
static BOOL FASTCALL fdc_writesec(void)
{
	DWORD offset;
	int handle;

	/* assert */
	ASSERT(fdc_drvreg < FDC_DRIVES);
	ASSERT(fdc_ready[fdc_drvreg] != FDC_TYPE_NOTREADY);
	ASSERT(fdc_dataptr);
	ASSERT(fdc_totalcnt > 0);

	/* �I�t�Z�b�g�Z�o */
	offset = fdc_seekofs[fdc_drvreg];
	offset += fdc_secofs[fdc_drvreg];

	/* �������� */
	handle = file_open(fdc_fname[fdc_drvreg], OPEN_RW);
	if (handle == -1) {
		return FALSE;
	}
	if (!file_seek(handle, offset)) {
		file_close(handle);
		return FALSE;
	}
	if (!file_write(handle, fdc_dataptr, fdc_totalcnt)) {
		file_close(handle);
		return FALSE;
	}
	file_close(handle);

	return TRUE;
}

/*
 *	�g���b�N�A�Z�N�^�A�T�C�h�ƈ�v����Z�N�^������
 *	�J�E���^�A�f�[�^�|�C���^��ݒ�
 */
static BYTE FASTCALL fdc_readsec(BYTE track, BYTE sector, BYTE side)
{
	int secs;
	int i;
	WORD offset;
	WORD size;
	BYTE stat;

	/* assert */
	ASSERT(fdc_drvreg < FDC_DRIVES);
	ASSERT(fdc_ready[fdc_drvreg] != FDC_TYPE_NOTREADY);

	/* 2D�t�@�C���̏ꍇ */
	if (fdc_ready[fdc_drvreg] == FDC_TYPE_2D) {
		if (track != fdc_track[fdc_drvreg]) {
			return FDC_ST_RECNFND;
		}
		if (side != fdc_sidereg) {
			return FDC_ST_RECNFND;
		}
		if ((sector < 1) || (sector > 16)) {
			return FDC_ST_RECNFND;
		}

		/* �f�[�^�|�C���^�ݒ� */
		fdc_dataptr = &fdc_buffer[(sector - 1) * 0x0100];
		fdc_secofs[fdc_drvreg] = (sector - 1) * 0x0100;

		/* �J�E���^�ݒ� */
		fdc_totalcnt = 0x0100;
		fdc_nowcnt = 0;

		return 0;
	}

	secs = fdc_buffer[0x0005];
	secs *= (WORD)256;
	secs |= fdc_buffer[0x0004];
	if (secs == 0) {
		return FDC_ST_RECNFND;
	}

	offset = 0;
	/* �Z�N�^���[�v */
	for (i=0; i<secs; i++) {
		/* ���̃Z�N�^�̃T�C�Y���Ɏ擾 */
		size = fdc_buffer[offset + 0x000f];
		size *= (WORD)256;
		size |= fdc_buffer[offset + 0x000e];

		/* C,H,R����v����Z�N�^�����邩 */
		if (fdc_buffer[offset + 0] != track) {
			offset += size;
			offset += (WORD)0x10;
			continue;
		}
		if (fdc_buffer[offset + 1] != side) {
			offset += size;
			offset += (WORD)0x10;
			continue;
		}
		if (fdc_buffer[offset + 2] != sector) {
			offset += size;
			offset += (WORD)0x10;
			continue;
		}

		/* �P���`�F�b�N */
		if (fdc_buffer[offset + 0x0006] != 0) {
			continue;
		}

		/* �f�[�^�|�C���^�A�J�E���^�ݒ� */
		fdc_dataptr = &fdc_buffer[offset + 0x0010];
		fdc_secofs[fdc_drvreg] = offset + 0x0010;
		fdc_totalcnt = size;
		fdc_nowcnt = 0;

		/* �X�e�[�^�X�ݒ� */
		stat = 0;
		if (fdc_buffer[offset + 0x0007] != 0) {
			stat |= FDC_ST_RECTYPE;
		}
		if (fdc_buffer[offset + 0x0008] == 0xb0) {
			stat = FDC_ST_CRCERR;
		}

		return stat;
	}

	/* �S�Z�N�^�������������A������Ȃ����� */
	return FDC_ST_RECNFND;
}

/*
 *	�g���b�N�ǂݍ���
 */
static void FASTCALL fdc_readbuf(int drive)
{
	DWORD offset;
	DWORD len;
	int trkside;
	int handle;

	/* �h���C�u�`�F�b�N */
	if ((drive < 0) || (drive >= FDC_DRIVES)) {
		return;
	}

	/* ���f�B�`�F�b�N */
	if (fdc_ready[drive] == FDC_TYPE_NOTREADY) {
		return;
	}

	/* �C���f�b�N�X�z�[���J�E���^���N���A */
	fdc_indexcnt = 0;

	/* �g���b�N�~�Q�{�T�C�h */
	trkside = fdc_track[drive] * 2 + fdc_sidereg;

	/*
	 * 2D�t�@�C��
	 */
	if (fdc_ready[drive] == FDC_TYPE_2D) {
		/* �͈̓`�F�b�N */
		if (trkside >= 80) {
			memset(fdc_buffer, 0, 0x1000);
			return;
		}

		/* �I�t�Z�b�g�Z�o */
		offset = (DWORD)trkside;
		offset *= 0x1000;
		fdc_seekofs[drive] = offset;
		fdc_trklen[drive] = 0x1000;

		/* �ǂݍ��� */
		memset(fdc_buffer, 0, 0x1000);
		handle = file_open(fdc_fname[drive], OPEN_R);
		if (handle == -1) {
			return;
		}
		if (!file_seek(handle, offset)) {
			file_close(handle);
			return;
		}
		file_read(handle, fdc_buffer, 0x1000);
		file_close(handle);
		return;
	}

	/*
	 * D77�t�@�C��
	 */
	if (trkside >= 84) {
		/* �g���b�N�I�[�o�[�A�Z�N�^��0 */
		fdc_buffer[4] = 0;
		fdc_buffer[5] = 0;
		return;
	}

	/* �w�b�_�t�@�C���ɏ]���A�I�t�Z�b�g�E�����O�X���Z�o */
	offset = *(DWORD *)(&fdc_header[drive][0x0020 + trkside * 4]);
	if (offset == 0) {
		/* ���݂��Ȃ��g���b�N */
		fdc_buffer[4] = 0;
		fdc_buffer[5] = 0;
		return;
	}

	len = *(DWORD *)(&fdc_header[drive][0x0020 + (trkside + 1) * 4]);
	if (len == 0) {
		/* �ŏI�g���b�N */
		len = *(DWORD *)(&fdc_header[drive][0x0014]);
	}
	len -= offset;
	if (len > 0x2000) {
		len = 0x2000;
	}
	fdc_seekofs[drive] = offset;
	fdc_trklen[drive] = (WORD)len;

	/* �V�[�N�A�ǂݍ��� */
	memset(fdc_buffer, 0, 0x2000);
	handle = file_open(fdc_fname[drive], OPEN_R);
	if (handle == -1) {
		return;
	}
	if (!file_seek(handle, offset)) {
		file_close(handle);
		return;
	}
	file_read(handle, fdc_buffer, len);
	file_close(handle);
}

/*
 *	D77�t�@�C�� �w�b�_�ǂݍ���
 */
static BOOL FASTCALL fdc_readhead(int drive, int index)
{
	int i;
	DWORD offset;
	DWORD temp;
	int handle;

	/* assert */
	ASSERT((drive >= 0) && (drive < FDC_DRIVES));
	ASSERT((index >= 0) && (index < FDC_MEDIAS));
	ASSERT(fdc_ready[drive] == FDC_TYPE_D77);

	/* �I�t�Z�b�g���� */
	offset = fdc_foffset[drive][index];

	/* �V�[�N�A�ǂݍ��� */
	handle = file_open(fdc_fname[drive], OPEN_R);
	if (handle == -1) {
		return FALSE;
	}
	if (!file_seek(handle, offset)) {
		file_close(handle);
		return FALSE;
	}
	if (!file_read(handle, fdc_header[drive], 0x2b0)) {
		file_close(handle);
		return FALSE;
	}
	file_close(handle);

	/* �^�C�v�`�F�b�N�A���C�g�v���e�N�g�ݒ� */
	if (fdc_header[drive][0x001b] != 0) {
		/* 2D�łȂ� */
		return FALSE;
	}
	if (fdc_fwritep[drive]) {
		fdc_writep[drive] = TRUE;
	}
	else {
		if (fdc_header[drive][0x001a] & 0x10) {
			fdc_writep[drive] = TRUE;
		}
		else {
			fdc_writep[drive] = FALSE;
		}
	}

	/* �g���b�N�I�t�Z�b�g��ݒ� */
	for (i=0; i<164; i++) {
		temp = 0;
		temp |= fdc_header[drive][0x0020 + i * 4 + 3];
		temp *= 256;
		temp |= fdc_header[drive][0x0020 + i * 4 + 2];
		temp *= 256;
		temp |= fdc_header[drive][0x0020 + i * 4 + 1];
		temp *= 256;
		temp |= fdc_header[drive][0x0020 + i * 4 + 0];

		if (temp != 0) {
			/* �f�[�^���� */
			temp += offset;
			*(DWORD *)(&fdc_header[drive][0x0020 + i * 4]) = temp;
		}
	}

	/* �f�B�X�N�T�C�Y�{�I�t�Z�b�g */
	temp = 0;
	temp |= fdc_header[drive][0x001c + 3];
	temp *= 256;
	temp |= fdc_header[drive][0x001c + 2];
	temp *= 256;
	temp |= fdc_header[drive][0x001c + 1];
	temp *= 256;
	temp |= fdc_header[drive][0x001c + 0];
	temp += offset;
	*(DWORD *)(&fdc_header[drive][0x0014]) = temp;

	return TRUE;
}

/*
 *	���݂̃��f�B�A�̃��C�g�v���e�N�g��؂�ւ���
 */
BOOL FASTCALL fdc_setwritep(int drive, BOOL writep)
{
	BYTE header[0x2b0];
	DWORD offset;
	int handle;

	/* assert */
	ASSERT((drive >= 0) && (drive < FDC_DRIVES));
	ASSERT((writep == TRUE) || (writep == FALSE));

	/* ���f�B�łȂ���΂Ȃ�Ȃ� */
	if (fdc_ready[drive] == FDC_TYPE_NOTREADY) {
		return FALSE;
	}

	/* �t�@�C�����������ݕs�Ȃ�_�� */
	if (fdc_fwritep[drive]) {
		return FALSE;
	}

	/* �ǂݍ��݁A�ݒ�A�������� */
	offset = fdc_foffset[drive][fdc_media[drive]];
	handle = file_open(fdc_fname[drive], OPEN_RW);
	if (handle == -1) {
		return FALSE;
	}
	if (!file_seek(handle, offset)) {
		file_close(handle);
		return FALSE;
	}
	if (!file_read(handle, header, 0x2b0)) {
		file_close(handle);
		return FALSE;
	}
	if (writep) {
		header[0x1a] |= 0x10;
	}
	else {
		header[0x1a] &= ~0x10;
	}
	if (!file_seek(handle, offset)) {
		file_close(handle);
		return FALSE;
	}
	if (!file_write(handle, header, 0x2b0)) {
		file_close(handle);
		return FALSE;
	}

	/* ���� */
	file_close(handle);
	fdc_writep[drive] = writep;
	return TRUE;
}

/*
 *	���f�B�A�ԍ���ݒ�
 */
BOOL FASTCALL fdc_setmedia(int drive, int index)
{
	/* assert */
	ASSERT((drive >= 0) && (drive < FDC_DRIVES));
	ASSERT((index >= 0) && (index < FDC_MEDIAS));

	/* ���f�B��Ԃ� */
	if (fdc_ready[drive] == 0) {
		return FALSE;
	}

	/* 2D�t�@�C���̏ꍇ�Aindex = 0�� */
	if ((fdc_ready[drive] == FDC_TYPE_2D) && (index != 0)) {
		return FALSE;
	}

	/* index > 0 �Ȃ�Afdc_foffset�𒲂ׂ�>0���K�v */
	if (index > 0) {
		if (fdc_foffset[drive][index] == 0) {
			return FALSE;
		}
	}

	/* D77�t�@�C���̏ꍇ�A�w�b�_�ǂݍ��� */
	if (fdc_ready[drive] == FDC_TYPE_D77) {
		/* ���C�g�v���e�N�g�͓����Őݒ� */
		if (!fdc_readhead(drive, index)) {
			return FALSE;
		}
	}
	else {
		/* 2D�t�@�C���Ȃ�A�t�@�C�������ɏ]�� */
		fdc_writep[drive] = fdc_fwritep[drive];
	}

	/* �ꎞ�C�W�F�N�g���������� */
	fdc_teject[fdc_drvreg] = FALSE;

	/* �f�[�^�o�b�t�@�ǂݍ��݁A���[�N�Z�[�u */
	fdc_readbuf(fdc_drvreg);
	fdc_media[drive] = (BYTE)index;

	return TRUE;
}

/*
 *	D77�t�@�C����́A���f�B�A������і��̎擾
 */
static int FASTCALL fdc_chkd77(int drive)
{
	int i;
	int handle;
	int count;
	DWORD offset;
	DWORD len;
	BYTE buf[0x20];

	/* ������ */
	for (i=0; i<FDC_MEDIAS; i++) {
		fdc_foffset[drive][i] = 0;
		fdc_name[drive][i][0] = '\0';
	}
	count = 0;
	offset = 0;

	/* �t�@�C���I�[�v�� */
	handle = file_open(fdc_fname[drive], OPEN_R);
	if (handle == -1) {
		return count;
	}

	/* ���f�B�A���[�v */
	while (count < FDC_MEDIAS) {
		/* �V�[�N */
		if (!file_seek(handle, offset)) {
			file_close(handle);
			return count;
		}

		/* �ǂݍ��� */
		if (!file_read(handle, buf, 0x0020)) {
			file_close(handle);
			return count;
		}

		/* �`�F�b�N */
		if (buf[0x1b] != 0) {
			file_close(handle);
			return count;
		}

		/* ok,�t�@�C�����A�I�t�Z�b�g�i�[ */
		buf[17] = '\0';
		memcpy(fdc_name[drive][count], buf, 17);
		fdc_foffset[drive][count] = offset;

		/* next���� */
		len = 0;
		len |= buf[0x1f];
		len *= 256;
		len |= buf[0x1e];
		len *= 256;
		len |= buf[0x1d];
		len *= 256;
		len |= buf[0x1c];
		offset += len;
		count++;
	}

	/* �ő僁�f�B�A�����ɒB���� */
	file_close(handle);
	return count;
}

/*
 *	�f�B�X�N�t�@�C����ݒ�
 */
int FASTCALL fdc_setdisk(int drive, char *fname)
{
	BOOL writep;
	int handle;
	DWORD fsize;
	int count;

	ASSERT((drive >= 0) && (drive < FDC_DRIVES));

	/* �m�b�g���f�B�ɂ���ꍇ */
	if (fname == NULL) {
		fdc_ready[drive] = FDC_TYPE_NOTREADY;
		fdc_fname[drive][0] = '\0';
		return 1;
	}

	/* �t�@�C�����I�[�v�����A�t�@�C���T�C�Y�𒲂ׂ� */
	strcpy(fdc_fname[drive], fname);
	writep = FALSE;
	handle = file_open(fname, OPEN_RW);
	if (handle == -1) {
		handle = file_open(fname, OPEN_R);
		if (handle == -1) {
			return 0;
		}
		writep = TRUE;
	}
	fsize = file_getsize(handle);
	file_close(handle);

	/*
	 * 2D�t�@�C��
	 */
	if (fsize == 327680) {
		/* �^�C�v�A�������ݑ����ݒ� */
		fdc_ready[drive] = FDC_TYPE_2D;
		fdc_fwritep[drive] = writep;

		/* ���f�B�A�ݒ� */
		if (!fdc_setmedia(drive, 0)) {
			fdc_ready[drive] = FDC_TYPE_NOTREADY;
			return 0;
		}

		/* �����B�ꎞ�C�W�F�N�g���� */
		fdc_teject[drive] = FALSE;
		fdc_medias[drive] = 1;
		return 1;
	}

	/*
	 * D77�t�@�C��
	 */
	fdc_ready[drive] = FDC_TYPE_D77;
	fdc_fwritep[drive] = writep;

	/* �t�@�C������ */
	count = fdc_chkd77(drive);
	if (count == 0){
		fdc_ready[drive] = FDC_TYPE_NOTREADY;
		return 0;
	}

	/* ���f�B�A�ݒ� */
	if (!fdc_setmedia(drive, 0)) {
		fdc_ready[drive] = FDC_TYPE_NOTREADY;
		return 0;
	}

	/* �����B�ꎞ�C�W�F�N�g���� */
	fdc_teject[drive] = FALSE;
	fdc_medias[drive] = (BYTE)count;
	return count;
}

/*-[ FDC�R�}���h ]----------------------------------------------------------*/

/*
 *	FDC �X�e�[�^�X�쐬
 */
static void FASTCALL fdc_make_stat(void)
{
	/* �h���C�u�`�F�b�N */
	if (fdc_drvreg >= FDC_DRIVES) {
		fdc_status |= FDC_ST_NOTREADY;
		return;
	}

	/* �L���ȃh���C�u */
	if (fdc_ready[fdc_drvreg] == FDC_TYPE_NOTREADY) {
		fdc_status |= FDC_ST_NOTREADY;
	}
	else {
		fdc_status &= (BYTE)(~FDC_ST_NOTREADY);
	}

	/* �ꎞ�C�W�F�N�g */
	if (fdc_teject[fdc_drvreg]) {
		fdc_status |= FDC_ST_NOTREADY;
	}

	/* ���C�g�v���e�N�g(Read�n�R�}���h��0) */
	if ((fdc_cmdtype == 2) || (fdc_cmdtype == 4) || (fdc_cmdtype == 6)) {
		fdc_status &= ~FDC_ST_WRITEP;
	}
	else {
		if (fdc_writep[fdc_drvreg] && !(fdc_status & FDC_ST_NOTREADY)) {
			fdc_status |= FDC_ST_WRITEP;
		}
		else {
			fdc_status &= ~FDC_ST_WRITEP;
		}
	}

	/* TYPE I �̂� */
	if (fdc_cmdtype != 1) {
		return;
	}

	/* TRACK00 */
	if (fdc_track[fdc_drvreg] == 0) {
		fdc_status |= FDC_ST_TRACK00;
	}
	else {
		fdc_status &= ~FDC_ST_TRACK00;
	}

	/* index */
	if (!(fdc_status & FDC_ST_NOTREADY)) {
		if (fdc_indexcnt == 0) {
			fdc_status |= FDC_ST_INDEX;
		}
		else {
			fdc_status &= ~FDC_ST_INDEX;
		}
		fdc_next_index();
	}
}

/*
 *	TYPE I
 *	RESTORE
 */
static void FASTCALL fdc_restore(void)
{
	/* TYPE I */
	fdc_cmdtype = 1;
	fdc_status = 0;

	/* �h���C�u�`�F�b�N */
	if (fdc_drvreg >= FDC_DRIVES) {
		mainetc_fdc();
		fdc_drqirq = 0x40;
		fdc_make_stat();
		return;
	}

	/* �g���b�N��r�A���X�g�A�A�ǂݍ��� */
	if (fdc_track[fdc_drvreg] != 0) {
		fdc_track[fdc_drvreg] = 0;
		fdc_readbuf(fdc_drvreg);
	}
	fdc_seekvct = TRUE;

	/* �����A�b�v�f�[�g */
	fdc_trkreg = 0;

	/* FDC���荞�� */
	mainetc_fdc();
	fdc_drqirq = 0x40;

	/* �X�e�[�^�X���� */
	fdc_status = FDC_ST_BUSY;
	if (fdc_command & 0x08) {
		fdc_status |= FDC_ST_HEADENG;
	}
	fdc_make_stat();

	/* �A�N�Z�X(SEEK) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_SEEK;
}

/*
 *	TYPE I
 *	SEEK
 */
static void FASTCALL fdc_seek(void)
{
	BYTE target;

	/* TYPE I */
	fdc_cmdtype = 1;
	fdc_status = 0;

	/* �h���C�u�`�F�b�N */
	if (fdc_drvreg >= FDC_DRIVES) {
		mainetc_fdc();
		fdc_drqirq = 0x40;
		fdc_make_stat();
		return;
	}

	/* ��Ƀx���t�@�C */
	if (fdc_command & 0x04) {
		if (fdc_trkreg != fdc_track[fdc_drvreg]) {
			fdc_status |= FDC_ST_SEEKERR;
			/* �����C��(��ՂĂ����΍�) */
			fdc_trkreg = fdc_track[fdc_drvreg];
		}
	}

	/* ���΃V�[�N */
	if (fdc_datareg > fdc_trkreg) {
		fdc_seekvct = FALSE;
		target = (BYTE)(fdc_track[fdc_drvreg] + fdc_datareg -  fdc_trkreg);
		if (target > 41) {
			target = 41;
		}
	}
	else {
		fdc_seekvct = TRUE;
		target = (BYTE)(fdc_track[fdc_drvreg] + fdc_datareg -  fdc_trkreg);
		if (target > 41) {
			target = 0;
		}
	}

	/* �g���b�N��r�A�V�[�N�A�ǂݍ��� */
	if (fdc_track[fdc_drvreg] != target) {
		fdc_track[fdc_drvreg] = target;
		fdc_readbuf(fdc_drvreg);
	}

	/* �A�b�v�f�[�g */
	if (fdc_command & 0x10) {
		fdc_trkreg = fdc_track[fdc_drvreg];
	}

	/* FDC���荞�� */
	mainetc_fdc();
	fdc_drqirq = 0x40;

	/* �X�e�[�^�X���� */
	fdc_status |= FDC_ST_BUSY;
	if (fdc_command & 0x08) {
		fdc_status |= FDC_ST_HEADENG;
	}
	fdc_make_stat();

	/* �A�N�Z�X(SEEK) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_SEEK;
}

/*
 *	TYPE I
 *	STEP IN
 */
static void FASTCALL fdc_step_in(void)
{
	/* TYPE I */
	fdc_cmdtype = 1;
	fdc_status = 0;

	/* �h���C�u�`�F�b�N */
	if (fdc_drvreg >= FDC_DRIVES) {
		mainetc_fdc();
		fdc_drqirq = 0x40;
		fdc_make_stat();
		return;
	}

	/* ��Ƀx���t�@�C */
	if (fdc_command & 0x04) {
		if (fdc_trkreg != fdc_track[fdc_drvreg]) {
			fdc_status |= FDC_ST_SEEKERR;
		}
	}

	/* �X�e�b�v�A�ǂݍ��� */
	if (fdc_track[fdc_drvreg] < 41) {
		fdc_track[fdc_drvreg]++;
		fdc_readbuf(fdc_drvreg);
	}
	fdc_seekvct = FALSE;

	/* �A�b�v�f�[�g */
	if (fdc_command & 0x10) {
		fdc_trkreg = fdc_track[fdc_drvreg];
	}

	/* FDC���荞�� */
	mainetc_fdc();
	fdc_drqirq = 0x40;

	/* �X�e�[�^�X���� */
	fdc_status |= FDC_ST_BUSY;
	if (fdc_command & 0x08) {
		fdc_status |= FDC_ST_HEADENG;
	}
	fdc_make_stat();

	/* �A�N�Z�X(SEEK) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_SEEK;
}

/*
 *	TYPE I
 *	STEP OUT
 */
static void FASTCALL fdc_step_out(void)
{
	/* TYPE I */
	fdc_cmdtype = 1;
	fdc_status = 0;

	/* �h���C�u�`�F�b�N */
	if (fdc_drvreg >= FDC_DRIVES) {
		mainetc_fdc();
		fdc_drqirq = 0x40;
		fdc_make_stat();
		return;
	}

	/* ��Ƀx���t�@�C */
	if (fdc_command & 0x04) {
		if (fdc_trkreg != fdc_track[fdc_drvreg]) {
			fdc_status |= FDC_ST_SEEKERR;
		}
	}

	/* �X�e�b�v�A�ǂݍ��� */
	if (fdc_track[fdc_drvreg] != 0) {
		fdc_track[fdc_drvreg]--;
		fdc_readbuf(fdc_drvreg);
	}
	fdc_seekvct = TRUE;

	/* �A�b�v�f�[�g */
	if (fdc_command & 0x10) {
		fdc_trkreg = fdc_track[fdc_drvreg];
	}

	/* FDC���荞�� */
	mainetc_fdc();
	fdc_drqirq = 0x40;

	/* �X�e�[�^�X���� */
	fdc_status |= FDC_ST_BUSY;
	if (fdc_command & 0x08) {
		fdc_status |= FDC_ST_HEADENG;
	}
	fdc_make_stat();

	/* �A�N�Z�X(SEEK) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_SEEK;
}

/*
 *	TYPE I
 *	STEP
 */
static void FASTCALL fdc_step(void)
{
	if (fdc_seekvct) {
		fdc_step_out();
	}
	else {
		fdc_step_in();
	}
}

/*
 *	TYPE II, III
 *	READ/WRITE �T�u
 */
static BOOL FASTCALL fdc_rw_sub(void)
{
	fdc_status = 0;

	/* �h���C�u�`�F�b�N */
	if (fdc_drvreg >= FDC_DRIVES) {
		fdc_make_stat();
		/* FDC���荞�� */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return FALSE;
	}

	/* NOT READY�`�F�b�N */
	if ((fdc_ready[fdc_drvreg] == FDC_TYPE_NOTREADY) || fdc_teject[fdc_drvreg]) {
		fdc_make_stat();
		/* FDC���荞�� */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return FALSE;
	}

	return TRUE;
}

/*
 *	TYPE II
 *	READ DATA
 */
static void FASTCALL fdc_read_data(void)
{
	BYTE stat;

	/* TYPE II, Read */
	fdc_cmdtype = 2;

	/* ��{�`�F�b�N */
	if (!fdc_rw_sub()) {
		return;
	}

	/* �u�[�g�t���O */
	if (!fdc_boot && (fdc_drvreg > 0)) {
		/* �[���I�ɁANOT READY����� */
		fdc_status |= 0x80;

		/* FDC���荞�� */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return;
	}

	/* �Z�N�^���� */
	if (fdc_command & 0x02) {
		stat = fdc_readsec(fdc_trkreg, fdc_secreg, (BYTE)((fdc_command & 0x08) >> 3));
	}
	else {
		stat = fdc_readsec(fdc_trkreg, fdc_secreg, fdc_sidereg);
	}

	/* ��ɃX�e�[�^�X��ݒ肷�� */
	fdc_status = stat;

	/* RECORD NOT FOUND ? */
	if (fdc_status & FDC_ST_RECNFND) {
		/* FDC���荞�� */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return;
	}

	/* �ŏ��̃f�[�^��ݒ� */
	fdc_status |= FDC_ST_BUSY;
	fdc_status |= FDC_ST_DRQ;
	fdc_datareg = fdc_dataptr[0];
	mainetc_fdc();
	fdc_drqirq = 0x80;

	/* �u�[�g�t���O�ݒ� */
	if ((fdc_trkreg == 0) && (fdc_secreg == 1) && (fdc_sidereg == 0)) {
		fdc_boot = TRUE;
	}

	/* �A�N�Z�X(READ) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_READ;
}

/*
 *	TYPE II
 *	WRITE DATA
 */
static void FASTCALL fdc_write_data(void)
{
	BYTE stat;

	/* TYPE II, Write */
	fdc_cmdtype = 3;

	/* ��{�`�F�b�N */
	if (!fdc_rw_sub()) {
		return;
	}

	/* WRITE PROTECT�`�F�b�N */
	if (fdc_writep[fdc_drvreg] != 0) {
		fdc_make_stat();
		/* FDC���荞�� */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return;
	}

	/* �Z�N�^���� */
	if (fdc_command & 0x02) {
		stat = fdc_readsec(fdc_trkreg, fdc_secreg, (BYTE)((fdc_command & 0x08) >> 3));
	}
	else {
		stat = fdc_readsec(fdc_trkreg, fdc_secreg, fdc_sidereg);
	}

	/* ��ɃX�e�[�^�X��ݒ肷�� */
	fdc_status = stat;

	/* RECORD NOT FOUND ? */
	if (fdc_status & FDC_ST_RECNFND) {
		fdc_status = FDC_ST_RECNFND;
		/* FDC���荞�� */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return;
	}

	/* DRQ */
	fdc_status = FDC_ST_BUSY | FDC_ST_DRQ;
	mainetc_fdc();
	fdc_drqirq = 0x80;

	/* �A�N�Z�X(WRITE) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_WRITE;
}

/*
 *	TYPE III
 *	READ ADDRESS
 */
static void FASTCALL fdc_read_addr(void)
{
	int idx;

	/* TYPE III, Read Address */
	fdc_cmdtype = 4;

	/* ��{�`�F�b�N */
	if (!fdc_rw_sub()) {
		return;
	}

	/* �g���b�N�擪����̑��΃Z�N�^�ԍ����擾 */
	idx = fdc_next_index();

	/* �A���t�H�[�}�b�g�`�F�b�N */
	if (idx == -1) {
		fdc_status = FDC_ST_RECNFND;
		/* FDC���荞�� */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return;
	}

	/* �f�[�^�쐬 */
	fdc_makeaddr(idx);

	/* �ŏ��̃f�[�^��ݒ� */
	fdc_status |= FDC_ST_BUSY;
	fdc_status |= FDC_ST_DRQ;
	fdc_datareg = fdc_dataptr[0];
	mainetc_fdc();
	fdc_drqirq = 0x80;

	/* �����ŁA�Z�N�^���W�X�^�Ƀg���b�N��ݒ� */
	fdc_secreg = fdc_track[fdc_drvreg];

	/* �A�N�Z�X(READ) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_READ;
}

/*
 *	TYPE III
 *	READ TRACK
 */
static void FASTCALL fdc_read_track(void)
{
	/* TYPE III, Read Track */
	fdc_cmdtype = 6;

	/* ��{�`�F�b�N */
	if (!fdc_rw_sub()) {
		return;
	}

	/* �f�[�^�쐬 */
	fdc_make_track();

	/* �ŏ��̃f�[�^��ݒ� */
	fdc_status |= FDC_ST_BUSY;
	fdc_status |= FDC_ST_DRQ;
	fdc_datareg = fdc_dataptr[0];
	mainetc_fdc();
	fdc_drqirq = 0x80;
}

/*
 *	TYPE III
 *	WRITE TRACK
 */
static void FASTCALL fdc_write_track(void)
{
	fdc_status = 0;

	/* TYPE III, Write */
	fdc_cmdtype = 5;

	/* ��{�`�F�b�N */
	if (!fdc_rw_sub()) {
		return;
	}

	/* WRITE PROTECT�`�F�b�N */
	if (fdc_writep[fdc_drvreg] != 0) {
		fdc_make_stat();
		/* FDC���荞�� */
		mainetc_fdc();
		fdc_drqirq = 0x40;
		return;
	}

	/* DRQ */
	fdc_status = FDC_ST_BUSY | FDC_ST_DRQ;
	mainetc_fdc();
	fdc_drqirq = 0x80;

	fdc_dataptr = fdc_buffer;
	fdc_totalcnt = 0x1800;
	fdc_nowcnt = 0;

	/* �A�N�Z�X(WRITE) */
	fdc_access[fdc_drvreg] = FDC_ACCESS_WRITE;
}

/*
 *	TYPE IV
 *	FORCE INTERRUPT
 */
static void FASTCALL fdc_force_intr(void)
{
	/* �X�e�[�^�X���� */
	if (fdc_status & 0x01) {
		fdc_status &= ~FDC_ST_BUSY;
		/* ���s���̃R�}���h�Ɠ����X�e�[�^�X������ */
	}
	else {
		/* INDEX, TRACK00, HEAD ENGAGED, WRTE PROTECT, NOT READY */
		fdc_cmdtype = 1;
		fdc_status = 0;
		fdc_indexcnt = 0;
		fdc_make_stat();
	}

	/* �����ꂩ�������Ă���΁AIRQ����(�����Ƃ��Ă͕s�\��) */
	if (fdc_command & 0x0f) {
		fdc_drqirq = 0x40;
		mainetc_fdc();
	}

	/* �A�N�Z�X��~ */
	fdc_readbuf(fdc_drvreg);
	fdc_dataptr = NULL;
	fdc_access[fdc_drvreg] = FDC_ACCESS_READY;
}

/*
 *	�R�}���h����
 */
static void FASTCALL fdc_process_cmd(void)
{
	BYTE high;

	high = (BYTE)(fdc_command >> 4);

	/* �f�[�^�]�������s���Ă���΁A�����~�߂� */
	fdc_dataptr = NULL;

	/* ���� */
	switch (high) {
		/* restore */
		case 0x00:
			fdc_restore();
			break;
		/* seek */
		case 0x01:
			fdc_seek();
			break;
		/* step */
		case 0x02:
		case 0x03:
			fdc_step();
			break;
		/* step in */
		case 0x04:
		case 0x05:
			fdc_step_in();
			break;
		/* step out */
		case 0x06:
		case 0x07:
			fdc_step_out();
			break;
		/* read data */
		case 0x08:
		case 0x09:
			fdc_read_data();
			break;
		/* write data */
		case 0x0a:
		case 0x0b:
			fdc_write_data();
			break;
		/* read address */
		case 0x0c:
			fdc_read_addr();
			break;
		/* force interrupt */
		case 0x0d:
			fdc_force_intr();
			break;
		/* read track */
		case 0x0e:
			fdc_read_track();
			break;
		/* write track */
		case 0x0f:
			fdc_write_track();
			break;
		/* ����ȊO */
		default:
			ASSERT(FALSE);
			break;
	}
}

/*
 *	FDC
 *	�P�o�C�g�ǂݏo��
 */
BOOL FASTCALL fdc_readb(WORD addr, BYTE *dat)
{
	switch (addr) {
		/* �X�e�[�^�X���W�X�^ */
		case 0xfd18:
			fdc_make_stat();
			*dat = fdc_status;
			/* BUSY���� */
			if ((fdc_status & FDC_ST_BUSY) && (fdc_dataptr == NULL)) {
				fdc_status &= ~FDC_ST_BUSY;
				fdc_access[fdc_drvreg] = FDC_ACCESS_READY;
				return TRUE;
			}
			/* FDC���荞�݂��A�����Ŏ~�߂� */
			mfd_irq_flag = FALSE;
			return TRUE; 

		/* �g���b�N���W�X�^ */
		case 0xfd19:
			*dat = fdc_trkreg;
			return TRUE;

		/* �Z�N�^���W�X�^ */
		case 0xfd1a:
			*dat = fdc_secreg;
			return TRUE;

		/* �f�[�^���W�X�^ */
		case 0xfd1b:
			*dat = fdc_datareg;
			/* �J�E���^���� */
			if (fdc_dataptr) {
				fdc_nowcnt++;
				/* DRQ,IRQ���� */
				if (fdc_nowcnt == fdc_totalcnt) {
					fdc_status &= ~FDC_ST_BUSY;
					fdc_status &= ~FDC_ST_DRQ;
					if ((fdc_cmdtype == 2) && (fdc_command & 0x10)) {
						/* �}���`�Z�N�^���� */
						/* �`�����s�I���v�����X�X�y�V�����΍� */
						fdc_dataptr = NULL;
						fdc_secreg++;
						fdc_read_data();
						return TRUE;
					}
					/* �V���O���Z�N�^ */
					fdc_dataptr = NULL;
					mainetc_fdc();
					fdc_drqirq = 0x40;
					fdc_access[fdc_drvreg] = FDC_ACCESS_READY;
					/* Read Track�͕K��LOST DATA�A�o�b�t�@�C�� */
					if (fdc_cmdtype == 6) {
						fdc_status |= FDC_ST_LOSTDATA;
						fdc_readbuf(fdc_drvreg);
					}
				}
				else {
					fdc_datareg = fdc_dataptr[fdc_nowcnt];
					mainetc_fdc();
					fdc_drqirq = 0x80;
				}
			}
			return TRUE;

		/* �w�b�h���W�X�^ */
		case 0xfd1c:
			*dat = (BYTE)(fdc_sidereg | 0xfe);
			return TRUE;

		/* �h���C�u���W�X�^ */
		case 0xfd1d:
			if (fdc_motor) {
				*dat = (BYTE)(0xbc | fdc_drvreg);
			}
			else {
				*dat = (BYTE)(0x3c | fdc_drvreg);
			}
			return TRUE;

		/* DRQ,IRQ */
		case 0xfd1f:
			*dat = (BYTE)(fdc_drqirq | 0x3f);
			fdc_drqirq = 0;
			/* �f�[�^�]�����Ŗ������BUSY�I��(���z�̐_�a�΍�) */
			if (fdc_dataptr == NULL) {
				fdc_status &= ~FDC_ST_BUSY;
				fdc_access[fdc_drvreg] = FDC_ACCESS_READY;
			}
			return TRUE;
	}

	return FALSE;
}

/*
 *	FDC
 *	�P�o�C�g��������
 */
BOOL FASTCALL fdc_writeb(WORD addr, BYTE dat)
{
	switch (addr) {
		/* �R�}���h���W�X�^ */
		case 0xfd18:
			fdc_command = dat;
			fdc_process_cmd();
			return TRUE;

		/* �g���b�N���W�X�^ */
		case 0xfd19:
			fdc_trkreg = dat;
			return TRUE;

		/* �Z�N�^���W�X�^ */
		case 0xfd1a:
			fdc_secreg = dat;
			return TRUE;

		/* �f�[�^���W�X�^ */
		case 0xfd1b:
			fdc_datareg = dat;
			/* �J�E���^���� */
			if (fdc_dataptr) {
				fdc_dataptr[fdc_nowcnt] = fdc_datareg;
				fdc_nowcnt++;
				/* DRQ,IRQ���� */
				if (fdc_nowcnt == fdc_totalcnt) {
					fdc_status &= ~FDC_ST_BUSY;
					fdc_status &= ~FDC_ST_DRQ;
					if (fdc_cmdtype == 3) {
						if (!fdc_writesec()) {
							fdc_status |= FDC_ST_WRITEFAULT;
						}
					}
					if (fdc_cmdtype == 5) {
						if (!fdc_writetrk()) {
							fdc_status |= FDC_ST_WRITEFAULT;
						}
						/* �t�H�[�}�b�g�̂��ߎg�p�����o�b�t�@���C�� */
						fdc_readbuf(fdc_drvreg);
					}
					if ((fdc_cmdtype == 3) && (fdc_command & 0x10)) {
						/* �}���`�Z�N�^���� */
						fdc_dataptr = NULL;
						fdc_secreg++;
						fdc_write_data();
						return TRUE;
					}
					fdc_dataptr = NULL;
					mainetc_fdc();
					fdc_drqirq = 0x40;
					fdc_access[fdc_drvreg] = FDC_ACCESS_READY;
				}
				else {
					mainetc_fdc();
					fdc_drqirq = 0x80;
				}
			}
			return TRUE;

		/* �w�b�h���W�X�^ */
		case 0xfd1c:
			if ((dat & 0x01) != fdc_sidereg) {
				fdc_sidereg = (BYTE)(dat & 0x01);
				fdc_readbuf(fdc_drvreg);
			}
			return TRUE;

		/* �h���C�u���W�X�^ */
		case 0xfd1d:
			/* �h���C�u�ύX�Ȃ�Afdc_readbuf */
			if (fdc_drvreg != (dat & 0x03)) {
				fdc_drvreg = (BYTE)(dat & 0x03);
				fdc_readbuf(fdc_drvreg);
			}
			fdc_motor = (BYTE)(dat & 0x80);
			/* �h���C�u�����Ȃ�A���[�^�~�߂� */
			if (fdc_drvreg >= FDC_DRIVES) {
				fdc_motor = 0;
			}
			return TRUE;
	}

	return FALSE;
}

/*
 *	FDC
 *	�Z�[�u
 */
BOOL FASTCALL fdc_save(int fileh)
{
	int i;

	/* �t�@�C���֌W���Ɏ����Ă��� */
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_write(fileh, fdc_ready[i])) {
			return FALSE;
		}
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_write(fileh, (BYTE*)fdc_fname[i], 128 + 1)) {
			return FALSE;
		}
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_write(fileh, fdc_media[i])) {
			return FALSE;
		}
	}

	/* Ver4�g�� */
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_bool_write(fileh, fdc_teject[i]));
	}

	/* �t�@�C���X�e�[�^�X */
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_write(fileh, fdc_track[i])) {
			return FALSE;
		}
	}
	if (!file_write(fileh, fdc_buffer, 0x2000)) {
		return FALSE;
	}
	/* fdc_dataptr�͊��Ɉˑ�����f�[�^�|�C���^ */
	if (!file_word_write(fileh, (WORD)(fdc_dataptr - &fdc_buffer[0]))) {
		return FALSE;
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_dword_write(fileh, fdc_seekofs[i])) {
			return FALSE;
		}
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_dword_write(fileh, fdc_secofs[i])) {
			return FALSE;
		}
	}

	/* I/O */
	if (!file_byte_write(fileh, fdc_command)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_status)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_trkreg)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_secreg)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_datareg)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_sidereg)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_drvreg)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_motor)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_drqirq)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_cmdtype)) {
		return FALSE;
	}

	/* ���̑� */
	if (!file_word_write(fileh, fdc_totalcnt)) {
		return FALSE;
	}
	if (!file_word_write(fileh, fdc_nowcnt)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, fdc_seekvct)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, fdc_indexcnt)) {
		return FALSE;
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_write(fileh, fdc_access[i])) {
			return FALSE;
		}
	}
	if (!file_bool_write(fileh, fdc_boot)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	FDC
 *	���[�h
 */
BOOL FASTCALL fdc_load(int fileh, int ver)
{
	int i;
	BYTE ready[FDC_DRIVES];
	char fname[FDC_DRIVES][128 + 1];
	BYTE media[FDC_DRIVES];
	WORD offset;

	/* �o�[�W�����`�F�b�N */
	if (ver < 2) {
		return FALSE;
	}

	/* �t�@�C���֌W���Ɏ����Ă��� */
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_read(fileh, &ready[i])) {
			return FALSE;
		}
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_read(fileh, (BYTE*)fname[i], 128 + 1)) {
			return FALSE;
		}
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_read(fileh, &media[i])) {
			return FALSE;
		}
	}

	/* �ă}�E���g�����݂� */
	for (i=0; i<FDC_DRIVES; i++) {
		fdc_setdisk(i, NULL);
		if (ready[i] != FDC_TYPE_NOTREADY) {
			fdc_setdisk(i, fname[i]);
			if (fdc_ready[i] != FDC_TYPE_NOTREADY) {
				if (fdc_medias[i] >= (media[i] + 1)) {
					fdc_setmedia(i, media[i]);
				}
			}
		}
	}

	/* Ver4�g�� */
	if (ver >= 4) {
		for (i=0; i<FDC_DRIVES; i++) {
			if (!file_bool_read(fileh, &fdc_teject[i])) {
				return FALSE;
			}
		}
	}
	else {
		memset(fdc_teject, 0, sizeof(fdc_teject));
	}

	/* �t�@�C���X�e�[�^�X */
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_read(fileh, &fdc_track[i])) {
			return FALSE;
		}
	}
	if (!file_read(fileh, fdc_buffer, 0x2000)) {
		return FALSE;
	}
	/* fdc_dataptr�͊��Ɉˑ�����f�[�^�|�C���^ */
	if (!file_word_read(fileh, &offset)) {
		return FALSE;
	}
	fdc_dataptr = &fdc_buffer[offset];
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_dword_read(fileh, &fdc_seekofs[i])) {
			return FALSE;
		}
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_dword_read(fileh, &fdc_secofs[i])) {
			return FALSE;
		}
	}

	/* I/O */
	if (!file_byte_read(fileh, &fdc_command)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_status)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_trkreg)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_secreg)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_datareg)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_sidereg)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_drvreg)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_motor)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_drqirq)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_cmdtype)) {
		return FALSE;
	}

	/* ���̑� */
	if (!file_word_read(fileh, &fdc_totalcnt)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &fdc_nowcnt)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &fdc_seekvct)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &fdc_indexcnt)) {
		return FALSE;
	}
	for (i=0; i<FDC_DRIVES; i++) {
		if (!file_byte_read(fileh, &fdc_access[i])) {
			return FALSE;
		}
	}
	if (!file_bool_read(fileh, &fdc_boot)) {
		return FALSE;
	}

	return TRUE;
}

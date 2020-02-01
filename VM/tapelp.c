/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �J�Z�b�g�e�[�v���v�����^ ]
 */

#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "tapelp.h"
#include "mainetc.h"
#include "device.h"

/*
 *	�O���[�o�� ���[�N
 */
BOOL tape_in;							/* �e�[�v ���̓f�[�^ */
BOOL tape_out;							/* �e�[�v �o�̓f�[�^ */
BOOL tape_motor;						/* �e�[�v ���[�^ */
BOOL tape_rec;							/* �e�[�v REC�t���O */
BOOL tape_writep;						/* �e�[�v �������݋֎~ */
WORD tape_count;						/* �e�[�v �T�C�N���J�E���^ */
BYTE tape_subcnt;						/* �e�[�v �T�u�J�E���^ */
int tape_fileh;							/* �e�[�v �t�@�C���n���h�� */
DWORD tape_offset;						/* �e�[�v �t�@�C���I�t�Z�b�g */
char tape_fname[128+1];					/* �e�[�v �t�@�C���l�[�� */

WORD tape_incnt;						/* �e�[�v �ǂݍ��݃J�E���^ */
DWORD tape_fsize;						/* �e�[�v �t�@�C���T�C�Y */

BYTE lp_data;							/* �v�����^ �o�̓f�[�^ */
BOOL lp_busy;							/* �v�����^ BUSY�t���O */
BOOL lp_error;							/* �v�����^ �G���[�t���O */
BOOL lp_pe;								/* �v�����^ PE�t���O */
BOOL lp_ackng;							/* �v�����^ ACK�t���O */
BOOL lp_online;							/* �v�����^ �I�����C�� */
BOOL lp_strobe;							/* �v�����^ �X�g���[�u */
int lp_fileh;							/* �v�����^ �t�@�C���n���h�� */

char lp_fname[128+1];					/* �v�����^ �t�@�C���l�[�� */

/*
 *	�J�Z�b�g�e�[�v���v�����^
 *	������
 */
BOOL FASTCALL tapelp_init(void)
{
	/* �e�[�v */
	tape_fileh = -1;
	tape_fname[0] = '\0';
	tape_offset = 0;
	tape_fsize = 0;
	tape_writep = FALSE;

	/* �v�����^ */
	lp_fileh = -1;
	lp_fname[0] = '\0';

	return TRUE;
}

/*
 *	�J�Z�b�g�e�[�v���v�����^
 *	�N���[���A�b�v
 */
void FASTCALL tapelp_cleanup(void)
{
	/* �t�@�C�����J���Ă���΁A���� */
	if (tape_fileh != -1) {
		file_close(tape_fileh);
		tape_fileh = -1;
	}

	/* ���[�^OFF */
	tape_motor = FALSE;

	/* �t�@�C�����J���Ă���΁A���� */
	if (lp_fileh != -1) {
		file_close(lp_fileh);
		lp_fileh = -1;
	}
}

/*
 *	�J�Z�b�g�e�[�v���v�����^
 *	���Z�b�g
 */
void FASTCALL tapelp_reset(void)
{
	tape_motor = FALSE;
	tape_rec = FALSE;
	tape_count = 0;
	tape_in = FALSE;
	tape_out = FALSE;
	tape_incnt = 0;
	tape_subcnt = 0;

	lp_busy = FALSE;
	lp_error = FALSE;
	lp_ackng = TRUE;
	lp_pe = FALSE;
	lp_online = FALSE;
	lp_strobe = FALSE;
}

/*-[ �v�����^ ]-------------------------------------------------------------*/

/*
 *	�v�����^
 *	�f�[�^�o��
 */
static void FASTCALL lp_output(BYTE dat)
{
	/* �I�[�v�����Ă��Ȃ���΁C�J�� */
	if (lp_fileh == -1) {
		if (lp_fname[0] != '\0') {
			lp_fileh = file_open(lp_fname, OPEN_W);
		}
	}

	/* �I�[�v���`�F�b�N */
	if (lp_fileh == -1) {
		return;
	}

	/* �A�y���h */
	file_write(lp_fileh, &dat, 1);
}

/*
 *	�v�����^
 *	�t�@�C�����ݒ�
 */
void FASTCALL lp_setfile(char *fname)
{
	/* ��x�J���Ă���΁A���� */
	if (lp_fileh != -1) {
		file_close(lp_fileh);
		lp_fileh = -1;
	}

	/* �t�@�C�����Z�b�g */
	if (fname == NULL) {
		lp_fname[0] = '\0';
		return;
	}

	if (strlen(fname) < sizeof(lp_fname)) {
		strcpy(lp_fname, fname);
	}
	else {
		lp_fname[0] = '\0';
	}
}

/*-[ �e�[�v ]---------------------------------------------------------------*/

/*
 *	�e�[�v
 *	�f�[�^����
 */
static void FASTCALL tape_input(void)
{
	BYTE high;
	BYTE low;
	WORD dat;

	/* ���[�^������Ă��邩 */
	if (tape_motor == FALSE) {
		return;
	}

	/* �^������Ă���Γ��͂ł��Ȃ� */
	if (tape_rec) {
		return;
	}

	/* �V���O���J�E���^�����̓J�E���^���z���Ă���΁A0�ɂ��� */
	while (tape_count >= tape_incnt) {
		tape_count -= tape_incnt;
		tape_incnt = 0;

		/* �f�[�^�t�F�b�` */
		tape_in = FALSE;

		if (tape_fileh == -1) {
			return;
		}

		if (tape_offset >= tape_fsize) {
			return;
		}

		if (!file_read(tape_fileh, &high, 1)) {
			return;
		}
		if (!file_read(tape_fileh, &low, 1)) {
			return;
		}

		/* �I�t�Z�b�g�X�V */
		tape_offset += 2;

		/* �f�[�^�A�J�E���^�ݒ� */
		dat = (WORD)(high * 256 + low);
		if (dat > 0x7fff) {
			tape_in = TRUE;
		}
		tape_incnt = (WORD)(dat & 0x7fff);

		/* �J�E���^���J�肷�� */
		if (tape_count > tape_incnt) {
			tape_count -= tape_incnt;
			tape_incnt = 0;
		}
		else {
			tape_incnt -= tape_count;
			tape_count = 0;
		}
	}
}

/*
 *	�e�[�v
 *	�f�[�^�o��
 */
static void FASTCALL tape_output(BOOL flag)
{
	WORD dat;
	BYTE high, low;

	/* �e�[�v������Ă��邩 */
	if (tape_motor == FALSE) {
		return;
	}

	/* �^������ */
	if (tape_rec == FALSE) {
		return;
	}

	/* �J�E���^������Ă��邩 */
	if (tape_count == 0) {
		return;
	}

	/* �������݉\�� */
	if (tape_writep) {
		return;
	}

	/* �t�@�C�����I�[�v������Ă���΁A�f�[�^�������� */
	dat = tape_count;
	if (dat >= 0x8000) {
		dat = 0x7fff;
	}
	if (flag) {
		dat |= 0x8000;
	}
	high = (BYTE)(dat >> 8);
	low = (BYTE)(dat & 0xff);
	if (tape_fileh != -1) {
		if (file_write(tape_fileh, &high, 1)) {
			if (file_write(tape_fileh, &low, 1)) {
				tape_offset += 2;
				if (tape_offset >= tape_fsize) {
					tape_fsize = tape_offset;
				}
			}
		}
	}

	/* �J�E���^�����Z�b�g */
	tape_count = 0;
	tape_subcnt = 0;
}

/*
 *	�e�[�v
 *	�}�[�J�o��
 */
static void FASTCALL tape_mark(void)
{
	BYTE dat;

	/* �e�[�v������Ă��邩 */
	if (tape_motor == FALSE) {
		return;
	}

	/* �^������ */
	if (tape_rec == FALSE) {
		return;
	}

	/* �������݉\�� */
	if (tape_writep) {
		return;
	}

	/* �t�@�C�����I�[�v������Ă���΁A�f�[�^�������� */
	if (tape_fileh != -1) {
		dat = 0;
		if (file_write(tape_fileh, &dat, 1)) {
			if (file_write(tape_fileh, &dat, 1)) {
				tape_offset += 2;
				if (tape_offset >= tape_fsize) {
					tape_fsize = tape_offset;
				}
			}
		}
	}
}

/*
 *	�e�[�v
 *	�����߂�
 */
void FASTCALL tape_rew(void)
{
	WORD dat;

	/* �������� */
	if (tape_fileh == -1) {
		return;
	}

	/* assert */
	ASSERT(tape_fsize >= 16);
	ASSERT(tape_offset >= 16);
	ASSERT(!(tape_fsize & 0x01));
	ASSERT(!(tape_offset & 0x01));

	while (tape_offset > 16) {
		/* �Q�o�C�g�O�ɖ߂�A�ǂݍ��� */
		tape_offset -= 2;
		if (!file_seek(tape_fileh, tape_offset)) {
			return;
		}
		file_read(tape_fileh, (BYTE *)&dat, 2);

		/* $0000�Ȃ�A�����ɐݒ� */
		if (dat == 0) {
			file_seek(tape_fileh, tape_offset);
			return;
		}

		/* ���ܓǂݍ��񂾕������߂� */
		if (!file_seek(tape_fileh, tape_offset)) {
			return;
		}
	}
}

/*
 *	�e�[�v
 *	������
 */
void FASTCALL tape_ff(void)
{
	WORD dat;

	/* �������� */
	if (tape_fileh == -1) {
		return;
	}

	/* assert */
	ASSERT(tape_fsize >= 16);
	ASSERT(tape_offset >= 16);
	ASSERT(!(tape_fsize & 0x01));
	ASSERT(!(tape_offset & 0x01));

	while (tape_offset < tape_fsize) {
		/* ��֐i�߂� */
		tape_offset += 2;
		if (tape_offset >= tape_fsize){
			return;
		}
		if (!file_seek(tape_fileh, tape_offset)) {
			return;
		}
		file_read(tape_fileh, (BYTE *)&dat, 2);

		/* $0000�Ȃ�A���̎��ɐݒ� */
		if (dat == 0) {
			tape_offset += 2;
			if (tape_offset >= tape_fsize) {
				tape_fsize = tape_offset;
			}
			return;
		}
	}
}

/*
 *	�e�[�v
 *	�t�@�C�����ݒ�
 */
void FASTCALL tape_setfile(char *fname)
{
	char *header = "XM7 TAPE IMAGE 0";
	char buf[17];

	/* ��x�J���Ă���΁A���� */
	if (tape_fileh != -1) {
		file_close(tape_fileh);
		tape_fileh = -1;
		tape_writep = FALSE;
	}

	/* �t�@�C�����Z�b�g */
	if (fname == NULL) {
		tape_fname[0] = '\0';
	}
	else {
		if (strlen(fname) < sizeof(tape_fname)) {
			strcpy(tape_fname, fname);
		}
		else {
			tape_fname[0] = '\0';
		}
	}

	/* �t�@�C���I�[�v�������݂� */
	if (tape_fname[0] != '\0') {
		tape_fileh = file_open(tape_fname, OPEN_RW);
		if (tape_fileh != -1) {
			tape_writep = FALSE;
		}
		else {
			tape_fileh = file_open(tape_fname, OPEN_R);
			tape_writep = TRUE;
		}
	}

	/* �J���Ă���΁A�w�b�_��ǂݍ��݃`�F�b�N */
	if (tape_fileh != -1) {
		memset(buf, 0, sizeof(buf));
		file_read(tape_fileh, (BYTE*)buf, 16);
		if (strcmp(buf, header) != 0) {
			file_close(tape_fileh);
			tape_fileh = -1;
			tape_writep = FALSE;
		}
	}

	/* �t���O�̏��� */
	tape_setrec(FALSE);
	tape_count = 0;
	tape_incnt = 0;
	tape_subcnt = 0;

	/* �t�@�C�����J���Ă���΁A�t�@�C���T�C�Y�A�I�t�Z�b�g������ */
	if (tape_fileh != -1) {
		tape_fsize = file_getsize(tape_fileh);
		tape_offset = 16;
	}
}

/*
 *	�e�[�v
 *	�^���t���O�ݒ�
 */
void FASTCALL tape_setrec(BOOL flag)
{
	/* ���[�^������Ă���΁A�}�[�J���������� */
	if (tape_motor && !tape_rec) {
		if (flag == TRUE) {
			tape_rec = TRUE;
			tape_mark();
			return;
		}
	}

	tape_rec = flag;
}

/*-[ ������R/W ]------------------------------------------------------------*/

/*
 *	�J�Z�b�g�e�[�v���v�����^
 *	�P�o�C�g�ǂݏo��
 */
BOOL FASTCALL tapelp_readb(WORD addr, BYTE *dat)
{
	BYTE ret;

	/* �A�h���X�`�F�b�N */
	if (addr != 0xfd02) {
		return FALSE;
	}

	/* �v�����^ �X�e�[�^�X�쐬 */
	ret = 0x30;
	if (lp_busy) {
		ret |= 0x01;
	}
	if (!lp_error) {
		ret |= 0x02;
	}
	if (!lp_ackng) {
		ret |= 0x04;
	}
	if (lp_pe) {
		ret |= 0x08;
	}

	/* WIN32�łł̓v�����^���ڑ� */
#ifdef _WIN32
	ret |= 0x0f;
#endif

	/* �J�Z�b�g �f�[�^�쐬 */
	tape_input();
	if (tape_in) {
		ret |= 0x80;
	}

	/* ���� */
	*dat = ret;
	return TRUE;
}

/*
 *	�J�Z�b�g�e�[�v���v�����^
 *	�P�o�C�g��������
 */
BOOL FASTCALL tapelp_writeb(WORD addr, BYTE dat)
{
	switch (addr) {
		/* �J�Z�b�g����A�v�����^���� */
		case 0xfd00:
			/* �v�����^ �I�����C�� */
			if (dat & 0x80) {
				lp_online = FALSE;
			}
			else {
				lp_online = TRUE;
			}

			/* �v�����^ �X�g���[�u */
			if (dat & 0x40) {
				lp_strobe = TRUE;
			}
			else {
				if (lp_strobe && lp_online) {
					lp_output(lp_data);
					mainetc_lp();
				}
				lp_strobe = FALSE;
			}

			/* �e�[�v �o�̓f�[�^ */
			if (dat & 0x01) {
				if (!tape_out) {
					tape_output(FALSE);
				}
				tape_out = TRUE;
			}
			else {
				if (tape_out) {
					tape_output(TRUE);
				}
				tape_out = FALSE;
			}

			/* �e�[�v ���[�^ */
			if (dat & 0x02) {
				if (tape_motor == FALSE) {
					/* �V�K�X�^�[�g */
					tape_count = 0;
					tape_subcnt = 0;
					tape_motor = TRUE;
					if (tape_rec == TRUE) {
						tape_mark();
					}
				}
			}
			else {
				/* ���[�^��~ */
				tape_motor = FALSE;
			}

			return TRUE;

		/* �v�����^�o�̓f�[�^ */
		case 0xfd01:
			lp_data = dat;
			return TRUE;
	}

	return FALSE;
}

/*
 *	�J�Z�b�g�e�[�v���v�����^
 *	�Z�[�u
 */
BOOL FASTCALL tapelp_save(int fileh)
{
	BOOL tmp;

	if (!file_bool_write(fileh, tape_in)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, tape_out)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, tape_motor)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, tape_rec)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, tape_writep)) {
		return FALSE;
	}
	if (!file_word_write(fileh, tape_count)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, tape_subcnt)) {
		return FALSE;
	}

	if (!file_dword_write(fileh, tape_offset)) {
		return FALSE;
	}
	if (!file_write(fileh, (BYTE*)tape_fname, 128 + 1)) {
		return FALSE;
	}
	tmp = (tape_fileh != -1);
	if (!file_bool_write(fileh, tmp)) {
		return FALSE;
	}

	if (!file_word_write(fileh, tape_incnt)) {
		return FALSE;
	}
	if (!file_dword_write(fileh, tape_fsize)) {
		return FALSE;
	}

	if (!file_byte_write(fileh, lp_data)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, lp_busy)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, lp_error)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, lp_pe)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, lp_ackng)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, lp_online)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, lp_strobe)) {
		return FALSE;
	}

	if (!file_write(fileh, (BYTE*)lp_fname, 128 + 1)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	�J�Z�b�g�e�[�v���v�����^
 *	���[�h
 */
BOOL FASTCALL tapelp_load(int fileh, int ver)
{
	DWORD offset;
	char fname[128 + 1];
	BOOL flag;

	/* �o�[�W�����`�F�b�N */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &tape_in)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &tape_out)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &tape_motor)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &tape_rec)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &tape_writep)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &tape_count)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &tape_subcnt)) {
		return FALSE;
	}

	if (!file_dword_read(fileh, &offset)) {
		return FALSE;
	}
	if (!file_read(fileh, (BYTE*)fname, 128 + 1)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &flag)) {
		return FALSE;
	}

	/* �}�E���g */
	tape_setfile(NULL);
	if (flag) {
		tape_setfile(fname);
		if ((tape_fileh != -1) && ((tape_fsize +1 ) >= tape_offset)) {
			file_seek(tape_fileh, offset);
		}
	}

	if (!file_word_read(fileh, &tape_incnt)) {
		return FALSE;
	}
	/* tape_fsize�͖��� */
	if (!file_dword_read(fileh, &offset)) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &lp_data)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &lp_busy)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &lp_error)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &lp_pe)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &lp_ackng)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &lp_online)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &lp_strobe)) {
		return FALSE;
	}

	if (!file_read(fileh, (BYTE*)lp_fname, 128 + 1)) {
		return FALSE;
	}

	return TRUE;
}

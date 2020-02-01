/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �f�B�X�v���C ]
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "xm7.h"
#include "display.h"
#include "subctrl.h"
#include "device.h"
#include "ttlpalet.h"
#include "multipag.h"
#include "mainetc.h"
#include "aluline.h"
#include "keyboard.h"
#include "event.h"

/*
 *	�O���[�o�� ���[�N
 */
BOOL crt_flag;							/* CRT ON�t���O */
BOOL vrama_flag;						/* VRAM�A�N�Z�X�t���O */
WORD vram_offset[2];					/* VRAM�I�t�Z�b�g���W�X�^ */
WORD crtc_offset[2];					/* CRTC�I�t�Z�b�g */
BOOL vram_offset_flag;					/* �g��VRAM�I�t�Z�b�g���W�X�^�t���O */
BOOL subnmi_flag;						/* �T�uNMI�C�l�[�u���t���O */
BOOL vsync_flag;						/* VSYNC�t���O */
BOOL blank_flag;						/* �u�����L���O�t���O */

BYTE vram_active;						/* �A�N�e�B�u�y�[�W */
BYTE *vram_aptr;						/* VRAM�A�N�e�B�v�|�C���^ */
BYTE vram_display;						/* �\���y�[�W */
BYTE *vram_dptr;						/* VRAM�\���|�C���^ */

/*
 *	�X�^�e�B�b�N ���[�N
 */
static BYTE *vram_buf;					/* VRAM�X�N���[���o�b�t�@ */
static BYTE vram_offset_count[2];		/* VRAM�I�t�Z�b�g�ݒ�J�E���^ */
static WORD hsync_count;				/* HSYNC�J�E���^ */

/*
 *	�v���g�^�C�v�錾
 */
static BOOL FASTCALL subcpu_event(void); /* �T�uCPU�^�C�}�C�x���g */
static BOOL FASTCALL display_vsync(void);/* VSYNC�C�x���g */
static BOOL FASTCALL display_blank(void);/* VBLANK,HBLANK�C�x���g */

/*
 *	�f�B�X�v���C
 *	������
 */
BOOL FASTCALL display_init(void)
{
	vram_buf = (BYTE *)malloc(0x4000);

	if (vram_buf == NULL) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	�f�B�X�v���C
 *	�N���[���A�b�v
 */
void FASTCALL display_cleanup(void)
{
	ASSERT(vram_buf);
	if (vram_buf) {
		free(vram_buf);
	}
}

/*
 *	�f�B�X�v���C
 *	���Z�b�g
 */
void FASTCALL display_reset(void)
{
	WORD addr;

	/* CRT���W�X�^ */
	crt_flag = FALSE;
	vrama_flag = FALSE;
	memset(vram_offset, 0, sizeof(vram_offset));
	memset(crtc_offset, 0, sizeof(crtc_offset));
	vram_offset_flag = FALSE;
	memset(vram_offset_count, 0, sizeof(vram_offset_count));

	/* ���荞�݁A�C�x���g */
	subnmi_flag = TRUE;
	vsync_flag = FALSE;
	blank_flag = TRUE;

	/* �A�N�e�B�u�y�[�W�A�\���y�[�W������ */
	vram_active = 0;
	vram_aptr = vram_c;
	vram_display = 0;
	vram_dptr = vram_c;

	/* �{���AVRAM�N���A�͕K�v�Ȃ� */
	memset(vram_b, 0, 0xc000);
	memset(vram_c, 0, 0xc000);
	for (addr=0; addr<0xc000; addr++) {
		vram_notify(addr, 0);
	}

	/* 20ms���ƂɋN�����C�x���g��ǉ� */
	schedule_setevent(EVENT_SUBTIMER, 20000, subcpu_event);

	/* VSYNC�C�x���g���쐬 */
	schedule_setevent(EVENT_VSYNC, 1520, display_vsync);

	/* V,HBLANK�C�x���g���쐬 */
	schedule_setevent(EVENT_BLANK, 3940, display_blank);
	hsync_count = 300;
}

/*
 *	VSYNC�C�x���g
 */
static BOOL FASTCALL display_vsync(void)
{
	if (vsync_flag == FALSE) {
		/* ���ꂩ�琂������ 0.51ms */
		vsync_flag = TRUE;
		schedule_setevent(EVENT_VSYNC, 510, display_vsync);

		/* �r�f�I�f�B�W�^�C�Y */
		if (digitize_enable) {
			if (digitize_keywait || simpose_mode == 0x03) {
				digitize_notify();
			}
		}
	}
	else {
		/* ���ꂩ�琂���\�� 1.91ms + 12.7ms + 1.52ms */
		vsync_flag = FALSE;
		schedule_setevent(EVENT_VSYNC, 1910+12700+1520, display_vsync);
		vsync_notify();
	}

	return TRUE;
}

/*
 *	V/H BLANK�C�x���g
 */
static BOOL FASTCALL display_blank(void)
{
	if (hsync_count >= 200) {
		if (blank_flag == FALSE) {
			/* ���ꂩ��V-BLANK 3.94ms */
			blank_flag = TRUE;
			schedule_setevent(EVENT_BLANK, 3940, display_blank);
			return TRUE;
		}
		else {
			/* ���ꂩ��0���C���� 24��s */
			blank_flag = TRUE;
			schedule_setevent(EVENT_BLANK, 24, display_blank);
			hsync_count = 0;
			return TRUE;
		}
	}

	if (blank_flag == TRUE) {
		/* ���ꂩ�琅���\������ 39��s��������40��s */
		blank_flag = FALSE;
		schedule_setevent(EVENT_BLANK, 39 + (hsync_count & 1), display_blank);
		hsync_count++;
		return TRUE;
	}
	else {
		/* ������������ 24��s */
		blank_flag = TRUE;
		schedule_setevent(EVENT_BLANK, 24, display_blank);
		return TRUE;
	}
}

/*
 *	�T�uCPU
 *	�C�x���g����
 */	
static BOOL FASTCALL subcpu_event(void)
{
	/* �O�̂��߁A�`�F�b�N */
	if (!subnmi_flag) {
		return FALSE;
	}

	/* NMI���荞�݂��N���� */
	subcpu_nmi();
	return TRUE;
}

/*
 *	VRAM�X�N���[��
 */
static void FASTCALL vram_scroll(WORD offset)
{
	int i;
	BYTE *vram;

	if (mode320) {
		/* 320�� �I�t�Z�b�g�}�X�N */
		offset &= 0x1fff;

		/* ���[�v */
		for (i=0; i<6; i++) {
			vram = (BYTE *)(vram_aptr + 0x2000 * i);

			/* �e���|�����o�b�t�@�փR�s�[ */
			memcpy(vram_buf, vram, offset);

			/* �O�֋l�߂� */
			memcpy(vram, (vram + offset), 0x2000 - offset);

			/* �e���|�����o�b�t�@��蕜�� */
			memcpy(vram + (0x2000 - offset), vram_buf, offset);
		}
	}
	else {
		/* 640�� �I�t�Z�b�g�}�X�N */
		offset &= 0x3fff;

		/* ���[�v */
		for (i=0; i<3; i++) {
			vram = (BYTE *)(vram_aptr + 0x4000 * i);

			/* �e���|�����o�b�t�@�փR�s�[ */
			memcpy(vram_buf, vram, offset);

			/* �O�֋l�߂� */
			memcpy(vram, (vram + offset), 0x4000 - offset);

			/* �e���|�����o�b�t�@��蕜�� */
			memcpy(vram + (0x4000 - offset), vram_buf, offset);
		}
	}
}
/*
 *	�f�B�X�v���C
 *	�P�o�C�g�ǂݍ���
 *	�����C���|�T�u�C���^�t�F�[�X�M�������܂�
 */
BOOL FASTCALL display_readb(WORD addr, BYTE *dat)
{
	BYTE ret;

	switch (addr) {
		/* �L�����Z��IRQ ACK */
		case 0xd402: 
			subcancel_flag = FALSE;
			subcpu_irq();
			*dat = 0xff;
			return TRUE;

		/* BEEP */
		case 0xd403:
			beep_flag = TRUE;
			schedule_setevent(EVENT_BEEP, 205000, mainetc_beep);
			return TRUE;

		/* �A�e���V����IRQ ON */
		case 0xd404:
			subattn_flag = TRUE;
			*dat = 0xff;
			maincpu_firq();
			return TRUE;

		/* CRT ON */
		case 0xd408:
			if (!crt_flag) {
				crt_flag = TRUE;
				/* CRT OFF��ON */
				multipag_writeb(0xfd37, multi_page);
			}
			*dat = 0xff;
			return TRUE;

		/* VRAM�A�N�Z�X ON */
		case 0xd409:
			vrama_flag = TRUE;
			*dat = 0xff;
			return TRUE;

		/* BUSY�t���O OFF */
		case 0xd40a:
			subbusy_flag = FALSE;
			*dat = 0xff;
			return TRUE;

		/* FM77AV MISC���W�X�^ */
		case 0xd430:
			if (fm7_ver >= 2) {
				ret = 0xff;

				/* �u�����L���O */
				if (blank_flag) {
					ret &= (BYTE)~0x80;
				}

				/* ������� */
				if (line_busy) {
					ret &= (BYTE)~0x10;
				}

				/* VSYNC */
				if (!vsync_flag) {
					ret &= (BYTE)~0x04;
				}

				/* �T�uRESET�X�e�[�^�X */
				if (!subreset_flag) {
					ret &= (BYTE)~0x01;
				}

				*dat = ret;
				return TRUE;
			}
	}

	return FALSE;
}

/*
 *	�f�B�X�v���C
 *	�P�o�C�g��������
 *	�����C���|�T�u�C���^�t�F�[�X�M�������܂�
 */
BOOL FASTCALL display_writeb(WORD addr, BYTE dat)
{
	WORD offset;

	switch (addr) {
		/* CRT OFF */
		case 0xd408:
			if (crt_flag) {
				/* CRT ON��OFF */
				crt_flag = FALSE;
				multipag_writeb(0xfd37, multi_page);
			}
			crt_flag = FALSE;
			return TRUE;

		/* VRAM�A�N�Z�X OFF */
		case 0xd409:
			vrama_flag = FALSE;
			return TRUE;

		/* BUSY�t���O ON */
		case 0xd40a:
			subbusy_flag = TRUE;
			return TRUE;

		/* VRAM�I�t�Z�b�g�A�h���X ��� */
		case 0xd40e:
			offset = (WORD)(dat & 0x3f);
			offset <<= 8;
			offset |= (WORD)(vram_offset[vram_active] & 0xff);
			vram_offset[vram_active] = offset;
			/* �J�E���g�A�b�v�A�X�N���[�� */
			vram_offset_count[vram_active]++;
			if ((vram_offset_count[vram_active] & 1) == 0) {
				vram_scroll((WORD)(vram_offset[vram_active] -
									crtc_offset[vram_active]));
				crtc_offset[vram_active] = vram_offset[vram_active];
				display_notify();
			}
			return TRUE;

		/* VRAM�I�t�Z�b�g�A�h���X ���� */
		case 0xd40f:
			/* �g���t���O���Ȃ���΁A����5bit�͖��� */
			if (!vram_offset_flag) {
				dat &= 0xe0;
			}
			offset = (WORD)(vram_offset[vram_active] & 0x3f00);
			offset |= (WORD)dat;
			vram_offset[vram_active] = offset;
			/* �J�E���g�A�b�v�A�X�N���[�� */
			vram_offset_count[vram_active]++;
			if ((vram_offset_count[vram_active] & 1) == 0) {
				vram_scroll((WORD)(vram_offset[vram_active] -
									crtc_offset[vram_active]));
				crtc_offset[vram_active] = vram_offset[vram_active];
				display_notify();
			}
			return TRUE;

		/* FM77AV MISC���W�X�^ */
		case 0xd430:
			/* NMI�}�X�N */
			if (dat & 0x80) {
				subnmi_flag = FALSE;
				event[EVENT_SUBTIMER].flag = EVENT_DISABLED;
				subcpu.intr &= ~INTR_NMI;
			}
			else {
				subnmi_flag = TRUE;
				event[EVENT_SUBTIMER].flag = EVENT_ENABLED;
			}

			/* �f�B�X�v���C�y�[�W */
			if (dat & 0x40) {
				if (vram_display == 0) {
					vram_display = 1;
					vram_dptr = vram_b;
					display_notify();
				}
			}
			else {
				if (vram_display == 1){
					vram_display = 0;
					vram_dptr = vram_c;
					display_notify();
				}
			}

			/* �A�N�e�B�u�y�[�W */
			if (dat & 0x20) {
				vram_active = 1;
				vram_aptr = vram_b;
			}
			else {
				vram_active = 0;
				vram_aptr = vram_c;
			}

			/* �g��VRAM�I�t�Z�b�g���W�X�^ */
			if (dat & 0x04) {
				if (!vram_offset_flag) {
					vram_offset_flag = TRUE;
				}
			}
			else {
				if (vram_offset_flag) {
					vram_offset_flag = FALSE;
				}
			}

			/* CGROM�o���N */
			cgrom_bank = (BYTE)(dat & 0x03);
			return TRUE;

	}

	return FALSE;
}

/*
 *	�f�B�X�v���C
 *	�Z�[�u
 */
BOOL FASTCALL display_save(int fileh)
{
	if (!file_bool_write(fileh, crt_flag)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, vrama_flag)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, subnmi_flag)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, vsync_flag)) {
		return FALSE;
	}
	if (!file_bool_write(fileh, blank_flag)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, vram_offset_flag)) {
		return FALSE;
	}
	if (!file_word_write(fileh, vram_offset[0])) {
		return FALSE;
	}
	if (!file_word_write(fileh, vram_offset[1])) {
		return FALSE;
	}
	if (!file_word_write(fileh, crtc_offset[0])) {
		return FALSE;
	}
	if (!file_word_write(fileh, crtc_offset[1])) {
		return FALSE;
	}

	if (!file_byte_write(fileh, vram_active)) {
		return FALSE;
	}
	if (!file_byte_write(fileh, vram_display)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	�f�B�X�v���C
 *	���[�h
 */
BOOL FASTCALL display_load(int fileh, int ver)
{
	/* �o�[�W�����`�F�b�N */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &crt_flag)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &vrama_flag)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &subnmi_flag)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &vsync_flag)) {
		return FALSE;
	}
	if (!file_bool_read(fileh, &blank_flag)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &vram_offset_flag)) {
		return FALSE;
	}
	if (!file_word_read(fileh, &vram_offset[0])) {
		return FALSE;
	}
	if (!file_word_read(fileh, &vram_offset[1])) {
		return FALSE;
	}
	if (!file_word_read(fileh, &crtc_offset[0])) {
		return FALSE;
	}
	if (!file_word_read(fileh, &crtc_offset[1])) {
		return FALSE;
	}

	if (!file_byte_read(fileh, &vram_active)) {
		return FALSE;
	}
	if (!file_byte_read(fileh, &vram_display)) {
		return FALSE;
	}

	/* �|�C���^���\�� */
	if (vram_active == 0) {
		vram_aptr = vram_c;
	}
	else {
		vram_aptr = vram_b;
	}
	if (vram_display == 0) {
		vram_dptr = vram_c;
	}
	else {
		vram_dptr = vram_b;
	}

	/* �C�x���g */
	schedule_handle(EVENT_SUBTIMER, subcpu_event);
	schedule_handle(EVENT_VSYNC, display_vsync);
	schedule_handle(EVENT_BLANK, display_blank);

	return TRUE;
}

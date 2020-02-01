/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �V�X�e���Ǘ� ]
 */

#include <assert.h>
#include <string.h>
#include "xm7.h"
#include "display.h"
#include "ttlpalet.h"
#include "subctrl.h"
#include "keyboard.h"
#include "fdc.h"
#include "mainetc.h"
#include "multipag.h"
#include "kanji.h"
#include "tapelp.h"
#include "display.h"
#include "opn.h"
#include "mmr.h"
#include "aluline.h"
#include "apalet.h"
#include "rtc.h"
#include "whg.h"
#include "device.h"

/*
 *	�O���[�o�� ���[�N
 */
int fm7_ver;							/* �n�[�h�E�F�A�o�[�W���� */
int boot_mode;							/* �N�����[�h BASIC/DOS */

/*
 *	�V�X�e��
 *	������
 */
BOOL FASTCALL system_init(void)
{
	/* ���[�h�ݒ� */
	fm7_ver = 2;					/* FM77AV�����ɐݒ� */
	boot_mode = BOOT_BASIC;			/* BASIC MODE */

	/* �X�P�W���[���A�������o�X */
	if (!schedule_init()) {
		return FALSE;
	}
	if (!mmr_init()) {
		return FALSE;
	}

	/* �������ACPU */
	if (!mainmem_init()) {
		return FALSE;
	}
	if (!submem_init()) {
		return FALSE;
	}
	if (!maincpu_init()) {
		return FALSE;
	}
	if (!subcpu_init()) {
		return FALSE;
	}

	/* ���̑��f�o�C�X */
	if (!display_init()) {
		return FALSE;
	}
	if (!ttlpalet_init()) {
		return FALSE;
	}
	if (!subctrl_init()) {
		return FALSE;
	}
	if (!keyboard_init()) {
		return FALSE;
	}
	if (!fdc_init()) {
		return FALSE;
	}
	if (!mainetc_init()) {
		return FALSE;
	}
	if (!multipag_init()) {
		return FALSE;
	}
	if (!kanji_init()) {
		return FALSE;
	}
	if (!tapelp_init()) {
		return FALSE;
	}
	if (!opn_init()) {
		return FALSE;
	}
	if (!aluline_init()) {
		return FALSE;
	}
	if (!apalet_init()) {
		return FALSE;
	}
	if (!rtc_init()) {
		return FALSE;
	}
	if (!whg_init()) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	�V�X�e��
 *	�N���[���A�b�v
 */
void FASTCALL system_cleanup(void)
{
	/* ���̑��f�o�C�X */
	whg_cleanup();
	rtc_cleanup();
	apalet_cleanup();
	aluline_cleanup();
	opn_cleanup();
	tapelp_cleanup();
	kanji_cleanup();
	multipag_cleanup();
	mainetc_cleanup();
	fdc_cleanup();
	keyboard_cleanup();
	subctrl_cleanup();
	ttlpalet_cleanup();
	display_cleanup();

	/* �������ACPU */
	subcpu_cleanup();
	maincpu_cleanup();
	submem_cleanup();
	mainmem_cleanup();

	/* �X�P�W���[���A�������o�X */
	mmr_cleanup();
	schedule_cleanup();
}

/*
 *	�V�X�e��
 *	���Z�b�g
 */
void FASTCALL system_reset(void)
{
	/* �X�P�W���[���A�������o�X */
	schedule_reset();
	mmr_reset();

	/* ���̑��f�o�C�X */
	display_reset();
	ttlpalet_reset();
	subctrl_reset();
	keyboard_reset();
	fdc_reset();
	mainetc_reset();
	multipag_reset();
	kanji_reset();
	tapelp_reset();
	opn_reset();
	aluline_reset();
	apalet_reset();
	rtc_reset();
	whg_reset();

	/* �������ACPU */
	mainmem_reset();
	submem_reset();
	maincpu_reset();
	subcpu_reset();

	/* ��ʍĕ`�� */
	display_notify();
}

/*
 *	�V�X�e��
 *	�t�@�C���Z�[�u
 */
BOOL FASTCALL system_save(char *filename)
{
	int fileh;
	char *header = "XM7 VM STATE   5";
	BOOL flag;
	ASSERT(filename);

	/* �t�@�C���I�[�v�� */
	fileh = file_open(filename, OPEN_W);
	if (fileh == -1) {
		return FALSE;
	}

	/* �t���O������ */
	flag = TRUE;

	/* �w�b�_���Z�[�u */
	if (!file_write(fileh, (BYTE *)header, 16)) {
		flag = FALSE;
	}

	/* �V�X�e�����[�N */
	if (!file_word_write(fileh, (WORD)fm7_ver)) {
		return FALSE;
	}
	if (!file_word_write(fileh, (WORD)boot_mode)) {
		return FALSE;
	}

	/* ���ԂɌĂяo�� */
	if (!mainmem_save(fileh)) {
		flag = FALSE;
	}
	if (!submem_save(fileh)) {
		flag = FALSE;
	}
	if (!maincpu_save(fileh)) {
		flag = FALSE;
	}
	if (!subcpu_save(fileh)) {
		flag = FALSE;
	}
	if (!schedule_save(fileh)) {
		flag = FALSE;
	}
	if (!display_save(fileh)) {
		flag = FALSE;
	}
	if (!ttlpalet_save(fileh)) {
		flag = FALSE;
	}
	if (!subctrl_save(fileh)) {
		flag = FALSE;
	}
	if (!keyboard_save(fileh)) {
		flag = FALSE;
	}
	if (!fdc_save(fileh)) {
		flag = FALSE;
	}
	if (!mainetc_save(fileh)) {
		flag = FALSE;
	}
	if (!multipag_save(fileh)) {
		flag = FALSE;
	}
	if (!kanji_save(fileh)) {
		flag = FALSE;
	}
	if (!tapelp_save(fileh)) {
		flag = FALSE;
	}
	if (!opn_save(fileh)) {
		flag = FALSE;
	}
	if (!mmr_save(fileh)) {
		flag = FALSE;
	}
	if (!aluline_save(fileh)) {
		flag = FALSE;
	}
	if (!rtc_save(fileh)) {
		flag = FALSE;
	}
	if (!apalet_save(fileh)) {
		flag = FALSE;
	}
	if (!whg_save(fileh)) {
		flag = FALSE;
	}

	file_close(fileh);
	return flag;
}

/*
 *	�V�X�e��
 *	�t�@�C�����[�h
 */
BOOL FASTCALL system_load(char *filename)
{
	int fileh;
	int ver;
	char header[16];
	BOOL flag;
	ASSERT(filename);

	/* �t�@�C���I�[�v�� */
	fileh = file_open(filename, OPEN_R);
	if (fileh == -1) {
		return FALSE;
	}

	/* �t���O������ */
	flag = TRUE;

	/* �w�b�_�����[�h */
	if (!file_read(fileh, (BYTE *)header, 16)) {
		flag = FALSE;
	}
	else {
		if (memcmp(header, "XM7 VM STATE   ", 15) != 0) {
			flag = FALSE;
		}
	}

	/* �w�b�_�`�F�b�N */
	if (!flag) {
		file_close(fileh);
		return FALSE;
	}

	/* �t�@�C���o�[�W�����擾�A�o�[�W����2�ȏオ�Ώ� */
	ver = (int)(BYTE)(header[15]);
	ver -= 0x30;
	if (ver < 2) {
		return FALSE;
	}

	/* �V�X�e�����[�N */
	if (!file_word_read(fileh, (WORD *)&fm7_ver)) {
		return FALSE;
	}
	if (!file_word_read(fileh, (WORD *)&boot_mode)) {
		return FALSE;
	}

	/* ���ԂɌĂяo�� */
	if (!mainmem_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!submem_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!maincpu_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!subcpu_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!schedule_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!display_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!ttlpalet_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!subctrl_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!keyboard_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!fdc_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!mainetc_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!multipag_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!kanji_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!tapelp_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!opn_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!mmr_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!aluline_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!rtc_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!apalet_load(fileh, ver)) {
		flag = FALSE;
	}
	if (!whg_load(fileh, ver)) {
		flag = FALSE;
	}
	file_close(fileh);

	/* ��ʍĕ`�� */
	display_notify();

	return flag;
}

/*
 *	�t�@�C���ǂݍ���(BYTE)
 */
BOOL FASTCALL file_byte_read(int fileh, BYTE *dat)
{
	return file_read(fileh, dat, 1);
}

/*
 *	�t�@�C���ǂݍ���(WORD)
 */
BOOL FASTCALL file_word_read(int fileh, WORD *dat)
{
	BYTE tmp;

	if (!file_read(fileh, &tmp, 1)) {
		return FALSE;
	}
	*dat = tmp;

	if (!file_read(fileh, &tmp, 1)) {
		return FALSE;
	}
	*dat <<= 8;
	*dat |= tmp;

	return TRUE;
}

/*
 *	�t�@�C���ǂݍ���(DWORD)
 */
BOOL FASTCALL file_dword_read(int fileh, DWORD *dat)
{
	BYTE tmp;

	if (!file_read(fileh, &tmp, 1)) {
		return FALSE;
	}
	*dat = tmp;

	if (!file_read(fileh, &tmp, 1)) {
		return FALSE;
	}
	*dat *= 256;
	*dat |= tmp;

	if (!file_read(fileh, &tmp, 1)) {
		return FALSE;
	}
	*dat *= 256;
	*dat |= tmp;

	if (!file_read(fileh, &tmp, 1)) {
		return FALSE;
	}
	*dat *= 256;
	*dat |= tmp;

	return TRUE;
}

/*
 *	�t�@�C���ǂݍ���(BOOL)
 */
BOOL FASTCALL file_bool_read(int fileh, BOOL *dat)
{
	BYTE tmp;

	if (!file_read(fileh, &tmp, 1)) {
		return FALSE;
	}

	switch (tmp) {
		case 0:
			*dat = FALSE;
			return TRUE;
		case 0xff:
			*dat = TRUE;
			return TRUE;
	}

	return FALSE;
}

/*
 *	�t�@�C����������(BYTE)
 */
BOOL FASTCALL file_byte_write(int fileh, BYTE dat)
{
	return file_write(fileh, &dat, 1);
}

/*
 *	�t�@�C����������(WORD)
 */
BOOL FASTCALL file_word_write(int fileh, WORD dat)
{
	BYTE tmp;

	tmp = (BYTE)(dat >> 8);
	if (!file_write(fileh, &tmp, 1)) {
		return FALSE;
	}

	tmp = (BYTE)(dat & 0xff);
	if (!file_write(fileh, &tmp, 1)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	�t�@�C����������(DWORD)
 */
BOOL FASTCALL file_dword_write(int fileh, DWORD dat)
{
	BYTE tmp;

	tmp = (BYTE)(dat >> 24);
	if (!file_write(fileh, &tmp, 1)) {
		return FALSE;
	}

	tmp = (BYTE)(dat >> 16);
	if (!file_write(fileh, &tmp, 1)) {
		return FALSE;
	}

	tmp = (BYTE)(dat >> 8);
	if (!file_write(fileh, &tmp, 1)) {
		return FALSE;
	}

	tmp = (BYTE)(dat & 0xff);
	if (!file_write(fileh, &tmp, 1)) {
		return FALSE;
	}

	return TRUE;
}

/*
 *	�t�@�C����������(BOOL)
 */
BOOL FASTCALL file_bool_write(int fileh, BOOL dat)
{
	BYTE tmp;

	if (dat) {
		tmp = 0xff;
	}
	else {
		tmp = 0;
	}

	if (!file_write(fileh, &tmp, 1)) {
		return FALSE;
	}

	return TRUE;
}

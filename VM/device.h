/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �f�o�C�X�ˑ��� ]
 */

#ifndef _device_h_
#define _device_h_

/*
 *	�萔��`
 */
#define OPEN_R		1					/* �ǂݍ��݃��[�h */
#define OPEN_W		2					/* �������݃��[�h */
#define OPEN_RW		3					/* �ǂݏ������[�h */

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */

/* �`�� */
void FASTCALL vram_notify(WORD addr, BYTE dat);
										/* VRAM�������ݒʒm */
void FASTCALL ttlpalet_notify(void);
										/* �f�W�^���p���b�g�ύX�ʒm */
void FASTCALL apalet_notify(void);
										/* �A�i���O�p���b�g�ύX�ʒm */
void FASTCALL display_notify(void);
										/* ��ʖ����ʒm */
void FASTCALL digitize_notify(void);
										/* �f�B�W�^�C�Y�ʒm */
void FASTCALL vsync_notify(void);
										/* VSYNC�ʒm */

/* �����A�W���C�X�e�B�b�N */
void FASTCALL opn_notify(BYTE reg, BYTE dat);
										/* OPN�o�͒ʒm */
void FASTCALL whg_notify(BYTE reg, BYTE dat);
										/* WHG�o�͒ʒm */
BYTE FASTCALL joy_request(BYTE port);
										/* �W���C�X�e�B�b�N�v�� */

/* �t�@�C�� */
BOOL FASTCALL file_load(char *fname, BYTE *buf, int size);
										/* �t�@�C�����[�h(ROM��p) */
int FASTCALL file_open(char *fname, int mode);
										/* �t�@�C���I�[�v�� */
void FASTCALL file_close(int handle);
										/* �t�@�C���N���[�Y */
DWORD FASTCALL file_getsize(int handle);
										/* �t�@�C�������O�X�擾 */
BOOL FASTCALL file_seek(int handle, DWORD offset);
										/* �t�@�C���V�[�N */
BOOL FASTCALL file_read(int handle, BYTE *ptr, DWORD size);
										/* �t�@�C���ǂݍ��� */
BOOL FASTCALL file_write(int handle, BYTE *ptr, DWORD size);
										/* �t�@�C���������� */

/* �t�@�C���T�u(�v���b�g�t�H�[����ˑ��B���̂�system.c�ɂ���) */
BOOL FASTCALL file_byte_read(int handle, BYTE *dat);
										/* �o�C�g�ǂݍ��� */
BOOL FASTCALL file_word_read(int handle, WORD *dat);
										/* ���[�h�ǂݍ��� */
BOOL FASTCALL file_dword_read(int handle, DWORD *dat);
										/* �_�u�����[�h�ǂݍ��� */
BOOL FASTCALL file_bool_read(int handle, BOOL *dat);
										/* �u�[���ǂݍ��� */
BOOL FASTCALL file_byte_write(int handle, BYTE dat);
										/* �o�C�g�������� */
BOOL FASTCALL file_word_write(int handle, WORD dat);
										/* ���[�h�������� */
BOOL FASTCALL file_dword_write(int handle, DWORD dat);
										/* �_�u�����[�h�������� */
BOOL FASTCALL file_bool_write(int handle, BOOL dat);
										/* �u�[���������� */
#ifdef __cplusplus
}
#endif

#endif	/* _device_h_ */

/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �f�B�X�v���C ]
 */

#ifndef _display_h_
#define _display_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
BOOL FASTCALL display_init(void);
										/* ������ */
void FASTCALL display_cleanup(void);
										/* �N���[���A�b�v */
void FASTCALL display_reset(void);
										/* ���Z�b�g */
BOOL FASTCALL display_readb(WORD addr, BYTE *dat);
										/* �������ǂݏo�� */
BOOL FASTCALL display_writeb(WORD addr, BYTE dat);
										/* �������������� */
BOOL FASTCALL display_save(int fileh);
										/* �Z�[�u */
BOOL FASTCALL display_load(int fileh, int ver);
										/* ���[�h */

/*
 *	��v���[�N
 */
extern BOOL crt_flag;
										/* CRT�\���t���O */
extern BOOL vrama_flag;
										/* VRAM�A�N�Z�X�t���O */
extern WORD vram_offset[2];
										/* VRAM�I�t�Z�b�g���W�X�^ */
extern BOOL vram_offset_flag;
										/* �g��VRAM�I�t�Z�b�g�t���O */
extern BOOL subnmi_flag;
										/* �T�uNMI�C�l�[�u���t���O */
extern BOOL vsync_flag;
										/* VSYNC�t���O */
extern BOOL blank_flag;
										/* �u�����L���O�t���O */
extern BYTE vram_active;
										/* �A�N�e�B�u�y�[�W */
extern BYTE *vram_aptr;
										/* VRAM�A�N�e�B�u�|�C���^ */
extern BYTE vram_display;
										/* �\���y�[�W */
extern BYTE *vram_dptr;
										/* VRAM�\���|�C���^ */
#ifdef __cplusplus
}
#endif

#endif	/* _display_h_ */

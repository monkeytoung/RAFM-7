/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �J�Z�b�g�e�[�v���v�����^ ]
 */

#ifndef _tapelp_h_
#define _tapelp_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
BOOL FASTCALL tapelp_init(void);
										/* ������ */
void FASTCALL tapelp_cleanup(void);
										/* �N���[���A�b�v */
void FASTCALL tapelp_reset(void);
										/* ���Z�b�g */
BOOL FASTCALL tapelp_readb(WORD addr, BYTE *dat);
										/* �������ǂݏo�� */
BOOL FASTCALL tapelp_writeb(WORD addr, BYTE dat);
										/* �������������� */
BOOL FASTCALL tapelp_save(int fileh);
										/* �Z�[�u */
BOOL FASTCALL tapelp_load(int fileh, int ver);
										/* ���[�h */
void FASTCALL lp_setfile(char *fname);
										/* �v�����^�t�@�C���ݒ� */
void FASTCALL tape_setfile(char *fname);
										/* �e�[�v�t�@�C���ݒ� */
void FASTCALL tape_setrec(BOOL flag);
										/* �e�[�v�^���t���O�ݒ� */
void FASTCALL tape_rew(void);
										/* �e�[�v�����߂� */
void FASTCALL tape_ff(void);
										/* �e�[�v������ */

/*
 *	��v���[�N
 */
extern BOOL tape_in;
										/* �e�[�v ���̓f�[�^ */
extern BOOL tape_out;
										/* �e�[�v �o�̓f�[�^ */
extern BOOL tape_motor;
										/* �e�[�v ���[�^ */
extern BOOL tape_rec;
										/* �e�[�v �^���� */
extern BOOL tape_writep;
										/* �e�[�v �^���s�� */
extern WORD tape_count;
										/* �e�[�v �J�E���^ */
extern BYTE tape_subcnt;
										/* �e�[�v �T�u�J�E���g */
extern int tape_fileh;
										/* �e�[�v �t�@�C���n���h�� */
extern DWORD tape_offset;
										/* �e�[�v�@�t�@�C���I�t�Z�b�g */
extern char tape_fname[128+1];
										/* �e�[�v �t�@�C���l�[�� */

extern BYTE lp_data;
										/* �v�����^ �o�̓f�[�^ */
extern BOOL lp_busy;
										/* �v�����^ BUSY�t���O */
extern BOOL lp_error;
										/* �v�����^ �G���[�t���O */
extern BOOL lp_pe;
										/* �v�����^ PE�t���O */
extern BOOL lp_ackng;
										/* �v�����^ ACK�t���O */
extern BOOL lp_online;
										/* �v�����^ �I�����C�� */
extern BOOL lp_strobe;
										/* �v�����^ �X�g���[�u */
extern int lp_fileh;
										/* �v�����^ �t�@�C���n���h�� */
#ifdef __cplusplus
}
#endif

#endif	/* _tapelp_h_ */

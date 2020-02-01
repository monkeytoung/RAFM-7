/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ OPN(YM2203) ]
 */

#ifndef _opn_h_
#define _opn_h_

/*
 *	�萔��`
 */
#define OPN_INACTIVE		0x00			/* �C���A�N�e�B�u�R�}���h */
#define OPN_READDAT			0x01			/* ���[�h�f�[�^�R�}���h */
#define OPN_WRITEDAT		0x02			/* ���C�g�f�[�^�R�}���h */
#define OPN_ADDRESS			0x03			/* ���b�`�A�h���X�R�}���h */
#define OPN_READSTAT		0x04			/* ���[�h�X�e�[�^�X�R�}���h */
#define OPN_JOYSTICK		0x09			/* �W���C�X�e�B�b�N�R�}���h */

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
BOOL FASTCALL opn_init(void);
										/* ������ */
void FASTCALL opn_cleanup(void);
										/* �N���[���A�b�v */
void FASTCALL opn_reset(void);
										/* ���Z�b�g */
BOOL FASTCALL opn_readb(WORD addr, BYTE *dat);
										/* �������ǂݏo�� */
BOOL FASTCALL opn_writeb(WORD addr, BYTE dat);
										/* �������������� */
BOOL FASTCALL opn_save(int fileh);
										/* �Z�[�u */
BOOL FASTCALL opn_load(int fileh, int ver);
										/* ���[�h */

/*
 *	��v���[�N
 */
extern BYTE opn_reg[256];
										/* OPN���W�X�^ */
extern BOOL opn_key[4];
										/* OPN�L�[�I���t���O */
extern BOOL opn_timera;
										/* �^�C�}�[A����t���O */
extern BOOL opn_timerb;
										/* �^�C�}�[B����t���O */
extern DWORD opn_timera_tick;
										/* �^�C�}�[A�Ԋu(us) */
extern DWORD opn_timerb_tick;
										/* �^�C�}�[B�Ԋu(us) */
extern BYTE opn_scale;
										/* �v���X�P�[�� */
#ifdef __cplusplus
}
#endif

#endif	/* _opn_h_ */

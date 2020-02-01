/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ WHG(YM2203) ]
 */

#ifndef _whg_h_
#define _whg_h_

/*
 *	�萔��`
 */
#define WHG_INACTIVE		0x00			/* �C���A�N�e�B�u�R�}���h */
#define WHG_READDAT			0x01			/* ���[�h�f�[�^�R�}���h */
#define WHG_WRITEDAT		0x02			/* ���C�g�f�[�^�R�}���h */
#define WHG_ADDRESS			0x03			/* ���b�`�A�h���X�R�}���h */
#define WHG_READSTAT		0x04			/* ���[�h�X�e�[�^�X�R�}���h */
#define WHG_JOYSTICK		0x09			/* �W���C�X�e�B�b�N�R�}���h */

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
BOOL FASTCALL whg_init(void);
										/* ������ */
void FASTCALL whg_cleanup(void);
										/* �N���[���A�b�v */
void FASTCALL whg_reset(void);
										/* ���Z�b�g */
BOOL FASTCALL whg_readb(WORD addr, BYTE *dat);
										/* �������ǂݏo�� */
BOOL FASTCALL whg_writeb(WORD addr, BYTE dat);
										/* �������������� */
BOOL FASTCALL whg_save(int fileh);
										/* �Z�[�u */
BOOL FASTCALL whg_load(int fileh, int ver);
										/* ���[�h */

/*
 *	��v���[�N
 */
extern BOOL whg_enable;
										/* WHG�L���E�����t���O */
extern BOOL whg_use;
										/* WHG�g�p�t���O */
extern BYTE whg_reg[256];
										/* WHG���W�X�^ */
extern BOOL whg_key[4];
										/* WHG�L�[�I���t���O */
extern BOOL whg_timera;
										/* �^�C�}�[A����t���O */
extern BOOL whg_timerb;
										/* �^�C�}�[B����t���O */
extern DWORD whg_timera_tick;
										/* �^�C�}�[A�Ԋu(us) */
extern DWORD whg_timerb_tick;
										/* �^�C�}�[B�Ԋu(us) */
extern BYTE whg_scale;
										/* �v���X�P�[�� */
#ifdef __cplusplus
}
#endif

#endif	/* _whg_h_ */

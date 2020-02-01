/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �T�uCPU�R���g���[�� ]
 */

#ifndef _subctrl_h_
#define _subctrl_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
BOOL FASTCALL subctrl_init(void);
										/* ������ */
void FASTCALL subctrl_cleanup(void);
										/* �N���[���A�b�v */
void FASTCALL subctrl_reset(void);
										/* ���Z�b�g */
BOOL FASTCALL subctrl_readb(WORD addr, BYTE *dat);
										/* �������ǂݏo�� */
BOOL FASTCALL subctrl_writeb(WORD addr, BYTE dat);
										/* �������������� */
BOOL FASTCALL subctrl_save(int fileh);
										/* �Z�[�u */
BOOL FASTCALL subctrl_load(int fileh, int ver);
										/* ���[�h */

/*
 *	��v���[�N
 */
extern BOOL subhalt_flag;
										/* �T�uHALT�t���O */
extern BOOL subbusy_flag;
										/* �T�uBUSY�t���O */
extern BOOL subcancel_flag;
										/* �T�u�L�����Z���t���O */
extern BOOL subattn_flag;
										/* �T�u�A�e���V�����t���O */
extern BYTE shared_ram[0x80];
										/* ���LRAM */
extern BOOL subreset_flag;
										/* �T�u�ċN���t���O */
extern BOOL mode320;
										/* 320x200���[�h */
#ifdef __cplusplus
}
#endif

#endif	/* _subctrl_h_ */

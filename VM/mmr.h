/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ MMR ]
 */

#ifndef _mmr_h_
#define _mmr_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
BOOL FASTCALL mmr_init(void);
										/* ������ */
void FASTCALL mmr_cleanup(void);
										/* �N���[���A�b�v */
void FASTCALL mmr_reset(void);
										/* ���Z�b�g */
BOOL FASTCALL mmr_readb(WORD addr, BYTE *dat);
										/* �������ǂݏo�� */
BOOL FASTCALL mmr_writeb(WORD addr, BYTE dat);
										/* �������������� */
BOOL FASTCALL mmr_extrb(WORD *addr, BYTE *dat);
										/* MMR�o�R�ǂݏo�� */
BOOL FASTCALL mmr_extbnio(WORD *addr, BYTE *dat);
										/* MMR�o�R�ǂݏo�� */
BOOL FASTCALL mmr_extwb(WORD *addr, BYTE dat);
										/* MMR�o�R�������� */
BOOL FASTCALL mmr_save(int fileh);
										/* �Z�[�u */
BOOL FASTCALL mmr_load(int fileh, int ver);
										/* ���[�h */

/*
 *	��v���[�N
 */
extern BOOL mmr_flag;
										/* MMR�L���t���O */
extern BYTE mmr_seg;
										/* MMR�Z�O�����g */
extern BYTE mmr_reg[0x40];
										/* MMR���W�X�^ */
extern BOOL twr_flag;
										/* TWR�L���t���O */
extern BYTE twr_reg;
										/* TWR���W�X�^ */
#ifdef __cplusplus
}
#endif

#endif	/* _mmr_h_ */

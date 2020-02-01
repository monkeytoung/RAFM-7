/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ ����ROM ]
 */

#ifndef _kanji_h_
#define _kanji_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
BOOL FASTCALL kanji_init(void);
										/* ������ */
void FASTCALL kanji_cleanup(void);
										/* �N���[���A�b�v */
void FASTCALL kanji_reset(void);
										/* ���Z�b�g */
BOOL FASTCALL kanji_readb(WORD addr, BYTE *dat);
										/* �������ǂݏo�� */
BOOL FASTCALL kanji_writeb(WORD addr, BYTE dat);
										/* �������������� */
BOOL FASTCALL kanji_save(int fileh);
										/* �Z�[�u */
BOOL FASTCALL kanji_load(int fileh, int ver);
										/* �Z�[�u */

/*
 *	��v���[�N
 */
extern WORD kanji_addr;
										/* ��P���� �A�h���X */
extern BYTE *kanji_rom;
										/* ��P����ROM */
#ifdef __cplusplus
}
#endif

#endif	/* _kanji_h_ */

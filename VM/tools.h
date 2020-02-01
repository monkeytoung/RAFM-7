/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �⏕�c�[�� ]
 */

#ifndef _tools_h_
#define _tools_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
BOOL make_new_d77(char *fname, char *name);
										/* �u�����N�f�B�X�N�쐬 */
BOOL make_new_t77(char *fname);
										/* �u�����N�e�[�v�쐬 */
BOOL conv_vfd_to_d77(char *src, char *dst, char *name);
										/* VFD��D77�ϊ� */
BOOL conv_2d_to_d77(char *src, char *dst, char *name);
										/* 2D��D77�ϊ� */
BOOL conv_vtp_to_t77(char *src, char *dst);
										/* VTP��T77�ϊ� */
BOOL capture_to_bmp(char *fname);
										/* ��ʃL���v�`��(BMP) */
#ifdef __cplusplus
}
#endif

#endif	/* _tools_h_ */

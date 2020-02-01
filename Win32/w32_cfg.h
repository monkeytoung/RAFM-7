/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �R���t�B�M�����[�V���� ]
 */

#ifdef _WIN32

#ifndef _w32_cfg_h_
#define _w32_cfg_h_

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	��v�G���g��
 */
void FASTCALL LoadCfg(void);
										/* �ݒ胍�[�h */
void FASTCALL SaveCfg(void);
										/* �ݒ�Z�[�u */
void FASTCALL ApplyCfg(void);
										/* �ݒ�K�p */
void FASTCALL OnConfig(HWND hWnd);
										/* �ݒ�_�C�A���O */

/*
 *	��v���[�N
 */
#ifdef __cplusplus
}
#endif

#endif	/* _w32_cfg_h_ */
#endif	/* _WIN32 */

/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �C�x���gID ]
 */

#ifndef _event_h_
#define _event_h_

/*
 *	�C�x���g��`
 */
#define EVENT_MAINTIMER			0		/* ���C��CPU 2.03ms�^�C�} */
#define EVENT_SUBTIMER			1		/* �T�uCPU 20ms�^�C�} */
#define EVENT_OPN_A				2		/* OPN �^�C�}A */
#define EVENT_OPN_B				3		/* OPN �^�C�}B */
#define EVENT_KEYBOARD			4		/* �L�[�{�[�h ���s�[�g�^�C�} */
#define EVENT_BEEP				5		/* BEEP�� �P���^�C�} */
#define EVENT_VSYNC				6		/* VSYNC */
#define EVENT_BLANK				7		/* BLANK */
#define EVENT_LINE				8		/* �������LSI */
#define EVENT_RTC				9		/* ���v 1s */
#define EVENT_WHG_A				10		/* WHG �^�C�}A */
#define EVENT_WHG_B				11		/* WHG �^�C�}B */

#endif	/* _event_h_ */

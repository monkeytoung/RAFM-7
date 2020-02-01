/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 ＰＩ．(ytanaka@ipc-tokai.or.jp)
 *	[ イベントID ]
 */

#ifndef _event_h_
#define _event_h_

/*
 *	イベント定義
 */
#define EVENT_MAINTIMER			0		/* メインCPU 2.03msタイマ */
#define EVENT_SUBTIMER			1		/* サブCPU 20msタイマ */
#define EVENT_OPN_A				2		/* OPN タイマA */
#define EVENT_OPN_B				3		/* OPN タイマB */
#define EVENT_KEYBOARD			4		/* キーボード リピートタイマ */
#define EVENT_BEEP				5		/* BEEP音 単音タイマ */
#define EVENT_VSYNC				6		/* VSYNC */
#define EVENT_BLANK				7		/* BLANK */
#define EVENT_LINE				8		/* 直線補間LSI */
#define EVENT_RTC				9		/* 時計 1s */
#define EVENT_WHG_A				10		/* WHG タイマA */
#define EVENT_WHG_B				11		/* WHG タイマB */

#endif	/* _event_h_ */

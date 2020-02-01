// ---------------------------------------------------------------------------
//	FM sound generator common timer module
//	Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	$Id: fmtimer.h,v 1.1 2000/09/08 13:45:56 cisc Exp $

#ifndef FM_TIMER_H
#define FM_TIMER_H

#include "cisc.h"

// ---------------------------------------------------------------------------

namespace FM
{
	class Timer
	{
	public:
		void FASTCALL	Reset();
		bool FASTCALL	Count(int32 us);
		int32 FASTCALL	GetNextEvent();

	protected:
		virtual void FASTCALL SetStatus(uint bit) = 0;
		virtual void FASTCALL ResetStatus(uint bit) = 0;

		void FASTCALL	SetTimerBase(uint clock);
		void FASTCALL	SetTimerA(uint addr, uint data);
		void FASTCALL	SetTimerB(uint data);
		void FASTCALL	SetTimerControl(uint data);

		uint8	status;
		uint8	regtc;

	private:
		virtual void FASTCALL TimerA() {}
		uint8	regta[2];

		int32	timera, timera_count;
		int32	timerb, timerb_count;
		int32	timer_step;
	};

// ---------------------------------------------------------------------------
//	èâä˙âª
//
inline void FASTCALL Timer::Reset()
{
	timera_count = 0;
	timerb_count = 0;
}

} // namespace FM

#endif // FM_TIMER_H

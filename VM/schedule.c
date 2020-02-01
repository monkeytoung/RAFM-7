/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ �X�P�W���[�� ]
 */

#include <string.h>
#include <assert.h>
#include "xm7.h"
#include "subctrl.h"
#include "display.h"
#include "mmr.h"
#include "device.h"

/*
 *	�O���[�o�� ���[�N
 */
BOOL run_flag;							/* ���쒆�t���O */
BOOL stopreq_flag;						/* ��~�v���t���O */
breakp_t breakp[BREAKP_MAXNUM];			/* �u���[�N�|�C���g �f�[�^ */
event_t event[EVENT_MAXNUM];			/* �C�x���g �f�[�^ */
DWORD main_speed;						/* ���C��CPU�X�s�[�h */
DWORD mmr_speed;						/* ���C��(MMR)�X�s�[�h */
BOOL cycle_steel;						/* �T�C�N���X�`�[���t���O */

/*
 *	�v���g�^�C�v�錾
 */
static BOOL FASTCALL schedule_runbreak(void);

/*
 *	�X�P�W���[��
 *	������
 */
BOOL FASTCALL schedule_init(void)
{
	run_flag = FALSE;
	stopreq_flag = FALSE;
	memset(breakp, 0, sizeof(breakp));
	memset(event, 0, sizeof(event));

	/* CPU���x�����ݒ� */
	main_speed = 17520;
	mmr_speed = 14660;
	cycle_steel = TRUE;

	return TRUE;
}

/*
 *	�X�P�W���[��
 *	�N���[���A�b�v
 */
void FASTCALL schedule_cleanup(void)
{
}

/*
 *	�X�P�W���[��
 *	���Z�b�g
 */
void FASTCALL schedule_reset(void)
{
	/* �S�ẴX�P�W���[�����N���A */
	memset(event, 0, sizeof(event));

	/* �u���[�N�|�C���g�Đݒ� */
	schedule_runbreak();
}

/*-[ �C�x���g ]-------------------------------------------------------------*/

/*
 *	�X�P�W���[��
 *	�C�x���g�ݒ�
 */
BOOL FASTCALL schedule_setevent(int id, DWORD microsec, BOOL FASTCALL (*func)(void))
{
	DWORD exec;

	ASSERT((id >= 0) && (id < EVENT_MAXNUM));
	ASSERT(func);

	if ((id < 0) || (id >= EVENT_MAXNUM)) {
		return FALSE;
	}
	if (microsec == 0) {
		event[id].flag = EVENT_NOTUSE;
		return FALSE;
	}

	/* �o�^ */
	event[id].current = microsec;
	event[id].reload = microsec;
	event[id].callback = func;
	event[id].flag = EVENT_ENABLED;

	/* ���s���Ȃ�A���Ԃ𑫂��Ă����K�v������(��ň�������) */
	if (run_flag) {
		if (mmr_flag || twr_flag) {
			exec = (DWORD)maincpu.total;
			exec *= 10000;
			exec /= mmr_speed;
			event[id].current += exec;
		}
		else {
			exec = (DWORD)maincpu.total;
			exec *= 10000;
			exec /= main_speed;
			event[id].current += exec;
		}
	}

	return TRUE;
}

/*
 *	�X�P�W���[��
 *	�C�x���g�폜
 */
BOOL FASTCALL schedule_delevent(int id)
{
	ASSERT((id >= 0) && (id < EVENT_MAXNUM));

	if ((id < 0) || (id >= EVENT_MAXNUM)) {
		return FALSE;
	}

	/* ���g�p�� */
	event[id].flag = EVENT_NOTUSE;

	return TRUE;
}

/*
 *	�X�P�W���[��
 *	�C�x���g�n���h���ݒ�
 */
void FASTCALL schedule_handle(int id, BOOL FASTCALL (*func)(void))
{
	ASSERT((id >= 0) && (id < EVENT_MAXNUM));
	ASSERT(func);

	/* �R�[���o�b�N�֐���o�^����̂݁B����ȊO�͐G��Ȃ� */
	event[id].callback = func;
}

/*
 *	�X�P�W���[��
 *	�ŒZ���s���Ԓ���
 */
static DWORD FASTCALL schedule_chkevent(DWORD microsec)
{
	DWORD exectime;
	int i;

	/* �����ݒ� */
	exectime = microsec;

	/* �C�x���g������Ē��� */
	for (i=0; i<EVENT_MAXNUM; i++) {
		if (event[i].flag == EVENT_NOTUSE) {
			continue;
		}

		ASSERT(event[i].current > 0);
		ASSERT(event[i].reload > 0);

		if (event[i].current < exectime) {
			exectime = event[i].current;
		}
	}

	return exectime;
}

/*
 *	�X�P�W���[��
 *	�i�s����
 */
static void FASTCALL schedule_doevent(DWORD microsec)
{
	int i;

	for (i=0; i<EVENT_MAXNUM; i++) {
		if (event[i].flag == EVENT_NOTUSE) {
			continue;
		}

		ASSERT(event[i].current > 0);
		ASSERT(event[i].reload > 0);

		/* ���s���Ԃ����� */
		if (event[i].current < microsec) {
			event[i].current = 0;
		}
		else {
			event[i].current -= microsec;
		}

		/* �J�E���^��0�Ȃ� */
		if (event[i].current == 0) {
			/* ���Ԃ�ENABLE,DISABLE�ɂ�����炸�����[�h */
			event[i].current = event[i].reload;
			/* �R�[���o�b�N���s */
			if (event[i].flag == EVENT_ENABLED) {
				if (!event[i].callback()) {
					event[i].flag = EVENT_DISABLED;
				}
			}
		}
	}
}

/*-[ �u���[�N�|�C���g ]-----------------------------------------------------*/

/*
 *	�X�P�W���[��
 *	�u���[�N�|�C���g�Z�b�g(���łɃZ�b�g���Ă���Ώ���)
 */
BOOL FASTCALL schedule_setbreak(int cpu, WORD addr)
{
	int i;

	/* �܂��A�S�Ẵu���[�N�|�C���g���������A�����邩 */
	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (breakp[i].flag != BREAKP_NOTUSE) {
			if ((breakp[i].cpu == cpu) && (breakp[i].addr == addr)) {
				break;
			}
		}
	}
	/* ������΁A�폜 */
	if (i != BREAKP_MAXNUM) {
		breakp[i].flag = BREAKP_NOTUSE;
		return TRUE;
	}	

	/* �󂫂𒲍� */
	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (breakp[i].flag == BREAKP_NOTUSE) {
			break;
		}
	}
	/* ���ׂĖ��܂��Ă��邩 */
	if (i == BREAKP_MAXNUM) {
		return FALSE;
	}

	/* �Z�b�g */
	breakp[i].flag = BREAKP_ENABLED;
	breakp[i].cpu = cpu;
	breakp[i].addr = addr;

	return TRUE;
}

/*
 *	�X�P�W���[��
 *	Stop�t���O�N���A
 */
static BOOL FASTCALL schedule_runbreak(void)
{
	int i;
	BOOL flag;

	/* ��~���Ȃ�A�L���� */
	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (breakp[i].flag == BREAKP_STOPPED) {
			breakp[i].flag = BREAKP_ENABLED;
		}
	}

	/* �P�ł��L��������� */
	flag = FALSE;
	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (breakp[i].flag == BREAKP_ENABLED) {
			flag = TRUE;
		}
	}

	return flag;
}

/*
 *	�X�P�W���[��
 *	�u���[�N�`�F�b�N
 */
static BOOL FASTCALL schedule_chkbreak(void)
{
	int i;

	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (breakp[i].flag == BREAKP_ENABLED) {
			if (breakp[i].cpu == MAINCPU) {
				if (breakp[i].addr == maincpu.pc) {
					return TRUE;
				}
			}
			else {
				if (breakp[i].addr == subcpu.pc) {
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

/*
 *	�X�P�W���[��
 *	Stop�t���O�ݒ�
 */
static BOOL FASTCALL schedule_stopbreak(void)
{
	int i;
	int ret;

	/* �t���O�𗎂Ƃ��Ă��� */
	ret = FALSE;

	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (breakp[i].flag == BREAKP_ENABLED) {
			if (breakp[i].cpu == MAINCPU) {
				if (breakp[i].addr == maincpu.pc) {
					breakp[i].flag = BREAKP_STOPPED;
					ret = TRUE;
				}
			}
			else {
				if (breakp[i].addr == subcpu.pc) {
					breakp[i].flag = BREAKP_STOPPED;
					ret = TRUE;
				}
			}
		}
	}

	return ret;
}

/*-[ ���s�� ]---------------------------------------------------------------*/

/*
 *	�g���[�X
 */
void FASTCALL schedule_trace(void)
{
	schedule_runbreak();

	/* �P���ߎ��s */
	maincpu_execline();
	if (!subhalt_flag && (cycle_steel || !vrama_flag || blank_flag)) {
		subcpu_execline();
	}

	schedule_stopbreak();
}

/*
 *	���s
 */
DWORD FASTCALL schedule_exec(DWORD microsec)
{
	DWORD exec;
	DWORD count;
	WORD main;

	ASSERT(run_flag);
	ASSERT(!stopreq_flag);

	/* �ŒZ�̎��s���Ԃ𓾂� */
	exec = schedule_chkevent(microsec);

	/* CPU���ԂɊ��Z */
	if (mmr_flag || twr_flag) {
		count = mmr_speed;
		count *= exec;
		count /= 10000;
	}
	else {
		count = main_speed;
		count *= exec;
		count /= 10000;
	}
	main = (WORD)count;

	/* �J�E���^�N���A */
	maincpu.total = 0;
	subcpu.total = 0;

	if (cycle_steel) {
		if (schedule_runbreak()) {
			/* �u���[�N�|�C���g���� */
			schedule_stopbreak();

			/* ���s */
			while (maincpu.total < main) {
				if (schedule_chkbreak()) {
					stopreq_flag = TRUE;
				}
				if (stopreq_flag) {
					break;
				}

				/* ���C��CPU���s */
				maincpu_exec();

				/* �T�uCPU���s */
				if (!subhalt_flag) {
					subcpu_exec();
				}
			}

			/* �u���[�N�����ꍇ�̏��� */
			schedule_stopbreak();
			if (stopreq_flag == TRUE) {
				/* exec�����Ԃ�i�߂�(�����ĕ␳���Ȃ�) */
				run_flag = FALSE;
			}

			/* �C�x���g���� */
			maincpu.total = 0;
			subcpu.total = 0;
			schedule_doevent(exec);
		}
		else {
			/* �u���[�N�|�C���g�Ȃ� */
			/* ���s */
			while (maincpu.total < main) {
				/* ���C��CPU���s */
				maincpu_exec();

				/* �T�uCPU���s */
				if (!subhalt_flag) {
					subcpu_exec();
				}
			}

			/* �C�x���g���� */
			maincpu.total = 0;
			subcpu.total = 0;
			schedule_doevent(exec);
		}
	}
	else {
		if (schedule_runbreak()) {
			/* �u���[�N�|�C���g���� */
			schedule_stopbreak();

			/* ���s */
			while (maincpu.total < main) {
				if (schedule_chkbreak()) {
					stopreq_flag = TRUE;
				}
				if (stopreq_flag) {
					break;
				}

				/* ���C��CPU���s */
				maincpu_exec();

				/* �T�uCPU���s */
				if (!subhalt_flag && (!vrama_flag || blank_flag)) {
					subcpu_exec();
				}
			}

			/* �u���[�N�����ꍇ�̏��� */
			schedule_stopbreak();
			if (stopreq_flag == TRUE) {
				/* exec�����Ԃ�i�߂�(�����ĕ␳���Ȃ�) */
				run_flag = FALSE;
			}

			/* �C�x���g���� */
			maincpu.total = 0;
			subcpu.total = 0;
			schedule_doevent(exec);
		}
		else {
			/* �u���[�N�|�C���g�Ȃ� */
			/* ���s */
			while (maincpu.total < main) {
				/* ���C��CPU���s */
				maincpu_exec();

				/* �T�uCPU���s */
				if (!subhalt_flag && (!vrama_flag || blank_flag)) {
					subcpu_exec();
				}
			}

			/* �C�x���g���� */
			maincpu.total = 0;
			subcpu.total = 0;
			schedule_doevent(exec);
		}
	}

	return exec;
}

/*-[ �t�@�C��I/O ]----------------------------------------------------------*/

/*
 *	�X�P�W���[��
 *	�Z�[�u
 */
BOOL FASTCALL schedule_save(int fileh)
{
	int i;

	if (!file_bool_write(fileh, run_flag)) {
		return FALSE;
	}

	if (!file_bool_write(fileh, stopreq_flag)) {
		return FALSE;
	}

	/* �u���[�N�|�C���g */
	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (!file_byte_write(fileh, (BYTE)breakp[i].flag)) {
			return FALSE;
		}
		if (!file_byte_write(fileh, (BYTE)breakp[i].cpu)) {
			return FALSE;
		}
		if (!file_word_write(fileh, breakp[i].addr)) {
			return FALSE;
		}
	}

	/* �C�x���g */
	for (i=0; i<EVENT_MAXNUM; i++) {
		/* �R�[���o�b�N�ȊO��ۑ� */
		if (!file_byte_write(fileh, (BYTE)event[i].flag)) {
			return FALSE;
		}
		if (!file_dword_write(fileh, event[i].current)) {
			return FALSE;
		}
		if (!file_dword_write(fileh, event[i].reload)) {
			return FALSE;
		}
	}

	return TRUE;
}

/*
 *	�X�P�W���[��
 *	���[�h
 */
BOOL FASTCALL schedule_load(int fileh, int ver)
{
	int i;
	BYTE tmp;

	/* �o�[�W�����`�F�b�N */
	if (ver < 2) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &run_flag)) {
		return FALSE;
	}

	if (!file_bool_read(fileh, &stopreq_flag)) {
		return FALSE;
	}

	/* �u���[�N�|�C���g */
	for (i=0; i<BREAKP_MAXNUM; i++) {
		if (!file_byte_read(fileh, &tmp)) {
			return FALSE;
		}
		breakp[i].flag = (int)tmp;
		if (!file_byte_read(fileh, &tmp)) {
			return FALSE;
		}
		breakp[i].cpu = (int)tmp;
		if (!file_word_read(fileh, &breakp[i].addr)) {
			return FALSE;
		}
	}

	/* �C�x���g */
	for (i=0; i<EVENT_MAXNUM; i++) {
		/* �R�[���o�b�N�ȊO��ݒ� */
		if (!file_byte_read(fileh, &tmp)) {
			return FALSE;
		}
		event[i].flag = (int)tmp;
		if (!file_dword_read(fileh, &event[i].current)) {
			return FALSE;
		}
		if (!file_dword_read(fileh, &event[i].reload)) {
			return FALSE;
		}
	}

	return TRUE;
}

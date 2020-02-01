/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API ���j���[�R�}���h ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <stdlib.h>
#include <assert.h>
#include <tchar.h>
#include "xm7.h"
#include "fdc.h"
#include "tapelp.h"
#include "tools.h"
#include "w32.h"
#include "w32_bar.h"
#include "w32_draw.h"
#include "w32_snd.h"
#include "w32_sch.h"
#include "w32_sub.h"
#include "w32_cfg.h"
#include "w32_res.h"

/*
 *	�X�^�e�B�b�N ���[�N
 */
static char InitialDir[_MAX_DRIVE + _MAX_PATH];
static char StatePath[_MAX_PATH];
static char DiskTitle[16 + 1];

/*
 *	�v���g�^�C�v�錾
 */
static void FASTCALL OnRefresh(HWND hWnd);

/*-[ �^�C�g�����̓_�C�A���O ]------------------------------------------------*/

/*
 *	�^�C�g�����̓_�C�A���O
 *	�_�C�A���O������
 */
static BOOL FASTCALL TitleDlgInit(HWND hDlg)
{
	HWND hWnd;
	RECT prect;
	RECT drect;

	ASSERT(hDlg);

	/* �e�E�C���h�E�̒����ɐݒ� */
	hWnd = GetParent(hDlg);
	GetWindowRect(hWnd, &prect);
	GetWindowRect(hDlg, &drect);
	drect.right -= drect.left;
	drect.bottom -= drect.top;
	drect.left = (prect.right - prect.left) / 2 + prect.left;
	drect.left -= (drect.right / 2);
	drect.top = (prect.bottom - prect.top) / 2 + prect.top;
	drect.top -= (drect.bottom / 2);
	MoveWindow(hDlg, drect.left, drect.top, drect.right, drect.bottom, FALSE);

	/* �G�f�B�b�g�e�L�X�g���� */
	hWnd = GetDlgItem(hDlg, IDC_TITLEEDIT);
	ASSERT(hWnd);
	strcpy(DiskTitle, "Default");
	SetWindowText(hWnd, DiskTitle);

	return TRUE;
}

/*
 *	�^�C�g�����̓_�C�A���O
 *	�_�C�A���OOK
 */
static void FASTCALL TitleDlgOK(HWND hDlg)
{
	HWND hWnd;
	char string[128];

	ASSERT(hDlg);

	/* �G�f�B�b�g�e�L�X�g���� */
	hWnd = GetDlgItem(hDlg, IDC_TITLEEDIT);
	ASSERT(hWnd);

	/* ��������擾�A�R�s�[ */
	GetWindowText(hWnd, string, sizeof(string) - 1);
	memset(DiskTitle, 0, sizeof(DiskTitle));
	string[16] = '\0';
	strcpy(DiskTitle, string);
}

/*
 *	�^�C�g�����̓_�C�A���O
 *	�_�C�A���O�v���V�[�W��
 */
static BOOL CALLBACK TitleDlgProc(HWND hDlg, UINT iMsg,
									WPARAM wParam, LPARAM lParam)
{
	switch (iMsg) {
		/* �_�C�A���O������ */
		case WM_INITDIALOG:
			return TitleDlgInit(hDlg);

		/* �R�}���h���� */
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				/* OK */
				case IDOK:
					TitleDlgOK(hDlg);
					EndDialog(hDlg, IDOK);
					return TRUE;

				/* �L�����Z�� */
				case IDCANCEL:
					EndDialog(hDlg, IDCANCEL);
					return TRUE;
			}
			break;
	}

	/* ����ȊO�́AFALSE */
	return FALSE;
}

/*-[ �ėp�T�u ]-------------------------------------------------------------*/

/*
 *	�t�@�C���I���R�����_�C�A���O
 */
static BOOL FASTCALL FileSelectSub(BOOL bOpen, UINT uFilterID, char *path, char *defext)
{
	OPENFILENAME ofn;
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME + _MAX_EXT];
	char filter[128];
	int i, j;

	ASSERT((bOpen == TRUE) || (bOpen == FALSE));
	ASSERT(uFilterID > 0);
	ASSERT(path);

	/* �f�[�^�쐬 */
	memset(&ofn, 0, sizeof(ofn));
	memset(path, 0, _MAX_PATH);
	memset(fname, 0, sizeof(fname));
	ofn.lStructSize = 76;	// sizeof(ofn)��V5�g�����܂�
	ofn.hwndOwner = hMainWnd;

	LoadString(hAppInstance, uFilterID, filter, sizeof(filter));
	j = strlen(filter);
	for (i=0; i<j; i++) {
		if (filter[i] == '|') {
			filter[i] = '\0';
		}
	}

	ofn.lpstrFilter = filter;
	ofn.lpstrFile = path;
	ofn.nMaxFile = _MAX_PATH;
	ofn.lpstrFileTitle = fname;
	ofn.nMaxFileTitle = sizeof(fname);
	ofn.lpstrDefExt = defext;
	ofn.lpstrInitialDir = InitialDir;

	/* �R�����_�C�A���O���s */
	if (bOpen) {
		ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
		if (!GetOpenFileName(&ofn)) {
			return FALSE;
		}
	}
	else {
		ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
		if (!GetSaveFileName(&ofn)) {
			return FALSE;
		}
	}

	/* �f�B���N�g����ۑ� */
	_splitpath(path, InitialDir, dir, NULL, NULL);
	strcat(InitialDir, dir);

	return TRUE;
}

/*
 *	���j���[Enable
 */
static void FASTCALL EnableMenuSub(HMENU hMenu, UINT uID, BOOL bEnable)
{
	ASSERT(hMenu);
	ASSERT(uID > 0);

	if (bEnable) {
		EnableMenuItem(hMenu, uID, MF_BYCOMMAND | MF_ENABLED);
	}
	else {
		EnableMenuItem(hMenu, uID, MF_BYCOMMAND | MF_GRAYED);
	}
}

/*
 *	���j���[Check
 */
static void FASTCALL CheckMenuSub(HMENU hMenu, UINT uID, BOOL bCheck)
{
	ASSERT(hMenu);
	ASSERT(uID > 0);

	if (bCheck) {
		CheckMenuItem(hMenu, uID, MF_BYCOMMAND | MF_CHECKED);
	}
	else {
		CheckMenuItem(hMenu, uID, MF_BYCOMMAND | MF_UNCHECKED);
	}
}

/*-[ �t�@�C�����j���[ ]-----------------------------------------------------*/

/*
 *	�J��(O)
 */
static void FASTCALL OnOpen(HWND hWnd)
{
	char path[_MAX_PATH];

	ASSERT(hWnd);

	/* �t�@�C���I���T�u */
	if (!FileSelectSub(TRUE, IDS_STATEFILTER, path, NULL)) {
		return;
	}

	/* �X�e�[�g���[�h */
	LockVM();
	StopSnd();
	if (!system_load(path)) {
		LoadString(hAppInstance, IDS_STATEERROR, path, sizeof(path));
		MessageBox(hWnd, path, "XM7", MB_ICONSTOP | MB_OK);
		system_reset();
	}
	else {
		strcpy(StatePath, path);
	}
	PlaySnd();
	ResetSch();
	ApplyCfg();
	UnlockVM();

	/* ��ʍĕ`�� */
	OnRefresh(hWnd);
}

/*
 *	�ۑ�(A)
 */
static void FASTCALL OnSaveAs(HWND hWnd)
{
	char path[_MAX_PATH];

	ASSERT(hWnd);

	/* �t�@�C���I���T�u */
	if (!FileSelectSub(FALSE, IDS_STATEFILTER, path, "XM7")) {
		return;
	}

	/* �X�e�[�g�Z�[�u */
	LockVM();
	StopSnd();
	if (!system_save(path)) {
		LoadString(hAppInstance, IDS_STATEERROR, path, sizeof(path));
		MessageBox(hWnd, path, "XM7", MB_ICONSTOP | MB_OK);
	}
	else {
		strcpy(StatePath, path);
	}
	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	���O��t���ĕۑ�(S)
 */
static void FASTCALL OnSave(HWND hWnd)
{
	char string[128];

	/* �܂��ۑ�����Ă��Ȃ���΁A���O������ */
	if (StatePath[0] == '\0') {
		OnSaveAs(hWnd);
		return;
	}

	/* �X�e�[�g�Z�[�u */
	LockVM();
	StopSnd();
	if (!system_save(StatePath)) {
		LoadString(hAppInstance, IDS_STATEERROR, string, sizeof(string));
		MessageBox(hWnd, string, "XM7", MB_ICONSTOP | MB_OK);
	}
	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	���Z�b�g(R)
 */
static void FASTCALL OnReset(HWND hWnd)
{
	LockVM();
	system_reset();
	UnlockVM();

	/* �ĕ`�� */
	OnRefresh(hWnd);
}

/*
 *	BASIC���[�h(B)
 */
static void FASTCALL OnBasic(void)
{
	LockVM();
	boot_mode = BOOT_BASIC;
	system_reset();
	UnlockVM();
}

/*
 *	DOS���[�h(D)
 */
static void FASTCALL OnDos(void)
{
	LockVM();
	boot_mode = BOOT_DOS;
	system_reset();
	UnlockVM();
}

/*
 *	�I��(X)
 */
static void FASTCALL OnExit(HWND hWnd)
{
	/* �E�C���h�E�N���[�Y */
	PostMessage(hWnd, WM_CLOSE, 0, 0);
}

/*
 *	�t�@�C��(F)���j���[
 */
static BOOL OnFile(HWND hWnd, WORD wID)
{
	ASSERT(hWnd);

	switch (wID) {
		/* �I�[�v�� */
		case IDM_OPEN:
			OnOpen(hWnd);
			return TRUE;

		/* �ۑ� */
		case IDM_SAVE:
			OnSave(hWnd);
			return TRUE;

		/* ���O�����ĕۑ� */
		case IDM_SAVEAS:
			OnSaveAs(hWnd);
			return TRUE;

		/* ���Z�b�g */
		case IDM_RESET:
			OnReset(hWnd);
			return TRUE;

		/* BASIC���[�h */
		case IDM_BASIC:
			OnBasic();
			return TRUE;

		/* DOS���[�h */
		case IDM_DOS:
			OnDos();
			return TRUE;

		/* �I�� */
		case IDM_EXIT:
			OnExit(hWnd);
			return TRUE;
	}

	return FALSE;
}

/*
 *	�t�@�C��(F)���j���[�X�V
 */
static void FASTCALL OnFilePopup(HMENU hMenu)
{
	ASSERT(hMenu);

	CheckMenuSub(hMenu, IDM_BASIC, (boot_mode == BOOT_BASIC));
	CheckMenuSub(hMenu, IDM_DOS, (boot_mode == BOOT_DOS));
}

/*-[ �f�B�X�N���j���[ ]-----------------------------------------------------*/

/*
 *	�h���C�u���J��
 */
static void FASTCALL OnDiskOpen(int Drive)
{
	char path[_MAX_PATH];

	ASSERT((Drive == 0) || (Drive == 1));

	/* �t�@�C���I�� */
	if (!FileSelectSub(TRUE, IDS_DISKFILTER, path, NULL)) {
		return;
	}

	/* �Z�b�g */
	LockVM();
	fdc_setdisk(Drive, path);
	ResetSch();
	UnlockVM();
}

/*
 *	���h���C�u���J��
 */
static void FASTCALL OnDiskBoth(void)
{
	char path[_MAX_PATH];

	/* �t�@�C���I�� */
	if (!FileSelectSub(TRUE, IDS_DISKFILTER, path, NULL)) {
		return;
	}

	/* �Z�b�g */
	LockVM();
	fdc_setdisk(0, path);
	fdc_setdisk(1, NULL);
	if ((fdc_ready[0] != FDC_TYPE_NOTREADY) && (fdc_medias[0] >= 2)) {
		fdc_setdisk(1, path);
		fdc_setmedia(1, 1);
	}
	ResetSch();
	UnlockVM();
}

/*
 *	�f�B�X�N�C�W�F�N�g
 */
static void FASTCALL OnDiskEject(int Drive)
{
	ASSERT((Drive == 0) || (Drive == 1));

	/* �C�W�F�N�g */
	LockVM();
	fdc_setdisk(Drive, NULL);
	UnlockVM();
}

/*
 *	�f�B�X�N�ꎞ���o��
 */
static void FASTCALL OnDiskTemp(int Drive)
{
	ASSERT((Drive == 0) || (Drive == 1));

	/* �������݋֎~�؂�ւ� */
	LockVM();
	if (fdc_teject[Drive]) {
		fdc_teject[Drive] = FALSE;
	}
	else {
		fdc_teject[Drive] = TRUE;
	}
	UnlockVM();
}

/*
 *	�f�B�X�N�������݋֎~
 */
static void FASTCALL OnDiskProtect(int Drive)
{
	ASSERT((Drive == 0) || (Drive == 1));

	/* �������݋֎~�؂�ւ� */
	LockVM();
	if (fdc_writep[Drive]) {
		fdc_setwritep(Drive, FALSE);
	}
	else {
		fdc_setwritep(Drive, TRUE);
	}
	ResetSch();
	UnlockVM();
}

/*
 *	�f�B�X�N(1)(0)���j���[
 */
static BOOL FASTCALL OnDisk(WORD wID)
{
	switch (wID) {
		/* �J�� */
		case IDM_D0OPEN:
			OnDiskOpen(0);
			break;
		case IDM_D1OPEN:
			OnDiskOpen(1);
			break;

		/* ���h���C�u�ŊJ�� */
		case IDM_DBOPEN:
			OnDiskBoth();
			break;

		/* ���O�� */
		case IDM_D0EJECT:
			OnDiskEject(0);
			break;
		case IDM_D1EJECT:
			OnDiskEject(1);
			break;

		/* �ꎞ�C�W�F�N�g */
		case IDM_D0TEMP:
			OnDiskTemp(0);
			break;
		case IDM_D1TEMP:
			OnDiskTemp(1);
			break;

		/* �������݋֎~ */
		case IDM_D0WRITE:
			OnDiskProtect(0);
			break;
		case IDM_D1WRITE:
			OnDiskProtect(1);
			break;
	}

	/* ���f�B�A���� */
	if ((wID >= IDM_D0MEDIA00) && (wID <= IDM_D0MEDIA15)) {
		LockVM();
		fdc_setmedia(0, wID - IDM_D0MEDIA00);
		ResetSch();
		UnlockVM();
	}
	if ((wID >= IDM_D1MEDIA00) && (wID <= IDM_D1MEDIA15)) {
		LockVM();
		fdc_setmedia(1, wID - IDM_D1MEDIA00);
		ResetSch();
		UnlockVM();
	}

	return FALSE;
}

/*
 *	�f�B�X�N(1)(0)���j���[�X�V
 */
static void FASTCALL OnDiskPopup(HMENU hMenu, int Drive)
{
	MENUITEMINFO mii;
	char string[128];
	int offset;
	int i;

	ASSERT(hMenu);
	ASSERT((Drive == 0) || (Drive == 1));

	/* ���j���[���ׂč폜 */
	while (GetMenuItemCount(hMenu) > 0) {
		DeleteMenu(hMenu, 0, MF_BYPOSITION);
	}

	/* �I�t�Z�b�g�m�� */
	if (Drive == 0) {
		offset = 0;
	}
	else {
		offset = IDM_D1OPEN - IDM_D0OPEN;
	}

	/* ���j���[�\���̏����� */
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = 44;	/* sizeof(mii)��WINVER>=0x0500���� */
	mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.fState = MFS_ENABLED;

	/* �I�[�v���ƁA���I�[�v�� */
	mii.wID = IDM_D0OPEN + offset;
	LoadString(hAppInstance, IDS_DISKOPEN, string, sizeof(string));
	mii.dwTypeData = string;
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 0, TRUE, &mii);
	mii.wID = IDM_DBOPEN;
	LoadString(hAppInstance, IDS_DISKBOTH, string, sizeof(string));
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 1, TRUE, &mii);

	/* �f�B�X�N���}������Ă��Ȃ���΁A�����܂� */
	if (fdc_ready[Drive] == FDC_TYPE_NOTREADY) {
		return;
	}

	/* �C�W�F�N�g */
	mii.wID = IDM_D0EJECT + offset;
	LoadString(hAppInstance, IDS_DISKEJECT, string, sizeof(string));
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 2, TRUE, &mii);

	/* �Z�p���[�^�}�� */
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(hMenu, 3, TRUE, &mii);

	/* �ꎞ���o�� */
	mii.wID = IDM_D0TEMP + offset;
	LoadString(hAppInstance, IDS_DISKTEMP, string, sizeof(string));
	if (fdc_teject[Drive]) {
		mii.fState = MFS_CHECKED | MFS_ENABLED;
	}
	else {
		mii.fState = MFS_ENABLED;
	}
	mii.fType = MFT_STRING;
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 4, TRUE, &mii);

	/* ���C�g�v���e�N�g */
	mii.wID = IDM_D0WRITE + offset;
	LoadString(hAppInstance, IDS_DISKPROTECT, string, sizeof(string));
	if (fdc_fwritep[Drive]) {
		mii.fState = MFS_GRAYED;
	}
	else {
		if (fdc_writep[Drive]) {
			mii.fState = MFS_CHECKED | MFS_ENABLED;
		}
		else {
			mii.fState = MFS_ENABLED;
		}
	}
	mii.fType = MFT_STRING;
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 5, TRUE, &mii);

	/* �Z�p���[�^�}�� */
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(hMenu, 6, TRUE, &mii);

	/* 2D�Ȃ���ꏈ�� */
	if (fdc_ready[Drive] == FDC_TYPE_2D) {
		mii.wID = IDM_D0MEDIA00 + offset;
		mii.fState = MFS_CHECKED | MFS_ENABLED;
		mii.fType = MFT_STRING;
		LoadString(hAppInstance, IDS_DISK2D, string, sizeof(string));
		mii.cch = strlen(string);
		InsertMenuItem(hMenu, 7, TRUE, &mii);
		return;
	}

	/* ���f�B�A���� */
	for (i=0; i<fdc_medias[Drive]; i++) {
		mii.wID = IDM_D0MEDIA00 + offset + i;
		if (fdc_media[Drive] == i) {
			mii.fState = MFS_CHECKED | MFS_ENABLED;
		}
		else {
			mii.fState = MFS_ENABLED;
		}
		mii.fType = MFT_STRING;
		strcpy(string, fdc_name[Drive][i]);
		mii.cch = strlen(string);
		InsertMenuItem(hMenu, 7 + i, TRUE, &mii);
	}
}

/*-[ �e�[�v���j���[ ]-------------------------------------------------------*/

/*
 *  �e�[�v�I�[�v��
 */
static void FASTCALL OnTapeOpen(void)
{
	char path[_MAX_PATH];

	/* �t�@�C���I�� */
	if (!FileSelectSub(TRUE, IDS_TAPEFILTER, path, NULL)) {
		return;
	}

	/* �Z�b�g */
	LockVM();
	tape_setfile(path);
	ResetSch();
	UnlockVM();
}

/*
 *	�e�[�v�C�W�F�N�g
 */
static void FASTCALL OnTapeEject(void)
{
	/* �C�W�F�N�g */
	LockVM();
	tape_setfile(NULL);
	UnlockVM();
}

/*
 *	�����߂�
 */
static void FASTCALL OnRew(void)
{
	HCURSOR hCursor;

	/* �����߂� */
	hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	LockVM();
	StopSnd();

	tape_rew();

	PlaySnd();
	ResetSch();
	UnlockVM();
	SetCursor(hCursor);
}

/*
 *	������
 */
static void FASTCALL OnFF(void)
{
	HCURSOR hCursor;

	/* �����߂� */
	hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	LockVM();
	StopSnd();

	tape_ff();

	PlaySnd();
	ResetSch();
	UnlockVM();
	SetCursor(hCursor);
}

/*
 *	�^��
 */
static void FASTCALL OnRec(void)
{
	/* �^�� */
	LockVM();
	if (tape_rec) {
		tape_setrec(FALSE);
	}
	else {
		tape_setrec(TRUE);
	}
	UnlockVM();
}

/*
 *	�e�[�v(A)���j���[
 */
static BOOL FASTCALL OnTape(WORD wID)
{
	switch (wID) {
		/* �J�� */
		case IDM_TOPEN:
			OnTapeOpen();
			return TRUE;

		/* ���O�� */
		case IDM_TEJECT:
			OnTapeEject();
			return TRUE;

		/* �����߂� */
		case IDM_REW:
			OnRew();
			return TRUE;

		/* ������ */
		case IDM_FF:
			OnFF();
			return TRUE;

		/* �^�� */
		case IDM_REC:
			OnRec();
			return TRUE;
	}

	return FALSE;
}

/*
 *	�e�[�v(A)���j���[�X�V
 */
static void FASTCALL OnTapePopup(HMENU hMenu)
{
	MENUITEMINFO mii;
	char string[128];

	ASSERT(hMenu);

	/* ���j���[���ׂč폜 */
	while (GetMenuItemCount(hMenu) > 0) {
		DeleteMenu(hMenu, 0, MF_BYPOSITION);
	}

	/* ���j���[�\���̏����� */
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = 44;	// sizeof(mii)��WINVER>=0x0500����
	mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.fState = MFS_ENABLED;

	/* �I�[�v�� */
	mii.wID = IDM_TOPEN;
	LoadString(hAppInstance, IDS_TAPEOPEN, string, sizeof(string));
	mii.dwTypeData = string;
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 0, TRUE, &mii);

	/* �e�[�v���Z�b�g����Ă��Ȃ���΁A�����܂� */
	if (tape_fileh == -1) {
		return;
	}

	/* �C�W�F�N�g */
	mii.wID = IDM_TEJECT;
	LoadString(hAppInstance, IDS_TAPEEJECT, string, sizeof(string));
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 1, TRUE, &mii);

	/* �Z�p���[�^�}�� */
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(hMenu, 2, TRUE, &mii);
	mii.fType = MFT_STRING;

	/* �����߂� */
	mii.wID = IDM_REW;
	LoadString(hAppInstance, IDS_TAPEREW, string, sizeof(string));
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 3, TRUE, &mii);

	/* ������ */
	mii.wID = IDM_FF;
	LoadString(hAppInstance, IDS_TAPEFF, string, sizeof(string));
	mii.cch = strlen(string);
	InsertMenuItem(hMenu, 4, TRUE, &mii);

	/* �Z�p���[�^�}�� */
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(hMenu, 5, TRUE, &mii);
	mii.fType = MFT_STRING;

	/* �^�� */
	mii.wID = IDM_REC;
	LoadString(hAppInstance, IDS_TAPEREC, string, sizeof(string));
	mii.cch = strlen(string);
	if (tape_writep) {
		mii.fState = MFS_GRAYED;
	}
	else {
		if (tape_rec) {
			mii.fState = MFS_CHECKED | MFS_ENABLED;
		}
		else {
			mii.fState = MFS_ENABLED;
		}
	}
	InsertMenuItem(hMenu, 6, TRUE, &mii);
}

/*-[ �\�����j���[ ]---------------------------------------------------------*/

/*
 *	�t���b�s�[�f�B�X�N�R���g���[��(F)
 */
static void FASTCALL OnFDC(void)
{
	ASSERT(hDrawWnd);

	/* �E�C���h�E�����݂���΁A�N���[�Y�w�����o�� */
	if (hSubWnd[SWND_FDC]) {
		PostMessage(hSubWnd[SWND_FDC], WM_CLOSE, 0, 0);
		return;
	}

	/* �E�C���h�E�쐬 */
	hSubWnd[SWND_FDC] = CreateFDC(hDrawWnd, SWND_FDC);
}

/*
 *	FM�������W�X�^(O)
 */
static void FASTCALL OnOPNReg(void)
{
	ASSERT(hDrawWnd);

	/* �E�C���h�E�����݂���΁A�N���[�Y�w�����o�� */
	if (hSubWnd[SWND_OPNREG]) {
		PostMessage(hSubWnd[SWND_OPNREG], WM_CLOSE, 0, 0);
		return;
	}

	/* �E�C���h�E�쐬 */
	hSubWnd[SWND_OPNREG] = CreateOPNReg(hDrawWnd, SWND_OPNREG);
}

/*
 *	FM�����f�B�X�v���C(D)
 */
static void FASTCALL OnOPNDisp(void)
{
	ASSERT(hDrawWnd);

	/* �E�C���h�E�����݂���΁A�N���[�Y�w�����o�� */
	if (hSubWnd[SWND_OPNDISP]) {
		PostMessage(hSubWnd[SWND_OPNDISP], WM_CLOSE, 0, 0);
		return;
	}

	/* �E�C���h�E�쐬 */
	hSubWnd[SWND_OPNDISP] = CreateOPNDisp(hDrawWnd, SWND_OPNDISP);
}

/*
 *	�T�uCPU�R���g���[��(C)
 */
static void FASTCALL OnSubCtrl(void)
{
	ASSERT(hDrawWnd);

	/* �E�C���h�E�����݂���΁A�N���[�Y�w�����o�� */
	if (hSubWnd[SWND_SUBCTRL]) {
		PostMessage(hSubWnd[SWND_SUBCTRL], WM_CLOSE, 0, 0);
		return;
	}

	/* �E�C���h�E�쐬 */
	hSubWnd[SWND_SUBCTRL] = CreateSubCtrl(hDrawWnd, SWND_SUBCTRL);
}

/*
 *	�L�[�{�[�h(K)
 */
static void FASTCALL OnKeyboard(void)
{
	ASSERT(hDrawWnd);

	/* �E�C���h�E�����݂���΁A�N���[�Y�w�����o�� */
	if (hSubWnd[SWND_KEYBOARD]) {
		PostMessage(hSubWnd[SWND_KEYBOARD], WM_CLOSE, 0, 0);
		return;
	}

	/* �E�C���h�E�쐬 */
	hSubWnd[SWND_KEYBOARD] = CreateKeyboard(hDrawWnd, SWND_KEYBOARD);
}

/*
 *	�������Ǘ�(M)
 */
static void FASTCALL OnMMR(void)
{
	ASSERT(hDrawWnd);

	/* �E�C���h�E�����݂���΁A�N���[�Y�w�����o�� */
	if (hSubWnd[SWND_MMR]) {
		PostMessage(hSubWnd[SWND_MMR], WM_CLOSE, 0, 0);
		return;
	}

	/* �E�C���h�E�쐬 */
	hSubWnd[SWND_MMR] = CreateMMR(hDrawWnd, SWND_MMR);
}

/*
 *	�X�e�[�^�X�o�[(S)
 */
static void FASTCALL OnStatus(HWND hWnd)
{
	RECT rect;

	/* �X�e�[�^�X�o�[���L���łȂ���΁A�������Ȃ� */
	if (!hStatusBar) {
		return;
	}

	if (IsWindowVisible(hStatusBar)) {
		/* ���� */
		ShowWindow(hStatusBar, SW_HIDE);
	}
	else {
		/* �\�� */
		ShowWindow(hStatusBar, SW_SHOW);
	}

	/* �t���[���E�C���h�E�̃T�C�Y��␳ */
	GetClientRect(hWnd, &rect);
	PostMessage(hWnd, WM_SIZE, 0, MAKELPARAM(rect.right, rect.bottom));
}

/*
 *	�ŐV�̏��ɍX�V(R)
 */
static void FASTCALL OnRefresh(HWND hWnd)
{
	int i;

	ASSERT(hWnd);
	ASSERT(hDrawWnd);

	/* �t�A�Z���u���E�C���h�E�@�A�h���X�X�V */
	if (hSubWnd[SWND_DISASM_MAIN]) {
		AddrDisAsm(TRUE, maincpu.pc);
	}
	if (hSubWnd[SWND_DISASM_SUB]) {
		AddrDisAsm(FALSE, subcpu.pc);
	}

	/* ���C���E�C���h�E */
	InvalidateRect(hWnd, NULL, FALSE);
	InvalidateRect(hDrawWnd, NULL, FALSE);

	/* �T�u�E�C���h�E */
	for (i=0; i<SWND_MAXNUM; i++) {
		if (hSubWnd[i]) {
			InvalidateRect(hSubWnd[i], NULL, FALSE);
		}
	}
}

/*
 *	���s�ɓ���(Y)
 */
static void FASTCALL OnSync(void)
{
	bSync = (!bSync);
}

/*
 *	�t���X�N���[��(U)
 */
static void FASTCALL OnFullScreen(HWND hWnd)
{
	BOOL bRun;

	ASSERT(hWnd);

	/* VM�����b�N�A�X�g�b�v */
	LockVM();
	bRun = run_flag;
	run_flag = FALSE;
	StopSnd();

	/* ���[�h�؂�ւ� */
	if (bFullScreen) {
		ModeDraw(hWnd, FALSE);
	}
	else {
		ModeDraw(hWnd, TRUE);
	}

	/* VM���A�����b�N */
	run_flag = bRun;
	ResetSch();
	UnlockVM();
	PlaySnd();
}

/*
 *	�\��(V)���j���[
 */
static BOOL FASTCALL OnView(HWND hWnd, WORD wID)
{
	ASSERT(hWnd);

	switch (wID) {
		/* FDC */
		case IDM_FDC:
			OnFDC();
			return TRUE;

		/* OPN���W�X�^ */
		case IDM_OPNREG:
			OnOPNReg();
			return TRUE;

		/* OPN�f�B�X�v���C */
		case IDM_OPNDISP:
			OnOPNDisp();
			return TRUE;

		/* �T�uCPU�R���g���[�� */
		case IDM_SUBCTRL:
			OnSubCtrl();
			return TRUE;

		/* �L�[�{�[�h */
		case IDM_KEYBOARD:
			OnKeyboard();
			return TRUE;

		/* �������Ǘ� */
		case IDM_MMR:
			OnMMR();
			return TRUE;

		/* �X�e�[�^�X�o�[ */
		case IDM_STATUS:
			OnStatus(hWnd);
			return TRUE;

		/* �ŐV�̏��ɍX�V */
		case IDM_REFRESH:
			OnRefresh(hWnd);
			return TRUE;

		/* ���s�ɓ��� */
		case IDM_SYNC:
			OnSync();
			return TRUE;

		/* �t���X�N���[�� */
		case IDM_FULLSCREEN:
			OnFullScreen(hWnd);
			return TRUE;
	}

	return FALSE;
}

/*
 *	�\��(V)���j���[�X�V
 */
static void FASTCALL OnViewPopup(HMENU hMenu)
{
	/* �T�u�E�C���h�E�Q */
	CheckMenuSub(hMenu, IDM_FDC, (BOOL)hSubWnd[SWND_FDC]);
	CheckMenuSub(hMenu, IDM_OPNREG, (BOOL)hSubWnd[SWND_OPNREG]);
	CheckMenuSub(hMenu, IDM_OPNDISP, (BOOL)hSubWnd[SWND_OPNDISP]);
	CheckMenuSub(hMenu, IDM_SUBCTRL, (BOOL)hSubWnd[SWND_SUBCTRL]);
	CheckMenuSub(hMenu, IDM_KEYBOARD, (BOOL)hSubWnd[SWND_KEYBOARD]);
	CheckMenuSub(hMenu, IDM_MMR, (BOOL)hSubWnd[SWND_MMR]);

	/* ���̑� */
	if (hStatusBar) {
		CheckMenuSub(hMenu, IDM_STATUS, IsWindowVisible(hStatusBar));
	}
	else {
		CheckMenuSub(hMenu, IDM_STATUS, FALSE);
	}
	CheckMenuSub(hMenu, IDM_SYNC, bSync);
	CheckMenuSub(hMenu, IDM_FULLSCREEN, bFullScreen);
}

/*-[ �f�o�b�O���j���[ ]-----------------------------------------------------*/

/*
 *	���s(X)
 */
static void FASTCALL OnExec(void)
{
	/* ���Ɏ��s���Ȃ�A�������Ȃ� */
	if (run_flag) {
		return;
	}

	/* �X�^�[�g */
	LockVM();
	stopreq_flag = FALSE;
	run_flag = TRUE;
	UnlockVM();
}

/*
 *	��~(B)
 */
static void FASTCALL OnBreak(void)
{
	/* ���ɒ�~��ԂȂ�A�������Ȃ� */
	if (!run_flag) {
		return;
	}

	/* ��~ */
	LockVM();
	stopreq_flag = TRUE;
	UnlockVM();
}

/*
 *	�g���[�X(T)
 */
static void FASTCALL OnTrace(HWND hWnd)
{
	ASSERT(hWnd);

	/* ��~��ԂłȂ���΁A���^�[�� */
	if (run_flag) {
		return;
	}

	/* ���s */
	schedule_trace();

	/* �\���X�V */
	OnRefresh(hWnd);
}

/*
 *	�u���[�N�|�C���g(B)
 */
static void FASTCALL OnBreakPoint(void)
{
	ASSERT(hDrawWnd);

	/* �E�C���h�E�����݂���΁A�N���[�Y�w�����o�� */
	if (hSubWnd[SWND_BREAKPOINT]) {
		PostMessage(hSubWnd[SWND_BREAKPOINT], WM_CLOSE, 0, 0);
		return;
	}

	/* �E�C���h�E�쐬 */
	hSubWnd[SWND_BREAKPOINT] = CreateBreakPoint(hDrawWnd, SWND_BREAKPOINT);
}

/*
 *	�X�P�W���[��(S)
 */
static void FASTCALL OnScheduler(void)
{
	ASSERT(hDrawWnd);

	/* �E�C���h�E�����݂���΁A�N���[�Y�w�����o�� */
	if (hSubWnd[SWND_SCHEDULER]) {
		PostMessage(hSubWnd[SWND_SCHEDULER], WM_CLOSE, 0, 0);
		return;
	}

	/* �E�C���h�E�쐬 */
	hSubWnd[SWND_SCHEDULER] = CreateScheduler(hDrawWnd, SWND_SCHEDULER);
}

/*
 *	CPU���W�X�^(C)
 */
static void FASTCALL OnCPURegister(BOOL bMain)
{
	int index;

	ASSERT((bMain == TRUE) || (bMain == FALSE));
	ASSERT(hDrawWnd);

	/* �C���f�b�N�X���� */
	index = SWND_CPUREG_MAIN;
	if (!bMain) {
		index++;
	}

	/* �E�C���h�E�����݂���΁A�N���[�Y�w�����o�� */
	if (hSubWnd[index]) {
		PostMessage(hSubWnd[index], WM_CLOSE, 0, 0);
		return;
	}

	/* �E�C���h�E�쐬 */
	hSubWnd[index] = CreateCPURegister(hDrawWnd, bMain, index);
}

/*
 *	�t�A�Z���u��(D)
 */
static void FASTCALL OnDisAsm(BOOL bMain)
{
	int index;

	ASSERT((bMain == TRUE) || (bMain == FALSE));
	ASSERT(hDrawWnd);

	/* �C���f�b�N�X���� */
	index = SWND_DISASM_MAIN;
	if (!bMain) {
		index++;
	}

	/* �E�C���h�E�����݂���΁A�N���[�Y�w�����o�� */
	if (hSubWnd[index]) {
		PostMessage(hSubWnd[index], WM_CLOSE, 0, 0);
		return;
	}

	/* �E�C���h�E�쐬 */
	hSubWnd[index] = CreateDisAsm(hDrawWnd, bMain, index);
}

/*
 *	�������_���v(M)
 */
static void FASTCALL OnMemory(BOOL bMain)
{
	int index;

	ASSERT((bMain == TRUE) || (bMain == FALSE));
	ASSERT(hDrawWnd);

	/* �C���f�b�N�X���� */
	index = SWND_MEMORY_MAIN;
	if (!bMain) {
		index++;
	}

	/* �E�C���h�E�����݂���΁A�N���[�Y�w�����o�� */
	if (hSubWnd[index]) {
		PostMessage(hSubWnd[index], WM_CLOSE, 0, 0);
		return;
	}

	/* �E�C���h�E�쐬 */
	hSubWnd[index] = CreateMemory(hDrawWnd, bMain, index);
}

/*
 *	�f�o�b�O(D)���j���[
 */
static BOOL FASTCALL OnDebug(HWND hWnd, WORD wID)
{
	ASSERT(hWnd);

	switch (wID) {
		/* ���s */
		case IDM_EXEC:
			OnExec();
			return TRUE;

		/* �u���[�N */
		case IDM_BREAK:
			OnBreak();
			return TRUE;

		/* �g���[�X */
		case IDM_TRACE:
			OnTrace(hWnd);
			return TRUE;

		/* �u���[�N�|�C���g */
		case IDM_BREAKPOINT:
			OnBreakPoint();
			return TRUE;

		/* �X�P�W���[�� */
		case IDM_SCHEDULER:
			OnScheduler();
			return TRUE;

		/* CPU���W�X�^(���C��) */
		case IDM_CPU_MAIN:
			OnCPURegister(TRUE);
			return TRUE;

		/* CPU���W�X�^(�T�u) */
		case IDM_CPU_SUB:
			OnCPURegister(FALSE);
			return TRUE;

		/* �t�A�Z���u��(���C��) */
		case IDM_DISASM_MAIN:
			OnDisAsm(TRUE);
			return TRUE;

		/* �t�A�Z���u��(�T�u) */
		case IDM_DISASM_SUB:
			OnDisAsm(FALSE);
			return TRUE;

		/* �������_���v(���C��) */
		case IDM_MEMORY_MAIN:
			OnMemory(TRUE);
			return TRUE;

		/* �������_���v(�T�u) */
		case IDM_MEMORY_SUB:
			OnMemory(FALSE);
			return TRUE;
	}

	return FALSE;
}

/*
 *	�f�o�b�O(D)���j���[�X�V
 */
static void FASTCALL OnDebugPopup(HMENU hMenu)
{
	ASSERT(hMenu);

	/* ���s���� */
	EnableMenuSub(hMenu, IDM_EXEC, !run_flag);
	EnableMenuSub(hMenu, IDM_BREAK, run_flag);
	EnableMenuSub(hMenu, IDM_TRACE, !run_flag);

	/* �T�u�E�C���h�E�Q */
	CheckMenuSub(hMenu, IDM_BREAKPOINT, (BOOL)hSubWnd[SWND_BREAKPOINT]);
	CheckMenuSub(hMenu, IDM_SCHEDULER, (BOOL)hSubWnd[SWND_SCHEDULER]);
	CheckMenuSub(hMenu, IDM_CPU_MAIN, (BOOL)hSubWnd[SWND_CPUREG_MAIN]);
	CheckMenuSub(hMenu, IDM_CPU_SUB, (BOOL)hSubWnd[SWND_CPUREG_SUB]);
	CheckMenuSub(hMenu, IDM_DISASM_MAIN, (BOOL)hSubWnd[SWND_DISASM_MAIN]);
	CheckMenuSub(hMenu, IDM_DISASM_SUB, (BOOL)hSubWnd[SWND_DISASM_SUB]);
	CheckMenuSub(hMenu, IDM_MEMORY_MAIN, (BOOL)hSubWnd[SWND_MEMORY_MAIN]);
	CheckMenuSub(hMenu, IDM_MEMORY_SUB, (BOOL)hSubWnd[SWND_MEMORY_SUB]);
}

/*-[ �c�[�����j���[ ]-------------------------------------------------------*/

/*
 *	��ʃL���v�`��(C)
 */
static void FASTCALL OnGrpCapture(void)
{
	char path[_MAX_PATH];

	/* �t�@�C���I�� */
	if (!FileSelectSub(FALSE, IDS_GRPCAPFILTER, path, "BMP")) {
		return;
	}

	/* �L���v�`�� */
	LockVM();
	StopSnd();

	capture_to_bmp(path);

	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	WAV�L���v�`��(W)
 */
static void FASTCALL OnWavCapture(HWND hWnd)
{
	char path[_MAX_PATH];

	ASSERT(hWnd);

	/* ���ɃL���v�`�����Ȃ�A�N���[�Y */
	if (hWavCapture >= 0) {
		LockVM();
		CloseCaptureSnd();
		UnlockVM();
		return;
	}

	/* �t�@�C���I�� */
	if (!FileSelectSub(FALSE, IDS_WAVCAPFILTER, path, "WAV")) {
		return;
	}

	/* �L���v�`�� */
	LockVM();
	OpenCaptureSnd(path);
	UnlockVM();

	/* �������� */
	if (hWavCapture < 0) {
		LockVM();
		StopSnd();

		LoadString(hAppInstance, IDS_WAVCAPERROR, path, sizeof(path));
		MessageBox(hWnd, path, "XM7", MB_ICONSTOP | MB_OK);

		PlaySnd();
		ResetSch();
		UnlockVM();
	}
}

/*
 *	�V�K�f�B�X�N�쐬(D)
 */
static void FASTCALL OnNewDisk(HWND hWnd)
{
	char path[_MAX_PATH];
	int ret;

	ASSERT(hWnd);

	/* �^�C�g������ */
	ret = DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_TITLEDLG),
						hWnd, TitleDlgProc);
	if (ret != IDOK) {
		return;
	}

	/* �t�@�C���I�� */
	if (!FileSelectSub(FALSE, IDS_NEWDISKFILTER, path, "D77")) {
		return;
	}

	/* �쐬 */
	LockVM();
	StopSnd();

	if (make_new_d77(path, DiskTitle)) {
		LoadString(hAppInstance, IDS_NEWDISKOK, path, sizeof(path));
		MessageBox(hWnd, path, "XM7", MB_OK);
	}

	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	�V�K�e�[�v�쐬(T)
 */
static void FASTCALL OnNewTape(HWND hWnd)
{
	char path[_MAX_PATH];

	ASSERT(hWnd);

	/* �t�@�C���I�� */
	if (!FileSelectSub(FALSE, IDS_TAPEFILTER, path, "T77")) {
		return;
	}

	/* �쐬 */
	LockVM();
	StopSnd();

	if (make_new_t77(path)) {
		LoadString(hAppInstance, IDS_NEWTAPEOK, path, sizeof(path));
		MessageBox(hWnd, path, "XM7", MB_OK);
	}

	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	VFD��D77�ϊ�(V)
 */
static void FASTCALL OnVFD2D77(HWND hWnd)
{
	char src[_MAX_PATH];
	char dst[_MAX_PATH];
	int ret;

	ASSERT(hWnd);

	/* �t�@�C���I�� */
	if (!FileSelectSub(TRUE, IDS_VFDFILTER, src, "VFD")) {
		return;
	}

	/* �^�C�g������ */
	ret = DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_TITLEDLG),
						hWnd, TitleDlgProc);
	if (ret != IDOK) {
		return;
	}

	/* �t�@�C���I�� */
	if (!FileSelectSub(FALSE, IDS_NEWDISKFILTER, dst, "D77")) {
		return;
	}

	/* �쐬 */
	LockVM();
	StopSnd();

	if (conv_vfd_to_d77(src, dst, DiskTitle)) {
		LoadString(hAppInstance, IDS_CONVERTOK, src, sizeof(src));
		MessageBox(hWnd, src, "XM7", MB_OK);
	}

	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	2D��D77�ϊ�(2)
 */
static void FASTCALL On2D2D77(HWND hWnd)
{
	char src[_MAX_PATH];
	char dst[_MAX_PATH];
	int ret;

	ASSERT(hWnd);

	/* �t�@�C���I�� */
	if (!FileSelectSub(TRUE, IDS_2DFILTER, src, "2D")) {
		return;
	}

	/* �^�C�g������ */
	ret = DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_TITLEDLG),
						hWnd, TitleDlgProc);
	if (ret != IDOK) {
		return;
	}

	/* �t�@�C���I�� */
	if (!FileSelectSub(FALSE, IDS_NEWDISKFILTER, dst, "D77")) {
		return;
	}

	/* �쐬 */
	LockVM();
	StopSnd();

	if (conv_2d_to_d77(src, dst, DiskTitle)) {
		LoadString(hAppInstance, IDS_CONVERTOK, src, sizeof(src));
		MessageBox(hWnd, src, "XM7", MB_OK);
	}

	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	VTP��T77�ϊ�(P)
 */
static void FASTCALL OnVTP2T77(HWND hWnd)
{
	char src[_MAX_PATH];
	char dst[_MAX_PATH];

	ASSERT(hWnd);

	/* �t�@�C���I�� */
	if (!FileSelectSub(TRUE, IDS_VTPFILTER, src, "VTP")) {
		return;
	}

	/* �t�@�C���I�� */
	if (!FileSelectSub(FALSE, IDS_TAPEFILTER, dst, "T77")) {
		return;
	}

	/* �쐬 */
	LockVM();
	StopSnd();

	if (conv_vtp_to_t77(src, dst)) {
		LoadString(hAppInstance, IDS_CONVERTOK, src, sizeof(src));
		MessageBox(hWnd, src, "XM7", MB_OK);
	}

	PlaySnd();
	ResetSch();
	UnlockVM();
}

/*
 *	�c�[��(T)���j���[
 */
static BOOL FASTCALL OnTool(HWND hWnd, WORD wID)
{
	ASSERT(hWnd);

	switch (wID) {
		/* �ݒ� */
		case IDM_CONFIG:
			OnConfig(hWnd);
			return TRUE;

		/* ��ʃL���v�`�� */
		case IDM_GRPCAP:
			OnGrpCapture();
			return TRUE;

		/* WAV�L���v�`�� */
		case IDM_WAVCAP:
			OnWavCapture(hWnd);
			return TRUE;

		/* �V�K�f�B�X�N */
		case IDM_NEWDISK:
			OnNewDisk(hWnd);
			return TRUE;

		/* �V�K�e�[�v */
		case IDM_NEWTAPE:
			OnNewTape(hWnd);
			return TRUE;

		/* VFD��D77 */
		case IDM_VFD2D77:
			OnVFD2D77(hWnd);
			return TRUE;

		/* 2D��D77 */
		case IDM_2D2D77:
			On2D2D77(hWnd);
			return TRUE;

		/* VTP��T77 */
		case IDM_VTP2T77:
			OnVTP2T77(hWnd);
			return TRUE;
	}

	return FALSE;
}

/*
 *	�c�[��(T)���j���[�X�V
 */
static void FASTCALL OnToolPopup(HMENU hMenu)
{
	ASSERT(hMenu);

	/* WAV�L���v�`���̂݁B�n���h�������ŃI�[�v���� */
	CheckMenuSub(hMenu, IDM_WAVCAP, (hWavCapture >= 0));
}

/*-[ �E�B���h�E���j���[ ]---------------------------------------------------*/

/*
 *	�d�˂ĕ\��(C)
 */
static void FASTCALL OnCascade(void)
{
	/* �d�˂ĕ\�� */
	ASSERT(hDrawWnd);
	CascadeWindows(hDrawWnd, 0, NULL, 0, NULL);
}

/*
 *	���ׂĕ\��(T)
 */
static void FASTCALL OnTile(void)
{
	/* ���ׂĕ\�� */
	ASSERT(hDrawWnd);
	TileWindows(hDrawWnd, MDITILE_VERTICAL, NULL, 0, NULL);
}

/*
 *	�S�ăA�C�R����(I)
 */
static void FASTCALL OnIconic(void)
{
	int i;

	/* �S�ăA�C�R���� */
	for (i=0; i<SWND_MAXNUM; i++) {
		if (hSubWnd[i]) {
			ShowWindow(hSubWnd[i], SW_MINIMIZE);
		}
	}
}

/*
 *	�A�C�R���̐���(A)
 */
static void FASTCALL OnArrangeIcon(void)
{
	/* �A�C�R���̐��� */
	ASSERT(hDrawWnd);
	ArrangeIconicWindows(hDrawWnd);
}

/*
 *	�E�B���h�E(W)���j���[
 */
static BOOL FASTCALL OnWindow(WORD wID)
{
	int i;

	switch (wID) {
		/* �d�˂ĕ\�� */
		case IDM_CASCADE:
			OnCascade();
			return TRUE;

		/* ���ׂĕ\�� */
		case IDM_TILE:
			OnTile();
			return TRUE;

		/* �S�ăA�C�R���� */
		case IDM_ICONIC:
			OnIconic();
			return TRUE;

		/* �A�C�R���̐��� */
		case IDM_ARRANGEICON:
			OnArrangeIcon();
			return TRUE;
	}

	/* �E�B���h�E�I���� */
	if ((wID >= IDM_SWND00) && (wID <= IDM_SWND15)) {
		for (i=0; i<SWND_MAXNUM; i++) {
			if (hSubWnd[i] == NULL) {
				continue;
			}

			/* �J�E���g�_�E�����AOK */
			if (wID == IDM_SWND00) {
				ShowWindow(hSubWnd[i], SW_RESTORE);
				SetWindowPos(hSubWnd[i], HWND_TOP, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE);
				break;
			}
			else {
				wID--;
			}
		}
	}

	return FALSE;
}

/*
 *	�E�B���h�E(W)���j���[�X�V
 */
static void FASTCALL OnWindowPopup(HMENU hMenu)
{
	int i;
	BOOL flag;
	MENUITEMINFO mii;
	UINT nID;
	int count;
	char string[128];

	ASSERT(hMenu);

	/* �擪�̂S���c���č폜 */
	while (GetMenuItemCount(hMenu) > 4) {
		DeleteMenu(hMenu, 4, MF_BYPOSITION);
	}

	/* �L���ȃT�u�E�C���h�E���Ȃ���΁A���̂܂܃��^�[�� */
	flag = FALSE;
	for (i=0; i<SWND_MAXNUM; i++) {
		if (hSubWnd[i] != NULL) {
			flag = TRUE;
		}
	}
	if (!flag) {
		return;
	}

	/* ���j���[�\���̏����� */
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = 44;	// sizeof(mii)��WINVER>=0x0500����
	mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.fState = MFS_ENABLED;

	/* �Z�p���[�^�}�� */
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(hMenu, 4, TRUE, &mii);
	mii.fType = MFT_STRING;

	/* �E�C���h�E�^�C�g�����Z�b�g */
	count = 0;
	nID = IDM_SWND00;
	for (i=0; i<SWND_MAXNUM; i++) {
		if (hSubWnd[i] != NULL) {
			/* ���j���[�}�� */
			mii.wID = nID;
			memset(string, 0, sizeof(string));
			GetWindowText(hSubWnd[i], string, sizeof(string) - 1);
			mii.dwTypeData = string;
			mii.cch = strlen(string);
			InsertMenuItem(hMenu, count + 5, TRUE, &mii);

			/* ���� */
			nID++;
			count++;
		}
	}
}

/*-[ �w���v���j���[ ]-------------------------------------------------------*/

/*
 *	�w���v(H)���j���[
 */
static BOOL FASTCALL OnHelp(HWND hWnd, WORD wID)
{
	if (wID == IDM_ABOUT) {
		OnAbout(hWnd);
		return TRUE;
	}

	return FALSE;
}

/*-[ ���j���[�R�}���h���� ]-------------------------------------------------*/

/*
 *	���j���[�R�}���h����
 */
void FASTCALL OnCommand(HWND hWnd, WORD wID)
{
	ASSERT(hWnd);

	if (OnFile(hWnd, wID)) {
		return;
	}
	if (OnDisk(wID)) {
		return;
	}
	if (OnTape(wID)) {
		return;
	}
	if (OnView(hWnd, wID)) {
		return;
	}
	if (OnDebug(hWnd, wID)) {
		return;
	}
	if (OnTool(hWnd, wID)) {
		return;
	}
	if (OnWindow(wID)) {
		return;
	}
	if (OnHelp(hWnd, wID)) {
		return;
	}
}

/*
 *	���j���[�R�}���h�X�V����
 */
void FASTCALL OnMenuPopup(HWND hWnd, HMENU hSubMenu, UINT uPos)
{
	HMENU hMenu;

	ASSERT(hWnd);
	ASSERT(hSubMenu);

	/* ���C�����j���[�̍X�V���`�F�b�N */
	hMenu = GetMenu(hWnd);
	if (GetSubMenu(hMenu, uPos) != hSubMenu) {
		return;
	}

	/* ���b�N���K�v */
	LockVM();

	switch (uPos) {
		/* �t�@�C�� */
		case 0:
			OnFilePopup(hSubMenu);
			break;

		/* �h���C�u1 */
		case 1:
			OnDiskPopup(hSubMenu, 1);
			break;

		/* �h���C�u0 */
		case 2:
			OnDiskPopup(hSubMenu, 0);
			break;

		/* �e�[�v */
		case 3:
			OnTapePopup(hSubMenu);
			break;

		/* �\�� */
		case 4:
			OnViewPopup(hSubMenu);
			break;

		/* �f�o�b�O */
		case 5:
			OnDebugPopup(hSubMenu);
			break;

		/* �c�[�� */
		case 6:
			OnToolPopup(hSubMenu);
			break;

		/* �E�B���h�E */
		case 7:
			OnWindowPopup(hSubMenu);
			break;
	}

	/* �A�����b�N */
	UnlockVM();
}

/*
 *	�t�@�C���h���b�v�T�u
 */
void FASTCALL OnDropSub(char *path)
{
	char dir[_MAX_DIR];
	char ext[_MAX_EXT];

	ASSERT(path);

	/* �g���q�������� */
	_splitpath(path, InitialDir, dir, NULL, ext);
	strcat(InitialDir, dir);

	/* D77 */
	if (stricmp(ext, ".D77") == 0) {
		LockVM();
		StopSnd();
		fdc_setdisk(0, path);
		fdc_setdisk(1, NULL);
		if ((fdc_ready[0] != FDC_TYPE_NOTREADY) && (fdc_medias[0] >= 2)) {
			fdc_setdisk(1, path);
			fdc_setmedia(1, 1);
		}
		system_reset();
		PlaySnd();
		ResetSch();
		UnlockVM();
	}

	/* 2D */
	if (stricmp(ext, ".2D") == 0) {
		LockVM();
		StopSnd();
		fdc_setdisk(0, path);
		fdc_setdisk(1, NULL);
		system_reset();
		PlaySnd();
		ResetSch();
		UnlockVM();
	}

	/* T77 */
	if (stricmp(ext, ".T77") == 0) {
		LockVM();
		tape_setfile(path);
		UnlockVM();
	}

	/* XM7 */
	if (stricmp(ext, ".XM7") == 0) {
		LockVM();
		StopSnd();
		if (!system_load(path)) {
			LoadString(hAppInstance, IDS_STATEERROR, path, sizeof(path));
			MessageBox(hMainWnd, path, "XM7", MB_ICONSTOP | MB_OK);
			system_reset();
		}
		else {
			strcpy(StatePath, path);
		}
		PlaySnd();
		ResetSch();
		ApplyCfg();
		UnlockVM();
	}
}

/*
 *	�t�@�C���h���b�v
 */
void FASTCALL OnDropFiles(HANDLE hDrop)
{
	char path[_MAX_PATH];

	ASSERT(hDrop);

	/* �t�@�C�����󂯎�� */
	DragQueryFile(hDrop, 0, path, sizeof(path));
	DragFinish(hDrop);

	/* ���� */
	OnDropSub(path);
}

/*
 *	�R�}���h���C������
 */
void FASTCALL OnCmdLine(LPTSTR lpCmdLine)
{
	LPTSTR p;
	LPTSTR q;
	BOOL flag;

	ASSERT(lpCmdLine);

	/* �t�@�C�������X�L�b�v */
	p = lpCmdLine;
	flag = FALSE;
	for (;;) {
		/* �I���`�F�b�N */
		if (*p == '\0') {
			return;
		}

		/* �N�H�[�g�`�F�b�N */
		if (*p == '"') {
			flag = (!flag);
		}

		/* �X�y�[�X�`�F�b�N */
		if ((*p == ' ') && !flag) {
			break;
		}

		/* ���� */
		p = _tcsinc(p);
	}

	/* �X�y�[�X��ǂݔ�΂� */
	for (;;) {
		/* �I���`�F�b�N */
		if (*p == '\0') {
			return;
		}

		if (*p != ' ') {
			break;
		}

		/* ���� */
		p = _tcsinc(p);
	}

	/* �N�H�[�g�`�F�b�N */
	if (*p == '"') {
		p = _tcsinc(p);
		q = p;

		/* �N�H�[�g���o��܂ő����� */
		for (;;) {
			/* �I���`�F�b�N */
			if (*q == '\0') {
				return;
			}

			/* �N�H�[�g�`�F�b�N */
			if (*q == '"') {
				*q = '\0';
				break;
			}

			/* ���� */
			q = _tcsinc(q);
		}
	}

	/* ���� */
	OnDropSub(p);
}

#endif	/* _WIN32 */

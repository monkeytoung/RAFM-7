/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API �o�[�W�������_�C�A���O ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <assert.h>
#include "xm7.h"
#include "w32.h"
#include "w32_res.h"

/*
 *	�X�^�e�B�b�N ���[�N
 */
static RECT AboutURLRect;
static BOOL bAboutURLHit;
static char pszAboutURL[128];

/*
 *	�_�C�A���O������
 */
static void FASTCALL AboutDlgInit(HWND hDlg)
{
	HWND hWnd;
	RECT prect;
	RECT drect;
	POINT point;

	ASSERT(hDlg);

	/* �����񃊃\�[�X�����[�h */
	pszAboutURL[0] = '\0';
	LoadString(hAppInstance, IDS_ABOUTURL, pszAboutURL, sizeof(pszAboutURL));

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

	/* IDC_URL�̃N���C�A���g���W���擾 */
	hWnd = GetDlgItem(hDlg, IDC_URL);
	ASSERT(hWnd);
	GetWindowRect(hWnd, &prect);

	point.x = prect.left;
	point.y = prect.top;
	ScreenToClient(hDlg, &point);
	AboutURLRect.left = point.x;
	AboutURLRect.top = point.y;

	point.x = prect.right;
	point.y = prect.bottom;
	ScreenToClient(hDlg, &point);
	AboutURLRect.right = point.x;
	AboutURLRect.bottom = point.y;

	/* IDC_URL���폜 */
	DestroyWindow(hWnd);

	/* ���̑� */
	bAboutURLHit = FALSE;
}

/*
 *	�_�C�A���O�`��
 */
static void AboutDlgPaint(HWND hDlg)
{
	HDC hDC;
	PAINTSTRUCT ps;
	HFONT hFont;
	HFONT hDefFont;
	TEXTMETRIC tm;
	LOGFONT lf;

	ASSERT(hDlg);

	/* DC���擾 */
	hDC = BeginPaint(hDlg, &ps);

	/* GUI�t�H���g�̃��g���b�N�𓾂� */
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	hDefFont = (HFONT)SelectObject(hDC, hFont);
	GetTextMetrics(hDC, &tm);
	memset(&lf, 0, sizeof(lf));
	GetTextFace(hDC, LF_FACESIZE, lf.lfFaceName);
	SelectObject(hDC, hDefFont);

	/* �A���_�[���C����t�������t�H���g���쐬 */
	lf.lfHeight = tm.tmHeight;
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = FW_DONTCARE;
	lf.lfItalic = tm.tmItalic;
	lf.lfUnderline = TRUE;
	lf.lfStrikeOut = tm.tmStruckOut;
	lf.lfCharSet = tm.tmCharSet;
	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = tm.tmPitchAndFamily;
	hFont = CreateFontIndirect(&lf);
	hDefFont = (HFONT)SelectObject(hDC, hFont);

	/* �`�� */
	if (bAboutURLHit) {
		SetTextColor(hDC, RGB(255, 0, 0));
	}
	else {
		SetTextColor(hDC, RGB(0, 0, 255));
	}
	SetBkColor(hDC, GetSysColor(COLOR_3DFACE));
	DrawText(hDC, pszAboutURL, strlen(pszAboutURL),
		&AboutURLRect, DT_CENTER | DT_SINGLELINE);

	/* �t�H���g��߂� */
	SelectObject(hDC, hDefFont);
	DeleteObject(hFont);

	/* DC��� */
	EndPaint(hDlg, &ps);
}

/*
 *	�_�C�A���O�v���V�[�W��
 */
static BOOL CALLBACK AboutDlgProc(HWND hDlg, UINT iMsg,
									WPARAM wParam, LPARAM lParam)
{
	POINT point;
	BOOL bFlag;
	HCURSOR hCursor;

	switch (iMsg) {

		/* �_�C�A���O������ */
		case WM_INITDIALOG:
			AboutDlgInit(hDlg);
			return TRUE;

		/* �ĕ`�� */
		case WM_PAINT:
			AboutDlgPaint(hDlg);
			return 0;

		/* �R�}���h */
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				/* �I�� */
				case IDOK:
				case IDCANCEL:
					EndDialog(hDlg, 0);
					return TRUE;
			}

		/* �̈�`�F�b�N */
		case WM_NCHITTEST:
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			ScreenToClient(hDlg, &point);
			bFlag = PtInRect(&AboutURLRect, point);
			/* �t���O���قȂ�����A�X�V���čĕ`�� */
			if (bFlag != bAboutURLHit) {
				bAboutURLHit = bFlag;
				InvalidateRect(hDlg, &AboutURLRect, FALSE);
			}
			return FALSE;

		/* �J�[�\���ݒ� */
		case WM_SETCURSOR:
			if (bAboutURLHit) {
				/* OS�̃o�[�W�����ɂ���Ă�IDC_HAND�͎��s���� */
				hCursor = LoadCursor(NULL, MAKEINTRESOURCE(32649));
				if (!hCursor) {
					hCursor = LoadCursor(NULL, IDC_IBEAM);
				}
				SetCursor(hCursor);

				/* �}�E�X���������΁AURL���s */
				if ((HIWORD(lParam) == WM_LBUTTONDOWN) ||
					(HIWORD(lParam) == WM_LBUTTONDBLCLK)) {
					ShellExecute(hDlg, NULL, pszAboutURL, NULL, NULL, SW_SHOWNORMAL);
				}
				return TRUE;
			}
	}

	/* ����ȊO��FALSE */
	return FALSE;
}

/*
 *	�o�[�W�������
 */
void FASTCALL OnAbout(HWND hWnd)
{
	ASSERT(hWnd);

	/* ���[�_���_�C�A���O���s */
	DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_ABOUTDLG), hWnd, AboutDlgProc);
}

#endif	/* _WIN32 */

/*
 *	FM-7 EMULATOR "XM7"
 *
 *	Copyright (C) 1999-2001 �o�h�D(ytanaka@ipc-tokai.or.jp)
 *	[ Win32API ���C���v���O���� ]
 */

#ifdef _WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <imm.h>
#include <shellapi.h>
#include <assert.h>
#include "xm7.h"
#include "w32.h"
#include "w32_bar.h"
#include "w32_draw.h"
#include "w32_dd.h"
#include "w32_kbd.h"
#include "w32_sch.h"
#include "w32_snd.h"
#include "w32_res.h"
#include "w32_sub.h"
#include "w32_cfg.h"

/*
 *	�O���[�o�� ���[�N
 */
HINSTANCE hAppInstance;					/* �A�v���P�[�V���� �C���X�^���X */
HWND hMainWnd;							/* ���C���E�C���h�E */
HWND hDrawWnd;							/* �h���[�E�C���h�E */
int nErrorCode;							/* �G���[�R�[�h */
BOOL bMenuLoop;							/* ���j���[���[�v�� */
BOOL bCloseReq;							/* �I���v���t���O */
LONG lCharWidth;						/* �L�����N�^���� */
LONG lCharHeight;						/* �L�����N�^�c�� */
BOOL bSync;								/* ���s�ɓ��� */
BOOL bActivate;							/* �A�N�e�B�x�[�g�t���O */

/*
 *	�X�^�e�B�b�N ���[�N
 */
static CRITICAL_SECTION CSection;		/* �N���e�B�J���Z�N�V���� */

/*-[ �T�u�E�B���h�E�T�|�[�g ]-----------------------------------------------*/

/*
 *	�e�L�X�g�t�H���g�쐬
 *	���Ăяo������DeleteObject���邱��
 */
HFONT FASTCALL CreateTextFont(void)
{
	HFONT hFont;
	LANGID lang;

	/* �f�t�H���g������擾 */
	lang = GetUserDefaultLangID();

	/* ���ꔻ�肵�āA�V�t�gJIS�EANSI�ǂ��炩�̃t�H���g����� */
	if ((lang & 0xff) == 0x11) {
		/* ���{�� */
		hFont = CreateFont(-16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, FIXED_PITCH, NULL);
	}
	else {
		/* �p�� */
		hFont = CreateFont(-16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
			ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, FIXED_PITCH, NULL);
	}

	ASSERT(hFont);
	return hFont;
}

/*
 *	�T�u�E�B���h�E�Z�b�g�A�b�v
 */
static void FASTCALL SetupSubWnd(HWND hWnd)
{
	HFONT hFont;
	HFONT hBackup;
	HDC hDC;
	TEXTMETRIC tm;

	ASSERT(hWnd);

	/* �E�C���h�E���[�N���N���A */
	memset(hSubWnd, 0, sizeof(hSubWnd));

	/* �t�H���g�쐬 */
	hFont = CreateTextFont();
	ASSERT(hFont);

	/* DC�擾�A�t�H���g�Z���N�g */
	hDC = GetDC(hWnd);
	ASSERT(hDC);
	hBackup = SelectObject(hDC, hFont);
	ASSERT(hBackup);

	/* �e�L�X�g���g���b�N�擾 */
	GetTextMetrics(hDC, &tm);

	/* �t�H���g�ADC�N���[���A�b�v */
	SelectObject(hDC, hBackup);
	DeleteObject(hFont);
	ReleaseDC(hWnd, hDC);

	/* ���ʂ��X�g�A */
	lCharWidth = tm.tmAveCharWidth;
	lCharHeight = tm.tmHeight + tm.tmExternalLeading;
}

/*-[ ���� ]-----------------------------------------------------------------*/

/*
 *	VM�����b�N
 */
void FASTCALL LockVM(void)
{
	EnterCriticalSection(&CSection);
}

/*
 *	VM���A�����b�N
 */
void FASTCALL UnlockVM(void)
{
	LeaveCriticalSection(&CSection);
}

/*-[ �h���[�E�C���h�E ]-----------------------------------------------------*/

/*
 * 	�E�C���h�E�v���V�[�W��
 */
static LRESULT CALLBACK DrawWndProc(HWND hWnd, UINT message,
								 WPARAM wParam,LPARAM lParam)
{
	/* ���b�Z�[�W�� */
	switch (message) {
		/* �E�C���h�E�w�i�`�� */
		case WM_ERASEBKGND:
			/* �G���[�Ȃ��Ȃ�A�w�i�`�悹�� */
			if (nErrorCode == 0) {
				return 0;
			}
			break;

		/* �E�C���h�E�ĕ`�� */
		case WM_PAINT:
			/* ���b�N���K�v */
			LockVM();
			OnPaint(hWnd);
			UnlockVM();
			return 0;
	}

	/* �f�t�H���g �E�C���h�E�v���V�[�W�� */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 *	�h���[�E�C���h�E�쐬
 */
static HWND FASTCALL CreateDraw(HWND hParent)
{
	WNDCLASSEX wcex;
	char szWndName[] = "XM7_Draw";
	RECT rect;
	HWND hWnd;

	ASSERT(hParent);

	/* �e�E�C���h�E�̋�`���擾 */
	GetClientRect(hParent, &rect);

	/* �E�C���h�E�N���X�̓o�^ */
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = DrawWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hAppInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWndName;
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&wcex);

	/* �E�C���h�E�쐬 */
	hWnd = CreateWindow(szWndName,
						szWndName,
						WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
						0,
						0,
						rect.right,
						rect.bottom,
						hParent,
						NULL,
						hAppInstance,
						NULL);

	/* ���ʂ������A�� */
	return hWnd;
}

/*-[ ���C���E�C���h�E ]-----------------------------------------------------*/

/*
 *	�E�C���h�E�쐬
 */
static void FASTCALL OnCreate(HWND hWnd)
{
	BOOL flag;

	ASSERT(hWnd);

	/* �N���e�B�J���Z�N�V�����쐬 */
	InitializeCriticalSection(&CSection);

	/* �h���[�E�C���h�E�A�X�e�[�^�X�o�[���쐬 */
	hDrawWnd = CreateDraw(hWnd);
	hStatusBar = CreateStatus(hWnd);

	/* IME���֎~���� */
	ImmAssociateContext(hWnd, (HIMC)NULL);

	/* �t�@�C���h���b�v���� */
	DragAcceptFiles(hWnd, TRUE);

	/* ���[�N�G���A������ */
	nErrorCode = 0;
	bMenuLoop = FALSE;
	bCloseReq = FALSE;
	bSync = TRUE;
	bActivate = FALSE;

	/* �T�u�E�B���h�E���� */
	SetupSubWnd(hWnd);

	/* �R���|�[�l���g������ */
	LoadCfg();
	InitDraw();
	InitSnd();
	InitKbd();
	InitSch();

	/* ���z�}�V�������� */
	if (!system_init()) {
		nErrorCode = 1;
		PostMessage(hWnd, WM_USER, 0, 0);
		return;
	}
	/* ����A���Z�b�g */
	ApplyCfg();
	system_reset();

	/* �R���|�[�l���g�Z���N�g */
	flag = TRUE;
	if (!SelectDraw(hDrawWnd)) {
		flag = FALSE;
	}
	if (!SelectSnd(hWnd)) {
		flag = FALSE;
	}
	if (!SelectKbd(hWnd)) {
		flag = FALSE;
	}
	if (!SelectSch()) {
		flag = FALSE;
	}

	/* �G���[�R�[�h���Z�b�g�����A�X�^�[�g */
	if (!flag) {
		nErrorCode = 2;
	}
	PostMessage(hWnd, WM_USER, 0, 0);
}

/*
 *	�E�C���h�E�N���[�Y
 */
static void FASTCALL OnClose(HWND hWnd)
{
	ASSERT(hWnd);

	/* ���C���E�C���h�E����x����(�^�X�N�o�[�΍�) */
	ShowWindow(hWnd, SW_HIDE);

	/* �t���O�A�b�v */
	LockVM();
	bCloseReq = TRUE;
	UnlockVM();

	/* DestroyWindow */
	DestroyWindow(hWnd);
}

/*
 *	�E�C���h�E�폜
 */
static void FASTCALL OnDestroy(void)
{
	/* �R���|�[�l���g �N���[���A�b�v */
	SaveCfg();
	CleanSch();
	CleanKbd();
	CleanSnd();
	CleanDraw();

	/* �N���e�B�J���Z�N�V�����폜 */
	DeleteCriticalSection(&CSection);

	/* ���z�}�V�� �N���[���A�b�v */
	system_cleanup();

	/* �E�C���h�E�n���h�����N���A */
	hMainWnd = NULL;
	hDrawWnd = NULL;
	hStatusBar = NULL;
}

/*
 *	�T�C�Y�ύX
 */
static void FASTCALL OnSize(HWND hWnd, WORD cx, WORD cy)
{
	RECT crect;
	RECT wrect;
	RECT trect;
	RECT srect;
	int height;

	ASSERT(hWnd);
	ASSERT(hDrawWnd);

	/* �ŏ����̏ꍇ�́A�������Ȃ� */
	if ((cx == 0) && (cy == 0)) {
		return;
	}

	/* �t���X�N���[���������l */
	if (bFullScreen) {
		return;
	}

	/* �c�[���o�[�A�X�e�[�^�X�o�[�̗L�����l���ɓ���A�v�Z */
	height = 400;
	memset(&trect, 0, sizeof(trect));
	if (IsWindowVisible(hStatusBar)) {
		GetWindowRect(hStatusBar, &srect);
		height += (srect.bottom - srect.top);
	}
	else {
		memset(&srect, 0, sizeof(srect));
	}

	/* �N���C�A���g�̈�̃T�C�Y���擾 */
	GetClientRect(hWnd, &crect);

	/* �v���T�C�Y�Ɣ�r���A�␳ */
	if ((crect.right != 640) || (crect.bottom != height)) {
		GetWindowRect(hWnd, &wrect);
		wrect.right -= wrect.left;
		wrect.bottom -= wrect.top;
		wrect.right -= crect.right;
		wrect.bottom -= crect.bottom;
		wrect.right += 640;
		wrect.bottom += height;
		MoveWindow(hWnd, wrect.left, wrect.top, wrect.right, wrect.bottom, TRUE);
	}

	/* ���C���E�C���h�E�z�u */
	MoveWindow(hDrawWnd, 0, trect.bottom, 640, 400, TRUE);

	/* �X�e�[�^�X�o�[�z�u */
	if (IsWindowVisible(hStatusBar)) {
		MoveWindow(hStatusBar, 0, trect.bottom + 400, 640,
					(srect.bottom - srect.top), TRUE);
		SizeStatus(640);
	}
}

/*
 *	�L�b�N(���j���[�ɂ͖���`)
 */
static void FASTCALL OnKick(HWND hWnd)
{
	char buffer[128];

	buffer[0] = '\0';

	/* �G���[�R�[�h�� */
	switch (nErrorCode) {
		/* �G���[�Ȃ� */
		case 0:
			/* ���s�J�n */
			stopreq_flag = FALSE;
			run_flag = TRUE;

			/* �R�}���h���C������ */
			OnCmdLine(GetCommandLine());
			break;

		/* VM�������G���[ */
		case 1:
			LoadString(hAppInstance, IDS_VMERROR, buffer, sizeof(buffer));
			MessageBox(hWnd, buffer, "XM7", MB_ICONSTOP | MB_OK);
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		/* �R���|�[�l���g�������G���[ */
		case 2:
			LoadString(hAppInstance, IDS_COMERROR, buffer, sizeof(buffer));
			MessageBox(hWnd, buffer, "XM7", MB_ICONSTOP | MB_OK);
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;
	}
}

/*
 * 	�E�C���h�E�v���V�[�W��
 */
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
								 WPARAM wParam,LPARAM lParam)
{
	HDC hDC;
	PAINTSTRUCT ps;

	/* ���b�Z�[�W�� */
	switch (message) {
		/* �E�C���h�E�쐬 */
		case WM_CREATE:
			OnCreate(hWnd);
			break;

		/* �E�C���h�E�N���[�Y */
		case WM_CLOSE:
			OnClose(hWnd);
			break;

		/* �E�C���h�E�폜 */
		case WM_DESTROY:
			OnDestroy();
			PostQuitMessage(0);
			return 0;

		/* �E�C���h�E�T�C�Y�ύX */
		case WM_SIZE:
			OnSize(hWnd, LOWORD(lParam), HIWORD(wParam));
			break;

		/* �E�C���h�E�ĕ`�� */
		case WM_PAINT:
			/* ���b�N���K�v */
			LockVM();
			hDC = BeginPaint(hWnd, &ps);
			PaintStatus();
			EndPaint(hWnd, &ps);
			UnlockVM();
			return 0;

		/* ���[�U+0:�X�^�[�g */
		case WM_USER + 0:
			OnKick(hWnd);
			return 0;

		/* ���[�U+1:�R�}���h���C�� */
		case WM_USER + 1:
			OnCmdLine((LPTSTR)wParam);
			break;

		/* ���j���[���[�v�J�n */
		case WM_ENTERMENULOOP:
			bMenuLoop = TRUE;
			EnterMenuDD(hWnd);
			break;

		/* ���j���[�I�� */
		case WM_MENUSELECT:
			OnMenuSelect(wParam);
			break;

		/* ���j���[���[�v�I�� */
		case WM_EXITMENULOOP:
			ExitMenuDD();
			OnExitMenuLoop();
			bMenuLoop = FALSE;
			break;

		/* ���j���[�|�b�v�A�b�v */
		case WM_INITMENUPOPUP:
			if (!HIWORD(lParam)) {
				OnMenuPopup(hWnd, (HMENU)wParam, (UINT)LOWORD(lParam));
				return 0;
			}
			break;

		/* ���j���[�R�}���h */
		case WM_COMMAND:
			EnterMenuDD(hWnd);
			OnCommand(hWnd, LOWORD(wParam));
			ExitMenuDD();
			return 0;

		/* �t�@�C���h���b�v */
		case WM_DROPFILES:
			OnDropFiles((HANDLE)wParam);
			return 0;

		/* �A�N�e�B�x�[�g */
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE) {
				bActivate = FALSE;
			}
			else {
				bActivate = TRUE;
			}
			break;

		/* �I�[�i�[�h���[ */
		case WM_DRAWITEM:
			if (wParam == ID_STATUS_BAR) {
				OwnerDrawStatus((LPDRAWITEMSTRUCT)lParam);
				return TRUE;
			}
			break;
	}

	/* �f�t�H���g �E�C���h�E�v���V�[�W�� */
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*-[ �A�v���P�[�V���� ]-----------------------------------------------------*/

/*
 *	XM7�E�C���h�E�������A�R�}���h���C���n��
 */
static BOOL FASTCALL FindXM7Wnd(void)
{
	HWND hWnd;
	char string[128];

	/* �E�C���h�E�N���X�Ō��� */
	hWnd = FindWindow("XM7", NULL);
	if (!hWnd) {
		return FALSE;
	}

	/* �e�L�X�g������擾 */
	GetWindowText(hWnd, string, sizeof(string));
	string[5] = '\0';
	if (strcmp("XM7 [", string) != 0) {
		return FALSE;
	}

	/* ���b�Z�[�W�𑗐M */
	SendMessage(hWnd, WM_USER + 1, (WPARAM)GetCommandLine(), (LPARAM)NULL);
	return TRUE;
}

/*
 *	�C���X�^���X������
 */
static HWND FASTCALL InitInstance(HINSTANCE hInst, int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wcex;
	char szAppName[] = "XM7";

	ASSERT(hInst);

	/* �E�C���h�E�n���h���N���A */
	hMainWnd = NULL;
	hDrawWnd = NULL;
	hStatusBar = NULL;

	/* �E�C���h�E�N���X�̓o�^ */
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APPICON));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
	wcex.lpszClassName = szAppName;
	wcex.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APPICON));
	RegisterClassEx(&wcex);

	/* �E�C���h�E�쐬 */
	hWnd = CreateWindow(szAppName,
						szAppName,
						WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_BORDER |
						WS_CLIPCHILDREN | WS_MINIMIZEBOX,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						NULL,
						NULL,
						hInst,
						NULL);
	if (!hWnd) {
		return NULL;
	}

	/* �E�C���h�E�\�� */
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}

/*
 *	WinMain
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
						LPSTR lpszCmdParam, int nCmdShow)
{
	MSG msg;
	HACCEL hAccel;

	/* XM7�`�F�b�N�A�R�}���h���C���n�� */
	if (FindXM7Wnd()) {
		return 0;
	}

	/* �C���X�^���X��ۑ��A������ */
	hAppInstance = hInstance;
	hMainWnd = InitInstance(hInstance, nCmdShow);
	if (!hMainWnd) {
		return FALSE;
	}

	/* �A�N�Z�����[�^�����[�h */
	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));
	ASSERT(hAccel);

	/* ���b�Z�[�W ���[�v */
	for (;;) {
		/* ���b�Z�[�W���`�F�b�N���A���� */
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}
			if (!TranslateAccelerator(hMainWnd, hAccel, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			continue;
		}

		/* �X�e�[�^�X�`�恕�X���[�v */
		if (nErrorCode == 0) {
			DrawStatus();
		}
		if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			Sleep(20);
		}
	}

	/* �I�� */
	return msg.wParam;
}

#endif	/* _WIN32 */

// Gemstone.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Gemstone.h"
#include "ImgCvt.h"
#include <commdlg.h>
#include <io.h>
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>

using namespace cv;
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


vector<FONT> fonts;
FONT	curFont;
char	renderText[256];
HWND	hMainDlg = NULL;
BYTE	*matrix = NULL;
long	*intImg = NULL;
vector<RECT>	mainFeat;

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_GEMSTONE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DLG_MAIN), NULL, (DLGPROC)WndProc);
}

bool FileExists(const char *filePathPtr)
{
	char filePath[_MAX_PATH];

	// Strip quotation marks (if any)
	if (filePathPtr[0] == '"')
	{
		strcpy(filePath, filePathPtr + 1);
	}
	else
	{
		strcpy(filePath, filePathPtr);
	}

	// Strip quotation marks (if any)
	if (filePath[strlen(filePath) - 1] == '"')
		filePath[strlen(filePath) - 1] = 0;

	return (_access(filePath, 0) != -1);
}

void ClearBuffer() {
	if (matrix) free(matrix);
	if (intImg) free(intImg);
	matrix = NULL;
	intImg = NULL;
}

void IntegralImage(BYTE *src, long *integral, int width, int height)
{
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			long a, b, c;

			if (j > 0) a = integral[(j - 1) * width + i]; else a = 0;
			if (i > 0) b = integral[j * width + (i - 1)]; else b = 0;
			if (i > 0 && j > 0) c = integral[(j - 1) * width + (i - 1)]; else c = 0;

			a = a + b + src[j * width + i] - c;
			integral[j * width + i] = a;
		}
	}
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GEMSTONE));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_GEMSTONE);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

void ToSBCS(LPWSTR lpszText)
{
	int j = wcslen(lpszText);
	if (j == 0)
	{
		strcpy((char *)lpszText, "");
		return;
	}
	else
	{
		char *lpszNewText = (char *)malloc(j + 1);
		j = WideCharToMultiByte(CP_ACP, 0L, lpszText, -1, lpszNewText, j + 1, NULL, NULL);
		if (j > 0)
			strcpy((char *)lpszText, lpszNewText);
		else
			strcpy((char *)lpszText, "");
		free(lpszNewText);
	}
}

void ShowRenderResult(HWND hWnd, int ID) {
	HDC hdc = GetDC(GetDlgItem(hWnd, ID));
	HDC hdcMem = CreateCompatibleDC(hdc);
	HGDIOBJ oldBitmap;
	HBITMAP hBmp = CreateBitmap(RESIZED_WIDTH, RESIZED_HEIGHT, 1, 32, renderImg);
	RECT rect;
	GetClientRect(GetDlgItem(hWnd, ID), &rect);

	SetStretchBltMode(hdc, HALFTONE);

	oldBitmap = SelectObject(hdcMem, hBmp);

	StretchBlt(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hdcMem, 0, 0, RESIZED_WIDTH, RESIZED_HEIGHT, SRCCOPY);

	SelectObject(hdcMem, oldBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(hWnd, hdc);
	DeleteDC(hdc);
	DeleteObject(hBmp);
}

vector<BYTE> ToPixels(HBITMAP hBitmap, int &width, int &height)
{
	BITMAP Bmp = { 0 };
	BITMAPINFO Info = { 0 };
	std::vector<BYTE> Pixels = std::vector<BYTE>();
	HDC DC = CreateCompatibleDC(NULL);
	HBITMAP OldBitmap = (HBITMAP)SelectObject(DC, hBitmap);

	std::memset(&Info, 0, sizeof(BITMAPINFO)); //not necessary really..

	GetObject(hBitmap, sizeof(Bmp), &Bmp);

	Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	Info.bmiHeader.biWidth = width = Bmp.bmWidth;
	Info.bmiHeader.biHeight = height = Bmp.bmHeight;
	Info.bmiHeader.biPlanes = 1;
	Info.bmiHeader.biBitCount = Bmp.bmBitsPixel;
	Info.bmiHeader.biCompression = BI_RGB;
	Info.bmiHeader.biSizeImage = ((width * Bmp.bmBitsPixel + 31) / 32) * 4 * height;

	Pixels.resize(Info.bmiHeader.biSizeImage);
	GetDIBits(DC, hBitmap, 0, height, &Pixels[0], &Info, DIB_RGB_COLORS);
	SelectObject(DC, OldBitmap);

	height = std::abs(height);
	DeleteDC(DC);
	return Pixels;
}

HBITMAP GenerateBitmap(std::string text) {
	int width = 1000 * text.size();
	int height = 1000;

	HDC		hdc = GetDC(NULL);
	HGDIOBJ previousSelectedHandle = NULL;
	HFONT	hFont = NULL;
	HFONT	previousFont = NULL;
	HBITMAP cropped = NULL;
	HDC		compatibleDeviceContext = NULL;
	HBITMAP bitmapHandle = NULL; 
	RECT	rect = RECT();

	rect.bottom = 0; rect.right = width; rect.top = 0; rect.bottom = height;
	compatibleDeviceContext = CreateCompatibleDC(hdc);
	bitmapHandle = CreateCompatibleBitmap(hdc, width, height);

	if (bitmapHandle == NULL) goto _final;
	previousSelectedHandle = SelectObject(compatibleDeviceContext, bitmapHandle);

	hFont = CreateFont(1000, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, TEXT(curFont.name));

	if (hFont == NULL) goto _final;
	
	SetTextColor(compatibleDeviceContext, RGB(255, 255, 255));
	SetBkMode(compatibleDeviceContext, TRANSPARENT);
	previousFont = (HFONT)SelectObject(compatibleDeviceContext, hFont);
	DrawText(compatibleDeviceContext, TEXT(text.c_str()), text.size(), &rect, DT_EDITCONTROL);
	//TextOut(compatibleDeviceContext, 0, 0, TEXT(text.c_str()), text.size());

	SelectObject(compatibleDeviceContext, previousFont);
	SelectObject(compatibleDeviceContext, previousSelectedHandle);

_final:
	if (compatibleDeviceContext) DeleteDC(compatibleDeviceContext);
	if (hFont) DeleteObject(hFont);
	if (hdc) DeleteDC(hdc);
	return bitmapHandle;
}

void ShowRenderText(HWND hWnd, int ID, int width, int height) {
	HDC hdc = GetDC(NULL);
	HDC hdcBmp = CreateCompatibleDC(hdc);
	HBITMAP hBmp = CreateCompatibleBitmap(hdc, width, height);

	HDC hdcClient = GetDC(GetDlgItem(hWnd, ID));
	HDC hdcMem = CreateCompatibleDC(hdcClient);
	HGDIOBJ oldBitmap = SelectObject(hdcBmp, hBmp);
	RECT rect;


	SelectObject(hdcBmp, GetStockObject(WHITE_BRUSH));
	//Rectangle(hdcBmp, 0, 0, width, height);
	for (int i = 0; i < mainFeat.size(); i++) {
		Rectangle(hdcBmp, mainFeat[i].left, mainFeat[i].top, mainFeat[i].right, mainFeat[i].bottom);
	}
	DeleteDC(hdc);
	DeleteDC(hdcBmp);

	oldBitmap = SelectObject(hdcMem, hBmp);
	GetClientRect(GetDlgItem(hWnd, ID), &rect);
	SetStretchBltMode(hdcClient, HALFTONE);

	StretchBlt(hdcClient, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hdcMem, 0, 0, width, height, SRCCOPY);
	
	SelectObject(hdcMem, oldBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(GetDlgItem(hWnd, ID), hdcClient);
	DeleteDC(hdcClient);
}

void ShowRenderText(HWND hWnd, int ID, HBITMAP hBmp) {
	BITMAP bm;
	HDC hdcClient = GetDC(GetDlgItem(hWnd, ID));
	HDC hdcMem = CreateCompatibleDC(hdcClient);
	HGDIOBJ oldBitmap = SelectObject(hdcMem, hBmp);
	RECT rect;

	GetObject(hBmp, sizeof(BITMAP), &bm);
	GetClientRect(GetDlgItem(hWnd, ID), &rect);

	SetStretchBltMode(hdcClient, HALFTONE);

	SelectObject(hdcMem, GetStockObject(GRAY_BRUSH));

	StretchBlt(hdcClient, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

	SelectObject(hdcMem, oldBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(GetDlgItem(hWnd, ID), hdcClient);
	DeleteDC(hdcClient);
}

void ShowResultImage(HWND hWnd, int ID) {
	//DrawRotationMark();
	HDC hdc = GetDC(GetDlgItem(hWnd, ID));
	HDC hdcMem = CreateCompatibleDC(hdc);
	HGDIOBJ oldBitmap;
	HBITMAP hBmp = CreateBitmap(RESIZED_WIDTH, RESIZED_HEIGHT, 1, 32, renderImg);
	RECT rect;
	GetClientRect(GetDlgItem(hWnd, ID), &rect);

	SetStretchBltMode(hdc, HALFTONE);

	oldBitmap = SelectObject(hdcMem, hBmp);

	StretchBlt(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hdcMem, 0, 0, RESIZED_WIDTH, RESIZED_HEIGHT, SRCCOPY);

	SelectObject(hdcMem, oldBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(hWnd, hdc);
	DeleteDC(hdc);
	DeleteObject(hBmp);
}

void coatMaterial() {
	for (int i = 0; i < RESIZED_HEIGHT; i++) {
		for (int j = 0; j < RESIZED_WIDTH; j++) {
			if (indexMap1[i * RESIZED_WIDTH + j] == -1 || indexMap2[i * RESIZED_WIDTH + j] == -1) {
				renderImg[i * RESIZED_WIDTH * 4 + j * 4] = 0;
				renderImg[i * RESIZED_WIDTH * 4 + j * 4 + 1] = 0;
				renderImg[i * RESIZED_WIDTH * 4 + j * 4 + 2] = 0;
				renderImg[i * RESIZED_WIDTH * 4 + j * 4 + 3] = 255;
			}
			else{
				VERTEX v1 = VERTEX(1, 1, 1);
				VERTEX v2 = VERTEX(vertMap[i * RESIZED_WIDTH + j].nor.val[0], vertMap[i * RESIZED_WIDTH + j].nor.val[1], vertMap[i * RESIZED_WIDTH + j].nor.val[2]);
				double m = doMagnitudeForVertex(&doCross(&v1, &v2));
				m = m * m * 70.0f;
				if (m > 255) m = 255;
				renderImg[i * RESIZED_WIDTH * 4 + j * 4] = m;
				renderImg[i * RESIZED_WIDTH * 4 + j * 4 + 1] = m;
				renderImg[i * RESIZED_WIDTH * 4 + j * 4 + 2] = m;
				renderImg[i * RESIZED_WIDTH * 4 + j * 4 + 3] = 255;
			}
		}
	}
}

void DoLowResolutionRender(HWND hWnd, int ID) {
	RESIZED_HEIGHT = 600;
	RESIZED_WIDTH = 600;
	ROI = 3;
	InitRenderBuffer(600, 600);
	doRender();
	normalizeDepth();
	coatMaterial();
	ShowResultImage(hWnd, ID);
	ReleaseRenderBuffer();
}

void ToThinner(Mat *frm) {
	Mat tmp = Mat::zeros(frm->rows, frm->cols, CV_8UC1);

	for (int i = 0; i < frm->rows; i++) {
		uchar *ptr = frm->ptr(i);
		uchar *ptrTmp = tmp.ptr(i);
		bool f = false;
		for (int j = 0; j < frm->cols; j++) {
			if (ptr[j] > 0 && f == false) {
				ptrTmp[j] = 1; 
				f = true;
			}
			else if (f == true && ptr[j] == 0) {
				f = false;
				ptrTmp[j - 1] = 0;
			}
		}
	}
	for (int j = 0; j < frm->cols; j++) {
		bool f = false;
		for (int i = 0; i < frm->rows; i++) {
			if (frm->at<uchar>(i, j) > 0 && !f) {
				tmp.at<uchar>(i, j) = 1;
				f = true;
			}
			else if (f && frm->at<uchar>(i, j) == 0) {
				f = false;
				tmp.at<uchar>(i - 1, j) = 1;
			}
		}
	}
	for (int i = 0; i < frm->rows; i++) {
		uchar *ptr = frm->ptr(i);
		uchar *ptrTmp = tmp.ptr(i);
		bool f = false;
		for (int j = 0; j < frm->cols; j++) {
			if (ptrTmp[j] > 0) ptr[j] = 0;
		}
	}
	tmp.release();
}

void GenerateModelFromImage(HWND hWnd, int ID, Mat *frm) {
	vector<BYTE> pixel;
	int		newW, newH;
	Mat		frmGray;
	int		threshold = 50;
	int		n = 0;
	int		quad = 1;
	mainFeat.clear();
	Mat		index = Mat::zeros(frm->rows, frm->cols, CV_32FC1);
	DWORD t = GetTickCount();
	SUBNODE subnode;

	cvtColor(*frm, frmGray, CV_BGR2GRAY);
	
	//blur(frmGray, frmGray, Size(5, 5));
	for (int i = 0; i < 3; i++) {
		ToThinner(&frmGray);
	}
	for (int i = 0; i < frmGray.rows; i++) {
		uchar *ptr = frmGray.ptr(i);
		bool f = false;
		for (int j = 0; j < frmGray.cols; j++) {
			if (ptr[j] > 0 && f == false) {
				ptr[j] = 0;
				f = true;
			}else if (f == true && ptr[j] == 0) {
				f = false;
				ptr[j - 1] = 0;
			}
		}
	}
	for (int j = 0; j < frmGray.cols; j++) {
		bool f = false;
		for (int i = 0; i < frmGray.rows; i++) {
			if (frmGray.at<uchar>(i, j) > 0 && !f) {
				frmGray.at<uchar>(i, j) = 0;
				f = true;
			}
			else if (f && frmGray.at<uchar>(i, j) == 0) {
				f = false;
				frmGray.at<uchar>(i, j - 1) = 0;
			}
		}
	}

	nodes.clear();
	for (int j = quad; j < frmGray.rows; j+=quad) {
		uchar *ptr1 = frmGray.ptr(j);
		uchar *ptr2 = frmGray.ptr(j - quad);
		float *idxPtr1 = (float *)index.ptr(j);
		float *idxPtr2 = (float *)index.ptr(j - quad);
		for (int i = quad; i < frmGray.cols; i+=quad) {
			if (ptr1[i] > threshold && ptr1[i - quad] > threshold &&
				ptr2[i] > threshold && ptr2[i - quad] > threshold) {
				float f = 3.0f;
				int idx[4];

				if (idxPtr2[i - quad] == 0) {
					subnode.vertex.push_back(VERTEX(i - quad + 0.5f, j - quad + 0.5f, (float)(255 - ptr2[i - quad]) / f));
					n++;
					idx[0] = n - 1;
					idxPtr2[i - quad] = n;
				}
				else{
					idx[0] = idxPtr2[i - quad] - 1;
				}
				if (idxPtr2[i] == 0) {
					subnode.vertex.push_back(VERTEX(i + 0.5f, j - quad + 0.5f, (float)(255 - ptr2[i]) / f));
					n++;
					idx[1] = n - 1;
					idxPtr2[i] = n;
				}
				else{
					idx[1] = idxPtr2[i] - 1;
				}
				if (idxPtr1[i] == 0) {
					subnode.vertex.push_back(VERTEX(i + 0.5f, j + 0.5f, (float)(255 - ptr1[i]) / f));
					n++;
					idx[2] = n - 1;
					idxPtr1[i] = n;
				}
				else{
					idx[2] = idxPtr1[i] - 1;
				}
				if (idxPtr1[i - quad] == 0) {
					subnode.vertex.push_back(VERTEX(i - quad + 0.5f, j + 0.5f, (float)(255 - ptr1[i - quad]) / f));
					n++;
					idx[3] = n - 1;
					idxPtr1[i - quad] = n;
				}
				else{
					idx[3] = idxPtr1[i - quad] - 1;
				}
				subnode.index.push_back(TRIANGLE(idx[2], idx[1], idx[0]));
				subnode.index.push_back(TRIANGLE(idx[3], idx[2], idx[0]));
			}
		}
	}
	index.release();
	nodes.push_back(subnode);

	OptimizeMesh(false);
	getCoordinate();
	CopyMesh();
	CorrectView();
	x_Rotate = 0.0f; y_Rotate = 0.0f; z_Rotate = 0.0f;
	DoLowResolutionRender(hWnd, ID);

	t = GetTickCount() - t;
	char buf[256];
	sprintf(buf, "Gemstone Model Generator: %d ms", t);
	SetWindowText(hWnd, buf);

	//ShowRenderText(hMainDlg, IDC_RENDER, width, height);
	//ShowRenderText(hMainDlg, IDC_RENDER, hBmp);
	frmGray.release();
}

void GenerateModelFromText(HWND hWnd, int ID) {
	vector<BYTE> pixel;
	int		width, height;
	int		mnX, mxX, mnY, mxY;
	int		newW, newH;

	ClearBuffer();
	DWORD t = GetTickCount();
	memset(renderText, 0, 256);
	GetWindowText(GetDlgItem(hWnd, IDC_RENDERTEXT), renderText, 256);
	HBITMAP hBmp = GenerateBitmap(string(renderText));
	pixel = ToPixels(hBmp, width, height);

	mxX = 0; mxY = 0; mnX = width; mnY = height;

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			if (pixel[j * width * 4 + i * 4] > 0) {
				if (i > mxX) mxX = i;
				if (i < mnX) mnX = i;
				if (j > mxY) mxY = j;
				if (j < mnY) mnY = j;
			}
		}
	}

	if (mxX < mnX || mxY < mnY) return;

	newW = mxX - mnX + 1;
	newH = mxY - mnY + 1;

	matrix = (BYTE *)malloc(newW * newH);
	intImg = (long *)malloc(newW * newH * sizeof(long));
	memset(matrix, 0, newW * newH);
	memset(intImg, 0, newW * newH * sizeof(long));
	for (int i = mnX; i <= mxX; i++) {
		for (int j = mnY; j <= mxY; j++) {
			if (pixel[j * width * 4 + i * 4] > 0) {
				matrix[(newH - (j - mnY) - 1) * newW + (i - mnX)] = 1;
			}
		}
	}

	mainFeat.clear();
	width = newW; height = newH;
	IntegralImage(matrix, intImg, width, height);
	for (int n = 30; n >= 2; n-=2) {
		for (int j = 0; j < height; j+=1) {
			for (int i = 0; i < width; i+=1) {
				int x1 = i - (n / 2);
				int y1 = j - (n / 2);
				int x2 = i + (n / 2);
				int y2 = j + (n / 2);
				bool flag = false;

				if (x1 < 0) continue;
				if (y1 < 0) continue;
				if (x2 >= width) continue;
				if (y2 >= height) continue;

				long a = intImg[y1 * width + x1];
				long b = intImg[y1 * width + x2];
				long c = intImg[y2 * width + x1];
				long d = intImg[y2 * width + x2];
				a = a + d - b - c;

				if (a == n * n) {
					for (int x = x1 + 1; x <= x2; x++) {
						for (int y = y1 + 1; y <= y2; y++) {
							if (matrix[y * width + x] == 0) {
								flag = true;
								break;
							}
						}
						if (flag) break;
					}
					if (flag) continue;
					RECT r;
					r.left = x1; r.right = x2; r.top = y1; r.bottom = y2;
					mainFeat.push_back(r);
					for (int x = x1 + 1; x <= x2; x++) {
						for (int y = y1 + 1; y <= y2; y++) {
							matrix[y * width + x] = 0;
						}
					}
				}
			}
		}
	}

	SUBNODE subnode;
	int n = 0;
	nodes.clear();
	x_Rotate = 0; y_Rotate = 0.7f; z_Rotate = 0;
	for (int i = 0; i < mainFeat.size(); i++) {
		subnode.vertex.push_back(VERTEX(mainFeat[i].left - 0.5f, mainFeat[i].top - 0.5f, 0));
		subnode.vertex.push_back(VERTEX(mainFeat[i].right + 0.5f, mainFeat[i].top - 0.5f, 0));
		subnode.vertex.push_back(VERTEX(mainFeat[i].right + 0.5f, mainFeat[i].bottom + 0.5f, 0));
		subnode.vertex.push_back(VERTEX(mainFeat[i].left - 0.5f, mainFeat[i].bottom + 0.5f, 0));
		n += 4;
		subnode.index.push_back(TRIANGLE(n - 2, n - 3, n - 4));
		subnode.index.push_back(TRIANGLE(n - 1, n - 2, n - 4));
	}
	nodes.push_back(subnode);

	OptimizeMesh(false);
	getCoordinate();
	CopyMesh();
	CorrectView();

	DoLowResolutionRender(hWnd, ID);

	t = GetTickCount() - t;
	char buf[256];
	sprintf(buf, "Gemstone Model Generator: %d ms", t);
	SetWindowText(hWnd, buf);

	//ShowRenderText(hMainDlg, IDC_RENDER, width, height);
	//ShowRenderText(hMainDlg, IDC_RENDER, hBmp);
	DeleteObject(hBmp);
	ClearBuffer();
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	static int x, y, bSel;

	switch (message)
	{
	case WM_MOUSEWHEEL:
		{
			x_Rotate = 0; y_Rotate = 0; z_Rotate = 0;
			zoomFactor -= GET_WHEEL_DELTA_WPARAM(wParam) / 120;
			if (zoomFactor < 1.0f) zoomFactor = 1.0f;
			DoLowResolutionRender(hWnd, IDC_RENDER);
		}
		break;
	case WM_LBUTTONDOWN:
		{
			x = LOWORD(lParam);
			y = HIWORD(lParam);
			bSel = true;
			
		}
		break;
	case WM_MOUSEMOVE:
		{
			float cx = LOWORD(lParam);
			float cy = HIWORD(lParam);

			if (!bSel) break;
			x_Rotate = (cy - y) / 300.0f; y_Rotate = (cx - x) / 300.0f;

			DoLowResolutionRender(hWnd, IDC_RENDER);
			x = cx; y = cy;
		}
		break;
	case WM_LBUTTONUP:
		x_Rotate = 0; y_Rotate = 0; z_Rotate = 0;
		bSel = false;
		break;
	case WM_INITDIALOG:
		{
			hMainDlg = hWnd;
			bSel = false;
			IDWriteFontCollection* pFontCollection = NULL;
			UINT32 familyCount = 0;

			IDWriteFactory *pDWriteFactory;
			HRESULT hr = DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(IDWriteFactory),
				reinterpret_cast<IUnknown**>(&pDWriteFactory)
				);
			// Get the system font collection.
			if (!SUCCEEDED(hr))
			{
				MessageBox(hWnd, "Error: DWriteCreateFactory", "Error", MB_ICONHAND | MB_OK);
				break;
			}
			
			hr = pDWriteFactory->GetSystemFontCollection(&pFontCollection);
			if (!SUCCEEDED(hr))
			{
				MessageBox(hWnd, "Error: GetSystemFontCollection", "Error", MB_ICONHAND | MB_OK);
				break;
			}
			familyCount = pFontCollection->GetFontFamilyCount();
			for (UINT32 i = 0; i < familyCount; ++i){
				IDWriteFontFamily* pFontFamily = NULL;
				IDWriteLocalizedStrings* pFamilyNames = NULL;
				UINT32 index = 0;
				BOOL exists = false;
				wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
				wchar_t name[LOCALE_NAME_MAX_LENGTH];
				UINT32 length = 0;

				hr = pFontCollection->GetFontFamily(i, &pFontFamily);
				if (!SUCCEEDED(hr))
				{
					continue;
				}
				hr = pFontFamily->GetFamilyNames(&pFamilyNames);
				if (!SUCCEEDED(hr))
				{
					continue;
				}

				int defaultLocaleSuccess = GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH);

				// If the default locale is returned, find that locale name, otherwise use "en-us".
				if (defaultLocaleSuccess)
				{
					hr = pFamilyNames->FindLocaleName(localeName, &index, &exists);
				}
				if (SUCCEEDED(hr) && !exists) // if the above find did not find a match, retry with US English
				{
					hr = pFamilyNames->FindLocaleName(L"en-us", &index, &exists);
				}
				if (!exists) index = 0;
				if (!SUCCEEDED(hr))
				{
					continue;
				}
				hr = pFamilyNames->GetStringLength(index, &length);
				
				if (!SUCCEEDED(hr) || name == NULL || length == 0) {
					continue;
				}
				hr = pFamilyNames->GetString(index, name, length + 1);
				
				ToSBCS(name);

				int nIndex = SendMessage(GetDlgItem(hWnd, IDC_CMB_FONT), CB_ADDSTRING, 0, (LPARAM)&name);
				//if (nIndex != CB_ERR)
				//	SendMessage(GetDlgItem(hWnd, IDC_CMB_FONT), CB_SETITEMDATA, nIndex, (LPARAM)nIndex);
				FONT f;
				strcpy(f.name, (char *)name);
				fonts.push_back(f);
			}
			if (fonts.size() > 0) {
				SendMessage(GetDlgItem(hWnd, IDC_CMB_FONT), CB_SETCURSEL, 0, 0);
				curFont = fonts[0];
				SetWindowText(GetDlgItem(hWnd, IDC_RENDERTEXT), "aaa");
			}
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDC_GENERATE_FROM_TEXT:
			GenerateModelFromText(hWnd, IDC_RENDER);
			break;
		case IDC_GENERATE_FROM_IMG:
			{
				char szFile[266];
				OPENFILENAME ofn;

				ZeroMemory(szFile, 256);
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = szFile;
				// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
				// use the contents of szFile to initialize itself.
				ofn.lpstrFile[0] = '\0';
				ofn.nMaxFile = sizeof(szFile);
				ofn.lpstrFilter = "png\0*.png\0bmp\0*.bmp\0jpg\0*.jpg";
				ofn.nFilterIndex = 1;
				ofn.lpstrFileTitle = NULL;
				ofn.nMaxFileTitle = 0;
				ofn.lpstrInitialDir = NULL;
				ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
				GetOpenFileName(&ofn);
				if (!FileExists(szFile)) break;
				Mat frm = imread(szFile);
				GenerateModelFromImage(hWnd, IDC_RENDER, &frm);
				frm.release();
			}
			break;
		case IDCANCEL:
			exit(0);
			break;
		case IDC_CMB_FONT:
			if (wmEvent == CBN_SELCHANGE) {
				int index = -1;

				index = SendMessage(GetDlgItem(hWnd, IDC_CMB_FONT), CB_GETCURSEL, 0, 0);
				if (index != -1) {
					curFont = fonts[index];
					GenerateModelFromText(hWnd, IDC_RENDER);
				}
			}
			break;
		}
		break;
	case WM_PAINT:
		{
		GenerateModelFromText(hWnd, IDC_RENDER);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

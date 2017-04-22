#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <tchar.h>
#include <strsafe.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <process.h>
#pragma comment(lib, "Ws2_32.lib")

using namespace std;
#define IDC_EDIT	101
#define IDC_BUTTON_SEND 102
#define IDC_LIST 104
#define IDC_EDIT_IP 105
#define IDC_RADIO1 106
#define IDC_RADIO2 107
#define IDC_EDIT_HOST	108
#define IDC_EDIT_PASS	109
#define IDC_EDIT_TO	110
#define IDC_EDIT_FROM	111
#define IDC_EDIT_SUBJ	112

#define HIGH2BIT_MASK (0xc0)
#define HIGH4BIT_MASK (0xf0)
#define HIGH6BIT_MASK (0xfc)
#define LOW2BIT_MASK  (0x03)
#define LOW4BIT_MASK  (0x0f)
#define LOW6BIT_MASK  (0x3f)
#define MSG_SIZE_IN_MB 5	
#define BUFFER_SIZE 10240	
#define SOCKET_READ_TIMEOUT_SEC 0.3;

#define MAX_DATA_SIZE 16

const char g_szClassName[] = "myWindowClass";
TCHAR szPass[50] = TEXT("");
TCHAR szTo[50] = TEXT("");
TCHAR szFrom[50] = TEXT("");
TCHAR szSubject[50] = TEXT("");
TCHAR szMessage[1024] = TEXT("");
TCHAR szServer[50] = TEXT("");
HWND hwnd;
HANDLE hMutex;
HANDLE hEvent;
HWND hWndIPAddress;
char RecvBuf[1024];
char codeBuf[3];
char *recvdData;
SOCKET Socket;
int recvdDataSize;
int recvdPackets;
BOOL firstReceived = TRUE;
#define Port 27015
struct hostent *hst;
char *host;

int base64_encode(char *, char*);
int sock; /*Socket */
SSL *ssl; /*Socket secured*/
SSL_CTX *ctx;
struct sockaddr_in server;
struct hostent *hp, *gethostbyname();
char buf[BUFSIZ + 1];
char FileBuffer[BUFSIZ + 1];
FILE* MailFilePtr;
int len;
char *host_id = "smtp.gmail.com"; //gmail
HANDLE hFileOut;
HWND hwndPB;
std::string base64_encode(unsigned char const*, unsigned int len);
std::string base64_decode(std::string const& s);
unsigned int WINAPI ThreadProc(LPVOID lpdwThreadParam);
int read_socket()
{
	int size_recv, total_size = 0;
	char chunk[BUFSIZ];
	char recvbuf[BUFSIZ] = "";
	while (1)
	{
		memset(chunk, 0, BUFSIZ);  //clear the variable
		if ((size_recv = recv(sock, chunk, BUFSIZ, 0)) < 0)
		{
			break;
		}
		else
		{
			total_size += size_recv;
			StringCchCat(recvbuf, BUFSIZ, chunk);
		}
	}
	cout << "S: " << recvbuf;
	cout << "\n";
	_tcscpy_s(RecvBuf, recvbuf);
	DWORD dwCount;
	if (!WriteFile(hFileOut, recvbuf, total_size, &dwCount, NULL))
	{
		MessageBox(hwnd, "Ошибка writing", "Error", MB_ICONEXCLAMATION | MB_OK);
	}
	return total_size;
}

int send_socket(char* buf)
{
	cout << "C: " << buf;
	cout << "\n";
	recvdData = buf;
	int all = strlen(buf);
	DWORD dwCount;
	if (!WriteFile(hFileOut, buf, all, &dwCount, NULL))
	{
		MessageBox(hwnd, "Ошибка writing", "Error", MB_ICONEXCLAMATION | MB_OK);
	}
	int sended = 0;
	while (all > 0)
	{
		sended = send(sock, buf, all, 0);
		if (sended < 0)
			return SOCKET_ERROR;
		buf += sended;
		all -= sended;
	}
	return all;
}

int send_SSLsocket(char *buf)
{
	cout << "C: " << buf;
	cout << "\n";
	recvdData = buf;
	int all = strlen(buf);
	DWORD dwCount;
	if (!WriteFile(hFileOut, buf, all, &dwCount, NULL))
	{
		MessageBox(hwnd, "Ошибка writing", "Error", MB_ICONEXCLAMATION | MB_OK);
	}
	int sended = 0;
	while (all > 0)
	{
		sended = SSL_write(ssl, buf, strlen(buf));
		if (sended < 0)
			return SOCKET_ERROR;
		buf += sended;
		all -= sended;
	}
	return all;
}

void read_SSLsocket()
{

	int size_recv, total_size = 0;
	char chunk[BUFSIZ];
	char recvbuf[BUFSIZ] = "";
	while (1)
	{
		memset(chunk, 0, BUFSIZ);  //clear the variable
		if ((size_recv = SSL_read(ssl, chunk, BUFSIZ)) < 0)
		{
			break;
		}
		else if (size_recv == 0) break;
		else
		{
			total_size += size_recv;
			StringCchCat(recvbuf, BUFSIZ, chunk);
		}
	}
	cout << "S: " << recvbuf;
	cout << "\n";
	DWORD dwCount;
	if (!WriteFile(hFileOut, recvbuf, total_size, &dwCount, NULL))
	{
		MessageBox(hwnd, "Ошибка writing", "Error", MB_ICONEXCLAMATION | MB_OK);
	}
	//recvdData = recvbuf;
	_tcscpy_s(RecvBuf, recvbuf);
	for (int i = 0; i < 3; i++)
		codeBuf[i] = RecvBuf[i];

}

unsigned int WINAPI ThreadProc(LPVOID lpdwThreadParam)
{
	GetDlgItemText(hwnd, IDC_EDIT_HOST, szServer, _countof(szServer));
	host_id = szServer;

	GetDlgItemText(hwnd, IDC_EDIT_FROM, szFrom, _countof(szFrom));
	char *from_id = "";
	from_id = szFrom;
	char from64[255] = "";
	base64_encode(from_id, from64);
	GetDlgItemText(hwnd, IDC_EDIT_PASS, szPass, _countof(szPass));
	char *password = "";
	password = szPass;
	char pass64[255];
	base64_encode(password, pass64);

	GetDlgItemText(hwnd, IDC_EDIT_TO, szTo, _countof(szTo));
	char *to_id = "bingabo@yahoo.fr";
	to_id = szTo;
	char to_id64[255] = "";
	base64_encode(to_id, to_id64);

	GetDlgItemText(hwnd, IDC_EDIT_SUBJ, szSubject, _countof(szSubject));
	_tcscat_s(szSubject, _countof(szSubject), "\r\n");
	char *subject = "very nice\r\n";
	subject = szSubject;

	GetDlgItemText(hwnd, IDC_EDIT, szMessage, _countof(szMessage));
	char *content;
	_tcscat_s(szMessage, _countof(szMessage), "\r\n");
	content = szMessage;

	if (inet_addr(host_id) != INADDR_NONE)
		server.sin_addr.s_addr = inet_addr(host_id);
	else
	{
		// попытка получить IP адрес по доменному имени сервера
		hp = gethostbyname(host_id);
		if (hp == (struct hostent *) 0)
		{
			MessageBox(NULL, RecvBuf, "Host not found", MB_ICONEXCLAMATION | MB_OK);
			goto closing;
		}

		cout << "Host ID: " << host_id << "\n";

		memcpy((char *)&(server.sin_addr), (char *)hp->h_addr, hp->h_length);
	}


	server.sin_family = AF_INET;
	server.sin_port = htons(25); /* SMTP PORT */
	if (connect(sock, (struct sockaddr *) &server, sizeof server) == -1)
	{
		MessageBox(NULL, "Can't connect to server", "error", MB_ICONEXCLAMATION | MB_OK);
		goto closing;

	}
	DWORD timeout = 1000 * SOCKET_READ_TIMEOUT_SEC;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

	UpdateWindow(hwnd);
	read_socket();
	send_socket("EHLO ");
	send_socket(host_id);
	send_socket("\r\n");
	read_socket();
	send_socket("STARTTLS\r\n");
	read_socket();
	SSL_set_fd(ssl, sock); //Associate socket to SLL
	SSL_connect(ssl);
	send_SSLsocket("EHLO ");
	send_SSLsocket(host_id);
	send_SSLsocket("\r\n");
	read_SSLsocket();
	send_SSLsocket("AUTH LOGIN\r\n");
	read_SSLsocket();
	send_SSLsocket(from64);
	send_SSLsocket("\r\n");
	read_SSLsocket();
	send_SSLsocket(pass64);
	send_SSLsocket("\r\n");
	read_SSLsocket();

	if (atoi(codeBuf) == 535)
	{
		MessageBox(NULL, RecvBuf, "Error!", MB_ICONEXCLAMATION | MB_OK);
		goto closing;

	}

	send_SSLsocket("MAIL FROM: <");
	send_SSLsocket(from64);
	send_SSLsocket(">\r\n");
	read_SSLsocket();
	if (atoi(codeBuf) == 501)
	{
		send_SSLsocket("MAIL FROM: <");
		send_SSLsocket(from_id);
		send_SSLsocket(">\r\n");
		read_SSLsocket(); // OK 
	}


	send_SSLsocket("RCPT TO: <");
	send_SSLsocket(to_id);
	send_SSLsocket(">\r\n");
	read_SSLsocket();

	if (atoi(codeBuf) == 550)
	{
		MessageBox(NULL, RecvBuf, "Error!", MB_ICONEXCLAMATION | MB_OK);
		goto closing;
	}

	send_SSLsocket("DATA\r\n");
	read_SSLsocket();
	send_SSLsocket("Subject: ");
	send_SSLsocket(subject);
	send_SSLsocket(content);
	send_SSLsocket(".");
	send_SSLsocket("\r\n");
	read_SSLsocket();
	if (atoi(codeBuf) == 250)
	{
		ShowWindow(hwndPB, SW_HIDE);
		SendMessage(hwndPB, (UINT)PBM_SETMARQUEE, (WPARAM)0, (LPARAM)NULL);
		SetWindowText(GetDlgItem(hwnd, 1004), "Message Sent");
	}
	else
	{
		read_SSLsocket();
		if (atoi(codeBuf) == 250)
		{
			SetWindowText(GetDlgItem(hwnd, 1004), "Message Sent");
			goto closing;
		}
	}
	send_SSLsocket("QUIT\r\n"); //bye
	read_SSLsocket();
closing:
	ShowWindow(hwndPB, SW_HIDE);
	Button_Enable(GetDlgItem(hwnd, IDC_BUTTON_SEND), 1);
	SendMessage(hwndPB, (UINT)PBM_SETMARQUEE, (WPARAM)0, (LPARAM)NULL);
	closesocket(sock);
	SSL_shutdown(ssl);

	SetEndOfFile(hFileOut);
	if (hFileOut)
	{
		CloseHandle(hFileOut);
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
					  HFONT hfDefault;
					  HWND hEdit, hEdit2;

					  hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"),
						  "hello pretty lady how are you today \r\nbonjour jolie dame comment allez-vous aujourd'hui ",
						  WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
						  5, 200, 405, 200, hwnd, (HMENU)IDC_EDIT, GetModuleHandle(NULL), NULL);

					  CreateWindowEx(0, TEXT("Static"), TEXT("Server"),
						  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 5, 65, 50, 25,
						  hwnd, (HMENU)1000, GetModuleHandle(NULL), NULL);

					  CreateWindowEx(0, TEXT("Static"), TEXT("From"),
						  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 5, 95, 50, 25,
						  hwnd, (HMENU)1001, GetModuleHandle(NULL), NULL);

					  CreateWindowEx(0, TEXT("Static"), TEXT("To"),
						  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 5, 125, 50, 25,
						  hwnd, (HMENU)1002, GetModuleHandle(NULL), NULL);

					  CreateWindowEx(0, TEXT("Static"), TEXT("Subject"),
						  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 5, 155, 50, 25,
						  hwnd, (HMENU)1003, GetModuleHandle(NULL), NULL);

					  CreateWindowEx(0, TEXT("Static"), TEXT(""),
						  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 205, 405, 130, 25,
						  hwnd, (HMENU)1004, GetModuleHandle(NULL), NULL);

					  CreateWindowEx(0, TEXT("Static"), TEXT("AGACIRO SMTP CLIENT"),
						  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 125, 25, 330, 25,
						  hwnd, (HMENU)1004, GetModuleHandle(NULL), NULL);

					  CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), "Hello",
						  WS_CHILD | WS_VISIBLE,
						  65, 150, 345, 25, hwnd, (HMENU)IDC_EDIT_SUBJ, GetModuleHandle(NULL), NULL);

					  CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), "example@yahoo.fr",
						  WS_CHILD | WS_VISIBLE,
						  65, 120, 345, 25, hwnd, (HMENU)IDC_EDIT_TO, GetModuleHandle(NULL), NULL);

					  CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), "example@gmail.com",
						  WS_CHILD | WS_VISIBLE,
						  65, 90, 195, 25, hwnd, (HMENU)IDC_EDIT_FROM, GetModuleHandle(NULL), NULL);

					  CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), "",
						  WS_CHILD | WS_VISIBLE,
						  260, 90, 150, 25, hwnd, (HMENU)IDC_EDIT_PASS, GetModuleHandle(NULL), NULL);

					  CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), "smtp.gmail.com",
						  WS_CHILD | WS_VISIBLE,
						  65, 60, 345, 25, hwnd, (HMENU)IDC_EDIT_HOST, GetModuleHandle(NULL), NULL);

					  CreateWindowEx(0, TEXT("Button"), TEXT("Send"),
						  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 5, 400, 130, 25,
						  hwnd, (HMENU)IDC_BUTTON_SEND, GetModuleHandle(NULL), NULL);


					  hwndPB = CreateWindowEx(
						  0, PROGRESS_CLASS, (LPCSTR)NULL,
						  WS_CHILD | WS_VISIBLE | PBS_MARQUEE,
						  5, 430, 405, 20,
						  hwnd, (HMENU)0, GetModuleHandle(NULL), NULL
						  );


					  hfDefault = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
					  SendMessage(hEdit, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
					  SendMessage(GetDlgItem(hwnd, IDC_EDIT_PASS), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
					  SendMessage(GetDlgItem(hwnd, IDC_EDIT_FROM), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
					  SendMessage(GetDlgItem(hwnd, IDC_EDIT_TO), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
					  SendMessage(GetDlgItem(hwnd, IDC_EDIT_HOST), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
					  SendMessage(GetDlgItem(hwnd, IDC_EDIT_SUBJ), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
					  SendMessage(GetDlgItem(hwnd, 1000), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
					  SendMessage(GetDlgItem(hwnd, 1001), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
					  SendMessage(GetDlgItem(hwnd, 1003), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
					  SendMessage(GetDlgItem(hwnd, 1002), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
					  SendMessage(GetDlgItem(hwnd, 1004), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
					  Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO2), 1);

					  LPARAM lpAdr = MAKEIPADDRESS(192, 168, 0, 104);
					  SendMessage(hWndIPAddress, IPM_SETADDRESS, 0, lpAdr);

					  HFONT hFont = CreateFont(24, 0, 0, 0, FW_DONTCARE, FALSE,
						  TRUE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
						  CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Impact"));

					  SendMessage(GetDlgItem(hwnd, IDC_EDIT_PASS), WM_SETFONT,
						  (WPARAM)hFont, MAKELPARAM(FALSE, 0));
					  Edit_SetPasswordChar(GetDlgItem(hwnd, IDC_EDIT_PASS), 149);

					  WSADATA wsadata = { 0 };
					  if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))
					  {
						  MessageBox(NULL, "Error WSAStartup!", "Error!", MB_ICONEXCLAMATION | MB_OK);
						  return(0);
					  }

					  SSL_library_init();
					  SSL_load_error_strings();

					  ctx = SSL_CTX_new(TLSv1_client_method());
					  ssl = SSL_new(ctx);
					  ShowWindow(hwndPB, SW_HIDE);
	}
		break;


	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_SEND:
		{
								SetWindowText(GetDlgItem(hwnd, 1004), "");
								sock = socket(AF_INET, SOCK_STREAM, 0);
								if (sock == -1)
								{
									MessageBox(NULL, "can't create socket!", "Error!", MB_ICONEXCLAMATION | MB_OK);
									return(0);
								}
								hFileOut = CreateFile("out.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, NULL, NULL);

								DWORD dwThreadId;
								HANDLE H = (HANDLE)_beginthreadex(NULL, NULL, ThreadProc, NULL, NULL, NULL);
								SendMessage(hwndPB, (UINT)PBM_SETMARQUEE, (WPARAM)1, (LPARAM)NULL);
								ShowWindow(hwndPB, SW_SHOW);
								Button_Enable(GetDlgItem(hwnd, IDC_BUTTON_SEND), 0);
		}

		}
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc;

	MSG Msg;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = g_szClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window Registration Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(
		0,
		g_szClassName,
		TEXT("SMTP Client"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 430, 495,
		NULL, NULL, hInstance, NULL);

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	MSG msg;
	BOOL bRet;
	for (;;)
	{
		while (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{

		}
		bRet = GetMessage(&msg, NULL, 0, 0);
		if (bRet == -1)
		{
			/* обработка ошибки и возможно выход из цикла */
		}
		else if (FALSE == bRet)
		{
			break;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	WSACleanup();
	return (int)msg.wParam;
}

char base64_index_table[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
	'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
	'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a',
	'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
	'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
	't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1',
	'2', '3', '4', '5', '6', '7', '8', '9', '+',
	'/'
};

int base64_encode(char *src, char *dst){
	int i = 0, len = 0;
	int index = 0;
	char buff[5] = { '\0' };
	int retval = 0;

	len = strlen(src);
	memset(dst, 0, sizeof(dst));

	while (i < len){
		if (i + 2 < len){
			index = (src[i] & HIGH6BIT_MASK) >> 2;
			buff[0] = base64_index_table[index];
			index = ((src[i] & LOW2BIT_MASK) << 4) + ((src[i + 1] & HIGH4BIT_MASK) >> 4);
			buff[1] = base64_index_table[index];
			index = ((src[i + 1] & LOW4BIT_MASK) << 2) + ((src[i + 2] & HIGH2BIT_MASK) >> 6);
			buff[2] = base64_index_table[index];
			index = src[i + 2] & LOW6BIT_MASK;
			buff[3] = base64_index_table[index];
		}
		else{
			switch (len - i){
			case 1:
				index = ((src[i] & HIGH6BIT_MASK) >> 2);
				buff[0] = base64_index_table[index];
				index = ((src[i] & LOW2BIT_MASK) << 4);
				buff[1] = base64_index_table[index];
				buff[2] = '=';
				buff[3] = '=';
				break;
			case 2:
				index = ((src[i] & HIGH6BIT_MASK) >> 2);
				buff[0] = base64_index_table[index];
				index = ((src[i] & LOW2BIT_MASK) << 4) + ((src[i + 1] & HIGH4BIT_MASK) >> 4);
				buff[1] = base64_index_table[index];
				index = ((src[i + 1] & LOW4BIT_MASK) << 2);
				buff[2] = base64_index_table[index];
				buff[3] = '=';
				break;
			default:
				retval = -1;
				return retval;
				break;
			}
		}
		strcat(dst, buff);
		i += 3;
	}
	return retval;
}

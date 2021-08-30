#include "control.h"
#include "utils.h"
#include "resource.h"

DWORD WINAPI ThreadVerifica_Tempo(LPVOID param) { //Remove avião da lista se passaram mais de 3 segundos desde seu ultimo ping no buffer circular
	DATA* dados = (DATA*)param;

	while (dados->terminar == 0) {

		WaitForSingleObject(dados->hMutex, INFINITE);

		verifica_temporizadorAviao(dados);
		verifica_temporizadorPassageiro(dados);

		ReleaseMutex(dados->hMutex);

		Sleep(1000);
	}

	return 0;
}

DWORD WINAPI ThreadActivaPipes(LPVOID param) {

	DATA* dados = (DATA*)param;
	Passageiro p;
	DWORD n = 0, offset, nBytes;
	int i;

	while (dados->terminar == 0 || n < N_PASSAGEIROS) {
		offset = WaitForMultipleObjects(N_PASSAGEIROS, dados->hEvents, FALSE, INFINITE);
		i = offset - WAIT_OBJECT_0;

		if (i >= 0 && i < N_PASSAGEIROS) { //se é um cliente válido (dentro do array)

			if (GetOverlappedResult(dados->hPipes[i].hPipe, &dados->hPipes[i].overlap, &nBytes, FALSE)) { //se entrada foi feita com sucesso
				ResetEvent(dados->hEvents[i]); //reset manual
				dados->hPipes[i].activo = TRUE;
			}

			if (ReadFile(dados->hPipes[i].hPipe, &p, sizeof(Passageiro), NULL, NULL)) {
				if (p.adicionado == -1) {
					if (verifica_nomeAeroportoPassageiro(dados->Aero_Controlo, dados->n_aero, p.aero_partida, p.aero_chegada)) { //verifica se aeroportos existem
						p.adicionado = adiciona_passageiro(dados->Pass_Controlo, dados->n_pass, p, i);
						n++;
					}
					else
					{
						p.adicionado = 0;
					}
				}
			}
			WriteFile(dados->hPipes[i].hPipe, &p, sizeof(Passageiro), NULL, NULL);
		}
	}

	for (i = 0; i < N_PASSAGEIROS; i++) {
		SetEvent(dados->hEvents[i]);
	}

	return 0;
}

DWORD WINAPI ThreadLeitura_MemPartilhada(LPVOID param) { //Avião -> Controlador
	DATA* dados = (DATA*)param;

	while (1) {
		WaitForSingleObject(dados->hEvent_A_C1, INFINITE);

		ResetEvent(dados->hEvent_A_C1);

		if (dados->terminar)
			break;

		actualiza_coordenadas(dados, dados->pM_Aviao_in);

		InvalidateRect(dados->hWnd, NULL, FALSE);
		
		SetEvent(dados->hEvent_A_C2);
		
	}

	return 0;
}

DWORD WINAPI ThreadLeitura_BufferCircular(LPVOID param) {
	DATA* dados = (DATA*)param;
	Aviao aviao;
	DWORD i;
	BOOL notDuplicado = TRUE;

	while (dados->terminar == 0) {

		WaitForSingleObject(dados->hSemLeitura, INFINITE);

		CopyMemory(&aviao, &dados->pB_Aviao->av[dados->pB_Aviao->posL], sizeof(Aviao));
		dados->pB_Aviao->posL++;
		if (dados->pB_Aviao->posL == 10) 
			dados->pB_Aviao->posL = 0;

		ReleaseSemaphore(dados->hSemEscrita, 1, NULL);

		if (aviao.adicionado == -1 && dados->aceita_avioes == 1) {//Se não ainda não foi adicionado, adicionar
			WaitForSingleObject(dados->hMutex, INFINITE);
			aviao.adicionado = adiciona_aviao(dados->Aero_Controlo, dados->n_aero, dados->Avi_Controlo, dados->n_avioes, aviao);
			if (aviao.adicionado) {
				for (i = 0; i < dados->n_aero; i++) {
					if (!(_tcsicmp(dados->Aero_Controlo[i].nome, aviao.aeroporto_partida))) {
						(dados->Aero_Controlo[i].cap_avioes)++;
					}
				}
			}
			ZeroMemory(dados->pM_Aviao_out, sizeof(Aviao));
			CopyMemory(dados->pM_Aviao_out, &aviao, sizeof(Aviao));
			ReleaseMutex(dados->hMutex);
			SetEvent(dados->hEvent_C_A);
		}
		if (aviao.adicionado == 1 && aviao.embarcar == -1) { //verificar se aeroporto de destino existe (quando embarca)
			WaitForSingleObject(dados->hMutex, INFINITE);
			if (verifica_embarqueEadiciona(dados->Aero_Controlo, dados->n_aero, aviao.aeroporto_chegada, dados->Avi_Controlo, dados->n_avioes, aviao)) {
				aviao.embarcar = 0;
				aviao.coordenadas = retorna_coordenadasAeroporto(dados->Aero_Controlo, dados->n_aero, aviao);
			}
			else {
				aviao.embarcar = -1;
			}
			ZeroMemory(dados->pM_Aviao_out, sizeof(Aviao));
			CopyMemory(dados->pM_Aviao_out, &aviao, sizeof(Aviao));
			ReleaseMutex(dados->hMutex);
			SetEvent(dados->hEvent_C_A);
		}
		if (aviao.adicionado == 1 && aviao.embarcar == 1) { //actualizar na lista que deu ordem de embarque
			WaitForSingleObject(dados->hMutex, INFINITE);
			actualiza_embarque(dados, aviao); //também guarda o num do pipe do passageiro
			aviao.embarcar = 2;
			ZeroMemory(dados->pM_Aviao_out, sizeof(Aviao));
			CopyMemory(dados->pM_Aviao_out, &aviao, sizeof(Aviao));
			ReleaseMutex(dados->hMutex);
			SetEvent(dados->hEvent_C_A);
		}
		if (aviao.adicionado == 1 && aviao.embarcar == 2 && aviao.voo == 1) { //actualizar na lista que está em voo
			WaitForSingleObject(dados->hMutex, INFINITE);
			actualiza_voo(dados->Avi_Controlo, dados->n_avioes, aviao, dados->Aero_Controlo, dados->n_aero);
			aviao.voo = 2;
			ZeroMemory(dados->pM_Aviao_out, sizeof(Aviao));
			CopyMemory(dados->pM_Aviao_out, &aviao, sizeof(Aviao));
			ReleaseMutex(dados->hMutex);
			SetEvent(dados->hEvent_C_A);
		}
		if (aviao.adicionado == 1 && aviao.voo == 0) { //reset avião
			WaitForSingleObject(dados->hMutex, INFINITE);
			desembarque_aviao(dados, aviao);
			InvalidateRect(dados->hWnd, NULL, FALSE);
			ReleaseMutex(dados->hMutex);
		}

		WaitForSingleObject(dados->hMutex, INFINITE);
		update_temporizador(dados->Avi_Controlo, dados->n_avioes, aviao);
		if (verifica_existencia_avioes(dados->Avi_Controlo, dados->n_avioes))
			ReleaseSemaphore(dados->hSemLeitura, 1, NULL);//dados->pB_Aviao->posL = 1;
		ReleaseMutex(dados->hMutex);

	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {

	DWORD i, tam = MAX;
	HANDLE hFileMapMemPartilhada_A_C, hThreadMemPartilhada_A_C, hFileMapMemPartilhada_C_A, hFileMapBufferCircular, hThreadBufferCircular, hThreadThreadVerificaTempo;

	HANDLE hPipe, hThreadAtendePassageiros, hEventTemporario;

	Aviao aviso;
	DATA dados;

	HDC hdc;
	HBITMAP hBmpAeroporto;
	HBITMAP hBmpAviao;
	HBITMAP hBmpMapa;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif


	dados.chave = NULL;
	dados.Pass_Controlo = malloc(sizeof(Passageiro) * N_PASSAGEIROS);
	dados.n_avioes = 0;
	dados.n_aero = 0;
	dados.n_pass = N_PASSAGEIROS;
	dados.aceita_avioes = 1;
	dados.terminar = 0;

	//MEM PART AVIAO ->CONTROLADOR
	hFileMapMemPartilhada_A_C = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(Mem_Aviao), TEXT("MEM_PARTILHADA_A_C"));
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		_ftprintf(stderr, TEXT("Uma instância do Processo CONTROLADOR já se encontra a decorrer!"));
		return 3;
	}
	if (hFileMapMemPartilhada_A_C == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateFileMapping.\n"));
		return 4;
	}

	dados.pM_Aviao_in = (Mem_Aviao*)MapViewOfFile(hFileMapMemPartilhada_A_C, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (dados.pM_Aviao_in == NULL) {
		_ftprintf(stderr, TEXT("Erro no MapViewOfFile.\n"));
		CloseHandle(hFileMapMemPartilhada_A_C);
		return 5;
	}

	dados.hEvent_A_C1 = CreateEvent(NULL, TRUE, FALSE, TEXT("EVENTO_A_C1"));
	if (dados.hEvent_A_C1 == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateEvent.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		fecha_handles(&dados);
		CloseHandle(hFileMapMemPartilhada_A_C);
		return 6;
	}

	dados.hEvent_A_C2 = CreateEvent(NULL, TRUE, FALSE, TEXT("EVENTO_A_C2"));
	if (dados.hEvent_A_C2 == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateEvent.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		fecha_handles(&dados);
		CloseHandle(hFileMapMemPartilhada_A_C);
		return 7;
	}

	dados.hMutex = CreateMutex(NULL, FALSE, TEXT("MUTEX_CONTROLADOR"));
	if (dados.hMutex == NULL) {
		_tprintf(TEXT("Erro no CreateMutex.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		fecha_handles(&dados);
		CloseHandle(hFileMapMemPartilhada_A_C);
		return 10;
	}

	hThreadMemPartilhada_A_C = CreateThread(NULL, 0, ThreadLeitura_MemPartilhada, &dados, 0, NULL);
	if (hThreadMemPartilhada_A_C == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateThread.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		fecha_handles(&dados);
		CloseHandle(hFileMapMemPartilhada_A_C);
		return 11;
	}
	//FIM MEM PART

	//MEM PART CONTROLADOR -> AVIAO
	hFileMapMemPartilhada_C_A = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(Aviao), TEXT("MEM_PARTILHADA_C_A"));
	if (hFileMapMemPartilhada_C_A == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateFileMapping.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		fecha_handles(&dados);
		CloseHandle(hFileMapMemPartilhada_A_C);
		return 12;
	}

	dados.hEvent_C_A = CreateEvent(NULL, TRUE, FALSE, TEXT("EVENTO_C_A"));
	if (dados.hEvent_C_A == NULL) {
		_tprintf(TEXT("Erro no CreateEvent\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		fecha_handles(&dados);
		CloseHandle(hFileMapMemPartilhada_A_C);
		CloseHandle(hFileMapMemPartilhada_C_A);
		return 13;
	}

	dados.pM_Aviao_out = (Aviao*)MapViewOfFile(hFileMapMemPartilhada_C_A, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (dados.pM_Aviao_out == NULL) {
		_ftprintf(stderr, TEXT("Erro no MapViewOfFile.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		fecha_handles(&dados);
		CloseHandle(hFileMapMemPartilhada_A_C);
		CloseHandle(hFileMapMemPartilhada_C_A);
		return 14;
	}
	//FIM MEM PART

	//BUFFER CIRCULAR
	dados.hSemEscrita = CreateSemaphore(NULL, TAM_BUFFER_C, TAM_BUFFER_C, TEXT("SEM_ESCRITA"));
	if (dados.hSemEscrita == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateSemaphore.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		UnmapViewOfFile(dados.pM_Aviao_out);
		fecha_handles(&dados);
		CloseHandle(hFileMapMemPartilhada_A_C);
		CloseHandle(hFileMapMemPartilhada_C_A);
		return 15;
	}

	dados.hSemLeitura = CreateSemaphore(NULL, 0, TAM_BUFFER_C, TEXT("SEM_LEITURA"));
	if (dados.hSemLeitura == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateSemaphore.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		UnmapViewOfFile(dados.pM_Aviao_out);
		fecha_handles(&dados);
		CloseHandle(hFileMapMemPartilhada_A_C);
		CloseHandle(hFileMapMemPartilhada_C_A);
		return 16;
	}

	hFileMapBufferCircular = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(Buffer_Aviao), TEXT("BUFFER"));
	if (hFileMapBufferCircular == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateFileMapping.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		UnmapViewOfFile(dados.pM_Aviao_out);
		fecha_handles(&dados);
		CloseHandle(hFileMapMemPartilhada_A_C);
		CloseHandle(hFileMapMemPartilhada_C_A);
		return 17;
	}

	dados.pB_Aviao = (Buffer_Aviao*)MapViewOfFile(hFileMapBufferCircular, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (dados.pB_Aviao == NULL) {
		_ftprintf(stderr, TEXT("Erro no MapViewOfFile.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		UnmapViewOfFile(dados.pM_Aviao_out);
		fecha_handles(&dados);
		CloseHandle(hFileMapMemPartilhada_A_C);
		CloseHandle(hFileMapMemPartilhada_C_A);
		CloseHandle(hFileMapBufferCircular);
		return 18;
	}

	dados.pB_Aviao->posL = 0;

	hThreadBufferCircular = CreateThread(NULL, 0, ThreadLeitura_BufferCircular, &dados, 0, NULL);
	if (hThreadBufferCircular == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateThread.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		UnmapViewOfFile(dados.pM_Aviao_out);
		UnmapViewOfFile(dados.pB_Aviao);
		fecha_handles(&dados);
		CloseHandle(hFileMapMemPartilhada_A_C);
		CloseHandle(hFileMapMemPartilhada_C_A);
		return 19;
	}

	//FIM BUFFER CIRCULAR

	//TEMPORIZADOR

	hThreadThreadVerificaTempo = CreateThread(NULL, 0, ThreadVerifica_Tempo, &dados, 0, NULL);
	if (hThreadThreadVerificaTempo == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateThread.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		UnmapViewOfFile(dados.pM_Aviao_out);
		UnmapViewOfFile(dados.pB_Aviao);
		fecha_handles(&dados);
		CloseHandle(hFileMapMemPartilhada_A_C);
		CloseHandle(hFileMapMemPartilhada_C_A);
		CloseHandle(hFileMapBufferCircular);
		return 20;
	}

	//FIM TEMPORIZADOR
	
	//NAMED PIPES

	for (i = 0; i < N_PASSAGEIROS; i++) {

		hPipe = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, N_PASSAGEIROS, sizeof(Passageiro), sizeof(Passageiro), 1000, NULL);

		if (hPipe == NULL) {
			_ftprintf(stderr, TEXT("Erro no CreateNamedPipe.\n"));
			UnmapViewOfFile(dados.pM_Aviao_in);
			UnmapViewOfFile(dados.pM_Aviao_out);
			UnmapViewOfFile(dados.pB_Aviao);
			fecha_handles(&dados);
			CloseHandle(hFileMapMemPartilhada_A_C);
			CloseHandle(hFileMapMemPartilhada_C_A);
			CloseHandle(hFileMapBufferCircular);
			return 21;

		}

		hEventTemporario = CreateEvent(NULL, TRUE, FALSE, NULL);

		if (hEventTemporario == NULL) {
			_ftprintf(stderr, TEXT("Erro no CreateEvent.\n"));
			UnmapViewOfFile(dados.pM_Aviao_in);
			UnmapViewOfFile(dados.pM_Aviao_out);
			UnmapViewOfFile(dados.pB_Aviao);
			fecha_handles(&dados);
			CloseHandle(hFileMapMemPartilhada_A_C);
			CloseHandle(hFileMapMemPartilhada_C_A);
			CloseHandle(hFileMapBufferCircular);
			return 22;
		}

		dados.hPipes[i].hPipe = hPipe;
		dados.hPipes[i].activo = FALSE;
		ZeroMemory(&dados.hPipes[i].overlap, sizeof(dados.hPipes[i].overlap));
		dados.hPipes[i].overlap.hEvent = hEventTemporario;
		dados.hEvents[i] = hEventTemporario;

		ConnectNamedPipe(hPipe, &dados.hPipes[i].overlap);
		if (!(GetLastError() == ERROR_IO_PENDING)) {
			_ftprintf(stderr, TEXT("Erro no ConnectNamedPipe.\n"));
			UnmapViewOfFile(dados.pM_Aviao_in);
			UnmapViewOfFile(dados.pM_Aviao_out);
			UnmapViewOfFile(dados.pB_Aviao);
			fecha_handles(&dados);
			CloseHandle(hFileMapMemPartilhada_A_C);
			CloseHandle(hFileMapMemPartilhada_C_A);
			CloseHandle(hFileMapBufferCircular);
			return 23;
		}
	}

	hThreadAtendePassageiros = CreateThread(NULL, 0, ThreadActivaPipes, &dados, 0, NULL);
	if (hThreadAtendePassageiros == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateThread.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		UnmapViewOfFile(dados.pM_Aviao_out);
		UnmapViewOfFile(dados.pB_Aviao);
		fecha_handles(&dados);
		CloseHandle(hFileMapMemPartilhada_A_C);
		CloseHandle(hFileMapMemPartilhada_C_A);
		CloseHandle(hFileMapBufferCircular);
		for (i = 0; i < N_PASSAGEIROS; i++)
			DisconnectNamedPipe(dados.hPipes[i].hPipe);
		return 24;
	}

	//FIM NAMED PIPES

	//GUI

	MSG lpMsg;
	WNDCLASSEX wcApp;
	wcApp.cbSize = sizeof(WNDCLASSEX);
	wcApp.hInstance = hInst;
	wcApp.lpszClassName = TEXT("Controlador");
	wcApp.lpfnWndProc = TrataEventos;
	wcApp.style = CS_HREDRAW | CS_VREDRAW;
	wcApp.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_PLANE));
	wcApp.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_PLANE));
	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcApp.lpszMenuName = MAKEINTRESOURCE(IDR_MENUCONTROL);
	wcApp.cbClsExtra = sizeof(DATA);
	wcApp.cbWndExtra = 0;
	wcApp.hbrBackground = CreateSolidBrush(RGB(160, 128, 64));

	if (!RegisterClassEx(&wcApp)) {
		_ftprintf(stderr, TEXT("Erro no RegisterClassEx.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		UnmapViewOfFile(dados.pM_Aviao_out);
		UnmapViewOfFile(dados.pB_Aviao);
		fecha_handles(&dados);
		CloseHandle(hFileMapMemPartilhada_A_C);
		CloseHandle(hFileMapMemPartilhada_C_A);
		CloseHandle(hFileMapBufferCircular);
		for (i = 0; i < N_PASSAGEIROS; i++)
			DisconnectNamedPipe(dados.hPipes[i].hPipe);
		return 25;
	}
		

	dados.hWnd = CreateWindow(TEXT("Controlador"), TEXT("Controlador"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1000, 1000, (HWND)HWND_DESKTOP, (HMENU)NULL, (HINSTANCE)hInst, 0);

	dados.memDC = NULL;

	hBmpAeroporto = (HBITMAP)LoadImage(NULL, TEXT("aero_50.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	GetObject(hBmpAeroporto, sizeof(dados.bmpAero), &dados.bmpAero);

	hBmpMapa = (HBITMAP)LoadImage(NULL, TEXT("mapa_final.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	GetObject(hBmpMapa, sizeof(dados.bmpMapa), &dados.bmpMapa);

	hBmpAviao = (HBITMAP)LoadImage(NULL, TEXT("airplane.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	GetObject(hBmpAviao, sizeof(dados.bmpAvi), &dados.bmpAvi);

	hdc = GetDC(dados.hWnd);
	dados.bmpDCAero = CreateCompatibleDC(hdc);
	SelectObject(dados.bmpDCAero, hBmpAeroporto);
	ReleaseDC(dados.hWnd, hdc);

	hdc = GetDC(dados.hWnd);
	dados.bmpDCMapa = CreateCompatibleDC(hdc);
	SelectObject(dados.bmpDCMapa, hBmpMapa);
	ReleaseDC(dados.hWnd, hdc);

	hdc = GetDC(dados.hWnd);
	dados.bmpDCAvi = CreateCompatibleDC(hdc);
	SelectObject(dados.bmpDCAvi, hBmpAviao);
	ReleaseDC(dados.hWnd, hdc);

	SetWindowLongPtr(dados.hWnd, GWLP_USERDATA, (LONG_PTR)&dados);
	ShowWindow(dados.hWnd, nCmdShow);	
	UpdateWindow(dados.hWnd);	

	if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\TPSO2"), 0, KEY_ALL_ACCESS, &dados.chave) == ERROR_SUCCESS) {
		if (RegQueryValueEx(dados.chave, TEXT("max_avioes"), NULL, NULL, (LPBYTE)&dados.n_avioes, &tam) != ERROR_SUCCESS || RegQueryValueEx(dados.chave, TEXT("max_aeroportos"), NULL, NULL, (LPBYTE)&dados.n_aero, &tam) != ERROR_SUCCESS)
			DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG_REGKEY), dados.hWnd, AdicionaChaves);
		else {
			dados.Avi_Controlo = malloc(sizeof(Aviao) * dados.n_avioes);
			dados.Aero_Controlo = malloc(sizeof(Aeroporto) * dados.n_aero);
		}
	}
	else {
		if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\TPSO2"), 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &dados.chave, NULL) != ERROR_SUCCESS)
		{
			_ftprintf(stderr, TEXT("Erro no RegCreateKeyEx.\n"));
			UnmapViewOfFile(dados.pM_Aviao_in);
			UnmapViewOfFile(dados.pM_Aviao_out);
			UnmapViewOfFile(dados.pB_Aviao);
			fecha_handles(&dados);
			CloseHandle(hFileMapMemPartilhada_A_C);
			CloseHandle(hFileMapMemPartilhada_C_A);
			CloseHandle(hFileMapBufferCircular);
			for (i = 0; i < N_PASSAGEIROS; i++)
				DisconnectNamedPipe(dados.hPipes[i].hPipe);
			return 26;
		}
		DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG_REGKEY), dados.hWnd, AdicionaChaves);
	}

	while (GetMessage(&lpMsg, NULL, 0, 0)) {
		TranslateMessage(&lpMsg);
		DispatchMessage(&lpMsg);
	}

	aviso.adicionado = -2;
	WaitForSingleObject(dados.hMutex, INFINITE); //responder aos aviões de volta a dizer que terminou
	ZeroMemory(dados.pM_Aviao_out, sizeof(Aviao));
	CopyMemory(dados.pM_Aviao_out, &aviso, sizeof(Aviao));
	SetEvent(dados.hEvent_C_A);
	ReleaseMutex(dados.hMutex);

	dados.terminar = 1; //encerrar as threads
	SetEvent(dados.hEvent_A_C1); //encerrar a unica thread que não tem a flag terminar no while
	ReleaseSemaphore(dados.hSemLeitura, 1, NULL); //avançar o ciclo do buffer circular para terminar no while
	Sleep(500); //dar tempo para as threads encerrarem
	UnmapViewOfFile(dados.pM_Aviao_out);
	UnmapViewOfFile(dados.pM_Aviao_in);
	UnmapViewOfFile(dados.pB_Aviao);
	fecha_handles(&dados);
	CloseHandle(hFileMapMemPartilhada_A_C);
	CloseHandle(hFileMapMemPartilhada_C_A);
	CloseHandle(hFileMapBufferCircular);
	CloseHandle(hThreadThreadVerificaTempo);
	RegCloseKey(dados.chave);
	free(dados.Aero_Controlo);
	free(dados.Avi_Controlo);
	free(dados.Pass_Controlo);
	
	for (i = 0; i < N_PASSAGEIROS; i++)
		DisconnectNamedPipe(dados.hPipes[i].hPipe);

	return((int)lpMsg.wParam);

}

LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {

	HDC hdc, wdc;
	TCHAR t_avi_info[TAM_MSG], t_aero_info[TAM_MSG];
	RECT rect, avi_info;
	PAINTSTRUCT ps;
	POINT pt;
	pt.x = 0; 
	pt.y = 0;
	DWORD i;
	DATA* dados;
	int x = 0, y = 0;
	dados = (DATA*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	
	switch (messg) {

	case WM_MOUSEMOVE:
		//InvalidateRect(dados->hWnd, NULL, FALSE);
		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);
		for (i = 0; i < dados->n_aero; i++) {
			if (dados->Avi_Controlo[i].adicionado == 1) {
				if (pt.x >= dados->Avi_Controlo[i].coordenadas.coords[2][0] && pt.x <= (dados->Avi_Controlo[i].coordenadas.coords[2][0] + 10) && pt.y >= dados->Avi_Controlo[i].coordenadas.coords[2][1] && pt.y <= (dados->Avi_Controlo[i].coordenadas.coords[2][1] + 10) && dados->Avi_Controlo[i].voo == 1) {
					wdc = GetWindowDC(hWnd);
					GetClientRect(hWnd, &avi_info);
					avi_info.left = pt.x + 20;
					avi_info.top = pt.y + 50;
					SetTextColor(wdc, 0x00000000);
					SetBkMode(wdc, OPAQUE);
					_stprintf_s(t_avi_info, TAM_MSG, TEXT("ID: %d\nAEROPORTO PARTIDA: %s\nAEROPORTO CHEGADA: %s\nNº PASSAGEIROS: %d"), dados->Avi_Controlo[i].id, dados->Avi_Controlo[i].aeroporto_partida, dados->Avi_Controlo[i].aeroporto_chegada, dados->Avi_Controlo[i].cap_atual);
					DrawText(wdc, t_avi_info, -1, &avi_info, DT_NOCLIP);
					DeleteDC(wdc);
				}
			}
		}
		break;

	case WM_LBUTTONUP:
		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);
		for (i = 0; i < dados->n_aero; i++) {
			if (dados->Aero_Controlo[i].adicionado == 1) {
				if (pt.x >= dados->Aero_Controlo[i].pos_x && pt.x <= (dados->Aero_Controlo[i].pos_x + 50) && pt.y >= dados->Aero_Controlo[i].pos_y && pt.y <= (dados->Aero_Controlo[i].pos_y + 50)) {
					_stprintf_s(t_aero_info, TAM_MSG, TEXT("NOME: %s\nNº AVIÕES: %d\nNº PASSAGEIROS: %d"), dados->Aero_Controlo[i].nome, dados->Aero_Controlo[i].cap_avioes, dados->Aero_Controlo[i].cap_atual);
					MessageBox(hWnd, t_aero_info, TEXT("Informação do Aeroporto"), MB_OK | MB_ICONINFORMATION);
				}
			}
		}
		break;

	case WM_COMMAND:
		
		switch (LOWORD(wParam)) {
		//case ID_ACCELERATOR_C:
		case ID_MENU_ADDAIRPORT:
			DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG_ADDAIRPORT), hWnd, AdicionaAeroportos);
			break;

		case ID_MENU_LIST_AIRPORTS:
			DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG_LIST_AIRPORTS), hWnd, ListaAeroportos);
			break;

		case ID_MENU_LIST_AIRPLANES:
			DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG_LIST_AIRPLANES), hWnd, ListaAvioes);
			break;

		case ID_MENU_LIST_PASSENGERS:
			DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG_LIST_PASSENGERS), hWnd, ListaPassageiros);
			break;

		case ID_MENU_SUSPEND_AIRPLANES:
			CheckMenuItem(GetMenu(hWnd), ID_MENU_SUSPEND_AIRPLANES, MF_CHECKED);
			CheckMenuItem(GetMenu(hWnd), ID_MENU_ACCEPT_AIRPLANES, MF_UNCHECKED);
			dados->aceita_avioes = 0;
			break;

		case ID_MENU_ACCEPT_AIRPLANES:
			CheckMenuItem(GetMenu(hWnd), ID_MENU_SUSPEND_AIRPLANES, MF_UNCHECKED);
			CheckMenuItem(GetMenu(hWnd), ID_MENU_ACCEPT_AIRPLANES, MF_CHECKED);
			dados->aceita_avioes = 1;
			break;

		case ID_MENU_ABOUT:
			DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG_ABOUT), hWnd, About);
			break;

		case ID_COMMANDS_EXIT:
			if (MessageBox(hWnd, TEXT("Tem a certeza que quer sair?"), TEXT("Confirmação"), MB_YESNO | MB_ICONQUESTION) == IDYES) {
				DestroyWindow(hWnd);
			}
			break;
		}

		break;
	case WM_PAINT: //sempre disparado sempre que há um refrescamento da janela
		hdc = BeginPaint(hWnd, &ps); //substitui o GetDC
		GetClientRect(hWnd, &rect);

		if (dados->memDC == NULL) { //1ª vez que estou a passar neste WM_PAINT
			dados->memDC = CreateCompatibleDC(hdc);
			dados->hBitmapDB = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
			SelectObject(dados->memDC, dados->hBitmapDB);
			DeleteObject(dados->hBitmapDB);
		}

		FillRect(dados->memDC, &rect, CreateSolidBrush(RGB(160, 128, 64)));

		BitBlt(dados->memDC, 0, 0, 1757, 1744, dados->bmpDCMapa, 0, 0, SRCCOPY);

		
		for (i = 0; i < dados->n_aero; i++) {
			if(dados->Aero_Controlo[i].adicionado == 1)
				BitBlt(dados->memDC, dados->Aero_Controlo[i].pos_x, dados->Aero_Controlo[i].pos_y, dados->bmpAero.bmWidth, dados->bmpAero.bmHeight, dados->bmpDCAero, 0, 0, SRCCOPY);
		}
		for (i = 0; i < dados->n_aero; i++) {
			if (dados->Avi_Controlo[i].adicionado == 1 && dados->Avi_Controlo[i].voo != -1)
				BitBlt(dados->memDC, dados->Avi_Controlo[i].coordenadas.coords[2][0], dados->Avi_Controlo[i].coordenadas.coords[2][1], dados->bmpAvi.bmWidth, dados->bmpAvi.bmHeight, dados->bmpDCAvi, 0, 0, SRCCOPY);
		}

		BitBlt(hdc, 0, 0, rect.right, rect.bottom, dados->memDC, 0, 0, SRCCOPY);

		EndPaint(hWnd, &ps); //substitui o ReleaseDC
		break;

	case WM_SIZE: //caso o tamaho da janela seja alterado
		WaitForSingleObject(dados->hMutex, INFINITE);
		dados->memDC = NULL;
		ReleaseMutex(dados->hMutex);

	case WM_ERASEBKGND:
		return TRUE;

	case WM_CLOSE:
		if (MessageBox(hWnd, TEXT("Tem a certeza que quer sair?"), TEXT("Confirmação"), MB_YESNO | MB_ICONQUESTION) == IDYES) {
			DestroyWindow(hWnd);
		}
		break;

	case WM_DESTROY:		
		PostQuitMessage(0);
		break;
	default:
		return(DefWindowProc(hWnd, messg, wParam, lParam));
		break;
	}
	return(0);
}

LRESULT CALLBACK AdicionaChaves(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	DWORD airports_key = 0, airplanes_key = 0;
	BOOL sucess = FALSE;

	DATA* dados;
	HWND hPai = GetParent(hWnd);
	dados = (DATA*)GetWindowLongPtr(hPai, GWLP_USERDATA);

	switch (messg)
	{
	case WM_COMMAND:

		if (LOWORD(wParam) == ID_OK_CREATE_KEYS)
		{
			airplanes_key = GetDlgItemInt(hWnd, IDC_EDIT_AIRPLANES_KEY, &sucess, FALSE);
			if (sucess && (airplanes_key > 0)) {
				airports_key = GetDlgItemInt(hWnd, IDC_EDIT_AIRPORTS_KEY, &sucess, FALSE);
				if (sucess && (airports_key > 0)) {
					RegSetValueEx(dados->chave, TEXT("max_avioes"), 0, REG_DWORD, (BYTE*)&airplanes_key, sizeof(DWORD));
					RegSetValueEx(dados->chave, TEXT("max_aeroportos"), 0, REG_DWORD, (BYTE*)&airports_key, sizeof(DWORD));
					dados->n_avioes = airplanes_key;
					dados->n_aero = airports_key;
					dados->Avi_Controlo = malloc(sizeof(Aviao) * dados->n_avioes);
					dados->Aero_Controlo = malloc(sizeof(Aeroporto) * dados->n_aero);
					EndDialog(hWnd, 0);
					return TRUE;
				}
				else {
					SetDlgItemText(hWnd, IDC_EDIT_AIRPLANES_KEY, TEXT(""));
					SetDlgItemText(hWnd, IDC_EDIT_AIRPORTS_KEY, TEXT(""));
					MessageBox(hWnd, TEXT("Valores inválidos!"), TEXT("Aviso"), MB_OK | MB_ICONERROR);
				}
			}
			else {
				SetDlgItemText(hWnd, IDC_EDIT_AIRPLANES_KEY, TEXT(""));
				SetDlgItemText(hWnd, IDC_EDIT_AIRPORTS_KEY, TEXT(""));
				MessageBox(hWnd, TEXT("Valores inválidos!"), TEXT("Aviso"), MB_OK | MB_ICONERROR);
			}
		}
		break;
	}
	return FALSE;
}

LRESULT CALLBACK AdicionaAeroportos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam){
	TCHAR a_nome[TAM_MSG];
	int x, y, flag;
	BOOL sucess = FALSE;
	
	DATA* dados;
	HWND hPai = GetParent(hWnd);
	dados = (DATA*)GetWindowLongPtr(hPai, GWLP_USERDATA);

	switch (messg)
	{
	case WM_COMMAND:

		if (LOWORD(wParam) == ID_ADD_AIRPORT)
		{
			GetDlgItemText(hWnd, IDC_EDIT_NAME, a_nome, TAM_MSG);
			if (a_nome[0] == '\0')
				MessageBox(hWnd, TEXT("Nome do Aeroporto inválido!"), TEXT("Aviso"), MB_OK | MB_ICONERROR);
			else{
				x = GetDlgItemInt(hWnd, IDC_EDIT_X_COORD, &sucess, FALSE);
				if (sucess && (x >= 0 && x <= 1000)) {
					y = GetDlgItemInt(hWnd, IDC_EDIT_Y_COORD, &sucess, FALSE);
					if (sucess && (y >= 0 && y <= 1000)) {
						flag = cria_aeroportos(dados->Aero_Controlo, dados->n_aero, a_nome, x, y);

						if (flag == 0) {
							InvalidateRect(dados->hWnd, NULL, FALSE);
							SetDlgItemText(hWnd, IDC_EDIT_NAME, TEXT(""));
							SetDlgItemText(hWnd, IDC_EDIT_X_COORD, TEXT(""));
							SetDlgItemText(hWnd, IDC_EDIT_Y_COORD, TEXT(""));
							MessageBox(hWnd, TEXT("Aeroporto adicionado com sucesso!"), TEXT("Aviso"), MB_OK | MB_ICONINFORMATION);
						}
						if (flag == 1) {
							SetDlgItemText(hWnd, IDC_EDIT_NAME, TEXT(""));
							SetDlgItemText(hWnd, IDC_EDIT_X_COORD, TEXT(""));
							SetDlgItemText(hWnd, IDC_EDIT_Y_COORD, TEXT(""));
							MessageBox(hWnd, TEXT("Já existe um aeroporto no raio de 10 unidades!"), TEXT("Aviso"), MB_OK | MB_ICONERROR);
						}	
						if (flag == 2) {
							SetDlgItemText(hWnd, IDC_EDIT_NAME, TEXT(""));
							MessageBox(hWnd, TEXT("Já existe um aeroporto com o mesmo nome!"), TEXT("Aviso"), MB_OK | MB_ICONERROR);
						}
						if (flag == 3) {							
							SetDlgItemText(hWnd, IDC_EDIT_NAME, TEXT(""));
							SetDlgItemText(hWnd, IDC_EDIT_X_COORD, TEXT(""));
							SetDlgItemText(hWnd, IDC_EDIT_Y_COORD, TEXT(""));
							MessageBox(hWnd, TEXT("Atingiu o limite máximo de aeroportos!"), TEXT("Aviso"), MB_OK | MB_ICONERROR);
						}		
					}
					else {
						SetDlgItemText(hWnd, IDC_EDIT_X_COORD, TEXT(""));
						MessageBox(hWnd, TEXT("Coordenas y inválidas!"), TEXT("Aviso"), MB_OK | MB_ICONERROR);
					}
				}
				else {
					SetDlgItemText(hWnd, IDC_EDIT_Y_COORD, TEXT(""));
					MessageBox(hWnd, TEXT("Coordenas x inválidas!"), TEXT("Aviso"), MB_OK | MB_ICONERROR);
				}
			}
		}
		else if (LOWORD(wParam) == ID_EXIT_ADD_AIRPORT)
		{
			EndDialog(hWnd, 0);
			return TRUE;
		}

		break;

	case WM_CLOSE:

		EndDialog(hWnd, 0);
		return TRUE;
	}

	return FALSE;
}

LRESULT CALLBACK ListaAeroportos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	DWORD i;
	TCHAR msg[TAM_MSG];
	HWND hwndList;
	DATA* dados;
	HWND hPai = GetParent(hWnd);
	dados = (DATA*)GetWindowLongPtr(hPai, GWLP_USERDATA);

	switch (messg)
	{
	case WM_INITDIALOG:

		hwndList = GetDlgItem(hWnd, IDC_LISTBOX_LIST_AIRPORTS);
		SendMessage(hwndList, LB_RESETCONTENT, 0, 0);

		for (i = 0; i < dados->n_aero; i++) {
			if (dados->Aero_Controlo[i].adicionado == 1) {
				_stprintf_s(msg, TAM_MSG, TEXT("NOME: %s COORDS: (%d,%d) Nº AVIÕES: %d Nº PASSAGEIROS: %d"), dados->Aero_Controlo[i].nome, dados->Aero_Controlo[i].pos_x, dados->Aero_Controlo[i].pos_y, dados->Aero_Controlo[i].cap_avioes, dados->Aero_Controlo[i].cap_atual);
				SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)msg);
			}
		}

		break;

	case WM_CLOSE:

		EndDialog(hWnd, 0);
		return TRUE;
	}

	return FALSE;
}

LRESULT CALLBACK ListaAvioes(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	DWORD i;
	TCHAR msg[TAM_MSG];
	HWND hwndList;
	DATA* dados;
	HWND hPai = GetParent(hWnd);
	dados = (DATA*)GetWindowLongPtr(hPai, GWLP_USERDATA);

	switch (messg)
	{
	case WM_INITDIALOG:

		hwndList = GetDlgItem(hWnd, IDC_LISTBOX_LIST_AIRPLANES);
		SendMessage(hwndList, LB_RESETCONTENT, 0, 0);

		for (i = 0; i < dados->n_avioes; i++) {
			if (dados->Avi_Controlo[i].adicionado == 1) {
				_stprintf_s(msg, TAM_MSG, TEXT("ID: %d AEROPORTO DE PARTIDA: %s AEROPORTO DE CHEGADA: %s CAPACIDADE MAXIMA: %d CAPACIDADE ACTUAL: %d"), dados->Avi_Controlo[i].id, dados->Avi_Controlo[i].aeroporto_partida, dados->Avi_Controlo[i].aeroporto_chegada, dados->Avi_Controlo[i].cap_maxima, dados->Avi_Controlo[i].cap_atual);
				SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)msg);
			}
		}

		break;

	case WM_CLOSE:

		EndDialog(hWnd, 0);
		return TRUE;
	}

	return FALSE;
}

LRESULT CALLBACK ListaPassageiros(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	DWORD i;
	TCHAR msg[TAM_MSG];
	HWND hwndList;
	DATA* dados;
	HWND hPai = GetParent(hWnd);
	dados = (DATA*)GetWindowLongPtr(hPai, GWLP_USERDATA);

	switch (messg)
	{
	case WM_INITDIALOG:

		hwndList = GetDlgItem(hWnd, IDC_LISTBOX_LIST_PASSENGERS);
		SendMessage(hwndList, LB_RESETCONTENT, 0, 0);

		for (i = 0; i < dados->n_pass; i++) {
			if (dados->Pass_Controlo[i].adicionado == 1) {
				_stprintf_s(msg, TAM_MSG, TEXT("NOME: %s AEROPORTO DE PARTIDA: %s AEROPORTO DE CHEGADA: %s"), dados->Pass_Controlo[i].nome, dados->Pass_Controlo[i].aero_partida, dados->Pass_Controlo[i].aero_chegada);
				SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)msg);
			}
		}

		break;

	case WM_CLOSE:

		EndDialog(hWnd, 0);
		return TRUE;
	}

	return FALSE;
}

LRESULT CALLBACK About(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	
	switch (messg)
	{
	case WM_COMMAND:

		if (LOWORD(wParam) == IDR_ABOUT_OK)
		{
			EndDialog(hWnd, 0);
			return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return TRUE;
	}

	return FALSE;
}

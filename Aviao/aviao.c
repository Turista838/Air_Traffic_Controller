#include "aviao.h"
#include "SO2_TP_DLL_2021.h"

void fecha_handles(DATA* dados);

DWORD WINAPI ThreadLeitura_MemPartilhada(LPVOID param) {
	DATA* dados = (DATA*)param;

	while (1) {
		
		WaitForSingleObject(dados->hEvent_C_A, INFINITE);
		
		if (dados->terminar)
			break;

		WaitForSingleObject(dados->hMutex, INFINITE);
		if (dados->pM_Aviao_in->adicionado == -2) {
			_tprintf(TEXT("O Processo Controlador foi encerrado. Insira 'encerrar' para sair.\n"));
		}
		if (dados->pM_Aviao_in->id == _getpid()) {
			if (dados->pM_Aviao_in->adicionado == 1) {
				dados->aviao.adicionado = dados->pM_Aviao_in->adicionado;
			}
			if (dados->pM_Aviao_in->embarcar == 0) {
				_tprintf(TEXT("Aviao tem permissão para dar inicio ao embarque de passageiros\n"));
				dados->aviao.embarcar = dados->pM_Aviao_in->embarcar;
				dados->aviao.coordenadas.coords[0][0] = dados->pM_Aviao_in->coordenadas.coords[0][0];
				dados->aviao.coordenadas.coords[0][1] = dados->pM_Aviao_in->coordenadas.coords[0][1];
				dados->aviao.coordenadas.coords[1][0] = dados->pM_Aviao_in->coordenadas.coords[1][0];
				dados->aviao.coordenadas.coords[1][1] = dados->pM_Aviao_in->coordenadas.coords[1][1];

			}
			if (dados->pM_Aviao_in->embarcar == 2) {
				dados->aviao.embarcar = dados->pM_Aviao_in->embarcar;
			}
			if (dados->pM_Aviao_in->voo == 0) { //mudar a flag voo para não cair sempre na função reset_voo
				dados->aviao.voo = -1;
			}
			if (dados->pM_Aviao_in->voo == 2) { //mudar a flag voo para não cair sempre na função actualiza_voo
				dados->aviao.voo = dados->pM_Aviao_in->voo;
			}
		}
		ReleaseMutex(dados->hMutex);

		ResetEvent(dados->hEvent_C_A);
	}
	return 0;
}

DWORD WINAPI ThreadEscrita_MemPartilhada(LPVOID param) {

	DATA* dados = (DATA*)param;
	DWORD i;
	int res = 1;
	int cur_x = dados->aviao.coordenadas.coords[0][0], cur_y = dados->aviao.coordenadas.coords[0][1]; 
	int final_dest_x = dados->aviao.coordenadas.coords[1][0], final_dest_y = dados->aviao.coordenadas.coords[1][1];
	int next_x = 0; int next_y = 0;
	int random_x = 0, random_y = 0;

	while (res) {
		for (i = 0; i < dados->velocidade; i++) {

			res = move(cur_x, cur_y, final_dest_x, final_dest_y, &next_x, &next_y);
			if (!res)
				break;

			WaitForSingleObject(dados->hMutex, INFINITE);
			dados->pM_Aviao_out->id = _getpid();
			if (dados->pM_Aviao_out->m[next_x][next_y] == 0) {
				dados->pM_Aviao_out->cap[0] = next_x;
				dados->pM_Aviao_out->cap[1] = next_y;
				dados->pM_Aviao_out->m[next_x][next_y] = _getpid();
				dados->pM_Aviao_out->m[cur_x][cur_y] = 0;
			}
			else{ //está ocupado
				do {
					random_x = rand() % ((1 + 1) + 1) - 1;
					random_y = rand() % ((1 + 1) + 1) - 1;
				} while (dados->pM_Aviao_out->m[next_x + random_x][next_y + random_y] != 0);
				dados->pM_Aviao_out->cap[0] = next_x + random_x;
				dados->pM_Aviao_out->cap[1] = next_y + random_y;
				dados->pM_Aviao_out->m[next_x + random_x][next_y + random_y] = _getpid();
				dados->pM_Aviao_out->m[cur_x][cur_y] = 0;
			}


			SetEvent(dados->hEvent_A_C1);

			WaitForSingleObject(dados->hEvent_A_C2, INFINITE);
			
			ReleaseMutex(dados->hMutex);
			if (dados->terminar)
				res = 0; //terminar se encerrar a meio do voo
			cur_x = next_x;
			cur_y = next_y;
			ResetEvent(dados->hEvent_A_C2);
		}
		Sleep(1000);
	}

	WaitForSingleObject(dados->hMutex, INFINITE);
	_tcscpy_s(dados->aviao.aeroporto_partida, _tcslen(dados->aviao.aeroporto_partida) * sizeof(TCHAR), dados->aviao.aeroporto_chegada);
	_tcscpy_s(dados->aviao.aeroporto_chegada, _tcslen(TEXT("«Não definido»")) * sizeof(TCHAR), TEXT("«Não definido»"));
	dados->aviao.coordenadas.coords[0][0] = final_dest_x;
	dados->aviao.coordenadas.coords[0][1] = final_dest_y;
	dados->aviao.embarcar = -1;
	dados->aviao.voo = 0;
	ReleaseMutex(dados->hMutex);
	_tprintf(TEXT("Chegou ao destino...\n"));
	return 0;
}

DWORD WINAPI ThreadEscrita_BufferCircular(LPVOID param) {
	DATA* dados = (DATA*)param;

	_tprintf(TEXT("Contacto com o controlador estabelecido...\n\n"));

	while (dados->terminar == 0) {

		WaitForSingleObject(dados->hSemEscrita, INFINITE);
		WaitForSingleObject(dados->hMutex, INFINITE);

		dados->aviao.id = _getpid();
		CopyMemory(&dados->pB_Aviao->av[dados->pB_Aviao->posE], &dados->aviao, sizeof(Aviao));
		dados->pB_Aviao->posE++;

		if (dados->pB_Aviao->posE == 10)
			dados->pB_Aviao->posE = 0;

		ReleaseMutex(dados->hMutex);
		ReleaseSemaphore(dados->hSemLeitura, 1, NULL); //o que vai fazer a leitura vai ter oportunidade de ler 1 bloco
		Sleep(2000);
	}

	return 0;
}

int _tmain(int argc, TCHAR* argv[]) {
	DWORD cap_max, velocidade;
	TCHAR nome_aero[TAM_MSG], comando[TAM_MSG];
	BOOL primeiroProcesso = TRUE;
	HANDLE hFileMapMemPartilhada_A_C, hFileMapMemPartilhada_C_A, hThreadMemPartilhada_A_C, hThreadMemPartilhada_C_A, hFileMapBufferCircular, hThreadBufferCircular;
	DATA dados;
	int i;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	_tprintf(TEXT("INICIO DO PROCESSO AVIÃO\n\n"));

	srand(time(NULL));

	if (argc != 4) {
		_ftprintf(stderr, TEXT("Erro a criar aviao.\n"));
		return 1;
	}
	else {
		cap_max = _ttoi(argv[1]);
		velocidade = _ttoi(argv[2]);
		_tcscpy_s(nome_aero, _tcslen(argv[3]) * sizeof(TCHAR), argv[3]);
	}

	_tprintf(TEXT("Identificação: %d.\n"), _getpid());
	_tprintf(TEXT("Capacidade máxima: %d.\n"), cap_max);
	_tprintf(TEXT("Velocidade: %d por segundo.\n"), velocidade);
	_tprintf(TEXT("Aeroporto de partida: %s.\n"), nome_aero);
	_tprintf(TEXT("Escreva 'info' para saber o seu status\n"));

	dados.velocidade = velocidade;

	//BufferCircular
	dados.hSemEscrita = CreateSemaphore(NULL, TAM_BUFFER_C, TAM_BUFFER_C, TEXT("SEM_ESCRITA"));
	if (dados.hSemEscrita == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateSemaphore.\n"));
		return 2;
	}

	dados.hSemLeitura = CreateSemaphore(NULL, 0, TAM_BUFFER_C, TEXT("SEM_LEITURA"));
	if (dados.hSemLeitura == NULL) {
		fecha_handles(&dados);
		_ftprintf(stderr, TEXT("Erro no CreateSemaphore.\n"));
		return 3;
	}

	dados.hMutex = CreateMutex(NULL, FALSE, TEXT("MUTEX_PRODUTOR"));
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		primeiroProcesso = FALSE;
	}
	if (dados.hMutex == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateMutex.\n"));
		fecha_handles(&dados);
		return 4;
	}

	hFileMapBufferCircular = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, TEXT("BUFFER"));
	if (hFileMapBufferCircular == NULL) { //no caso de não existir controlador
		_ftprintf(stderr, TEXT("Não existe processo Controlador!"));
		fecha_handles(&dados);
		return 5; //!!!
	}

	dados.pB_Aviao = (Buffer_Aviao*)MapViewOfFile(hFileMapBufferCircular, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (dados.pB_Aviao == NULL) {
		_ftprintf(stderr, TEXT("Erro no MapViewOfFile.\n"));
		fecha_handles(&dados);
		CloseHandle(hFileMapBufferCircular);
		return 6;
	}
	
	if (primeiroProcesso == TRUE) {
		dados.pB_Aviao->posE = 0;
	}

	WaitForSingleObject(dados.hMutex, INFINITE);
	dados.aviao.id = _getpid();
	_tcscpy_s(dados.aviao.aeroporto_partida, _tcslen(nome_aero) * sizeof(TCHAR), nome_aero);
	dados.aviao.cap_maxima = cap_max;
	dados.aviao.voo = -1;
	dados.aviao.embarcar = -1;
	dados.aviao.adicionado = -1;
	dados.terminar = 0;
	ReleaseMutex(dados.hMutex);
	hThreadBufferCircular = CreateThread(NULL, 0, ThreadEscrita_BufferCircular, &dados, 0, NULL);
	if (hThreadBufferCircular == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateThread.\n"));
		UnmapViewOfFile(dados.pB_Aviao);
		fecha_handles(&dados);
		CloseHandle(hFileMapBufferCircular);
		return 7;
	}
	//Fim do BufferCircular


	//MEM PARTILHADA AVIAO -> CONTROLADOR
	hFileMapMemPartilhada_A_C = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, TEXT("MEM_PARTILHADA_A_C"));
	if (hFileMapMemPartilhada_A_C == NULL) {
		_ftprintf(stderr, TEXT("Erro no OpenFileMapping.\n"));
		UnmapViewOfFile(dados.pB_Aviao);
		fecha_handles(&dados);
		CloseHandle(hFileMapBufferCircular);
		CloseHandle(hThreadBufferCircular);
		return 8;
	}
	dados.pM_Aviao_out = (Mem_Aviao*)MapViewOfFile(hFileMapMemPartilhada_A_C, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (dados.pM_Aviao_out == NULL) {
		_ftprintf(stderr, TEXT("Erro no MapViewOfFile.\n"));
		UnmapViewOfFile(dados.pB_Aviao);
		fecha_handles(&dados);
		CloseHandle(hFileMapBufferCircular);
		CloseHandle(hThreadBufferCircular);
		CloseHandle(hFileMapMemPartilhada_A_C);
		return 9;
	}

	dados.hEvent_A_C1 = CreateEvent(NULL, TRUE, FALSE, TEXT("EVENTO_A_C1"));
	if (dados.hEvent_A_C1 == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateEvent.\n"));
		UnmapViewOfFile(dados.pB_Aviao);
		UnmapViewOfFile(dados.pM_Aviao_out);
		fecha_handles(&dados);
		CloseHandle(hFileMapBufferCircular);
		CloseHandle(hThreadBufferCircular);
		CloseHandle(hFileMapMemPartilhada_A_C);
		return 10;
	}

	dados.hEvent_A_C2 = CreateEvent(NULL, TRUE, FALSE, TEXT("EVENTO_A_C2"));
	if (dados.hEvent_A_C2 == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateEvent.\n"));
		UnmapViewOfFile(dados.pB_Aviao);
		UnmapViewOfFile(dados.pM_Aviao_out);
		fecha_handles(&dados);
		CloseHandle(hFileMapBufferCircular);
		CloseHandle(hThreadBufferCircular);
		CloseHandle(hFileMapMemPartilhada_A_C);
		return 11;
	}

	//FIM MEM PARTILHADA
	
	//MEM PARTILHADA CONTROLADOR -> AVIAO
	hFileMapMemPartilhada_C_A = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, TEXT("MEM_PARTILHADA_C_A"));
	if (hFileMapMemPartilhada_C_A == NULL) {
		_ftprintf(stderr, TEXT("Erro no OpenFileMapping.\n"));
		UnmapViewOfFile(dados.pB_Aviao);
		UnmapViewOfFile(dados.pM_Aviao_out);
		fecha_handles(&dados);
		CloseHandle(hFileMapBufferCircular);
		CloseHandle(hThreadBufferCircular);
		CloseHandle(hFileMapMemPartilhada_A_C);
		return 14;
	}

	dados.pM_Aviao_in = (Aviao*)MapViewOfFile(hFileMapMemPartilhada_C_A, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (dados.pM_Aviao_in == NULL) {
		_ftprintf(stderr, TEXT("Erro no MapViewOfFile.\n"));
		UnmapViewOfFile(dados.pB_Aviao);
		UnmapViewOfFile(dados.pM_Aviao_out);
		fecha_handles(&dados);
		CloseHandle(hFileMapBufferCircular);
		CloseHandle(hThreadBufferCircular);
		CloseHandle(hFileMapMemPartilhada_A_C);
		CloseHandle(hFileMapMemPartilhada_C_A);
		return 15;
	}

	dados.hEvent_C_A = CreateEvent(NULL, TRUE, FALSE, TEXT("EVENTO_C_A"));
	if (dados.pM_Aviao_in == NULL) {
		_ftprintf(stderr, TEXT("Erro a criar Evento.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		UnmapViewOfFile(dados.pB_Aviao);
		UnmapViewOfFile(dados.pM_Aviao_out);
		fecha_handles(&dados);
		CloseHandle(hFileMapBufferCircular);
		CloseHandle(hThreadBufferCircular);
		CloseHandle(hFileMapMemPartilhada_A_C);
		CloseHandle(hFileMapMemPartilhada_C_A);
		return 16;
	}

	hThreadMemPartilhada_C_A = CreateThread(NULL, 0, ThreadLeitura_MemPartilhada, &dados, 0, NULL);
	if (hThreadMemPartilhada_C_A == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateThread.\n"));
		UnmapViewOfFile(dados.pM_Aviao_in);
		UnmapViewOfFile(dados.pB_Aviao);
		UnmapViewOfFile(dados.pM_Aviao_out);
		fecha_handles(&dados);
		CloseHandle(hFileMapBufferCircular);
		CloseHandle(hThreadBufferCircular);
		CloseHandle(hFileMapMemPartilhada_A_C);
		CloseHandle(hFileMapMemPartilhada_C_A);
		return 17;
	}
	//FIM MEM PARTILHADA

	Sleep(500);

	do {

		_tprintf(TEXT("Introduza um comando:\n"));
		_fgetts(comando, TAM_MSG, stdin);
		comando[_tcslen(comando) - 1] = '\0';
		for (i = 0; i < (signed int)_tcslen(comando); i++)
			comando[i] = _totupper(comando[i]);

		if (!(_tcsicmp(TEXT("DEFINIR DESTINO"), comando))) {
			if (dados.aviao.voo != 1 && dados.aviao.adicionado == 1 && dados.aviao.embarcar != 1) {
				_tprintf(TEXT("Introduza o destino: "));
				_fgetts(comando, TAM_MSG, stdin);
				comando[_tcslen(comando) - 1] = '\0';
				WaitForSingleObject(dados.hMutex, INFINITE);
				_tcscpy_s(dados.aviao.aeroporto_chegada, _tcslen(comando) * sizeof(TCHAR), comando);
				ReleaseMutex(dados.hMutex);
			}
			else {
				if(dados.aviao.adicionado != 1)
					_tprintf(TEXT("Avião não está adicionado. Aguarde que o Controlador adicione.\n"));
				if (dados.aviao.embarcar == 1)
					_tprintf(TEXT("Não pode alterar o destino depois de embarcar.\n"));
				if (dados.aviao.voo == 1)
					_tprintf(TEXT("Não pode alterar o destino em pleno voo.\n"));
			}
		}

		if (!(_tcsicmp(TEXT("EMBARCAR"), comando))) {
			if (dados.aviao.voo != 1 && dados.aviao.embarcar == -1) {
				_tprintf(TEXT("Tem de definir o destino primeiro e ter a luz verde do controlador\n"));
			}
			else {
				if (dados.aviao.embarcar == 0) {
					WaitForSingleObject(dados.hMutex, INFINITE);
					dados.aviao.embarcar = 1;
					ReleaseMutex(dados.hMutex);
					_tprintf(TEXT("Embarque com sucesso!\n"));
				}
				if (dados.aviao.voo == 1)
					_tprintf(TEXT("Não embarcar em pleno voo.\n"));
			}
		}

		if (!(_tcsicmp(TEXT("INICIAR VOO"), comando))) {
			if (dados.aviao.voo != 1 && dados.aviao.embarcar == 2) {
				//Buff Circular
				WaitForSingleObject(dados.hMutex, INFINITE);
				dados.aviao.voo = 1;
				ReleaseMutex(dados.hMutex);
				//Memoria partilhada
				hThreadMemPartilhada_A_C = CreateThread(NULL, 0, ThreadEscrita_MemPartilhada, &dados, 0, NULL);
				if (hThreadMemPartilhada_A_C == NULL) {
					_ftprintf(stderr, TEXT("Erro no CreateThread.\n"));
					UnmapViewOfFile(dados.pM_Aviao_in);
					UnmapViewOfFile(dados.pB_Aviao);
					UnmapViewOfFile(dados.pM_Aviao_out);
					fecha_handles(&dados);
					CloseHandle(hFileMapBufferCircular);
					CloseHandle(hThreadBufferCircular);
					CloseHandle(hFileMapMemPartilhada_A_C);
					CloseHandle(hFileMapMemPartilhada_C_A);
					return 18;
				}
				_tprintf(TEXT("Voo iniciado...\n"));
			}
			else {
				if (dados.aviao.embarcar == 0)
					_tprintf(TEXT("Ainda não deu ordem de embarque.\n"));
				if (dados.aviao.embarcar == -1)
					_tprintf(TEXT("Não tem permissão para embarcar.\n"));
				if (dados.aviao.voo == 1 || dados.aviao.voo == 2)
					_tprintf(TEXT("Avião já se encontra em voo.\n"));
			}		
		}
		
		if (!(_tcsicmp(TEXT("INFO"), comando))) {
			_tprintf(TEXT("ID: %d\n"), dados.aviao.id);
			_tprintf(TEXT("Adicionado: %d\n"), dados.aviao.adicionado);
			_tprintf(TEXT("Cap maxima: %d\n"), dados.aviao.cap_maxima);
			if (dados.aviao.adicionado == 1)
				_tprintf(TEXT("Aeroporto Partida: %s\n"), dados.aviao.aeroporto_partida);
			if (dados.aviao.embarcar != -1) {
				_tprintf(TEXT("Aeroporto Chegada: %s\n"), dados.aviao.aeroporto_chegada);
				_tprintf(TEXT("Coords Aeroporto partida: (%d,%d)\n"), dados.aviao.coordenadas.coords[0][0], dados.aviao.coordenadas.coords[0][1]);
				_tprintf(TEXT("Coords Aeroporto chegada: (%d,%d)\n"), dados.aviao.coordenadas.coords[1][0], dados.aviao.coordenadas.coords[1][1]);
			}
			_tprintf(TEXT("Embarcar: %d\n"), dados.aviao.embarcar);
			_tprintf(TEXT("Em voo: %d\n"), dados.aviao.voo);
			if (dados.aviao.adicionado == 0)
				_tprintf(TEXT("Aviao não adicionado (Controlador cheio!)\n"));
			if (dados.aviao.adicionado == -1)
				_tprintf(TEXT("Aviao não adicionado (Aeroporto de partida não existe)\n"));
			if (dados.aviao.embarcar == -1 && dados.aviao.adicionado == 1)
				_tprintf(TEXT("Não tem permissão para embarcar (Aeroporto de chegada inválido)\n"));
			_tprintf(TEXT("\n"));
		}

	}while (_tcsicmp(TEXT("ENCERRAR"), comando));

	//FALTA FECHAR HANDLES E FILEMAPS
	dados.terminar = 1;
	SetEvent(dados.hEvent_A_C2); //encerrar a unica thread que não tem a flag terminar no while
	ReleaseSemaphore(dados.hSemEscrita, 1, NULL); //avançar o ciclo do buffer circular para terminar no while
	Sleep(500); //dar tempo para as threads encerrarem
	UnmapViewOfFile(dados.pM_Aviao_in);
	UnmapViewOfFile(dados.pB_Aviao);
	UnmapViewOfFile(dados.pM_Aviao_out);
	fecha_handles(&dados);
	CloseHandle(hFileMapBufferCircular);
	CloseHandle(hThreadBufferCircular);
	CloseHandle(hFileMapMemPartilhada_A_C);
	CloseHandle(hFileMapMemPartilhada_C_A);
	CloseHandle(hThreadMemPartilhada_C_A);

	_tprintf(TEXT("FIM do processo AVIÃO\n"));

}

void fecha_handles(DATA* dados) {
	dados->terminar = 1;
	if (dados->hMutex == NULL)
		CloseHandle(dados->hMutex);
	if (dados->hSemEscrita == NULL)
		CloseHandle(dados->hSemEscrita);
	if (dados->hSemLeitura == NULL)
		CloseHandle(dados->hSemLeitura);
	if (dados->hEvent_A_C1 == NULL)
		CloseHandle(dados->hEvent_A_C1);
	if (dados->hEvent_A_C2 == NULL)
		CloseHandle(dados->hEvent_A_C2);
	if (dados->hEvent_C_A == NULL)
		CloseHandle(dados->hEvent_C_A);
}
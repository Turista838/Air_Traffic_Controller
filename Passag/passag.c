#include "passag.h"

DWORD WINAPI ThreadPipesPassageiros(LPVOID param) {

	DATA* dados = (DATA*)param;
	DWORD nBytes;
	BOOL ret, sair = FALSE;
	Passageiro p;
	TCHAR c[TAM_MSG];
	time(&dados->pass.inicial);

	WriteFile(dados->hPipe, &dados->pass, sizeof(Passageiro), &nBytes, NULL);

	do {

		ret = ReadFile(dados->hPipe, &p, sizeof(Passageiro), &nBytes, NULL);

		dados->pass.adicionado = p.adicionado;

		if (!ret || !nBytes || p.adicionado == 0 || p.voo == 0) { //se controlador sair, se areoportos não existirem, ou se chegou ao destino sai
			sair = TRUE;
		}

		if (dados->pass.adicionado == -2) { //acabou o tempo
			_tprintf(TEXT("Passageiro desistiu da viagem (acabou o tempo). Pressione qualquer tecla para sair.\n"));
			WriteFile(dados->hPipe, &dados->pass, sizeof(Passageiro), &nBytes, NULL);
			_fgetts(c, TAM_MSG, stdin);
			sair = TRUE;
		}

		if (p.voo == -1) { //chegou ao destino
			_tprintf(TEXT("Passageiro chegou ao destino. Pressione qualquer tecla para sair.\n"));
			dados->pass.adicionado = -2;
			WriteFile(dados->hPipe, &dados->pass, sizeof(Passageiro), &nBytes, NULL);
			_fgetts(c, TAM_MSG, stdin);
			p.voo = 0;
			sair = TRUE;
		}

		if (p.embarque == 1) {
			_tprintf(TEXT("Passageiro embarcou no Avião\n"));
		}

		if (p.voo == 1) {
			_tprintf(TEXT("Coordenadas -> [%d,%d]\n"), p.pos_x, p.pos_y);
		}


	} while (!sair);

}

int _tmain(int argc, TCHAR* argv[]) {
	HANDLE hThreadLeituraPassageiros;
	int i = 0;
	DATA dados;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	_tprintf(TEXT("INICIO DO PROCESSO PASSAGEIRO\n\n"));

	if (argc > 5 || argc < 4) {
		_ftprintf(stderr, TEXT("Erro a criar passageiro.\n"));
		return 1;
	}
	else {
		_tcscpy_s(dados.pass.aero_partida, _tcslen(argv[1]) * sizeof(TCHAR), argv[1]);
		_tcscpy_s(dados.pass.aero_chegada, _tcslen(argv[2]) * sizeof(TCHAR), argv[2]);
		_tcscpy_s(dados.pass.nome, _tcslen(argv[3]) * sizeof(TCHAR), argv[3]);
		if (argc == 5)
			dados.pass.tempo_espera = (float)_ttoi(argv[4]);
		else
			dados.pass.tempo_espera = -1;
	}

	dados.pass.adicionado = -1;
	dados.pass.embarque = 0;

	_tprintf(TEXT("Identificação: %s.\n"), dados.pass.nome);
	_tprintf(TEXT("Aeroporto de partida: %s.\n"), dados.pass.aero_partida);
	_tprintf(TEXT("Aeroporto de chegada: %s.\n"), dados.pass.aero_chegada);
	if(dados.pass.tempo_espera != -1)
		_tprintf(TEXT("Tempo de espera: %.0f.\n"), dados.pass.tempo_espera);


	if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) { //PIPE NÂO FOI CRIADO PELO CONTROL - controlador não existe
		_ftprintf(stderr, TEXT("Erro ao ligar ao Pipe.\n"));
		return 2;
	}

	dados.hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (dados.hPipe == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateFile.\n"));
		return 3;
	}

	_tprintf(TEXT("Ligação ao Controlador efectuada com sucesso!\n"));

	hThreadLeituraPassageiros = CreateThread(NULL, 0, ThreadPipesPassageiros, &dados, 0, NULL);
	if (hThreadLeituraPassageiros == NULL) {
		_ftprintf(stderr, TEXT("Erro no CreateThread.\n"));
		CloseHandle(dados.hPipe);
		return 4;
	}
	else
		WaitForSingleObject(hThreadLeituraPassageiros, INFINITE);

	CloseHandle(dados.hPipe);

	return 0;

}
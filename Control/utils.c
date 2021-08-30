#include "control.h"
#include "utils.h"

void fecha_handles(DATA* dados) {
	dados->terminar = 1;
	if(dados->hMutex == NULL)
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

void lista_passageiros(Passageiro* passageiros, DWORD n_pass) {
	DWORD i;
	_tprintf(TEXT("LISTAGEM DE PASSAGEIROS:\n"));
	for (i = 0; i < n_pass; i++) {
		if (passageiros[i].adicionado == 1) {
			_tprintf(TEXT("Nome do passageiro: %s\n"), passageiros[i].nome);
			_tprintf(TEXT("Aeroporto de partida: %s\n"), passageiros[i].aero_partida);
			_tprintf(TEXT("Aeroporto de chegada: %s\n"), passageiros[i].aero_chegada);
			_tprintf(TEXT("Em voo: %d\n"), passageiros[i].voo);
			_tprintf(TEXT("\n"));
		}
	}
}

int verifica_nomeAeroportoPassageiro(Aeroporto* aeroportos, DWORD n_aero, TCHAR partida[MAX], TCHAR chegada[MAX]) {
	DWORD i, j;

	for (i = 0; i < n_aero; i++) {
		if (aeroportos[i].adicionado == 1) {
			if (!(_tcsicmp(aeroportos[i].nome, partida))) {
				for (j = 0; j < n_aero; j++) {
					if (aeroportos[j].adicionado == 1) {
						if (!(_tcsicmp(aeroportos[j].nome, chegada))) {
							(aeroportos[i].cap_atual)++;
							return 1;
						}
					}
				}
			}
		}
	}
	return 0;
}

Coords retorna_coordenadasAeroporto(Aeroporto* aeroportos, DWORD n_aero, Aviao aviao) {
	Coords c;
	DWORD i;

	for (i = 0; i < n_aero; i++) {
		if (aeroportos[i].adicionado == 1) {
			if (!(_tcsicmp(aeroportos[i].nome, aviao.aeroporto_partida))) {
				c.coords[0][0] = aeroportos[i].pos_x;
				c.coords[0][1] = aeroportos[i].pos_y;
			}
		}
	}

	for (i = 0; i < n_aero; i++) {
		if (aeroportos[i].adicionado == 1) {
			if (!(_tcsicmp(aeroportos[i].nome, aviao.aeroporto_chegada))) {
				c.coords[1][0] = aeroportos[i].pos_x;
				c.coords[1][1] = aeroportos[i].pos_y;
			}
		}
	}

	return c;

}

int cria_aeroportos(Aeroporto* aeroportos, DWORD n_aero, TCHAR nome[MAX], int x, int y) {
	DWORD i;
	int r = 10;

	for (i = 0; i < n_aero; i++) {
		if (aeroportos[i].adicionado == 1) {
			if (((x - aeroportos[i].pos_x) * (x - aeroportos[i].pos_x) + (y - aeroportos[i].pos_y) * (y - aeroportos[i].pos_y)) < r * r) {
				return 1; //Já existe um aeroporto no raio de 10 unidades!
			}
		}
	}

	for (i = 0; i < n_aero; i++) {
		if (aeroportos[i].adicionado == 1) {
			if (!(_tcsicmp(aeroportos[i].nome, nome))) {
				return 2; //Já existe um aeroporto com o mesmo nome!
			}
		}
	}

	for (i = 0; i < n_aero; i++) {
		if (aeroportos[i].adicionado != 1) {
			_tcscpy_s(aeroportos[i].nome, MAX, nome);
			aeroportos[i].pos_x = x;
			aeroportos[i].pos_y = y;
			aeroportos[i].cap_atual = 0;
			aeroportos[i].cap_avioes = 0;
			aeroportos[i].adicionado = 1;
			return 0;
		}
	}

	return 3; //Atingiu o limite máximo de aeroportos!
}

int adiciona_aviao(Aeroporto* aeroportos, DWORD n_aero, Aviao* avioes, DWORD n_avioes, Aviao aviao) {
	DWORD i, j;
	TCHAR nd[] = TEXT("«Não definido»");

	for (i = 0; i < n_avioes; i++) {
		if (avioes[i].adicionado != 1) {
			for (j = 0; j < n_aero; j++) {
				if (!(_tcsicmp(aeroportos[j].nome, aviao.aeroporto_partida))) {
					avioes[i].id = aviao.id;
					_tcscpy_s(avioes[i].aeroporto_partida, _tcslen(aviao.aeroporto_partida) * sizeof(TCHAR), aviao.aeroporto_partida);
					_tcscpy_s(avioes[i].aeroporto_chegada, _tcslen(nd) * sizeof(TCHAR), nd);
					avioes[i].cap_maxima = aviao.cap_maxima;
					avioes[i].cap_atual = 0;
					avioes[i].coordenadas.coords[0][0] = aeroportos[j].pos_x;
					avioes[i].coordenadas.coords[0][1] = aeroportos[j].pos_y;
					avioes[i].voo = -1;
					avioes[i].embarcar = -1;
					avioes[i].adicionado = 1;
					for (j = 0; j < N_PASSAGEIROS; j++)
						avioes[i].num_pass[j] = -1;
					return 1;
				}
			}
			return 0;
		}
	}
	return 0;
}

int adiciona_passageiro(Passageiro* passageiros, DWORD n_pass, Passageiro p, int num_pipe) {
	DWORD i;

	for (i = 0; i < n_pass; i++) {
		if (passageiros[i].adicionado != 1) {
			passageiros[i].adicionado = 1;
			passageiros[i].id = num_pipe;
			_tcscpy_s(passageiros[i].nome, _tcslen(p.nome) * sizeof(TCHAR), p.nome);
			_tcscpy_s(passageiros[i].aero_partida, _tcslen(p.aero_partida) * sizeof(TCHAR), p.aero_partida);
			_tcscpy_s(passageiros[i].aero_chegada, _tcslen(p.aero_chegada) * sizeof(TCHAR), p.aero_chegada);
			passageiros[i].voo = 0;
			passageiros[i].tempo_espera = p.tempo_espera;
			if (p.tempo_espera != -1) {
				passageiros[i].inicial = p.inicial;
			}
				
			return 1;
		}
	}
	return 0;
}

int verifica_embarqueEadiciona(Aeroporto* aeroportos, DWORD n_aero, TCHAR nome[MAX], Aviao* avioes, DWORD n_avioes, Aviao aviao) {
	DWORD i, j;

	if (!(_tcsicmp(aviao.aeroporto_chegada, aviao.aeroporto_partida))) //apanhou um aeroporto inválido (partida == destino)
		return 0;

	for (i = 0; i < n_aero; i++) {
		if (aeroportos[i].adicionado == 1) {
			if (!(_tcsicmp(aeroportos[i].nome, nome))) {
				for (j = 0; j < n_avioes; j++) {
					if (avioes[j].id == aviao.id) {
						_tcscpy_s(avioes[j].aeroporto_chegada, _tcslen(nome) * sizeof(TCHAR), nome);
						avioes[j].coordenadas.coords[0][0] = avioes[j].coordenadas.coords[1][0];
						avioes[j].coordenadas.coords[0][1] = avioes[j].coordenadas.coords[1][1];
						avioes[j].coordenadas.coords[1][0] = aeroportos[i].pos_x;
						avioes[j].coordenadas.coords[1][1] = aeroportos[i].pos_y;
						avioes[j].coordenadas.coords[2][0] = -10;
						avioes[j].coordenadas.coords[2][1] = -10;
						avioes[j].embarcar = 0;
						return 1; // pronto para embarque
					}
				}
			}
		}
	}
	return 0;
}

void actualiza_embarque(DATA* dados, Aviao aviao){
	DWORD i, j, k, l;
	Passageiro p;

	for (i = 0; i < dados->n_avioes; i++) {
		if (dados->Avi_Controlo[i].id == aviao.id) {
			dados->Avi_Controlo[i].embarcar = 2;
			if (dados->Avi_Controlo[i].cap_atual == dados->Avi_Controlo[i].cap_maxima)
				return;
			for (j = 0; j < dados->n_pass; j++) {
				if (!(_tcsicmp(dados->Avi_Controlo[i].aeroporto_partida, dados->Pass_Controlo[j].aero_partida))) {
					if (!(_tcsicmp(dados->Avi_Controlo[i].aeroporto_chegada, dados->Pass_Controlo[j].aero_chegada))) {
						if (dados->Pass_Controlo[j].embarque != 1 && dados->Pass_Controlo[j].adicionado == 1) {
							for (k = 0; k < dados->n_pass; k++) {
								if (dados->Avi_Controlo[i].num_pass[k] == -1) {
									dados->Avi_Controlo[i].num_pass[k] = dados->Pass_Controlo[j].id;
									(dados->Avi_Controlo[i].cap_atual)++;
									p.embarque = 1;
									dados->Pass_Controlo[j].embarque = 1;
									WriteFile(dados->hPipes[dados->Pass_Controlo[j].id].hPipe, &p, sizeof(Passageiro), NULL, NULL);
									if (dados->Avi_Controlo[i].cap_atual == dados->Avi_Controlo[i].cap_maxima)
										return;
									break;
								}
							}
							for (l = 0; l < dados->n_aero; l++) {
								if (!(_tcsicmp(dados->Avi_Controlo[i].aeroporto_partida, dados->Aero_Controlo[l].nome))) {
									(dados->Aero_Controlo[l].cap_atual)--;
									break;
								}
							}
						}
					}
				}
			}
			return;
		}
	}
}

void actualiza_voo(Aviao* avioes, DWORD n_avioes, Aviao aviao, Aeroporto* aeroportos, DWORD n_aero){
	DWORD i, j;

	for (i = 0; i < n_avioes; i++) {
		if (avioes[i].id == aviao.id) {
			avioes[i].voo = 1;
			for (j = 0; j < n_aero; j++) {
				if (!(_tcsicmp(aeroportos[j].nome, aviao.aeroporto_partida))) {
					(aeroportos[j].cap_avioes)--;
					return;
				}
			}
		}
	}
}

void desembarque_aviao(DATA* dados, Aviao aviao) {
	DWORD i, j, k;
	Passageiro p;
	p.voo = -1;

	for (i = 0; i < dados->n_avioes; i++) {
		if (dados->Avi_Controlo[i].id == aviao.id) {
			for (k = 0; k < dados->n_aero; k++) {
				if (!(_tcsicmp(dados->Aero_Controlo[k].nome, dados->Avi_Controlo[i].aeroporto_chegada))) {
					(dados->Aero_Controlo[k].cap_avioes)++;
					break;
				}
			}
			_tcscpy_s(dados->Avi_Controlo[i].aeroporto_partida, _tcslen(dados->Avi_Controlo[i].aeroporto_partida) * sizeof(TCHAR), dados->Avi_Controlo[i].aeroporto_chegada);
			_tcscpy_s(dados->Avi_Controlo[i].aeroporto_chegada, _tcslen(TEXT("«Não definido»")) * sizeof(TCHAR), TEXT("«Não definido»"));
			dados->Avi_Controlo[i].coordenadas.coords[2][0] = -1;
			dados->Avi_Controlo[i].coordenadas.coords[2][1] = -1;
			dados->Avi_Controlo[i].embarcar = -1;
			dados->Avi_Controlo[i].voo = -1;
			dados->Avi_Controlo[i].cap_atual = 0;
			for (j = 0; j < dados->n_pass; j++) {
				if (dados->Avi_Controlo[i].num_pass[j] != -1) {
					WriteFile(dados->hPipes[dados->Avi_Controlo[i].num_pass[j]].hPipe, &p, sizeof(Passageiro), NULL, NULL);
					dados->Pass_Controlo[dados->Avi_Controlo[i].num_pass[j]].adicionado = -1; //remover da lista
					dados->Avi_Controlo[i].num_pass[j] = -1; //remover do avião
				}
			}
			return;
		}
	}
}

void actualiza_coordenadas(DATA* dados, Mem_Aviao* mem_aviao) {
	DWORD i, j;
	Passageiro p;

	for (i = 0; i < dados->n_avioes; i++) {
		if (dados->Avi_Controlo[i].id == mem_aviao->id) {
			dados->Avi_Controlo[i].coordenadas.coords[2][0] = mem_aviao->cap[0];
			dados->Avi_Controlo[i].coordenadas.coords[2][1] = mem_aviao->cap[1];
			for (j = 0; j < dados->n_pass; j++) {
				if (dados->Avi_Controlo[i].num_pass[j] != -1) {
					p.embarque = 2;
					p.voo = 1;
					p.pos_x = dados->Avi_Controlo[i].coordenadas.coords[2][0];
					p.pos_y = dados->Avi_Controlo[i].coordenadas.coords[2][1];
					WriteFile(dados->hPipes[dados->Avi_Controlo[i].num_pass[j]].hPipe, &p, sizeof(Passageiro), NULL, NULL);
				}
			}
			return;
		}
	}
}

void update_temporizador(Aviao* avioes, DWORD n_avioes, Aviao aviao) {
	DWORD i;

	for (i = 0; i < n_avioes; i++) {
		if (avioes[i].id == aviao.id) {
			avioes[i].temporizador = time(&avioes[i].temporizador);
		}
	}
	return;
}

void verifica_temporizadorAviao(DATA* dados) {
	DWORD i, j;
	time_t t_actual;
	float diferencial;
	Passageiro p;

	for (i = 0; i < dados->n_avioes; i++) {
		if (dados->Avi_Controlo[i].adicionado == 1) {
			time(&t_actual);
			diferencial = (float)difftime(t_actual, dados->Avi_Controlo[i].temporizador);
			if (diferencial > 3) {
				dados->Avi_Controlo[i].adicionado = -1;
				InvalidateRect(dados->hWnd, NULL, FALSE);
				if (dados->Avi_Controlo[i].voo != 1) { //está aterrado
					for (j = 0; j < dados->n_aero; j++) {
						if (!(_tcsicmp(dados->Avi_Controlo[i].aeroporto_partida, dados->Aero_Controlo[j].nome))) {
							(dados->Aero_Controlo[j].cap_avioes)--;
						}
					}
				}
				if (dados->Avi_Controlo[i].cap_atual > 0) {
					for (j = 0; j < dados->n_pass; j++) {
						if (dados->Avi_Controlo[i].num_pass[j] != -1) {
							p.voo = 0;
							p.adicionado = -2;
							WriteFile(dados->hPipes[dados->Avi_Controlo[i].num_pass[j]].hPipe, &p, sizeof(Passageiro), NULL, NULL);
							dados->Pass_Controlo[dados->Avi_Controlo[i].num_pass[j]].adicionado = -1; //remover da lista
						}
					}
				}
			}
		}
	}
	return;
}

void verifica_temporizadorPassageiro(DATA* dados) {
	DWORD i, k;
	time_t t_actual;
	float diferencial;
	Passageiro p;
	time(&t_actual);

	for (i = 0; i < dados->n_pass; i++) {
		if (dados->Pass_Controlo[i].adicionado == 1 && dados->Pass_Controlo[i].tempo_espera != -1 && dados->Pass_Controlo[i].embarque != 1) {
			diferencial = (float)difftime(t_actual, dados->Pass_Controlo[i].inicial);
			if (diferencial > dados->Pass_Controlo[i].tempo_espera) {
				for (k = 0; k < dados->n_aero; k++) {
					if (!(_tcsicmp(dados->Aero_Controlo[k].nome, dados->Pass_Controlo[i].aero_partida)))
						(dados->Aero_Controlo[k].cap_atual)--;
				}
				dados->Pass_Controlo[i].adicionado = -1;
				p.adicionado = -2;
				WriteFile(dados->hPipes[dados->Pass_Controlo[i].id].hPipe, &p, sizeof(Passageiro), NULL, NULL);
			}
		}
	}

	return;
}

int verifica_existencia_avioes(Aviao* avioes, DWORD n_avioes) {
	DWORD i;

	for (i = 0; i < n_avioes; i++) {
		if (avioes[i].adicionado == 1) {
			return 0;
		}
	}
	return 1;
}

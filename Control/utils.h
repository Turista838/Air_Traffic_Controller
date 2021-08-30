#ifndef UTILS_H
#define UTILS_H

#define MAX 256

void fecha_handles(DATA* dados);
void lista_passageiros(Passageiro* passageiros, DWORD n_pass);
int verifica_nomeAeroportoPassageiro(Aeroporto* aeroportos, DWORD n_aero, TCHAR partida[MAX], TCHAR chegada[MAX]);
Coords retorna_coordenadasAeroporto(Aeroporto* aeroportos, DWORD n_aero, Aviao aviao);
int cria_aeroportos(Aeroporto* aeroportos, DWORD n_aero, TCHAR nome[MAX], int x, int y);
int adiciona_aviao(Aeroporto* aeroportos, DWORD n_aero, Aviao* avioes, DWORD n_avioes, Aviao aviao);
int adiciona_passageiro(Passageiro* passageiros, DWORD n_pass, Passageiro p, int num_pipe);
int verifica_embarqueEadiciona(Aeroporto* aeroportos, DWORD n_aero, TCHAR nome[MAX], Aviao* avioes, DWORD n_avioes, Aviao aviao);
void actualiza_embarque(DATA* dados, Aviao aviao);
void actualiza_voo(Aviao* avioes, DWORD n_avioes, Aviao aviao, Aeroporto* aeroportos, DWORD n_aero);
void desembarque_aviao(DATA* dados, Aviao aviao);
void actualiza_coordenadas(DATA* dados, Mem_Aviao* mem_aviao);
void verifica_temporizadorAviao(DATA* dados);
void verifica_temporizadorPassageiro(DATA* dados);
void update_temporizador(Aviao* avioes, DWORD n_avioes, Aviao aviao);
int verifica_existencia_avioes(Aviao* avioes, DWORD n_avioes);

LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK AdicionaAeroportos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ListaAeroportos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ListaAvioes(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ListaPassageiros(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK AdicionaChaves(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK About(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);

#endif

#ifndef CONTROL_H
#define CONTROL_H

#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <psapi.h>
#include <memoryapi.h>
#include <time.h>
#include <windowsx.h>

#define TAM_MSG 512
#define TAM_BUFFER_C 10
#define PIPE_NAME TEXT("\\\\.\\pipe\\TPSO2")
#define N_PASSAGEIROS 64

typedef struct { //PASSAGEIROS
    int adicionado; //flag adicionado
    int id; // id numérico do passageiro (numero do pipe)
    TCHAR nome[TAM_MSG]; //nome do passageiro
    TCHAR aero_partida[TAM_MSG]; // nome do aeroporto de partida
    TCHAR aero_chegada[TAM_MSG]; // nome do aeroporto de chegada
    float tempo_espera; // tempo de espera
    time_t inicial; //hora de entrada no aeroporto
    int voo; // flag para determinar se está em voo
    int embarque; // flag para determinar se embarcou
    int pos_x; // posição x
    int pos_y; // posição y
} Passageiro;

typedef struct { //AEROPORTOS
    TCHAR nome[TAM_MSG]; // nome do aeroporto
    int cap_atual; // número de passageiros actual
    int cap_avioes; //número de aviões estacionados
    int pos_x; // posição x
    int pos_y; // posição y
    int adicionado; // flag para determinar se está adicionado
} Aeroporto;

typedef struct { //COORDENADAS PARA OS AVIÕES (aeroporto partida, chegada, pos actual)
    int coords[3][2]; // [0][0] pos x partida | [0][1] pos y partida | [1][0] pos x chegada | [1][1] pos y chegada | [2][0] pos x actual | [2][1] pos y actual
} Coords;

typedef struct { //AVIÕES
    int id; // id numérico do avião (pid)
    int cap_maxima; // número de capacidade máxima
    int cap_atual; // número de passageiros actual
    TCHAR aeroporto_partida[TAM_MSG]; //nome aeroporto partida
    TCHAR aeroporto_chegada[TAM_MSG]; //nome aeroporto chegada
    int voo; // flag para determinar se está em voo
    int embarcar; // flag para embarcar passageiros
    int adicionado; // flag para determinar se está adicionado
    Coords coordenadas; //coordenadas do avião
    time_t temporizador; //temporizador do sinal
    int num_pass[N_PASSAGEIROS];
} Aviao;

typedef struct { //AVIÕES MEMÓRIA PARTILHADA
    int id; // id numérico do avião (pid)
    int cap[2]; // coordenadas ocupadas 
    int m[1000][1000]; //mapa
} Mem_Aviao;

typedef struct { //AVIÕES BUFFER CIRCULAR
    int posE; //posição escrita
    int posL; //posição leitura
    Aviao av[TAM_BUFFER_C];
} Buffer_Aviao;

typedef struct {
    HANDLE hPipe;
    OVERLAPPED overlap;
    BOOL activo;
}PipeDados;

typedef struct { //ESTRUTURA PRINCIPAL DE DADOS
    DWORD n_avioes, n_aero, n_pass;
    Buffer_Aviao* pB_Aviao;
    Aviao aviao;
    Aviao* pM_Aviao_out;
    Mem_Aviao* pM_Aviao_in;
    Aeroporto* Aero_Controlo;
    Aviao* Avi_Controlo;
    Passageiro* Pass_Controlo;
    HANDLE hMutex, hSemEscrita, hSemLeitura, hEvent_A_C1, hEvent_A_C2, hEvent_C_A;
    HWND hWnd;
    HBITMAP hBitmapDB;
    HDC memDC;
    BITMAP bmpAero, bmpMapa, bmpAvi;
    HDC bmpDCAero, bmpDCMapa, bmpDCAvi;
    HKEY chave;
    PipeDados hPipes[N_PASSAGEIROS];
    HANDLE hEvents[N_PASSAGEIROS];
    int aceita_avioes, terminar;
}DATA;

#endif


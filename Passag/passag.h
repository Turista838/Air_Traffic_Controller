#ifndef PASSAG_H
#define PASSAG_H

#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <psapi.h>
#include <memoryapi.h>
#include <time.h>

#define TAM_MSG 512
#define PIPE_NAME TEXT("\\\\.\\pipe\\TPSO2")

typedef struct { //PASSAGEIROS
    int adicionado;
    int id; // id numérico do passageiro (numero do pipe)
    TCHAR nome[TAM_MSG];
    TCHAR aero_partida[TAM_MSG]; // nome do aeroporto de partida
    TCHAR aero_chegada[TAM_MSG]; // nome do aeroporto de chegada
    float tempo_espera; // tempo de espera
    time_t inicial;
    int voo; // flag para determinar se está em voo
    int embarque; // flag para determinar se embarcou
    int pos_x; // posição x
    int pos_y; // posição y
} Passageiro;

typedef struct { //ESTRUTURA PRINCIPAL DE DADOS
    Passageiro pass;
    HANDLE hMutex, hPipe;
    int terminar;
}DATA;

#endif

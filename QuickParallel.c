#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "timer.h"
//muda o item do array a para a posição do b
#define swap(a, b) { int _h = a; a = b; b = _h; }
#define TAM (100000000)
// verifica entre o pivot, esq e direita e os ordena
#define ordena3(a, b, c)              \
    if (b < a) {                        \
        if (c < a) {                    \
            if (c < b) { swap(a, c); }  \
            else { int h = a; a = b; b = c; c = h;} \
        }                               \
        else { swap(a, b); }            \
    }                                   \
    else {                              \
        if (c < b) {                    \
            if (c < a) { int h = c; c = b; b = a; a = h;} \
            else { swap(b, c); }        \
        }                               \
    }                  
int data[TAM];
double ini, fim;

int max_threads;
int n_threads;

pthread_mutex_t mutex;
pthread_cond_t cond;

void quicksort(int* esq, int* dir);
/*
    insere as ordenações no array
*/
void insert_sort(int* esq, int* dir) {

    // coloca o menor na esquerda para poder começar  comparação do insert_sort
    for (int* pi = esq + 1; pi <= dir; pi++) {
        if (*pi < *esq) {
            swap(*pi, *esq);
        }
    }
    // compara com todos a partir do mais a esquerda os que são menores
    for (int* pi = esq + 2; pi <= dir; pi++) {
        int h = *pi;
        int* pj = pi - 1;
        while (h < *pj) {
            *(pj + 1) = *pj;
            pj -= 1;
        }
        *(pj + 1) = h;
    }
}
/*
    realiza as partições do quicksort
*/
void partition(int* esq0, int* dir0, int** e1, int** d1, int** e2, int** d2) {

    int* esq = esq0 + 1;
    int* dir = dir0;

    int* mid = esq0 + (dir0 - esq0) / 2;    
    int piv = *mid;
    *mid = *esq;
    ordena3(*esq0, piv, *dir0);
    *esq = piv; 

    while (1) {
        do esq += 1; while(*esq < piv);
        do dir -= 1; while (*dir > piv);
        if (esq >= dir) break;
        swap(*esq, *dir);
    }
    *(esq0 + 1) = *dir;
    *dir = piv;

    if (dir < mid) {
        *e1 = esq0; *d1 = dir - 1;
        *e2 = dir + 1; *d2 = dir0;
    }
    else {
        *e1 = dir + 1; *d1 = dir0;
        *e2 = esq0; *d2 = dir - 1;
    }
}
/*
    Função das threads e que chama o quicksort para ordenar
*/
void* thread_sort(void *arg) {
    int** par = (int**)arg;
    quicksort(par[0], par[1]);
    free(arg);
    pthread_mutex_lock(&mutex);
    n_threads -= 1;
    if (n_threads == 0) pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return NULL;
}
// quicksort sequencial
void quicksortnormal(int* esq, int* dir) {

    int *e, *d;
    while (dir - esq >= 50) {
        partition(esq, dir, &e, &d, &esq, &dir);
        quicksortnormal(e,d);
    }
    insert_sort(esq, dir);
}

/* 
    realiza o quicksort concorrente 
    chama uma nova thread para cada partição    
*/
void quicksort(int* esq, int* dir) {

    while (dir - esq >= 50) {
        int *e, *d;
        partition(esq, dir, &e, &d, &esq, &dir);

        if (dir - esq > 100000 && n_threads < max_threads) {
            pthread_t thread;
            int** param = malloc(2 * sizeof(int*));
            param[0] = esq;
            param[1] = dir;
            pthread_mutex_lock(&mutex);
            n_threads += 1;
            pthread_mutex_unlock(&mutex);
            pthread_create(&thread, NULL, thread_sort, param);
            esq = e;
            dir = d;
        }
        else {
            quicksort(e,d);
        }
    }
    insert_sort(esq, dir);
}
// função inicial para pedido de ordenação
void ordena(int* data, int len) {

    max_threads = 8;

    pthread_t thread;
    int** param = malloc(2 * sizeof(int*));
    param[0] = data;
    param[1] = data + len - 1;
    n_threads = 1;
    pthread_create(&thread, NULL, thread_sort, param);

    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);
}

//Função para inicializar o array
void init(int* data, int len) {

    for (int i = 0; i < len; i++) {
        data[i] = rand();
    }
}

// printar a lista
void print(int* data, int len) {
    for (int i = 0; i < len; i++) {
        printf("%d\n", data[i]);
    }
}

// Função para checar se está ordenado
bool isOrd(int * start, int * end)
{
	start++;
	if(start == end)
		return true;
	while(start != end)
	{
		if(*(start - 1) > *(start))
			return false;
		start++;  
	}
	return true;
}

int main(void) {

    init(data, TAM);
    int *data1 = data;
    // inicialização
	pthread_mutex_init(&mutex, NULL);
    pthread_cond_init (&cond, NULL);

    printf("Sorteando %d números com quicksort ...\n",
        TAM );
    GET_TIME(ini);
    ordena(data, TAM);
    GET_TIME(fim);
    // checando a ordenação
    if(isOrd(&data[0], &data[TAM]))
	{
		printf("Array está sorteado\n");
	}
	else
	{
		printf("Array não está sorteado\n");
	}

    printf("Tempo concorrente: %lf\n\n" , fim-ini);
    GET_TIME(ini);
    quicksortnormal(data1, data1 + TAM - 1);
    GET_TIME(fim);

    // checando a ordenação
    if(isOrd(&data[0], &data[TAM]))
	{
		printf("Array está sorteado\n");
	}
	else
	{
		printf("Array não está sorteado\n");
	}
    printf("Tempo sequencial: %lf\n" , fim-ini);
    return 0;
}
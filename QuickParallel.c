#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"
#define swap(a, b) { int _h = a; a = b; b = _h; }
#define min(a, b) ((a) < (b) ? (a) : (b))
#define TAM (50 * 1000000)
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

void partition(int* esq0, int* dir0, int** l1, int** r1, int** l2, int** r2) {

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
        *l1 = esq0; *r1 = dir - 1;
        *l2 = dir + 1; *r2 = dir0;
    }
    else {
        *l1 = dir + 1; *r1 = dir0;
        *l2 = esq0; *r2 = dir - 1;
    }
}

void* thread_sort(void *arg) {
    int** par = (int**)arg;
    printf("sou uma thread");
    quicksort(par[0], par[1]);
    free(arg);
    pthread_mutex_lock(&mutex);
    n_threads -= 1;
    if (n_threads == 0) pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void quicksort(int* esq, int* dir) {

    while (dir - esq >= 50) {
        int *l, *r;
        partition(esq, dir, &l, &r, &esq, &dir);

        if (dir - esq > 100000 && n_threads < max_threads) {
            // start a new thread - max_threads is a soft limit
            pthread_t thread;
            int** param = malloc(2 * sizeof(int*));
            param[0] = esq;
            param[1] = dir;
            pthread_mutex_lock(&mutex);
            n_threads += 1;
            pthread_mutex_unlock(&mutex);
            pthread_create(&thread, NULL, thread_sort, param);
            esq = l;
            dir = r;
        }
        else {
            quicksort(l, r);
        }
    }
    insert_sort(esq, dir);
}

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

void init(int* data, int len) {

    for (int i = 0; i < len; i++) {
        data[i] = rand();
    }
}

void test(int* data, int len) {
    for (int i = 1; i < len; i++) {
        if (data[i] < data[i - 1]) {
            printf("ERROR\n");
            break;
        }
    }
}

void print(int* data, int len) {
    for (int i = 0; i < len; i++) {
        printf("%d\n", data[i]);
    }
}

int main(void) {

    init(data, TAM);

	pthread_mutex_init(&mutex, NULL);
    pthread_cond_init (&cond, NULL);

    printf("Sorting %d million numbers with Quicksort ...\n",
        TAM / 1000000);
    GET_TIME(ini);
    ordena(data, TAM);
    GET_TIME(fim);
    test(data, TAM);
    printf("Tempo concorrente: %lf\n" , fim-ini);
    return 0;
}
#ifndef MATRIZ_H
#define MATRIZ_H

#include <stdio.h>

/*
 * matriz: camada de dados do Algoritmo de Banker.
 *
 * Este modulo e o unico dono de MAXIMUM, ALLOCATION, NEED e AVAILABLE.
 * Os demais modulos (algoritmo de seguranca, main) leem o estado pelas
 * funcoes get_* e o alteram somente pelas funcoes apply_* / rollback_*.
 *
 * Os indices de cliente (0..n-1) e de recurso (0..m-1) devem ser
 * validados por quem chama; ids invalidos sao ignorados (escritas) ou
 * retornam 0 (leituras).
 */

/* ---------- Inicializacao ---------- */

/* Define as dimensoes, aloca as estruturas e copia available_init
   (num_resources valores) para AVAILABLE. ALLOCATION, MAXIMUM e NEED
   comecam zeradas. Pode ser chamada de novo: libera o estado anterior. */
void init_matrices(int num_customers, int num_resources, int available_init[]);

/* Define a linha do cliente em MAXIMUM, zera sua linha em ALLOCATION
   e faz NEED = MAXIMUM. */
void set_customer_maximum(int customer_id, int max_resources[]);

/* ---------- Atualizacao de estado ---------- */

/* Aplica um pedido: AVAILABLE -= req; ALLOCATION += req; NEED -= req.
   Nao valida; use antes dos testes de need/available. */
void apply_request(int customer_id, int request[]);

/* Desfaz apply_request: AVAILABLE += req; ALLOCATION -= req; NEED += req. */
void rollback_request(int customer_id, int request[]);

/* Aplica uma liberacao: AVAILABLE += rel; ALLOCATION -= rel;
   NEED = MAXIMUM - ALLOCATION. Nao valida (rel <= allocation). */
void apply_release(int customer_id, int release[]);

/* ---------- Consulta (somente leitura) ---------- */

int get_num_customers(void);
int get_num_resources(void);
int get_available(int resource);
int get_maximum(int customer_id, int resource);
int get_allocation(int customer_id, int resource);
int get_need(int customer_id, int resource);

/* Ponteiro para o vetor AVAILABLE (num_resources valores), somente leitura. */
const int *get_available_vector(void);

/* ---------- Impressao ---------- */

/* Escreve em out a tabela MAXIMUM | ALLOCATION | NEED e a linha AVAILABLE. */
void print_state(FILE *out);

/* ---------- Limpeza ---------- */

/* Libera toda a memoria alocada. Pode ser chamada mais de uma vez. */
void free_matrices(void);

#endif /* MATRIZ_H */
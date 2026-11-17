#ifndef BANKER_H
#define BANKER_H

#include <stdbool.h>
#include <stdio.h>

/*
 * banker: modulo de decisao do Algoritmo de Banker.
 *
 * Nao guarda estado proprio: le e altera o estado exclusivamente pelas
 * funcoes do modulo matriz (get_*, apply_request, rollback_request,
 * apply_release). Todas as mensagens sao gravadas no FILE *out recebido
 * (normalmente result.txt).
 */

/* Algoritmo de Seguranca: true se existe uma sequencia segura em que todos
   os clientes conseguem terminar com os recursos disponiveis. */
bool is_safe_state(void);

/* Comando RQ. Retorna 0 se o pedido foi concedido, -1 se foi negado
   (estado inalterado) ou se customer_num e invalido. */
int request_resources(int customer_num, int request[], FILE *out);

/* Comando RL. Retorna 0 se a liberacao foi feita, -1 se foi recusada
   (estado inalterado) ou se customer_num e invalido. */
int release_resources(int customer_num, int release[], FILE *out);

#endif /* BANKER_H */
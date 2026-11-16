#include <stdlib.h>
#include <stdio.h>
/*
 * release_resources: processa o comando RL.
 *
 * Devolve ao sistema os recursos que o cliente customer_num esta liberando.
 *
 * Retorno:
 *   0  -> sucesso (estado atualizado)
 *  -1  -> erro (cliente invalido ou liberacao maior que a alocacao);
 *         nenhuma matriz e alterada
 */
int release_resources(int customer_num, int release[]) {
    /* 1. Valida o ID do cliente */
    if (customer_num < 0 || customer_num >= NUMBER_OF_CUSTOMERS)
        return -1;

    /* 2. Valida TODOS os recursos antes de alterar qualquer coisa,
          para nunca deixar o estado pela metade em caso de erro.
          Tambem rejeita valores negativos, que inflariam o need. */
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (release[i] < 0 || release[i] > allocation[customer_num][i])
            return -1;
    }

    /* 3. Validacao aprovada: atualiza o estado */
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        allocation[customer_num][i] -= release[i];   /* cliente tem menos */
        available[i]                += release[i];   /* sistema tem mais  */
        need[customer_num][i]       += release[i];   /* equivale a maximum - allocation */
    }

    return 0;
}
/*
 * request_resources: processa o comando RQ.
 *
 * Retorno:
 *   0  -> pedido concedido (estado atualizado)
 *  -1  -> pedido negado (mensagem de erro gravada em result_file);
 *         nenhuma matriz fica alterada
 *
 * Em caso de sucesso a funcao NAO imprime nada: a mensagem
 * "Allocate to customer X the resources ..." fica a cargo de quem chama.
 */
int request_resources(int customer_num, int request[]) {
    /* Validacao basica: cliente existente e valores nao negativos */
    if (customer_num < 0 || customer_num >= NUMBER_OF_CUSTOMERS)
        return -1;
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        if (request[i] < 0)
            return -1;

    /* Teste 1: request <= need (nao pode passar do maximo declarado) */
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > need[customer_num][i]) {
            fprintf(result_file, "The customer %d request ", customer_num);
            print_vec(result_file, request);
            fprintf(result_file, " was denied because exceed its maximum allocation\n");
            return -1;
        }
    }

    /* Teste 2: request <= available (o sistema precisa ter recursos livres) */
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > available[i]) {
            fprintf(result_file, "The resources ");
            print_vec(result_file, available);
            fprintf(result_file, " are not enough to customer %d request ", customer_num);
            print_vec(result_file, request);
            fprintf(result_file, "\n");
            return -1;
        }
    }

    /* Teste 3: simula a alocacao */
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i]                -= request[i];
        allocation[customer_num][i] += request[i];
        need[customer_num][i]       -= request[i];
    }

    if (is_safe_state())
        return 0;                       /* seguro: mantem as alteracoes */

    /* Inseguro: desfaz a simulacao */
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i]                += request[i];
        allocation[customer_num][i] -= request[i];
        need[customer_num][i]       += request[i];
    }
    fprintf(result_file, "The customer %d request ", customer_num);
    print_vec(result_file, request);
    fprintf(result_file, " was denied because result in an unsafe state\n");
    return -1;
}
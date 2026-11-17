#include <stdlib.h>
#include <string.h>
#include "banker.h"
#include "matriz.h"

/* ---------- Auxiliar de saida ---------- */

/* Grava um vetor de recursos em out como "a b c" (sem espaco no fim). */
static void print_vec(FILE *out, const int v[], int count) {
    for (int j = 0; j < count; j++)
        fprintf(out, j ? " %d" : "%d", v[j]);
}

/* ---------- Algoritmo de Seguranca ---------- */

bool is_safe_state(void) {
    const int n = get_num_customers();
    const int m = get_num_resources();

    int  *work   = malloc((size_t)m * sizeof(int));
    bool *finish = calloc((size_t)n, sizeof(bool));       /* tudo false */
    if (!work || !finish) {                  /* sem memoria: trata como inseguro */
        free(work);
        free(finish);
        return false;
    }

    /* Passo 1: work = available */
    for (int j = 0; j < m; j++)
        work[j] = get_available(j);

    /* Passos 2 e 3: procura um cliente nao finalizado com need <= work;
       ao achar, simula a devolucao de tudo que ele tem (work += allocation).
       Repete ate nenhum cliente poder terminar. */
    bool found = true;
    while (found) {
        found = false;
        for (int i = 0; i < n; i++) {
            if (finish[i]) continue;

            bool can_finish = true;
            for (int j = 0; j < m; j++) {
                if (get_need(i, j) > work[j]) { can_finish = false; break; }
            }

            if (can_finish) {
                for (int j = 0; j < m; j++)
                    work[j] += get_allocation(i, j);
                finish[i] = true;
                found = true;
            }
        }
    }

    /* Passo 4: seguro somente se todos os clientes terminaram */
    bool safe = true;
    for (int i = 0; i < n; i++) {
        if (!finish[i]) { safe = false; break; }
    }

    free(work);
    free(finish);
    return safe;
}

/* ---------- RQ: requisicao de recursos ---------- */

int request_resources(int customer_num, int request[], FILE *out) {
    const int n = get_num_customers();
    const int m = get_num_resources();

    /* Entrada invalida: cliente inexistente ou valor negativo.
       O main ja valida isso na leitura do commands.txt; aqui e so defesa. */
    if (customer_num < 0 || customer_num >= n) return -1;
    for (int j = 0; j < m; j++)
        if (request[j] < 0) return -1;

    /* Validacao 1: request <= need (nao pode passar do maximo declarado) */
    for (int j = 0; j < m; j++) {
        if (request[j] > get_need(customer_num, j)) {
            fprintf(out, "The customer %d request ", customer_num);
            print_vec(out, request, m);
            fprintf(out, " was denied because exceed its maximum allocation\n");
            return -1;
        }
    }

    /* Validacao 2: request <= available (precisa haver recursos livres) */
    for (int j = 0; j < m; j++) {
        if (request[j] > get_available(j)) {
            fprintf(out, "The resources ");
            print_vec(out, get_available_vector(), m);
            fprintf(out, " are not enough to customer %d request ", customer_num);
            print_vec(out, request, m);
            fprintf(out, "\n");
            return -1;
        }
    }

    /* Simulacao: aplica o pedido e testa se o estado continua seguro */
    apply_request(customer_num, request);

    if (is_safe_state()) {                   /* seguro: a alocacao fica */
        fprintf(out, "Allocate to customer %d the resources ", customer_num);
        print_vec(out, request, m);
        fprintf(out, "\n");
        return 0;
    }

    /* Inseguro: desfaz a simulacao e nega o pedido */
    rollback_request(customer_num, request);
    fprintf(out, "The customer %d request ", customer_num);
    print_vec(out, request, m);
    fprintf(out, " was denied because result in an unsafe state\n");
    return -1;
}

/* ---------- RL: liberacao de recursos ---------- */

int release_resources(int customer_num, int release[], FILE *out) {
    const int n = get_num_customers();
    const int m = get_num_resources();

    if (customer_num < 0 || customer_num >= n) return -1;
    for (int j = 0; j < m; j++)
        if (release[j] < 0) return -1;

    /* Validacao: release <= allocation (so devolve o que realmente tem).
       Checa todos os recursos antes de alterar qualquer coisa. */
    for (int j = 0; j < m; j++) {
        if (release[j] > get_allocation(customer_num, j)) {
            fprintf(out, "The customer %d release ", customer_num);
            print_vec(out, release, m);
            fprintf(out, " was denied because exceed its allocation\n");
            return -1;
        }
    }

    /* Atualizacao: available +=, allocation -=, need = maximum - allocation */
    apply_release(customer_num, release);

    fprintf(out, "Release from customer %d the resources ", customer_num);
    print_vec(out, release, m);
    fprintf(out, "\n");
    return 0;
}

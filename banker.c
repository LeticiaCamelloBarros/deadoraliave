#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
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

/* ================================================================
   Inicializacao do sistema (argumentos + customer.txt)
   ================================================================ */

/* Le uma linha de f para *buf (cresce conforme preciso), sem usar getline
   (que nao existe em todos os ambientes, como o MinGW).
   Retorna 1 se leu uma linha, 0 no fim do arquivo, -1 sem memoria. */
static int read_line(char **buf, size_t *cap, FILE *f) {
    if (!*buf) {
        *cap = 128;
        *buf = malloc(*cap);
        if (!*buf) return -1;
    }
    size_t len = 0;
    (*buf)[0] = '\0';
    for (;;) {
        if (!fgets(*buf + len, (int)(*cap - len), f)) {
            if (len == 0) return 0;               /* fim do arquivo */
            break;                                /* ultima linha sem \n */
        }
        len += strlen(*buf + len);
        if (len > 0 && (*buf)[len - 1] == '\n') break;
        if (len + 1 >= *cap) {                    /* buffer cheio: dobra */
            char *tmp = realloc(*buf, *cap * 2);
            if (!tmp) return -1;
            *buf = tmp;
            *cap *= 2;
        }
    }
    return 1;
}

/* Remove espacos, \r e \n nas pontas (in place) */
static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) *--end = '\0';
    return s;
}

/* Converte s em inteiro >= 0 usando atoi, mas so depois de checar que s
   tem apenas digitos (atoi sozinho aceita lixo como "abc" -> 0).
   Retorna 0 em sucesso, -1 se invalido. */
static int parse_nonneg(const char *s, int *out) {
    if (!s || !*s || strlen(s) > 9) return -1;   /* ate 9 digitos: sem overflow */
    for (const char *p = s; *p; p++)
        if (!isdigit((unsigned char)*p)) return -1;
    *out = atoi(s);
    return 0;
}

/* Libera uma lista de linhas temporarias */
static void free_rows(int **rows, int count) {
    for (int i = 0; i < count; i++) free(rows[i]);
    free(rows);
}

/*
 * init_system: configura o estado inicial do Banker.
 *
 *  - argv[1..argc-1] -> AVAILABLE (NUMBER_OF_RESOURCES = argc - 1)
 *  - customer_filename -> MAXIMUM (uma linha por cliente, valores separados
 *    por virgula; NUMBER_OF_CUSTOMERS = numero de linhas)
 *  - ALLOCATION = 0 e NEED = MAXIMUM (feito pelo modulo matriz)
 *
 * Retorna 0 em sucesso. Em erro, imprime mensagem em stderr, nao deixa
 * estado parcial no modulo matriz e retorna -1.
 */
int init_system(int argc, char *argv[], const char *customer_filename) {
    int *avail = NULL;
    int **rows = NULL;
    int n = 0, cap = 8;
    FILE *f = NULL;
    char *line = NULL;
    size_t len = 0;

    /* Passo 1: recursos iniciais a partir dos argumentos */
    if (argc < 2) {
        fprintf(stderr, "Erro: argumentos insuficientes. Uso: %s <inst_0> <inst_1> ...\n",
                argc > 0 ? argv[0] : "banker");
        return -1;
    }
    int m = argc - 1;                                  /* NUMBER_OF_RESOURCES */
    avail = malloc((size_t)m * sizeof(int));
    if (!avail) { fprintf(stderr, "Erro: falha de alocacao de memoria\n"); return -1; }

    for (int j = 0; j < m; j++) {
        if (parse_nonneg(argv[j + 1], &avail[j]) != 0) {
            fprintf(stderr, "Erro: argumento %d invalido ('%s'): esperado inteiro nao negativo\n",
                    j + 1, argv[j + 1]);
            goto fail;
        }
    }

    /* Passo 2: abrir e ler customer.txt */
    f = fopen(customer_filename, "r");
    if (!f) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s': %s\n",
                customer_filename, strerror(errno));
        goto fail;
    }

    rows = malloc((size_t)cap * sizeof(int *));
    if (!rows) { fprintf(stderr, "Erro: falha de alocacao de memoria\n"); goto fail; }

    int lineno = 0, blank_seen = 0;
    int rl;
    while ((rl = read_line(&line, &len, f)) == 1) {
        lineno++;
        char *t = trim(line);

        if (*t == '\0') { blank_seen = 1; continue; }  /* tolera vazias so no fim */
        if (blank_seen) {
            fprintf(stderr, "Erro: %s:%d: linha vazia no meio do arquivo\n",
                    customer_filename, lineno - 1);
            goto fail;
        }

        /* strtok ignora campos vazios; contar virgulas detecta "7,,3" e "7,5," */
        int commas = 0;
        for (const char *p = t; *p; p++) if (*p == ',') commas++;
        if (commas != m - 1) {
            fprintf(stderr, "Erro: %s:%d: formato invalido (esperado %d valores separados por virgula)\n",
                    customer_filename, lineno, m);
            goto fail;
        }

        if (n == cap) {                                /* Passo 3: cresce conforme as linhas */
            cap *= 2;
            int **tmp = realloc(rows, (size_t)cap * sizeof(int *));
            if (!tmp) { fprintf(stderr, "Erro: falha de alocacao de memoria\n"); goto fail; }
            rows = tmp;
        }
        rows[n] = malloc((size_t)m * sizeof(int));
        if (!rows[n]) { fprintf(stderr, "Erro: falha de alocacao de memoria\n"); goto fail; }
        n++;                                           /* linha ja pertence a rows (liberada no fail) */

        int j = 0;
        for (char *tok = strtok(t, ","); tok; tok = strtok(NULL, ",")) {
            tok = trim(tok);
            if (j >= m) break;                         /* protecao extra de indice */
            if (parse_nonneg(tok, &rows[n - 1][j]) != 0) {
                fprintf(stderr, "Erro: %s:%d: valor invalido '%s'\n",
                        customer_filename, lineno, tok);
                goto fail;
            }
            j++;
        }
        /* "7,,3" tem o numero certo de virgulas, mas um campo vazio */
        if (j != m) {
            fprintf(stderr, "Erro: %s:%d: campo vazio ou formato invalido (esperado %d valores)\n",
                    customer_filename, lineno, m);
            goto fail;
        }
    }

    if (rl < 0) { fprintf(stderr, "Erro: falha de alocacao de memoria\n"); goto fail; }

    if (n == 0) {
        fprintf(stderr, "Erro: '%s' nao contem nenhum cliente\n", customer_filename);
        goto fail;
    }

    /* Passo 4: tudo validado -> cria o estado (allocation = 0, need = maximum) */
    init_matrices(n, m, avail);
    for (int i = 0; i < n; i++)
        set_customer_maximum(i, rows[i]);

    free(line);
    fclose(f);
    free_rows(rows, n);
    free(avail);
    return 0;

fail:                                                  /* Passo 5: limpeza em erro */
    free(line);
    if (f) fclose(f);
    if (rows) free_rows(rows, n);
    free(avail);
    return -1;
}
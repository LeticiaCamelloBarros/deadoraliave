#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "matriz.h"

/* ---------- Estado interno (privado ao modulo) ---------- */

static int n = 0;                 /* NUMBER_OF_CUSTOMERS */
static int m = 0;                 /* NUMBER_OF_RESOURCES */
static int *available  = NULL;    /* [m]   */
static int *maximum    = NULL;    /* [n*m] linha a linha */
static int *allocation = NULL;    /* [n*m] */
static int *need       = NULL;    /* [n*m] */

/* Acesso a matriz achatada: linha i, coluna j */
#define AT(mat, i, j) ((mat)[(i) * m + (j)])

static int valid_customer(int c) { return c >= 0 && c < n; }
static int valid_resource(int r) { return r >= 0 && r < m; }

static void fatal_oom(void) {
    fprintf(stderr, "Erro: falha de alocacao de memoria\n");
    exit(EXIT_FAILURE);
}

/* ---------- Inicializacao ---------- */

void init_matrices(int num_customers, int num_resources, int available_init[]) {
    free_matrices();                      /* garante estado limpo */

    if (num_customers <= 0 || num_resources <= 0) {
        fprintf(stderr, "Erro: dimensoes invalidas (%d clientes, %d recursos)\n",
                num_customers, num_resources);
        exit(EXIT_FAILURE);
    }

    n = num_customers;
    m = num_resources;

    available  = malloc((size_t)m * sizeof(int));
    maximum    = calloc((size_t)n * m, sizeof(int));
    allocation = calloc((size_t)n * m, sizeof(int));   /* comeca zerada */
    need       = calloc((size_t)n * m, sizeof(int));
    if (!available || !maximum || !allocation || !need) fatal_oom();

    memcpy(available, available_init, (size_t)m * sizeof(int));
}

void set_customer_maximum(int customer_id, int max_resources[]) {
    if (!valid_customer(customer_id)) return;

    for (int j = 0; j < m; j++) {
        AT(maximum, customer_id, j)    = max_resources[j];
        AT(allocation, customer_id, j) = 0;
        AT(need, customer_id, j)       = max_resources[j];   /* need = max - 0 */
    }
}

/* ---------- Atualizacao de estado ---------- */

void apply_request(int customer_id, int request[]) {
    if (!valid_customer(customer_id)) return;

    for (int j = 0; j < m; j++) {
        available[j]                  -= request[j];
        AT(allocation, customer_id, j) += request[j];
        AT(need, customer_id, j)       -= request[j];
    }
}

void rollback_request(int customer_id, int request[]) {
    if (!valid_customer(customer_id)) return;

    for (int j = 0; j < m; j++) {
        available[j]                  += request[j];
        AT(allocation, customer_id, j) -= request[j];
        AT(need, customer_id, j)       += request[j];
    }
}

void apply_release(int customer_id, int release[]) {
    if (!valid_customer(customer_id)) return;

    for (int j = 0; j < m; j++) {
        available[j]                  += release[j];
        AT(allocation, customer_id, j) -= release[j];
        /* recalcula need a partir de maximum e allocation */
        AT(need, customer_id, j) = AT(maximum, customer_id, j)
                                 - AT(allocation, customer_id, j);
    }
}

/* ---------- Consulta ---------- */

int get_num_customers(void) { return n; }
int get_num_resources(void) { return m; }

int get_available(int resource) {
    return valid_resource(resource) ? available[resource] : 0;
}

int get_maximum(int customer_id, int resource) {
    return (valid_customer(customer_id) && valid_resource(resource))
           ? AT(maximum, customer_id, resource) : 0;
}

int get_allocation(int customer_id, int resource) {
    return (valid_customer(customer_id) && valid_resource(resource))
           ? AT(allocation, customer_id, resource) : 0;
}

int get_need(int customer_id, int resource) {
    return (valid_customer(customer_id) && valid_resource(resource))
           ? AT(need, customer_id, resource) : 0;
}

const int *get_available_vector(void) { return available; }

/* ---------- Impressao ---------- */

/* Numero de caracteres que a linha i de mat ocupa ("a b c"). */
static int row_len(const int *mat, int i) {
    int len = 0;
    for (int j = 0; j < m; j++)
        len += snprintf(NULL, 0, j ? " %d" : "%d", AT(mat, i, j));
    return len;
}

/* Maior largura entre todas as linhas de mat. */
static int max_row_len(const int *mat) {
    int w = 0;
    for (int i = 0; i < n; i++) {
        int len = row_len(mat, i);
        if (len > w) w = len;
    }
    return w;
}

/* Escreve a linha i de mat em out e preenche com espacos ate width. */
static void print_row(FILE *out, const int *mat, int i, int width) {
    int len = 0;
    for (int j = 0; j < m; j++)
        len += fprintf(out, j ? " %d" : "%d", AT(mat, i, j));
    for (; len < width; len++) fputc(' ', out);
}

void print_state(FILE *out) {
    if (!out || n == 0) return;

    /* Larguras das duas primeiras colunas: o cabecalho e o maior valor
       determinam o tamanho (para m = 3 resulta em 10 e 12). */
    int w1 = max_row_len(maximum);
    int w2 = max_row_len(allocation);
    if (w1 < (int)strlen("MAXIMUM") + 3)    w1 = (int)strlen("MAXIMUM") + 3;
    else                                    w1 += 1;
    if (w2 < (int)strlen("ALLOCATION") + 2) w2 = (int)strlen("ALLOCATION") + 2;
    else                                    w2 += 1;

    fprintf(out, "%-*s|   %-*s|   NEED\n", w1, "MAXIMUM", w2, "ALLOCATION");

    for (int i = 0; i < n; i++) {
        print_row(out, maximum, i, w1);
        fputs("|   ", out);
        print_row(out, allocation, i, w2);
        fputs("|   ", out);
        print_row(out, need, i, 0);
        fputc('\n', out);
    }

    fputs("AVAILABLE", out);
    for (int j = 0; j < m; j++) fprintf(out, " %d", available[j]);
    fputc('\n', out);
}

/* ---------- Limpeza ---------- */

void free_matrices(void) {
    free(available);
    free(maximum);
    free(allocation);
    free(need);
    available = maximum = allocation = need = NULL;
    n = m = 0;
}
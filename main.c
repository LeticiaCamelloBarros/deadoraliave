#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <limits.h>

#define CUSTOMER_FILE "customer.txt"
#define COMMANDS_FILE "commands.txt"

typedef enum { CMD_RQ, CMD_RL, CMD_STAR } CmdType;

typedef struct {
    CmdType type;
    int customer;   /* -1 para "*" */
    int *resources; /* NULL para "*" */
} Command;

/* Estado global (lido da entrada) */
static int  num_resources = 0;
static int  num_customers = 0;
static int *available_total = NULL;   /* instâncias totais (argumentos) */
static int **maximum = NULL;          /* [customer][resource] */
static Command *commands = NULL;
static int  num_commands = 0;

/* ---------- utilitários ---------- */

static void fail(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));
static void fail(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "Erro: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(EXIT_FAILURE);
}

/* Remove espaços/\r/\n nas pontas (in place) */
static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) *--end = '\0';
    return s;
}

/* Converte string inteira em inteiro >= 0. Retorna 0 em sucesso, -1 em erro. */
static int parse_nonneg_int(const char *s, int *out) {
    if (s == NULL || *s == '\0') return -1;
    for (const char *p = s; *p; p++)
        if (!isdigit((unsigned char)*p)) return -1;
    errno = 0;
    long v = strtol(s, NULL, 10);
    if (errno != 0 || v > INT_MAX) return -1;
    *out = (int)v;
    return 0;
}

/* ---------- argumentos ---------- */

static void parse_args(int argc, char *argv[]) {
    if (argc < 2)
        fail("uso: %s <inst_recurso_0> <inst_recurso_1> ... (ao menos 1 recurso)", argv[0]);

    num_resources = argc - 1;
    available_total = malloc(num_resources * sizeof(int));
    if (!available_total) fail("falha de alocacao de memoria");

    for (int i = 0; i < num_resources; i++) {
        if (parse_nonneg_int(argv[i + 1], &available_total[i]) != 0)
            fail("argumento %d invalido ('%s'): esperado inteiro nao negativo", i + 1, argv[i + 1]);
    }
}

/* ---------- customer.txt ---------- */

static void load_customers(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) fail("nao foi possivel abrir '%s': %s", path, strerror(errno));

    int cap = 8;
    maximum = malloc(cap * sizeof(int *));
    if (!maximum) fail("falha de alocacao de memoria");

    char *line = NULL;
    size_t len = 0;
    int lineno = 0;
    int blank_seen = 0;

    while (getline(&line, &len, f) != -1) {
        lineno++;
        char *t = trim(line);

        if (*t == '\0') { blank_seen = 1; continue; }   /* tolera linhas vazias apenas no fim */
        if (blank_seen)
            fail("%s:%d: linha vazia no meio do arquivo", path, lineno - 1);

        if (num_customers == cap) {
            cap *= 2;
            maximum = realloc(maximum, cap * sizeof(int *));
            if (!maximum) fail("falha de alocacao de memoria");
        }

        int *row = malloc(num_resources * sizeof(int));
        if (!row) fail("falha de alocacao de memoria");

        /* strtok ignora campos vazios ("7,,3" ou "7,5,"), entao conta virgulas antes */
        int commas = 0;
        for (const char *p = t; *p; p++) if (*p == ',') commas++;
        if (commas != num_resources - 1)
            fail("%s:%d: formato invalido (esperado %d valores separados por virgula)",
                 path, lineno, num_resources);

        int count = 0;
        char *save = NULL;
        for (char *tok = strtok_r(t, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
            tok = trim(tok);
            if (count >= num_resources)
                fail("%s:%d: mais valores que o numero de recursos (%d)", path, lineno, num_resources);
            if (parse_nonneg_int(tok, &row[count]) != 0)
                fail("%s:%d: valor invalido '%s' (formato esperado: n,n,n)", path, lineno, tok);
            count++;
        }
        if (count != num_resources)
            fail("%s:%d: %d valores encontrados, mas existem %d recursos nos argumentos",
                 path, lineno, count, num_resources);

        maximum[num_customers++] = row;
    }
    free(line);
    fclose(f);

    if (num_customers == 0) fail("'%s' nao contem nenhum cliente", path);
}

/* ---------- commands.txt ---------- */

static void load_commands(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) fail("nao foi possivel abrir '%s': %s", path, strerror(errno));

    int cap = 16;
    commands = malloc(cap * sizeof(Command));
    if (!commands) fail("falha de alocacao de memoria");

    char *line = NULL;
    size_t len = 0;
    int lineno = 0;

    while (getline(&line, &len, f) != -1) {
        lineno++;
        char *t = trim(line);
        if (*t == '\0') continue;   /* ignora linhas vazias */

        char *save = NULL;
        char *op = strtok_r(t, " \t", &save);
        Command cmd = { CMD_STAR, -1, NULL };

        if (strcmp(op, "*") == 0) {
            if (strtok_r(NULL, " \t", &save) != NULL)
                fail("%s:%d: o comando '*' nao recebe parametros", path, lineno);
        } else if (strcmp(op, "RQ") == 0 || strcmp(op, "RL") == 0) {
            cmd.type = (op[1] == 'Q') ? CMD_RQ : CMD_RL;

            char *tok = strtok_r(NULL, " \t", &save);
            if (!tok || parse_nonneg_int(tok, &cmd.customer) != 0)
                fail("%s:%d: numero de cliente invalido ou ausente", path, lineno);
            if (cmd.customer >= num_customers)
                fail("%s:%d: cliente %d inexistente (ha %d clientes em %s)",
                     path, lineno, cmd.customer, num_customers, CUSTOMER_FILE);

            cmd.resources = malloc(num_resources * sizeof(int));
            if (!cmd.resources) fail("falha de alocacao de memoria");

            int count = 0;
            while ((tok = strtok_r(NULL, " \t", &save)) != NULL) {
                if (count >= num_resources)
                    fail("%s:%d: mais valores que o numero de recursos (%d)", path, lineno, num_resources);
                if (parse_nonneg_int(tok, &cmd.resources[count]) != 0)
                    fail("%s:%d: valor invalido '%s'", path, lineno, tok);
                count++;
            }
            if (count != num_resources)
                fail("%s:%d: %d valores encontrados, mas existem %d recursos nos argumentos",
                     path, lineno, count, num_resources);
        } else {
            fail("%s:%d: comando desconhecido '%s' (esperado RQ, RL ou *)", path, lineno, op);
        }

        if (num_commands == cap) {
            cap *= 2;
            commands = realloc(commands, cap * sizeof(Command));
            if (!commands) fail("falha de alocacao de memoria");
        }
        commands[num_commands++] = cmd;
    }
    free(line);
    fclose(f);
}

/* ---------- liberação ---------- */

static void cleanup(void) {
    for (int i = 0; i < num_customers; i++) free(maximum[i]);
    free(maximum);
    for (int i = 0; i < num_commands; i++) free(commands[i].resources);
    free(commands);
    free(available_total);
}

/* ---------- main ---------- */

int main(int argc, char *argv[]) {
    parse_args(argc, argv);
    load_customers(CUSTOMER_FILE);
    load_commands(COMMANDS_FILE);

    /* TODO: inicializar available/allocation/need, executar comandos
       (request_resources / release_resources / print_state) e gravar result.txt */

    /* Saída de depuração temporária */
    printf("Recursos (%d):", num_resources);
    for (int j = 0; j < num_resources; j++) printf(" %d", available_total[j]);
    printf("\nClientes: %d\n", num_customers);
    for (int i = 0; i < num_customers; i++) {
        printf("  C%d max:", i);
        for (int j = 0; j < num_resources; j++) printf(" %d", maximum[i][j]);
        printf("\n");
    }
    printf("Comandos: %d\n", num_commands);
    for (int k = 0; k < num_commands; k++) {
        if (commands[k].type == CMD_STAR) { printf("  *\n"); continue; }
        printf("  %s %d", commands[k].type == CMD_RQ ? "RQ" : "RL", commands[k].customer);
        for (int j = 0; j < num_resources; j++) printf(" %d", commands[k].resources[j]);
        printf("\n");
    }

    cleanup();
    return EXIT_SUCCESS;
}
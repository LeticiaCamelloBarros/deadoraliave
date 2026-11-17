#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#include "matriz.h"
#include "banker.h"

#define CUSTOMER_FILE "customer.txt"
#define COMMANDS_FILE "commands.txt"
#define RESULT_FILE   "result.txt"

typedef enum { CMD_RQ, CMD_RL, CMD_STAR } CmdType;

typedef struct {
    CmdType type;
    int customer;    /* -1 para "*" */
    int *resources;  /* NULL para "*" */
} Command;

static Command *commands = NULL;
static int num_commands = 0;

/* ---------- utilitarios ---------- */

/* Libera tudo; registrada com atexit para valer tambem nos caminhos de erro */
static void cleanup(void) {
    for (int i = 0; i < num_commands; i++) free(commands[i].resources);
    free(commands);
    commands = NULL;
    num_commands = 0;
    free_matrices();
}

/* Mensagem de erro em stderr e encerramento controlado */
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

/* Inteiro >= 0, so digitos. Retorna 0 em sucesso, -1 se invalido. */
static int parse_nonneg_int(const char *s, int *out) {
    if (!s || !*s) return -1;
    for (const char *p = s; *p; p++)
        if (!isdigit((unsigned char)*p)) return -1;
    errno = 0;
    long v = strtol(s, NULL, 10);
    if (errno != 0 || v > INT_MAX) return -1;
    *out = (int)v;
    return 0;
}

/* ---------- commands.txt ---------- */

/* Le e valida todos os comandos. Usa as dimensoes ja definidas no modulo
   matriz (clientes do customer.txt, recursos dos argumentos). */
static void load_commands(const char *path) {
    const int n = get_num_customers();
    const int m = get_num_resources();

    FILE *f = fopen(path, "r");
    if (!f) fail("nao foi possivel abrir '%s': %s", path, strerror(errno));

    int cap = 16;
    commands = malloc((size_t)cap * sizeof(Command));
    if (!commands) fail("falha de alocacao de memoria");

    char *line = NULL;
    size_t len = 0;
    int lineno = 0;

    int rl;
    while ((rl = read_line(&line, &len, f)) == 1) {
        lineno++;
        char *t = trim(line);
        if (*t == '\0') continue;                      /* ignora linhas vazias */

        char *op = strtok(t, " \t");
        Command cmd = { CMD_STAR, -1, NULL };

        if (strcmp(op, "*") == 0) {
            if (strtok(NULL, " \t") != NULL) {
                free(line); fclose(f);
                fail("%s:%d: o comando '*' nao recebe parametros", path, lineno);
            }
        } else if (strcmp(op, "RQ") == 0 || strcmp(op, "RL") == 0) {
            cmd.type = (op[1] == 'Q') ? CMD_RQ : CMD_RL;

            char *tok = strtok(NULL, " \t");
            if (!tok || parse_nonneg_int(tok, &cmd.customer) != 0) {
                free(line); fclose(f);
                fail("%s:%d: numero de cliente invalido ou ausente", path, lineno);
            }
            if (cmd.customer >= n) {
                int c = cmd.customer;
                free(line); fclose(f);
                fail("%s:%d: cliente %d inexistente (ha %d clientes em %s)",
                     path, lineno, c, n, CUSTOMER_FILE);
            }

            cmd.resources = malloc((size_t)m * sizeof(int));
            if (!cmd.resources) { free(line); fclose(f); fail("falha de alocacao de memoria"); }

            int count = 0;
            while ((tok = strtok(NULL, " \t")) != NULL) {
                if (count >= m || parse_nonneg_int(tok, &cmd.resources[count]) != 0) {
                    free(cmd.resources); free(line); fclose(f);
                    fail("%s:%d: valor invalido ou em excesso '%s' (esperados %d valores)",
                         path, lineno, tok, m);
                }
                count++;
            }
            if (count != m) {
                free(cmd.resources); free(line); fclose(f);
                fail("%s:%d: %d valores encontrados, mas existem %d recursos nos argumentos",
                     path, lineno, count, m);
            }
        } else {
            char bad[32];
            snprintf(bad, sizeof bad, "%s", op);
            free(line); fclose(f);
            fail("%s:%d: comando desconhecido '%s' (esperado RQ, RL ou *)", path, lineno, bad);
        }

        if (num_commands == cap) {
            cap *= 2;
            Command *tmp = realloc(commands, (size_t)cap * sizeof(Command));
            if (!tmp) { free(cmd.resources); free(line); fclose(f); fail("falha de alocacao de memoria"); }
            commands = tmp;
        }
        commands[num_commands++] = cmd;
    }
    if (rl < 0) { free(line); fclose(f); fail("falha de alocacao de memoria"); }
    free(line);
    fclose(f);
}

/* ---------- execucao ---------- */

/* Percorre os comandos na ordem e delega ao modulo banker / matriz */
static void run_commands(FILE *out) {
    for (int k = 0; k < num_commands; k++) {
        const Command *c = &commands[k];
        switch (c->type) {
            case CMD_RQ:   request_resources(c->customer, c->resources, out); break;
            case CMD_RL:   release_resources(c->customer, c->resources, out); break;
            case CMD_STAR: print_state(out);                                  break;
        }
    }
}

/* ---------- main ---------- */

int main(int argc, char *argv[]) {
    atexit(cleanup);

    /* 1. Argumentos + customer.txt -> estado inicial (modulo matriz) */
    if (init_system(argc, argv, CUSTOMER_FILE) != 0)
        return EXIT_FAILURE;

    /* 2. commands.txt, validado contra as dimensoes ja conhecidas */
    load_commands(COMMANDS_FILE);

    /* 3. Executa e grava result.txt (so abre apos validar toda a entrada) */
    FILE *out = fopen(RESULT_FILE, "w");
    if (!out) fail("nao foi possivel criar '%s': %s", RESULT_FILE, strerror(errno));

    run_commands(out);

    if (fclose(out) != 0) fail("falha ao gravar '%s': %s", RESULT_FILE, strerror(errno));
    return EXIT_SUCCESS;
}
#!/usr/bin/env bash
# Gera o arquivo evidencias.log executando o ./banker em varios cenarios
# (caso do enunciado e casos de erro). Uso: make && ./gerar_evidencias.sh
set -u

LOG="$(pwd)/evidencias.log"
SRC="$(pwd)/banker"
[ -x "$SRC" ] || SRC="$(pwd)/banker.exe"

if [ ! -x "$SRC" ]; then
    echo "Execute 'make' antes de gerar as evidencias." >&2
    exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cp "$SRC" "$WORK/"          # roda como ./banker, igual ao uso real
cd "$WORK" || exit 1
BIN=./banker

# --- entradas do enunciado ---
printf '7,5,3\n3,2,2\n9,0,2\n2,2,2\n4,3,3\n' > customer.txt
cat > commands.txt <<'CMD'
RQ 0 0 2 1
RQ 2 3 0 2
RQ 1 2 0 0
RQ 4 0 0 2
RL 0 0 1 1
RQ 3 2 1 1
*
RQ 4 4 0 0
RQ 1 1 0 2
RQ 0 0 2 0
RQ 2 0 1 0
CMD
cp customer.txt customer.ok
cp commands.txt commands.ok

restore() { cp customer.ok customer.txt; cp commands.ok commands.txt; rm -f result.txt; }

# run <descricao> <argumentos...>: executa e registra saida, stderr e codigo
run() {
    local desc="$1"; shift
    {
        echo "----------------------------------------------------------------"
        echo "CENARIO: $desc"
        echo "COMANDO: ./banker $*"
    } >> "$LOG"
    rm -f result.txt
    "$BIN" "$@" > out.txt 2> err.txt
    local code=$?
    {
        echo "CODIGO DE SAIDA: $code"
        if [ -s err.txt ]; then echo "STDERR:"; sed 's/^/  /' err.txt; fi
        if [ -s out.txt ]; then echo "STDOUT:"; sed 's/^/  /' out.txt; fi
        if [ -f result.txt ]; then echo "result.txt:"; sed 's/^/  /' result.txt; fi
        echo
    } >> "$LOG"
}

{
    echo "EVIDENCIAS - Algoritmo de Banker"
    echo "Data: $(date '+%Y-%m-%d %H:%M:%S')"
    echo "Sistema: $(uname -srm)"
    echo "Compilador: $(gcc --version 2>/dev/null | head -n 1)"
    echo
} > "$LOG"

restore; run "Exemplo do enunciado (10 5 7)" 10 5 7

restore; run "Sem argumentos"
restore; run "Argumentos a menos que os recursos do customer.txt" 10 5
restore; run "Argumentos a mais que os recursos do customer.txt" 10 5 7 8
restore; run "Argumento nao numerico" 10 x 7

restore; rm customer.txt;            run "customer.txt inexistente" 10 5 7
restore; printf '7,,3\n' > customer.txt; run "customer.txt com campo vazio" 10 5 7
restore; printf '7,5,a\n' > customer.txt; run "customer.txt com valor nao numerico" 10 5 7
restore; printf '7 5 3\n' > customer.txt; run "customer.txt com separador invalido" 10 5 7
restore; : > customer.txt;           run "customer.txt vazio" 10 5 7

restore; rm commands.txt;            run "commands.txt inexistente" 10 5 7
restore; printf 'XX 0 1 1 1\n' > commands.txt; run "Comando desconhecido" 10 5 7
restore; printf 'RQ 9 1 1 1\n' > commands.txt; run "Cliente inexistente" 10 5 7
restore; printf 'RQ 0 1 1\n' > commands.txt;   run "Comando com valores a menos" 10 5 7
restore; printf 'RQ 0 1 1 1 1\n' > commands.txt; run "Comando com valores a mais" 10 5 7
restore; printf '* x\n' > commands.txt;        run "Comando * com parametros" 10 5 7

restore; printf 'RQ 1 1 0 0\nRL 1 5 0 0\nRL 1 1 0 0\n*\n' > commands.txt
run "RL maior que a alocacao (recusado) e RL valido" 10 5 7

restore
echo "Log gravado em $LOG"
# Algoritmo de Banker
 
Implementação em C do Algoritmo de Banker (Silberschatz et al., *Operating System Concepts*, 10ª ed., p. P-45) para a disciplina de Infraestrutura de Software. O número de clientes e de tipos de recursos é definido pelos arquivos de entrada, e os comandos são lidos de um arquivo em vez de digitados.
 
## Estrutura do projeto
 
| Arquivo | Responsabilidade |
|---|---|
| `main.c` | Controle de fluxo e I/O: lê o `commands.txt`, executa os comandos e grava o `result.txt` |
| `init.c` / `init.h` | `init_system`: lê os argumentos e o `customer.txt` e inicializa o estado |
| `matriz.c` / `matriz.h` | Camada de dados: guarda `MAXIMUM`, `ALLOCATION`, `NEED` e `AVAILABLE`, e imprime o estado |
| `banker.c` / `banker.h` | Algoritmo de segurança (`is_safe_state`) e comandos `RQ` e `RL` |
| `Makefile` | Compilação e limpeza |
| `gerar_evidencias.sh` | Executa vários cenários e gera o `evidencias.log` |
 
## Requisitos
 
- Linux, Unix ou macOS
- `gcc` e `make`
## Compilação
 
```bash
make          # gera o executável ./banker
make clean    # remove o executável, os .o e o result.txt
```
 
## Uso
 
```bash
./banker <instâncias do recurso 0> <instâncias do recurso 1> ...
```
 
Cada argumento é o número de instâncias de um tipo de recurso. Exemplo, com 3 recursos de 10, 5 e 7 instâncias:
 
```bash
./banker 10 5 7
```
 
O programa lê `customer.txt` e `commands.txt` do diretório atual e grava o resultado em `result.txt`.
 
### `customer.txt`
 
Uma linha por cliente, com o máximo de cada recurso separado por vírgula. O número de linhas define o número de clientes, e o número de valores por linha deve ser igual ao número de argumentos.
 
```
7,5,3
3,2,2
9,0,2
2,2,2
4,3,3
```
 
### `commands.txt`
 
Um comando por linha:
 
| Comando | Significado |
|---|---|
| `RQ <cliente> <r0> <r1> ...` | O cliente solicita recursos |
| `RL <cliente> <r0> <r1> ...` | O cliente libera recursos |
| `*` | Imprime o estado atual (`MAXIMUM`, `ALLOCATION`, `NEED` e `AVAILABLE`) |
 
```
RQ 0 0 2 1
RL 0 0 1 1
*
```
 
### `result.txt`
 
Uma linha por comando `RQ` e `RL`, e a tabela de estado para cada `*`:
 
```
Allocate to customer 0 the resources 0 2 1
Release from customer 0 the resources 0 1 1
The resources 3 3 2 are not enough to customer 4 request 4 0 0
The customer 0 request 0 2 0 was denied because result in an unsafe state
The customer 2 request 0 1 0 was denied because exceed its maximum allocation
```
 
## Como funciona
 
**Requisição (`RQ`)**
 
1. Se algum valor excede o `NEED` do cliente, o pedido é negado (*exceed its maximum allocation*).
2. Se algum valor excede o `AVAILABLE`, o pedido é negado (*not enough*).
3. Caso contrário, a alocação é simulada e o algoritmo de segurança é executado. Se o estado for seguro, a alocação é mantida. Se for inseguro, ela é desfeita e o pedido é negado (*result in an unsafe state*).
**Liberação (`RL`)**: o cliente só pode devolver o que tem alocado. Se for válido, `AVAILABLE` aumenta, `ALLOCATION` diminui e `NEED` é recalculado. Se o cliente tentar devolver mais do que possui, a liberação é recusada com a mensagem *denied because exceed its allocation* (texto escolhido pela equipe, pois o enunciado não define essa mensagem).
 
**Algoritmo de segurança**: parte de `work = available` e procura repetidamente um cliente não finalizado com `need <= work`, somando a alocação dele em `work`. O estado é seguro se todos os clientes puderem terminar.
 
## Tratamento de erros
 
O programa não trava: valida a entrada, imprime uma mensagem em `stderr` indicando o arquivo e a linha, e encerra com código 1 quando:
 
- não há argumentos, ou o número deles não bate com as colunas do `customer.txt` ou os valores do `commands.txt`;
- um argumento não é um inteiro não negativo;
- `customer.txt` ou `commands.txt` não existe, não pode ser aberto ou está vazio (no caso do `customer.txt`);
- uma linha do `customer.txt` tem formato inválido (campo vazio, valor não numérico, separador diferente de vírgula);
- uma linha do `commands.txt` tem comando desconhecido, cliente inexistente ou quantidade errada de valores.
Toda a entrada é validada antes de o `result.txt` ser criado, então um erro de formato não deixa um arquivo de saída parcial.
 
## Testes e evidências
 
```bash
make
./gerar_evidencias.sh    # gera o evidencias.log
```
 
O `evidencias.log` registra o exemplo do enunciado e os casos de erro (comando executado, código de saída, `stderr` e `result.txt`).
 
## Autoria
 
Nome: Leticia soares do nascimento de camello barros 
E-mail: lsncb@cesar.schoo;
 
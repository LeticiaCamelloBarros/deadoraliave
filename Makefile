# Makefile - Algoritmo de Banker
# Uso: make (gera ./banker) | make clean

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -MMD -MP
TARGET  = banker

SRCS    = main.c matriz.c banker.c
OBJS    = $(SRCS:.c=.o)
DEPS    = $(OBJS:.o=.d)

# Regra padrao: compila os objetos e gera o executavel
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Cada .o depende do seu .c; as dependencias de .h sao geradas
# automaticamente pelo gcc (-MMD) nos arquivos .d incluidos abaixo.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Inclui as dependencias de cabecalho (ignora se ainda nao existem)
-include $(DEPS)

# Remove objetos, dependencias e executavel
clean:
	rm -f *.o *.d banker banker.exe

.PHONY: all clean
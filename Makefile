CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
SRC = src/registro.c src/avl.c src/rubro_negra.c src/dataset.c
PYTHON ?= python3

.PHONY: all testes desempenho completo graficos dataset app limpar

all: testes desempenho

teste_corretude: testes/teste_corretude.c $(SRC) src/*.h
	$(CC) $(CFLAGS) -o $@ testes/teste_corretude.c $(SRC) -lm

teste_desempenho: testes/teste_desempenho.c $(SRC) src/*.h
	$(CC) $(CFLAGS) -o $@ testes/teste_desempenho.c $(SRC) -lm

# Testes de corretude (invariantes, casos extremos, dados reais)
testes: teste_corretude
	./teste_corretude

# Bateria rapida (1.000 e 10.000 registros)
desempenho: teste_desempenho
	@mkdir -p resultados
	./teste_desempenho 1000 10000
	@echo ""
	@echo "Isso foi so um teste rapido. Bateria completa (1k a 500k) + graficos:  make completo"

# Bateria completa (1k a 500k) e graficos
completo: teste_desempenho
	@mkdir -p resultados
	./teste_desempenho
	$(PYTHON) graficos/gerar_graficos.py

# So os graficos, a partir do resultados/resultados_desempenho.csv que ja existe
graficos:
	$(PYTHON) graficos/gerar_graficos.py

# Regera dataset/pokemon_dataset.csv (usa dataset/base_especies.csv, nao precisa de internet)
dataset:
	$(PYTHON) dataset/gerar_dataset.py

# Aplicativo grafico (servidor local + interface web): gera ./poketree
app:
	cd app && ./build.sh

limpar:
	rm -f teste_corretude teste_desempenho poketree *.exe

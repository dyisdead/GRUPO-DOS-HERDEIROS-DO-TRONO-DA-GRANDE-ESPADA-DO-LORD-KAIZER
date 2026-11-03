#!/usr/bin/env python3
"""
Gerador do dataset PokeTree.

Dados reais: especies, variantes (formas), stats base e golpes vem da PokeAPI
(usamos os CSVs oficiais do repositorio github.com/PokeAPI/pokeapi, que sao
exatamente os dados que a API serve). Cada registro do dataset e um Pokemon
individual sorteado em cima dessas especies reais: id unico, genero (respeitando
a proporcao real da especie), shiny, IV de 1 a 100 e 4 golpes do pool da especie.

Etapas:
  1) base_especies.csv  -> tabela compacta (uma linha por Pokemon/variante).
                           Criada baixando os CSVs da PokeAPI; so e refeita se
                           o arquivo nao existir (ou com --refazer-base).
                           Como ela ja vem no repositorio, NAO precisa de internet.
  2) pokemon_dataset.csv -> o dataset final (padrao: 500.000 registros).

Uso (rode de qualquer pasta):
    python dataset/gerar_dataset.py                 # 500.000 registros
    python dataset/gerar_dataset.py -n 300000       # outro volume
    python dataset/gerar_dataset.py --refazer-base  # rebaixa os dados da PokeAPI

Formula do total_stats:
    stat_final_i = base_i + ivs * 0.5      (para cada um dos 6 stats)
    total_stats  = soma dos 6              -> base_total + 3 * ivs
"""

import argparse
import csv
import io
import os
import random
import sys
import urllib.request

PASTA = os.path.dirname(os.path.abspath(__file__))
BASE_ESPECIES = os.path.join(PASTA, "base_especies.csv")
SAIDA_PADRAO = os.path.join(PASTA, "pokemon_dataset.csv")

URL_BASE = "https://raw.githubusercontent.com/PokeAPI/pokeapi/master/data/v2/csv/"
IDIOMA_EN = "9"          # local_language_id do ingles na PokeAPI
FATOR_ESCALA = 0.5       # cada ponto de IV vale 0,5 em cada stat
LIMITE_NOME = 63         # bytes (Registro.nome tem 64, contando o '\0')
LIMITE_ATAQUE = 31       # bytes (Registro.ataques[i] tem 32)


def baixar_csv(nome):
    url = URL_BASE + nome + ".csv"
    print(f"  baixando {url}")
    with urllib.request.urlopen(url, timeout=120) as r:
        texto = r.read().decode("utf-8")
    return list(csv.DictReader(io.StringIO(texto)))


def titulo(identificador):
    return " ".join(p.capitalize() for p in identificador.split("-"))


def checar_campo(valor, limite, rotulo):
    if any(c in valor for c in ',"\n\r|'):
        sys.exit(f"Erro: {rotulo} '{valor}' tem caractere proibido (virgula, aspas ou barra).")
    if len(valor.encode("utf-8")) > limite:
        sys.exit(f"Erro: {rotulo} '{valor}' passa de {limite} bytes (limite do struct em C).")


def construir_base():
    """Baixa os CSVs da PokeAPI e grava base_especies.csv (tabela compacta)."""
    print("Montando base_especies.csv a partir da PokeAPI...")
    pokemon = baixar_csv("pokemon")
    especies = baixar_csv("pokemon_species")
    nomes_esp = baixar_csv("pokemon_species_names")
    stats = baixar_csv("pokemon_stats")
    golpes_pk = baixar_csv("pokemon_moves")
    golpes = baixar_csv("moves")
    nomes_golpe = baixar_csv("move_names")

    esp_ident = {e["id"]: e["identifier"] for e in especies}
    esp_genero = {e["id"]: int(e["gender_rate"]) for e in especies}
    esp_nome = {n["pokemon_species_id"]: n["name"] for n in nomes_esp
                if n["local_language_id"] == IDIOMA_EN}

    nome_golpe = {g["id"]: titulo(g["identifier"]) for g in golpes}
    for n in nomes_golpe:
        if n["local_language_id"] == IDIOMA_EN:
            nome_golpe[n["move_id"]] = n["name"]

    stat_pk = {}
    for s in stats:  # stat_id 1..6 = hp, atk, def, spa, spd, spe
        stat_pk.setdefault(s["pokemon_id"], {})[int(s["stat_id"])] = int(s["base_stat"])

    pool_pk = {}
    for m in golpes_pk:
        pool_pk.setdefault(m["pokemon_id"], set()).add(m["move_id"])

    padrao_da_especie = {p["species_id"]: p["id"] for p in pokemon if p["is_default"] == "1"}

    linhas = []
    for p in pokemon:
        pid, sid = p["id"], p["species_id"]
        st = stat_pk.get(pid)
        if not st or len(st) != 6:
            continue
        # variantes sem golpes proprios herdam os da forma padrao da especie
        pool = pool_pk.get(pid) or pool_pk.get(padrao_da_especie.get(sid), set())
        if not pool:
            continue
        base_nome = esp_nome.get(sid) or titulo(esp_ident[sid])
        if p["is_default"] == "1":
            nome = base_nome
        else:
            resto = p["identifier"][len(esp_ident[sid]) + 1:]
            nome = f"{base_nome} {titulo(resto)}" if resto else base_nome
        nomes_golpes = sorted({nome_golpe[m] for m in pool if m in nome_golpe})
        checar_campo(nome, LIMITE_NOME, "nome")
        for g in nomes_golpes:
            checar_campo(g, LIMITE_ATAQUE, "golpe")
        linhas.append([pid, p["identifier"], nome] + [st[i] for i in range(1, 7)]
                      + [esp_genero[sid], "|".join(nomes_golpes)])

    linhas.sort(key=lambda r: int(r[0]))
    with open(BASE_ESPECIES, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["pokeapi_id", "identificador", "nome", "hp", "ataque", "defesa",
                    "ataque_esp", "defesa_esp", "velocidade", "gender_rate", "golpes"])
        w.writerows(linhas)
    print(f"  {len(linhas)} Pokemon/variantes gravados em {BASE_ESPECIES}")


def carregar_base():
    with open(BASE_ESPECIES, newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def quatro_golpes(rng, pool):
    escolhidos = rng.sample(pool, 4) if len(pool) >= 4 else list(pool)
    while len(escolhidos) < 4:      # poucas especies (ex.: Ditto) tem menos de 4 golpes
        escolhidos.append("-")      # "-" = slot vazio, como no jogo
    return escolhidos


def gerar(n, saida, semente, taxa_shiny):
    rng = random.Random(semente)
    especies = []
    for b in carregar_base():
        stats = [int(b[k]) for k in ("hp", "ataque", "defesa", "ataque_esp", "defesa_esp", "velocidade")]
        especies.append((b["nome"], sum(stats), int(b["gender_rate"]), b["golpes"].split("|")))

    with open(saida, "w", newline="", encoding="utf-8") as f:
        f.write("id,nome,genero,shiny,ataque1,ataque2,ataque3,ataque4,ivs,total_stats\n")
        for i in range(1, n + 1):
            ivs = rng.randint(1, 100)
            nome, total_base, gr, pool = rng.choice(especies)
            shiny = 1 if rng.random() < taxa_shiny else 0
            if gr < 0:                               # gender_rate -1 = sem genero (ex.: Magnemite)
                genero = "sem genero"
            else:                                    # gender_rate = chance de femea em oitavos
                genero = "femea" if rng.random() < gr / 8 else "macho"
            total = total_base + round(6 * ivs * FATOR_ESCALA)   # = base + 3*ivs
            g = quatro_golpes(rng, pool)
            f.write(f"{i},{nome},{genero},{shiny},{g[0]},{g[1]},{g[2]},{g[3]},{ivs},{total}\n")
    print(f"{n} registros gravados em {saida}")


def main():
    ap = argparse.ArgumentParser(description="Gera o dataset PokeTree com dados reais da PokeAPI.")
    ap.add_argument("-n", type=int, default=500000, help="quantidade de registros (padrao 500000)")
    ap.add_argument("-o", "--saida", default=SAIDA_PADRAO)
    ap.add_argument("--semente", type=int, default=42)
    ap.add_argument("--taxa-shiny", type=float, default=0.01, help="chance de shiny (padrao 1%%)")
    ap.add_argument("--refazer-base", action="store_true", help="rebaixa os dados da PokeAPI")
    a = ap.parse_args()

    if a.refazer_base or not os.path.exists(BASE_ESPECIES):
        construir_base()
    gerar(a.n, a.saida, a.semente, a.taxa_shiny)


if __name__ == "__main__":
    main()

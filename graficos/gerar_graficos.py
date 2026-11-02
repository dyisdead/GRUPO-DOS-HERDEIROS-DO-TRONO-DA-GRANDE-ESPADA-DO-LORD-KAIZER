#!/usr/bin/env python3
"""
Gera os graficos do relatorio a partir de resultados/resultados_desempenho.csv
(criado por ./teste_desempenho).

Uso (na raiz do projeto):
    python graficos/gerar_graficos.py

Precisa de: matplotlib  (pip install matplotlib)

Graficos gerados em graficos/:
  1_tempo_por_operacao.png    tempo medio por insercao/busca/remocao x n (eixo x em log)
                              comparado com a curva teorica c * log2(n)
  2_comparacoes_por_busca.png comparacoes de chave por busca x n, comparado com log2(n)
  3_altura.png                altura x n nas 3 ordens, com os limites teoricos
  4_rotacoes.png              rotacoes (e recoloracoes) por operacao, n = maior tamanho
  5_tempo_total_insercao.png  tempo total para inserir n registros (log-log) x n*log2(n)
  6_memoria.png               memoria ocupada pelos nos x n
"""

import csv
import math
import os
import sys
from collections import defaultdict

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.ticker import FuncFormatter
except ImportError:
    sys.exit("Erro: instale o matplotlib com:  pip install matplotlib")

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CSV = os.path.join(RAIZ, "resultados", "resultados_desempenho.csv")
SAIDA = os.path.join(RAIZ, "graficos")

# --- estilo -----------------------------------------------------------------
AZUL = "#2a78d6"      # AVL
LARANJA = "#eb6834"   # Rubro-Negra
CINZA = "#8a8984"     # curvas teoricas
TEXTO = "#0b0b0b"
TEXTO_2 = "#52514e"
GRADE = "#e4e3df"
FUNDO = "#fcfcfb"

ARVORES = {
    "AVL":        dict(rotulo="AVL", cor=AZUL, marcador="o", linha="-"),
    "RubroNegra": dict(rotulo="Rubro-Negra", cor=LARANJA, marcador="s", linha="--"),
}
ORDENS = ["aleatoria", "crescente", "decrescente"]
NOME_ORDEM = {"aleatoria": "Ordem aleatória", "crescente": "Ordem crescente",
              "decrescente": "Ordem decrescente"}

plt.rcParams.update({
    "figure.facecolor": FUNDO, "axes.facecolor": FUNDO, "savefig.facecolor": FUNDO,
    "axes.edgecolor": GRADE, "axes.labelcolor": TEXTO_2, "text.color": TEXTO,
    "xtick.color": TEXTO_2, "ytick.color": TEXTO_2,
    "axes.grid": True, "grid.color": GRADE, "grid.linewidth": 0.8,
    "axes.spines.top": False, "axes.spines.right": False,
    "font.size": 10, "axes.titlesize": 11, "axes.titleweight": "bold",
    "legend.frameon": False, "lines.linewidth": 2, "lines.markersize": 6,
})


def ler_csv():
    if not os.path.exists(CSV):
        sys.exit(f"Erro: {CSV} nao existe. Rode antes:  ./teste_desempenho")
    dados = defaultdict(dict)   # (arvore, ordem, metrica) -> {n: (media, desvio)}
    with open(CSV, newline="", encoding="utf-8") as f:
        for l in csv.DictReader(f):
            dados[(l["arvore"], l["ordem"], l["metrica"])][int(l["n"])] = (
                float(l["media"]), float(l["desvio_padrao"]))
    return dados


def serie(dados, arvore, ordem, metrica, escala=1.0):
    pontos = dados.get((arvore, ordem, metrica), {})
    ns = sorted(pontos)
    return ns, [pontos[n][0] * escala for n in ns], [pontos[n][1] * escala for n in ns]


def formatar_n(eixo):
    def fmt(v, _):
        if v >= 1e6:
            return f"{v / 1e6:g}M"
        if v >= 1e3:
            return f"{v / 1e3:g}k"
        return f"{v:g}"
    eixo.set_major_formatter(FuncFormatter(fmt))


def plotar_arvores(ax, dados, ordem, metrica, escala=1.0, barras_erro=True):
    """Desenha AVL e RN; devolve (ns, valores) de todas para ajustar a curva teorica."""
    todos = []
    for arv, est in ARVORES.items():
        ns, med, desv = serie(dados, arv, ordem, metrica, escala)
        if not ns:
            continue
        ax.errorbar(ns, med, yerr=desv if barras_erro else None, color=est["cor"],
                    marker=est["marcador"], linestyle=est["linha"], label=est["rotulo"],
                    capsize=3, markeredgecolor=FUNDO, markeredgewidth=1.5)
        todos.append((ns, med))
    return todos


def curva_log_ajustada(ax, todos, rotulo="c · log₂(n)  (teórico)"):
    """Curva c*log2(n), com c ajustado por minimos quadrados aos dados medidos."""
    xs = [n for ns, _ in todos for n in ns]
    ys = [v for _, vs in todos for v in vs]
    if not xs:
        return
    num = sum(y * math.log2(x) for x, y in zip(xs, ys))
    den = sum(math.log2(x) ** 2 for x in xs)
    c = num / den
    nmin, nmax = min(xs), max(xs)
    grade = [nmin * (nmax / nmin) ** (i / 60) for i in range(61)]
    ax.plot(grade, [c * math.log2(n) for n in grade], color=CINZA, linestyle=":",
            linewidth=2, label=rotulo, zorder=0)


def salvar(fig, nome):
    caminho = os.path.join(SAIDA, nome)
    fig.savefig(caminho, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  {caminho}")


# --- graficos ---------------------------------------------------------------

def grafico_tempo_por_operacao(dados):
    fases = [("t_insercao_por_op", "Inserção"), ("t_busca_media", "Busca"),
             ("t_remocao_por_op", "Remoção")]
    fig, eixos = plt.subplots(1, 3, figsize=(14, 4.2))
    for ax, (metrica, titulo) in zip(eixos, fases):
        todos = plotar_arvores(ax, dados, "aleatoria", metrica, escala=1e6)
        curva_log_ajustada(ax, todos)
        ax.set_xscale("log")
        formatar_n(ax.xaxis)
        ax.set_title(titulo)
        ax.set_xlabel("n (registros na árvore, escala log)")
        ax.set_ylim(bottom=0)
    eixos[0].set_ylabel("tempo médio por operação (µs)")
    eixos[0].legend(loc="upper left")
    fig.suptitle("Tempo por operação × n (ordem aleatória), comparado com c · log₂(n)",
                 fontweight="bold", x=0.01, ha="left")
    fig.text(0.01, -0.04,
             "Acima de ~50 mil registros a árvore (12 MB ou mais) deixa de caber no cache da CPU e cada nível "
             "visitado passa a custar um acesso à RAM;\npor isso o tempo de relógio cresce mais rápido que "
             "log₂(n). O número de passos, que é o que a notação O mede, segue log₂(n) exatamente (gráfico 2).",
             ha="left", va="top", fontsize=9, color=TEXTO_2)
    fig.tight_layout()
    salvar(fig, "1_tempo_por_operacao.png")


def grafico_comparacoes(dados):
    fig, eixos = plt.subplots(1, 3, figsize=(14, 4.2), sharey=True)
    for ax, ordem in zip(eixos, ORDENS):
        todos = plotar_arvores(ax, dados, ordem, "comparacoes_busca_media", barras_erro=False)
        xs = sorted({n for ns, _ in todos for n in ns})
        if xs:
            grade = [xs[0] * (xs[-1] / xs[0]) ** (i / 60) for i in range(61)]
            ax.plot(grade, [math.log2(n) for n in grade], color=CINZA, linestyle=":",
                    linewidth=2, label="log₂(n)  (árvore perfeita)", zorder=0)
        ax.set_xscale("log")
        formatar_n(ax.xaxis)
        ax.set_title(NOME_ORDEM[ordem])
        ax.set_xlabel("n (escala log)")
        ax.set_ylim(bottom=0)
    eixos[0].set_ylabel("comparações de chave por busca")
    eixos[0].legend(loc="upper left")
    fig.suptitle("Comparações por busca × n — medida independente de hardware",
                 fontweight="bold", x=0.01, ha="left")
    fig.tight_layout()
    salvar(fig, "2_comparacoes_por_busca.png")


def grafico_altura(dados):
    fig, eixos = plt.subplots(1, 3, figsize=(14, 4.4), sharey=True)
    for ax, ordem in zip(eixos, ORDENS):
        todos = plotar_arvores(ax, dados, ordem, "altura", barras_erro=False)
        xs = sorted({n for ns, _ in todos for n in ns})
        if xs:
            grade = [xs[0] * (xs[-1] / xs[0]) ** (i / 60) for i in range(61)]
            ax.plot(grade, [2 * math.log2(n + 1) for n in grade], color=LARANJA, alpha=0.35,
                    linewidth=1.5, linestyle="-.", label="limite RN: 2·log₂(n+1)", zorder=0)
            ax.plot(grade, [1.44 * math.log2(n + 2) for n in grade], color=AZUL, alpha=0.35,
                    linewidth=1.5, linestyle="-.", label="limite AVL: 1,44·log₂(n+2)", zorder=0)
            ax.plot(grade, [math.log2(n + 1) for n in grade], color=CINZA, linestyle=":",
                    linewidth=2, label="mínimo: log₂(n+1)", zorder=0)
        ax.set_xscale("log")
        formatar_n(ax.xaxis)
        ax.set_title(NOME_ORDEM[ordem])
        ax.set_xlabel("n (escala log)")
        ax.set_ylim(bottom=0)
    eixos[0].set_ylabel("altura da árvore")
    eixos[0].legend(loc="upper left", fontsize=8.5)
    fig.suptitle("Altura × n — ambas ficam abaixo do limite teórico (uma BST simples teria altura n em ordem)",
                 fontweight="bold", x=0.01, ha="left")
    fig.tight_layout()
    salvar(fig, "3_altura.png")


def grafico_rotacoes(dados):
    n_max = max(n for (a, o, m), pts in dados.items() for n in pts)
    metricas = [("rotacoes_insercao", n_max, "Rotações por inserção"),
                ("rotacoes_remocao", n_max // 10, "Rotações por remoção"),
                ("recoloracoes_insercao", n_max, "Recolorações por inserção (só RN)")]
    fig, eixos = plt.subplots(1, 3, figsize=(14, 4.2))
    largura = 0.38
    for ax, (metrica, qtd_ops, titulo) in zip(eixos, metricas):
        arvores = ["RubroNegra"] if metrica.startswith("recolor") else list(ARVORES)
        for i, arv in enumerate(arvores):
            est = ARVORES[arv]
            valores = [dados[(arv, o, metrica)][n_max][0] / qtd_ops for o in ORDENS]
            desloc = 0 if len(arvores) == 1 else (i - 0.5) * largura
            xs = [k + desloc for k in range(len(ORDENS))]
            barras = ax.bar(xs, valores, width=largura - 0.04, color=est["cor"],
                            label=est["rotulo"], hatch=None if arv == "AVL" else "//",
                            edgecolor=FUNDO, linewidth=0)
            for b, v in zip(barras, valores):
                ax.annotate(f"{v:.2f}", (b.get_x() + b.get_width() / 2, v),
                            ha="center", va="bottom", fontsize=8.5, color=TEXTO_2,
                            xytext=(0, 2), textcoords="offset points")
        ax.set_xticks(range(len(ORDENS)), [NOME_ORDEM[o].replace("Ordem ", "") for o in ORDENS])
        ax.set_title(titulo)
        ax.grid(axis="x", visible=False)
        ax.margins(y=0.15)
    eixos[0].set_ylabel("média por operação")
    eixos[0].legend(loc="upper left")
    fig.suptitle(f"Custo de rebalanceamento (n = {n_max:,})".replace(",", "."),
                 fontweight="bold", x=0.01, ha="left")
    fig.tight_layout()
    salvar(fig, "4_rotacoes.png")


def grafico_tempo_total(dados):
    fig, ax = plt.subplots(figsize=(7.5, 4.6))
    todos = plotar_arvores(ax, dados, "aleatoria", "t_insercao", escala=1e3)
    xs = [n for ns, _ in todos for n in ns]
    ys = [v for _, vs in todos for v in vs]
    if xs:
        c = sum(y * x * math.log2(x) for x, y in zip(xs, ys)) / sum((x * math.log2(x)) ** 2 for x in xs)
        grade = [min(xs) * (max(xs) / min(xs)) ** (i / 60) for i in range(61)]
        ax.plot(grade, [c * n * math.log2(n) for n in grade], color=CINZA, linestyle=":",
                linewidth=2, label="c · n·log₂(n)  (teórico)", zorder=0)
    ax.set_xscale("log")
    ax.set_yscale("log")
    formatar_n(ax.xaxis)
    ax.set_xlabel("n (escala log)")
    ax.set_ylabel("tempo total de inserção (ms, escala log)")
    ax.set_title("Tempo para inserir n registros (ordem aleatória)", loc="left")
    ax.legend(loc="upper left")
    fig.tight_layout()
    salvar(fig, "5_tempo_total_insercao.png")


def grafico_memoria(dados):
    fig, ax = plt.subplots(figsize=(7.5, 4.6))
    for arv, est in ARVORES.items():
        ns, med, _ = serie(dados, arv, "aleatoria", "memoria_bytes", 1 / (1024 * 1024))
        if not ns:
            continue
        por_no = dados[(arv, "aleatoria", "bytes_por_no")][ns[-1]][0]
        ax.plot(ns, med, color=est["cor"], marker=est["marcador"], linestyle=est["linha"],
                label=f'{est["rotulo"]} ({por_no:.0f} bytes por nó)',
                markeredgecolor=FUNDO, markeredgewidth=1.5)
    formatar_n(ax.xaxis)
    ax.set_xlabel("n")
    ax.set_ylabel("memória dos nós (MB)")
    ax.set_title("Memória ocupada × n — cresce em O(n)", loc="left")
    ax.legend(loc="upper left")
    ax.set_ylim(bottom=0)
    fig.tight_layout()
    salvar(fig, "6_memoria.png")


def main():
    dados = ler_csv()
    os.makedirs(SAIDA, exist_ok=True)
    print("Gerando graficos:")
    grafico_tempo_por_operacao(dados)
    grafico_comparacoes(dados)
    grafico_altura(dados)
    grafico_rotacoes(dados)
    grafico_tempo_total(dados)
    grafico_memoria(dados)


if __name__ == "__main__":
    main()

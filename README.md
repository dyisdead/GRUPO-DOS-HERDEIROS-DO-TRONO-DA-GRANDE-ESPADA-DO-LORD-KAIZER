# GRUPO DOS HERDEIROS DO TRONO DA GRANDE ESPADA DO LORD KAIZER

**PokéTree: AVL vs Rubro-Negra** *(sugestão de nome — trocar se o grupo preferir outro)*

Entrega 1 do trabalho da disciplina Estruturas de Dados II — UNICID
Prof. Cid Rodrigues de Andrade

## 👥 Integrantes do Grupo

| Nome completo | RA |
|---|---|
| Diogo Henrique Pinheiro Da Silva | _____ |
| Pedro Bruno Theodoro Nascimento | _____ |

---

## 1. Dataset

### 1.1 Descrição

Dataset sintético combinando **Pokémons e personagens humanos**, gerado a partir de dados base da PokéAPI. Cada registro representa uma "criatura" ou personagem com atributos como espécie/nome, gênero, variante shiny, 4 ataques e valores de IV. Formato: CSV/JSON. Volume alvo: **entre 300 mil e 500 mil registros**, dentro da faixa exigida (50 mil a 1 milhão).

### 1.2 Fonte

- Dados base de espécies e movimentos: [PokéAPI](https://pokeapi.co/)
- Registros individuais (instâncias): gerados pelo grupo por amostragem aleatória dentro do espaço de combinações possíveis (não é uma enumeração exaustiva de todas as combinações, que ultrapassaria centenas de milhões)

### 1.3 Estrutura dos dados

- `id` (int)
- `nome` (string) — nome do Pokémon ou personagem humano
- `tipo` (string) — "pokemon" ou "humano"
- `genero` (string)
- `shiny` (bool)
- `ataques` (array de 4 strings) — golpes escolhidos de um pool por criatura
- `ivs` (int, 1 a 100) — valor individual usado nos cálculos de status
- `total_stats` (int) — **chave de inserção, busca e comparação nas árvores**

### 1.4 Justificativa da escolha

`total_stats` é um valor numérico com boa distribuição entre os registros, o que favorece testes de balanceamento tanto na AVL quanto na Rubro-Negra. O volume de centenas de milhares de registros permite avaliar o comportamento das árvores em escala e comparar o crescimento real de tempo/altura com a complexidade teórica O(log n).

---

## 2. Estrutura(s) de Árvore Escolhida(s)

### 2.1 Estrutura(s)

- **AVL**
- **Rubro-Negra**

### 2.2 Justificativa técnica

As duas estruturas resolvem o problema de degeneração da BST simples (que pode virar uma lista encadeada com dados ordenados), mas com trade-offs diferentes:

- **AVL**: balanceamento estrito (fator de altura entre -1 e 1), resultando em árvores mais baixas e buscas mais rápidas, ao custo de mais rotações em inserção/remoção.
- **Rubro-Negra**: regras de balanceamento mais flexíveis (baseadas em cor dos nós), com menos rotações por operação de escrita, em troca de uma altura ligeiramente maior.

Escolher as duas permite comparar na prática o trade-off **leitura vs. escrita** entre estruturas auto-balanceadas.

### 2.3 Operações implementadas (Para Entrega 2)

- [ ] Inserção
- [ ] Remoção
- [ ] Busca
- [ ] Percursos (pré-ordem, em ordem, pós-ordem)
- [ ] Balanceamento (rotações/recoloração)
- [ ] Outra: ______

### 2.4 Complexidade

| Operação | Melhor caso | Caso médio | Pior caso |
|---|---|---|---|
| Inserção | O(log n) | O(log n) | O(log n) |
| Busca | O(log n) | O(log n) | O(log n) |
| Remoção | O(log n) | O(log n) | O(log n) |

*Tanto AVL quanto Rubro-Negra garantem O(log n) no pior caso por serem auto-balanceadas — diferente da BST simples, cujo pior caso degenera para O(n).*

---

## 3. Plano de Testes

### 3.1 Objetivo dos testes

Validar a corretude das operações (inserção, busca, remoção) e o balanceamento das árvores, além de medir e comparar o desempenho (tempo, altura, rotações e memória) entre AVL e Rubro-Negra à medida que o volume de dados cresce.

### 3.2 Cenários de teste

| Cenário | Entrada | Resultado esperado | Status |
|---|---|---|---|
| 1 | Inserir dataset completo em cada árvore | Todas as árvores mantêm suas invariantes de balanceamento após a carga | ☐ |
| 2 | Buscas aleatórias em amostra representativa | Tempo médio de busca compatível com O(log n) | ☐ |
| 3 | Remoções em lote seguidas de validação de invariantes | Estrutura permanece válida (AVL: fator -1 a 1; RN: 5 regras de cor) | ☐ |

### 3.3 Casos extremos (edge cases)

- Árvore vazia
- Único elemento
- Dados duplicados (mesmo `total_stats`)
- Dados inseridos em ordem crescente/decrescente (pior caso para BST simples)
- Volume máximo do dataset (~500 mil registros)

### 3.4 Testes de desempenho (Para Entrega 2)

Medir tempo de execução e uso de memória com tamanhos de entrada crescentes (ex: 1.000, 10.000, 100.000, 500.000 registros), comparando o número de rotações/comparações entre AVL e Rubro-Negra.

### 3.5 Resultados obtidos (Para Entrega 2)

_A preencher na Entrega 2 com tabelas, gráficos de tempo × tamanho de entrada (escala log) e comparação com a complexidade Big-O teórica. Arquivos de saída em `/resultados`._

---

## 4. Como Executar

### 4.1 Pré-requisitos (Para Entrega 2)

_A definir: linguagem, versão e dependências._

### 4.2 Instruções (Para Entrega 2)

```
git clone <repositorio>
cd <pasta-do-projeto>
# comandos de compilação/execução
```

### 4.3 Estrutura do repositório

```
/src         → código-fonte
/dataset     → dataset utilizado
/testes      → scripts e casos de teste
/resultados  → saídas e relatórios de desempenho
README.md
```

---

## 5. Referências

- PokéAPI — https://pokeapi.co/
- _Adicionar demais materiais consultados (livros, artigos, aulas)_

# GRUPO DOS HERDEIROS DO TRONO DA GRANDE ESPADA DO LORD KAIZER

PokéTree: AVL vs Rubro-Negra

Entrega 1 do trabalho da disciplina Estruturas de Dados II - UNICID
Prof. Cid Rodrigues de Andrade

## Integrantes do Grupo

| Nome completo | RA |
|---|---|
| Diogo Henrique Pinheiro da Silva Pedro | 43614442 |
| Pedro Henrique Segala | 43695469 |
| Marconio Soares de Sousa Junior | 43840868 |
| Bruno Oliveira Theodoro | 43023452 |
| Kauan Alba Elias | 43555829 |

---

## 1. Dataset

### 1.1 Descrição

Dataset combinando Pokémons e personagens humanos, gerado a partir de dados base da PokéAPI. Cada registro representa uma criatura ou personagem com atributos como espécie/nome, gênero, variante shiny, 4 ataques e valores de IV. Formato: CSV/JSON. Volume alvo: entre 300 mil e 500 mil registros, dentro da faixa exigida (10 mil a 10 milhões).

### 1.2 Fonte

* Dados base de espécies e movimentos: PokéAPI (https://pokeapi.co/)
* Registros individuais: gerados pelo grupo por amostragem aleatória dentro do espaço de combinações possíveis. Não é uma enumeração exaustiva de todas as combinações, que passaria de centenas de milhões

### 1.3 Estrutura dos dados

* id (int): identificador único e sequencial do registro, atribuído na geração do dataset; é o campo de desempate final da chave composta
* nome (string): nome do Pokémon ou personagem humano
* tipo (string): "pokemon" ou "humano"
* genero (string)
* shiny (bool)
* ataques (array de 4 strings): golpes escolhidos de um pool por criatura
* ivs (int, 1 a 100): valor individual usado no cálculo de total_stats
* total_stats (int): ver cálculo e papel na chave composta em 1.3.1

### 1.3.1 Cálculo de total_stats

total_stats é derivado dos status base da espécie (HP, Ataque, Defesa, Ataque Especial, Defesa Especial e Velocidade, obtidos da PokéAPI) ajustados pelo IV do registro:

```
stat_final_i = status_base_i + (ivs * fator_escala)   // fator_escala = 0.5
total_stats  = soma dos 6 stat_final_i
```

Para personagens humanos (sem status base na PokéAPI), o grupo usa uma tabela de status base fixa definida internamente, aplicando a mesma fórmula.

### 1.3.2 Chave composta e comparador

A chave de inserção, busca e comparação nas árvores é composta:

```
chave = (total_stats, nome, id)
```

Comparador (ordem lexicográfica, nessa ordem de prioridade):

1. **total_stats** — critério principal; é o campo que o grupo quer observar em termos de distribuição e balanceamento.
2. **nome** — desempate quando dois registros têm o mesmo total_stats.
3. **id** — desempate final; como é único e sequencial, garante que a chave composta nunca tenha empate total, mesmo entre registros com mesmo total_stats e mesmo nome (ex.: dois Pikachu shiny com os mesmos stats).

Isso significa que inserção, busca e remoção por chave deixam de ser ambíguas mesmo com total_stats tendo só algumas centenas de valores possíveis para até 500 mil registros: o terceiro campo (id) sempre resolve o empate, e a altura da árvore passa a refletir corretamente o volume de dados inserido.

### 1.4 Justificativa da escolha

total_stats é um valor numérico com boa distribuição entre os registros, o que ajuda nos testes de balanceamento da AVL e da Rubro-Negra; nome e id complementam a chave garantindo unicidade sem distorcer essa distribuição. O volume de centenas de milhares de registros permite ver o comportamento das árvores em escala e comparar o crescimento real de tempo/altura com a complexidade teórica O(log n).

---

## 2. Estrutura(s) de Árvore Escolhida(s)

### 2.1 Estrutura(s)

* AVL
* Rubro-Negra

### 2.2 Justificativa técnica

As duas estruturas resolvem o problema de degeneração da BST simples, que pode virar uma lista encadeada quando os dados são inseridos em ordem. Mas cada uma tem um trade-off diferente:

* AVL: balanceamento estrito (fator de altura entre -1 e 1). Árvore mais baixa e buscas mais rápidas, com mais rotações em inserção/remoção
* Rubro-Negra: regras de balanceamento mais flexíveis, baseadas em cor dos nós. Menos rotações por operação de escrita, com altura um pouco maior

Escolhemos as duas pra comparar na prática o trade-off entre leitura e escrita.

### 2.3 Operações implementadas (Para Entrega 2)

* [ ] Inserção
* [ ] Remoção
* [ ] Busca
* [ ] Percursos (pré-ordem, em ordem, pós-ordem)
* [ ] Balanceamento (rotações/recoloração)
* [ ] Outra: ______

### 2.4 Complexidade

| Operação | Melhor caso | Caso médio | Pior caso |
|---|---|---|---|
| Inserção | O(log n) | O(log n) | O(log n) |
| Busca | O(log n) | O(log n) | O(log n) |
| Remoção | O(log n) | O(log n) | O(log n) |

AVL e Rubro-Negra garantem O(log n) no pior caso por serem auto-balanceadas. Já a BST simples degenera pra O(n) no pior caso.

---

## 3. Plano de Testes

### 3.1 Objetivo dos testes

Validar a corretude das operações (inserção, busca, remoção) e o balanceamento das árvores, e medir e comparar o desempenho (tempo, altura, rotações e memória) entre AVL e Rubro-Negra conforme o volume de dados cresce.

### 3.2 Cenários de teste

| Cenário | Entrada | Operação | Métricas registradas | Resultado esperado | Status |
|---|---|---|---|---|---|
| 1 | Inserir dataset completo em cada árvore | Inserção | Rotações e recolorações por inserção; tempo total | Todas as árvores mantêm as invariantes de balanceamento após a carga | ☐ |
| 2 | Buscas aleatórias em amostra representativa | Busca | Comparações por busca; tempo médio | Tempo médio de busca compatível com O(log n) | ☐ |
| 3 | Remoções em lote seguidas de validação de invariantes | Remoção | Rotações/recolorações por remoção; tempo total | Estrutura permanece válida (AVL: fator -1 a 1; RN: 5 regras de cor) | ☐ |

### 3.2.1 Metodologia de execução

* **Tamanhos de entrada testados:** 1.000 / 10.000 / 100.000 / 300.000 / 500.000 registros, inseridos em ordem progressiva pra observar o crescimento do tempo em cada faixa
* **Condições de medição:** todos os testes rodam na mesma máquina, logo após ligá-la, sem outros programas em segundo plano, pra minimizar ruído de medição
* **Aquecimento (warm-up):** a primeira execução de cada tamanho de entrada é descartada antes das 5 repetições, para reduzir efeitos de JIT/cache (relevante especialmente se a implementação for em Java ou Python)
* **Repetições por teste:** cada medição roda 5 vezes com o mesmo tamanho de entrada; o resultado reportado é a média e o desvio padrão das 5 execuções (evita que uma variação pontual da máquina distorça o gráfico)
* **Busca (Cenário 2):** amostra de 1.000 buscas aleatórias por rodada, metade com chaves existentes e metade com chaves inexistentes (pra testar também o caso de busca sem sucesso)
* **Remoção (Cenário 3):** remove-se 10% dos registros inseridos, escolhidos aleatoriamente, e as invariantes são checadas logo depois de cada lote de remoção (não só no final)
* **Ordem de inserção:** um teste com dados em ordem aleatória (caso médio/comum) e outro com dados em ordem crescente de total_stats (pior caso teórico pra uma BST simples), pra reforçar por que o balanceamento importa
* **Ferramenta de cronometragem:** função de tempo da própria linguagem escolhida pra implementação (ex: `time.perf_counter()` em Python ou `System.nanoTime()` em Java), medindo só o tempo da operação em si, sem contar geração/leitura do dataset

### 3.3 Casos extremos (edge cases)

| # | Caso | Comportamento esperado |
|---|---|---|
| 1 | Árvore vazia | Busca e remoção retornam "não encontrado" sem lançar erro/exceção |
| 2 | Único elemento | Inserção, busca (encontra) e remoção (árvore volta a ficar vazia) funcionam sem exigir rotações em nenhuma das duas árvores |
| 3 | Dados com mesmo total_stats (e, eventualmente, mesmo nome) | Como a chave é composta (total_stats, nome, id) e id é sempre único, todos os registros entram sem colisão real, mesmo com 3+ registros de total_stats idêntico |
| 4 | Inserção em ordem crescente de total_stats | Pior caso teórico para uma BST simples (degenera em lista, O(n)); AVL deve manter altura ≈ 1,44 · log₂(n) e Rubro-Negra ≤ 2 · log₂(n), evitando a degeneração |
| 5 | Inserção em ordem decrescente de total_stats | Comportamento simétrico ao caso crescente, com as mesmas garantias de altura |
| 6 | Volume máximo do dataset (~500 mil registros) | Tempo de inserção/busca cresce conforme O(log n) (não linearmente); altura final respeita os mesmos limites teóricos dos casos 4 e 5 |
| 7 | Remover todos os elementos até a árvore ficar vazia de novo | Estado final consistente: árvore vazia, sem nós residuais, e sujeita ao comportamento do caso 1 |

### 3.4 Testes de desempenho (Para Entrega 2)

Descreva como o grupo mediu tempo de execução e/ou uso de memória, e com quais tamanhos de entrada (ex: 100, 1.000, 10.000 registros).

### 3.5 Resultados obtidos (Para Entrega 2)

Resuma os resultados (tabelas, gráficos ou links para arquivos de saída na pasta /resultados) e compare-os com a complexidade assintótica (Big-O) teórica.

---

## 4. Como Executar

### 4.1 Pré-requisitos (Para Entrega 2)

Linguagem, versão e dependências necessárias.

### 4.2 Instruções (Para Entrega 2)

Exemplo:
* git clone / cd
* comandos de compilação/execução

### 4.3 Estrutura do repositório

```
/src         -> código-fonte
/dataset     -> dataset utilizado
/testes      -> scripts e casos de teste
/resultados  -> saídas e relatórios de desempenho
README.md
```

---

## 5. Referências

* PokéAPI - https://pokeapi.co/

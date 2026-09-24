# GRUPO DOS HERDEIROS DO TRONO DA GRANDE ESPADA DO LORD KAIZER

PokéTree: AVL vs Rubro-Negra (sugestão de nome, trocar se o grupo preferir outro)

Entrega 1 do trabalho da disciplina Estruturas de Dados II - UNICID
Prof. Cid Rodrigues de Andrade

## Integrantes do Grupo

| Nome completo | RA |
|---|---|
| Diogo Henrique Pinheiro Da Silva | 43614442 |
| Pedro Henrique Segala | 43695469 |
| Marconio Soares de Sousa Junior | 43840868 |
| Bruno Oliveira Theodoro | 43023452 |
| Kauan Albas Elias |  |

---

## 1. Dataset

### 1.1 Descrição

Dataset sintético combinando Pokémons e personagens humanos, gerado a partir de dados base da PokéAPI. Cada registro representa uma criatura ou personagem com atributos como espécie/nome, gênero, variante shiny, 4 ataques e valores de IV. Formato: CSV/JSON. Volume alvo: entre 300 mil e 500 mil registros, dentro da faixa exigida (50 mil a 1 milhão).

### 1.2 Fonte

- Dados base de espécies e movimentos: PokéAPI (https://pokeapi.co/).
- Registros individuais: gerados pelo grupo por amostragem aleatória dentro do espaço de combinações possíveis. Não é uma enumeração exaustiva de todas as combinações, que passaria de centenas de milhões.

### 1.3 Estrutura dos dados

- id (int)
- nome (string): nome do Pokémon ou personagem humano
- tipo (string): "pokemon" ou "humano"
- genero (string)
- shiny (bool)
- ataques (array de 4 strings): golpes escolhidos de um pool por criatura
- ivs (int, 1 a 100): valor individual usado nos cálculos de status
- total_stats (int): chave de inserção, busca e comparação nas árvores

### 1.4 Justificativa da escolha

total_stats é um valor numérico com boa distribuição entre os registros, o que ajuda nos testes de balanceamento da AVL e da Rubro-Negra. O volume de centenas de milhares de registros permite ver o comportamento das árvores em escala e comparar o crescimento real de tempo/altura com a complexidade teórica O(log n).

---

## 2. Estrutura(s) de Árvore Escolhida(s)

### 2.1 Estrutura(s)

- AVL
- Rubro-Negra

### 2.2 Justificativa técnica

As duas estruturas resolvem o problema de degeneração da BST simples, que pode virar uma lista encadeada quando os dados são inseridos em ordem. Mas cada uma tem um trade-off diferente:

- AVL: balanceamento estrito (fator de altura entre -1 e 1). Árvore mais baixa e buscas mais rápidas, com mais rotações em inserção/remoção
- Rubro-Negra: regras de balanceamento mais flexíveis, baseadas em cor dos nós. Menos rotações por operação de escrita, com altura um pouco maior

Escolhemos as duas pra comparar na prática o trade-off entre leitura e escrita.

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

AVL e Rubro-Negra garantem O(log n) no pior caso por serem auto-balanceadas. Já a BST simples degenera pra O(n) no pior caso.

---

## 3. Plano de Testes

### 3.1 Objetivo dos testes

Validar a corretude das operações (inserção, busca, remoção) e o balanceamento das árvores, e medir e comparar o desempenho (tempo, altura, rotações e memória) entre AVL e Rubro-Negra conforme o volume de dados cresce.

### 3.2 Cenários de teste

| Cenário | Entrada | Resultado esperado | Status |
|---|---|---|---|
| 1 | Inserir dataset completo em cada árvore | Todas as árvores mantêm as invariantes de balanceamento após a carga | ☐ |
| 2 | Buscas aleatórias em amostra representativa | Tempo médio de busca compatível com O(log n) | ☐ |
| 3 | Remoções em lote seguidas de validação de invariantes | Estrutura permanece válida (AVL: fator -1 a 1; RN: 5 regras de cor) | ☐ |

### 3.3 Casos extremos (edge cases)

- Árvore vazia
- Único elemento
- Dados duplicados (mesmo total_stats)
- Dados inseridos em ordem crescente/decrescente (pior caso pra BST simples)
- Volume máximo do dataset (~500 mil registros)

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
- git clone / cd
- comandos de compilação/execução

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

- PokéAPI - https://pokeapi.co/
- Adicionar demais materiais consultados (livros, artigos, aulas)

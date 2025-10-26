# TP2 – Banco de Dados I (UFAM) – 2025/02

Repositório da equipe para o Trabalho Prático 2.  


---

##  Estrutura do projeto

```
app/
├── src/                    # Código-fonte (.cpp)
├── include/                # Cabeçalhos (.h)
├── bin/                    # Executáveis gerados
├── data/                   # Arquivos de dados e índices
├── tests/                  # Scripts ou dados de teste
├── Dockerfile              # Configuração Docker
├── docker-compose.yml      # Orquestração Docker
├── Makefile               # Automação de build
├── README.md              # Este arquivo
└── TP2-BD1-2025-02.docx.pdf  # Especificação do trabalho
```

---

##  Como compilar localmente

1. **Navegue até a pasta do projeto:**
   ```bash
   cd app/
   ```

2. **Compile todos os binários:**
   ```bash
   make build
   ```
   Os executáveis serão gerados na pasta `bin/`.

3. **Exemplo de execução de teste:**
   ```bash
   ./bin/findrec   # nesse caso ainda falta o ID
   ```

> **Nota:** Se ainda não houver arquivos `.cpp`, o Makefile funciona como skeleton e não gera binários.

##  Como executar os programas

###  Programas disponíveis

**`upload`** – Cria os arquivos de dados e índices a partir do CSV de entrada:
```bash
./bin/upload /data/artigo.csv
```

**`findrec`** – Busca um registro diretamente pelo ID no arquivo de dados:
```bash
./bin/findrec 123
```

**`seek1`** – Busca pelo ID usando o índice primário:
```bash
./bin/seek1 123
```

**`seek2`** – Busca pelo Título usando o índice secundário:
```bash
./bin/seek2 "Spatially-multiplexed MIMO markers."
```

###  Saída dos programas
Todos os programas imprimem:
- Total de registros encontrados;
- Total de blocos no arquivo de dados;
- Quantidade de blocos lidos na operação;
- Tempo total de execução em milissegundos.

##  Testando com Makefile e Docker

###  Comandos locais (Makefile)
```bash
# Build local
make build

# Limpar binários - caso já existam
make clean
```

###  Comandos Docker
```bash
# Build da imagem Docker
make docker-build

# Executar upload no Docker
make docker-run-upload

# Executar findrec no Docker
make docker-run-findrec

# Executar seek1 no Docker
make docker-run-seek1

# Executar seek2 no Docker
make docker-run-seek2
```

> **Nota:** Para todos os comandos Docker, o diretório `/data` é montado como volume e usado para entrada e persistência.

---

##  Requisitos

- **C++11** ou superior
- **Make** para automação de build
- **Docker** (opcional, para execução containerizada)

##  Equipe
- Guilherme Dias Corrêa - 22352926
- Luiza da Costa Caxeixa - 22354553
- Sofia de Castro Sato  - 22354600


Trabalho Prático 2 - Banco de Dados I  
UFAM - 2025/02


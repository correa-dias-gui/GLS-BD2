# Dockerfile - básico
FROM debian:bookworm-slim

# Instala dependências
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    cmake \
    && rm -rf /var/lib/apt/lists/*

# Diretório de trabalho
WORKDIR /app

# Copia o projeto
COPY . /app

# Compila os binários
RUN make build

# Configura volumes
VOLUME ["/data"]

# Variáveis de ambiente
ENV DATA_DIR=/data \
    LOG_LEVEL=info \
    CSV_PATH=/data/input.csv

# Comando default
CMD ["bash"]

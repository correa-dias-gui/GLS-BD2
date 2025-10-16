# Dockerfile - aplicação C++ com logs e variáveis de ambiente
FROM debian:bookworm-slim

# Instalar dependências
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    cmake \
    && rm -rf /var/lib/apt/lists/*

# Diretório de trabalho dentro do container
WORKDIR /app

# Copiar projeto inteiro
COPY . .

# Definir variáveis de ambiente padrão
ENV DATA_DIR=/app/data \
    LOG_LEVEL=info \
    CSV_PATH=/app/data/input.csv

# Compilar o projeto (gera binários em /app/bin)
RUN make build

# Montar /app/data como volume (para persistência)
VOLUME ["/app/data"]

# Comando padrão (pode mudar para ./bin/main futuramente)
CMD ["bash"]

# Makefile - TP2 Banco de Dados I

CXX = g++
CXXFLAGS = -Wall -std=c++17
SRC_DIR = src
BIN_DIR = bin
INCLUDE_DIR = include

# Arquivos que têm função main() e devem virar executáveis
MAIN_SOURCES = findrec.cpp upload.cpp seek1.cpp seek2.cpp
EXECUTABLES = $(patsubst %.cpp,$(BIN_DIR)/%,$(MAIN_SOURCES))

# Biblioteca (sem main)
LIB_SOURCES = BTplus_mem.cpp hash.cpp

.PHONY: build clean docker-build

# Compila todos os executáveis
build: $(BIN_DIR) $(EXECUTABLES)

# Cria diretório bin se não existir
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Regras específicas para cada executável
$(BIN_DIR)/findrec: $(SRC_DIR)/findrec.cpp
	$(CXX) $(CXXFLAGS) -I $(INCLUDE_DIR) $< -o $@

# $(BIN_DIR)/main: $(SRC_DIR)/main.cpp $(SRC_DIR)/BTplus_mem.cpp
# 	$(CXX) $(CXXFLAGS) -I $(INCLUDE_DIR) $^ -o $@

# $(BIN_DIR)/arvore: $(SRC_DIR)/arvore.cpp
# 	$(CXX) $(CXXFLAGS) -I $(INCLUDE_DIR) $< -o $@

# $(BIN_DIR)/parser: $(SRC_DIR)/parser.cpp
# 	$(CXX) $(CXXFLAGS) -I $(INCLUDE_DIR) $< -o $@

# Programas principais do TP2 (quando implementados)
$(BIN_DIR)/upload: $(SRC_DIR)/upload.cpp $(SRC_DIR)/BTplus_mem.cpp $(SRC_DIR)/hash.cpp
	$(CXX) $(CXXFLAGS) -I $(INCLUDE_DIR) $^ -o $@

$(BIN_DIR)/seek1: $(SRC_DIR)/seek1.cpp $(SRC_DIR)/BTplus_mem.cpp
	$(CXX) $(CXXFLAGS) -I $(INCLUDE_DIR) $^ -o $@

$(BIN_DIR)/seek2: $(SRC_DIR)/seek2.cpp $(SRC_DIR)/BTplus_mem.cpp
	$(CXX) $(CXXFLAGS) -I $(INCLUDE_DIR) $^ -o $@

# Remove binários
clean:
	rm -f $(BIN_DIR)/*

# Docker
docker-build:
	docker build -t tp2 .

docker-run-upload:
	docker run --rm -v "$(PWD)/data:/data" tp2 ./bin/upload /data/artigo.csv

docker-run-findrec:
	docker run --rm -v "$(PWD)/data:/data" tp2 ./bin/findrec 123

docker-run-seek1:
	docker run --rm -v "$(PWD)/data:/data" tp2 ./bin/seek1 123

docker-run-seek2:
	docker run --rm -v "$(PWD)/data:/data" tp2 ./bin/seek2 "Spatially-multiplexed MIMO markers."

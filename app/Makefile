# Makefile - Skeleton inicial

CXX = g++
CXXFLAGS = -Wall -std=c++17
SRC_DIR = src
BIN_DIR = bin
INCLUDE_DIR = include

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
TARGETS = $(patsubst $(SRC_DIR)/%.cpp,$(BIN_DIR)/%,$(SOURCES))

# Compila todos os binários
build: $(TARGETS)

$(BIN_DIR)/%: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I $(INCLUDE_DIR) $< -o $@

# Remove binários
clean:
	rm -f $(BIN_DIR)/*

# Docker
docker-build:
	docker build -t tp2 .

docker-run-upload:
	docker run --rm -v $(PWD)/data:/data tp2 ./bin/upload /data/input.csv

docker-run-findrec:
	docker run --rm -v $(PWD)/data:/data tp2 ./bin/findrec 123

docker-run-seek1:
	docker run --rm -v $(PWD)/data:/data tp2 ./bin/seek1 123

docker-run-seek2:
	docker run --rm -v $(PWD)/data:/data tp2 ./bin/seek2 "Um Título"

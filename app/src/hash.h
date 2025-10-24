#ifndef HASH_H
#define HASH_H

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>

using namespace std;

// ================================
// CONFIGURAÇÕES
// ================================
extern const int TAM_BLOCO_HASH;
extern const int ARTIGOS_POR_BLOCO;
extern const int NUM_BUCKETS;

#pragma pack(push, 1)
struct Artigo {
    int id;
    char titulo[301];
    int ano;
    char autores[151];
    int citacoes;
    char dataAtualizacao[21];
    char snippet[1025];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BlocoHeader {
    int64_t prox;  // offset do próximo bloco (-1 se não houver)
    int nRegs;     // quantos registros válidos neste bloco
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BucketInfo {
    int64_t offset;   // offset do primeiro bloco do bucket
    int nBlocos;      // total de blocos no bucket
    int nRegistros;   // total de registros no bucket
};
#pragma pack(pop)

// Estrutura de controle leve em RAM para cada bucket
struct ControleBucket {
    int64_t offsetPrimeiro = -1;
    int64_t offsetUltimo = -1;
    int registrosNoBloco = 0; // registros no bloco atual
    int nBlocos = 0;
    int nRegistros = 0;
};

// Função pública que gera data_hash.dat, hash.bin e index.bin a partir de data.bin
// Mantém política original: mínimo de RAM, overflow por bloco, 2.000 buckets
void gerarHash(const char *arquivoData);

#endif // HASH_H

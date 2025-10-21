#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>

using namespace std;

const int TAM_BLOCO = 4096;
const int ARTIGOS_POR_BLOCO = 2;
const int NUM_BUCKETS = 2000;

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

// Função hash simples
int funcaoHash(int id) {
    return id % NUM_BUCKETS;
}

// Estrutura de controle leve em RAM para cada bucket
struct ControleBucket {
    int64_t offsetPrimeiro = -1;
    int64_t offsetUltimo = -1;
    int registrosNoBloco = 0; // registros no bloco atual
    int nBlocos = 0;
    int nRegistros = 0;
};

int main() {
    ifstream dataIn("data.bin", ios::binary);
    if (!dataIn) { cerr << "Erro ao abrir data.bin\n"; return 1; }

    // Cria ou sobrescreve data_hash.dat
    fstream dataOut("data_hash.dat", ios::in | ios::out | ios::binary | ios::trunc);
    if (!dataOut) { cerr << "Erro ao criar data_hash.dat\n"; return 1; }

    vector<ControleBucket> controle(NUM_BUCKETS);

    Artigo a;
    while (dataIn.read(reinterpret_cast<char*>(&a), sizeof(Artigo))) {
        int b = funcaoHash(a.id);
        auto &bucket = controle[b];

        // Se bucket ainda não tem bloco, cria o primeiro
        if (bucket.offsetPrimeiro == -1) {
            dataOut.seekp(0, ios::end);
            int64_t novoOffset = dataOut.tellp();

            BlocoHeader header = { -1, 0 };
            dataOut.write(reinterpret_cast<char*>(&header), sizeof(header));
            vector<char> espaco(TAM_BLOCO - sizeof(header), 0);
            dataOut.write(espaco.data(), espaco.size());

            bucket.offsetPrimeiro = novoOffset;
            bucket.offsetUltimo = novoOffset;
            bucket.nBlocos = 1;
            bucket.registrosNoBloco = 0;
        }

        // Se bloco atual cheio, criar novo bloco de overflow
        if (bucket.registrosNoBloco == ARTIGOS_POR_BLOCO) {
            dataOut.seekp(0, ios::end);
            int64_t novoOffset = dataOut.tellp();

            // Atualiza header do bloco anterior
            BlocoHeader headerAnt;
            dataOut.seekg(bucket.offsetUltimo, ios::beg);
            dataOut.read(reinterpret_cast<char*>(&headerAnt), sizeof(headerAnt));
            headerAnt.prox = novoOffset;
            dataOut.seekp(bucket.offsetUltimo, ios::beg);
            dataOut.write(reinterpret_cast<char*>(&headerAnt), sizeof(headerAnt));

            // Cria novo bloco
            BlocoHeader novoHeader = { -1, 0 };
            dataOut.seekp(novoOffset, ios::beg);
            dataOut.write(reinterpret_cast<char*>(&novoHeader), sizeof(novoHeader));
            vector<char> esp(TAM_BLOCO - sizeof(novoHeader), 0);
            dataOut.write(esp.data(), esp.size());

            bucket.offsetUltimo = novoOffset;
            bucket.registrosNoBloco = 0;
            bucket.nBlocos++;
        }

        // Escreve artigo no bloco atual
        int64_t posArtigo = bucket.offsetUltimo + sizeof(BlocoHeader) + bucket.registrosNoBloco * sizeof(Artigo);
        dataOut.seekp(posArtigo, ios::beg);
        dataOut.write(reinterpret_cast<char*>(&a), sizeof(Artigo));

        bucket.registrosNoBloco++;
        bucket.nRegistros++;

        // Atualiza header do bloco atual
        BlocoHeader headerAtual;
        dataOut.seekg(bucket.offsetUltimo, ios::beg);
        dataOut.read(reinterpret_cast<char*>(&headerAtual), sizeof(headerAtual));
        headerAtual.nRegs = bucket.registrosNoBloco;
        dataOut.seekp(bucket.offsetUltimo, ios::beg);
        dataOut.write(reinterpret_cast<char*>(&headerAtual), sizeof(headerAtual));
    }

    dataIn.close();
    dataOut.close();

    // Cria hash.bin
    ofstream hashOut("hash.bin", ios::binary);
    if (!hashOut) { cerr << "Erro ao criar hash.bin\n"; return 1; }

    for (int i = 0; i < NUM_BUCKETS; ++i) {
        BucketInfo info;
        info.offset = controle[i].offsetPrimeiro;
        info.nBlocos = controle[i].nBlocos;
        info.nRegistros = controle[i].nRegistros;
        hashOut.write(reinterpret_cast<char*>(&info), sizeof(info));
    }
    hashOut.close();

    cout << "✅ Data hash criado com overflow, 2000 buckets, 2 artigos por bloco.\n";
    return 0;
}

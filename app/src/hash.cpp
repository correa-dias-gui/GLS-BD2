#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdint>

using namespace std;

const int TAM_BLOCO = 4096;
const int ARTIGOS_POR_BLOCO = 2;
const int NUM_BUCKETS = 1009;

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
struct BucketInfo {
    int64_t offset;   // posição em bytes do início do bucket em data_hash.dat
    int nBlocos;      // blocos efetivamente ocupados
    int nRegistros;   // registros válidos no bucket
};
#pragma pack(pop)

int funcaoHash(int id) {
    return id % NUM_BUCKETS;
}

int main() {
    vector<Artigo> artigos;

    // =========================
    // 1️⃣ Lê os artigos do data.bin
    // =========================
    ifstream dataIn("data.bin", ios::binary);
    if (!dataIn) { cerr << "Erro ao abrir data.bin\n"; return 1; }

    Artigo a;
    while (dataIn.read(reinterpret_cast<char*>(&a), sizeof(Artigo))) {
        artigos.push_back(a);
    }
    dataIn.close();

    if (artigos.empty()) { 
        cerr << "Nenhum artigo encontrado em data.bin\n"; 
        return 1; 
    }

    // =========================
    // 2️⃣ Inicializa hash
    // =========================
    vector<BucketInfo> hash(NUM_BUCKETS);
    for (int i = 0; i < NUM_BUCKETS; ++i) {
        hash[i].offset = 0;
        hash[i].nBlocos = 0;
        hash[i].nRegistros = 0;
    }

    vector<vector<Artigo>> buckets(NUM_BUCKETS);
    for (const auto& art : artigos) {
        int b = funcaoHash(art.id);
        buckets[b].push_back(art);
    }

    // =========================
    // 3️⃣ Escreve data_hash.dat e atualiza hash
    // =========================
    ofstream dataOut("data_hash.dat", ios::binary);
    if (!dataOut) { cerr << "Erro ao criar data_hash.dat\n"; return 1; }

    int64_t offsetAtual = 0;
    for (int i = 0; i < NUM_BUCKETS; ++i) {
        if (buckets[i].empty()) continue;

        hash[i].offset = offsetAtual;
        hash[i].nRegistros = buckets[i].size();
        hash[i].nBlocos = (buckets[i].size() + ARTIGOS_POR_BLOCO - 1) / ARTIGOS_POR_BLOCO;

        vector<char> bloco(TAM_BLOCO, 0);
        int indiceArtigoNoBloco = 0;

        for (size_t j = 0; j < buckets[i].size(); ++j) {
            memcpy(bloco.data() + indiceArtigoNoBloco * sizeof(Artigo),
                   &buckets[i][j],
                   sizeof(Artigo));
            indiceArtigoNoBloco++;

            if (indiceArtigoNoBloco == ARTIGOS_POR_BLOCO || j == buckets[i].size() - 1) {
                dataOut.write(bloco.data(), TAM_BLOCO);
                offsetAtual += TAM_BLOCO;
                bloco.assign(TAM_BLOCO, 0);
                indiceArtigoNoBloco = 0;
            }
        }
    }
    dataOut.close();

    // =========================
    // 4️⃣ Salva hash.bin
    // =========================
    ofstream hashOut("hash.bin", ios::binary);
    if (!hashOut) { cerr << "Erro ao criar hash.bin\n"; return 1; }

    for (int i = 0; i < NUM_BUCKETS; ++i) {
        hashOut.write(reinterpret_cast<char*>(&hash[i]), sizeof(BucketInfo));
    }
    hashOut.close();

    // =========================
    // 5️⃣ Imprime total de blocos
    // =========================
    int64_t totalBlocos = offsetAtual / TAM_BLOCO;
    cout << "✅ Data hash criado com sucesso!\n";
    cout << "Total blocos usados: " << totalBlocos << "\n";

    return 0;
}

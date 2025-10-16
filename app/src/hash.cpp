#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstring>
#include <cstdio>

using namespace std;

// ================================
// CONFIGURAÇÕES
// ================================
const int TAM_BLOCO = 4096;
const int TAM_BUCKET = 500; // limite de artigos por bloco
const int NUM_BUCKETS = 1009; // número primo para hash

// ================================
// ESTRUTURAS
// ================================
#pragma pack(push, 1)
struct Artigo {
    int id;
    char titulo[301];
    int ano;
    char autores[151];
    int citacoes;
    long long dataAtualizacao; // AAAAMMDD
    char snippet[1025];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BucketInfo {
    long offset;    // posição do primeiro bloco do bucket em data_hash.bin
    int nBlocos;    // blocos ocupados
    int nRegistros; // total de registros no bucket
};
#pragma pack(pop)

// ================================
// FUNÇÃO DE HASH
// ================================
int funcaoHash(int id) {
    return id % NUM_BUCKETS;
}

// ================================
// FUNÇÃO PRINCIPAL
// ================================
int main() {
    ifstream entrada("data.bin", ios::binary);
    if (!entrada) {
        cerr << "Erro ao abrir data.bin\n";
        return 1;
    }

    cout << "✅ Distribuindo registros em arquivos temporários por bucket...\n";

    // 1️⃣ Criar arquivos temporários por bucket
    vector<ofstream> tempBuckets(NUM_BUCKETS);
    for (int i = 0; i < NUM_BUCKETS; i++) {
        string filename = "bucket_" + to_string(i) + ".tmp";
        tempBuckets[i].open(filename, ios::binary);
        if (!tempBuckets[i]) {
            cerr << "Erro ao criar arquivo temporário " << filename << "\n";
            return 1;
        }
    }

    // Ler data.bin e distribuir nos buckets temporários
    Artigo a;
    while (entrada.read(reinterpret_cast<char*>(&a), sizeof(Artigo))) {
        int bucketId = funcaoHash(a.id);
        tempBuckets[bucketId].write(reinterpret_cast<char*>(&a), sizeof(Artigo));
    }
    entrada.close();
    for (int i = 0; i < NUM_BUCKETS; i++) tempBuckets[i].close();

    // 2️⃣ Processar cada bucket temporário e escrever em blocos
    ofstream dataHash("data_hash.bin", ios::binary);
    ofstream hashTable("hash.bin", ios::binary);
    if (!dataHash || !hashTable) {
        cerr << "Erro ao criar arquivos finais.\n";
        return 1;
    }

    long offsetAtual = 0;

    for (int i = 0; i < NUM_BUCKETS; i++) {
        string filename = "bucket_" + to_string(i) + ".tmp";
        ifstream tempIn(filename, ios::binary);
        if (!tempIn) {
            // Bucket vazio
            BucketInfo info = {offsetAtual, 0, 0};
            hashTable.write(reinterpret_cast<char*>(&info), sizeof(BucketInfo));
            continue;
        }

        // Contar quantos registros tem o bucket
        tempIn.seekg(0, ios::end);
        long sizeBytes = tempIn.tellg();
        int totalRegs = sizeBytes / sizeof(Artigo);
        tempIn.seekg(0, ios::beg);

        if (totalRegs == 0) {
            BucketInfo info = {offsetAtual, 0, 0};
            hashTable.write(reinterpret_cast<char*>(&info), sizeof(BucketInfo));
            tempIn.close();
            remove(filename.c_str());
            continue;
        }

        long offsetBucket = offsetAtual;
        int nBlocosBucket = 0;
        int regsProcessados = 0;

        while (regsProcessados < totalRegs) {
            vector<char> bloco(TAM_BLOCO);
            memset(bloco.data(), 0, TAM_BLOCO);

            int pos = 0;
            int regsNoBloco = 0;

            while (regsNoBloco < TAM_BUCKET && regsProcessados < totalRegs
                   && pos + sizeof(Artigo) <= TAM_BLOCO) {
                Artigo art;
                tempIn.read(reinterpret_cast<char*>(&art), sizeof(Artigo));
                memcpy(bloco.data() + pos, &art, sizeof(Artigo));
                pos += sizeof(Artigo);
                regsNoBloco++;
                regsProcessados++;
            }

            dataHash.write(bloco.data(), TAM_BLOCO);
            offsetAtual += TAM_BLOCO;
            nBlocosBucket++;
        }

        BucketInfo info = {offsetBucket, nBlocosBucket, totalRegs};
        hashTable.write(reinterpret_cast<char*>(&info), sizeof(BucketInfo));

        tempIn.close();
        remove(filename.c_str()); // Apaga arquivo temporário
    }

    dataHash.close();
    hashTable.close();

    cout << "✅ Indexação concluída com sucesso.\n";
    cout << "📦 Arquivos gerados: data_hash.bin e hash.bin\n";
    cout << "📘 Cada bloco armazena até " << TAM_BUCKET << " artigos.\n";
    cout << "💾 Tamanho de bloco: " << TAM_BLOCO << " bytes.\n";
    cout << "🔢 Buckets totais: " << NUM_BUCKETS << "\n";

    return 0;
}

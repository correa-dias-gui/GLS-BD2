#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstring>

using namespace std;

// ================================
// CONFIGURAÇÕES (mesmas do index.cpp)
// ================================
const int TAM_BLOCO = 4096;
const int NUM_BUCKETS = 1009; // número primo para hash

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
int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Uso: ./findrec <ID>\n";
        return 1;
    }

    int idBusca = atoi(argv[1]);

    // Abre índice e arquivo de dados
    ifstream hashTable("hash.bin", ios::binary);
    ifstream dataHash("data_hash.bin", ios::binary);

    if (!hashTable || !dataHash) {
        cerr << "Erro ao abrir arquivos de hash ou dados.\n";
        return 1;
    }

    // Total de blocos do arquivo de dados
    dataHash.seekg(0, ios::end);
    long tamanhoArquivo = dataHash.tellg();
    int totalBlocos = ceil(tamanhoArquivo / (double)TAM_BLOCO);

    // Calcula bucket
    int bucketId = funcaoHash(idBusca);

    // Lê o BucketInfo correspondente
    hashTable.seekg(bucketId * sizeof(BucketInfo), ios::beg);
    BucketInfo info;
    hashTable.read(reinterpret_cast<char*>(&info), sizeof(BucketInfo));

    if (info.nBlocos == 0) {
        cout << "Registro não encontrado.\n";
        cout << "Blocos lidos: 0\n";
        cout << "Total de blocos no arquivo: " << totalBlocos << "\n";
        return 0;
    }

    // Lê blocos do bucket
    int blocosLidos = 0;
    bool encontrado = false;
    Artigo a;

    for (int b = 0; b < info.nBlocos; b++) {
        dataHash.seekg(info.offset + b * TAM_BLOCO, ios::beg);

        vector<char> bloco(TAM_BLOCO);
        dataHash.read(bloco.data(), TAM_BLOCO);
        blocosLidos++;

        // Percorre registros no bloco
        int pos = 0;
        while (pos + sizeof(Artigo) <= TAM_BLOCO) {
            memcpy(&a, bloco.data() + pos, sizeof(Artigo));
            pos += sizeof(Artigo);

            // Verifica se registro está válido
            if (a.id == 0) continue; // bloco zerado ou registro inexistente

            if (a.id == idBusca) {
                encontrado = true;
                break;
            }
        }

        if (encontrado) break;
    }

    if (encontrado) {
        cout << "✅ Registro encontrado:\n";
        cout << "ID: " << a.id << "\n";
        cout << "Título: " << a.titulo << "\n";
        cout << "Ano: " << a.ano << "\n";
        cout << "Autores: " << a.autores << "\n";
        cout << "Citações: " << a.citacoes << "\n";
        cout << "Data Atualização: " << a.dataAtualizacao << "\n";
        cout << "Snippet: " << a.snippet << "\n";
    } else {
        cout << "Registro não encontrado.\n";
    }

    cout << "Blocos lidos: " << blocosLidos << "\n";
    cout << "Total de blocos no arquivo: " << totalBlocos << "\n";

    hashTable.close();
    dataHash.close();

    return 0;
}

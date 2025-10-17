#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdint>

using namespace std;

// ================================
// CONFIGURAÇÕES
// ================================
const int TAM_BLOCO = 4096;
const int ARTIGOS_POR_BLOCO = 2;
const int NUM_BUCKETS = 1009;

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
        cerr << "Uso: findrec <ID>\n";
        return 1;
    }

    int idBuscado = stoi(argv[1]);

    // Abre hash.bin
    ifstream hashFile("hash.bin", ios::binary);
    if (!hashFile) {
        cerr << "Erro: não foi possível abrir hash.bin\n";
        return 1;
    }

    // Abre data_hash.dat
    ifstream dataFile("data_hash.dat", ios::binary);
    if (!dataFile) {
        cerr << "Erro: não foi possível abrir data_hash.dat\n";
        return 1;
    }

    // Calcula bucket correspondente
    int bucket = funcaoHash(idBuscado);

    // Lê as informações do bucket correspondente
    hashFile.seekg(bucket * sizeof(BucketInfo), ios::beg);
    BucketInfo info;
    hashFile.read(reinterpret_cast<char*>(&info), sizeof(BucketInfo));

    if (info.nRegistros == 0) {
        cout << "Registro não encontrado (bucket vazio).\n";
        return 0;
    }

    // Calcula o total de blocos do arquivo inteiro
    dataFile.seekg(0, ios::end);
    int64_t totalBytes = dataFile.tellg();
    int64_t totalBlocosArquivo = totalBytes / TAM_BLOCO;

    // Começa a busca no bucket
    dataFile.seekg(info.offset, ios::beg);

    int blocosLidos = 0;
    bool encontrado = false;
    Artigo a;

    for (int b = 0; b < info.nBlocos; ++b) {
        blocosLidos++;
        vector<char> bloco(TAM_BLOCO);
        dataFile.read(bloco.data(), TAM_BLOCO);

        // Dentro do bloco, há até 2 artigos
        for (int i = 0; i < ARTIGOS_POR_BLOCO; ++i) {
            int offsetArtigo = i * sizeof(Artigo);
            if (offsetArtigo + sizeof(Artigo) > TAM_BLOCO)
                break;

            memcpy(&a, bloco.data() + offsetArtigo, sizeof(Artigo));

            if (a.id == idBuscado) {
                encontrado = true;
                break;
            }
        }

        if (encontrado)
            break;
    }

    if (encontrado) {
        cout << "✅ Registro encontrado!\n\n";
        cout << "ID: " << a.id << "\n";
        cout << "Título: " << a.titulo << "\n";
        cout << "Ano: " << a.ano << "\n";
        cout << "Autores: " << a.autores << "\n";
        cout << "Citações: " << a.citacoes << "\n";
        cout << "Data Atualização: " << a.dataAtualizacao << "\n";
        cout << "Snippet: " << a.snippet << "\n\n";
        cout << "📦 Blocos lidos até encontrar: " << blocosLidos << "\n";
        cout << "💾 Total de blocos no arquivo de dados: " << totalBlocosArquivo << "\n";
    } else {
        cout << "❌ Registro com ID " << idBuscado << " não encontrado.\n";
        cout << "📦 Blocos lidos: " << blocosLidos << "\n";
        cout << "💾 Total de blocos do arquivo: " << totalBlocosArquivo << "\n";
    }

    return 0;
}

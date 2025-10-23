#include <iostream>
#include <fstream>
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
    int64_t prox;
    int nRegs;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BucketInfo {
    int64_t offset;
    int nBlocos;
    int nRegistros;
};
#pragma pack(pop)

int funcaoHash(int id) {
    return id % NUM_BUCKETS;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Uso: findrec <ID>\n";
        return 1;
    }

    int idBuscado = stoi(argv[1]);

    ifstream hashFile("hash.bin", ios::binary);
    if (!hashFile) { cerr << "Erro ao abrir hash.bin\n"; return 1; }

    ifstream dataFile("data_hash.dat", ios::binary);
    if (!dataFile) { cerr << "Erro ao abrir data_hash.dat\n"; return 1; }

    // Calcula o total de blocos existentes no arquivo de dados
    dataFile.seekg(0, ios::end);
    int64_t totalBytes = dataFile.tellg();
    int64_t totalBlocosArquivo = totalBytes / TAM_BLOCO;
    dataFile.seekg(0, ios::beg);

    // Determina o bucket do registro
    int bucket = funcaoHash(idBuscado);

    // Lê as informações do bucket correspondente
    hashFile.seekg(bucket * sizeof(BucketInfo), ios::beg);
    BucketInfo info;
    hashFile.read(reinterpret_cast<char*>(&info), sizeof(BucketInfo));

    if (info.nRegistros == 0) {
        cout << "Registro não encontrado (bucket vazio).\n";
        cout << "💾 Total de blocos no arquivo de dados: " << totalBlocosArquivo << "\n";
        return 0;
    }

    // Busca sequencial no bucket
    int64_t offsetAtual = info.offset;
    int blocosLidos = 0;
    bool encontrado = false;
    Artigo a;

    while (offsetAtual != -1) {
        dataFile.seekg(offsetAtual, ios::beg);

        // Lê o cabeçalho do bloco
        BlocoHeader header;
        dataFile.read(reinterpret_cast<char*>(&header), sizeof(header));
        blocosLidos++;

        // Lê os registros do bloco
        for (int i = 0; i < header.nRegs; ++i) {
            dataFile.read(reinterpret_cast<char*>(&a), sizeof(Artigo));
            if (a.id == idBuscado) {
                encontrado = true;
                break;
            }
        }

        if (encontrado) break;

        offsetAtual = header.prox;
    }

    // Saída formatada
    cout << "💾 Total de blocos no arquivo de dados: " << totalBlocosArquivo << "\n";

    if (encontrado) {
        cout << "✅ Registro encontrado!\n";
        cout << "📦 Blocos lidos até encontrar: " << blocosLidos << "\n\n";
        cout << "ID: " << a.id << "\n";
        cout << "Título: " << a.titulo << "\n";
        cout << "Ano: " << a.ano << "\n";
        cout << "Autores: " << a.autores << "\n";
        cout << "Citações: " << a.citacoes << "\n";
        cout << "Data Atualização: " << a.dataAtualizacao << "\n";
        cout << "Snippet: " << a.snippet << "\n";
    } else {
        cout << "❌ Registro com ID " << idBuscado << " não encontrado.\n";
        cout << "📦 Blocos lidos: " << blocosLidos << "\n";
    }

    return 0;
}

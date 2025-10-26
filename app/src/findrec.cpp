#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <chrono>
using namespace std;

const int TAM_BLOCO = 4096;
const int ARTIGOS_POR_BLOCO = 2;
const int NUM_BUCKETS = 2000;

enum LogLevel { ERROR = 0, WARN = 1, INFO = 2, DEBUG = 3 };
const LogLevel currentLevel = DEBUG;

void logMsg(LogLevel level, const string &msg) {
    if (level <= currentLevel) {
        switch (level) {
            case ERROR: cerr << "[ERROR] " << msg << endl; break;
            case WARN:  cout << "[WARN]  " << msg << endl; break;
            case INFO:  cout << "[INFO]  " << msg << endl; break;
            case DEBUG: cout << "[DEBUG] " << msg << endl; break;
        }
    }
}

//Struct de artigo
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
        logMsg(ERROR, "Uso: findrec <ID>");
        return 1;
    }

    int idBuscado = stoi(argv[1]);
    string hashPath = "../data/hash.bin";
    string dataPath = "../data/data_hash.dat";

    logMsg(INFO, "================ INÍCIO DA BUSCA ================");
    logMsg(INFO, "ID buscado: " + to_string(idBuscado));
    logMsg(INFO, "Arquivo de índice: " + hashPath);
    logMsg(INFO, "Arquivo de dados:  " + dataPath);

    auto inicio = chrono::high_resolution_clock::now();

    //Abre os arquivos
    ifstream hashFile(hashPath, ios::binary);
    if (!hashFile) {
        logMsg(ERROR, "Erro ao abrir " + hashPath);
        return 1;
    }
    ifstream dataFile(dataPath, ios::binary);
    if (!dataFile) {
        logMsg(ERROR, "Erro ao abrir " + dataPath);
        return 1;
    }

    //Calcula o total de blocos
    dataFile.seekg(0, ios::end);
    int64_t totalBytes = dataFile.tellg();
    int64_t totalBlocosArquivo = totalBytes / TAM_BLOCO;
    dataFile.seekg(0, ios::beg);
    logMsg(DEBUG, "Total de bytes no arquivo de dados: " + to_string(totalBytes));
    logMsg(DEBUG, "Total de blocos no arquivo: " + to_string(totalBlocosArquivo));

    //Acha o bucket do id e le suas informacoes
    int bucket = funcaoHash(idBuscado);
    logMsg(INFO, "Bucket calculado: " + to_string(bucket));

    hashFile.seekg(bucket * sizeof(BucketInfo), ios::beg);
    BucketInfo info;
    hashFile.read(reinterpret_cast<char*>(&info), sizeof(BucketInfo));
    logMsg(DEBUG, "Bucket offset = " + to_string(info.offset));
    logMsg(DEBUG, "Bucket nBlocos = " + to_string(info.nBlocos));
    logMsg(DEBUG, "Bucket nRegistros = " + to_string(info.nRegistros));

    if (info.nRegistros == 0) {
        logMsg(WARN, "Bucket vazio. Nenhum registro armazenado.");
        cout << "💾 Total de blocos no arquivo de dados: " << totalBlocosArquivo << "\n";
        return 0;
    }

    //Busca sequencial dentro dos blocos (a sequencial foi escolhida apesar dos
    //valores ordenados de id para que o codigo nao fique dependente de ordenacao
    //dos valores passados)
    int64_t offsetAtual = info.offset;
    int blocosLidos = 0;
    bool encontrado = false;
    Artigo a;

    while (offsetAtual != -1) {
        dataFile.seekg(offsetAtual, ios::beg);

        BlocoHeader header;
        dataFile.read(reinterpret_cast<char*>(&header), sizeof(header));
        blocosLidos++;

        //Imprime debug de 50 em 50 blocos
        if (blocosLidos % 50 == 0) {
            logMsg(DEBUG, "Percorridos " + to_string(blocosLidos) + " blocos no bucket " + to_string(bucket));
        }

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

    auto fim = chrono::high_resolution_clock::now();
    auto duracao = chrono::duration_cast<chrono::milliseconds>(fim - inicio).count();

    logMsg(INFO, "================ RESULTADO FINAL ================");
    logMsg(INFO, "Tempo total de execução: " + to_string(duracao) + " ms");
    logMsg(INFO, "Total de blocos lidos: " + to_string(blocosLidos));
    logMsg(INFO, "Total de blocos no arquivo: " + to_string(totalBlocosArquivo));
    logMsg(INFO, "=================================================");

    if (encontrado) {
        cout << "\nRegistro encontrado!\n\n";
        cout << "ID: " << a.id << "\n";
        cout << "Título: " << a.titulo << "\n";
        cout << "Ano: " << a.ano << "\n";
        cout << "Autores: " << a.autores << "\n";
        cout << "Citações: " << a.citacoes << "\n";
        cout << "Data Atualização: " << a.dataAtualizacao << "\n";
        cout << "Snippet: " << a.snippet << "\n\n";
    } else {
        cout << "\nRegistro com ID " << idBuscado << " não encontrado.\n\n";
    }
    return 0;
}

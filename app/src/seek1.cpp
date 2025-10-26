#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <cstdlib>
#include "../include/BTplus_mem.h"

using namespace std;

// ================================
// Log
// ================================
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

// ================================
// Estrutura de Artigo (mesma do upload/findrec)
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

// ================================
// Programa principal
// ================================
int main(int argc, char* argv[]) {
    if (argc != 2) {
        logMsg(ERROR, "Uso: seek1 <ID>");
        return 1;
    }

    int idBuscado = stoi(argv[1]);

    // ✅ CAMINHOS CORRIGIDOS - baseado na estrutura real dos arquivos
    string indiceFile = "data/indice_id.bin";    // índice principal na raiz (15M)
    string dataFile = "data/data_hash.dat";      // dados na raiz (2.0G)

    logMsg(INFO, "================ BUSCA COM ÍNDICE PRIMÁRIO ================");
    logMsg(INFO, "ID buscado: " + to_string(idBuscado));
    logMsg(INFO, "Arquivo de índice: " + indiceFile);
    logMsg(INFO, "Arquivo de dados:  " + dataFile);

    auto inicio = chrono::high_resolution_clock::now();

    // ================================
    // Callbacks para B+ Tree de ID
    // ================================
    auto cmp_id = [](const void* a, const void* b) {
        const unsigned char* pa = (const unsigned char*)a;
        const unsigned char* pb = (const unsigned char*)b;
        int r = memcmp(pa, pb, 4);
        return r < 0 ? -1 : (r > 0 ? 1 : 0);
    };
    
    auto serialize_id = [](const void* chave) {
        int v = *reinterpret_cast<const int*>(chave);
        unsigned char buf[4];
        buf[0] = (v >> 24) & 0xFF;
        buf[1] = (v >> 16) & 0xFF;
        buf[2] = (v >> 8) & 0xFF;
        buf[3] = v & 0xFF;
        return string((char*)buf, 4);
    };
    
    auto deserialize_id = [](const string& s) -> void* {
        if (s.size() != 4) return nullptr;
        const unsigned char* p = (const unsigned char*)s.data();
        int* v = new int((int(p[0]) << 24) | (int(p[1]) << 16) | (int(p[2]) << 8) | int(p[3]));
        return v;
    };
    
    auto print_id = [](const char* raw, int tam) {
        if (tam != 4) return string("<inv>");
        const unsigned char* p = (const unsigned char*)raw;
        int v = (int(p[0]) << 24) | (int(p[1]) << 16) | (int(p[2]) << 8) | int(p[3]);
        return to_string(v);
    };

    try {
        // Verifica se o arquivo de índice existe
        ifstream testIndex(indiceFile);
        if (!testIndex.is_open()) {
            logMsg(ERROR, "Arquivo de índice não encontrado em: " + indiceFile);
            cerr << "Erro: índice primário não encontrado. Execute o programa upload primeiro.\n";
            return 1;
        }
        testIndex.close();

        // Instancia a árvore B+ para o índice primário
        ArvoreBMais indice_id(indiceFile, 4, cmp_id, serialize_id, deserialize_id, print_id);

        logMsg(DEBUG, "Árvore B+ carregada com sucesso");

        // Busca o ID no índice B+
        long offsetRegistro = indice_id.buscar(&idBuscado);

        // Simula contador de blocos lidos (ajustar futuramente conforme B+ Tree real)
        int blocosLidosIndice = 1;

        if (offsetRegistro == -1) {
            logMsg(WARN, "ID não encontrado no índice");
            cout << "\nRegistro com ID " << idBuscado << " não encontrado.\n\n";

            // Calcula total de blocos do arquivo de dados
            ifstream dataStream(dataFile, ios::binary);
            if (dataStream) {
                dataStream.seekg(0, ios::end);
                int64_t totalBytes = dataStream.tellg();
                int64_t totalBlocosArquivo = totalBytes / 4096; // TAM_BLOCO = 4096
                cout << "Total de blocos no arquivo de dados: " << totalBlocosArquivo << "\n";
                cout << "Blocos de índice lidos: " << blocosLidosIndice << "\n";
                dataStream.close();
            }
            return 0;
        }

        logMsg(INFO, "ID encontrado no índice! Offset: " + to_string(offsetRegistro));

        // Abre o arquivo de dados para ler o registro
        ifstream dataStream(dataFile, ios::binary);
        if (!dataStream) {
            logMsg(ERROR, "Erro ao abrir arquivo de dados: " + dataFile);
            return 1;
        }

        // Calcula total de blocos do arquivo
        dataStream.seekg(0, ios::end);
        int64_t totalBytes = dataStream.tellg();
        int64_t totalBlocosArquivo = totalBytes / 4096; // TAM_BLOCO = 4096

        // Vai para a posição específica e lê o registro
        dataStream.seekg(offsetRegistro, ios::beg);
        Artigo a;
        dataStream.read(reinterpret_cast<char*>(&a), sizeof(Artigo));

        if (!dataStream || dataStream.gcount() != sizeof(Artigo)) {
            logMsg(ERROR, "Erro ao ler registro na posição " + to_string(offsetRegistro));
            return 1;
        }

        // Verifica se o ID confere
        if (a.id != idBuscado) {
            logMsg(WARN, "ID inconsistente! Esperado: " + to_string(idBuscado) + 
                         ", Encontrado: " + to_string(a.id));
        }

        dataStream.close();

        auto fim = chrono::high_resolution_clock::now();
        auto duracao = chrono::duration_cast<chrono::milliseconds>(fim - inicio).count();

        logMsg(INFO, "================ RESULTADO FINAL ================");
        logMsg(INFO, "Tempo total de execução: " + to_string(duracao) + " ms");
        logMsg(INFO, "=================================================");

        // Exibe o registro encontrado
        cout << "\nRegistro encontrado!\n\n";
        cout << "ID: " << a.id << "\n";
        cout << "Título: " << a.titulo << "\n";
        cout << "Ano: " << a.ano << "\n";
        cout << "Autores: " << a.autores << "\n";
        cout << "Citações: " << a.citacoes << "\n";
        cout << "Data Atualização: " << a.dataAtualizacao << "\n";
        cout << "Snippet: " << a.snippet << "\n\n";

        // Informações sobre performance
        cout << "Total de blocos no arquivo de dados: " << totalBlocosArquivo << "\n";
        cout << "Blocos de índice lidos: " << blocosLidosIndice << "\n";
        cout << "Tempo de execução: " << duracao << " ms\n";

        return 0;

    } catch (const exception& e) {
        logMsg(ERROR, "Exceção capturada: " + string(e.what()));
        return 1;
    } catch (...) {
        logMsg(ERROR, "Erro desconhecido");
        return 1;
    }
}
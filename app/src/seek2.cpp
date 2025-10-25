#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <vector>
#include "../include/BTplus_mem.h"

using namespace std;

// -------------------- Log --------------------
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

// -------------------- Estrutura do Artigo --------------------
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

// -------------------- Função para buscar todos os offsets com o mesmo título --------------------
vector<long> buscarTodosOffsets(ArvoreBMais& indice, const string& tituloBuscado) {
    vector<long> offsets;
    
    try {
        // Cria buffer de tamanho fixo (300 bytes para títulos)
        char tituloBuffer[301];
        memset(tituloBuffer, 0, 301);
        strncpy(tituloBuffer, tituloBuscado.c_str(), 300);
        
        // Busca o nó que contém o título
        No no = indice.buscarNo(tituloBuffer);
        
        if (no.qtd_chaves == 0) {
            logMsg(DEBUG, "Nó vazio retornado - título não encontrado");
            return offsets;
        }
        
        logMsg(DEBUG, "Nó encontrado com " + to_string(no.qtd_chaves) + " chaves, folha: " + to_string(no.folha));
        
        // Se não é folha, navegar até encontrar a folha correta
        long atual_offset = -1;
        if (!no.folha) {
            logMsg(DEBUG, "Buscando folha que contém o título...");
            atual_offset = indice.encontrarFolha(indice.getMetadata().raiz_offset, tituloBuffer);
            if (atual_offset != -1) {
                no = indice.carregarNo(atual_offset);
                logMsg(DEBUG, "Folha encontrada com " + to_string(no.qtd_chaves) + " chaves");
            }
        }
        
        // Agora navega pela folha atual e folhas subsequentes (lista encadeada)
        while (no.qtd_chaves > 0) {
            bool encontrou_neste_no = false;
            
            // Procura todas as ocorrências neste nó
            for (int i = 0; i < no.qtd_chaves; i++) {
                string chaveAtual = string(no.getChave(i), 300);
                
                // Remove caracteres nulos da comparação
                size_t pos = chaveAtual.find('\0');
                if (pos != string::npos) {
                    chaveAtual = chaveAtual.substr(0, pos);
                }
                
                logMsg(DEBUG, "Comparando: '" + chaveAtual + "' com '" + tituloBuscado + "'");
                
                if (chaveAtual == tituloBuscado) {
                    long offset = *no.getOffset(i);
                    offsets.push_back(offset);
                    encontrou_neste_no = true;
                    logMsg(DEBUG, "Match encontrado! Offset: " + to_string(offset));
                } else if (encontrou_neste_no && chaveAtual > tituloBuscado) {
                    // Se já encontramos matches e agora a chave é maior, paramos
                    // (assume que as chaves estão ordenadas)
                    logMsg(DEBUG, "Chave maior encontrada, parando busca neste nó");
                    break;
                }
            }
            
            // Se não encontrou nada neste nó e não encontrou antes, provavelmente não existe
            if (!encontrou_neste_no && offsets.empty()) {
                logMsg(DEBUG, "Nenhum match neste nó e nenhum encontrado antes - parando");
                break;
            }
            
            // Segue para o próximo nó folha (se existir)
            if (no.proximo != -1) {
                logMsg(DEBUG, "Seguindo para próximo nó folha: " + to_string(no.proximo));
                no = indice.carregarNo(no.proximo);
            } else {
                logMsg(DEBUG, "Não há próximo nó folha - finalizando busca");
                break;
            }
        }
        
    } catch (const exception& e) {
        logMsg(ERROR, "Erro ao buscar nós: " + string(e.what()));
    }
    
    return offsets;
}

// -------------------- Programa Principal --------------------
int main(int argc, char* argv[]) {
    if (argc != 2) {
        logMsg(ERROR, "Uso: seek2 <TITULO_EXATO>");
        return 1;
    }

    string tituloBuscado = argv[1];
    
    // ✅ CAMINHOS CORRIGIDOS - baseado na estrutura real dos arquivos
    string indiceFile = "indice_titulo.bin";  // índice secundário na raiz
    string dataFile = "data_hash.dat";        // dados na raiz

    logMsg(INFO, "================ BUSCA COM ÍNDICE SECUNDÁRIO ================");
    logMsg(INFO, "Título buscado: " + tituloBuscado);
    logMsg(INFO, "Arquivo de índice: " + indiceFile);
    logMsg(INFO, "Arquivo de dados:  " + dataFile);

    auto inicio = chrono::high_resolution_clock::now();

    // ---------- Callbacks para string ----------
    auto cmp_str = [](const void* a, const void* b) -> int {
        return strcmp((const char*)a, (const char*)b);
    };

    auto serialize_str = [](const void* chave) -> string {
        const char* s = (const char*)chave;
        string result(s, 300); // títulos têm 300 bytes fixos
        return result;
    };

    auto deserialize_str = [](const string& s) -> void* {
        char* buf = new char[301];
        memset(buf, 0, 301);
        memcpy(buf, s.c_str(), min(s.size(), (size_t)300));
        return buf;
    };

    auto print_str = [](const char* raw, int tam) -> string {
        string result(raw, min(tam, 300));
        size_t pos = result.find('\0');
        if (pos != string::npos) {
            result = result.substr(0, pos);
        }
        return result;
    };

    try {
        // Verifica se o arquivo de índice existe
        ifstream testIndex(indiceFile);
        if (!testIndex.is_open()) {
            logMsg(ERROR, "Arquivo de índice não encontrado em: " + indiceFile);
            cerr << "Erro: índice secundário não encontrado. Execute o programa upload primeiro.\n";
            return 1;
        }
        testIndex.close();

        // ---------- Carrega o índice secundário ----------
        ArvoreBMais indice_titulo(indiceFile, 300, cmp_str, serialize_str, deserialize_str, print_str);
        logMsg(DEBUG, "Árvore B+ carregada com sucesso (índice secundário)");

        // ---------- Busca TODOS os offsets com o título ----------
        vector<long> offsets = buscarTodosOffsets(indice_titulo, tituloBuscado);

        if (offsets.empty()) {
            logMsg(WARN, "Título não encontrado no índice");
            cout << "\nRegistro com título \"" << tituloBuscado << "\" não encontrado.\n\n";

            ifstream dataStream(dataFile, ios::binary);
            if (dataStream) {
                dataStream.seekg(0, ios::end);
                int64_t totalBytes = dataStream.tellg();
                int64_t totalBlocosArquivo = totalBytes / 4096;
                cout << "Total de blocos no arquivo de dados: " << totalBlocosArquivo << "\n";
                cout << "Blocos de índice lidos: [valor da árvore B+]\n";
                dataStream.close();
            }
            return 0;
        }

        logMsg(INFO, "Encontrados " + to_string(offsets.size()) + " artigos com este título!");

        // ---------- Lê TODOS os registros encontrados ----------
        ifstream dataStream(dataFile, ios::binary);
        if (!dataStream) {
            logMsg(ERROR, "Erro ao abrir arquivo de dados: " + dataFile);
            return 1;
        }

        dataStream.seekg(0, ios::end);
        int64_t totalBytes = dataStream.tellg();
        int64_t totalBlocosArquivo = totalBytes / 4096;

        cout << "\n========== ARTIGOS ENCONTRADOS ==========\n";
        
        for (size_t i = 0; i < offsets.size(); i++) {
            dataStream.seekg(offsets[i], ios::beg);
            Artigo a;
            dataStream.read(reinterpret_cast<char*>(&a), sizeof(Artigo));

            if (!dataStream || dataStream.gcount() != sizeof(Artigo)) {
                logMsg(ERROR, "Erro ao ler registro na posição " + to_string(offsets[i]));
                continue;
            }

            cout << "\n--- Artigo " << (i+1) << " ---\n";
            cout << "ID: " << a.id << "\n";
            cout << "Título: " << a.titulo << "\n";
            cout << "Ano: " << a.ano << "\n";
            cout << "Autores: " << a.autores << "\n";
            cout << "Citações: " << a.citacoes << "\n";
            cout << "Data Atualização: " << a.dataAtualizacao << "\n";
            cout << "Snippet: " << a.snippet << "\n";
        }

        dataStream.close();

        auto fim = chrono::high_resolution_clock::now();
        auto duracao = chrono::duration_cast<chrono::milliseconds>(fim - inicio).count();

        // ---------- Estatísticas finais ----------
        cout << "\n========== ESTATÍSTICAS ==========\n";
        cout << "Total de artigos encontrados: " << offsets.size() << "\n";
        cout << "Total de blocos no arquivo de dados: " << totalBlocosArquivo << "\n";
        cout << "Blocos de índice lidos: [valor da árvore B+]\n";
        cout << "Tempo total: " << duracao << " ms\n";

        logMsg(INFO, "================ RESULTADO FINAL ================");
        logMsg(INFO, "Tempo total de execução: " + to_string(duracao) + " ms");
        logMsg(INFO, "Artigos encontrados: " + to_string(offsets.size()));
        logMsg(INFO, "=================================================");

    } catch (const exception& e) {
        logMsg(ERROR, "Exceção capturada: " + string(e.what()));
        return 1;
    } catch (...) {
        logMsg(ERROR, "Erro desconhecido");
        return 1;
    }

    return 0;
}
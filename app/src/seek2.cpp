#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <vector>
#include <algorithm>
#include "../include/BTplus_mem.h"

using namespace std;

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
        char tituloBuffer[301];
        memset(tituloBuffer, 0, 301);
        strncpy(tituloBuffer, tituloBuscado.c_str(), 300);

        No no = indice.buscarNo(tituloBuffer);

        if (no.qtd_chaves == 0)
            return offsets;

        for (int i = 0; i < no.qtd_chaves; i++) {
            string chaveAtual = string(no.getChave(i), 300);
            size_t pos = chaveAtual.find('\0');
            if (pos != string::npos)
                chaveAtual = chaveAtual.substr(0, pos);

            if (chaveAtual == tituloBuscado) {
                long offset = *no.getOffset(i);
                offsets.push_back(offset);
            }
        }

    } catch (const exception& e) {
        cerr << "[ERRO] Falha ao buscar offsets: " << e.what() << endl;
    }

    return offsets;
}

// -------------------- Programa Principal --------------------
int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Uso: seek2 <TITULO_EXATO>" << endl;
        return 1;
    }

    string tituloBuscado = argv[1];
    string indiceFile = "indice_titulo.bin";
    string dataFile = "data_hash.dat";

    cout << "===== BUSCA COM ÍNDICE SECUNDÁRIO =====" << endl;
    cout << "Título buscado: " << tituloBuscado << endl;

    auto inicio = chrono::high_resolution_clock::now();

    // ---------- Callbacks para string ----------
    auto cmp_str = [](const void* a, const void* b) -> int {
        return strcmp((const char*)a, (const char*)b);
    };

    auto serialize_str = [](const void* chave) -> string {
        const char* s = (const char*)chave;
        return string(s, 300);
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
        if (pos != string::npos)
            result = result.substr(0, pos);
        return result;
    };

    try {
        ifstream testIndex(indiceFile);
        if (!testIndex.is_open()) {
            cerr << "[ERRO] Arquivo de índice não encontrado em: " << indiceFile << endl;
            cerr << "Execute o programa upload antes de rodar o seek2." << endl;
            return 1;
        }
        testIndex.close();

        ArvoreBMais indice_titulo(indiceFile, 300, cmp_str, serialize_str, deserialize_str, print_str);

        vector<long> offsets = buscarTodosOffsets(indice_titulo, tituloBuscado);

        // Remove duplicatas de offsets
        sort(offsets.begin(), offsets.end());
        offsets.erase(unique(offsets.begin(), offsets.end()), offsets.end());

        if (offsets.empty()) {
            cout << "\nNenhum artigo encontrado com o título especificado." << endl;
            return 0;
        }

        cout << "\nForam encontrados " << offsets.size() << " artigo(s) com esse título." << endl;

        ifstream dataStream(dataFile, ios::binary);
        if (!dataStream) {
            cerr << "[ERRO] Falha ao abrir o arquivo de dados: " << dataFile << endl;
            return 1;
        }

        dataStream.seekg(0, ios::end);
        int64_t totalBytes = dataStream.tellg();
        int64_t totalBlocosArquivo = totalBytes / 4096;
        dataStream.seekg(0, ios::beg);

        cout << "\n===== ARTIGOS ENCONTRADOS =====" << endl;

        for (size_t i = 0; i < offsets.size(); i++) {
            dataStream.seekg(offsets[i], ios::beg);
            Artigo a;
            dataStream.read(reinterpret_cast<char*>(&a), sizeof(Artigo));

            if (!dataStream || dataStream.gcount() != sizeof(Artigo)) {
                cerr << "[ERRO] Falha ao ler o registro no offset " << offsets[i] << endl;
                continue;
            }

            cout << "\n--- Artigo " << (i + 1) << " ---" << endl;
            cout << "ID: " << a.id << endl;
            cout << "Título: " << a.titulo << endl;
            cout << "Ano: " << a.ano << endl;
            cout << "Autores: " << a.autores << endl;
            cout << "Citações: " << a.citacoes << endl;
            cout << "Data Atualização: " << a.dataAtualizacao << endl;
            cout << "Snippet: " << a.snippet << endl;
        }

        dataStream.close();

        auto fim = chrono::high_resolution_clock::now();
        auto duracao = chrono::duration_cast<chrono::milliseconds>(fim - inicio).count();

        cout << "\n===== ESTATÍSTICAS =====" << endl;
        cout << "Total de artigos encontrados: " << offsets.size() << endl;
        cout << "Total de blocos no arquivo de dados: " << totalBlocosArquivo << endl;
        cout << "Tempo total de execução: " << duracao << " ms" << endl;

    } catch (const exception& e) {
        cerr << "[ERRO] Exceção capturada: " << e.what() << endl;
        return 1;
    } catch (...) {
        cerr << "[ERRO] Erro desconhecido." << endl;
        return 1;
    }

    return 0;
}

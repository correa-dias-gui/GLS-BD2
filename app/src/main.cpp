#include <iostream>
#include <fstream>
#include <cstdlib> // para getenv
#include <string>
#include "../include/BTplus_mem.h"
using namespace std;

static int cmpStr(const void* a, const void* b) { return strcmp((const char*)a, (const char*)b); }
static void* deserStr(const string& s){ return (void*)s.c_str(); }
static string serStr(const void* p){ return string((const char*)p); }

int main() {
    const char* logLevel = std::getenv("LOG_LEVEL");
    const char* dataDir = std::getenv("DATA_DIR");

    std::string logPath = std::string(dataDir ? dataDir : ".") + "/log.txt";
    std::ofstream logFile(logPath, std::ios::app);

    if (logFile.is_open()) {
        logFile << "[INFO] Programa iniciado com LOG_LEVEL=" 
                << (logLevel ? logLevel : "NULO") << std::endl;
        logFile.close();
    }

    // Teste simples da árvore
    ArvoreBMais arv("indice.bin", 50, cmpStr, serStr, deserStr);
    const char* chave = "teste";
    arv.inserir(chave, 12345);
    long pos = arv.buscar(chave);
    cout << "Busca chave '" << chave << "' retornou offset de dado: " << pos << endl;

    std::cout << "Log gravado em " << logPath << std::endl;
    return 0;
}

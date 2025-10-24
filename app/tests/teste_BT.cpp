#include "../include/BTplus_mem.h"
#include <cstring>
#include <string>
#include <iostream>

using namespace std;

static int g_tam_chave = 64;
int cmp_str(const void* a, const void* b) { return strncmp((const char*)a, (const char*)b, g_tam_chave); }
string serialize_str(const void* chave) { return string((const char*)chave); }
void* deserialize_str(const string& s) {
    char* buf = new char[g_tam_chave];
    memset(buf, 0, g_tam_chave);
    strncpy(buf, s.c_str(), g_tam_chave - 1);
    return buf;
}
string print_str(const char* buf, int tam) { return string(buf, strnlen(buf, tam)); }

int main() {
    int tam_chave = g_tam_chave; // adequado para 'ArtigoX'
    // Remove arquivo antigo para evitar formato legado
    remove("data/indice.bin");
    ArvoreBMais arv("data/indice.bin", tam_chave, cmp_str, serialize_str, deserialize_str, print_str);

    const char* artigos[] = {"ArtigoA","ArtigoB","ArtigoC","ArtigoD","ArtigoE","ArtigoF","ArtigoG","ArtigoH","ArtigoE","ArtigoI","ArtigoJ","ArtigoK","ArtigoL","ArtigoM","ArtigoN","ArtigoO","ArtigoP","ArtigoQ","ArtigoR","ArtigoS","ArtigoT","ArtigoU","ArtigoV","ArtigoW","ArtigoX","ArtigoY","ArtigoZ","ArtigoA","ArtigoC","ArtigoZ"};
    long offs[] = {100,200,300,400,500,600,700,800,900,1000,1100,1200,1300,1400,1500,1600,1700,1800,1900,2000,2100,2200,2300,2400,2500,2600,2700,2800,2900,3000};
    for (int i=0;i<30;i++) arv.inserir(artigos[i], offs[i]);

    cout << "Inseridos 30 artigos na árvore B+" << endl;
    const char* chaveBusca = "ArtigoE"; long pos = arv.buscar(chaveBusca);
    if (pos != -1) cout << chaveBusca << " encontrado com offset de dado: " << pos << endl; else cout << chaveBusca << " não encontrado." << endl;
    arv.exibir();
    return 0;
}

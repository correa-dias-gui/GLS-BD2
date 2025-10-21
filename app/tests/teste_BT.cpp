#include "../include/BTplus_mem.h"
#include <cstring>

// Função de comparação para strings
int cmp_str(const void* a, const void* b) {
    return strncmp((const char*)a, (const char*)b, TAM_CHAVE);
}

// Serializa: recebe ponteiro para string, retorna string
string serialize_str(const void* chave) {
    return string((const char*)chave);
}

// Desserializa: recebe string, retorna ponteiro para buffer
void* deserialize_str(const string& s) {
    char* buf = new char[TAM_CHAVE];
    memset(buf, 0, TAM_CHAVE);
    strncpy(buf, s.c_str(), TAM_CHAVE);
    return buf;
}

int main() {
    ArvoreBMais arv("data/indice.bin", cmp_str, serialize_str, deserialize_str);
    arv.inserir("ArtigoA", 100);
    arv.inserir("ArtigoB", 200);
    arv.inserir("ArtigoC", 300);
    arv.inserir("ArtigoD", 400);
    arv.inserir("ArtigoE", 500);
    arv.inserir("ArtigoE", 900);
    arv.exibir();
    return 0;
}

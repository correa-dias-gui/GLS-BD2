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
    
    // Inserindo 30 exemplos variados
    arv.inserir("ArtigoA", 100);
    arv.inserir("ArtigoB", 200);
    arv.inserir("ArtigoC", 300);
    arv.inserir("ArtigoD", 400);
    arv.inserir("ArtigoE", 500);
    arv.inserir("ArtigoF", 600);
    arv.inserir("ArtigoG", 700);
    arv.inserir("ArtigoH", 800);
    arv.inserir("ArtigoE", 900);  // Duplicata
    arv.inserir("ArtigoI", 1000);
    arv.inserir("ArtigoJ", 1100);
    arv.inserir("ArtigoK", 1200);
    arv.inserir("ArtigoL", 1300);
    arv.inserir("ArtigoM", 1400);
    arv.inserir("ArtigoN", 1500);
    arv.inserir("ArtigoO", 1600);
    arv.inserir("ArtigoP", 1700);
    arv.inserir("ArtigoQ", 1800);
    arv.inserir("ArtigoR", 1900);
    arv.inserir("ArtigoS", 2000);
    arv.inserir("ArtigoT", 2100);
    arv.inserir("ArtigoU", 2200);
    arv.inserir("ArtigoV", 2300);
    arv.inserir("ArtigoW", 2400);
    arv.inserir("ArtigoX", 2500);
    arv.inserir("ArtigoY", 2600);
    arv.inserir("ArtigoZ", 2700);
    arv.inserir("ArtigoA", 2800);  // Duplicata
    arv.inserir("ArtigoC", 2900);  // Duplicata
    arv.inserir("ArtigoZ", 3000);  // Duplicata
    
    cout << "Inseridos 30 artigos na árvore B+" << endl;
    
    // Teste de busca
    long pos = arv.buscar("ArtigoE");
    if (pos != -1) {
        cout << "ArtigoE encontrado na posição: " << pos << endl;
    } else {
        cout << "ArtigoE não encontrado." << endl;
    }
    
    // Exibe a estrutura da árvore
    arv.exibir();
    return 0;
}

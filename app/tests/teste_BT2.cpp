#include "../include/BTplus_mem.h"
#include <cstring>

using namespace std;

// Função de comparação para inteiros
int cmp_int(const void* a, const void* b) {
    return (*(int*)a) - (*(int*)b);
}

// Serializa: recebe ponteiro para inteiro, retorna string
string serialize_int(const void* chave) {
    int valor = *(int*)chave;
    return to_string(valor);
}

// Desserializa: recebe string, retorna ponteiro para inteiro
void* deserialize_int(const string& s) {
    int* valor = new int;
    *valor = stoi(s);
    return valor;
}

int main() {
    // ArvoreBMais(nome_arquivo, M, tamanho_chave, compare, serialize, deserialize)
    ArvoreBMais arv("data/indice_int.bin", cmp_int, serialize_int, deserialize_int);

    // Inserindo exemplos variados
    int chaves[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 25, 35, 45, 55};
    long offsets[] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 250, 350, 450, 550};

    for (int i = 0; i < 14; i++) {
        arv.inserir(&chaves[i], offsets[i]);
    }

    cout << "Inseridos 14 valores inteiros na árvore B+" << endl;

    // Teste de busca
    int busca = 50;
    long pos = arv.buscar(&busca);
    if (pos != -1) {
        cout << "Chave " << busca << " encontrada na posição: " << pos << endl;
    } else {
        cout << "Chave " << busca << " não encontrada." << endl;
    }

    // Exibe a árvore
    arv.exibir();

    return 0;
}
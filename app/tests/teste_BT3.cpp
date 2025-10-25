#include "../include/BTplus_mem.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <iomanip>
using namespace std;

static int g_tam_chave = 300; // tamanho fixo para títulos

int cmp_str_fixed(const void* a, const void* b) {
    return strncmp((const char*)a, (const char*)b, g_tam_chave);
}

string serialize_str_fixed(const void* chave) {
    const char* c = (const char*)chave;
    string s(c);
    if ((int)s.size() < g_tam_chave) s.resize(g_tam_chave, '\0');
    else if ((int)s.size() > g_tam_chave) s = s.substr(0, g_tam_chave);
    return s;
}

void* deserialize_str_fixed(const string& s) {
    char* buf = new char[g_tam_chave];
    memcpy(buf, s.data(), g_tam_chave);
    return buf;
}

string print_str_fixed(const char* buf, int tam) {
    return string(buf, strnlen(buf, tam));
}

void mostrarNo(const No& no, int tam_chave) {
    cout << "[NO folha=" << no.folha << " qtd=" << no.qtd_chaves << "]\n";
    for (int i=0;i<no.qtd_chaves;++i) {
        string chave(no.getChave(i), tam_chave);
        size_t p = chave.find('\0');
        if (p != string::npos) chave = chave.substr(0,p);
        cout << "  " << setw(10) << chave << " -> " << *no.getOffset(i) << "\n";
    }
}

int main() {
    remove("data/indice_titulo.bin");
    ArvoreBMais arv("data/indice_titulo.bin", g_tam_chave, cmp_str_fixed, serialize_str_fixed, deserialize_str_fixed, print_str_fixed);

    // Gerar 200 títulos
    vector<string> titulos;
    titulos.reserve(200);
    for (int i=1;i<=200;++i) {
        ostringstream os; os << "Titulo" << setfill('0') << setw(3) << i;
        titulos.push_back(os.str());
    }

    // Inserir com offsets artificiais
    long base = 1000;
    for (int i=0;i<200;++i) {
        arv.inserir(titulos[i].c_str(), base + i * 128);
    }
    cout << "Inseridos 200 títulos." << endl;

    // Chaves para buscar
    const char* buscas[] = {"Titulo010","Titulo150","Titulo200","Titulo999"};
    for (const char* chave : buscas) {
        long off = arv.buscar(chave);
        cout << "Busca(" << chave << ") -> offset=" << off << endl;
        No noAchado = arv.buscarNo(chave);
        if (noAchado.qtd_chaves > 0) {
            cout << "Nó que contém (ou percorreu) chave: \n";
            mostrarNo(noAchado, g_tam_chave);
        } else {
            cout << "Chave não encontrada em nenhum nó." << endl;
        }
        cout << "-----------------------------\n";
    }

    // Exibir árvore inteira (opcional)
    //arv.exibir();
    return 0;
}

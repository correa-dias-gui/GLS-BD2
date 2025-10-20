#ifndef ARVORE_H
#define ARVORE_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

const int M = 2; // ordem da árvore

struct No {
    bool folha;
    int qtd_chaves;
    vector<string> chaves;
    vector<long> offsets;
    long proximo;

    No(bool eh_folha = true);
};

class ArvoreBMais {
private:
    fstream arquivo;
    string nome_arquivo;
    long raiz_offset;

    void abrirArquivo();
    long salvarNo(No &no);
    void reescreverNo(long pos, No &no);
    No carregarNo(long pos);

    long dividirNo(long pos, No &no, string chave, long offset_dado);

public:
    ArvoreBMais(string nome);
    ~ArvoreBMais();

    void inserir(string chave, long offset_dado);
    long buscar(string chave);
    void exibir();
};

#endif

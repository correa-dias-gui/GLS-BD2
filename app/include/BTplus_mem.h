#ifndef ARVORE_H
#define ARVORE_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <functional>

using namespace std;

const int M = 2; // ordem da árvore
const int TAM_BLOCO = 4906; // tamanho do bloco em bytes
const int MAX_CHAVES = 2 * M; // número máximo de chaves por nó
const int TAM_CHAVE = 300; // tamanho máximo de cada chave em bytes

#pragma pack(push, 1)
struct No {
    bool folha;                                    // 1 byte
    int qtd_chaves;                               // 4 bytes
    char chaves[MAX_CHAVES * TAM_CHAVE];         // 4 * 300 = 1200 bytes
    long offsets[MAX_CHAVES + 1];                // 5 * 8 = 40 bytes (assumindo long = 8 bytes)
    long proximo;                                // 8 bytes
    char padding[TAM_BLOCO - 1 - 4 - 1200 - 40 - 8]; // resto para completar 4906 bytes

    No(bool eh_folha = true);
};
#pragma pack(pop)

typedef function<int(const void*, const void*)> CompareFunc;
typedef function<void*(const string&)> DeserializeFunc;
typedef function<string(const void*)> SerializeFunc;

class ArvoreBMais {
private:
    fstream arquivo;
    string nome_arquivo;
    long raiz_offset;

    CompareFunc compare;
    SerializeFunc serialize;
    DeserializeFunc deserialize;

    void abrirArquivo();
    long salvarNo(No &no);
    void reescreverNo(long pos, No &no);
    No carregarNo(long pos);

    long dividirNo(long pos, No &no, const std::vector<std::pair<std::string, long>>& pares);
    void exibirNo(long offset, int nivel);
    long encontrarFolha(long no_offset, const void* chave);

public:
    ArvoreBMais(string nome, CompareFunc cmp, SerializeFunc ser, DeserializeFunc deser);
    ~ArvoreBMais();

    void inserir(const void* chave, long offset_dado);
    long buscar(const void* chave);
    void exibir();
};

#endif

#ifndef ARVORE_H
#define ARVORE_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <functional>

using namespace std;

const int TAM_BLOCO = 4906; // tamanho do bloco em bytes

#pragma pack(push, 1)
struct No {
    bool folha;
    int qtd_chaves;
    int max_chaves; // capacidade de chaves neste nó
    int tam_chave;  // tamanho da chave em bytes
    long proximo;   // encadeamento entre folhas
    // Área flexível de dados: chaves consecutivas seguidas de offsets (implementado no .cpp via ponteiros)
    char dados[TAM_BLOCO - 1 - 4 - 4 - 4 - 8];
    No(bool eh_folha = true, int m = 2, int tam_ch = 300);
    char* getChave(int index);
    const char* getChave(int index) const;
    long* getOffset(int index);
    const long* getOffset(int index) const;
    void setChave(int index, const char* chave);
    void setOffset(int index, long offset);
private:
    int calcularOffsetChaves() const; // início das chaves
    int calcularOffsetOffsets() const; // início dos offsets
};
#pragma pack(pop)

typedef function<int(const void*, const void*)> CompareFunc;
typedef function<void*(const string&)> DeserializeFunc;
typedef function<string(const void*)> SerializeFunc;
typedef function<string(const char*, int)> PrintKeyFunc;

class ArvoreBMais {
private:
    struct Metadata {
        int altura;        // altura da árvore
        long raiz_offset;  // offset do bloco raiz
        long qtd_itens;    // quantidade total de itens
        int tam_chave;     // tamanho da chave em bytes
        int M;             // ordem calculada
        int max_chaves;    // 2*M
    } metadata;

    struct SplitResult {
        bool split;              // houve divisão
        std::string promotedKey; // chave promovida para o pai
        long rightOffset;        // offset do novo nó à direita
        SplitResult(): split(false), promotedKey(), rightOffset(-1) {}
    };

    fstream arquivo;
    string nome_arquivo;

    CompareFunc compare;
    SerializeFunc serialize;
    DeserializeFunc deserialize;
    PrintKeyFunc printKey;

    // Métodos internos
    void abrirArquivo();
    void escreverMetadata();
    void carregarMetadata();
    void atualizarMetadata();

    long salvarNo(No &no);           // adiciona nó após bloco de metadata
    void reescreverNo(long pos, No &no);
    No carregarNo(long pos);

    long dividirNo(long pos, No &no, const std::vector<std::pair<std::string, long>>& pares); // (pode ser removida futuramente)
    void exibirNo(long offset, int nivel);
    long encontrarFolha(long no_offset, const void* chave);

    int calcularM(int tam_chave) const; // cálculo de M conforme fórmula

    void registrarBusca(const string& chave, int blocos_lidos);

    SplitResult inserirRecursivo(long no_offset, const std::string& chave_ser, long dado_offset); // nova função

public:
    ArvoreBMais(string nome, int tam_chave, CompareFunc cmp, SerializeFunc ser, DeserializeFunc deser, PrintKeyFunc pkey);
    ~ArvoreBMais();

    void inserir(const void* chave, long offset_dado);
    long buscar(const void* chave);
    void exibir();
};

#endif

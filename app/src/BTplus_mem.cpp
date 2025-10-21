// arvore.cpp
#include "../include/BTplus_mem.h"
#include <algorithm>

No::No(bool eh_folha) {
    folha = eh_folha;
    qtd_chaves = 0;
    memset(chaves, 0, sizeof(chaves));
    memset(offsets, -1, sizeof(offsets));
    proximo = -1;
    memset(padding, 0, sizeof(padding)); // inicializa o padding
}

ArvoreBMais::ArvoreBMais(string nome, CompareFunc cmp, SerializeFunc ser, DeserializeFunc deser)
    : nome_arquivo(nome), compare(cmp), serialize(ser), deserialize(deser) {
    raiz_offset = -1;
    abrirArquivo();
}

ArvoreBMais::~ArvoreBMais() {
    arquivo.close();
}

void ArvoreBMais::abrirArquivo() {
    arquivo.open(nome_arquivo, ios::in | ios::out | ios::binary);
    if (!arquivo.is_open()) {
        ofstream criar(nome_arquivo, ios::binary);
        criar.close();
        arquivo.open(nome_arquivo, ios::in | ios::out | ios::binary);
    }
}

long ArvoreBMais::salvarNo(No &no) {
    arquivo.clear();
    arquivo.seekp(0, ios::end);
    long pos = arquivo.tellp();
    arquivo.write(reinterpret_cast<char*>(&no), TAM_BLOCO);
    arquivo.flush();
    return pos;
}

void ArvoreBMais::reescreverNo(long pos, No &no) {
    arquivo.clear();
    arquivo.seekp(pos);
    arquivo.write(reinterpret_cast<char*>(&no), TAM_BLOCO);
    arquivo.flush();
}

No ArvoreBMais::carregarNo(long pos) {
    arquivo.clear();
    arquivo.seekg(pos);
    No no;
    arquivo.read(reinterpret_cast<char*>(&no), TAM_BLOCO);
    return no;
}

long ArvoreBMais::dividirNo(long pos, No &no, const std::vector<std::pair<std::string, long>>& pares) {
    int total = pares.size();
    int meio = total / 2;
    // Atualiza nó atual (primeira metade)
    for (int i = 0; i < meio; i++) {
        strncpy(no.chaves + i * TAM_CHAVE, pares[i].first.c_str(), TAM_CHAVE);
        no.offsets[i] = pares[i].second;
    }
    no.qtd_chaves = meio;
    // Cria novo nó folha (segunda metade)
    No novo_no(true);
    novo_no.qtd_chaves = total - meio;
    for (int i = 0; i < novo_no.qtd_chaves; i++) {
        strncpy(novo_no.chaves + i * TAM_CHAVE, pares[meio + i].first.c_str(), TAM_CHAVE);
        novo_no.offsets[i] = pares[meio + i].second;
    }
    // Encadeamento das folhas
    novo_no.proximo = no.proximo;
    no.proximo = salvarNo(novo_no);
    // Atualiza o nó original
    reescreverNo(pos, no);
    return no.proximo;
}


void ArvoreBMais::inserir(const void* chave, long offset_dado) {
    if (raiz_offset == -1) {
        No novo(true);
        string chave_serializada = serialize(chave);
        strncpy(novo.chaves, chave_serializada.c_str(), TAM_CHAVE);
        novo.offsets[0] = offset_dado;
        novo.qtd_chaves = 1;
        raiz_offset = salvarNo(novo);
        return;
    }

    No raiz = carregarNo(raiz_offset);
    vector<pair<string, long>> pares;
    for (int i = 0; i < raiz.qtd_chaves; i++) {
        string chave_str(raiz.chaves + i * TAM_CHAVE, TAM_CHAVE);
        chave_str = chave_str.substr(0, chave_str.find('\0'));
        pares.emplace_back(chave_str, raiz.offsets[i]);
    }
    string chave_serializada = serialize(chave);
    pares.emplace_back(chave_serializada, offset_dado);
    stable_sort(pares.begin(), pares.end(), [this](const auto &a, const auto &b) {
        return compare(a.first.c_str(), b.first.c_str()) < 0;
    });
    int total = pares.size();
    if (total <= MAX_CHAVES) {
        for (int i = 0; i < total; i++) {
            strncpy(raiz.chaves + i * TAM_CHAVE, pares[i].first.c_str(), TAM_CHAVE);
            raiz.offsets[i] = pares[i].second;
        }
        raiz.qtd_chaves = total;
        reescreverNo(raiz_offset, raiz);
    } else {
        // Divisão da raiz folha
        int meio = total / 2;
        // Atualiza nó folha original (primeira metade)
        for (int i = 0; i < meio; i++) {
            strncpy(raiz.chaves + i * TAM_CHAVE, pares[i].first.c_str(), TAM_CHAVE);
            raiz.offsets[i] = pares[i].second;
        }
        raiz.qtd_chaves = meio;
        // Cria novo nó folha (segunda metade)
        No novo_no(true);
        novo_no.qtd_chaves = total - meio;
        for (int i = 0; i < novo_no.qtd_chaves; i++) {
            strncpy(novo_no.chaves + i * TAM_CHAVE, pares[meio + i].first.c_str(), TAM_CHAVE);
            novo_no.offsets[i] = pares[meio + i].second;
        }
        novo_no.proximo = raiz.proximo;
        long novo_offset = salvarNo(novo_no);
        raiz.proximo = novo_offset;
        reescreverNo(raiz_offset, raiz);
        // Cria novo nó raiz interno
        No raiz_interna(false);
        raiz_interna.qtd_chaves = 1;
        // Menor chave do segundo nó folha
        strncpy(raiz_interna.chaves, novo_no.chaves, TAM_CHAVE);
        raiz_interna.offsets[0] = raiz_offset;
        raiz_interna.offsets[1] = novo_offset;
        raiz_interna.proximo = -1;
        raiz_offset = salvarNo(raiz_interna);
    }
}


long ArvoreBMais::buscar(const void* chave) {
    arquivo.clear();
    arquivo.seekg(0);
    string chave_serializada = serialize(chave);
    long pos = 0;
    No no;

    while (arquivo.read(reinterpret_cast<char*>(&no), TAM_BLOCO)) {
        for (int i = 0; i < no.qtd_chaves; i++) {
            if (compare(chave_serializada.c_str(), no.chaves + i * TAM_CHAVE) == 0) {
                return pos;
            }
        }
        pos = no.proximo;
    }
    return -1;
}

void ArvoreBMais::exibir() {
    arquivo.clear();
    arquivo.seekg(0);
    cout << "=== Conteúdo da árvore (" << nome_arquivo << ") ===" << endl;
    No no;
    long pos = 0;
    int no_idx = 0;
    while (arquivo.read(reinterpret_cast<char*>(&no), TAM_BLOCO)) {
        cout << "[Nó " << no_idx++ << "] Folha: " << no.folha << ", qtd_chaves: " << no.qtd_chaves << ", proximo: " << no.proximo << endl;
        for (int i = 0; i < no.qtd_chaves; i++) {
            string chave_str(no.chaves + i * TAM_CHAVE, TAM_CHAVE);
            // Remove possíveis caracteres nulos do final
            chave_str = chave_str.substr(0, chave_str.find('\0'));
            cout << "  Chave: '" << chave_str << "' | Offset: " << no.offsets[i] << endl;
        }
        cout << "-----------------------------" << endl;
        pos = no.proximo;
    }
}

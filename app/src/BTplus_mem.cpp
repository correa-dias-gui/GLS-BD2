// arvore.cpp
#include "../include/BTplus_mem.h"
#include <algorithm>

No::No(bool eh_folha) {
    folha = eh_folha;
    qtd_chaves = 0;
    chaves.resize(2 * M);
    offsets.resize(2 * M + 1, -1);
    proximo = -1;
}

ArvoreBMais::ArvoreBMais(string nome) {
    nome_arquivo = nome;
    raiz_offset = -1;
    abrirArquivo();
}

ArvoreBMais::~ArvoreBMais() {
    arquivo.close();
}

void ArvoreBMais::abrirArquivo() {
    arquivo.open(nome_arquivo, ios::in | ios::out | ios::app);
    if (!arquivo.is_open()) {
        ofstream criar(nome_arquivo);
        criar.close();
        arquivo.open(nome_arquivo, ios::in | ios::out | ios::app);
    }
}

long ArvoreBMais::salvarNo(No &no) {
    arquivo.clear();
    arquivo.seekp(0, ios::end);
    long pos = arquivo.tellp();
    arquivo << (no.folha ? "F" : "I") << ";"
            << no.qtd_chaves << ";";
    for (int i = 0; i < 2 * M; i++)
        arquivo << no.chaves[i] << ";";
    for (int i = 0; i < 2 * M + 1; i++)
        arquivo << no.offsets[i] << ";";
    arquivo << no.proximo << "\n";
    arquivo.flush();
    return pos;
}

void ArvoreBMais::reescreverNo(long pos, No &no) {
    arquivo.clear();
    arquivo.seekp(pos);
    arquivo << (no.folha ? "F" : "I") << ";"
            << no.qtd_chaves << ";";
    for (int i = 0; i < 2 * M; i++)
        arquivo << no.chaves[i] << ";";
    for (int i = 0; i < 2 * M + 1; i++)
        arquivo << no.offsets[i] << ";";
    arquivo << no.proximo << "\n";
    arquivo.flush();
}

No ArvoreBMais::carregarNo(long pos) {
    arquivo.clear();
    arquivo.seekg(pos);
    string linha;
    getline(arquivo, linha);
    stringstream ss(linha);

    No no;
    string tipo;
    getline(ss, tipo, ';');
    no.folha = (tipo == "F");

    string temp;
    getline(ss, temp, ';');
    no.qtd_chaves = stoi(temp);

    for (int i = 0; i < 2 * M; i++)
        getline(ss, no.chaves[i], ';');
    for (int i = 0; i < 2 * M + 1; i++) {
        getline(ss, temp, ';');
        no.offsets[i] = stol(temp);
    }
    getline(ss, temp, ';');
    no.proximo = stol(temp);
    return no;
}

long ArvoreBMais::dividirNo(long pos, No &no, string chave, long offset_dado) {
    // Combina chaves existentes + nova
    vector<pair<string, long>> pares;
    for (int i = 0; i < no.qtd_chaves; i++)
        pares.push_back({no.chaves[i], no.offsets[i]});
    if (!chave.empty())
        pares.push_back({chave, offset_dado});

    // Ordena mantendo duplicatas
    stable_sort(pares.begin(), pares.end(),
                [](auto &a, auto &b) { return a.first < b.first; });

    int total = pares.size();
    int meio = total / 2;

    // Atualiza nó atual (primeira metade)
    for (int i = 0; i < meio; i++) {
        no.chaves[i] = pares[i].first;
        no.offsets[i] = pares[i].second;
    }
    no.qtd_chaves = meio;

    // Cria novo nó folha (segunda metade)
    No novo_no(true);
    novo_no.qtd_chaves = total - meio;
    for (int i = 0; i < novo_no.qtd_chaves; i++) {
        novo_no.chaves[i] = pares[meio + i].first;
        novo_no.offsets[i] = pares[meio + i].second;
    }

    // Encadeamento das folhas
    novo_no.proximo = no.proximo;
    no.proximo = salvarNo(novo_no);

    // Atualiza o nó original
    reescreverNo(pos, no);

    return no.proximo;
}


void ArvoreBMais::inserir(string chave, long offset_dado) {
    if (raiz_offset == -1) {
        No novo(true);
        novo.chaves[0] = chave;
        novo.offsets[0] = offset_dado;
        novo.qtd_chaves = 1;
        raiz_offset = salvarNo(novo);
        return;
    }

    No raiz = carregarNo(raiz_offset);

    // Aceita chaves duplicadas → apenas insere no fim e ordena
    raiz.chaves[raiz.qtd_chaves] = chave;
    raiz.offsets[raiz.qtd_chaves] = offset_dado;
    raiz.qtd_chaves++;

    // Ordena pares (chave, offset) mantendo duplicatas
    vector<pair<string, long>> pares;
    for (int i = 0; i < raiz.qtd_chaves; i++)
        pares.push_back({raiz.chaves[i], raiz.offsets[i]});
    sort(pares.begin(), pares.end(),
         [](auto &a, auto &b) { return a.first < b.first; });

    for (int i = 0; i < raiz.qtd_chaves; i++) {
        raiz.chaves[i] = pares[i].first;
        raiz.offsets[i] = pares[i].second;
    }

    if (raiz.qtd_chaves <= 2 * M)
        reescreverNo(raiz_offset, raiz);
    else
        dividirNo(raiz_offset, raiz, "", -1);
}


long ArvoreBMais::buscar(string chave) {
    arquivo.clear();
    arquivo.seekg(0);
    string linha;
    long pos = 0;
    while (getline(arquivo, linha)) {
        if (linha.find(chave + ";") != string::npos)
            return pos;
        pos = arquivo.tellg();
    }
    return -1;
}

void ArvoreBMais::exibir() {
    arquivo.clear();
    arquivo.seekg(0);
    string linha;
    cout << "=== Conteúdo da árvore (" << nome_arquivo << ") ===" << endl;
    while (getline(arquivo, linha))
        cout << linha << endl;
}

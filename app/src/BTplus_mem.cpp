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
    
    // Se a raiz é uma folha, inserir diretamente
    if (raiz.folha) {
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
            // Copia apenas a chave separadora (primeira chave do segundo nó)
            strncpy(raiz_interna.chaves, novo_no.chaves, TAM_CHAVE);
            // Offsets apontam para os nós filhos (não para dados)
            raiz_interna.offsets[0] = raiz_offset;  // primeiro filho (folha esquerda)
            raiz_interna.offsets[1] = novo_offset;  // segundo filho (folha direita)
            raiz_interna.proximo = -1;
            raiz_offset = salvarNo(raiz_interna);
        }
    } else {
        // Se a raiz é um nó interno, encontrar a folha correta para inserir
        long folha_offset = encontrarFolha(raiz_offset, chave);
        string chave_serializada = serialize(chave);
        
        // Carrega a folha e insere nela
        No folha = carregarNo(folha_offset);
        vector<pair<string, long>> pares;
        for (int i = 0; i < folha.qtd_chaves; i++) {
            string chave_str(folha.chaves + i * TAM_CHAVE, TAM_CHAVE);
            chave_str = chave_str.substr(0, chave_str.find('\0'));
            pares.emplace_back(chave_str, folha.offsets[i]);
        }
        pares.emplace_back(chave_serializada, offset_dado);
        stable_sort(pares.begin(), pares.end(), [this](const auto &a, const auto &b) {
            return compare(a.first.c_str(), b.first.c_str()) < 0;
        });
        
        // Reinsere na folha
        int total = pares.size();
        if (total <= MAX_CHAVES) {
            for (int i = 0; i < total; i++) {
                strncpy(folha.chaves + i * TAM_CHAVE, pares[i].first.c_str(), TAM_CHAVE);
                folha.offsets[i] = pares[i].second;
            }
            folha.qtd_chaves = total;
            reescreverNo(folha_offset, folha);
        } else {
            // Divisão de folha necessária
            int meio = total / 2;
            
            // Atualiza folha atual (primeira metade)
            for (int i = 0; i < meio; i++) {
                strncpy(folha.chaves + i * TAM_CHAVE, pares[i].first.c_str(), TAM_CHAVE);
                folha.offsets[i] = pares[i].second;
            }
            folha.qtd_chaves = meio;
            
            // Cria nova folha (segunda metade)
            No nova_folha(true);
            nova_folha.qtd_chaves = total - meio;
            for (int i = 0; i < nova_folha.qtd_chaves; i++) {
                strncpy(nova_folha.chaves + i * TAM_CHAVE, pares[meio + i].first.c_str(), TAM_CHAVE);
                nova_folha.offsets[i] = pares[meio + i].second;
            }
            nova_folha.proximo = folha.proximo;
            long nova_folha_offset = salvarNo(nova_folha);
            folha.proximo = nova_folha_offset;
            reescreverNo(folha_offset, folha);
            
            // Atualizar raiz para incluir nova folha
            // Precisa adicionar nova chave separadora na raiz
            string chave_separadora(nova_folha.chaves, TAM_CHAVE);
            chave_separadora = chave_separadora.substr(0, chave_separadora.find('\0'));
            
            // Carrega a raiz e adiciona nova entrada
            No raiz = carregarNo(raiz_offset);
            if (raiz.qtd_chaves < MAX_CHAVES) {
                // Espaço na raiz - adiciona nova entrada
                raiz.qtd_chaves++;
                strncpy(raiz.chaves + (raiz.qtd_chaves - 1) * TAM_CHAVE, chave_separadora.c_str(), TAM_CHAVE);
                raiz.offsets[raiz.qtd_chaves] = nova_folha_offset;
                reescreverNo(raiz_offset, raiz);
            } else {
                // Raiz cheia - precisa dividir a raiz (crescimento da árvore)
                vector<pair<string, long>> entradas_raiz;
                for (int i = 0; i < raiz.qtd_chaves; i++) {
                    string chave_str(raiz.chaves + i * TAM_CHAVE, TAM_CHAVE);
                    chave_str = chave_str.substr(0, chave_str.find('\0'));
                    entradas_raiz.emplace_back(chave_str, raiz.offsets[i + 1]);
                }
                entradas_raiz.emplace_back(chave_separadora, nova_folha_offset);
                
                sort(entradas_raiz.begin(), entradas_raiz.end(), [this](const auto &a, const auto &b) {
                    return compare(a.first.c_str(), b.first.c_str()) < 0;
                });
                
                int meio_raiz = entradas_raiz.size() / 2;
                
                // Atualiza raiz atual (primeira metade)
                raiz.qtd_chaves = meio_raiz;
                for (int i = 0; i < meio_raiz; i++) {
                    strncpy(raiz.chaves + i * TAM_CHAVE, entradas_raiz[i].first.c_str(), TAM_CHAVE);
                    raiz.offsets[i + 1] = entradas_raiz[i].second;
                }
                
                // Cria novo nó interno (segunda metade)
                No novo_interno(false);
                novo_interno.qtd_chaves = entradas_raiz.size() - meio_raiz - 1;
                for (int i = 0; i < novo_interno.qtd_chaves; i++) {
                    strncpy(novo_interno.chaves + i * TAM_CHAVE, entradas_raiz[meio_raiz + 1 + i].first.c_str(), TAM_CHAVE);
                    novo_interno.offsets[i + 1] = entradas_raiz[meio_raiz + 1 + i].second;
                }
                novo_interno.offsets[0] = entradas_raiz[meio_raiz].second;
                long novo_interno_offset = salvarNo(novo_interno);
                
                reescreverNo(raiz_offset, raiz);
                
                // Cria nova raiz
                No nova_raiz(false);
                nova_raiz.qtd_chaves = 1;
                strncpy(nova_raiz.chaves, entradas_raiz[meio_raiz].first.c_str(), TAM_CHAVE);
                nova_raiz.offsets[0] = raiz_offset;
                nova_raiz.offsets[1] = novo_interno_offset;
                raiz_offset = salvarNo(nova_raiz);
            }
        }
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
    cout << "=== Conteúdo da árvore (" << nome_arquivo << ") ===" << endl;
    cout << "Raiz offset: " << raiz_offset << endl;
    
    if (raiz_offset == -1) {
        cout << "Árvore vazia" << endl;
        return;
    }
    
    // Exibe a árvore começando pela raiz
    exibirNo(raiz_offset, 0);
}

void ArvoreBMais::exibirNo(long offset, int nivel) {
    if (offset == -1) return;
    
    No no = carregarNo(offset);
    string indent(nivel * 2, ' ');
    
    cout << indent << "[Nó offset=" << offset << "] Folha: " << no.folha 
         << ", qtd_chaves: " << no.qtd_chaves << ", proximo: " << no.proximo << endl;
    
    for (int i = 0; i < no.qtd_chaves; i++) {
        string chave_str(no.chaves + i * TAM_CHAVE, TAM_CHAVE);
        chave_str = chave_str.substr(0, chave_str.find('\0'));
        cout << indent << "  Chave: '" << chave_str << "' | Offset: " << no.offsets[i] << endl;
        
        // Se é nó interno, exibe recursivamente os filhos
        if (!no.folha && i == 0) {
            exibirNo(no.offsets[i], nivel + 1);
        }
    }
    
    // Para nós internos, exibe o último filho
    if (!no.folha && no.qtd_chaves > 0) {
        exibirNo(no.offsets[no.qtd_chaves], nivel + 1);
    }
    
    cout << indent << "-----------------------------" << endl;
}

long ArvoreBMais::encontrarFolha(long no_offset, const void* chave) {
    No no = carregarNo(no_offset);
    
    if (no.folha) {
        return no_offset;
    }
    
    string chave_serializada = serialize(chave);
    
    // Encontra o filho correto baseado nas chaves separadoras
    int i = 0;
    while (i < no.qtd_chaves && compare(chave_serializada.c_str(), no.chaves + i * TAM_CHAVE) >= 0) {
        i++;
    }
    
    // Recursivamente encontra a folha no filho apropriado
    return encontrarFolha(no.offsets[i], chave);
}

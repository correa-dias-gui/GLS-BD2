// arvore.cpp
#include "../include/BTplus_mem.h"
#include <algorithm>
#include <cmath>

// ================= Implementação do No dinâmico =================
No::No(bool eh_folha, int m, int tam_ch) {
    folha = eh_folha;
    qtd_chaves = 0;
    max_chaves = 2 * m;
    tam_chave = tam_ch;
    proximo = -1;
    memset(dados, 0, sizeof(dados));
    for (int i = 0; i <= max_chaves; ++i) setOffset(i, -1);
}

char* No::getChave(int index) {
    if (index < 0 || index >= max_chaves) return nullptr;
    return dados + (index * tam_chave);
}
const char* No::getChave(int index) const {
    if (index < 0 || index >= max_chaves) return nullptr;
    return dados + (index * tam_chave);
}
long* No::getOffset(int index) {
    if (index < 0 || index > max_chaves) return nullptr;
    char* base = dados + calcularOffsetOffsets();
    return reinterpret_cast<long*>(base) + index;
}
const long* No::getOffset(int index) const {
    if (index < 0 || index > max_chaves) return nullptr;
    const char* base = dados + calcularOffsetOffsets();
    return reinterpret_cast<const long*>(base) + index;
}
void No::setChave(int index, const char* chave) {
    if (index < 0 || index >= max_chaves || !chave) return;
    char* pos = getChave(index);
    if (!pos) return;
    // Copia exatamente tam_chave bytes; caller garante tamanho (preencher com zeros se menor)
    memcpy(pos, chave, tam_chave);
}
void No::setOffset(int index, long offset) {
    long* dst = getOffset(index);
    if (dst) *dst = offset;
}
int No::calcularOffsetChaves() const { return 0; }
int No::calcularOffsetOffsets() const { return max_chaves * tam_chave; }

// ================= ArvoreBMais =================
int ArvoreBMais::calcularM(int tam_chave) const {
    // Fórmula: 2M * tam_chave + (2M + 1) * sizeof(long) <= TAM_BLOCO - sizeof(bool) - sizeof(int)
    // Resolve para M dado tam_chave.
    int overhead = sizeof(bool) + sizeof(int); // folha + qtd_chaves dentro do nó (simplificado para cálculo)
    int available = TAM_BLOCO - overhead;
    // 2M*tam_chave + (2M+1)*8 <= available
    // 2M*(tam_chave + 8) + 8 <= available
    // 2M*(tam_chave + 8) <= available - 8
    // M <= (available - 8) / (2*(tam_chave + 8))
    int M = (available - 8) / (2 * (tam_chave + 8));
    if (M < 1) M = 1;
    return M;
}

ArvoreBMais::ArvoreBMais(string nome, int tam_chave, CompareFunc cmp, SerializeFunc ser, DeserializeFunc deser, PrintKeyFunc pk)
    : nome_arquivo(nome), compare(cmp), serialize(ser), deserialize(deser), printKey(pk) {
    abrirArquivo();
    // Se arquivo vazio, inicializa metadata
    arquivo.seekg(0, ios::end);
    if (arquivo.tellg() == 0) {
        metadata.tam_chave = tam_chave;
        metadata.M = calcularM(tam_chave);
        metadata.max_chaves = 2 * metadata.M;
        metadata.altura = 0; // árvore vazia
        metadata.raiz_offset = -1;
        metadata.qtd_itens = 0;
        escreverMetadata();
    } else {
        carregarMetadata();
    }
}

ArvoreBMais::~ArvoreBMais() { arquivo.close(); }

void ArvoreBMais::abrirArquivo() {
    arquivo.open(nome_arquivo, ios::in | ios::out | ios::binary);
    if (!arquivo.is_open()) {
        ofstream criar(nome_arquivo, ios::binary);
        criar.close();
        arquivo.open(nome_arquivo, ios::in | ios::out | ios::binary);
    }
}

void ArvoreBMais::escreverMetadata() {
    arquivo.clear();
    arquivo.seekp(0, ios::beg);
    arquivo.write(reinterpret_cast<char*>(&metadata), sizeof(Metadata));
    // Preenche restante do bloco de metadata com zeros
    size_t restante = TAM_BLOCO - sizeof(Metadata);
    if (restante > 0) {
        vector<char> zeros(restante, 0);
        arquivo.write(zeros.data(), restante);
    }
    arquivo.flush();
}
void ArvoreBMais::carregarMetadata() {
    arquivo.clear();
    arquivo.seekg(0, ios::beg);
    arquivo.read(reinterpret_cast<char*>(&metadata), sizeof(Metadata));
}
void ArvoreBMais::atualizarMetadata() { escreverMetadata(); }

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
    int total = (int)pares.size();
    int meio = total / 2;
    // Atualiza nó atual
    for (int i = 0; i < meio; ++i) {
        no.setChave(i, pares[i].first.c_str());
        no.setOffset(i, pares[i].second);
    }
    no.qtd_chaves = meio;
    // Novo nó folha
    No novo(true, metadata.M, metadata.tam_chave);
    novo.qtd_chaves = total - meio;
    for (int i = 0; i < novo.qtd_chaves; ++i) {
        novo.setChave(i, pares[meio + i].first.c_str());
        novo.setOffset(i, pares[meio + i].second);
    }
    novo.proximo = no.proximo;
    long novo_offset = salvarNo(novo);
    no.proximo = novo_offset;
    reescreverNo(pos, no);
    return novo_offset;
}

long ArvoreBMais::encontrarFolha(long no_offset, const void* chave) {
    No no = carregarNo(no_offset);
    if (no.folha) return no_offset;
    string chave_ser = serialize(chave);
    int i = 0;
    while (i < no.qtd_chaves && compare(chave_ser.c_str(), no.getChave(i)) >= 0) i++;
    long* childPtr = const_cast<long*>(no.getOffset(i));
    return encontrarFolha(*childPtr, chave);
}

ArvoreBMais::SplitResult ArvoreBMais::inserirRecursivo(long no_offset, const std::string& chave_ser, long dado_offset) {
    No no = carregarNo(no_offset);
    debug(string("[DEBUG] inserirRecursivo offset=") + to_string(no_offset) + " folha=" + to_string(no.folha) + " qtd=" + to_string(no.qtd_chaves) + " chave='" + chave_ser + "'");
    if (no.folha) {
        // Inserir em folha
        vector<pair<string,long>> pares;
        for (int i=0;i<no.qtd_chaves;++i) {
            string c(no.getChave(i), no.tam_chave); // representação fixa
            pares.emplace_back(c, *no.getOffset(i));
        }
        pares.emplace_back(chave_ser, dado_offset);
        std::sort(pares.begin(), pares.end(), [&](const auto& A, const auto& B){
            void* ca = deserialize(A.first);
            void* cb = deserialize(B.first);
            int r = compare(ca, cb);
            delete reinterpret_cast<char*>(ca);
            delete reinterpret_cast<char*>(cb);
            return r < 0;
        });
        if ((int)pares.size() <= no.max_chaves) {
            for (int i=0;i<(int)pares.size();++i) { no.setChave(i, pares[i].first.c_str()); no.setOffset(i, pares[i].second); }
            no.qtd_chaves = (int)pares.size();
            reescreverNo(no_offset, no);
            debug(string("[DEBUG] Inserção em folha sem split offset=") + to_string(no_offset) + " total=" + to_string(pares.size()));
            return ArvoreBMais::SplitResult();
        } else {
            // split folha
            int meio = pares.size()/2;
            for (int i=0;i<meio;++i){ no.setChave(i, pares[i].first.c_str()); no.setOffset(i, pares[i].second);} no.qtd_chaves = meio;
            No direita(true, metadata.M, metadata.tam_chave);
            direita.qtd_chaves = (int)pares.size()-meio;
            for (int i=0;i<direita.qtd_chaves;++i){ direita.setChave(i, pares[meio+i].first.c_str()); direita.setOffset(i, pares[meio+i].second);} 
            direita.proximo = no.proximo;
            long direita_offset = salvarNo(direita);
            no.proximo = direita_offset;
            reescreverNo(no_offset, no);
            debug(string("[DEBUG] Split folha offset=") + to_string(no_offset) + " meio=" + to_string(meio) + " nova_folha_offset=" + to_string(direita_offset));
            ArvoreBMais::SplitResult res; res.split = true; res.promotedKey = string(direita.getChave(0), metadata.tam_chave); res.rightOffset = direita_offset; return res;
        }
    } else {
        // Nó interno: encontrar filho
    int i=0; while (i<no.qtd_chaves && compare(chave_ser.c_str(), no.getChave(i))>=0) { i++; }
    debug(string("[DEBUG] Nó interno descendo index=") + to_string(i) + " offset=" + to_string(no_offset));
        long child = *no.getOffset(i);
    debug(string("[DEBUG] Child offset=") + to_string(child));
        SplitResult childRes = inserirRecursivo(child, chave_ser, dado_offset);
    if (!childRes.split) return ArvoreBMais::SplitResult();
        // Precisa inserir chave promovida neste nó interno
        vector<string> chaves; vector<long> filhos;
        // Carregar filhos existentes
        for (int k=0;k<=no.qtd_chaves;++k) filhos.push_back(*no.getOffset(k));
    for (int k=0;k<no.qtd_chaves;++k){ string c(no.getChave(k), no.tam_chave); chaves.push_back(c);}        
        // Inserir promotedKey na posição correta
        int pos=0; while (pos<(int)chaves.size() && compare(childRes.promotedKey.c_str(), chaves[pos].c_str())>0) pos++;
        chaves.insert(chaves.begin()+pos, childRes.promotedKey);
        filhos.insert(filhos.begin()+pos+1, childRes.rightOffset);
        if ((int)chaves.size() <= no.max_chaves) {
            no.qtd_chaves = (int)chaves.size();
            for (int k=0;k<no.qtd_chaves;++k){ no.setChave(k, chaves[k].c_str()); no.setOffset(k, filhos[k]); }
            no.setOffset(no.qtd_chaves, filhos[no.qtd_chaves]);
            reescreverNo(no_offset, no);
            debug(string("[DEBUG] Inserido promotedKey em interno offset=") + to_string(no_offset) + " qtd_chaves=" + to_string(no.qtd_chaves));
            return ArvoreBMais::SplitResult();
        } else {
            // Split interno
            int meio = chaves.size()/2;
            string chave_promovida = chaves[meio];
            debug(string("[DEBUG] Split interno offset=") + to_string(no_offset) + " meio=" + to_string(meio));
            No direita(false, metadata.M, metadata.tam_chave);
            // direita recebe chaves após meio
            direita.qtd_chaves = (int)chaves.size() - meio - 1;
            for (int k=0;k<direita.qtd_chaves;++k){ direita.setChave(k, chaves[meio+1+k].c_str()); direita.setOffset(k, filhos[meio+1+k]); }
            direita.setOffset(direita.qtd_chaves, filhos.back());
            long direita_offset = salvarNo(direita);
            // no (esquerda) mantém chaves antes de meio
            no.qtd_chaves = meio;
            for (int k=0;k<meio;++k){ no.setChave(k, chaves[k].c_str()); no.setOffset(k, filhos[k]); }
            no.setOffset(meio, filhos[meio]);
            reescreverNo(no_offset, no);
            ArvoreBMais::SplitResult res; res.split=true; res.promotedKey=chave_promovida; res.rightOffset=direita_offset; return res;
        }
    }
}

void ArvoreBMais::inserir(const void* chave, long offset_dado) {
    std::string chave_ser = serialize(chave);
    if (metadata.raiz_offset == -1) {
        No folha(true, metadata.M, metadata.tam_chave);
        folha.setChave(0, chave_ser.c_str()); folha.setOffset(0, offset_dado); folha.qtd_chaves=1;
        metadata.raiz_offset = salvarNo(folha); metadata.qtd_itens=1; metadata.altura=1; atualizarMetadata(); return;
    }
    ArvoreBMais::SplitResult res = inserirRecursivo(metadata.raiz_offset, chave_ser, offset_dado);
    if (res.split) {
        // Criar nova raiz
        No nova_raiz(false, metadata.M, metadata.tam_chave);
        nova_raiz.qtd_chaves = 1;
        nova_raiz.setChave(0, res.promotedKey.c_str());
        nova_raiz.setOffset(0, metadata.raiz_offset);
        nova_raiz.setOffset(1, res.rightOffset);
        metadata.raiz_offset = salvarNo(nova_raiz);
        metadata.altura++;
    }
    metadata.qtd_itens++;
    atualizarMetadata();
}

void ArvoreBMais::registrarBusca(const string& chave, int blocos_lidos) {
    string log_nome = nome_arquivo + ".log";
    ofstream log(log_nome, ios::app);
    log << chave << ';' << blocos_lidos << '\n';
}

long ArvoreBMais::buscar(const void* chave) {
    if (metadata.raiz_offset == -1) {
        registrarBusca(serialize(chave), 0);
        return -1;
    }
    string chave_ser = serialize(chave);
    int blocos = 0;
    long atual = metadata.raiz_offset;
    while (atual != -1) {
        No no = carregarNo(atual);
        ++blocos;
        // Busca linear nas chaves
        for (int i=0;i<no.qtd_chaves;++i){
            if (compare(chave_ser.c_str(), no.getChave(i))==0){
                registrarBusca(chave_ser, blocos);
                return *no.getOffset(i);
            }
            if (!no.folha && compare(chave_ser.c_str(), no.getChave(i))<0){
                atual = *no.getOffset(i);
                goto next_loop; // simula descida
            }
        }
        if (!no.folha) {
            atual = *no.getOffset(no.qtd_chaves);
        } else {
            atual = no.proximo; // seguir folhas para busca sequencial
        }
        next_loop:;
    }
    registrarBusca(chave_ser, blocos);
    return -1;
}

No ArvoreBMais::buscarNo(const void* chave) {
    No vazio(true, metadata.M, metadata.tam_chave); // retornado se não encontrar
    if (metadata.raiz_offset == -1) return vazio;
    string chave_ser = serialize(chave);
    long atual = metadata.raiz_offset;
    while (atual != -1) {
        No no = carregarNo(atual);
        // Procura chave neste nó
        for (int i = 0; i < no.qtd_chaves; ++i) {
            if (compare(chave_ser.c_str(), no.getChave(i)) == 0) {
                return no; // nó encontrado (folha ou interno)
            }
            if (!no.folha && compare(chave_ser.c_str(), no.getChave(i)) < 0) {
                long prox = *no.getOffset(i);
                atual = prox;
                goto next_iter; // descer
            }
        }
        if (!no.folha) {
            atual = *no.getOffset(no.qtd_chaves);
        } else {
            // folha: tentar seguir lista encadeada só se duplicatas possíveis
            atual = no.proximo;
        }
        next_iter:;
    }
    return vazio;
}

void ArvoreBMais::exibir() {
    cout << "=== Conteúdo da árvore (" << nome_arquivo << ") ===" << endl;
    cout << "Altura: " << metadata.altura << " | Itens: " << metadata.qtd_itens << " | tam_chave: " << metadata.tam_chave << " | M: " << metadata.M << endl;
    cout << "Raiz offset: " << metadata.raiz_offset << endl;
    if (metadata.raiz_offset == -1) { cout << "Árvore vazia" << endl; return; }
    exibirNo(metadata.raiz_offset, 0);
}

void ArvoreBMais::exibirNo(long offset, int nivel) {
    if (offset == -1) return;
    No no = carregarNo(offset);
    string indent(nivel*2, ' ');
    cout << indent << "[Nó offset=" << offset << "] Folha:" << no.folha
         << " qtd_chaves:" << no.qtd_chaves << " proximo:" << no.proximo << "\n";
    for (int i = 0; i < no.qtd_chaves; ++i) {
        string chave_fmt;
        if (printKey) {
            chave_fmt = printKey(no.getChave(i), metadata.tam_chave);
        } else {
            // fallback: usa conteúdo bruto até '\0'
            string raw(no.getChave(i), metadata.tam_chave);
            chave_fmt = raw; // imprime bytes brutos (para texto zero-terminado ainda funciona)
        }
        cout << indent << "  Chave:'" << chave_fmt << "'";
        if (no.folha) cout << " | DadoOffset:" << *no.getOffset(i) << "\n";
        else          cout << " | FilhoOffset:" << *no.getOffset(i) << "\n";
    }
    if (!no.folha) {
        // último ponteiro
        cout << indent << "  Filho final offset:" << *no.getOffset(no.qtd_chaves) << "\n";
        for (int i = 0; i <= no.qtd_chaves; ++i)
            exibirNo(*no.getOffset(i), nivel + 1);
    }
    cout << indent << "-----------------------------\n";
}

void ArvoreBMais::debug(const string& msg) {
    if (!debug_ativo) return;
    const string path = log_debug_arquivo.empty() ? (nome_arquivo + ".debug.txt") : log_debug_arquivo;
    ofstream out(path, ios::app); // texto
    out << msg << '\n';
}

#include "hash.h"

// mantém constantes visíveis (mesmos valores do código original)
const int TAM_BLOCO = 4096;
const int ARTIGOS_POR_BLOCO = 2;
const int NUM_BUCKETS = 2000;

// Função hash simples
static int funcaoHash(int id) {
    return id % NUM_BUCKETS;
}

void gerarHash(const char *arquivoData) {
    ifstream dataIn(arquivoData, ios::binary);
    if (!dataIn) {
        cerr << "Erro ao abrir " << arquivoData << "\n";
        return;
    }

    // Cria ou sobrescreve data_hash.dat
    fstream dataOut("data_hash.dat", ios::in | ios::out | ios::binary | ios::trunc);
    if (!dataOut) {
        cerr << "Erro ao criar data_hash.dat\n";
        dataIn.close();
        return;
    }

    // Cria index.bin (streaming)
    ofstream indexOut("index.bin", ios::binary);
    if (!indexOut) {
        cerr << "Erro ao criar index.bin\n";
        dataIn.close();
        dataOut.close();
        return;
    }

    vector<ControleBucket> controle(NUM_BUCKETS);

    Artigo a;
    while (dataIn.read(reinterpret_cast<char*>(&a), sizeof(Artigo))) {
        int b = funcaoHash(a.id);
        auto &bucket = controle[b];

        // Se bucket ainda não tem bloco, cria o primeiro
        if (bucket.offsetPrimeiro == -1) {
            dataOut.seekp(0, ios::end);
            int64_t novoOffset = static_cast<int64_t>(dataOut.tellp());

            BlocoHeader header = { -1, 0 };
            dataOut.write(reinterpret_cast<char*>(&header), sizeof(header));
            vector<char> espaco(TAM_BLOCO - static_cast<int>(sizeof(header)), 0);
            dataOut.write(espaco.data(), espaco.size());

            bucket.offsetPrimeiro = novoOffset;
            bucket.offsetUltimo = novoOffset;
            bucket.nBlocos = 1;
            bucket.registrosNoBloco = 0;
        }

        // Se bloco atual cheio, criar novo bloco de overflow
        if (bucket.registrosNoBloco == ARTIGOS_POR_BLOCO) {
            dataOut.seekp(0, ios::end);
            int64_t novoOffset = static_cast<int64_t>(dataOut.tellp());

            // Atualiza header do bloco anterior
            BlocoHeader headerAnt;
            dataOut.seekg(bucket.offsetUltimo, ios::beg);
            dataOut.read(reinterpret_cast<char*>(&headerAnt), sizeof(headerAnt));
            headerAnt.prox = novoOffset;
            dataOut.seekp(bucket.offsetUltimo, ios::beg);
            dataOut.write(reinterpret_cast<char*>(&headerAnt), sizeof(headerAnt));

            // Cria novo bloco
            BlocoHeader novoHeader = { -1, 0 };
            dataOut.seekp(novoOffset, ios::beg);
            dataOut.write(reinterpret_cast<char*>(&novoHeader), sizeof(novoHeader));
            vector<char> esp(TAM_BLOCO - static_cast<int>(sizeof(novoHeader)), 0);
            dataOut.write(esp.data(), esp.size());

            bucket.offsetUltimo = novoOffset;
            bucket.registrosNoBloco = 0;
            bucket.nBlocos++;
        }

        // Escreve artigo no bloco atual
        int64_t posArtigo = bucket.offsetUltimo + static_cast<int64_t>(sizeof(BlocoHeader))
                            + static_cast<int64_t>(bucket.registrosNoBloco) * static_cast<int64_t>(sizeof(Artigo));
        dataOut.seekp(posArtigo, ios::beg);
        dataOut.write(reinterpret_cast<char*>(&a), sizeof(Artigo));

        // Escreve também no index.bin imediatamente
        indexOut.write(reinterpret_cast<char*>(&a.id), sizeof(int));
        indexOut.write(reinterpret_cast<char*>(&b), sizeof(int));
        indexOut.write(reinterpret_cast<char*>(&posArtigo), sizeof(int64_t));

        bucket.registrosNoBloco++;
        bucket.nRegistros++;

        // Atualiza header do bloco atual
        BlocoHeader headerAtual;
        dataOut.seekg(bucket.offsetUltimo, ios::beg);
        dataOut.read(reinterpret_cast<char*>(&headerAtual), sizeof(headerAtual));
        headerAtual.nRegs = bucket.registrosNoBloco;
        dataOut.seekp(bucket.offsetUltimo, ios::beg);
        dataOut.write(reinterpret_cast<char*>(&headerAtual), sizeof(headerAtual));
    }

    dataIn.close();
    dataOut.close();
    indexOut.close();

    // Cria hash.bin (BucketInfo)
    ofstream hashOut("hash.bin", ios::binary);
    if (!hashOut) {
        cerr << "Erro ao criar hash.bin\n";
        return;
    }

    for (int i = 0; i < NUM_BUCKETS; ++i) {
        BucketInfo info;
        info.offset = controle[i].offsetPrimeiro;
        info.nBlocos = controle[i].nBlocos;
        info.nRegistros = controle[i].nRegistros;
        hashOut.write(reinterpret_cast<char*>(&info), sizeof(info));
    }
    hashOut.close();

    cout << "✅ Data hash criado com overflow, " << NUM_BUCKETS
         << " buckets, " << ARTIGOS_POR_BLOCO << " artigos por bloco.\n";
    cout << "📄 Arquivos gerados: data_hash.dat, hash.bin, index.bin\n";
}

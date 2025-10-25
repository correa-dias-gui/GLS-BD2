#include "hash.h"
#include <chrono>
#include <cstdlib>

enum LogLevel { ERROR = 0, WARN = 1, INFO = 2, DEBUG = 3 };
const LogLevel currentLevel = DEBUG;

void logMsg(LogLevel level, const string &msg) {
    if (level <= currentLevel) {
        switch (level) {
            case ERROR: cerr << "[ERROR] " << msg << endl; break;
            case WARN:  cout << "[WARN]  " << msg << endl; break;
            case INFO:  cout << "[INFO]  " << msg << endl; break;
            case DEBUG: cout << "[DEBUG] " << msg << endl; break;
        }
    }
}

const int TAM_BLOCO_HASH = 4096;
const int ARTIGOS_POR_BLOCO = 2;
const int NUM_BUCKETS = 2000;

//Funcao hash
static int funcaoHash(int id) {
    return id % NUM_BUCKETS;
}

void gerarHash(const char *arquivoData) {
    logMsg(INFO, "================ INÍCIO DA INDEXAÇÃO ================");
    logMsg(INFO, string("Arquivo de entrada: ") + arquivoData);
    logMsg(INFO, "Número de buckets: " + to_string(NUM_BUCKETS));
    logMsg(INFO, "Artigos por bloco: " + to_string(ARTIGOS_POR_BLOCO));
    logMsg(INFO, "Tamanho do bloco: " + to_string(TAM_BLOCO_HASH) + " bytes");

    auto inicio = chrono::high_resolution_clock::now();

    ifstream dataIn(arquivoData, ios::binary);
    if (!dataIn) {
        logMsg(ERROR, string("Erro ao abrir arquivo de dados: ") + arquivoData);
        return;
    }

    //Cria ou sobrescreve data_hash.dat
    fstream dataOut("data/data_hash.dat", ios::in | ios::out | ios::binary | ios::trunc);
    if (!dataOut) {
        logMsg(ERROR, "Erro ao criar data_hash.dat");
        return;
    }

    //Cria index.bin
    ofstream indexOut("data/index.bin", ios::binary);
    if (!indexOut) {
        logMsg(ERROR, "Erro ao criar index.bin");
        return;
    }

    vector<ControleBucket> controle(NUM_BUCKETS);

    Artigo a;
    long totalArtigos = 0;
    long totalBlocos = 0;

    //Loop de insercao
    while (dataIn.read(reinterpret_cast<char*>(&a), sizeof(Artigo))) {
        totalArtigos++;
        int b = funcaoHash(a.id);
        auto &bucket = controle[b];

        //debug a cada 100000 artigos
        if (totalArtigos % 100000 == 0)
            logMsg(DEBUG, "Processando artigo #" + to_string(totalArtigos) +
                          " ID=" + to_string(a.id) + " no bucket " + to_string(b));

        //Se o bucket ainda nao tem bloco, cria o primeiro
        if (bucket.offsetPrimeiro == -1) {
            dataOut.seekp(0, ios::end);
            int64_t novoOffset = static_cast<int64_t>(dataOut.tellp());

            BlocoHeader header = { -1, 0 };
            dataOut.write(reinterpret_cast<char*>(&header), sizeof(header));
            vector<char> espaco(TAM_BLOCO_HASH - static_cast<int>(sizeof(header)), 0);
            dataOut.write(espaco.data(), espaco.size());

            bucket.offsetPrimeiro = novoOffset;
            bucket.offsetUltimo = novoOffset;
            bucket.nBlocos = 1;
            bucket.registrosNoBloco = 0;
            totalBlocos++;

            if (totalArtigos % 100000 == 0)
                logMsg(DEBUG, "Criado novo bloco inicial no offset " + to_string(novoOffset));
        }

        //Se bloco atual esta cheio, cria um novo bloco de overflow
        if (bucket.registrosNoBloco == ARTIGOS_POR_BLOCO) {
            dataOut.seekp(0, ios::end);
            int64_t novoOffset = static_cast<int64_t>(dataOut.tellp());

            BlocoHeader headerAnt;
            dataOut.seekg(bucket.offsetUltimo, ios::beg);
            dataOut.read(reinterpret_cast<char*>(&headerAnt), sizeof(headerAnt));
            headerAnt.prox = novoOffset;
            dataOut.seekp(bucket.offsetUltimo, ios::beg);
            dataOut.write(reinterpret_cast<char*>(&headerAnt), sizeof(headerAnt));

            BlocoHeader novoHeader = { -1, 0 };
            dataOut.seekp(novoOffset, ios::beg);
            dataOut.write(reinterpret_cast<char*>(&novoHeader), sizeof(novoHeader));
            vector<char> esp(TAM_BLOCO_HASH - static_cast<int>(sizeof(novoHeader)), 0);
            dataOut.write(esp.data(), esp.size());

            bucket.offsetUltimo = novoOffset;
            bucket.registrosNoBloco = 0;
            bucket.nBlocos++;
            totalBlocos++;

            if (totalArtigos % 100000 == 0)
                logMsg(DEBUG, "Bloco cheio, criado overflow no offset " + to_string(novoOffset));
        }

        //Escreve o artigo no bloco atual
        int64_t posArtigo = bucket.offsetUltimo + static_cast<int64_t>(sizeof(BlocoHeader))
                            + static_cast<int64_t>(bucket.registrosNoBloco) * static_cast<int64_t>(sizeof(Artigo));
        dataOut.seekp(posArtigo, ios::beg);
        dataOut.write(reinterpret_cast<char*>(&a), sizeof(Artigo));

        //Escreve no index.bin
        indexOut.write(reinterpret_cast<char*>(&a.id), sizeof(int));
        indexOut.write(reinterpret_cast<char*>(&b), sizeof(int));
        indexOut.write(reinterpret_cast<char*>(&posArtigo), sizeof(int64_t));

        bucket.registrosNoBloco++;
        bucket.nRegistros++;

        //Atualiza o header do bloco atual
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

    ofstream hashOut("data/hash.bin", ios::binary);
    if (!hashOut) {
        logMsg(ERROR, "Erro ao criar hash.bin");
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

    auto fim = chrono::high_resolution_clock::now();
    auto duracao = chrono::duration_cast<chrono::milliseconds>(fim - inicio).count();

    logMsg(INFO, "================ FIM DA INDEXAÇÃO ================");
    logMsg(INFO, "Total de artigos processados: " + to_string(totalArtigos));
    logMsg(INFO, "Total de blocos criados: " + to_string(totalBlocos));
    logMsg(INFO, "Tempo total de execução: " + to_string(duracao) + " ms");
    logMsg(INFO, "Arquivos gerados:");
    logMsg(INFO, "  • data_hash.dat");
    logMsg(INFO, "  • hash.bin");
    logMsg(INFO, "  • index.bin");
    logMsg(INFO, "==================================================");
}

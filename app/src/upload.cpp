#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include "hash.h"
#include "../include/BTplus_mem.h"

using namespace std;

//Separa as linhas do csv em campos
vector<string> splitCSVLine(const string &linha) {
    vector<string> campos;
    string atual;
    bool dentroAspas = false;

    for (char c : linha) {
        if (c == '"') {
            dentroAspas = !dentroAspas;
        } else if (c == ';' && !dentroAspas) {
            campos.push_back(atual);
            atual.clear();
        } else {
            atual += c;
        }
    }
    campos.push_back(atual);

    //Remove aspas externas
    for (auto &campo : campos) {
        if (!campo.empty() && campo.front() == '"') campo.erase(0, 1);
        if (!campo.empty() && campo.back() == '"') campo.pop_back();
    }

    return campos;
}

//Preenche a struct Artigo a partir do vetor de campos csv
bool preencherArtigo(Artigo &a, const vector<string> &campos) {
    if (campos.size() != 7) return false;

    try {
        memset(&a, 0, sizeof(Artigo));

        a.id = stoi(campos[0]);
        strncpy(a.titulo, campos[1].c_str(), sizeof(a.titulo)-1);

        a.ano = stoi(campos[2]);
        strncpy(a.autores, campos[3].c_str(), sizeof(a.autores)-1);

        a.citacoes = stoi(campos[4]);
        strncpy(a.dataAtualizacao, campos[5].c_str(), sizeof(a.dataAtualizacao)-1);

        strncpy(a.snippet, campos[6].c_str(), sizeof(a.snippet)-1);

        return true;
    } catch (...) {
        return false;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " <arquivo_csv>\n";
        return 1;
    }

    ifstream arquivoCSV(argv[1]);
    if (!arquivoCSV.is_open()) {
        cerr << "Erro ao abrir arquivo CSV: " << argv[1] << "\n";
        return 1;
    }

    FILE *dataBin = fopen("data/data.bin", "wb");
    if (!dataBin) {
        cerr << "Erro ao criar data.bin\n";
        return 1;
    }

    string linha;
    long total = 0;
    while (getline(arquivoCSV, linha)) {
        if (linha.empty()) continue;

        vector<string> campos = splitCSVLine(linha);
        Artigo a;
        if (!preencherArtigo(a, campos)) {
            cerr << "Linha ignorada (malformada): " << linha << "\n";
            continue;
        }

        fwrite(&a, sizeof(Artigo), 1, dataBin);
        total++;

        if (total % 100000 == 0)
            cout << total << " registros processados...\n";
    }

    fclose(dataBin);
    arquivoCSV.close();
    cout << "\nUpload concluído e arquivo gerado: data.bin (" << sizeof(Artigo) << " bytes por registro)\n\n";

    //Chama o hash para gerar arquivos de buckets
    gerarHash("data/data.bin");

    // ==== Construção dos índices B+ (id e titulo) ====
    // Callbacks para ID (int) big-endian
    auto cmp_id = [](const void* a, const void* b){
        const unsigned char* pa = (const unsigned char*)a;
        const unsigned char* pb = (const unsigned char*)b;
        int r = memcmp(pa, pb, 4); return r < 0 ? -1 : (r>0?1:0);
    };
    auto serialize_id = [](const void* chave){
        int v = *reinterpret_cast<const int*>(chave);
        unsigned char buf[4];
        buf[0]=(v>>24)&0xFF; buf[1]=(v>>16)&0xFF; buf[2]=(v>>8)&0xFF; buf[3]=v&0xFF;
        return string((char*)buf,4);
    };
    auto deserialize_id = [](const string& s)->void*{
        if (s.size()!=4) return nullptr; const unsigned char* p=(const unsigned char*)s.data();
        int* v = new int((int(p[0])<<24)|(int(p[1])<<16)|(int(p[2])<<8)|int(p[3])); return v;
    };
    auto print_id = [](const char* raw, int tam){
        if (tam!=4) return string("<inv>"); const unsigned char* p=(const unsigned char*)raw;
        int v=(int(p[0])<<24)|(int(p[1])<<16)|(int(p[2])<<8)|int(p[3]); return to_string(v);
    };

    // Callbacks para titulo (string fixa 300 bytes)
    const int TAM_TITULO = 300; // usada para índice
    auto cmp_titulo = [](const void* a, const void* b){ return strncmp((const char*)a,(const char*)b,300); };
    auto serialize_titulo = [](const void* chave){ return string((const char*)chave); }; // já null-terminated ou menor
    auto deserialize_titulo = [](const string& s)->void*{
        char* buf = new char[TAM_TITULO]; memset(buf,0,TAM_TITULO); strncpy(buf,s.c_str(),TAM_TITULO-1); return buf;
    };
    auto print_titulo = [](const char* raw, int tam){ return string(raw, strnlen(raw,tam)); };

    // Instâncias das árvores
    ArvoreBMais indice_id("indice_id.bin", 4, cmp_id, serialize_id, deserialize_id, print_id);
    ArvoreBMais indice_titulo("indice_titulo.bin", TAM_TITULO, cmp_titulo, serialize_titulo, deserialize_titulo, print_titulo);

    // Abrir hash.bin para obter offsets dos buckets
    ifstream hashIn("hash.bin", ios::binary);
    if (!hashIn){ cerr << "Erro ao abrir hash.bin para indexar" << endl; return 1; }
    vector<BucketInfo> buckets(NUM_BUCKETS);
    for (int i=0;i<NUM_BUCKETS;i++) hashIn.read(reinterpret_cast<char*>(&buckets[i]), sizeof(BucketInfo));
    hashIn.close();

    // Abrir data_hash.dat para percorrer blocos
    fstream dataHash("data_hash.dat", ios::in | ios::out | ios::binary);
    if (!dataHash){ cerr << "Erro ao abrir data_hash.dat" << endl; return 1; }

    long registrosInseridos = 0;
    for (int b=0;b<NUM_BUCKETS;b++) {
        int64_t blocoOffset = buckets[b].offset;
        while (blocoOffset != -1) {
            // Ler header
            BlocoHeader header; dataHash.seekg(blocoOffset, ios::beg);
            dataHash.read(reinterpret_cast<char*>(&header), sizeof(header));
            // Ler registros válidos
            for (int r=0; r<header.nRegs; r++) {
                int64_t posArtigo = blocoOffset + sizeof(BlocoHeader) + r * sizeof(Artigo);
                dataHash.seekg(posArtigo, ios::beg);
                Artigo art; dataHash.read(reinterpret_cast<char*>(&art), sizeof(Artigo));
                // Inserir ID
                indice_id.inserir(&art.id, posArtigo);
                // Preparar título para índice (300 bytes fixos)
                char tituloIdx[TAM_TITULO]; memset(tituloIdx,0,TAM_TITULO);
                strncpy(tituloIdx, art.titulo, TAM_TITULO-1);
                indice_titulo.inserir(tituloIdx, posArtigo);
                registrosInseridos++;
            }
            blocoOffset = header.prox;
        }
    }
    dataHash.close();
    cout << "Índices B+ construídos: " << registrosInseridos << " registros indexados." << endl;
    //indice_id.exibir();
    //indice_titulo.exibir();
    return 0;
}

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include "hash.h"

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

    FILE *dataBin = fopen("data.bin", "wb");
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

    //Chama o hash
    gerarHash("data.bin");
    return 0;
}

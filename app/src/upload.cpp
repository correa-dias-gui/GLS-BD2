#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring> // strncpy, memset

#pragma pack(push, 1)
struct Artigo {
    int id;
    char titulo[301];
    int ano;
    char autores[151];
    int citacoes;
    char dataAtualizacao[21];
    char snippet[1025];
};
#pragma pack(pop)

std::vector<std::string> splitCSVLine(const std::string &linha) {
    std::vector<std::string> campos;
    std::string atual;
    bool dentroAspas = false;

    for (size_t i = 0; i < linha.size(); ++i) {
        char c = linha[i];
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

    // Remove aspas externas
    for (auto &campo : campos) {
        if (!campo.empty() && campo.front() == '"') campo.erase(0, 1);
        if (!campo.empty() && campo.back() == '"') campo.pop_back();
    }

    return campos;
}

bool preencherArtigo(Artigo &a, const std::vector<std::string> &campos) {
    if (campos.size() != 7) return false;

    try {
        // Inicializa todo o struct com zeros
        memset(&a, 0, sizeof(Artigo));

        a.id = std::stoi(campos[0]);
        strncpy(a.titulo, campos[1].c_str(), sizeof(a.titulo) - 1);

        a.ano = std::stoi(campos[2]);
        strncpy(a.autores, campos[3].c_str(), sizeof(a.autores) - 1);

        a.citacoes = std::stoi(campos[4]);
        strncpy(a.dataAtualizacao, campos[5].c_str(), sizeof(a.dataAtualizacao) - 1);

        strncpy(a.snippet, campos[6].c_str(), sizeof(a.snippet) - 1);

        return true;
    } catch (...) {
        return false;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <arquivo_csv>\n";
        return 1;
    }

    std::ifstream arquivoCSV(argv[1]);
    if (!arquivoCSV.is_open()) {
        std::cerr << "Erro ao abrir arquivo CSV: " << argv[1] << "\n";
        return 1;
    }

    FILE *arquivoBin = fopen("data.bin", "wb");
    if (!arquivoBin) {
        std::cerr << "Erro ao criar data.bin\n";
        return 1;
    }

    std::string linha;
    long total = 0;

    while (std::getline(arquivoCSV, linha)) {
        if (linha.empty()) continue;

        auto campos = splitCSVLine(linha);
        Artigo a;

        if (!preencherArtigo(a, campos)) {
            std::cerr << "Linha ignorada (malformada): " << linha << "\n";
            continue;
        }

        fwrite(&a, sizeof(Artigo), 1, arquivoBin);
        total++;

        if (total <= 3) {
            std::cout << "Lido artigo ID=" << a.id << " | Título=" << a.titulo << "\n";
        }

        if (total % 10000 == 0)
            std::cout << total << " registros processados...\n";
    }

    fclose(arquivoBin);
    arquivoCSV.close();

    std::cout << "✅ Upload concluído. Total de registros: " << total << "\n";
    std::cout << "Arquivo gerado: data.bin (" << sizeof(Artigo) << " bytes por registro)\n";

    return 0;
}

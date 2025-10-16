#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

const int M = 2; // ordem da árvore (máximo de 2M chaves e 2M+1 offsets)

struct No {
    bool folha;
    int qtd_chaves;
    vector<string> chaves;
    vector<long> offsets; // offsets dos filhos ou registros
    long proximo;         // encadeamento de folhas

    No(bool eh_folha = true) {
        folha = eh_folha;
        qtd_chaves = 0;
        chaves.resize(2 * M);
        offsets.resize(2 * M + 1, -1);
        proximo = -1;
    }
};

class B_mem {
private:
    fstream arquivo;
    string nome_arquivo;
    long raiz_offset;

public:
    B_mem(string nome) {
        nome_arquivo = nome;
        raiz_offset = -1;
        abrirArquivo();
    }

    ~B_mem() {
        arquivo.close();
    }

    void abrirArquivo() {
        arquivo.open(nome_arquivo, ios::in | ios::out | ios::app);
        if (!arquivo.is_open()) {
            ofstream criar(nome_arquivo);
            criar.close();
            arquivo.open(nome_arquivo, ios::in | ios::out | ios::app);
        }
    }

    // Serializa um nó e grava no final do arquivo texto
    long salvarNo(No &no) {
        arquivo.clear();
        arquivo.seekp(0, ios::end);
        long pos = arquivo.tellp();
        arquivo << (no.folha ? "F" : "I") << ";"
                << no.qtd_chaves << ";";
        for (int i = 0; i < 2 * M; i++) {
            arquivo << no.chaves[i] << ";";
        }
        for (int i = 0; i < 2 * M + 1; i++) {
            arquivo << no.offsets[i] << ";";
        }
        arquivo << no.proximo << "\n";
        arquivo.flush();
        return pos;
    }

    // Lê um nó a partir de um offset (posição no arquivo)
    No carregarNo(long pos) {
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

        for (int i = 0; i < 2 * M; i++) {
            getline(ss, no.chaves[i], ';');
        }
        for (int i = 0; i < 2 * M + 1; i++) {
            getline(ss, temp, ';');
            no.offsets[i] = stol(temp);
        }
        getline(ss, temp, ';');
        no.proximo = stol(temp);
        return no;
    }

    // Inserção simplificada (apenas em nó folha)
    void inserir(string chave, long offset_dado) {
        if (raiz_offset == -1) {
            No novo(true);
            novo.chaves[0] = chave;
            novo.offsets[0] = offset_dado;
            novo.qtd_chaves = 1;
            raiz_offset = salvarNo(novo);
        } else {
            No raiz = carregarNo(raiz_offset);
            if (raiz.qtd_chaves < 2 * M) {
                raiz.chaves[raiz.qtd_chaves] = chave;
                raiz.offsets[raiz.qtd_chaves] = offset_dado;
                raiz.qtd_chaves++;
                raiz_offset = salvarNo(raiz);
            } else {
                cout << "Nó cheio (divisão não implementada nesta versão)." << endl;
            }
        }
    }

    // Busca simples (varre o arquivo)
    long buscar(string chave) {
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

    void exibir() {
        arquivo.clear();
        arquivo.seekg(0);
        string linha;
        cout << "=== Conteúdo da árvore (" << nome_arquivo << ") ===" << endl;
        while (getline(arquivo, linha))
            cout << linha << endl;
    }
};

int main() {
    B_mem arv("indice.txt");

    arv.inserir("ArtigoA", 100);
    arv.inserir("ArtigoB", 200);
    arv.inserir("ArtigoC", 300);

    arv.exibir();

    long pos = arv.buscar("ArtigoB");
    if (pos != -1)
        cout << "Chave encontrada no offset lógico: " << pos << endl;
    else
        cout << "Chave não encontrada." << endl;

    return 0;
}

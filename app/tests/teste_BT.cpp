#include "../include/BTplus_mem.h"


int main() {
    ArvoreBMais arv("data/indice.txt");
    arv.inserir("ArtigoA", 100);
    arv.inserir("ArtigoB", 200);
    arv.inserir("ArtigoC", 300);
    arv.inserir("ArtigoD", 400);
    arv.inserir("ArtigoE", 500);
    arv.inserir("ArtigoE", 900);
    arv.exibir();
    return 0;
}

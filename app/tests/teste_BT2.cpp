#include "../include/BTplus_mem.h"
#include <cstring>
#include <string>
#include <iostream>
using namespace std;

static int g_tam_chave = 4;

// Comparação binária big-endian
int cmp_int_bin(const void* a, const void* b) {
    const unsigned char* pa = reinterpret_cast<const unsigned char*>(a);
    const unsigned char* pb = reinterpret_cast<const unsigned char*>(b);
    int r = memcmp(pa, pb, 4);
    if (r < 0) return -1;
    if (r > 0) return 1;
    return 0;
}

// Serializa int em big-endian (ordem lexicográfica == ordem numérica)
string serialize_int_bin(const void* chave) {
    int v = *reinterpret_cast<const int*>(chave);
    unsigned char buf[4];
    buf[0] = (v >> 24) & 0xFF;
    buf[1] = (v >> 16) & 0xFF;
    buf[2] = (v >> 8) & 0xFF;
    buf[3] = v & 0xFF;
    return string(reinterpret_cast<char*>(buf), 4);
}

// Desserializa big-endian
void* deserialize_int_bin(const string& s) {
    int* v = new int;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.data());
    *v = (int(p[0]) << 24) | (int(p[1]) << 16) | (int(p[2]) << 8) | int(p[3]);
    return v;
}

string print_int_be(const char* buf, int tam) {
    if (tam != 4) return "<n>";
    const unsigned char* p = reinterpret_cast<const unsigned char*>(buf);
    int v = (int(p[0]) << 24) | (int(p[1]) << 16) | (int(p[2]) << 8) | int(p[3]);
    return to_string(v);
}

int main() {
    remove("data/indice_int.bin");
    ArvoreBMais arv("data/indice_int.bin", g_tam_chave, cmp_int_bin, serialize_int_bin, deserialize_int_bin, print_int_be);

    int chaves[]  = {10,20,30,40,50,60,70,80,90,100,25,35,45,55};
    long offs[]   = {100,200,300,400,500,600,700,800,900,1000,250,350,450,550};

    for (int i = 0; i < 14; ++i)
        arv.inserir(&chaves[i], offs[i]);

    int busca = 50;
    long pos = arv.buscar(&busca);
    cout << "Busca 50 -> " << pos << endl;

    arv.exibir();
    return 0;
}
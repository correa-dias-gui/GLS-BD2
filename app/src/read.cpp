void ArvoreBMais::exibirNo(long offset, int nivel) {
    if (offset == -1) return;
    No no = carregarNo(offset);
    string indent(nivel*2, ' ');
    cout << indent << "[Nó offset=" << offset << "] Folha:" << no.folha
         << " qtd_chaves:" << no.qtd_chaves << " proximo:" << no.proximo << "\n";
    for (int i = 0; i < no.qtd_chaves; ++i) {
        // Usa deserialize -> serialize para obter representação textual sem buscar '\0'
        string raw(no.getChave(i), metadata.tam_chave);
        void* obj = deserialize(raw);              // reconstrói a chave tipada
        string chave_fmt = serialize(obj);         // formata a chave
        // liberar se deserialize alocou (assumindo alocação dinâmica para inteiros/strings)
        // para string retornando ponteiro interno não liberar; ajuste conforme sua deser
        if (metadata.tam_chave == 4) delete reinterpret_cast<int*>(obj);
        else delete[] reinterpret_cast<char*>(obj);

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
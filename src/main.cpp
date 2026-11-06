/*
 * Trabalho Grau B - Processamento Grafico: Fundamentos
 * Guilherme Menna Barreto
 * Editor de fotos e frames da webcam em C++ / OpenCV.
 * Base de organizacao: Listas 2 e 3 e Zelda 2D Game.
 */
#include "Aplicativo.h"
#include <iostream>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#ifdef _WIN32
int wmain(int argc, wchar_t **argv)
#else
int main(int argc, char **argv)
#endif
{
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    try
    {
        std::vector<std::string> argumentos;
        for (int i = 0; i < argc; ++i)
            argumentos.push_back(std::filesystem::path(argv[i]).u8string());
        std::filesystem::path foto;
        int camera = 0;
        for (int i = 1; i < argc; ++i)
        {
            const std::string &argumento = argumentos[i];
            if (argumento == "--help" || argumento == "-h")
            {
                std::cout << "Uso: grau_b [caminho_da_foto] [--camera indice]\n"
                    << "Sem foto inicial, use F na janela para carregar uma. V liga a webcam.\n";
                return 0;
            }
            if (argumento == "--camera")
            {
                if (++i >= argc) throw std::invalid_argument("Faltou o indice da camera.");
                std::size_t lidos = 0;
                camera = std::stoi(argumentos[i], &lidos);
                if (lidos != argumentos[i].size() || camera < 0)
                    throw std::invalid_argument("Indice da camera deve ser inteiro nao negativo.");
            }
            else if (foto.empty()) foto = std::filesystem::u8path(argumento);
            else throw std::invalid_argument("Argumento inesperado: " + argumento);
        }
        // Recursos e saidas independem do diretorio de onde o programa foi aberto.
#ifdef _WIN32
        wchar_t caminho[32768];
        const DWORD tamanho = GetModuleFileNameW(nullptr, caminho, 32768);
        if (tamanho == 0 || tamanho == 32768) throw std::runtime_error("Falha ao localizar o executavel.");
        const auto pasta = std::filesystem::path(caminho).parent_path();
#else
        const auto pasta = std::filesystem::absolute(argv[0]).parent_path();
#endif
        Aplicativo aplicativo(pasta, camera);
        return aplicativo.executar(foto);
    }
    catch (const std::exception &erro)
    {
        std::cerr << "Erro: " << erro.what() << '\n';
        return 1;
    }
}

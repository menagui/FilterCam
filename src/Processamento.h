#ifndef PROCESSAMENTO_H
#define PROCESSAMENTO_H

#include <opencv2/core.hpp>
#include <filesystem>
#include <string>
#include <vector>

// OpenCV armazena os canais na ordem BGR. A interface apresenta RGB.
enum class Canal { RGB = 0, R = 1, G = 2, B = 3 };
enum class Operacao { Nenhuma = 0, Adicao = 1, Subtracao = 2, Blending = 3 };

struct DescricaoFiltro
{
    const char *nome;
    const char *descricao;
    const char *parametro;
};

struct Efeito
{
    int filtro = 0;
    Operacao operacao = Operacao::Nenhuma;
    Canal canal = Canal::RGB;
    int parametro = 128;
};

struct StickerPosicionado
{
    int indice;
    cv::Point centro;
    int largura;
};

const std::vector<DescricaoFiltro> &listaFiltros();
std::string nomeCanal(Canal canal);
std::string nomeOperacao(Operacao operacao);
std::string nomeEfeito(const Efeito &efeito);
cv::Mat lerImagem(const std::filesystem::path &caminho, int flags);
void salvarPNG(const std::filesystem::path &caminho, const cv::Mat &imagem);
cv::Mat selecionarCanal(const cv::Mat &imagem, Canal canal);
cv::Mat aplicarFiltro(const cv::Mat &origem, int filtro, int parametro);
cv::Mat aplicarOperacao(const cv::Mat &a, const cv::Mat &b, Operacao operacao, int parametro);
// Sempre usa a origem; o resultado anterior nunca entra no processamento.
cv::Mat processar(const cv::Mat &origem, const cv::Mat &segunda, const Efeito &efeito);
void sobreporSticker(cv::Mat &destino, const cv::Mat &stickerBGRA, cv::Point centro, int largura);
cv::Mat comporResultado(const cv::Mat &origem, const cv::Mat &segunda, const Efeito &efeito,
    const std::vector<cv::Mat> &stickers, const std::vector<StickerPosicionado> &posicoes);

#endif

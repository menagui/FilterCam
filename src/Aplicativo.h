#ifndef APLICATIVO_H
#define APLICATIVO_H

#include "Processamento.h"
#include <opencv2/videoio.hpp>

// Separacao simples de responsabilidades, como Game.h no Zelda:
// aqui ficam a entrada, o estado da aplicacao e o loop da janela.
class Aplicativo
{
public:
    explicit Aplicativo(std::filesystem::path pastaExecutavel, int camera = 0);
    int executar(const std::filesystem::path &fotoInicial = {});

private:
    static constexpr const char *JANELA = "Grau B - Fotos e webcam";
    static constexpr int LARGURA = 1000;
    static constexpr int ALTURA_FOTO = 480;
    static constexpr int CABECALHO = 80;
    static constexpr int RODAPE = 130;

    std::filesystem::path pasta;
    int indiceCamera;
    cv::VideoCapture camera;
    bool modoVideo = false;
    bool atualizar = true;
    cv::Mat original, segundaImagem, resultado;
    Efeito selecionado, aplicado;
    int ultimoFiltro = 0, ultimaOperacao = 0, ultimoParametro = 128, ultimoCanal = 0;
    int stickerSelecionado = -1;
    std::vector<cv::Mat> stickers;
    std::vector<StickerPosicionado> posicoes;
    cv::Rect areaFoto;
    std::string mensagem = "F: carregar foto | V: iniciar webcam. Veja os controles no console.";

    void carregarStickers();
    void carregarFoto(const std::filesystem::path &caminho);
    void alternarWebcam();
    void resetar();
    void salvar();
    void lerSelecao();
    void descreverSelecao() const;
    void aplicarSelecao();
    void desenhar();
    void clicar(int evento, int x, int y);
    static void mouse_callback(int evento, int x, int y, int flags, void *dados);
    static std::filesystem::path pedirCaminho(const std::string &rotulo);
    static void ajuda();
};

#endif

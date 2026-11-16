#ifndef APLICATIVO_H
#define APLICATIVO_H

#include "Processamento.h"
#include <opencv2/videoio.hpp>

// Separacao simples de responsabilidades, como Game.h no Zelda:
// aqui ficam a entrada, o estado da aplicacao e o loop da janela.
// A interface (branco e azul-marinho) e desenhada manualmente num cv::Mat,
// sem trackbars nativas do HighGUI.
class Aplicativo
{
public:
    explicit Aplicativo(std::filesystem::path pastaExecutavel, int camera = 0);
    int executar(const std::filesystem::path &fotoInicial = {});

private:
    static constexpr const char *JANELA = "FilterCam - Grau B";
    static constexpr int LARGURA = 1100;
    static constexpr int ALTURA = 700;

    std::filesystem::path pasta;
    int indiceCamera;
    cv::VideoCapture camera;
    bool modoVideo = false;
    bool atualizar = true;
    cv::Mat original, segundaImagem, resultado;
    Efeito selecionado, aplicado;
    int ultimoFiltro = 0, ultimaOperacao = 0, ultimoParametro = 128, ultimoCanal = 0;

    // Valores dos controles desenhados na tela (substituem as trackbars).
    int uiFiltro = 0, uiCanal = 0, uiParametro = 128, uiOperacao = 0;
    int arrastando = -1;   // -1 nenhum, 0 slider de filtro, 1 slider de parametro
    int teclaPendente = 0; // acao pedida por clique em botao, tratada no loop

    int stickerSelecionado = -1;
    std::vector<cv::Mat> stickers;
    std::vector<StickerPosicionado> posicoes;
    cv::Rect areaFoto;
    std::string mensagem = "F: carregar foto | V: iniciar webcam.";

    void carregarStickers();
    void carregarFoto(const std::filesystem::path &caminho);
    void alternarWebcam();
    void resetar();
    void salvar();
    void lerSelecao();
    void descreverSelecao() const;
    void aplicarSelecao();
    void desenhar();
    bool tratarTecla(int tecla); // retorna true para sair
    void clicar(int evento, int x, int y, int flags);
    static void mouse_callback(int evento, int x, int y, int flags, void *dados);
    static std::filesystem::path pedirCaminho(const std::string &rotulo);
    static void ajuda();
};

#endif

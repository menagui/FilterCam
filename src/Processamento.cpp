#include "Processamento.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>

const std::vector<DescricaoFiltro> &listaFiltros()
{
    static const std::vector<DescricaoFiltro> filtros = {
        {"Original", "Exibe a imagem sem filtro.", "Sem parametro."},
        {"Tons de cinza", "Converte BGR em luminancia: 0.114 B + 0.587 G + 0.299 R.", "Sem parametro."},
        {"Negativo", "Inverte a intensidade de cada pixel: 255 - pixel.", "Sem parametro."},
        {"Box Blur", "Suaviza pela media dos pixels em uma vizinhanca quadrada.", "P: kernel impar de 1 a 31; k = 2*(P/17)+1."},
        {"Gaussian Blur", "Suaviza com pesos gaussianos, maiores no centro do kernel.", "P: kernel impar de 1 a 31; sigma automatico."},
        {"Mediana", "Substitui cada pixel pela mediana da vizinhanca, reduzindo ruido impulsivo.", "P: kernel impar de 1 a 31; k = 2*(P/17)+1."},
        {"Sharpen", "Realca detalhes: centro 1+4a, quatro vizinhos -a; soma dos pesos = 1.", "P: intensidade a = 2*P/255 (0 a 2)."},
        {"Laplaciano", "Destaca variacoes de intensidade pela segunda derivada; mostra modulo.", "P: ganho = 1 + 3*P/255; kernel 3x3."},
        {"Sobel", "Destaca bordas pela magnitude dos gradientes horizontal e vertical.", "P: ganho = 1 + 3*P/255; kernel 3x3."},
        {"Limiarizacao", "Transforma intensidades acima do limiar em branco e as demais em preto.", "P: limiar de 0 a 255; usa luminancia ou o canal escolhido."},
        {"Sepia", "Combina os canais por uma matriz de tons quentes, com saturacao em 255.", "Sem parametro. Em canal isolado, preserva a visualizacao monocromatica."}
    };
    return filtros;
}

std::string nomeCanal(Canal canal)
{
    switch (canal)
    {
        case Canal::R: return "R";
        case Canal::G: return "G";
        case Canal::B: return "B";
        default: return "RGB";
    }
}

std::string nomeOperacao(Operacao operacao)
{
    switch (operacao)
    {
        case Operacao::Adicao: return "Adicao saturada";
        case Operacao::Subtracao: return "Subtracao ponderada";
        case Operacao::Blending: return "Blending";
        default: return "Nenhuma";
    }
}

std::string nomeEfeito(const Efeito &efeito)
{
    return efeito.operacao == Operacao::Nenhuma
        ? listaFiltros().at(efeito.filtro).nome : nomeOperacao(efeito.operacao);
}

cv::Mat lerImagem(const std::filesystem::path &caminho, int flags)
{
    // Streams com filesystem::path aceitam caminhos Windows com acentos.
    std::ifstream arquivo(caminho, std::ios::binary);
    if (!arquivo) throw std::runtime_error("Nao foi possivel abrir: " + caminho.u8string());
    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(arquivo)), {});
    if (bytes.empty()) throw std::runtime_error("Arquivo vazio: " + caminho.u8string());
    cv::Mat imagem = cv::imdecode(bytes, flags);
    if (imagem.empty()) throw std::runtime_error("Formato de imagem invalido: " + caminho.u8string());
    return imagem;
}

void salvarPNG(const std::filesystem::path &caminho, const cv::Mat &imagem)
{
    if (imagem.empty()) throw std::runtime_error("Nao ha imagem para salvar.");
    std::vector<unsigned char> bytes;
    if (!cv::imencode(".png", imagem, bytes)) throw std::runtime_error("Falha ao codificar PNG.");
    if (!caminho.parent_path().empty()) std::filesystem::create_directories(caminho.parent_path());
    std::ofstream arquivo(caminho, std::ios::binary);
    arquivo.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!arquivo) throw std::runtime_error("Falha ao salvar: " + caminho.u8string());
}

cv::Mat selecionarCanal(const cv::Mat &imagem, Canal canal)
{
    if (imagem.empty() || imagem.type() != CV_8UC3)
        throw std::invalid_argument("A origem deve ser uma imagem BGR de 8 bits.");
    if (canal == Canal::RGB) return imagem.clone();
    cv::Mat resultado;
    const int indice = canal == Canal::R ? 2 : canal == Canal::G ? 1 : 0;
    cv::extractChannel(imagem, resultado, indice);
    return resultado;
}

static cv::Mat cinza(const cv::Mat &imagem)
{
    if (imagem.channels() == 1) return imagem.clone();
    cv::Mat resultado;
    cv::cvtColor(imagem, resultado, cv::COLOR_BGR2GRAY);
    return resultado;
}

cv::Mat aplicarFiltro(const cv::Mat &origem, int filtro, int parametro)
{
    if (origem.empty()) throw std::invalid_argument("Imagem vazia.");
    if (filtro < 0 || filtro >= static_cast<int>(listaFiltros().size()))
        throw std::invalid_argument("Filtro invalido.");
    parametro = std::clamp(parametro, 0, 255);
    const int kernel = 2 * (parametro / 17) + 1;
    const double ganho = 1.0 + 3.0 * parametro / 255.0;
    cv::Mat resultado;
    switch (filtro)
    {
        case 0: return origem.clone();
        case 1: return cinza(origem);
        case 2: cv::bitwise_not(origem, resultado); break;
        case 3: cv::blur(origem, resultado, cv::Size(kernel, kernel)); break;
        case 4: cv::GaussianBlur(origem, resultado, cv::Size(kernel, kernel), 0.0); break;
        case 5:
            if (kernel == 1) return origem.clone();
            cv::medianBlur(origem, resultado, kernel);
            break;
        case 6:
        {
            const float a = 2.0f * parametro / 255.0f;
            cv::Mat mascara = (cv::Mat_<float>(3, 3) << 0, -a, 0, -a, 1 + 4*a, -a, 0, -a, 0);
            cv::filter2D(origem, resultado, -1, mascara);
            break;
        }
        case 7:
        {
            cv::Mat derivada;
            cv::Laplacian(origem, derivada, CV_32F, 3);
            cv::convertScaleAbs(derivada, resultado, ganho);
            break;
        }
        case 8:
        {
            // Sobel preserva os tres canais em RGB e um canal no modo R/G/B.
            std::vector<cv::Mat> canais, bordas;
            cv::split(origem, canais);
            for (const auto &canal : canais)
            {
                cv::Mat dx, dy, magnitude, borda;
                cv::Sobel(canal, dx, CV_32F, 1, 0, 3);
                cv::Sobel(canal, dy, CV_32F, 0, 1, 3);
                cv::magnitude(dx, dy, magnitude);
                magnitude.convertTo(borda, CV_8U, ganho);
                bordas.push_back(borda);
            }
            cv::merge(bordas, resultado);
            break;
        }
        case 9: cv::threshold(cinza(origem), resultado, parametro, 255, cv::THRESH_BINARY); break;
        case 10:
        {
            if (origem.channels() == 1) return origem.clone();
            // Linhas B, G, R; colunas B, G, R (ordem de armazenamento do OpenCV).
            const cv::Mat matriz = (cv::Mat_<float>(3, 3) <<
                0.131f, 0.534f, 0.272f,
                0.168f, 0.686f, 0.349f,
                0.189f, 0.769f, 0.393f);
            cv::transform(origem, resultado, matriz);
            break;
        }
    }
    return resultado;
}

cv::Mat aplicarOperacao(const cv::Mat &a, const cv::Mat &b, Operacao operacao, int parametro)
{
    if (a.empty() || b.empty()) throw std::invalid_argument("Carregue as duas imagens.");
    if (a.type() != b.type()) throw std::invalid_argument("As imagens devem ter o mesmo tipo.");
    cv::Mat ajustada, resultado;
    cv::resize(b, ajustada, a.size(), 0, 0, cv::INTER_LINEAR);
    const double peso = std::clamp(parametro, 0, 255) / 255.0;
    switch (operacao)
    {
        case Operacao::Adicao: cv::add(a, ajustada, resultado); break;
        case Operacao::Subtracao: cv::addWeighted(a, 1.0, ajustada, -peso, 0, resultado); break;
        case Operacao::Blending: cv::addWeighted(a, peso, ajustada, 1.0 - peso, 0, resultado); break;
        default: return a.clone();
    }
    return resultado;
}

cv::Mat processar(const cv::Mat &origem, const cv::Mat &segunda, const Efeito &efeito)
{
    cv::Mat entrada = selecionarCanal(origem, efeito.canal);
    cv::Mat resultado = efeito.operacao == Operacao::Nenhuma
        ? aplicarFiltro(entrada, efeito.filtro, efeito.parametro)
        : aplicarOperacao(entrada, selecionarCanal(segunda, efeito.canal), efeito.operacao, efeito.parametro);
    // Um canal isolado e mostrado como intensidade cinza na imagem principal.
    if (resultado.channels() == 1) cv::cvtColor(resultado, resultado, cv::COLOR_GRAY2BGR);
    return resultado;
}

void sobreporSticker(cv::Mat &destino, const cv::Mat &stickerBGRA, cv::Point centro, int largura)
{
    if (destino.type() != CV_8UC3 || stickerBGRA.type() != CV_8UC4 || largura < 1)
        throw std::invalid_argument("Sticker deve ser BGRA e destino BGR; largura positiva.");
    const int altura = std::max(1, cvRound(static_cast<double>(largura) * stickerBGRA.rows / stickerBGRA.cols));
    cv::Mat sticker;
    cv::resize(stickerBGRA, sticker, cv::Size(largura, altura), 0, 0, cv::INTER_NEAREST);
    const cv::Point inicio(centro.x - largura/2, centro.y - altura/2);
    const cv::Rect area = cv::Rect(inicio, sticker.size()) & cv::Rect(0, 0, destino.cols, destino.rows);
    for (int y = area.y; y < area.y + area.height; ++y)
    {
        auto *linha = destino.ptr<cv::Vec3b>(y);
        const auto *sobreposta = sticker.ptr<cv::Vec4b>(y - inicio.y);
        for (int x = area.x; x < area.x + area.width; ++x)
        {
            const auto pixel = sobreposta[x - inicio.x];
            const float alfa = pixel[3] / 255.0f;
            for (int c = 0; c < 3; ++c)
                linha[x][c] = cv::saturate_cast<unsigned char>(alfa * pixel[c] + (1-alfa) * linha[x][c]);
        }
    }
}

cv::Mat comporResultado(const cv::Mat &origem, const cv::Mat &segunda, const Efeito &efeito,
    const std::vector<cv::Mat> &stickers, const std::vector<StickerPosicionado> &posicoes)
{
    cv::Mat resultado = processar(origem, segunda, efeito);
    for (const auto &posicao : posicoes)
        sobreporSticker(resultado, stickers.at(posicao.indice), posicao.centro, posicao.largura);
    return resultado;
}

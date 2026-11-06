#include "Processamento.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <stdexcept>

static int verificacoes = 0;
static void conferir(bool condicao, const char *mensagem)
{
    ++verificacoes;
    if (!condicao) throw std::runtime_error(mensagem);
}

static bool iguais(const cv::Mat &a, const cv::Mat &b)
{
    return a.size() == b.size() && a.type() == b.type() && cv::norm(a, b, cv::NORM_INF) == 0;
}

int main(int argc, char **argv)
{
    try
    {
        if (argc != 2) throw std::runtime_error("Informe a pasta assets.");
        cv::Mat origem(35, 47, CV_8UC3);
        cv::RNG gerador(2026);
        gerador.fill(origem, cv::RNG::UNIFORM, 0, 256);
        const cv::Mat copia = origem.clone();
        const cv::Mat segunda(12, 19, CV_8UC3, cv::Scalar(10,20,30));
        conferir(listaFiltros().size() == 11, "Devem existir original + 10 filtros.");

        // Percorre todos os filtros/canais e os extremos dos parametros.
        for (int filtro = 0; filtro <= 10; ++filtro)
            for (int canal = 0; canal <= 3; ++canal)
                for (int parametro : {0, 128, 255})
                {
                    Efeito efeito;
                    efeito.filtro = filtro;
                    efeito.canal = static_cast<Canal>(canal);
                    efeito.parametro = parametro;
                    const cv::Mat saida = processar(origem, {}, efeito);
                    conferir(saida.size() == origem.size() && saida.type() == CV_8UC3, "Filtro mudou tamanho ou formato.");
                    if (canal != 0)
                    {
                        std::vector<cv::Mat> canais;
                        cv::split(saida, canais);
                        conferir(iguais(canais[0], canais[1]) && iguais(canais[1], canais[2]), "Canal isolado deve aparecer em cinza.");
                    }
                    conferir(iguais(origem, copia), "O processamento alterou a origem.");
                }

        const cv::Mat pixel(1, 1, CV_8UC3, cv::Scalar(10,50,200));
        conferir(selecionarCanal(pixel, Canal::R).at<unsigned char>(0,0) == 200, "Mapeamento R/B incorreto.");
        conferir(selecionarCanal(pixel, Canal::G).at<unsigned char>(0,0) == 50, "Mapeamento G incorreto.");
        conferir(selecionarCanal(pixel, Canal::B).at<unsigned char>(0,0) == 10, "Mapeamento B/R incorreto.");
        const auto negativo = aplicarFiltro(pixel, 2, 128).at<cv::Vec3b>(0,0);
        conferir(negativo == cv::Vec3b(245,205,55), "Negativo incorreto.");
        const auto sepia = aplicarFiltro(pixel, 10, 128).at<cv::Vec3b>(0,0);
        // A rotina SIMD de transform pode diferir em 1 pela quantizacao dos pesos.
        conferir(std::abs(sepia[0]-82) <= 1 && std::abs(sepia[1]-106) <= 1 && std::abs(sepia[2]-119) <= 1,
            "Matriz sepia incorreta.");
        const cv::Mat uniforme(35, 35, CV_8UC3, cv::Scalar(90,120,160));
        for (int filtro : {3,4,5,6})
            conferir(iguais(aplicarFiltro(uniforme,filtro,128), uniforme), "Suavizacao/realce deve preservar imagem constante.");
        for (int filtro : {7,8})
            conferir(cv::norm(aplicarFiltro(uniforme,filtro,128), cv::NORM_INF) == 0, "Imagem constante nao tem bordas.");
        const cv::Mat impulso = (cv::Mat_<unsigned char>(3,3) << 0,0,0,0,255,0,0,0,0);
        conferir(aplicarFiltro(impulso,5,17).at<unsigned char>(1,1) == 0, "Mediana nao removeu ruido impulsivo.");
        const cv::Mat tons = (cv::Mat_<unsigned char>(1,3) << 99,100,101);
        conferir(iguais(aplicarFiltro(tons,9,100), (cv::Mat_<unsigned char>(1,3) << 0,0,255)), "Limiarizacao incorreta.");

        const cv::Mat a(3,4,CV_8UC3,cv::Scalar(250,30,100));
        const cv::Mat b(2,2,CV_8UC3,cv::Scalar(20,60,50));
        conferir(aplicarOperacao(a,b,Operacao::Adicao,0).at<cv::Vec3b>(0,0) == cv::Vec3b(255,90,150), "Soma deve saturar, sem overflow.");
        conferir(aplicarOperacao(a,b,Operacao::Subtracao,255).at<cv::Vec3b>(0,0) == cv::Vec3b(230,0,50), "Subtracao deve limitar valores negativos.");
        conferir(iguais(aplicarOperacao(a,b,Operacao::Blending,255), a), "Blending peso 1 deve devolver A.");
        conferir(aplicarOperacao(a,b,Operacao::Blending,0).at<cv::Vec3b>(0,0) == cv::Vec3b(20,60,50), "Blending peso 0 deve devolver B.");
        for (int operacao = 1; operacao <= 3; ++operacao)
            for (int canal = 0; canal <= 3; ++canal)
            {
                Efeito efeito;
                efeito.operacao = static_cast<Operacao>(operacao);
                efeito.canal = static_cast<Canal>(canal);
                conferir(processar(origem,segunda,efeito).size() == origem.size(), "Operacao nao ajustou a segunda imagem.");
            }
        bool recusou = false;
        try { aplicarOperacao(a,{},Operacao::Blending,128); }
        catch (const std::invalid_argument &) { recusou = true; }
        conferir(recusou, "Operacao deve rejeitar segunda imagem ausente.");

        cv::Mat fundo(4,4,CV_8UC3,cv::Scalar(100,100,100));
        cv::Mat sticker(2,2,CV_8UC4,cv::Scalar(200,0,0,128));
        sticker.at<cv::Vec4b>(0,0)[3] = 0;
        sobreporSticker(fundo, sticker, cv::Point(1,1), 2);
        conferir(fundo.at<cv::Vec3b>(0,0) == cv::Vec3b(100,100,100), "Alfa zero deve preservar o fundo.");
        conferir(fundo.at<cv::Vec3b>(1,1) == cv::Vec3b(150,50,50), "Composicao alfa parcial incorreta.");
        sticker.setTo(cv::Scalar(12,34,56,255));
        sobreporSticker(fundo, sticker, cv::Point(0,0), 2);
        conferir(fundo.at<cv::Vec3b>(0,0) == cv::Vec3b(12,34,56), "Sticker na borda deve ser recortado corretamente.");
        sobreporSticker(fundo, sticker, cv::Point(-100,-100), 2);
        conferir(fundo.at<cv::Vec3b>(0,0) == cv::Vec3b(12,34,56), "Sticker fora da imagem deve ser ignorado.");

        std::vector<cv::Mat> stickers;
        for (const auto &nome : {"link.png","princesa.png","arvore.png","arbusto.png","ponte.png"})
        {
            const auto imagem = lerImagem(std::filesystem::u8path(argv[1]) / "stickers" / nome, cv::IMREAD_UNCHANGED);
            conferir(imagem.type() == CV_8UC4, "Sticker real deve conter alfa.");
            cv::Mat alfa;
            cv::extractChannel(imagem,alfa,3);
            double minimo, maximo;
            cv::minMaxLoc(alfa,&minimo,&maximo);
            conferir(minimo == 0 && maximo > 0, "Sticker real deve ter pixels transparentes e visiveis.");
            stickers.push_back(imagem);
        }
        Efeito efeito;
        efeito.filtro = 2;
        const auto comSticker = comporResultado(origem,{},efeito,stickers,{{0,{20,20},12}});
        conferir(!iguais(comSticker, processar(origem,{},efeito)), "Sticker nao apareceu no resultado.");
        conferir(iguais(comporResultado(origem,{},Efeito{},stickers,{}),copia), "Reset deve restaurar exatamente a origem.");
        conferir(iguais(origem,copia), "Stickers alteraram a origem.");

        const auto caminho = std::filesystem::current_path() / "output" / std::filesystem::u8path(u8"teste_acentuação") / "resultado.png";
        salvarPNG(caminho,comSticker);
        conferir(iguais(lerImagem(caminho,cv::IMREAD_COLOR),comSticker), "Salvar/reabrir PNG com acentos deve preservar pixels.");
        // Fixture geometrica para a verificacao visual da interface.
        cv::Mat foto(600,800,CV_8UC3,cv::Scalar(220,170,105));
        cv::rectangle(foto,{0,380},{799,599},cv::Scalar(60,150,65),cv::FILLED);
        cv::rectangle(foto,{220,260},{580,500},cv::Scalar(80,180,225),cv::FILLED);
        std::vector<cv::Point> telhado={{180,260},{400,100},{620,260}};
        cv::fillConvexPoly(foto,telhado,cv::Scalar(65,70,180));
        cv::rectangle(foto,{350,350},{450,500},cv::Scalar(55,85,115),cv::FILLED);
        cv::circle(foto,{690,100},50,cv::Scalar(65,225,250),cv::FILLED);
        salvarPNG(std::filesystem::current_path()/"output"/"foto_teste.png",foto);
        std::cout << verificacoes << " verificacoes passaram.\n";
        return 0;
    }
    catch (const std::exception &erro)
    {
        std::cerr << "Falha: " << erro.what() << '\n';
        return 1;
    }
}

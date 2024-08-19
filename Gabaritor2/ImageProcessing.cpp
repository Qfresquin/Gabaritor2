#include <iostream>
#include <fstream>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-image.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <opencv2/opencv.hpp>
#include <glad/glad.h>
#include <filesystem>
#include <GLFW/glfw3.h>
#include "ImageProcessing.h"
#include <tesseract/baseapi.h>
#include <cmath>
#include <numeric>
#include <vector>



const int MAX_FEATURES = 800;
const float GOOD_MATCH_PERCENT = 0.10f;
// Defina a margem de segurança (em pixels)
const int marginX = 15;  // Margem para o eixo X
const int marginY = 10;  // Margem para o eixo Y
const int offsetX = 0;  // Deslocamento para o eixo X
const int offsetY = 20;  // Deslocamento para o eixo Y

const int tolerancia = 1;

void processPdf(ConsoleBuffer& consoleBuffer, const std::string& filenamePdf, const std::string& imag_output_folder, int DPI) {
    auto mypdf = poppler::document::load_from_file(filenamePdf);
    if (mypdf == nullptr) {
        consoleBuffer.AddLogMessage(LogLevel::Error, "couldn't read pdf: " + filenamePdf);
        return;
    }

    int num_pages = mypdf->pages();
    consoleBuffer.AddLogMessage(LogLevel::Info, "pdf has " + std::to_string(num_pages) + " pages");

    for (int i = 0; i < num_pages; i++) {
        auto mypage = mypdf->create_page(i);
        poppler::page_renderer renderer;
        renderer.set_render_hint(poppler::page_renderer::text_antialiasing);
        poppler::image myimage = renderer.render_page(mypage, DPI, DPI);

        cv::Mat cvimg;
        if (myimage.format() == poppler::image::format_enum::format_rgb24) {
            cv::Mat(myimage.height(), myimage.width(), CV_8UC3, myimage.data()).copyTo(cvimg);
        }
        else if (myimage.format() == poppler::image::format_enum::format_argb32) {
            cv::Mat(myimage.height(), myimage.width(), CV_8UC4, myimage.data()).copyTo(cvimg);
        }
        else {
            consoleBuffer.AddLogMessage(LogLevel::Error, "Unsupported PDF format in page " + std::to_string(i + 1));
            continue; // Alterado de 'return' para 'continue' para processar as demais páginas mesmo com erro
        }

        // Ajuste aqui: passa somente o nome do arquivo para salvarImagem, não o caminho completo
        std::string nomeArquivo = "page_" + std::to_string(i + 1) + ".png";

        salvarImagem(consoleBuffer, imag_output_folder, nomeArquivo, cvimg); // Ajustado para passar o consoleBuffer
    }

    consoleBuffer.AddLogMessage(LogLevel::Info, "Todas as páginas foram salvas com sucesso!");
}

void alignImagesORB(cv::Mat& im1, cv::Mat& im2, cv::Mat& im1Reg, cv::Mat& h) {
    // Convert images to grayscale
    cv::Mat im1Gray, im2Gray;
    cv::cvtColor(im1, im1Gray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(im2, im2Gray, cv::COLOR_BGR2GRAY);

    // Variables to store keypoints and descriptors
    std::vector<cv::KeyPoint> keypoints1, keypoints2;
    cv::Mat descriptors1, descriptors2;

    // Detect ORB features and compute descriptors.
    cv::Ptr<cv::Feature2D> orb = cv::ORB::create(MAX_FEATURES);
    orb->detectAndCompute(im1Gray, cv::Mat(), keypoints1, descriptors1);
    orb->detectAndCompute(im2Gray, cv::Mat(), keypoints2, descriptors2);

    // Match features.
    std::vector<cv::DMatch> matches;
    cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create("BruteForce-Hamming");
    matcher->match(descriptors1, descriptors2, matches, cv::Mat());

    // Sort matches by score
    std::sort(matches.begin(), matches.end());

    // Remove not so good matches
    const int numGoodMatches = static_cast<int>(matches.size() * GOOD_MATCH_PERCENT);
    matches.erase(matches.begin() + numGoodMatches, matches.end());

    // Extract location of good matches
    std::vector<cv::Point2f> points1, points2;

    for (size_t i = 0; i < matches.size(); i++) {
        points1.push_back(keypoints1[matches[i].queryIdx].pt);
        points2.push_back(keypoints2[matches[i].trainIdx].pt);
    }

    // Find homography
    h = cv::findHomography(points1, points2, cv::RANSAC);

    // Use homography to warp image
    cv::warpPerspective(im1, im1Reg, h, im2.size());
}

void alinharImagens(ConsoleBuffer& consoleBuffer, const std::string& imag_output_folder, const std::string& aling_imag_folder, 
    const std::string& reference_image_path) {
    cv::Mat imageRef = cv::imread(reference_image_path);
    if (imageRef.empty()) {
        consoleBuffer.AddLogMessage(LogLevel::Error, "Error loading reference image from path: " + reference_image_path);
        return;
    }

    std::vector<cv::String> filenames;
    cv::glob(imag_output_folder + "/*.png", filenames, false);

    for (const auto& filename : filenames) {
        consoleBuffer.AddLogMessage(LogLevel::Info, "Processing file: " + filename);

        cv::Mat image = cv::imread(filename);
        if (image.empty()) {
            consoleBuffer.AddLogMessage(LogLevel::Error, "Error loading image: " + filename);
            continue;
        }

        cv::Mat alignedImage, h;
        alignImagesORB(image, imageRef, alignedImage, h);

        if (alignedImage.empty()) {
            consoleBuffer.AddLogMessage(LogLevel::Error, "Error aligning image: " + filename);
            continue;
        }

        // Extrai o nome do arquivo do caminho completo
        auto pos = filename.find_last_of("/\\");
        std::string fileName = filename.substr(pos + 1);

        salvarImagem(consoleBuffer, aling_imag_folder, fileName, alignedImage); // Adicionado consoleBuffer como parâmetro
    }

    consoleBuffer.AddLogMessage(LogLevel::Info, "All images have been aligned and saved.");
}

void calcularParametrosDinamicos(const cv::Mat& image, int& tolerancia, int& intensidadeMinima) {
    std::vector<int> intensities;
    std::vector<int> diffs;

    for (int y = 0; y < image.rows; y++) {
        for (int x = 0; x < image.cols; x++) {
            cv::Vec3b pixel = image.at<cv::Vec3b>(y, x);

            int blue = pixel[0];
            int green = pixel[1];
            int red = pixel[2];

            int intensity = (red + green + blue) / 3;
            intensities.push_back(intensity);

            int diffRG = abs(red - green);
            int diffRB = abs(red - blue);
            int diffGB = abs(green - blue);

            diffs.push_back(diffRG);
            diffs.push_back(diffRB);
            diffs.push_back(diffGB);
        }
    }

    // Calcula a média e o desvio padrão das intensidades
    double meanIntensity = std::accumulate(intensities.begin(), intensities.end(), 0.0) / intensities.size();
    double sq_sum_intensity = std::inner_product(intensities.begin(), intensities.end(), intensities.begin(), 0.0);
    double stdevIntensity = std::sqrt(sq_sum_intensity / intensities.size() - meanIntensity * meanIntensity);

    // Calcula a média e o desvio padrão das diferenças
    double meanDiff = std::accumulate(diffs.begin(), diffs.end(), 0.0) / diffs.size();
    double sq_sum_diff = std::inner_product(diffs.begin(), diffs.end(), diffs.begin(), 0.0);
    double stdevDiff = std::sqrt(sq_sum_diff / diffs.size() - meanDiff * meanDiff);

    // Define a tolerância e a intensidade mínima baseadas nas médias e desvios padrão
    tolerancia = static_cast<int>((meanDiff + stdevDiff + 6 ) * 2 );
    intensidadeMinima = static_cast<int> (meanIntensity - stdevIntensity);
}

void removerCinzaLeveDinamico(cv::Mat& image) {
    int tolerancia, intensidadeMinima;
    calcularParametrosDinamicos(image, tolerancia, intensidadeMinima);

    for (int y = 0; y < image.rows; y++) {
        for (int x = 0; x < image.cols; x++) {
            cv::Vec3b& pixel = image.at<cv::Vec3b>(y, x);

            int blue = pixel[0];
            int green = pixel[1];
            int red = pixel[2];

            int diffRG = abs(red - green);
            int diffRB = abs(red - blue);
            int diffGB = abs(green - blue);

            int intensity = (red + green + blue) / 3;

            if (diffRG < tolerancia && diffRB < tolerancia && diffGB < tolerancia && intensity > intensidadeMinima) {
                pixel = cv::Vec3b(255, 255, 255); // Substitua por preto: cv::Vec3b(0, 0, 0)
            }
        }
    }
}

void aplicarFiltroReducaoRuido(ConsoleBuffer& consoleBuffer, const std::string& pastaImagensAlinhadas, const std::string& pastaDestino) {
    std::vector<cv::String> arquivos;
    cv::glob(pastaImagensAlinhadas + "/*.png", arquivos, false);

    for (const auto& arquivo : arquivos) {
        cv::Mat imagem = cv::imread(arquivo, cv::IMREAD_COLOR);
        if (imagem.empty()) {
            consoleBuffer.AddLogMessage(LogLevel::Error, "Erro ao carregar a imagem: " + std::string(arquivo));
            continue;
        }

        // Aplica o filtro de média bilateral
        cv::Mat imagemFiltrada;
        cv::bilateralFilter(imagem, imagemFiltrada, 9, 75, 75);

        // Remove tons de cinza leve de forma dinâmica
        removerCinzaLeveDinamico(imagemFiltrada);


        // Extrai o nome do arquivo do caminho completo
        auto pos = arquivo.find_last_of("/\\");
        std::string fileName = arquivo.substr(pos + 1);

        // Salva a imagem processada na pasta de destino
        salvarImagem(consoleBuffer, pastaDestino, fileName, imagemFiltrada);
    }

    consoleBuffer.AddLogMessage(LogLevel::Info, "Filtro de redução de ruído aplicado a todas as imagens com sucesso.");
}

void extrairContornos(ConsoleBuffer& consoleBuffer, const std::string& pastaOrigem, const std::string& pastaDestino, const std::string& pastaThreshold) {
    std::vector<cv::String> arquivos;
    cv::glob(pastaOrigem + "/*.png", arquivos, false); // Adaptar o padrão conforme necessário

    int contourThickness = 5; // Ajuste a espessura do contorno conforme necessário

    // Cria o diretório de saída de threshold se não existir
    if (!criarDiretorio(consoleBuffer, pastaThreshold)) {
        consoleBuffer.AddLogMessage(LogLevel::Error, "Failed to create threshold directory: " + pastaThreshold);
        return; // Se não foi possível criar o diretório, aborta o processamento
    }

    for (const auto& arquivo : arquivos) {
        cv::Mat imagem = cv::imread(arquivo, cv::IMREAD_GRAYSCALE);
        if (imagem.empty()) {
            consoleBuffer.AddLogMessage(LogLevel::Error, "Erro ao carregar a imagem: " + std::string(arquivo));
            continue;
        }

        // Aplica threshold adaptativo
        cv::Mat imagemThreshold;
        cv::adaptiveThreshold(imagem, imagemThreshold, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY_INV, 11, 2);

        // Salva a imagem de threshold na pasta de threshold
        auto pos = arquivo.find_last_of("/\\");
        std::string nomeArquivo = arquivo.substr(pos + 1);
        salvarImagem(consoleBuffer, pastaThreshold, nomeArquivo, imagemThreshold);

        // Encontra contornos
        std::vector<std::vector<cv::Point>> contornos;
        cv::findContours(imagemThreshold, contornos, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

        // Desenha contornos em uma nova imagem
        cv::Mat imagemContornos = cv::Mat::zeros(imagem.size(), CV_8UC3);
        for (size_t i = 0; i < contornos.size(); i++) {
            cv::Scalar cor = cv::Scalar(0, 255, 0); // Cor verde para os contornos
            cv::drawContours(imagemContornos, contornos, static_cast<int>(i), cor, contourThickness, cv::LINE_8);
        }

        // Salva a imagem com contornos na pasta de destino
        salvarImagem(consoleBuffer, pastaDestino, nomeArquivo, imagemContornos);
    }

    consoleBuffer.AddLogMessage(LogLevel::Info, "Extração de contornos e salvamento de imagens de threshold concluídos.");
}

std::vector<RectangleData> loadAnswerRectangles(const std::string& filepath) {
    std::vector<RectangleData> rectangles;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo de coordenadas: " << filepath << std::endl;
        return rectangles;
    }

    std::string name;
    float x, y, z, w;
    int lines, columns;
    bool analyzeVertical;
    bool isWord; 
    bool isNumber;

    while (std::getline(file, name, '|')) {
        if (file >> x >> y >> z >> w >> lines >> columns >> analyzeVertical >> isWord >> isNumber) {
            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Ignora o resto da linha
            rectangles.push_back({ ImVec4(x, y, z, w), {lines, columns}, name, analyzeVertical, isWord, isNumber });
        }
        else {
            std::cerr << "Erro ao ler os dados do retângulo do arquivo: " << filepath << std::endl;
            break;
        }
    }

    file.close();
    return rectangles;
}

std::vector<char> readAnswersFromRectangles(const cv::Mat& image, const std::vector<RectangleData>& rectangles, ConsoleBuffer& consoleBuffer) {
    std::vector<char> answers;

    for (const auto& rectData : rectangles) {
        int x = static_cast<int>(rectData.coordinates.x * image.cols);
        int y = static_cast<int>(rectData.coordinates.y * image.rows);
        int width = static_cast<int>((rectData.coordinates.z - rectData.coordinates.x) * image.cols);
        int height = static_cast<int>((rectData.coordinates.w - rectData.coordinates.y) * image.rows);
        int cellWidth = width / rectData.subdivisions.second;
        int cellHeight = height / rectData.subdivisions.first;

        int numAlternatives = rectData.analyzeVertical ? rectData.subdivisions.first : rectData.subdivisions.second;
        int numChoices = rectData.analyzeVertical ? rectData.subdivisions.second : rectData.subdivisions.first;

        for (int alt = 0; alt < numAlternatives; ++alt) {
            int subX = x + (rectData.analyzeVertical ? 0 : alt * cellWidth);
            int subY = y + (rectData.analyzeVertical ? alt * cellHeight : 0);

            char selectedAnswer = 'X';
            int maxWhitePixels = 0;
            int totalWhitePixels = 0; // Total de pixels brancos para calcular a média
            std::vector<int> whitePixelsPerChoice(numChoices, 0); // Pixels brancos por escolha

            for (int choice = 0; choice < numChoices; ++choice) {
                int roiX = subX + (rectData.analyzeVertical ? choice * cellWidth : 0) + marginX + offsetX;
                int roiY = subY + (rectData.analyzeVertical ? 0 : choice * cellHeight) + marginY + offsetY;
                int roiWidth = cellWidth - 2 * marginX;
                int roiHeight = cellHeight - 2 * marginY;

                // Verifica se a ROI ajustada está dentro dos limites da imagem
                if (roiX >= 0 && roiY >= 0 && roiX + roiWidth <= image.cols && roiY + roiHeight <= image.rows) {
                    cv::Rect roi(roiX, roiY, roiWidth, roiHeight);
                    cv::Mat region = image(roi);

                    // Salva a ROI para debug
                    //std::string nomeArquivo = "roi_alt_" + std::to_string(alt) + "_choice_" + std::to_string(choice) + ".png";
                    //salvarImagem(consoleBuffer, "ROIs", nomeArquivo, region);

                    // Conta pixels não zero (brancos) na ROI
                    int whitePixels = cv::countNonZero(region);
                    whitePixelsPerChoice[choice] = whitePixels;
                    totalWhitePixels += whitePixels;

                    if (whitePixels > maxWhitePixels) {
                        maxWhitePixels = whitePixels;
                        selectedAnswer = 'A' + choice; // 'A', 'B', 'C', ou 'D', etc.
                        if (rectData.isNumber) selectedAnswer = '0' + choice;
                    }
                }
                else {
                    consoleBuffer.AddLogMessage(LogLevel::Warning, "ROI fora dos limites: (" + std::to_string(roiX) + ", " + std::to_string(roiY) + ")");
                }
            }

            // Calcula a média dos pixels brancos
            int averageWhitePixels = totalWhitePixels / numChoices;

            // Calcula o dynamic threshold
            int dynamicThreshold = ((maxWhitePixels / 2) + (averageWhitePixels)/1.5);
            int selectedCount = 0;

            // Conta quantas escolhas estão acima do threshold dinâmico
            for (int choice = 0; choice < numChoices; ++choice) {
                if (whitePixelsPerChoice[choice] > dynamicThreshold) {
                    selectedCount++;
                }
            }

            // Se mais de uma escolha está acima do threshold, marque como 'X'
            if (selectedCount > 1) {
                selectedAnswer = 'X';
            }
            else if (selectedCount == 0) {
                selectedAnswer = 'V';
            }


            answers.push_back(selectedAnswer);
        }
    }

    return answers;
}

void processImagesAndReadAnswers(ConsoleBuffer& consoleBuffer, const std::string& contourImageFolder, const std::string& coordinatesFilePath, const std::string& outputFolder) {
    std::vector<std::string> filenames;
    cv::glob(contourImageFolder + "/*.png", filenames, false);

    std::vector<RectangleData> rectangles = loadAnswerRectangles(coordinatesFilePath);
    if (rectangles.empty()) {
        consoleBuffer.AddLogMessage(LogLevel::Error, "Failed to load answer areas from file: " + coordinatesFilePath);
        return;
    }

    // Cria o diretório de saída se não existir
    if (!criarDiretorio(consoleBuffer, outputFolder)) {
        consoleBuffer.AddLogMessage(LogLevel::Error, "Failed to create output directory: " + outputFolder);
        return; // Se não foi possível criar o diretório, aborta o processamento
    }

    for (const auto& filename : filenames) {
        cv::Mat image = cv::imread(filename, cv::IMREAD_GRAYSCALE);
        if (image.empty()) {
            consoleBuffer.AddLogMessage(LogLevel::Error, "Erro ao carregar a imagem: " + filename);
            continue;
        }

        std::vector<char> answers = readAnswersFromRectangles(image, rectangles, consoleBuffer);

        // Extrai o nome do arquivo do caminho completo
        auto pos = filename.find_last_of("/\\");
        std::string fileName = filename.substr(pos + 1);

        // Salva as respostas no diretório de saída
        std::string outputFilePath = outputFolder + "/" + fileName + "_answers.txt";
        std::ofstream outputFile(outputFilePath);
        if (!outputFile.is_open()) {
            consoleBuffer.AddLogMessage(LogLevel::Error, "Erro ao salvar as respostas: " + outputFilePath);
            continue;
        }

        int answerIndex = 0;
        for (const auto& rectData : rectangles) {
            int numSubdivisions = rectData.analyzeVertical ? rectData.subdivisions.first : rectData.subdivisions.second;
            for (int sub = 0; sub < numSubdivisions; ++sub) {
                outputFile << rectData.name << " Subdivision " << sub + 1 << ": " << answers[answerIndex++] << std::endl;
            }
        }

        outputFile.close();
        consoleBuffer.AddLogMessage(LogLevel::Info, "Respostas salvas em: " + outputFilePath);
    }
}

std::string extractWordsFromRegion(const cv::Mat& image, const cv::Rect& region) {
    // Corte a região de interesse (ROI) da imagem
    cv::Mat roi = image(region);

    // Inicialize o Tesseract OCR
    tesseract::TessBaseAPI* ocr = new tesseract::TessBaseAPI();
    if (ocr->Init(NULL, "eng")) {
        std::cerr << "Could not initialize tesseract.\n";
        return "";
    }

    // Converta a imagem para o formato que o Tesseract entende
    ocr->SetImage(roi.data, roi.cols, roi.rows, roi.elemSize(), roi.step);

    // Realize o OCR na ROI e obtenha o texto
    std::string extractedText = std::string(ocr->GetUTF8Text());

    // Limpe
    ocr->End();
    delete ocr;

    return extractedText;
}

void processImagesAndExtractWords(ConsoleBuffer& consoleBuffer, const std::string& imageFolder, const std::string& coordinatesFilePath, const std::string& outputFolder) {
    std::vector<std::string> filenames;
    cv::glob(imageFolder + "/*.png", filenames, false);

    std::vector<RectangleData> rectangles = loadAnswerRectangles(coordinatesFilePath);
    if (rectangles.empty()) {
        consoleBuffer.AddLogMessage(LogLevel::Error, "Failed to load regions from file: " + coordinatesFilePath);
        return;
    }

    // Cria o diretório de saída se não existir
    if (!criarDiretorio(consoleBuffer, outputFolder)) {
        consoleBuffer.AddLogMessage(LogLevel::Error, "Failed to create output directory: " + outputFolder);
        return; // Se não foi possível criar o diretório, aborta o processamento
    }

    for (const auto& filename : filenames) {
        cv::Mat image = cv::imread(filename, cv::IMREAD_GRAYSCALE);
        if (image.empty()) {
            consoleBuffer.AddLogMessage(LogLevel::Error, "Erro ao carregar a imagem: " + filename);
            continue;
        }

        // Processa cada região definida
        for (const auto& rectData : rectangles) {
            if (!rectData.isWord) {
                continue; // Ignora retângulos que não são palavras
            }
            int x = static_cast<int>(rectData.coordinates.x * image.cols);
            int y = static_cast<int>(rectData.coordinates.y * image.rows);
            int width = static_cast<int>((rectData.coordinates.z - rectData.coordinates.x) * image.cols);
            int height = static_cast<int>((rectData.coordinates.w - rectData.coordinates.y) * image.rows);

            cv::Rect region(x, y, width, height);
            std::string extractedWords = extractWordsFromRegion(image, region);

            // Extrai o nome do arquivo do caminho completo
            auto pos = filename.find_last_of("/\\");
            std::string fileName = filename.substr(pos + 1);
            std::string baseName = fileName.substr(0, fileName.find_last_of('.'));

            // Salva o texto extraído no diretório de saída
            std::string outputFilePath = outputFolder + "/" + baseName + "_region_" + rectData.name + "_words.txt";
            std::ofstream outputFile(outputFilePath);
            if (!outputFile.is_open()) {
                consoleBuffer.AddLogMessage(LogLevel::Error, "Erro ao salvar as palavras: " + outputFilePath);
                continue;
            }

            outputFile << "Extracted Words for " << rectData.name << ":\n" << extractedWords;
            outputFile.close();
            consoleBuffer.AddLogMessage(LogLevel::Info, "Palavras extraídas salvas em: " + outputFilePath);
        }
    }
}

void binarizarImagemDinamico(cv::Mat& image) {
    // Converte a imagem para escala de cinza
    cv::Mat grayImage;
    cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);

    // Calcula o threshold usando o método de Otsu
    double otsuThreshold = cv::threshold(grayImage, grayImage, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    // Aplica o limiar calculado
    cv::threshold(grayImage, grayImage, otsuThreshold, 255, cv::THRESH_BINARY);

    // Converte a imagem de volta para BGR
    cv::cvtColor(grayImage, image, cv::COLOR_GRAY2BGR);
}

void BinarizarDinamico(ConsoleBuffer& consoleBuffer, const std::string& pastaOrigem, const std::string& pastaDestino) {
    std::vector<cv::String> arquivos;
    cv::glob(pastaOrigem + "/*.png", arquivos, false);

    for (const auto& arquivo : arquivos) {
        cv::Mat imagem = cv::imread(arquivo, cv::IMREAD_COLOR);
        if (imagem.empty()) {
            consoleBuffer.AddLogMessage(LogLevel::Error, "Erro ao carregar a imagem: " + std::string(arquivo));
            continue;
        }

        // Binariza a imagem com threshold dinâmico
        binarizarImagemDinamico(imagem);

        // Extrai o nome do arquivo do caminho completo
        auto pos = arquivo.find_last_of("/\\");
        std::string fileName = arquivo.substr(pos + 1);

        // Salva a imagem processada na pasta de destino
        salvarImagem(consoleBuffer, pastaDestino, fileName, imagem);
    }

    consoleBuffer.AddLogMessage(LogLevel::Info, "Binarização dinâmica aplicada a todas as imagens com sucesso.");
}

bool compararArquivos(const std::string& a, const std::string& b) {
    int numA = std::stoi(a.substr(a.find("page_") + 5, a.find(".png_answers") - (a.find("page_") + 5)));
    int numB = std::stoi(b.substr(b.find("page_") + 5, b.find(".png_answers") - (a.find("page_") + 5)));
    return numA < numB;
}

void juntarRespostasEmTXT(ConsoleBuffer& consoleBuffer, const std::string& pastaRespostas, const std::string& pastaDestino) {
    // Cria o diretório de destino se não existir
    if (!criarDiretorio(consoleBuffer, pastaDestino)) {
        return;
    }

    std::vector<std::string> arquivos;
    cv::glob(pastaRespostas + "/*.txt", arquivos, false);

    // Ordena os arquivos pela numeração da página
    std::sort(arquivos.begin(), arquivos.end(), compararArquivos);

    std::string arquivoTXT = pastaDestino + "/respostas.txt";  // Nome do arquivo de respostas

    std::ofstream txtFile(arquivoTXT);
    if (!txtFile.is_open()) {
        consoleBuffer.AddLogMessage(LogLevel::Error, "Erro ao abrir o arquivo TXT para escrita: " + arquivoTXT);
        return;
    }

    for (const auto& arquivo : arquivos) {
        std::ifstream inputFile(arquivo);
        if (!inputFile.is_open()) {
            consoleBuffer.AddLogMessage(LogLevel::Error, "Erro ao abrir o arquivo de respostas: " + arquivo);
            continue;
        }

        std::string linha;
        std::string respostasConcatenadas;
        std::vector<std::string> respostasRestantes;
        int count = 0;

        while (std::getline(inputFile, linha)) {
            size_t pos = linha.find(":");
            if (pos != std::string::npos) {
                std::string resposta = linha.substr(pos + 1);
                resposta.erase(0, resposta.find_first_not_of(" \t")); // Remove espaços em branco antes da resposta
                if (count < 5) {
                    respostasConcatenadas += resposta;
                }
                else {
                    respostasRestantes.push_back(resposta);
                }
                count++;
            }
        }

        inputFile.close();

        txtFile << respostasConcatenadas;

        for (const std::string& resposta : respostasRestantes) {
            txtFile << "," << resposta;
        }
        txtFile << ",\n";
    }

    txtFile.close();
    consoleBuffer.AddLogMessage(LogLevel::Info, "Todas as respostas foram unidas no arquivo TXT com sucesso: " + arquivoTXT);
}
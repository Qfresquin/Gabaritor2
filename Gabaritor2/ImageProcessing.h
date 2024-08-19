#pragma once

#include "ConsoleBuffer.h"

// Estrutura para armazenar dados de retângulo
struct RectangleData {
    ImVec4 coordinates;
    std::pair<int, int> subdivisions;
    std::string name;
    bool analyzeVertical;  // Novo campo para a análise vertical
    bool isWord;
    bool isNumber;
};

void processPdf(ConsoleBuffer& consoleBuffer,const std::string& filenamePdf, const std::string& imag_output_folder, int DPI);
void alignImagesORB(cv::Mat& im1, cv::Mat& im2, cv::Mat& im1Reg, cv::Mat& h);
void alinharImagens(ConsoleBuffer& consoleBuffer, const std::string& imag_output_folder, const std::string& aling_imag_folder, const std::string& reference_image_path);
void aplicarFiltroReducaoRuido(ConsoleBuffer& consoleBuffer, const std::string& pastaImagensAlinhadas, const std::string& pastaDestino);
void extrairContornos(ConsoleBuffer& consoleBuffer, const std::string& pastaOrigem, const std::string& pastaDestino, const std::string& pastaThreshold);
void BinarizarDinamico(ConsoleBuffer& consoleBuffer, const std::string& pastaOrigem, const std::string& pastaDestino);

void salvarImagem(ConsoleBuffer& consoleBuffer,const std::string& pastaDestino, const std::string& nomeArquivo, const cv::Mat& imagem);
bool criarDiretorio(ConsoleBuffer& consoleBuffer,const std::string& pastaDestino);
void processImagesAndReadAnswers(ConsoleBuffer& consoleBuffer, const std::string& contourImageFolder, const std::string& coordinatesFilePath, const std::string& outputFolder);
void processImagesAndExtractWords(ConsoleBuffer& consoleBuffer, const std::string& imageFolder, const std::string& coordinatesFilePath, const std::string& outputFolder);
void juntarRespostasEmTXT(ConsoleBuffer& consoleBuffer, const std::string& pastaRespostas, const std::string& arquivoTXT);
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/filesystem.hpp>
#include "ImageProcessing.h"

// Fun��o para criar um diret�rio
bool criarDiretorio(ConsoleBuffer& consoleBuffer, const std::string& pastaDestino) {
    if (cv::utils::fs::exists(pastaDestino)) {
        return true;
    }

    // Tenta criar o diret�rio
    if (!cv::utils::fs::createDirectories(pastaDestino)) {
        consoleBuffer.AddLogMessage(LogLevel::Error, "N�o foi poss�vel criar a pasta de destino: " + pastaDestino);
        return false;
    }

    return true;
}

// Fun��o para salvar uma imagem
void salvarImagem(ConsoleBuffer& consoleBuffer, const std::string& pastaDestino, const std::string& nomeArquivo, const cv::Mat& imagem) {
	// Verifica se o diret�rio de destino existe
	if (!criarDiretorio(consoleBuffer, pastaDestino)) {
		return;
	}

	// Constr�i o caminho completo para salvar a imagem
	std::string caminhoCompleto = pastaDestino + "/" + nomeArquivo;

	// Tenta salvar a imagem
	if (!cv::imwrite(caminhoCompleto, imagem)) {
		consoleBuffer.AddLogMessage(LogLevel::Error, "Falha ao salvar a imagem em: " + caminhoCompleto);
		return;
	}

	consoleBuffer.AddLogMessage(LogLevel::Warning, "Imagem salva com sucesso em: " + caminhoCompleto);
}

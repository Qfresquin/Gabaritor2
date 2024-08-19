#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/filesystem.hpp>
#include "ImageProcessing.h"

// Função para criar um diretório
bool criarDiretorio(ConsoleBuffer& consoleBuffer, const std::string& pastaDestino) {
    if (cv::utils::fs::exists(pastaDestino)) {
        return true;
    }

    // Tenta criar o diretório
    if (!cv::utils::fs::createDirectories(pastaDestino)) {
        consoleBuffer.AddLogMessage(LogLevel::Error, "Não foi possível criar a pasta de destino: " + pastaDestino);
        return false;
    }

    return true;
}

// Função para salvar uma imagem
void salvarImagem(ConsoleBuffer& consoleBuffer, const std::string& pastaDestino, const std::string& nomeArquivo, const cv::Mat& imagem) {
	// Verifica se o diretório de destino existe
	if (!criarDiretorio(consoleBuffer, pastaDestino)) {
		return;
	}

	// Constrói o caminho completo para salvar a imagem
	std::string caminhoCompleto = pastaDestino + "/" + nomeArquivo;

	// Tenta salvar a imagem
	if (!cv::imwrite(caminhoCompleto, imagem)) {
		consoleBuffer.AddLogMessage(LogLevel::Error, "Falha ao salvar a imagem em: " + caminhoCompleto);
		return;
	}

	consoleBuffer.AddLogMessage(LogLevel::Warning, "Imagem salva com sucesso em: " + caminhoCompleto);
}

# Gabaritor

**Gabaritor** é um projeto desenvolvido para extrair automaticamente as respostas de gabaritos. Ele utiliza processamento de imagem para identificar e extrair as respostas marcadas em um gabarito digitalizado, automatizando o processo de correção de provas.

## Funcionalidades

- **Processamento de Imagem:** Analisa imagens de gabaritos para identificar as respostas marcadas.
- **Automação:** Automatiza o processo de correção, eliminando a necessidade de verificação manual.
- **Armazenamento:** Salva as respostas extraídas em um formato acessível para posterior análise.

## Estrutura do Projeto

O projeto é composto pelos seguintes arquivos principais:

- `Application.h`: Define as funções principais utilizadas na aplicação.
- `ConsoleBuffer.h`: Gera e manipula a interface de console para exibição de informações.
- `ImageProcessing.cpp` e `ImageProcessing.h`: Implementam o núcleo de processamento de imagem, responsável pela análise das imagens dos gabaritos.
- `main.cpp`: Ponto de entrada da aplicação, coordena a execução das funções principais.
- `saving.cpp`: Gerencia o armazenamento dos dados extraídos, como as respostas identificadas.

## Como Usar

1. **Compilação:** Compile o projeto utilizando um compilador C++ compatível, sendo necessario possuir as biliotecas linkadas ao seu projeto, boa sorte com isso.

2. Forneça a imagem do gabarito que deseja processar conforme as instruções na tela.

O programa exibirá as respostas extraídas e as salvará no formato especificado.

## Requisitos

- Compilador C++ (GCC, Clang, etc.)
- **Sistema Operacional**: Windows, Linux, macOS
- **Dependências**:
  - [Tesseract OCR](https://github.com/tesseract-ocr/tesseract) - OCR Engine
  - [OpenCV](https://opencv.org/) - Biblioteca de Visão Computacional
  - [GLFW](https://www.glfw.org/) - Biblioteca de janelas e input
  - [ImGui](https://github.com/ocornut/imgui) - Biblioteca de Interface Gráfica
  - [Poppler](https://poppler.freedesktop.org/) - Biblioteca de Manipulação de PDFs
  - [GLAD](https://glad.dav1d.de/) - Loader de funções OpenGL


## Contribuições

Contribuições são bem-vindas! Sinta-se à vontade para enviar pull requests ou relatar problemas no repositório.

## Licença

Este projeto é licenciado sob... não sei ainda, vou ver isso 


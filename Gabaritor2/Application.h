#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <opencv2/opencv.hpp>
#include "ImageProcessing.h" // Assumindo que suas funções e classes estejam aqui
#include <tinyfiledialogs/tinyfiledialogs.h>
#include <thread>
#include <fstream>
#include <atomic>
#include <vector>
#include <string>


class Application {
public:
    Application();
    ~Application();
    void run();

private:
    void initGLFW();
    void initGLAD();
    void initImGui();
    void cleanup();
    void mainLoop();
    void renderGUI();
    void processTask();
    static void glfwErrorCallback(int error, const char* description);

    // Funções de renderização
    void renderDockSpace();
    void renderMenuBar();
    void renderProcessPDFWindow();
    void renderReferenceImageWindow();
    void handleRectangleDrawing(const ImVec2& imagePos, const ImVec2& imageSize);
    void renderRectanglePropertiesWindow(bool* p_open);

    // Funções auxiliares
    GLuint loadImageAsTexture(const char* imagePath);
    void saveRectanglesToFile(const std::string& filename);
    void loadRectanglesFromFile(const std::string& filename);
    
    
	// Variáveis de estado
    ConsoleBuffer consoleBuffer;
    GLFWwindow* window;
    std::thread processingThread;
    std::atomic<bool> isProcessing;
    std::atomic<bool> processFinished;
    bool showProcessPDFWindow;
    bool skipPdfConversion, skipPdfAlignment, skipNoiseReduction, skipContourExtraction, skipReadAnswers, skipReadWords, skipBinarize;
    GLuint referenceImageTexture;
    bool showReferenceImageWindow;
    char filenamePdf[1024];
    char referenceImage[1024];
    char coordinatesFilePath[1024];
    bool startDrawing;
    bool isDrawing;
    ImVec2 rectStart;
    ImVec2 rectEnd;
    ImVec4 currentRectangle;
    std::vector<RectangleData> rectangles;
    cv::Size originalImageSize;
    bool showRectanglePropertiesWindow;
    bool isMaximized;

};

Application::Application()
    : window(nullptr),
    showProcessPDFWindow(true),
    skipPdfConversion(false), skipPdfAlignment(false), skipNoiseReduction(false), skipContourExtraction(false),
    skipReadAnswers(false), skipReadWords(false), skipBinarize(false), // Inicializa a variável da nova checkbox
    referenceImageTexture(0), showReferenceImageWindow(false),
    startDrawing(false), isDrawing(false),
    originalImageSize(0, 0), showRectanglePropertiesWindow(true),
    isMaximized(false) {
    strncpy_s(filenamePdf, "C:/Users/Pedro/Downloads/AA.pdf", sizeof(filenamePdf));
    strncpy_s(referenceImage, "C:/Users/Pedro/Desktop/Nova pasta/Referencia.png", sizeof(referenceImage));
    strncpy_s(coordinatesFilePath, "D:/Projetos/Aprendizado/Garbaritor/Garbaritor/rectangles.txt", sizeof(coordinatesFilePath)); // Inicializa o caminho do arquivo de coordenadas
}

Application::~Application() {
    if (referenceImageTexture) {
        glDeleteTextures(1, &referenceImageTexture);
    }
    if (processingThread.joinable()) {
        processingThread.join();
    }
    cleanup();
}

void Application::initGLFW() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    const char* glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    window = glfwCreateWindow(1280, 720, "PDF Processing GUI", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable V-sync
}

void Application::glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void Application::initGLAD() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }
}

void Application::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

void Application::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::run() {
    try {
        initGLFW();
        initGLAD();
        initImGui();
        mainLoop();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        cleanup();
    }
}

void Application::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderGUI();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backupCurrentContext);
        }

        glfwSwapBuffers(window);
    }
}

void Application::renderGUI() {
    renderDockSpace();
    renderMenuBar();

    if (showProcessPDFWindow) {
        renderProcessPDFWindow();
    }

    if (showReferenceImageWindow && referenceImageTexture) {
        renderReferenceImageWindow();
    }

    if (showRectanglePropertiesWindow) {
        renderRectanglePropertiesWindow(&showRectanglePropertiesWindow);
    }
}

void Application::renderDockSpace() {
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y));
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    ImGui::PopStyleVar(2);

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    ImGui::End();
}

void Application::renderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void Application::renderProcessPDFWindow() {
    ImGui::Begin("PDF Processor", &showProcessPDFWindow);

    if (ImGui::Button("Select PDF File") && !isProcessing) {
        const char* filterPatterns[1] = { "*.pdf" };
        const char* pdfPath = tinyfd_openFileDialog("Select PDF File", filenamePdf, 1, filterPatterns, NULL, 0);
        if (pdfPath) {
            strncpy_s(filenamePdf, pdfPath, sizeof(filenamePdf));
        }
    }
    ImGui::SameLine();
    ImGui::Text("PDF: %s", filenamePdf);

    if (ImGui::Button("Select Reference Image") && !isProcessing) {
        const char* imageFilterPatterns[2] = { "*.png", "*.jpg" };
        const char* imagePath = tinyfd_openFileDialog("Select Reference Image", referenceImage, 2, imageFilterPatterns, "Image Files", 0);
        if (imagePath) {
            strncpy_s(referenceImage, imagePath, sizeof(referenceImage));
        }
    }
    ImGui::SameLine();
    ImGui::Text("Image: %s", referenceImage);

    if (ImGui::Button("Load Reference Image") && !isProcessing) {
        referenceImageTexture = loadImageAsTexture(referenceImage);
        showReferenceImageWindow = true;
    }

    if (ImGui::Button("Select Coordinates File") && !isProcessing) {
        const char* filterPatterns[1] = { "*.txt" };
        const char* filePath = tinyfd_openFileDialog("Select Coordinates File", coordinatesFilePath, 1, filterPatterns, NULL, 0);
        if (filePath) {
            strncpy_s(coordinatesFilePath, filePath, sizeof(coordinatesFilePath));
        }
    }
    ImGui::SameLine();
    ImGui::Text("Coordinates: %s", coordinatesFilePath);

    ImGui::Checkbox("Skip PDF Conversion", &skipPdfConversion);
    ImGui::Checkbox("Skip PDF Alignment", &skipPdfAlignment);
    ImGui::Checkbox("Skip Noise Reduction", &skipNoiseReduction);
    ImGui::Checkbox("Skip Contour Extraction", &skipContourExtraction);
    ImGui::Checkbox("Skip Binarize Feature", &skipBinarize);
    ImGui::Checkbox("Skip Read Answers", &skipReadAnswers);
    ImGui::Checkbox("Skip Read Words", &skipReadWords);

    ImGui::Separator();

    if (ImGui::Button("Start Processing") && !isProcessing) {
        if (processingThread.joinable()) processingThread.join();
        processingThread = std::thread(&Application::processTask, this);
        isProcessing = true;
    }

    if (isProcessing) {
        ImGui::Text("Processing...");
    }
    else if (processFinished) {
        ImGui::Text("Processing Finished.");
        processFinished = false;
    }

    consoleBuffer.Draw("Console");

    ImGui::End();
}

GLuint Application::loadImageAsTexture(const char* imagePath) {
    cv::Mat image = cv::imread(imagePath, cv::IMREAD_UNCHANGED);
    cv::cvtColor(image, image, cv::COLOR_BGR2RGBA);

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.cols, image.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    return textureID;
}

void Application::processTask() {
    isProcessing = true;
    processFinished = false;

    if (!skipPdfConversion) {
        consoleBuffer.AddLogMessage(LogLevel::Info, "Iniciando processamento do PDF: " + std::string(filenamePdf));
        processPdf(consoleBuffer, filenamePdf, "Imagens", 300);
        consoleBuffer.AddLogMessage(LogLevel::Info, "Processamento de PDF concluido.");
    }

    if (!skipPdfAlignment) {
        consoleBuffer.AddLogMessage(LogLevel::Info, "Iniciando processamento de alinhamento de Imagens");
        alinharImagens(consoleBuffer, "Imagens", "ImagensAlinhadas", referenceImage);
        consoleBuffer.AddLogMessage(LogLevel::Info, "Processamento de alinhamento de Imagens concluido.");
    }

    if (!skipNoiseReduction) {
        consoleBuffer.AddLogMessage(LogLevel::Info, "Iniciando processamento de Reducao de Ruido");
        aplicarFiltroReducaoRuido(consoleBuffer, "ImagensAlinhadas", "ImagensSemRuidos");
        consoleBuffer.AddLogMessage(LogLevel::Info, "Processamento de Reducao de Ruído concluido.");
    }

    if (!skipContourExtraction) {
        consoleBuffer.AddLogMessage(LogLevel::Info, "Iniciando processamento de Extracao de Contornos");
        extrairContornos(consoleBuffer, "ImagensSemRuidos", "Contornos", "ImagemThreshold");
        consoleBuffer.AddLogMessage(LogLevel::Info, "Processamento de Extracao de Contornos concluido.");
    }

    if (!skipBinarize) {
        consoleBuffer.AddLogMessage(LogLevel::Info, "Iniciando processamento de Binarização de Imagem");
        BinarizarDinamico(consoleBuffer, "ImagensSemRuidos", "ImagemBinarizadas");
        consoleBuffer.AddLogMessage(LogLevel::Info, "Processamento de Extracao de Contornos concluido.");
    }

    if (!skipReadAnswers) {
        consoleBuffer.AddLogMessage(LogLevel::Info, "Iniciando leitura de respostas");
        processImagesAndReadAnswers(consoleBuffer, "ImagemBinarizadas", coordinatesFilePath, "Respostas");
        consoleBuffer.AddLogMessage(LogLevel::Info, "Leitura de respostas concluida.");
    }

    if (!skipReadWords) {
        consoleBuffer.AddLogMessage(LogLevel::Info, "Iniciando leitura de palavras");
        processImagesAndExtractWords(consoleBuffer, "ImagemThreshold", coordinatesFilePath, "Respostas1");
        consoleBuffer.AddLogMessage(LogLevel::Info, "Leitura de palavras concluida.");
    }

        juntarRespostasEmTXT(consoleBuffer, "Respostas", "Resposta");




    isProcessing = false;
    processFinished = true;
}

void Application::renderReferenceImageWindow() {
    ImGui::Begin("Reference Image", &showReferenceImageWindow, ImGuiWindowFlags_NoCollapse);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    cv::Mat img = cv::imread(referenceImage);
    if (!img.empty()) {
        if (originalImageSize.width == 0 && originalImageSize.height == 0) {
            originalImageSize = img.size(); // Armazena o tamanho original da imagem
        }

        float aspectRatio = static_cast<float>(img.rows) / img.cols;
        ImVec2 imageSize(avail.x, avail.x * aspectRatio);

        ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(referenceImageTexture)), imageSize);

        ImVec2 imagePos = ImGui::GetCursorScreenPos();
        handleRectangleDrawing(imagePos, imageSize);

        // Desenha todos os retângulos armazenados
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        for (const auto& rectData : rectangles) {
            const auto& rect = rectData.coordinates;
            ImVec2 p1(imagePos.x + rect.x * imageSize.x, imagePos.y + ( rect.y - 1.0f) * imageSize.y);
            ImVec2 p2(imagePos.x + rect.z * imageSize.x, imagePos.y + ( rect.w - 1.0f) * imageSize.y);
            draw_list->AddRect(p1, p2, IM_COL32(255, 0, 0, 255));

            // Desenha subdivisões
            int lines = rectData.subdivisions.first;
            int columns = rectData.subdivisions.second;
            float cellWidth = (p2.x - p1.x) / columns;
            float cellHeight = (p2.y - p1.y) / lines;
            for (int l = 1; l < lines; ++l) {
                draw_list->AddLine(ImVec2(p1.x, p1.y + l * cellHeight), ImVec2(p2.x, p1.y + l * cellHeight), IM_COL32(255, 0, 0, 255));
            }
            for (int c = 1; c < columns; ++c) {
                draw_list->AddLine(ImVec2(p1.x + c * cellWidth, p1.y), ImVec2(p1.x + c * cellWidth, p2.y), IM_COL32(255, 0, 0, 255));
            }
        }

        // Desenha o retângulo atual
        if (currentRectangle.z != 0 && currentRectangle.w != 0) {
            ImVec2 p1(imagePos.x + currentRectangle.x * imageSize.x, imagePos.y + ( currentRectangle.y - 1.0f) * imageSize.y);
            ImVec2 p2(imagePos.x + currentRectangle.z * imageSize.x, imagePos.y + ( currentRectangle.w - 1.0f) * imageSize.y);
            draw_list->AddRect(p1, p2, IM_COL32(255, 0, 0, 255));
        }
    }
    else {
        ImGui::Text("Failed to load the image.");
    }

    ImGui::End();
}

void Application::handleRectangleDrawing(const ImVec2& imagePos, const ImVec2& imageSize) {
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Inicia o processo de desenho se startDrawing estiver ativado e o mouse estiver sobre a imagem
    if (startDrawing && ImGui::IsItemHovered()) {
        // Quando o botão esquerdo do mouse é pressionado, inicia o desenho do retângulo
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            isDrawing = true;
            rectStart = io.MousePos;
            rectEnd = rectStart;
        }

        // Enquanto o botão esquerdo do mouse estiver sendo arrastado, atualiza o ponto final do retângulo
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && isDrawing) {
            rectEnd = io.MousePos;
        }

        // Quando o botão esquerdo do mouse é liberado, finaliza o desenho do retângulo
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && isDrawing) {
            isDrawing = false;
            startDrawing = false;

            // Calcula as coordenadas normalizadas e ajusta o eixo Y para coordenadas OpenCV
            currentRectangle = ImVec4(
                (rectStart.x - imagePos.x) / imageSize.x,
                1.0f + (rectStart.y - imagePos.y) / imageSize.y,
                (rectEnd.x - imagePos.x) / imageSize.x,
                1.0f + (rectEnd.y - imagePos.y) / imageSize.y
            );

            // Adiciona o retângulo desenhado à lista de retângulos
            rectangles.push_back({ currentRectangle, {1, 1}, "Rectangle " + std::to_string(rectangles.size()) });
        }
    }

    // Se o desenho está em andamento, desenha o retângulo atual na tela
    if (isDrawing) {
        draw_list->AddRect(rectStart, rectEnd, IM_COL32(0, 255, 0, 255));
    }
}

void Application::renderRectanglePropertiesWindow(bool* p_open) {
    ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Rectangle Properties", p_open)) {
        // General modifications
        ImGui::Text("General Modifications");
        if (ImGui::Button("Start Drawing")) {
            startDrawing = true;
            isDrawing = false; // Reinicia o estado de desenho
            currentRectangle = ImVec4(0, 0, 0, 0); // Limpa o retângulo atual
        }

        // Novo botão para salvar as coordenadas dos retângulos
        if (ImGui::Button("Save Rectangles to File")) {
            const char* filterPatterns[1] = { "*.txt" };
            const char* filename = tinyfd_saveFileDialog("Save File", "rectangles.txt", 1, filterPatterns, NULL);
            if (filename) {
                saveRectanglesToFile(filename);
            }
        }

        // Novo botão para carregar as coordenadas dos retângulos
        if (ImGui::Button("Load Rectangles from File")) {
            const char* filterPatterns[1] = { "*.txt" };
            const char* filename = tinyfd_openFileDialog("Select File to Load", "", 1, filterPatterns, NULL, 0);
            if (filename) {
                loadRectanglesFromFile(filename);
            }
        }

        ImGui::Separator();

        // Left pane
        static int selected = -1;
        {
            ImGui::BeginChild("left pane", ImVec2(150, 0), true);
            for (int i = 0; i < rectangles.size(); i++) {
                char label[128];
                sprintf(label, "%s", rectangles[i].name.c_str());
                if (ImGui::Selectable(label, selected == i))
                    selected = i;

                ImGui::SameLine();
                char renameLabel[128];
                sprintf(renameLabel, "##Rename%d", i);
                ImGui::InputText(renameLabel, &rectangles[i].name[0], rectangles[i].name.size() + 1);
            }
            ImGui::EndChild();
        }
        ImGui::SameLine();

        // Right pane
        {
            ImGui::BeginGroup();
            ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
            ImGui::Text("Rectangle: %d", selected);
            ImGui::Separator();
            if (selected >= 0 && selected < rectangles.size()) {
                ImVec4& rect = rectangles[selected].coordinates;
                ImGui::Text("Coordinates: (%.2f, %.2f) - (%.2f, %.2f)", rect.x, rect.y, rect.z, rect.w);

                int& lines = rectangles[selected].subdivisions.first;
                int& columns = rectangles[selected].subdivisions.second;
                ImGui::SliderInt("Lines", &lines, 1, 20);    // Aumenta o limite dos sliders
                ImGui::SliderInt("Columns", &columns, 1, 20); // Aumenta o limite dos sliders

                bool& analyzeVertical = rectangles[selected].analyzeVertical;
                bool& isWord = rectangles[selected].isWord;
                bool& isNumber = rectangles[selected].isNumber;
                ImGui::Checkbox("Analyze Vertical", &analyzeVertical); // Checkbox para análise vertical
                ImGui::Checkbox("Is word", &isWord);
                ImGui::Checkbox("Is Number ", &isNumber);

                if (ImGui::Button("Delete")) {
                    rectangles.erase(rectangles.begin() + selected);
                    selected = -1; // Reseta a seleção após deletar
                }
                ImGui::SameLine();
                if (ImGui::Button("Modify")) {
                    startDrawing = true;
                    isDrawing = false; // Reinicia o estado de desenho
                    currentRectangle = rect; // Usa as coordenadas do retângulo selecionado
                    rectangles.erase(rectangles.begin() + selected);
                    selected = -1; // Reseta a seleção para iniciar o desenho modificado
                }
            }
            ImGui::EndChild();
            ImGui::EndGroup();
        }

        ImGui::End();
    }
}

void Application::saveRectanglesToFile(const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile) {
        consoleBuffer.AddLogMessage(LogLevel::Error, "Failed to open file for writing: " + filename);
        return;
    }

    for (const auto& rectData : rectangles) {
        outFile << rectData.name << "| "  // Usa '|' como delimitador para o nome
            << rectData.coordinates.x << " "
            << rectData.coordinates.y << " "
            << rectData.coordinates.z << " "
            << rectData.coordinates.w << " "
            << rectData.subdivisions.first << " "
            << rectData.subdivisions.second << " "
            << rectData.analyzeVertical << " "
            << rectData.isWord << " "
            << rectData.isNumber << "\n";
    }

    outFile.close();
    if (!outFile.fail()) {
        consoleBuffer.AddLogMessage(LogLevel::Info, "Rectangles saved successfully to " + filename);
    }
    else {
        consoleBuffer.AddLogMessage(LogLevel::Error, "Error occurred while saving to " + filename);
    }
}

void Application::loadRectanglesFromFile(const std::string& filename) {
    std::ifstream inFile(filename);
    if (!inFile) {
        consoleBuffer.AddLogMessage(LogLevel::Error, "Failed to open file for reading: " + filename);
        return;
    }

    rectangles.clear();
    std::string name;
    float x, y, z, w;
    int lines, columns;
    bool analyzeVertical;
    bool isWord;
    bool isNumber;
    while (std::getline(inFile, name, '|')) { // Usa '|' como delimitador para o nome
        inFile >> x >> y >> z >> w >> lines >> columns >> analyzeVertical >> isWord >> isNumber;
        inFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Ignora o resto da linha
        rectangles.push_back({ ImVec4(x, y, z, w), {lines, columns}, name, analyzeVertical, isWord, isNumber });
    }

    inFile.close();
    if (!inFile.fail()) {
        consoleBuffer.AddLogMessage(LogLevel::Info, "Rectangles loaded successfully from " + filename);
    }
    else {
        consoleBuffer.AddLogMessage(LogLevel::Error, "Error occurred while loading from " + filename);
    }
}


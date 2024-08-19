#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <mutex>
#include <queue>
#include <imgui.h>

enum class LogLevel {
    Info,
    Warning,
    Error
};

struct LogEntry {
    std::string message;
    LogLevel level;
};

// Estrutura para enfileirar mensagens com nível de log
struct LogMessage {
    std::string message;
    LogLevel level;
};

class ConsoleBuffer {
public:
    ConsoleBuffer() {}

    // Função thread-safe para adicionar mensagens à fila
    void AddLogMessage(LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        logQueue.push(LogEntry{ message, level });
    }

    void Draw(const char* title) {
        ProcessLogs(); // Assegura que todos os logs sejam processados antes de desenhar
        if (ImGui::Begin(title)) {
            for (const auto& entry : logs) {
                ImVec4 color;
                switch (entry.level) {
                case LogLevel::Info: color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
                case LogLevel::Warning: color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break;
                case LogLevel::Error: color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;
                }
                ImGui::TextColored(color, "%s", entry.message.c_str());
            }
        }
        ImGui::End();
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(logMutex);
        logs.clear();
    }

private:
    std::vector<LogEntry> logs;
    std::queue<LogEntry> logQueue;
    std::mutex logMutex;

    // Processa a fila de logs
    void ProcessLogs() {
        std::lock_guard<std::mutex> lock(logMutex);
        while (!logQueue.empty()) {
            logs.push_back(logQueue.front());
            logQueue.pop();
        }
    }
};

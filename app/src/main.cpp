#include <iostream>
#include <fstream>
#include <cstdlib> // para getenv
#include <string>

int main() {
    const char* logLevel = std::getenv("LOG_LEVEL");
    const char* dataDir = std::getenv("DATA_DIR");

    std::string logPath = std::string(dataDir ? dataDir : ".") + "/log.txt";
    std::ofstream logFile(logPath, std::ios::app);

    if (logFile.is_open()) {
        logFile << "[INFO] Programa iniciado com LOG_LEVEL=" 
                << (logLevel ? logLevel : "NULO") << std::endl;
        logFile.close();
    }

    std::cout << "Log gravado em " << logPath << std::endl;
    return 0;
}

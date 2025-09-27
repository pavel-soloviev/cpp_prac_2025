#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

// Enum to represent log levels
enum class Loglevel { DEBUG, INFO, WARNING, ERROR, CRITICAL };

class Logger {
public:
    // Constructor: Opens the log file in append mode
    Logger(const std::string& filename)
    {
        logsDir = std::filesystem::current_path() / "logs";
        std::filesystem::create_directory(logsDir);
        logFile.open(logsDir / filename, std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "Error opening log file." << std::endl;
        }
    }

    // Destructor: Closes the log file
    ~Logger() { logFile.close(); }

    // Logs a message with a given log level
    void log(Loglevel level, const std::string& message)
    {
        // Get current timestamp
        // time_t now = time(0);
        // tm* timeinfo = localtime(&now);
        // char timestamp[20];
        // strftime(timestamp, sizeof(timestamp),
        //          "%Y-%m-%d %H:%M:%S", timeinfo);

        // Create log entry
        std::ostringstream logEntry;
        // logEntry << "[" << timestamp << "] "
        logEntry << levelToString(level) << ": " << message
                 << std::endl;

#ifdef DEBUG_MODE
        // Output to console
        std::cout << logEntry.str();
#endif

        // Output to log file
        if (logFile.is_open()) {
            logFile << logEntry.str();
            logFile.flush(); // Ensure immediate write to file
        }
    }

private:
    std::filesystem::path logsDir;
    std::ofstream logFile; // File stream for the log file

    // Converts log level to a string for output
    std::string levelToString(Loglevel level)
    {
        switch (level) {
            case Loglevel::DEBUG:
                return "DEBUG";
            case Loglevel::INFO:
                return "INFO";
            case Loglevel::WARNING:
                return "WARNING";
            case Loglevel::ERROR:
                return "ERROR";
            case Loglevel::CRITICAL:
                return "CRITICAL";
            default:
                return "UNKNOWN";
        }
    }
};

// int main() {
//     Logger logger("logfile.txt"); // Create logger instance
//     // Example usage of the logger
//     logger.log(Loglevel::INFO, "Program started.");
//     logger.log(Loglevel::DEBUG, "Debugging information.");
//     logger.log(Loglevel::ERROR, "An error occurred.");
//     return 0;
// }
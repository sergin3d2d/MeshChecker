#include "Logger.h"

std::ofstream Logger::logfile;

void Logger::init(const std::string& filename)
{
    logfile.open(filename, std::ios_base::app);
}

void Logger::log(const std::string& message)
{
    if (logfile.is_open()) {
        logfile << message << std::endl;
    }
}

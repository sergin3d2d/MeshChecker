#include "Logger.h"
#include <QString>

Logger& Logger::getInstance()
{
    static Logger instance;
    return instance;
}

void Logger::init(const std::string& filename)
{
    logfile.open(filename, std::ios_base::app);
}

void Logger::log(const std::string& message)
{
    if (logfile.is_open()) {
        logfile << message << std::endl;
    }
    emit messageLogged(QString::fromStdString(message));
}

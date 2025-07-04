#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <QObject>

class Logger : public QObject
{
    Q_OBJECT
public:
    static Logger& getInstance();
    void init(const std::string& filename);
    void log(const std::string& message);

signals:
    void messageLogged(const QString& message);

private:
    Logger() {}
    Logger(const Logger&) = delete;
    void operator=(const Logger&) = delete;

    std::ofstream logfile;
};

#endif // LOGGER_H

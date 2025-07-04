#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>

class Logger
{
public:
    static void init(const std::string& filename);
    static void log(const std::string& message);

private:
    static std::ofstream logfile;
};

#endif // LOGGER_H

#ifndef TPXSERVER_LOGGER_H
#define TPXSERVER_LOGGER_H

#include <string>

namespace logger {

    void log(const std::string &msg);
    void warn(const std::string &msg);
    void err(const std::string &msg);

}

#endif //TPXSERVER_LOGGER_H

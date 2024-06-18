#ifndef TIMEPIXCOMMANDINFO_H
#define TIMEPIXCOMMANDINFO_H

#include "common_defs.h"
#include "TimepixCodes.h"
#include "ServerCodes.h"

class PythonConnectionManager;

struct TimepixCommandInfo {
    TpxCommand command;
    DataVec data;
    PythonConnectionManager *sender;
};

#endif // TIMEPIXCOMMANDINFO_H

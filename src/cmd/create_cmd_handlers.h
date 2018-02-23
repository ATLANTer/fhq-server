#ifndef CREATE_CMD_HANDLERS_H
#define CREATE_CMD_HANDLERS_H

#include <icmdhandler.h>
#include <QString>
#include <QMap>

void create_cmd_handlers(QMap<std::string, ICmdHandler *> &pHandlers);

#endif // CREATE_CMD_HANDLERS_H

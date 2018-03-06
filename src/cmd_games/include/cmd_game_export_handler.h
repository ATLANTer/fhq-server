#ifndef CMD_GAME_EXPORT_HANDLER_H
#define CMD_GAME_EXPORT_HANDLER_H

#include <iserver.h>

class CmdGameExportHandler : public ICmdHandler {
    public:
        CmdGameExportHandler();
        virtual std::string cmd();
        virtual bool accessUnauthorized();
        virtual bool accessUser();
        virtual bool accessTester();
        virtual bool accessAdmin();
        virtual const std::vector<CmdInputDef> &inputs();
        virtual QString description();
        virtual QStringList errors();
        virtual void handle(ModelRequest *pRequest);

    private:
        std::vector<CmdInputDef> m_vInputs;
        QString TAG;
};

#endif // CMD_GAME_EXPORT_HANDLER_H

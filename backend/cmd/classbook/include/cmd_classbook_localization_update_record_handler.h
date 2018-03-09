#ifndef CMD_CLASSBOOK_LOCALIZATION_UPDATE_RECORD_HANDLER_H
#define CMD_CLASSBOOK_LOCALIZATION_UPDATE_RECORD_HANDLER_H

#include <iserver.h>
#include <cmd_handlers.h>

class CmdClassbookLocalizationUpdateRecordHandler : public ICmdHandler {

    public:
        CmdClassbookLocalizationUpdateRecordHandler();
        virtual std::string cmd();
        virtual std::string description();
        virtual const ModelCommandAccess &access();
        virtual const std::vector<CmdInputDef> &inputs();
        virtual void handle(ModelRequest *pRequest);

    private:
        QString TAG;
        ModelCommandAccess m_modelCommandAccess;
        std::vector<CmdInputDef> m_vInputs;
};

REGISTRY_CMD(CmdClassbookLocalizationUpdateRecordHandler)

#endif // CMD_CLASSBOOK_LOCALIZATION_UPDATE_RECORD_HANDLER_H


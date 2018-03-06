#ifndef CMD_CLASSBOOK_ADD_RECORD_HANDLER
#define CMD_CLASSBOOK_ADD_RECORD_HANDLER

#include <iserver.h>

class CmdClassbookAddRecordHandler : public ICmdHandler {

    public:
        CmdClassbookAddRecordHandler();
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
        QString TAG;
        std::vector<CmdInputDef> m_vInputs;
};

#endif // CMD_CLASSBOOK_ADD_RECORD_HANDLER


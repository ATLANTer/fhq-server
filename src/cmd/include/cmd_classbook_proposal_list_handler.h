#ifndef CMD_CLASSBOOK_PROPOSAL_LIST_HANDLER_H
#define CMD_CLASSBOOK_PROPOSAL_LIST_HANDLER_H

#include <iserver.h>
#include <cmd_handlers.h>

class CmdClassbookProposalListHandler : public ICmdHandler {

    public:
        CmdClassbookProposalListHandler();
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

#endif // CMD_CLASSBOOK_PROPOSAL_LIST_HANDLER_H


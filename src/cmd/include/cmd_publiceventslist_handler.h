#ifndef CMD_PUBLICEVENTSLIST_HANDLER_H
#define CMD_PUBLICEVENTSLIST_HANDLER_H

#include <iserver.h>
#include <cmd_handlers.h>

class CmdPublicEventsListHandler : public ICmdHandler {
	
	public:
		CmdPublicEventsListHandler();
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

#endif // CMD_PUBLICEVENTSLIST_HANDLER_H

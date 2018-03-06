#ifndef CMD_DELETEPUBLICEVENT_HANDLER_H
#define CMD_DELETEPUBLICEVENT_HANDLER_H

#include <iserver.h>

class CmdDeletePublicEventHandler : public ICmdHandler {
	
	public:
		CmdDeletePublicEventHandler();
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

#endif // CMD_DELETEPUBLICEVENT_HANDLER_H

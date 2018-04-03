#ifndef CMD_UPDATE_USER_LOCATION_HANDLER_H
#define CMD_UPDATE_USER_LOCATION_HANDLER_H

#include <iserver.h>
#include <cmd_handlers.h>

class CmdUpdateUserLocationHandler : public ICmdHandler {
	
	public:
		CmdUpdateUserLocationHandler();
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

REGISTRY_CMD(CmdUpdateUserLocationHandler)

#endif // CMD_UPDATE_USER_LOCATION_HANDLER_H

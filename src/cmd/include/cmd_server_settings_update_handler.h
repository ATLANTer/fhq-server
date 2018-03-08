#ifndef CMD_SERVER_SETTINGS_UPDATE_HANDLER_H
#define CMD_SERVER_SETTINGS_UPDATE_HANDLER_H

#include <iserver.h>
#include <cmd_handlers.h>

class CmdServerSettingsUpdateHandler : public ICmdHandler {
	
	public:
        CmdServerSettingsUpdateHandler();
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

#endif // CMD_SERVER_SETTINGS_UPDATE_HANDLER_H

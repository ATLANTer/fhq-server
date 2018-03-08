#ifndef CMD_QUEST_STATISTICS_HANDLER_H
#define CMD_QUEST_STATISTICS_HANDLER_H

#include <iserver.h>
#include <cmd_handlers.h>

class CmdQuestStatisticsHandler : public ICmdHandler {
	
	public:
        CmdQuestStatisticsHandler();
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

REGISTRY_CMD(CmdQuestStatisticsHandler)

#endif // CMD_QUEST_STATISTICS_HANDLER_H

#ifndef CMD_HADNLERS_H
#define CMD_HADNLERS_H

#include <map>
#include <utils_logger.h>
#include <model_request.h>
#include <model_command_access.h>
#include <cmd_input_def.h>

class CmdHandlerBase {

    public:
        CmdHandlerBase(const std::string &sCmd, const std::string &sDescription);
        virtual std::string cmd();
        virtual std::string description();
        std::string activatedFromVersion();
        std::string deprecatedFromVersion();

        virtual const ModelCommandAccess &access();
        virtual const std::vector<CmdInputDef> &inputs();
        virtual void handle(ModelRequest *pRequest) = 0;

        // virtual void success(nlohmann::json jsonResponse);
        // virtual void error(int nCode, const std::string &sErrorMessage);

    protected:
        void setAccessUnauthorized(bool bAccess);
        void setAccessUser(bool bAccess);
        void setAccessAdmin(bool bAccess);
        void setActivatedFromVersion(const std::string &sActivatedFromVersion);
        void setDeprecatedFromVersion(const std::string &sDeprecatedFromVersion);

        std::string TAG;
        std::string m_sCmd;
        std::string m_sDescription;

        ModelCommandAccess m_modelCommandAccess;
        std::vector<CmdInputDef> m_vInputs;

    private:
        std::string m_sActivatedFromVersion;
        std::string m_sDeprecatedFromVersion;
};

extern std::map<std::string, CmdHandlerBase*> *g_pCmdHandlers;

class CmdHandlers {
    public:
        static void initGlobalVariables();
        static void addHandler(const std::string &sName, CmdHandlerBase* pCmdHandler);
        static CmdHandlerBase *findCmdHandler(const std::string &sCmd);
};

// RegistryCmdHandler
#define REGISTRY_CMD( classname ) \
    static classname * pRegistry ## classname = new classname(); \


#endif // CMD_HADNLERS_H

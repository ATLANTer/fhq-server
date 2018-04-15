#include <cmd_lxd_containers.h>
#include <employ_orchestra.h>
#include <model_lxd_container.h>

#include <string>
#include <QJsonValue>

CmdLXDContainersHandler::CmdLXDContainersHandler(){

    m_modelCommandAccess.setAccessUnauthorized(false);
    m_modelCommandAccess.setAccessUser(false);
    m_modelCommandAccess.setAccessAdmin(true);

    // validation and description input fields
    m_vInputs.push_back(CmdInputDef("name").string_().required().description("Container name"));
    m_vInputs.push_back(CmdInputDef("action").string_().required().description("Actions: create, start, stop and delete container"));
}

// ---------------------------------------------------------------------

std::string CmdLXDContainersHandler::cmd(){
    return "lxd_containers";
}
// ---------------------------------------------------------------------

const ModelCommandAccess & CmdLXDContainersHandler::access(){
    return m_modelCommandAccess;
}

// ---------------------------------------------------------------------

const std::vector<CmdInputDef> &CmdLXDContainersHandler::inputs(){
    return m_vInputs;
}

std::string CmdLXDContainersHandler::description(){
    return "Any actions with the container. Actions: create, start, stop and delete container";
}

// ---------------------------------------------------------------------

void CmdLXDContainersHandler::handle(ModelRequest *pRequest){
    using namespace std;
    QJsonObject jsonRequest = pRequest->data();
    QJsonObject jsonResponse;

    QSqlDatabase db = *(pRequest->server()->database());

    string name = jsonRequest["name"].toString().trimmed().toStdString();
    string error, response;

    //TO DO
    //Action switch
    EmployOrchestra *pOrchestra = findEmploy<EmployOrchestra>();
    pOrchestra->initSettings();
    if(jsonRequest["action"] == "create"){
        LXDContainer container = pOrchestra->create_container(name);
        if(!(container.get_error() == "")){
            jsonResponse["error"] = QJsonValue(QString::fromStdString(container.get_error()));
        }
    }
    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}


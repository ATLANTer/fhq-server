#include "cmd_handlers_useful_links.h"
#include <utils_logger.h>
#include <utils.h>
#include <md5.h>
#include <employ_settings.h>
#include <employ_database.h>
#include <employ_server_info.h>
#include <employ_notify.h>

/*********************************************
 * Useful Links List
**********************************************/

REGISTRY_CMD(CmdHandlerUsefulLinksList)

CmdHandlerUsefulLinksList::CmdHandlerUsefulLinksList()
    : CmdHandlerBase("useful_links_list", "Method will be return list of useful links"){

    m_modelCommandAccess.setAccessUnauthorized(true);
    m_modelCommandAccess.setAccessUser(true);
    m_modelCommandAccess.setAccessAdmin(true);

    // validation and description input fields
    m_vInputs.push_back(CmdInputDef("filter").required().string_().description("Filter by word"));
}


// ---------------------------------------------------------------------

void CmdHandlerUsefulLinksList::handle(ModelRequest *pRequest){
    nlohmann::json jsonRequest = pRequest->jsonRequest();

    IUserToken *pUserToken = pRequest->userToken();
    bool bIsAdmin = false;
    if (pUserToken != NULL) {
        bIsAdmin = pUserToken->isAdmin();
    }

    std::string sFilter = "";
    if (jsonRequest["filter"].is_string()) {
        sFilter = jsonRequest["filter"];
    }

    auto jsonData = nlohmann::json::array();
    QString sWhere = "";
    if (!bIsAdmin) {
        sWhere = " WHERE status = 'ok' ";
    }

    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();
    QSqlDatabase db = *(pDatabase->database());
    QSqlQuery query(db);
    // TODO paginator
    query.prepare("SELECT * FROM useful_links " + sWhere + " ORDER BY stars, dt DESC LIMIT 0,25");

    if (!query.exec()) {
        pRequest->sendMessageError(cmd(), Error(500, query.lastError().text().toStdString()));
        return;
    }

    while (query.next()) {
        QSqlRecord record = query.record();
        nlohmann::json jsonLink;
        jsonLink["url"] = record.value("url").toString().toHtmlEscaped().toStdString();
        jsonLink["description"] = record.value("description").toString().toHtmlEscaped().toStdString();
        jsonLink["author"] = record.value("message").toString().toHtmlEscaped().toStdString();
        jsonLink["stars"] = record.value("stars").toInt();
        jsonLink["dt"] = record.value("dt").toString().toStdString();
        jsonData.push_back(jsonLink);
    }
    
    nlohmann::json jsonResponse;
    jsonResponse["data"] = jsonData;
    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

/*********************************************
 * Useful Links Add
**********************************************/

REGISTRY_CMD(CmdHandlerUsefulLinksAdd)

CmdHandlerUsefulLinksAdd::CmdHandlerUsefulLinksAdd()
    : CmdHandlerBase("useful_links_add", "Method for add new useful link") {

    m_modelCommandAccess.setAccessUnauthorized(false);
    m_modelCommandAccess.setAccessUser(true);
    m_modelCommandAccess.setAccessAdmin(true);

    // validation and description input fields
    m_vInputs.push_back(CmdInputDef("url").required().string_().description("URL"));
    m_vInputs.push_back(CmdInputDef("description").required().string_().description("Description"));
    m_vInputs.push_back(CmdInputDef("author").required().string_().description("Author"));
}

// ---------------------------------------------------------------------

void CmdHandlerUsefulLinksAdd::handle(ModelRequest *pRequest){
    
}

/*********************************************
 * Useful Links Delete
**********************************************/

REGISTRY_CMD(CmdHandlerUsefulLinksDelete)

CmdHandlerUsefulLinksDelete::CmdHandlerUsefulLinksDelete()
    : CmdHandlerBase("useful_links_delete", "Method for delete link by admin") {

    m_modelCommandAccess.setAccessUnauthorized(false);
    m_modelCommandAccess.setAccessUser(false);
    m_modelCommandAccess.setAccessAdmin(true);

    // validation and description input fields
    m_vInputs.push_back(CmdInputDef("url").required().string_().description("URL"));
    m_vInputs.push_back(CmdInputDef("description").required().string_().description("Description"));
    m_vInputs.push_back(CmdInputDef("author").required().string_().description("Author"));
}

// ---------------------------------------------------------------------

void CmdHandlerUsefulLinksDelete::handle(ModelRequest *pRequest){
    
}

/*********************************************
 * Useful Links Update
**********************************************/

REGISTRY_CMD(CmdHandlerUsefulLinksUpdate)

CmdHandlerUsefulLinksUpdate::CmdHandlerUsefulLinksUpdate()
    : CmdHandlerBase("useful_links_update", "Method for update useful link"){

    m_modelCommandAccess.setAccessUnauthorized(false);
    m_modelCommandAccess.setAccessUser(false);
    m_modelCommandAccess.setAccessAdmin(true);

    // validation and description input fields
    m_vInputs.push_back(CmdInputDef("url").required().string_().description("URL"));
    m_vInputs.push_back(CmdInputDef("description").required().string_().description("Description"));
    m_vInputs.push_back(CmdInputDef("author").required().string_().description("Author"));
    m_vInputs.push_back(CmdInputDef("status").required().string_().description("Status"));
}

// ---------------------------------------------------------------------

void CmdHandlerUsefulLinksUpdate::handle(ModelRequest *pRequest){
    
}

/*********************************************
 * Useful Links Star
**********************************************/

REGISTRY_CMD(CmdHandlerUsefulLinksStar)

CmdHandlerUsefulLinksStar::CmdHandlerUsefulLinksStar()
    : CmdHandlerBase("useful_links_star", "Method set star"){

    m_modelCommandAccess.setAccessUnauthorized(false);
    m_modelCommandAccess.setAccessUser(true);
    m_modelCommandAccess.setAccessAdmin(true);

    // validation and description input fields
}

// ---------------------------------------------------------------------

void CmdHandlerUsefulLinksStar::handle(ModelRequest *pRequest){
    
}

/*********************************************
 * Useful Links Unstar
**********************************************/

REGISTRY_CMD(CmdHandlerUsefulLinksUnstar)

CmdHandlerUsefulLinksUnstar::CmdHandlerUsefulLinksUnstar()
    : CmdHandlerBase("useful_links_unstar", "Method set unstar"){

    m_modelCommandAccess.setAccessUnauthorized(false);
    m_modelCommandAccess.setAccessUser(true);
    m_modelCommandAccess.setAccessAdmin(true);

    // validation and description input fields
}

// ---------------------------------------------------------------------

void CmdHandlerUsefulLinksUnstar::handle(ModelRequest *pRequest){
    
}

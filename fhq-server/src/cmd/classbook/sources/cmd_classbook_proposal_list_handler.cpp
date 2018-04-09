#include <cmd_classbook_proposal_list_handler.h>
#include <QJsonArray>
#include <QSqlError>
#include <QMap>
#include <log.h>
#include <employ_database.h>

CmdClassbookProposalListHandler::CmdClassbookProposalListHandler(){

    m_modelCommandAccess.setAccessUnauthorized(false);
    m_modelCommandAccess.setAccessUser(true);
    m_modelCommandAccess.setAccessAdmin(true);

    // validation and description input fields
    m_vInputs.push_back(CmdInputDef("classbookid").optional().integer_().description("Classbookid for an article"));
    m_vInputs.push_back(CmdInputDef("lang").optional().string_().description("Language"));
}

// ---------------------------------------------------------------------

std::string CmdClassbookProposalListHandler::cmd(){
    return "classbook_proposal_list";
}

// ---------------------------------------------------------------------

const ModelCommandAccess & CmdClassbookProposalListHandler::access(){
    return m_modelCommandAccess;
}

// ---------------------------------------------------------------------

const std::vector<CmdInputDef> &CmdClassbookProposalListHandler::inputs(){
    return m_vInputs;
}

// ---------------------------------------------------------------------

std::string CmdClassbookProposalListHandler::description(){
    return "Display list of proposals by classbookid";
}

// ---------------------------------------------------------------------

void CmdClassbookProposalListHandler::handle(ModelRequest *pRequest){
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();
    QJsonObject jsonRequest = pRequest->data();
    QJsonObject jsonResponse;

    QSqlDatabase db = *(pDatabase->database());

    QSqlQuery query(db);

    QJsonObject item;
    QJsonArray data;

    QString sQuery;
    QMap<QString, QJsonValue> mapFilter;

    //checkout and validation of classbookid
    if(jsonRequest.contains("classbookid")){
        query.prepare("SELECT id FROM classbook WHERE id = :classbookid");
        query.bindValue(":classbookid", jsonRequest["classbookid"].toInt());
        if(!query.exec()){
            pRequest->sendMessageError(cmd(), Error(500, query.lastError().text()));
            return;
        }
        if(!query.next()){
            pRequest->sendMessageError(cmd(), Error(404, "This article doesn't exist"));
            return;
        }
        mapFilter.insert("classbookid", jsonRequest["classbookid"].toInt());
    }

    //checkout of lang and generation of query's bone
    if(jsonRequest.contains("lang")){
        mapFilter.insert("lang", jsonRequest["lang"].toString().trimmed());
        sQuery = "SELECT id, name FROM classbook_proposal";
    }
    else sQuery = "SELECT id, name, lang FROM classbook_proposal";

    //generation of the rest of the query
    if(mapFilter.size() > 0) sQuery += " WHERE ";
    bool bFirst = true;
    foreach(QString key, mapFilter.keys()){
        if(!bFirst) sQuery += " AND ";
        bFirst = false;
        sQuery +=  key + " = :" + key;
    }
    query.prepare(sQuery);

    //binding of values
    foreach(QString key, mapFilter.keys()){
        QMap<QString, QJsonValue>::const_iterator v = mapFilter.lowerBound(key);
        if(key=="classbookid")
            query.bindValue(":" + key, v.value().toInt());
        else
            query.bindValue(":" + key, v.value());
    }
    if (!query.exec()){
        pRequest->sendMessageError(cmd(), Error(500, query.lastError().text()));
        return;
    }

    while (query.next()) {
        QSqlRecord record = query.record();
        item["id"] = record.value("id").toInt();
        item["classbookid"] = jsonRequest["classbookid"].toInt();
        if(jsonRequest.contains("lang")){
            item["lang"] = jsonRequest["lang"].toString().trimmed();
        }
        else item["lang"] = record.value("lang").toString().trimmed();
        item["name"] = record.value("name").toString();
        data.push_back(item);
    }

    jsonResponse["data"] = data;
    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

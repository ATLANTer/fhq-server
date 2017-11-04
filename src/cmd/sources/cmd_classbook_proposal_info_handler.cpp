#include <cmd_classbook_proposal_info_handler.h>
#include <QJsonArray>
#include <QSqlError>
#include <log.h>

CmdClassbookProposalInfoHandler::CmdClassbookProposalInfoHandler(){
    m_vInputs.push_back(CmdInputDef("classbook_proposal_id").required().integer_().description("Proposal id"));
}

QString CmdClassbookProposalInfoHandler::cmd(){
    return "classbook_proposal_info";
}

bool CmdClassbookProposalInfoHandler::accessUnauthorized(){
    return true;
}

bool CmdClassbookProposalInfoHandler::accessUser(){
    return true;
}

bool CmdClassbookProposalInfoHandler::accessTester(){
    return true;
}

bool CmdClassbookProposalInfoHandler::accessAdmin(){
    return true;
}

const QVector<CmdInputDef> &CmdClassbookProposalInfoHandler::inputs(){
    return m_vInputs;
};

QString CmdClassbookProposalInfoHandler::description(){
    return "Find and display all proposal data by id";
}

QStringList CmdClassbookProposalInfoHandler::errors(){
    QStringList	list;
    return list;
}

void CmdClassbookProposalInfoHandler::handle(QWebSocket *pClient, IWebSocketServer *pWebSocketServer, QString m, QJsonObject obj){

    QSqlDatabase db = *(pWebSocketServer->database());

    QJsonObject data;

    QSqlQuery query(db);
    QString classbook_proposal_id;
    query.prepare("SELECT classbook_proposal_id FROM classbook_proposal WHERE id = :classbook_proposal_id");
    query.bindValue(":classbook_proposal_id", obj["classbook_proposal_id"].toInt());
    if(!query.exec()){
        pWebSocketServer->sendMessageError(pClient, cmd(), m, Error(500, query.lastError().text()));
        return;
    }
    if(query.next()){
        classbook_proposal_id = obj["classbook_proposal_id"].toInt();
    } else {
        pWebSocketServer->sendMessageError(pClient, cmd(), m, Error(404, "This localization doesn't exist"));
        return;
    }

    query.prepare("SELECT id, classbookid, lang, name, content FROM classbook_proposal WHERE id = :classbook_proposal_id");
    query.bindValue(":classbook_proposal_id", classbook_proposal_id);
    if (!query.exec()){
        pWebSocketServer->sendMessageError(pClient, cmd(), m, Error(500, query.lastError().text()));
        return;
    }
    QSqlRecord record = query.record();
    data["classbookid"] = record.value("classbookid").toInt();
    data["id"] = classbook_proposal_id;
    data["lang"] = record.value("lang").toString();
    data["name"] = record.value("name").toString();
    data["content"] = record.value("content").toString();

    QJsonObject jsonData;
    jsonData["cmd"] = QJsonValue(cmd());
    jsonData["m"] = QJsonValue(m);
    jsonData["result"] = QJsonValue("DONE");
    jsonData["data"] = data;
    pWebSocketServer->sendMessage(pClient, jsonData);
}


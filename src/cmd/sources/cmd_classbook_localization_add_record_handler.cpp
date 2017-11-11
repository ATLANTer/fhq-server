#include <cmd_classbook_localization_add_record_handler.h>
#include <QJsonArray>
#include <QSqlError>
#include <log.h>
#include <QUuid>
#include <QCryptographicHash>

CmdClassbookLocalizationAddRecordHandler::CmdClassbookLocalizationAddRecordHandler(){
    m_vInputs.push_back(CmdInputDef("classbookid").required().integer_().description("Classbookid for article localization"));
    m_vInputs.push_back(CmdInputDef("lang").required().string_().description("Language"));
    m_vInputs.push_back(CmdInputDef("name").required().string_().description("Article name"));
    m_vInputs.push_back(CmdInputDef("content").required().string_().description("The content of the article"));
}

QString CmdClassbookLocalizationAddRecordHandler::cmd(){
    return "classbook_localization_add_record";
}

bool CmdClassbookLocalizationAddRecordHandler::accessUnauthorized(){
    return false;
}

bool CmdClassbookLocalizationAddRecordHandler::accessUser(){
    return false;
}

bool CmdClassbookLocalizationAddRecordHandler::accessTester(){
    return false;
}

bool CmdClassbookLocalizationAddRecordHandler::accessAdmin(){
    return true;
}

const QVector<CmdInputDef> &CmdClassbookLocalizationAddRecordHandler::inputs(){
    return m_vInputs;
};

QString CmdClassbookLocalizationAddRecordHandler::description(){
    return "Add a new article localization for the English version";
}

QStringList CmdClassbookLocalizationAddRecordHandler::errors(){
    QStringList	list;
    return list;
}

void CmdClassbookLocalizationAddRecordHandler::handle(QWebSocket *pClient, IWebSocketServer *pWebSocketServer, QString m, QJsonObject obj){

    int classbookid = obj["classbookid"].toInt();

    QSqlDatabase db = *(pWebSocketServer->database());

    QJsonObject data;

    QSqlQuery query(db);
    QString lang;
    query.prepare("SELECT lang FROM classbook_localization WHERE lang = :lang");
    query.bindValue(":lang", obj["lang"].toString().trimmed());
    if(!query.exec()){
        pWebSocketServer->sendMessageError(pClient, cmd(), m, Error(500, query.lastError().text()));
        return;
    }
    if(!query.next()){
        lang = obj["lang"].toString().trimmed();
    } else {
        pWebSocketServer->sendMessageError(pClient, cmd(), m, Error(403, "This lang already exist"));
        return;
    }

    QString name = obj["name"].toString().trimmed();
    QString content = obj["content"].toString().trimmed();

    //Set md5_content hash
    QString md5_content = QString(QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Md5).toHex());

    //generate uuid
    QString uuid = QUuid::createUuid().toString().replace("{", "").replace("}", "");

    query.prepare("INSERT INTO classbook_localization("
                  "classbookid,"
                  "uuid,"
                  "lang,"
                  "name,"
                  "content,"
                  "md5_content,"
                  "created,"
                  "updated"
                  ") "
                  "VALUES("
                  ":classbookid,"
                  ":uuid"
                  ":lang,"
                  ":name,"
                  ":content,"
                  ":md5_content"
                  "NOW(),"
                  "NOW()"
                  ")");
    query.bindValue(":classbookid", classbookid);
    query.bindValue(":uuid", uuid);
    query.bindValue(":lang", lang);
    query.bindValue(":name", name);
    query.bindValue(":content", content);
    query.bindValue(":md5_content", md5_content);
    if(!query.exec()){
        pWebSocketServer->sendMessageError(pClient, cmd(), m, Error(500, query.lastError().text()));
        return;
    }
    data["classbookid"] = classbookid;
    data["classbook_localizationid"] = QJsonValue(query.lastInsertId().toInt());
    data["lang"] = lang;
    data["name"] = name;
    data["content"] = content;
    data["md5_content"] = md5_content;

    QJsonObject jsonData;
    jsonData["cmd"] = QJsonValue(cmd());
    jsonData["m"] = QJsonValue(m);
    jsonData["result"] = QJsonValue("DONE");
    jsonData["data"] = data;
    pWebSocketServer->sendMessage(pClient, jsonData);
}

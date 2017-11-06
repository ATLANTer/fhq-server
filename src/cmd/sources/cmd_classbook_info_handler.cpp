#include <cmd_classbook_info_handler.h>
#include <QJsonArray>
#include <QSqlError>

CmdClassbookInfoHandler::CmdClassbookInfoHandler(){
    m_vInputs.push_back(CmdInputDef("classbookid").required().integer_().description("id for the classbook article"));
    m_vInputs.push_back(CmdInputDef("lang").optional().string_().description("Set lang for the article"));
}

QString CmdClassbookInfoHandler::cmd(){
    return "classbook_info";
}

bool CmdClassbookInfoHandler::accessUnauthorized(){
    return true;
}

bool CmdClassbookInfoHandler::accessUser(){
    return true;
}

bool CmdClassbookInfoHandler::accessTester(){
    return true;
}

bool CmdClassbookInfoHandler::accessAdmin(){
    return true;
}

const QVector<CmdInputDef> &CmdClassbookInfoHandler::inputs(){
    return m_vInputs;
};

QString CmdClassbookInfoHandler::description(){
    return "Return name and content, langs, path classbook article with a given id";
}

QStringList CmdClassbookInfoHandler::errors(){
    QStringList	list;
    return list;
}

void CmdClassbookInfoHandler::handle(QWebSocket *pClient, IWebSocketServer *pWebSocketServer, QString m, QJsonObject obj){

    QSqlDatabase db = *(pWebSocketServer->database());

    int classbookid = obj["classbookid"].toInt();

    QSqlQuery query(db);
    QJsonObject info;

    //GET parentid and uuid for the article
    query.prepare("SELECT parentid, uuid FROM classbook WHERE id=:classbookid");
    query.bindValue(":classbookid", classbookid);
    query.exec();
    if (query.next()) {
        QSqlRecord record = query.record();
        info["classbookid"] = classbookid;
        info["parentid"] = record.value("parentid").toInt();
        info["uuid"] = record.value("uuid").toString();
    } else {
        pWebSocketServer->sendMessageError(pClient, cmd(), m, Errors::NotFound("the article"));
        return;
    }

    //SET lang
    QString lang;
    if (obj.contains("lang")){
        lang = obj.value("lang").toString().trimmed();
        QList<QString> allow_lang = {"en", "ru","de"};
        if(!allow_lang.contains(lang)){
            pWebSocketServer->sendMessageError(pClient, cmd(), m, Error(404, "Language is not support"));
            return;
        }
    } else {
        lang = "en";
    }

    //GET localization
    if(lang != "en"){
        //GET localization for the article with a given lang
        lang = obj.value("lang").toString().trimmed();
        query.prepare("SELECT name, content FROM classbook_localization WHERE classbookid=:classbookid AND lang=:lang");
        query.bindValue(":classbookid", classbookid);
        query.bindValue(":lang", lang);
        query.exec();
        if (query.next()) {
            QSqlRecord record = query.record();
            info["lang"] = QJsonValue(lang);
            info["name"] = record.value("name").toString();
            info["content"] = record.value("content").toString();
        } else {
            //GET default localization for the article
            query.prepare("SELECT name, content FROM classbook WHERE id=:classbookid");
            query.bindValue(":classbookid", classbookid);
            query.exec();
            if (query.next()) {
                QSqlRecord record = query.record();
                info["lang"] = QJsonValue("en");
                info["name"] = record.value("name").toString();
                info["content"] = record.value("content").toString();
            } else {
                pWebSocketServer->sendMessageError(pClient, cmd(), m, Errors::NotFound("the article"));
                return;
            }
        }
    } else {
        //GET default localization for the article
        query.prepare("SELECT name, content FROM classbook WHERE id=:classbookid");
        query.bindValue(":classbookid", classbookid);
        query.exec();
        if (query.next()) {
            QSqlRecord record = query.record();
            info["lang"] = QJsonValue(lang);
            info["name"] = record.value("name").toString();
            info["content"] = record.value("content").toString();
        } else {
            pWebSocketServer->sendMessageError(pClient, cmd(), m, Errors::NotFound("the article"));
            return;
        }
    }

    //FIND langs for the article
    QJsonObject langs;
    query.prepare("SELECT id, lang FROM classbook_localization WHERE classbookid=:classbookid");
    query.bindValue(":classbookid", classbookid);
    query.exec();
    if (query.next()) {
        QSqlRecord record = query.record();
        QString local_lang;
        local_lang = record.value("lang").toString();
        langs[local_lang] = record.value("id").toInt();
    }
    info["langs"] = langs;

    //FIND parents for the article
    QJsonArray parents;
    QSet<int> set_of_parent;
    int parentid = info.value("parentid").toInt();
    for (int i=0; i < 5; ++i){
        //END IT root article
        if (parentid==0){
            QJsonObject parent;
            parent["classbookid"] = 0;
            parent["parentid"] = 0;
            parent["name"] = "Root";
            parents.push_back(parent);
            break;
        }
        //CONTINUE if already have a article in parents
        if (set_of_parent.contains(parentid)){
            continue;
        }
        query.prepare("SELECT id, name, parentid FROM classbook WHERE id=:parentid");
        query.bindValue(":parentid", parentid);
        query.exec();
        if (query.next()) {
            QSqlRecord record = query.record();
            QJsonObject parent;
            parent["classbookid"] = record.value("id").toInt();
            if (classbookid == parent["classbookid"].toInt()){
                continue;
            }
            parentid = record.value("parentid").toInt();
            parent["parentid"] = parentid;
            parent["name"] = record.value("name").toString();
            parents.push_back(parent);
            set_of_parent.insert(parent["classbookid"].toInt());
        } else {
            pWebSocketServer->sendMessageError(pClient, cmd(), m, Error(404, "Error in PATHFINDER. Not found the article with a given classbookid"));
            return;
        }
    }
    //ADD parents to response
    info["parents"] = parents;

    QJsonObject jsonData;
    jsonData["cmd"] = QJsonValue(cmd());
    jsonData["m"] = QJsonValue(m);
    jsonData["result"] = QJsonValue("DONE");
    jsonData["data"] = info;
    pWebSocketServer->sendMessage(pClient, jsonData);
}

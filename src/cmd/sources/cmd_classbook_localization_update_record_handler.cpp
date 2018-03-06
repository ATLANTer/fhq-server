#include <cmd_classbook_localization_update_record_handler.h>
#include <QJsonArray>
#include <QSqlError>
#include <log.h>
#include <QCryptographicHash>

CmdClassbookLocalizationUpdateRecordHandler::CmdClassbookLocalizationUpdateRecordHandler(){
    m_vInputs.push_back(CmdInputDef("classbook_localizationid").required().integer_().description("Localization id"));
    m_vInputs.push_back(CmdInputDef("name").required().string_().description("Article name"));
    m_vInputs.push_back(CmdInputDef("content").required().string_().description("The content of the article"));
}

std::string CmdClassbookLocalizationUpdateRecordHandler::cmd(){
    return "classbook_localization_update_record";
}

bool CmdClassbookLocalizationUpdateRecordHandler::accessUnauthorized(){
    return false;
}

bool CmdClassbookLocalizationUpdateRecordHandler::accessUser(){
    return false;
}

bool CmdClassbookLocalizationUpdateRecordHandler::accessTester(){
    return false;
}

bool CmdClassbookLocalizationUpdateRecordHandler::accessAdmin(){
    return true;
}

const std::vector<CmdInputDef> &CmdClassbookLocalizationUpdateRecordHandler::inputs(){
    return m_vInputs;
};

QString CmdClassbookLocalizationUpdateRecordHandler::description(){
    return "Update table with localization by classbookid";
}

QStringList CmdClassbookLocalizationUpdateRecordHandler::errors(){
    QStringList	list;
    return list;
}

void CmdClassbookLocalizationUpdateRecordHandler::handle(ModelRequest *pRequest){
    QJsonObject jsonRequest = pRequest->data();
    QJsonObject jsonResponse;

    QSqlDatabase db = *(pRequest->server()->database());

    QJsonObject data;

    QSqlQuery query(db);
    int classbook_localizationid = jsonRequest["classbook_localizationid"].toInt();
    query.prepare("SELECT id FROM classbook_localization WHERE id = :classbook_localizationid");
    query.bindValue(":classbook_localizationid", classbook_localizationid);
    if(!query.exec()){
        pRequest->sendMessageError(cmd(), Error(500, query.lastError().text()));
        return;
    }
    if(!query.next()){
        pRequest->sendMessageError(cmd(), Error(404, "This localization doesn't exist"));
        return;
    }

    QString name = jsonRequest["name"].toString().trimmed();
    QString content = jsonRequest["content"].toString().trimmed();

    //Set md5_content hash
    QString md5_content = QString(QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Md5).toHex());

    query.prepare("UPDATE classbook_localization SET name = :name, content = :content, md5_content = :md5_content, updated = NOW() "
                  "WHERE id = :classbook_localizationid");
    query.bindValue(":classbook_localizationid", classbook_localizationid);
    query.bindValue(":name", name);
    query.bindValue(":content", content);
    query.bindValue(":md5_content", md5_content);
    if(!query.exec()){
        pRequest->sendMessageError(cmd(), Error(500, query.lastError().text()));
        return;
    }
    query.prepare("SELECT classbookid, lang FROM classbook_localization WHERE id=:id");
    query.bindValue(":id", classbook_localizationid);
    query.exec();
    query.next();
    QSqlRecord record = query.record();
    data["classbookid"] = record.value("classbookid").toInt();
    data["classbook_localizationid"] = classbook_localizationid;
    data["lang"] = record.value("lang").toString();
    data["name"] = name;
    data["content"] = content;
    data["md5_content"] = md5_content;

    jsonResponse["data"] = data;
    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

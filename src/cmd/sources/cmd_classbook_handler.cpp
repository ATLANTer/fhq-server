#include <cmd_classbook_handler.h>
#include <QJsonArray>
#include <QSqlError>

CmdClassbookHandler::CmdClassbookHandler(){
}

std::string CmdClassbookHandler::cmd(){
	return "classbook";
}

bool CmdClassbookHandler::accessUnauthorized(){
	return true;
}

bool CmdClassbookHandler::accessUser(){
	return true;
}

bool CmdClassbookHandler::accessTester(){
	return true;
}

bool CmdClassbookHandler::accessAdmin(){
	return true;
}

const QVector<CmdInputDef> &CmdClassbookHandler::inputs(){
	return m_vInputs;
};

QString CmdClassbookHandler::description(){
	return "Return classbook contents";
}

QStringList CmdClassbookHandler::errors(){
	QStringList	list;
	return list;
}

void CmdClassbookHandler::handle(QWebSocket *pClient, IWebSocketServer *pWebSocketServer, QString m, QJsonObject /*obj*/){

	QSqlDatabase db = *(pWebSocketServer->database());

	QJsonArray contents;

	QSqlQuery query(db);
	query.prepare("SELECT * FROM classbook ORDER BY id ASC");
	query.exec();
	while (query.next()) {
		QSqlRecord record = query.record();
		QString uuid = record.value("uuid").toString();
		QJsonObject jsonItem;
		jsonItem["id"] = record.value("id").toInt();
		jsonItem["parentid"] = record.value("parentid").toInt();
		jsonItem["uuid"] = uuid;
		jsonItem["parentuuid"] = record.value("parentuuid").toString();
		jsonItem["name_ru"] = record.value("name_ru").toString();
		jsonItem["name_en"] = record.value("name_en").toString();
		jsonItem["name"] = record.value("name_en").toString();
		jsonItem["link"] = QJsonValue("files/classbook/" + uuid + "_en.md");
		contents.push_back(jsonItem);
	}

	QJsonObject jsonData;
    jsonData["cmd"] = QJsonValue(QString(cmd().c_str()));
	jsonData["result"] = QJsonValue("DONE");
    jsonData["m"] = QJsonValue(m);
	jsonData["items"] = contents;
	pWebSocketServer->sendMessage(pClient, jsonData);
}

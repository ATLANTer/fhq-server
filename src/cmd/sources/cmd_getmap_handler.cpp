#include <cmd_getmap_handler.h>
#include <QJsonArray>
#include <memory_cache_serversettings.h>

CmdGetMapHandler::CmdGetMapHandler(){
	
}

QString CmdGetMapHandler::cmd(){
	return "getmap";
}

bool CmdGetMapHandler::accessUnauthorized(){
	return true;
}

bool CmdGetMapHandler::accessUser(){
	return true;
}

bool CmdGetMapHandler::accessTester(){
	return true;
}

bool CmdGetMapHandler::accessAdmin(){
	return true;
}

const QVector<CmdInputDef> &CmdGetMapHandler::inputs(){
	return m_vInputs;
};

QString CmdGetMapHandler::description(){
	return "Returned coordinate list";
}

QStringList CmdGetMapHandler::errors(){
	QStringList	list;
	return list;
}

void CmdGetMapHandler::handle(QWebSocket *pClient, IWebSocketServer *pWebSocketServer, QString m, QJsonObject /*obj*/){

    IMemoryCache *pMemoryCache = pWebSocketServer->findMemoryCache("serversettings");
    if(pMemoryCache == NULL){
        pWebSocketServer->sendMessageError(pClient, cmd(), m, Errors::InternalServerError());
        return;
    }

    MemoryCacheServerSettings *pMemoryCacheServerSettings = dynamic_cast<MemoryCacheServerSettings*>(pMemoryCache);

	QJsonArray coords;
	QSqlDatabase db = *(pWebSocketServer->database());
	QSqlQuery query(db);
	query.prepare("SELECT COUNT(*) as cnt, latitude, longitude FROM `users` GROUP BY latitude, longitude");
	query.exec();
	while (query.next()) {
		QSqlRecord record = query.record();
		double lat = record.value("latitude").toDouble();
		double lon = record.value("longitude").toDouble();
		int count = record.value("cnt").toInt();
		if(lat == 0 && lon == 0){
			continue;
		}
		QJsonObject item;
		item["lat"] = lat;
		item["lng"] = lon;
		item["count"] = count;
		coords.push_back(item);
	}

	QJsonObject jsonData;
	jsonData["cmd"] = QJsonValue(cmd());
	jsonData["result"] = QJsonValue("DONE");
    jsonData["m"] = QJsonValue(m);
	jsonData["data"] = coords;
    jsonData["google_map_api_key"] = pMemoryCacheServerSettings->getSettString("google_map_api_key");
	pWebSocketServer->sendMessage(pClient, jsonData);
}

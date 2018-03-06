#include <cmd_getmap_handler.h>
#include <QJsonArray>
#include <employ_settings.h>

CmdGetMapHandler::CmdGetMapHandler(){
	
}

std::string CmdGetMapHandler::cmd(){
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

const std::vector<CmdInputDef> &CmdGetMapHandler::inputs(){
	return m_vInputs;
};

QString CmdGetMapHandler::description(){
	return "Returned coordinate list";
}

QStringList CmdGetMapHandler::errors(){
	QStringList	list;
	return list;
}

void CmdGetMapHandler::handle(ModelRequest *pRequest){
    QJsonObject jsonRequest = pRequest->data();
    QJsonObject jsonResponse;

    EmploySettings *pSettings = findEmploy<EmploySettings>();

	QJsonArray coords;
    QSqlDatabase db = *(pRequest->server()->database());
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

    jsonResponse["data"] = coords;
    jsonResponse["google_map_api_key"] = pSettings->getSettString("google_map_api_key");
    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

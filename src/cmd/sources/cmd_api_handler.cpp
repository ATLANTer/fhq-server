#include <cmd_api_handler.h>
#include <QtCore>

CmdApiHandler::CmdApiHandler(){
}

std::string CmdApiHandler::cmd(){
	return "api";
}

bool CmdApiHandler::accessUnauthorized(){
	return true;
}

bool CmdApiHandler::accessUser(){
	return true;
}

bool CmdApiHandler::accessTester(){
	return true;
}

bool CmdApiHandler::accessAdmin(){
	return true;
}

const std::vector<CmdInputDef> &CmdApiHandler::inputs(){
	return m_vInputs;
};

QString CmdApiHandler::description(){
	return "This method Will be return list of all handlers";
}

QStringList CmdApiHandler::errors(){
	QStringList	list;
	return list;
}

void CmdApiHandler::handle(QWebSocket *pClient, IWebSocketServer *pWebSocketServer, QString m, QJsonObject /*obj*/){
	QJsonObject jsonData;
    jsonData["cmd"] = QJsonValue(QString(cmd().c_str()));
	jsonData["m"] = QJsonValue(m);
	QJsonObject data;
	pWebSocketServer->exportApi(data);
	jsonData["data"] = data;
    jsonData["version"] = QCoreApplication::applicationVersion();
	pWebSocketServer->sendMessage(pClient, jsonData);
}

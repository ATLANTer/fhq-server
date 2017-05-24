#include "../headers/cmd_createpublicevent_handler.h"
#include <QJsonArray>

QString CmdCreatePublicEventHandler::cmd(){
	return "createpublicevent";
}

bool CmdCreatePublicEventHandler::accessUnauthorized(){
	return false;
}

bool CmdCreatePublicEventHandler::accessUser(){
	return false;
}

bool CmdCreatePublicEventHandler::accessTester(){
	return false;
}

bool CmdCreatePublicEventHandler::accessAdmin(){
	return true;
}

QString CmdCreatePublicEventHandler::short_description(){
	return "Create public event";
}

QString CmdCreatePublicEventHandler::description(){
	return
		"Create public event\n"
		" Input params: \n"
		"   * type \n"
		"   * message  \n";
}

QStringList CmdCreatePublicEventHandler::errors(){
	QStringList	list;
	return list;
}

void CmdCreatePublicEventHandler::handle(QWebSocket *pClient, IWebSocketServer *pWebSocketServer, QJsonObject obj){

	QJsonObject jsonData;
	jsonData["cmd"] = QJsonValue(cmd());

	if(!obj.contains("type")){
		pWebSocketServer->sendMessageError(pClient, cmd(), Errors::EventTypeExpected());
		return;
	}
	
	if(!obj.contains("message")){
		pWebSocketServer->sendMessageError(pClient, cmd(), Errors::EventMessageExpected());
		return;
	}

	QString type = obj["type"].toString().trimmed();
	QString message = obj["message"].toString().trimmed();
	
	if(type != "info" && type != "users" && type != "games" && type != "quests" && type != "warning"){
		pWebSocketServer->sendMessageError(pClient, cmd(), Errors::EventTypeValueExpected());
		return;
	}
	

	QSqlDatabase db = *(pWebSocketServer->database());
	QSqlQuery query(db);
	query.prepare("INSERT INTO public_events(type,message,dt) VALUES(:type,:message,NOW())");
	query.bindValue(":type", type);
	query.bindValue(":message", message);
	query.exec();

	jsonData["result"] = QJsonValue("DONE");
	pWebSocketServer->sendMessage(pClient, jsonData);
}

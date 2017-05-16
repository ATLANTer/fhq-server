#include "../headers/sendchatmessage.h"

QString CmdSendChatMessageHandler::cmd(){
	return "sendchatmessage";
}

bool CmdSendChatMessageHandler::accessUnauthorized(){
	return true;
}

bool CmdSendChatMessageHandler::accessUser(){
	return true;
}

bool CmdSendChatMessageHandler::accessTester(){
	return true;
}

bool CmdSendChatMessageHandler::accessAdmin(){
	return true;
}

QString CmdSendChatMessageHandler::short_description(){
	return "some short description";
}

QString CmdSendChatMessageHandler::description(){
	return "some description";
}

QStringList CmdSendChatMessageHandler::errors(){
	QStringList	list;
	return list;
}

void CmdSendChatMessageHandler::handle(QWebSocket *pClient, IWebSocketServer *pWebSocketServer, QJsonObject obj){
	IUserToken *pUserToken = pWebSocketServer->getUserToken(pClient);
	QString username = "";
	if(pUserToken != NULL){
		username = pUserToken->nick();
	}else{
		username = "Guest";
	}

	QJsonObject jsonData;
	jsonData["cmd"] = QJsonValue(cmd());
	jsonData["result"] = QJsonValue("DONE");
	pWebSocketServer->sendMessage(pClient, jsonData);
	
	QJsonObject jsonData2;
	jsonData2["cmd"] = QJsonValue("chat");
	jsonData2["type"] = obj["type"];
	jsonData2["user"] = username;
	jsonData2["message"] = obj["message"];

	pWebSocketServer->sendToAll(jsonData2);
	
	QSqlDatabase db = *(pWebSocketServer->database());
	QSqlQuery query(db);
	query.prepare("INSERT INTO chatmessages(user, message, dt) VALUES(:user,:message, NOW())");
	query.bindValue(":user", username);
	query.bindValue(":message", obj["message"].toString());
	query.exec();
}

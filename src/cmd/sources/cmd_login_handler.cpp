#include <cmd_login_handler.h>
#include <runtasks.h>
#include <log.h>
#include <usertoken.h>
#include <QCryptographicHash>
#include <QUuid>

CmdLoginHandler::CmdLoginHandler(){
	TAG = "CmdLoginHandler";
    m_vInputs.push_back(CmdInputDef("email").required().string_().description("E-mail"));
    m_vInputs.push_back(CmdInputDef("password").required().string_().description("Password"));
}

QString CmdLoginHandler::cmd(){
	return "login";
}

bool CmdLoginHandler::accessUnauthorized(){
	return true;
}

bool CmdLoginHandler::accessUser(){
	return true;
}

bool CmdLoginHandler::accessTester(){
	return true;
}

bool CmdLoginHandler::accessAdmin(){
	return true;
}

const QVector<CmdInputDef> &CmdLoginHandler::inputs(){
	return m_vInputs;
};

QString CmdLoginHandler::description(){
	return "Method for login";
}

QStringList CmdLoginHandler::errors(){
	QStringList	list;
	return list;
}

void CmdLoginHandler::handle(QWebSocket *pClient, IWebSocketServer *pWebSocketServer, QString m, QJsonObject obj){
	QJsonObject jsonData;
	jsonData["cmd"] = QJsonValue(cmd());
	
    QString email = obj["email"].toString();
    QString password = obj["password"].toString();

    QString password_sha1 = email.toUpper() + password;

    password_sha1 = QString("%1").arg(QString(QCryptographicHash::hash(password_sha1.toUtf8(),QCryptographicHash::Sha1).toHex()));

    QSqlDatabase db = *(pWebSocketServer->database());
    QSqlQuery query(db);
    query.prepare("SELECT * FROM users WHERE email = :email AND pass = :pass");
    query.bindValue(":email", email);
    query.bindValue(":pass", password_sha1);

    if(!query.exec()){
        Log::err(TAG, query.lastError().text());
        pWebSocketServer->sendMessageError(pClient, cmd(), m, Error(500, query.lastError().text()));
        return;
    }
    if (query.next()) {
        QSqlRecord record = query.record();

        // {"user":{"id":"47","email":"freehackquest@gmail.com","nick":"admin1","role":"admin"}}

        Log::info(TAG, "Found");

        int userid = record.value("id").toInt();
        QString email = record.value("email").toString();
        QString nick = record.value("nick").toString();
        QString role = record.value("role").toString();

        Log::info(TAG, "UserID: " + QString::number(userid));
        Log::info(TAG, "Nick: " + nick);
        Log::info(TAG, "Role: " + role);

        QJsonObject user;
        user["id"] = userid;
        user["email"] = email;
        user["nick"] = nick;
        user["role"] = role;

        QJsonObject user_token;
        user_token["user"] = user;

        QJsonDocument doc(user_token);
        QString data = doc.toJson(QJsonDocument::Compact);

        QString token = QUuid::createUuid().toString();
        token = token.mid(1,token.length()-2);
        token = token.toUpper();

        Log::info(TAG, "token: " + token);

        QSqlQuery query_token(db);
        query_token.prepare("INSERT INTO users_tokens (userid, token, status, data, start_date, end_date) VALUES(:userid, :token, :status, :data, NOW(), NOW() + INTERVAL 1 DAY)");
        query_token.bindValue(":userid", userid);
        query_token.bindValue(":token", token);
        query_token.bindValue(":status", "active");
        query_token.bindValue(":data", data);

        if(!query_token.exec()){
            Log::err(TAG, query_token.lastError().text());
            pWebSocketServer->sendMessageError(pClient, cmd(), m, Error(500, query_token.lastError().text()));
            return;
        }

        jsonData["token"] = token;
        jsonData["user"] = user;

        pWebSocketServer->setUserToken(pClient, new UserToken(user_token));

        // update user location
        QString lastip = pClient->peerAddress().toString();
        RunTasks::UpdateUserLocation(pWebSocketServer, userid, lastip);

    }else{
        Log::err(TAG, "Invalid login or password");
        // ["error"]
        pWebSocketServer->sendMessageError(pClient, cmd(), m, Error(401, "Invalid login or password"));
        return;
    }

	jsonData["result"] = QJsonValue("DONE");
	jsonData["m"] = QJsonValue(m);
	pWebSocketServer->sendMessage(pClient, jsonData);
}

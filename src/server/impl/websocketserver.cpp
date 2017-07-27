#include <websocketserver.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QRegExp>

#include <QHostAddress>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>

#include <create_cmd_handlers.h>
#include "../smtp/smtp.h"
#include <create_list_updates.h>
#include <create_memory_cache.h>

// QT_USE_NAMESPACE

// ---------------------------------------------------------------------

WebSocketServer::WebSocketServer(QObject *parent) : QObject(parent) {

	m_pServerConfig = new ServerConfig();
	
	if(!m_pServerConfig->load()){
		m_bFailed = true;
		return;
	}

	m_pDBConnection = new DatabaseConnection("qt_sql_default_connection_1");
	m_pDBConnection_older = new DatabaseConnection("qt_sql_default_connection_2");
	
	if(!m_pDBConnection->connect(m_pServerConfig)){
		m_bFailed = true;
		return;
	}

	tryUpdateDatabase(m_pDBConnection->db());
	
	create_cmd_handlers(m_mapCmdHandlers);
	create_memory_cache(m_mapMemoryCache, this);

	{
		// init memory cache server info
		m_pMemoryCacheServerInfo = NULL;
		IMemoryCache *pMemoryCache = findMemoryCache("serverinfo");
		if(pMemoryCache != NULL){
			m_pMemoryCacheServerInfo = dynamic_cast<MemoryCacheServerInfo*>(pMemoryCache);
		}
	}
	
	m_pWebSocketServer = new QWebSocketServer(QStringLiteral("freehackquest-backend"), QWebSocketServer::NonSecureMode, this);
	m_pWebSocketServerSSL = new QWebSocketServer(QStringLiteral("freehackquest-backend"), QWebSocketServer::SecureMode, this);
	
    if (m_pWebSocketServer->listen(QHostAddress::Any, m_pServerConfig->serverPort())) {
		qDebug() << "freehackquest-backend listening on port" << m_pServerConfig->serverPort();
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
        connect(m_pWebSocketServer, &QWebSocketServer::closed, this, &WebSocketServer::closed);
    }else{
		qDebug() << "ERROR: freehackquest-backend can not listening on port " << m_pServerConfig->serverPort();
		m_bFailed = true;
		return;
	}

	if(m_pServerConfig->serverSslOn()){
		QSslConfiguration sslConfiguration;
		QFile certFile(m_pServerConfig->serverSslCertFile());
		QFile keyFile(m_pServerConfig->serverSslKeyFile());
		certFile.open(QIODevice::ReadOnly);
		keyFile.open(QIODevice::ReadOnly);
		QSslCertificate certificate(&certFile, QSsl::Pem);
		QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem);
		certFile.close();
		keyFile.close();
		sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
		sslConfiguration.setLocalCertificate(certificate);
		sslConfiguration.setPrivateKey(sslKey);
		sslConfiguration.setProtocol(QSsl::TlsV1SslV3);
		m_pWebSocketServerSSL->setSslConfiguration(sslConfiguration);
		
		if (m_pWebSocketServerSSL->listen(QHostAddress::Any, m_pServerConfig->serverSslPort())) {
			qDebug() << "freehackquest-backend listening (via ssl) on port" << m_pServerConfig->serverSslPort();
			connect(m_pWebSocketServerSSL, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnectionSSL);
			connect(m_pWebSocketServerSSL, &QWebSocketServer::sslErrors, this, &WebSocketServer::onSslErrors);
		}else{
			qDebug() << "ERROR: freehackquest-backend can not listening (via ssl) on port" << m_pServerConfig->serverSslPort();
			m_bFailed = true;
			return;
		}
	}
}

// ---------------------------------------------------------------------

WebSocketServer::~WebSocketServer() {
    m_pWebSocketServer->close();
    m_pWebSocketServerSSL->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

// ---------------------------------------------------------------------

/*! 
 *  Handling new connection by ws://
 */
 
void WebSocketServer::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();
	qDebug() << "NewConnection " << pSocket->peerAddress().toString() << " " << pSocket->peerPort();
        
    connect(pSocket, &QWebSocket::textMessageReceived, this, &WebSocketServer::processTextMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &WebSocketServer::processBinaryMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &WebSocketServer::socketDisconnected);

    m_clients << pSocket;
}

// ---------------------------------------------------------------------

/*! 
 *  Handling new connection by wss://
 */

void WebSocketServer::onNewConnectionSSL(){
	QWebSocket *pSocket = m_pWebSocketServerSSL->nextPendingConnection();
	qDebug() << "NewConnectionSSL " << pSocket->peerAddress().toString() << " " << pSocket->peerPort();
        
    connect(pSocket, &QWebSocket::textMessageReceived, this, &WebSocketServer::processTextMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &WebSocketServer::processBinaryMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &WebSocketServer::socketDisconnected);

    m_clients << pSocket;
}

// ---------------------------------------------------------------------

void WebSocketServer::processTextMessage(QString message) {
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
	// qDebug() << QDateTime::currentDateTimeUtc().toString() << " [WS] <<< " << message;

	QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
	QJsonObject jsonData = doc.object();
	
	if(jsonData.contains("cmd")){
		QString cmd = jsonData["cmd"].toString();
		
		if(m_mapCmdHandlers.contains(cmd)){
			ICmdHandler *pCmdHandler = m_mapCmdHandlers[cmd];
			
			if(m_pMemoryCacheServerInfo != NULL){
				m_pMemoryCacheServerInfo->incrementRequests(cmd);
			}
			
			// check access
			if(!pCmdHandler->accessUnauthorized()){
				IUserToken *pUserToken = getUserToken(pClient);
				if(pUserToken == NULL){
					this->sendMessageError(pClient, pCmdHandler->cmd(), Errors::NotAuthorizedRequest());
					return;
				}
				
				// access user
				if(pUserToken->isUser() && !pCmdHandler->accessUser()){
					this->sendMessageError(pClient, pCmdHandler->cmd(), Errors::AccessDenyForUser());
					return;
				}
				
				// access tester
				if(pUserToken->isTester() && !pCmdHandler->accessTester()){
					this->sendMessageError(pClient, pCmdHandler->cmd(), Errors::AccessDenyForTester());
					return;
				}
				
				// access admin
				if(pUserToken->isAdmin() && !pCmdHandler->accessAdmin()){
					this->sendMessageError(pClient, pCmdHandler->cmd(), Errors::AccessDenyForAdmin());
					return;
				}

				// allow access
				Error error = Errors::NoneError();
				if(this->validateInputParameters(error, pCmdHandler, jsonData)){
					pCmdHandler->handle(pClient, this, jsonData);	
				}else{
					this->sendMessageError(pClient, pCmdHandler->cmd(), error);
					return;
				}
			}else{
				// allow unauthorized request
				Error error = Errors::NoneError();
				if(this->validateInputParameters(error, pCmdHandler, jsonData)){
					pCmdHandler->handle(pClient, this, jsonData);
				}else{
					this->sendMessageError(pClient, pCmdHandler->cmd(), error);
					return;
				}
			}
		}else{
			qDebug() << "Unknown command: " << cmd;
			QJsonObject jsonData;
			jsonData["cmd"] = QJsonValue(cmd);
			jsonData["error"] = QString("Unknown command");
			this->sendMessage(pClient, jsonData);
		}
	}else{
		QJsonObject jsonData;
		jsonData["error"] = QString("Invalid command format");
		this->sendMessage(pClient, jsonData);
	}
}

// ---------------------------------------------------------------------

void WebSocketServer::processBinaryMessage(QByteArray message) {
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    qDebug() << "Binary Message received:" << message;
    if (pClient) {
        pClient->sendBinaryMessage(message);
    }
}

// ---------------------------------------------------------------------

void WebSocketServer::socketDisconnected() {
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    
	qDebug() << "socketDisconnected:" << pClient;
    if (pClient) {
		m_tokens.remove(pClient);
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}

// ---------------------------------------------------------------------

void WebSocketServer::onSslErrors(const QList<QSslError> &){
    qDebug() << "Ssl errors occurred";
}

// ---------------------------------------------------------------------

int WebSocketServer::getConnectedUsers(){
	return m_clients.length();
}

// ---------------------------------------------------------------------

void WebSocketServer::sendMessage(QWebSocket *pClient, QJsonObject obj){
	 if (pClient) {
		QJsonDocument doc(obj);
		QString message = doc.toJson(QJsonDocument::Compact);
		// qDebug() << QDateTime::currentDateTimeUtc().toString() << " [WS] >>> " << message;
        pClient->sendTextMessage(message);
    }
}

// ---------------------------------------------------------------------

void WebSocketServer::sendMessageError(QWebSocket *pClient, QString cmd, Error error){
	QJsonObject jsonData;
	jsonData["cmd"] = QJsonValue(cmd);
	jsonData["result"] = QJsonValue("FAIL");
	jsonData["error"] = QJsonValue(error.message());
	jsonData["code"] = QJsonValue(error.codeError());
	this->sendMessage(pClient, jsonData);
	return;
}

// ---------------------------------------------------------------------

void WebSocketServer::sendToAll(QJsonObject obj){
	for(int i = 0; i < m_clients.size(); i++){
		this->sendMessage(m_clients.at(i), obj);
	}
}

// ---------------------------------------------------------------------

QSqlDatabase *WebSocketServer::database(){
	// swap connection
	QMutexLocker locker (&m_mtxSwapConenctions);
	if(m_pDBConnection->isOutdated()){
		m_pDBConnection_older->close();
		m_pDBConnection_older->swap(m_pDBConnection);
		m_pDBConnection->connect(m_pServerConfig);
	}
	return m_pDBConnection->db();
}

// ---------------------------------------------------------------------

void WebSocketServer::setUserToken(QWebSocket *pClient, IUserToken *pUserToken){
	m_tokens[pClient] = pUserToken;
}

// ---------------------------------------------------------------------

IUserToken * WebSocketServer::getUserToken(QWebSocket *pClient){
	if(m_tokens.contains(pClient)){
		return m_tokens[pClient];
	}
	return NULL;
}

// ---------------------------------------------------------------------

void WebSocketServer::sendLettersBcc(QStringList emails, QString subject, QString text){
	QString username = m_pServerConfig->emailUsername();
	QString password = m_pServerConfig->emailPassword();
	QString smtphost = m_pServerConfig->emailSmtpHost();
	int smtpport = m_pServerConfig->emailSmtpPort();
	
	Smtp* smtp = new Smtp(username, password, smtphost, smtpport);
	// smtp->disableDebugMode();
    //connect(smtp, SIGNAL(status(QString)), this, SLOT(mailSent(QString)));
	smtp->sendMailBcc("freehackquest@gmail.com", emails, subject, text);
}

// ---------------------------------------------------------------------

IMemoryCache *WebSocketServer::findMemoryCache(QString name){
	if(m_mapMemoryCache.contains(name)){
		return m_mapMemoryCache[name];
	}
	return NULL;
}

// ---------------------------------------------------------------------

void WebSocketServer::exportApi(QJsonObject &result){
	
	result["port"] = m_pServerConfig->serverPort();
	result["ssl_port"] = m_pServerConfig->serverPort();
	
	QJsonArray handlers;
	
	foreach( QString key, m_mapCmdHandlers.keys()){
		ICmdHandler *pHandler = m_mapCmdHandlers.value(key);
		QJsonObject handler;
		
		handler["cmd"] = pHandler->cmd();
		handler["description"] = pHandler->description();
		handler["access_unauthorized"] = pHandler->accessUnauthorized();
		handler["access_user"] = pHandler->accessUser();
		handler["access_tester"] = pHandler->accessTester();
		handler["access_admin"] = pHandler->accessAdmin();

		QJsonArray inputs;
		QVector<CmdInputDef> ins = pHandler->inputs();
		for(int i = 0; i < ins.size(); i++){
			inputs.append(ins[i].toJson());
		}
		handler["inputs"] = inputs;

		QJsonArray errors;
		QStringList errs = pHandler->errors();
		if(errs.size() > 0){
			for(int i = 0; i < errors.size(); i++){
				errors.append(errors.at(i));
			}
		}
		handler["errors"] = errors;
		handlers.append(handler);
	}
	result["handlers"] = handlers;
}

// ---------------------------------------------------------------------

bool WebSocketServer::validateInputParameters(Error &error, ICmdHandler *pCmdHandler, QJsonObject &jsonData){
	const QVector<CmdInputDef> vInputs = pCmdHandler->inputs();
	for(int i = 0; i < vInputs.size(); i++){
		CmdInputDef inDef = vInputs[i];
		QString sParamName = inDef.getName();
		if(inDef.isRequired() && !jsonData.contains(sParamName)){
			error = Errors::ParamExpected(sParamName);
			return false;
		}
		
		if(jsonData.contains(sParamName)){
			if(inDef.isInteger()){
				QJsonValueRef vParam = jsonData[sParamName];
				if(!vParam.isDouble()){
					error = Errors::ParamMustBeInteger(sParamName);
					return false;
				}
			}
			
			if(inDef.isEnum()){
				QString val = jsonData[sParamName].toString().trimmed();
				QStringList eList = inDef.getEnumList();
				if(!eList.contains(val)){
					error = Errors::ParamExpectedValueOneFrom(sParamName,eList);
					return false;
				}
			}
			
			if(inDef.isUUID()){
				QString val = jsonData[sParamName].toString();
				QRegExp rx("[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}");
				if(!rx.isValid()){
					qDebug() << rx.errorString();
				}
				if(!rx.exactMatch(val)){
					error = Errors::ParamExpectedUUID(sParamName);
					return false;
				}
			}
		}
	}
	return true;
}

// ---------------------------------------------------------------------
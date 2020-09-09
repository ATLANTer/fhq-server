#include <websocketserver.h>

#include <QtCore> // TODO remove
#include <QDateTime>
#include <QRegularExpression>

#include <QHostAddress>
#include <QThread>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>

#include <fallen.h>

#include <employees.h>
#include <employ_server_info.h>
#include <cmd_handlers.h>

// ---------------------------------------------------------------------
// WebSocketClient

WebSocketClient::WebSocketClient(QWebSocket *pClient) {
    TAG = "WebSocketClient";
    m_pClient = pClient;
}

// ---------------------------------------------------------------------

QWebSocket *WebSocketClient::getClient() {
    return m_pClient;
}

// ---------------------------------------------------------------------

std::string WebSocketClient::getIpAddress() {
    std::string sAddress = ((QWebSocket*)m_pClient)->peerAddress().toString().toStdString();
    #if QT_VERSION >= 0x050600
        QNetworkRequest r = ((QWebSocket*)m_pClient)->request();
        QByteArray qbaHeaderName = QString("x-forwarded-for").toLatin1();
        if (r.hasRawHeader(qbaHeaderName)) {
            sAddress = r.rawHeader(qbaHeaderName).toStdString();
        }
    #endif
    return sAddress;
}

// ---------------------------------------------------------------------
// WebSocketServer

WebSocketServer::WebSocketServer(QObject *parent) : QObject(parent) {
    TAG = "WebSocketServer";

    m_bFailed = false;
    if (!WsjcppEmployees::init({"start_ws_server"})) {
        m_bFailed = true;
        return;
    }
    
    EmployGlobalSettings *pGlobalSettings = findWsjcppEmploy<EmployGlobalSettings>();
    int nWsPort = pGlobalSettings->get("port").getNumberValue();
    int bSslEnabled = pGlobalSettings->get("ssl_on").getBooleanValue();
    int nWssPort = pGlobalSettings->get("ssl_port").getNumberValue();

    EmployServerInfo *pServerInfo = findWsjcppEmploy<EmployServerInfo>();
    EmployServer *pServer = findWsjcppEmploy<EmployServer>();

    m_pWebSocketServer = new QWebSocketServer(QStringLiteral("fhq-server"), QWebSocketServer::NonSecureMode, this);
    m_pWebSocketServerSSL = new QWebSocketServer(QStringLiteral("fhq-server"), QWebSocketServer::SecureMode, this);

    if (m_pWebSocketServer->listen(QHostAddress::Any, nWsPort)) {
        WsjcppLog::info(TAG, "fhq-server listening on port " + std::to_string(nWsPort));
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
        connect(m_pWebSocketServer, &QWebSocketServer::closed, this, &WebSocketServer::closed);
    } else {
        WsjcppLog::err(TAG, "fhq-server can not listening on port " + std::to_string(nWsPort));
        m_bFailed = true;
        return;
    }

    if (bSslEnabled) {
        QSslConfiguration sslConfiguration;
        std::string sSslKeyFilepath = pGlobalSettings->get("ssl_key_file").getFilePathValue();
        std::string sSslCertFilepath = pGlobalSettings->get("ssl_cert_file").getFilePathValue();

        QFile certFile(QString::fromStdString(sSslCertFilepath));
        QFile keyFile(QString::fromStdString(sSslKeyFilepath));
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

        if (m_pWebSocketServerSSL->listen(QHostAddress::Any, nWssPort)) {
            WsjcppLog::info(TAG, "fhq-server listening (via ssl) on port" + std::to_string(nWssPort));
            connect(m_pWebSocketServerSSL, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnectionSSL);
            connect(m_pWebSocketServerSSL, &QWebSocketServer::sslErrors, this, &WebSocketServer::onSslErrors);
        } else {
            WsjcppLog::err(TAG, "fhq-server can not listening (via ssl) on port " + std::to_string(nWssPort));
            m_bFailed = true;
            return;
        }
    }
    connect(this, &WebSocketServer::sig_sendToAll, this, &WebSocketServer::slot_sendToAll);
    connect(this, &WebSocketServer::signal_sendToOne, this, &WebSocketServer::slot_sendToOne);

    // connect(this, SIGNAL(sig_sendToAll(nlohmann::json)), this, SLOT(slot_sendToAll(nlohmann::json)));

    pServerInfo->serverStarted();
    pServer->setServer(this); // temporary

    // TODO save in database information about server started
}

// ---------------------------------------------------------------------

WebSocketServer::~WebSocketServer() {
    m_pWebSocketServer->close();
    m_pWebSocketServerSSL->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

// ---------------------------------------------------------------------

bool WebSocketServer::isFailed() {
    return m_bFailed;
}

// ---------------------------------------------------------------------

void WebSocketServer::sendServerMessage(QWebSocket *pSocket) {
    nlohmann::json jsonServer;
    jsonServer["cmd"] = "server";
    jsonServer["m"] = "s0";
    jsonServer["app"] = "fhq-server";
    jsonServer["version"] = WSJCPP_APP_VERSION;

    EmployServerInfo *pServerInfo = findWsjcppEmploy<EmployServerInfo>();
    jsonServer["developers"] = pServerInfo->developers();

    sendMessage(pSocket, jsonServer);
}

// ---------------------------------------------------------------------

void WebSocketServer::initNewConnection(const std::string &sPrefix, QWebSocket *pSocket) {
    std::string sAddress = pSocket->peerAddress().toString().toStdString();
    std::string sInitMessage = "NewConnection" + sPrefix + ", url=" + pSocket->requestUrl().toString().toStdString();
    
    sInitMessage += sAddress + " " + std::to_string(pSocket->peerPort());

    #if QT_VERSION >= 0x050600
        QNetworkRequest r = pSocket->request();
        QByteArray qbaHeaderName = QString("x-forwarded-for").toLatin1();
        if (r.hasRawHeader(qbaHeaderName)) {
            sAddress = r.rawHeader(qbaHeaderName).toStdString();
            sInitMessage += " Apache Proxy (" + qbaHeaderName.toStdString() + "="
            + sAddress + ")";
        }        
    #endif

    WsjcppLog::info(TAG, sInitMessage);

    connect(pSocket, &QWebSocket::textMessageReceived, this, &WebSocketServer::processTextMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &WebSocketServer::processBinaryMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &WebSocketServer::socketDisconnected);

    m_clients << pSocket;
    sendServerMessage(pSocket);
}

// ---------------------------------------------------------------------

/*!
 *  Handling new connection by ws://
 */

void WebSocketServer::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();
    initNewConnection("", pSocket);
}

// ---------------------------------------------------------------------

/*!
 *  Handling new connection by wss://
 */

void WebSocketServer::onNewConnectionSSL() {
    QWebSocket *pSocket = m_pWebSocketServerSSL->nextPendingConnection();
    initNewConnection("SSL", pSocket);
}

// ---------------------------------------------------------------------

void WebSocketServer::processTextMessage(const QString &message) {
    EmployServerInfo *pServerInfo = findWsjcppEmploy<EmployServerInfo>();
    EmployServer *pServer = findWsjcppEmploy<EmployServer>();

    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    WsjcppLog::warn(TAG, "QWebSocket *pClient = " + WsjcppCore::getPointerAsHex(pClient));
    WsjcppLog::warn(TAG, "pClient->localPort() = " + std::to_string(pClient->localPort()));

    // TODO find socket client in server with user session
    WebSocketClient *pWebSocketClient = new WebSocketClient(pClient);
    WsjcppJsonRpc20Request *pWsjcppJsonRpc20Request = new WsjcppJsonRpc20Request(pWebSocketClient, this);

    if (!pWsjcppJsonRpc20Request->parseIncomeData(message.toStdString())) {
        delete pWsjcppJsonRpc20Request;
        delete pWebSocketClient;
        return;
    }

    std::string sMethod = pWsjcppJsonRpc20Request->getMethod();
    std::string sId = pWsjcppJsonRpc20Request->getId();
    try {

        CmdHandlerBase *pCmdHandler = CmdHandlers::findCmdHandler(sMethod);
        if (pCmdHandler == NULL) {
            WsjcppLog::warn(TAG, "Unknown command: " + sMethod);
            pWsjcppJsonRpc20Request->sendMessageError(sMethod, WsjcppJsonRpc20Error(404, "UNKNOWN_COMMAND"));
            return;
        }

        pServerInfo->incrementRequests(sMethod);
        WsjcppJsonRpc20Error error(404, "NONE");

        // check access
        // TODO move check WsjcppJsonRpc20WebSocketServer
        if (!pCmdHandler->checkAccess(pWsjcppJsonRpc20Request, error)) {
            pWsjcppJsonRpc20Request->sendMessageError(pCmdHandler->cmd(), error);
            return;
        }

        // allow access
        // TODO move to WsjcppJsonRpc20WebSocketServer
        
        if (!pServer->validateInputParameters(error, pCmdHandler, pWsjcppJsonRpc20Request->jsonRequest())) {
            pWsjcppJsonRpc20Request->sendMessageError(pCmdHandler->cmd(), error);
            return;
        }
        pCmdHandler->handle(pWsjcppJsonRpc20Request);
    } catch (const std::exception &e) {
        this->sendMessageError(pWebSocketClient, sMethod, sId, WsjcppJsonRpc20Error(500, "InternalServerError"));
        WsjcppLog::err(TAG, e.what());
    }
}

// ---------------------------------------------------------------------

void WebSocketServer::processBinaryMessage(QByteArray /*message*/) {
    // QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    // NOT SUPPORTED
    // WsjcppLog::info(TAG, "Binary Message received: " + message.toHex());
    /*if (pClient) {
        pClient->sendBinaryMessage(message);
    }*/
}

// ---------------------------------------------------------------------

void WebSocketServer::socketDisconnected() {
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    long long nClient = (long long) pClient;
    // TODO hex print
    WsjcppLog::info(TAG, "socketDisconnected:" + std::to_string(nClient));
    if (pClient) {
        this->unsetUserSession(pClient);
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}

// ---------------------------------------------------------------------

void WebSocketServer::onSslErrors(const QList<QSslError> &) {
    WsjcppLog::err(TAG, "Ssl errors occurred");
}

// ---------------------------------------------------------------------

int WebSocketServer::getConnectedUsers() {
    return m_clients.length();
}

// ---------------------------------------------------------------------

void WebSocketServer::sendMessage(QWebSocket *pClient, const nlohmann::json& jsonResponse) {
    if (pClient) {
        std::string sCmd = "";
        if (jsonResponse.find("cmd") != jsonResponse.end() && jsonResponse["cmd"].is_string()) {
            sCmd = jsonResponse["cmd"];
        }
        std::string sM = "";
        
        if (jsonResponse.find("m") != jsonResponse.end() && jsonResponse["m"].is_string()) {
            sM = jsonResponse["m"];
        }
        std::string sMessage = jsonResponse.dump();

        WsjcppLog::info(TAG, "[WS] <<< " + sCmd + ":" + sM);
        if (m_clients.contains(pClient)) {
            try {
                pClient->sendTextMessage(QString::fromStdString(sMessage));
            } catch(...) {
                WsjcppLog::err(TAG, "Could not send message <<< " + sMessage);
            }
        } else {
            WsjcppLog::warn(TAG, "Could not send message, client disappeared");
        }
   } else {
        WsjcppLog::warn(TAG, "Client is wrong");
   }
}

// ---------------------------------------------------------------------

void WebSocketServer::sendMessageError(WsjcppJsonRpc20WebSocketClient *pWebSocketClient, const std::string &sCmd, const std::string &sId, WsjcppJsonRpc20Error error) {
    nlohmann::json jsonResponse;
    jsonResponse["method"] = sCmd;
    jsonResponse["jsonrpc"] = "2.0";
    jsonResponse["id"] = sId;
    jsonResponse["cmd"] = sCmd; // deprecated
    jsonResponse["m"] = sId; // deprecated
    jsonResponse["result"] = "FAIL"; // deprecated
    jsonResponse["error"] = error.getErrorMessage(); // deprecated
    jsonResponse["code"] = error.getErrorCode(); // deprecated
    QWebSocket *pClient = ((WebSocketClient *)pWebSocketClient)->getClient();

    WsjcppLog::err(TAG, "WS-ERROR >>> " + sCmd + ":" + sId + ", messsage: " + error.getErrorMessage());
    this->sendMessage(pClient, jsonResponse);
    // TODO jsonrpc20
    // {"jsonrpc": "2.0", "error": {"code": -32601, "message": "Method not found."}, "id": "1"}
    // I think that will be better add yet method name like '"method": sCmd,'
    return;
}

// ---------------------------------------------------------------------

void WebSocketServer::sendToAll(const nlohmann::json& jsonMessage) {
    std::string message = jsonMessage.dump();
    emit sig_sendToAll(QString(message.c_str()));
}

// ---------------------------------------------------------------------

void WebSocketServer::sendToOne(WsjcppJsonRpc20WebSocketClient *pWebSocketClient, const nlohmann::json &jsonMessage) {
    std::string message = jsonMessage.dump();
    QWebSocket *pClient = ((WebSocketClient *)pWebSocketClient)->getClient();
   
    emit signal_sendToOne(pClient, QString(message.c_str()));
}

// ---------------------------------------------------------------------
// slot
void WebSocketServer::slot_sendToAll(QString message) {
    std::string sMsg = message.toStdString();
    if (!nlohmann::json::accept(sMsg)) {
        return;
    }
    nlohmann::json jsonMessage = nlohmann::json::parse(sMsg);
    for (int i = 0; i < m_clients.size(); i++) {
        this->sendMessage(m_clients.at(i), jsonMessage);
    }
}

// ---------------------------------------------------------------------
// slot
void WebSocketServer::slot_sendToOne(QWebSocket *pClient, QString message) {
    if (!nlohmann::json::accept(message.toStdString())) {
        return;
    }
    nlohmann::json jsonMessage = nlohmann::json::parse(message.toStdString());
    this->sendMessage(pClient, jsonMessage);
}

// ---------------------------------------------------------------------

// TODO move to EmployServer

void WebSocketServer::setUserSession(void *pClient, WsjcppJsonRpc20UserSession *pWsjcppJsonRpc20UserSession) {
    std::lock_guard<std::mutex> lock(m_mtxUserSession);
    if (m_mapUserSession.find(pClient) == m_mapUserSession.end()) {
        WsjcppLog::err(TAG, "pWsjcppJsonRpc20UserSession pointer: " + WsjcppCore::getPointerAsHex(pWsjcppJsonRpc20UserSession));
        m_mapUserSession.insert(std::pair<void *, WsjcppJsonRpc20UserSession *>(pClient, pWsjcppJsonRpc20UserSession));
    } else {
        WsjcppLog::err(TAG, "User Session already exists");
    }
}

// ---------------------------------------------------------------------

void WebSocketServer::unsetUserSession(void *pClient) {
    std::lock_guard<std::mutex> lock(m_mtxUserSession);
    std::map<void *, WsjcppJsonRpc20UserSession *>::iterator it = m_mapUserSession.find(pClient);
    if (it != m_mapUserSession.end()) {
        WsjcppJsonRpc20UserSession* pUserSession = it->second;
        m_mapUserSession.erase(it);
        delete pUserSession;
    }
}

// ---------------------------------------------------------------------
// TODO EmployServer

WsjcppJsonRpc20UserSession *WebSocketServer::findUserSession(void *pClient) {
    std::lock_guard<std::mutex> lock(m_mtxUserSession);
    if (m_mapUserSession.find(pClient) != m_mapUserSession.end()) {
        return m_mapUserSession[pClient];
    }
    return nullptr;
};

// ---------------------------------------------------------------------

void WebSocketServer::logSocketError(QAbstractSocket::SocketError socketError) {
    std::string msg = "Unknown error";
    if (socketError == QAbstractSocket::ConnectionRefusedError) {
        msg = "QAbstractSocket::ConnectionRefusedError, The connection was refused by the peer (or timed out).";
    } else if (socketError == QAbstractSocket::RemoteHostClosedError) {
        msg = "QAbstractSocket::RemoteHostClosedError, The remote host closed the connection. Note that the client socket (i.e., this socket) will be closed after the remote close notification has been sent.";
    } else if (socketError == QAbstractSocket::HostNotFoundError) {
        msg = "QAbstractSocket::HostNotFoundError, The host address was not found.";
    } else if (socketError == QAbstractSocket::SocketAccessError) {
        msg = "QAbstractSocket::SocketAccessError, The socket operation failed because the application lacked the required privileges.";
    } else if (socketError == QAbstractSocket::SocketResourceError) {
        msg = "QAbstractSocket::SocketResourceError, The local system ran out of resources (e.g., too many sockets).";
    } else if (socketError == QAbstractSocket::SocketTimeoutError) {
        msg = "QAbstractSocket::SocketTimeoutError, The socket operation timed out.";
    } else if (socketError == QAbstractSocket::DatagramTooLargeError) {
        msg = "QAbstractSocket::DatagramTooLargeError, The datagram was larger than the operating system's limit (which can be as low as 8192 bytes).";
    } else if (socketError == QAbstractSocket::NetworkError) {
        msg = "QAbstractSocket::NetworkError, An error occurred with the network (e.g., the network cable was accidentally plugged out).";
    } else if (socketError == QAbstractSocket::AddressInUseError) {
        msg = "QAbstractSocket::AddressInUseError, The address specified to QAbstractSocket::bind() is already in use and was set to be exclusive.";
    } else if (socketError == QAbstractSocket::SocketAddressNotAvailableError) {
        msg = "QAbstractSocket::SocketAddressNotAvailableError, The address specified to QAbstractSocket::bind() does not belong to the host.";
    } else if (socketError == QAbstractSocket::UnsupportedSocketOperationError) {
        msg = "QAbstractSocket::UnsupportedSocketOperationError, The requested socket operation is not supported by the local operating system (e.g., lack of IPv6 support).";
    } else if (socketError == QAbstractSocket::ProxyAuthenticationRequiredError) {
        msg = "QAbstractSocket::ProxyAuthenticationRequiredError, The socket is using a proxy, and the proxy requires authentication.";
    } else if (socketError == QAbstractSocket::SslHandshakeFailedError) {
        msg = "QAbstractSocket::SslHandshakeFailedError, The SSL/TLS handshake failed, so the connection was closed (only used in QSslSocket)";
    } else if (socketError == QAbstractSocket::UnfinishedSocketOperationError) {
        msg = "QAbstractSocket::UnfinishedSocketOperationError, Used by QAbstractSocketEngine only, The last operation attempted has not finished yet (still in progress in the background).";
    } else if (socketError == QAbstractSocket::ProxyConnectionRefusedError) {
        msg = "QAbstractSocket::ProxyConnectionRefusedError, Could not contact the proxy server because the connection to that server was denied";
    } else if (socketError == QAbstractSocket::ProxyConnectionClosedError) {
        msg = "QAbstractSocket::ProxyConnectionClosedError, The connection to the proxy server was closed unexpectedly (before the connection to the final peer was established)";
    } else if (socketError == QAbstractSocket::ProxyConnectionTimeoutError) {
        msg = "QAbstractSocket::ProxyConnectionTimeoutError, The connection to the proxy server timed out or the proxy server stopped responding in the authentication phase.";
    } else if (socketError == QAbstractSocket::ProxyNotFoundError) {
        msg = "QAbstractSocket::ProxyNotFoundError, The proxy address set with setProxy() (or the application proxy) was not found.";
    } else if (socketError == QAbstractSocket::ProxyProtocolError) {
        msg = "QAbstractSocket::ProxyProtocolError, The connection negotiation with the proxy server failed, because the response from the proxy server could not be understood.";
    } else if (socketError == QAbstractSocket::OperationError) {
        msg = "QAbstractSocket::OperationError, An operation was attempted while the socket was in a state that did not permit it.";
    } else if (socketError == QAbstractSocket::SslInternalError) {
        msg = "QAbstractSocket::SslInternalError, The SSL library being used reported an internal error. This is probably the result of a bad installation or misconfiguration of the library.";
    } else if (socketError == QAbstractSocket::SslInvalidUserDataError) {
        msg = "QAbstractSocket::SslInvalidUserDataError, Invalid data (certificate, key, cypher, etc.) was provided and its use resulted in an error in the SSL library.";
    } else if (socketError == QAbstractSocket::TemporaryError) {
        msg = "QAbstractSocket::TemporaryError, A temporary error occurred (e.g., operation would block and socket is non-blocking).";
    } else if (socketError == QAbstractSocket::UnknownSocketError) {
        msg = "QAbstractSocket::UnknownSocketError, An unidentified error occurred.";
    }
    WsjcppLog::err(TAG, msg);
}

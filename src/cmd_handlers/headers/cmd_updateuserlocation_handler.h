#ifndef CMD_UPDATEUSERLOCATION_HANDLER_H
#define CMD_UPDATEUSERLOCATION_HANDLER_H

#include "../../interfaces/icmdhandler.h"
#include "../../interfaces/iwebsocketserver.h"

#include <QString>
#include <QVariant>

class CmdUpdateUserLocationHandler : public ICmdHandler {
	
	public:
		virtual QString cmd();
		virtual bool accessUnauthorized();
		virtual bool accessUser();
		virtual bool accessTester();
		virtual bool accessAdmin();
		virtual QString short_description();
		virtual QString description();
		virtual QStringList errors();
		virtual void handle(QWebSocket *pClient, IWebSocketServer *pWebSocketServer, QJsonObject obj);
};

#endif // CMD_UPDATEUSERLOCATION_HANDLER_H

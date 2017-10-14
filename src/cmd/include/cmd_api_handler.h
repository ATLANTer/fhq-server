#ifndef CMD_API_HANDLER_H
#define CMD_API_HANDLER_H

#include <iserver.h>

/*! 
 * This handler will be return list of handlers - publish api interfaces
 * */
 
class CmdApiHandler : public ICmdHandler {
	
	public:
		CmdApiHandler();
		virtual QString cmd();
		virtual bool accessUnauthorized();
		virtual bool accessUser();
		virtual bool accessTester();
		virtual bool accessAdmin();
		virtual const QVector<CmdInputDef> &inputs();
		virtual QString description();
		virtual QStringList errors();
		virtual void handle(QWebSocket *pClient, IWebSocketServer *pWebSocketServer, QString m, QJsonObject obj);
	private:
        QString TAG;
		QVector<CmdInputDef> m_vInputs;
};

#endif // CMD_API_HANDLER_H

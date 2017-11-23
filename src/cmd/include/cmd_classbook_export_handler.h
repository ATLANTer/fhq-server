#ifndef CMD_CLASSBOOK_EXPORT
#define CMD_CLASSBOOK_EXPORT

#include <iserver.h>

class CmdClassbookExportHandler : public ICmdHandler {

    public:
        CmdClassbookExportHandler();
        virtual QString cmd();
        virtual bool accessUnauthorized();
        virtual bool accessUser();
        virtual bool accessTester();
        virtual bool accessAdmin();
        virtual const QVector<CmdInputDef> &inputs();
        virtual QString description();
        virtual QStringList errors();
        virtual void handle(QWebSocket *pClient, IWebSocketServer *pWebSocketServer, QString m, QJsonObject obj);
        virtual void createHtml(QTextStream out, QString lang);

    private:
        QString TAG;
        QVector<CmdInputDef> m_vInputs;
};

#endif // CMD_CLASSBOOK_EXPORT


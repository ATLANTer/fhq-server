//
// Created by sergo on 26.10.18.
//

#ifndef FHQ_SERVER_LXDASYNCOPERATIONTASK_H
#define FHQ_SERVER_LXDASYNCOPERATIONTASK_H

#include <QRunnable>
#include <model_request.h>

class LXDAsyncOperationTask : public QRunnable {

    void run() override;

public:
    LXDAsyncOperationTask(void (*func)(std::string, std::string &, int &),
                          std::string sName, std::string sCMD, ModelRequest *pRequest);

    ~LXDAsyncOperationTask() override;

private:
    void (*m_func)(std::string, std::string &, int &);

    std::string m_sCMD;
    std::string TAG;
    std::string m_sName;
    std::string m_sM;
    QWebSocket *m_pRequestClient;
};


#endif //FHQ_SERVER_LXDASYNCOPERATIONTASK_H

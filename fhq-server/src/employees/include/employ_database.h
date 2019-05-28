#ifndef EMPLOY_DATABASE_H
#define EMPLOY_DATABASE_H

#include <model_database_connection.h>
#include <wjscpp_employees.h>

#include <storages.h>
#include <mutex>
#include <QMap>

class EmployDatabase : public EmployBase {
    public:
       EmployDatabase();
       static std::string name() { return "EmployDatabase"; }
       virtual bool init();
       QSqlDatabase *database();
       bool manualCreateDatabase(const std::string& sRootPassword, std::string& sError);
       StorageConnection *getStorageConnection();

    private:
        std::string TAG;
        std::string m_sStorageType;
        Storage *m_pStorage;

        // new new
        std::map<std::string, StorageConnection*> m_mapStorageConnections;

        // db two connections
        std::mutex m_mtxSwapConenctions;

        // old
        QMap<long long, ModelDatabaseConnection *> m_mDatabaseConnections;
        QMap<long long, ModelDatabaseConnection *> m_mDatabaseConnections_older;
        ModelDatabaseConnection *m_pDBConnection;
        ModelDatabaseConnection *m_pDBConnection_older;


};

#endif // EMPLOY_DATABASE_H

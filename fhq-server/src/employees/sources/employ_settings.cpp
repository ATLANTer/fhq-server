#include <employ_settings.h>
#include <employ_database.h>
#include <log.h>
#include <QTextStream>
#include <QFile>
#include <QByteArray>
#include <QDateTime>
#include <QDir>

REGISTRY_EMPLOY(EmploySettings)

// ---------------------------------------------------------------------

EmploySettings::EmploySettings()
    : EmployBase(EmploySettings::name(), {EmployDatabase::name()}){
    TAG = EmploySettings::name();
}

// ---------------------------------------------------------------------

bool EmploySettings::init(){
	Log::info(TAG, "Start init settings");

    std::string sGroupProfile = "profile";
    addNewSetting(new ServerSettHelper(sGroupProfile, "profile_change_nick", true));

    std::string sGroupMail = "mail";
    addNewSetting(new ServerSettHelper(sGroupMail, "mail_from", QString("fhqdebug@gmail.com")));
    addNewSetting(new ServerSettHelper(sGroupMail, "mail_host", QString("smtp.gmail.com")));
    addNewSetting(new ServerSettHelper(sGroupMail, "mail_port", 465));
    addNewSetting(new ServerSettHelper(sGroupMail, "mail_username", QString("fhqdebug@gmail.com")));
    addNewSetting(new ServerSettHelper(sGroupMail, "mail_password", QString("Friday-73"), true));
    addNewSetting(new ServerSettHelper(sGroupMail, "mail_auth", true));
    addNewSetting(new ServerSettHelper(sGroupMail, "mail_allow", true));
    addNewSetting(new ServerSettHelper(sGroupMail, "mail_system_message_admin_email", QString("")));

    // Google Map API
    std::string sGroupGoogleMap = "google_map";
    addNewSetting(new ServerSettHelper(sGroupGoogleMap, "google_map_api_key", QString("some")));

    // server folders
    std::string sGroupServerFolders = "server_folders";
    addNewSetting(new ServerSettHelper(sGroupServerFolders, "server_folder_games", QString("/var/www/html/fhq/files/games/")));
    addNewSetting(new ServerSettHelper(sGroupServerFolders, "server_folder_games_url", QString("https://freehackquest.com/files/games/")));
    addNewSetting(new ServerSettHelper(sGroupServerFolders, "server_folder_quests", QString("/var/www/html/fhq/files/quests/")));
    addNewSetting(new ServerSettHelper(sGroupServerFolders, "server_folder_quests_url", QString("https://freehackquest.com/files/quests/")));
    addNewSetting(new ServerSettHelper(sGroupServerFolders, "server_folder_users", QString("/var/www/html/fhq/files/quests/")));
    addNewSetting(new ServerSettHelper(sGroupServerFolders, "server_folder_users_url", QString("https://freehackquest.com/files/quests/")));

    std::vector<std::string> vFoundInDatabase;

	EmployDatabase *pDatabase = findEmploy<EmployDatabase>();
    QSqlDatabase db = *(pDatabase->database());

    // load from database
    {
        QSqlQuery query(db);
        query.prepare("SELECT * FROM settings");
        query.exec();
        while (query.next()) {
            QSqlRecord record = query.record();
            std::string sName = record.value("name").toString().toStdString();
            QString sValue = record.value("value").toString();
            std::string sType = record.value("type").toString().toStdString();
            std::string sGroup = record.value("group").toString().toStdString();

            vFoundInDatabase.push_back(sName);

            if(m_mapSettings.count(sName)){
                ServerSettHelper *pServerSettHelper = m_mapSettings[sName];
                if(sType != pServerSettHelper->type()){
                    Log::err(TAG, "Wrong type for setting '" + sName + "' (expected '" + pServerSettHelper->type() + "', but got: '" + sType + "'");
                    // TODO change type of setting or remove
                }else{
                    if(pServerSettHelper->isString()){
                        pServerSettHelper->setValue(sValue);
                    }else if(pServerSettHelper->isBoolean()){
                        pServerSettHelper->setValue(sValue == "yes");
                    }else if(pServerSettHelper->isInteger()){
                        // TODO check convertation string to int
                        pServerSettHelper->setValue(sValue.toInt());
                    }else if(pServerSettHelper->isPassword()){
                        pServerSettHelper->setValue(sValue);
                    }else{
                        Log::err(TAG, "No handle type for setting '" + sName + "'");
                    }
                }
            }else{
                Log::warn(TAG, "Undefined settings name in database: " + sName);
            }
        }
    }

    // check string settings in database
    std::map<std::string, ServerSettHelper*>::iterator it = m_mapSettings.begin();
    for (; it!=m_mapSettings.end(); ++it){
        std::string sName = it->first;
        // ServerSettHelper *pServerSettHelper = it->second;
        if(!m_mapSettings.count(sName)){
            ServerSettHelper *pServerSettHelper = m_mapSettings.at(sName);
            initSettingDatabase(pServerSettHelper);
        }
    }
    return true;
}

// ---------------------------------------------------------------------

void EmploySettings::addNewSetting(ServerSettHelper* pServerSettHelper){
    std::string sName = pServerSettHelper->name();
    if(!m_mapSettings.count(sName)){
        m_mapSettings.insert(std::pair<std::string, ServerSettHelper*>(sName,pServerSettHelper));
    }else{
        Log::warn(TAG, "Duplicate setting '" + sName + "'. Skip");
    }
}


// ---------------------------------------------------------------------

QString EmploySettings::getSettString(const std::string &sName){
    QMutexLocker locker (&m_mtxServerSettings);
    QString sResult = "";
    if(m_mapSettings.count(sName)){
        ServerSettHelper* pServerSettHelper = m_mapSettings.at(sName);
        if(!pServerSettHelper->isString()){
            Log::err(TAG, "Wrong type setting string (get): " + sName);
        }else{
            sResult = pServerSettHelper->valueAsString();
        }
    }else{
        Log::err(TAG, "Not found server setting string (get): " + sName);
    }
    return sResult;
}

// ---------------------------------------------------------------------

void EmploySettings::setSettString(const std::string &sName, QString sValue){
    QMutexLocker locker (&m_mtxServerSettings);
    if(m_mapSettings.count(sName)){
        ServerSettHelper* pServerSettHelper = m_mapSettings.at(sName);
        if(!pServerSettHelper->isString()){
            Log::err(TAG, "Wrong type setting string (set): " + sName);
        }else{
            pServerSettHelper->setValue(sValue);
            updateSettingDatabase(pServerSettHelper);
        }
    }else{
        Log::err(TAG, "Not found server setting string (set): " + sName);
    }
}

// ---------------------------------------------------------------------

QString EmploySettings::getSettPassword(const std::string &sName){
    QMutexLocker locker (&m_mtxServerSettings);
    QString sResult = "";
    if(m_mapSettings.count(sName)){
        ServerSettHelper* pServerSettHelper = m_mapSettings.at(sName);
        if(!pServerSettHelper->isPassword()){
            Log::err(TAG, "Wrong type setting password (get): " + sName);
        }else{
            sResult = pServerSettHelper->valueAsString();
        }
    }else{
        Log::err(TAG, "Not found server setting password (get): " + sName);
    }
    return sResult;
}

// ---------------------------------------------------------------------

void EmploySettings::setSettPassword(const std::string &sName, QString sValue){
    QMutexLocker locker (&m_mtxServerSettings);
    if(m_mapSettings.count(sName)){
        ServerSettHelper* pServerSettHelper = m_mapSettings.at(sName);
        if(!pServerSettHelper->isPassword()){
            Log::err(TAG, "Wrong type setting string (set): " + sName);
        }else{
            pServerSettHelper->setValue(sValue);
            updateSettingDatabase(pServerSettHelper);
        }
    }else{
        Log::err(TAG, "Not found server setting string (set): " + sName);
    }
}

// ---------------------------------------------------------------------

int EmploySettings::getSettInteger(const std::string &sName){
    QMutexLocker locker (&m_mtxServerSettings);
    int nResult = 0;
    if(m_mapSettings.count(sName)){
        ServerSettHelper* pServerSettHelper = m_mapSettings.at(sName);
        if(!pServerSettHelper->isInteger()){
            Log::err(TAG, "Wrong type setting integer (get): " + sName);
        }else{
            nResult = pServerSettHelper->valueAsInteger();
        }
    }else{
        Log::err(TAG, "Not found server setting integer (get): " + sName);
    }
    return nResult;
}

// ---------------------------------------------------------------------

void EmploySettings::setSettInteger(const std::string &sName, int nValue){
    QMutexLocker locker (&m_mtxServerSettings);
    if(m_mapSettings.count(sName)){
        ServerSettHelper* pServerSettHelper = m_mapSettings.at(sName);
        if(!pServerSettHelper->isInteger()){
            Log::err(TAG, "Wrong type setting integer (set): " + sName);
        }else{
            pServerSettHelper->setValue(nValue);
            updateSettingDatabase(pServerSettHelper);
        }
    }else{
        Log::err(TAG, "Not found server setting integer (set): " + sName);
    }
}

// ---------------------------------------------------------------------

bool EmploySettings::getSettBoolean(const std::string &sName){
    QMutexLocker locker (&m_mtxServerSettings);
    bool bResult = false;
    if(m_mapSettings.count(sName)){
        ServerSettHelper* pServerSettHelper = m_mapSettings.at(sName);
        if(!pServerSettHelper->isBoolean()){
            Log::err(TAG, "Wrong type setting boolean (get): " + sName);
        }else{
            bResult = pServerSettHelper->valueAsBoolean();
        }
    }else{
        Log::err(TAG, "Not found server setting boolean (get): " + sName);
    }
    return bResult;
}

// ---------------------------------------------------------------------

void EmploySettings::setSettBoolean(const std::string &sName, bool bValue){
    QMutexLocker locker (&m_mtxServerSettings);
    if(m_mapSettings.count(sName)){
        ServerSettHelper* pServerSettHelper = m_mapSettings.at(sName);
        if(!pServerSettHelper->isBoolean()){
            Log::err(TAG, "Wrong type setting boolean (set): " + sName);
        }else{
            pServerSettHelper->setValue(bValue);
            updateSettingDatabase(pServerSettHelper);
        }
    }else{
        Log::err(TAG, "Not found server setting integer (set): " + sName);
    }
}

// ---------------------------------------------------------------------

bool EmploySettings::hasSett(const std::string &sName){
    return m_mapSettings.count(sName) != 0;
}

// ---------------------------------------------------------------------

const std::string &EmploySettings::getSettType(const std::string &sName){
    if(m_mapSettings.count(sName)){
        return m_mapSettings.at(sName)->type();
    }
    return SETT_TYPE_UNKNOWN;
}

// ---------------------------------------------------------------------

nlohmann::json EmploySettings::toJson(){
    auto jsonSettings = nlohmann::json::array();

    std::map<std::string, ServerSettHelper*>::iterator it = m_mapSettings.begin();
    for (; it!=m_mapSettings.end(); ++it){
        std::string sName = it->first;
        ServerSettHelper *pServerSettHelper = it->second;

        nlohmann::json jsonSett;
        jsonSett["name"] = pServerSettHelper->name();
        if(pServerSettHelper->isBoolean()){
            jsonSett["value"] = pServerSettHelper->valueAsBoolean();
        }else if(pServerSettHelper->isString()){
            jsonSett["value"] = pServerSettHelper->valueAsString().toStdString();
        }else if(pServerSettHelper->isInteger()){
            jsonSett["value"] = pServerSettHelper->valueAsInteger();
        }else if(pServerSettHelper->isPassword()){
            jsonSett["value"] = "******";
        }else{
            jsonSett["value"] = pServerSettHelper->valueAsString().toStdString();
        }

        jsonSett["group"] = pServerSettHelper->group();
        jsonSett["type"] = pServerSettHelper->type();
        jsonSettings.push_back(jsonSett);
    }
    return jsonSettings;
}

// ---------------------------------------------------------------------

void EmploySettings::initSettingDatabase(ServerSettHelper *pServerSettHelper){
    Log::info(TAG, "Init settings to database: " + pServerSettHelper->name());
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();
    QSqlDatabase db = *(pDatabase->database());
    QSqlQuery query(db);
    query.prepare("INSERT INTO settings (`name`, `value`, `group`, `type`) VALUES (:name, :value, :group, :type)");
    query.bindValue(":name", QString(pServerSettHelper->name().c_str()));
    query.bindValue(":value", pServerSettHelper->valueAsString());
    query.bindValue(":group", QString(pServerSettHelper->group().c_str()));
    query.bindValue(":type", QString(pServerSettHelper->type().c_str()));
    if(!query.exec()){
        Log::err(TAG, query.lastError().text().toStdString());
    }
}

// ---------------------------------------------------------------------

void EmploySettings::updateSettingDatabase(ServerSettHelper *pServerSettHelper){
	EmployDatabase *pDatabase = findEmploy<EmployDatabase>();
    QSqlDatabase db = *(pDatabase->database());
    QSqlQuery query(db);
    query.prepare("UPDATE settings SET value = :value WHERE name = :name");
    query.bindValue(":value", pServerSettHelper->valueAsString());
    query.bindValue(":name", QString(pServerSettHelper->name().c_str()));
    if(!query.exec()){
        Log::err(TAG, query.lastError().text().toStdString());
    }
}

// ---------------------------------------------------------------------


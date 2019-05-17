#include <cmd_handlers_users.h>
#include <utils_logger.h>
#include <runtasks.h>
#include <iostream>
#include <employ_settings.h>
#include <employ_database.h>
#include <employ_server_info.h>
#include <employ_scoreboard.h>
#include <QtCore>
#include <sha1_wrapper.h>
#include <QUuid>

/*********************************************
 * This handler will be return scoreboard of user
**********************************************/

CmdHandlerUsersScoreboard::CmdHandlerUsersScoreboard()
    : CmdHandlerBase("scoreboard", "Method return scoreboard") {

    setAccessUnauthorized(true);
    setAccessUser(true);
    setAccessAdmin(true);

    requireIntegerParam("page", "Number of page");
    requireIntegerParam("onpage", "How much rows in one page");
}

// ---------------------------------------------------------------------

void CmdHandlerUsersScoreboard::handle(ModelRequest *pRequest) {
    const nlohmann::json &jsonRequest = pRequest->jsonRequest();
    nlohmann::json jsonResponse;

    int nPage = jsonRequest.at("page");
    jsonResponse["page"] = nPage;

    int nOnPage = jsonRequest.at("onpage");
    if (nOnPage > 50) {
        pRequest->sendMessageError(cmd(), Error(400, "Parameter 'onpage' could not be more then 50"));
    }
    jsonResponse["onpage"] = nOnPage;

    QStringList filters;
    QMap<QString,QString> filter_values;

    if (jsonRequest.find("user") != jsonRequest.end()) {
        QString user = QString::fromStdString(jsonRequest.at("user"));
        user = user.trimmed();
        filters << "(u.nick like :nick)";
        filter_values[":nick"] = "%" + user + "%";
    }

    filters << "(rating > 0)";

    QString where = filters.join(" AND ");
    if (where.length() > 0) {
        where = "WHERE " + where;
    }

    EmployScoreboard *pScoreboard = findEmploy<EmployScoreboard>();
    pScoreboard->loadSync();

    jsonResponse["count"] = pScoreboard->count();
    jsonResponse["data"] = pScoreboard->toJson();

    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

/*********************************************
 * This handler will be return json map users
**********************************************/

CmdHandlerGetMap::CmdHandlerGetMap()
    : CmdHandlerBase("getmap", "Returned coordinate list") {

    setAccessUnauthorized(true);
    setAccessUser(true);
    setAccessAdmin(true);
}

// ---------------------------------------------------------------------

void CmdHandlerGetMap::handle(ModelRequest *pRequest) {
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();

//    QJsonObject jsonRequest = pRequest->jsonRequest();
    nlohmann::json jsonResponse;

    EmploySettings *pSettings = findEmploy<EmploySettings>();

    nlohmann::json coords;
    QSqlDatabase db = *(pDatabase->database());
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) as cnt, latitude, longitude FROM `users` GROUP BY latitude, longitude");
    query.exec();
    while (query.next()) {
        QSqlRecord record = query.record();
        double lat = record.value("latitude").toDouble();
        double lon = record.value("longitude").toDouble();
        int count = record.value("cnt").toInt();
        if (lat == 0. && lon == 0.) {
            continue;
        }
        nlohmann::json item;
        item["lat"] = lat;
        item["lng"] = lon;
        item["count"] = count;
        coords.push_back(item);
    }

    jsonResponse["data"] = coords;
    jsonResponse["google_map_api_key"] = pSettings->getSettString("google_map_api_key").toStdString();
    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

/*********************************************
 * User login
**********************************************/

CmdHandlerLogin::CmdHandlerLogin()
    : CmdHandlerBase("login", "Method for login") {

    setAccessUnauthorized(true);
    setAccessUser(false);
    setAccessAdmin(false);

    requireStringParam("email", "E-mail"); // TODO validator no empty
    requireStringParam("password", "Password"); // TODO validator no empty
}

// ---------------------------------------------------------------------

void CmdHandlerLogin::handle(ModelRequest *pRequest) {
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();

    nlohmann::json const & jsonRequest = pRequest->jsonRequest();
    nlohmann::json jsonResponse;

    QString email = QString::fromStdString(jsonRequest.at("email"));
    QString password = QString::fromStdString(jsonRequest.at("password"));

    QString password_sha1 = email.toUpper() + password;
    std::string _password_sha1 = sha1::calc_string_to_hex(password_sha1.toStdString());
    password_sha1 = QString(_password_sha1.c_str());

    QSqlDatabase db = *(pDatabase->database());
    QSqlQuery query(db);
    query.prepare("SELECT * FROM users WHERE email = :email AND pass = :pass");
    query.bindValue(":email", email);
    query.bindValue(":pass", password_sha1);

    if (!query.exec()) {
        Log::err(TAG, query.lastError().text().toStdString());
        pRequest->sendMessageError(cmd(), Error(500, query.lastError().text().toStdString()));
        return;
    }
    if (query.next()) {
        QSqlRecord record = query.record();

        int nUserId = record.value("id").toInt();
        QString email = record.value("email").toString();
        QString nick = record.value("nick").toString();
        QString role = record.value("role").toString();

        nlohmann::json user;
        user["id"] = nUserId;
        user["email"] = email.toStdString();
        user["nick"] = nick.toStdString();
        user["role"] = role.toStdString();

        nlohmann::json user_token;
        user_token["user"] = user;

        // QJsonDocument doc(user_token);
        QString data = QString::fromStdString(user_token.dump());

        QString token = QUuid::createUuid().toString();
        token = token.mid(1,token.length()-2);
        token = token.toUpper();

        QSqlQuery query_token(db);
        query_token.prepare("INSERT INTO users_tokens (userid, token, status, data, start_date, end_date) VALUES(:userid, :token, :status, :data, NOW(), NOW() + INTERVAL 1 DAY)");
        query_token.bindValue(":userid", nUserId);
        query_token.bindValue(":token", token);
        query_token.bindValue(":status", "active");
        query_token.bindValue(":data", data);

        if (!query_token.exec()) {
            Log::err(TAG, query_token.lastError().text().toStdString());
            pRequest->sendMessageError(cmd(), Error(500, query_token.lastError().text().toStdString()));
            return;
        }

        jsonResponse["token"] = token.toStdString();
        jsonResponse["user"] = user;

        pRequest->server()->setWSJCppUserSession(pRequest->client(), new WSJCppUserSession(user_token));

        // update user location
        std::string sLastIP = pRequest->client()->peerAddress().toString().toStdString();
        RunTasks::UpdateUserLocation(nUserId, sLastIP);

    } else {
        Log::err(TAG, "Invalid login or password");
        pRequest->sendMessageError(cmd(), Error(401, "Invalid login or password"));
        return;
    }
    Log::info(TAG, jsonResponse.dump());
    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

/*********************************************
 * User registration
**********************************************/

CmdHandlerRegistration::CmdHandlerRegistration()
    : CmdHandlerBase("registration", "Method for registration") {

    setAccessUnauthorized(true);
    setAccessUser(false);
    setAccessAdmin(false);

    // validation and description input fields
    requireStringParam("email", "E-mail")
        .addValidator(new ValidatorEmail());
    requireStringParam("university", "University");
}

// ---------------------------------------------------------------------

void CmdHandlerRegistration::handle(ModelRequest *pRequest) {
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();

    const nlohmann::json &jsonRequest = pRequest->jsonRequest();
    nlohmann::json jsonResponse;

    //EmploySettings *pSettings = findEmploy<EmploySettings>();

    QString sEmail = QString::fromStdString(jsonRequest.at("email"));
    QString sUniversity = QString::fromStdString(jsonRequest.at("university"));

    QSqlDatabase db = *(pDatabase->database());
    QSqlQuery query(db);
    query.prepare("SELECT * FROM users WHERE email = :email");
    query.bindValue(":email", sEmail);
    if (!query.exec()) {
        Log::err(TAG, query.lastError().text().toStdString());
        pRequest->sendMessageError(cmd(), Error(500, query.lastError().text().toStdString()));
        return;
    }
    if (query.next()) {
        Log::err(TAG, "User already exists " + sEmail.toStdString());
        pRequest->sendMessageError(cmd(), Error(403, "This email already exists"));
        return;
    }

    // TODO: move to helpers
    // // generate random password
    const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    const int randomStringLength = 12; // assuming you want random strings of 12 characters
    QString sPassword;
    for (int i=0; i < randomStringLength; ++i) {
        int index = qrand() % possibleCharacters.length();
        QChar nextChar = possibleCharacters.at(index);
        sPassword.append(nextChar);
    }

    QString sPassword_sha1 = sEmail.toUpper() + sPassword;
    std::string _password_sha1 = sha1::calc_string_to_hex(sPassword_sha1.toStdString());
    sPassword_sha1 = QString(_password_sha1.c_str());

    // generate random nick
    const QString possibleCharacters2("ABCDEFH0123456789");
    const int randomStringLength2 = 8; // assuming you want random strings of 12 characters
    QString sNick;
    for (int i=0; i<randomStringLength2; ++i) {
        int index = qrand() % possibleCharacters2.length();
        QChar nextChar = possibleCharacters2.at(index);
        sNick.append(nextChar);
    }
    sNick = "hacker-" + sNick;

    QSqlQuery query_insert(db);
    query_insert.prepare(""
                         "INSERT INTO users ("
                         "   uuid, "
                         "   email, "
                         "   pass, "
                         "   role, "
                         "   nick,"
                         "   logo,"
                         "   dt_create,"
                         "   dt_last_login,"
                         "   last_ip,"
                         "   status,"
                         "   country,"
                         "   region,"
                         "   city,"
                         "   university,"
                         "   latitude,"
                         "   longitude,"
                         "   rating,"
                         "   about)"
                         "VALUES("
                         "   :uuid, "
                         "   :email, "
                         "   :pass, "
                         "   :role, "
                         "   :nick,"
                         "   :logo,"
                         "   NOW(),"
                         "   NOW(),"
                         "   :last_ip,"
                         "   :status,"
                         "   :country,"
                         "   :region,"
                         "   :city,"
                         "   :university,"
                         "   :latitude,"
                         "   :longitude,"
                         "   :rating,"
                         "   :about);"
    );

    std::string sLastIP = pRequest->client()->peerAddress().toString().toStdString();

    // TODO move to helpers
    QString sUuid = QUuid::createUuid().toString();
    sUuid = sUuid.mid(1,sUuid.length()-2);
    sUuid = sUuid.toUpper();

    query_insert.bindValue(":uuid", sUuid);
    query_insert.bindValue(":email", sEmail);
    query_insert.bindValue(":pass", sPassword_sha1);
    query_insert.bindValue(":role", "user");
    query_insert.bindValue(":nick", sNick);
    query_insert.bindValue(":logo", "files/users/0.png");
    query_insert.bindValue(":last_ip", "");
    query_insert.bindValue(":status", "activated");
    query_insert.bindValue(":country", "");
    query_insert.bindValue(":region", "");
    query_insert.bindValue(":city", "");
    query_insert.bindValue(":university", sUniversity);
    query_insert.bindValue(":latitude", 0);
    query_insert.bindValue(":longitude", 0);
    query_insert.bindValue(":rating", 0);
    query_insert.bindValue(":about", "");

    if (!query_insert.exec()) {
        pRequest->sendMessageError(cmd(), Error(500, query_insert.lastError().text().toStdString()));
        return;
    }

    int nUserID = query_insert.lastInsertId().toInt();

    RunTasks::AddPublicEvents("users", "New [user#" + std::to_string(nUserID) + "]  " + sNick.toStdString());

    std::string sSubject = "Registration on FreeHackQuest";
    std::string sContext = "Welcome to FreeHackQuest!\n"
                       "You login: " + sEmail.toStdString() + "\n"
                       "You password: " + sPassword.toStdString() + "\n";

    RunTasks::MailSend(sEmail.toStdString(), sSubject, sContext);

    pRequest->sendMessageSuccess(cmd(), jsonResponse);
    RunTasks::UpdateUserLocation(nUserID, sLastIP);
}

/*********************************************
 * User login by token
**********************************************/

CmdHandlerToken::CmdHandlerToken()
    : CmdHandlerBase("token", "Method for login by token") {

    setAccessUnauthorized(true);
    setAccessUser(false);
    setAccessAdmin(false);

    // validation and description input fields
    requireStringParam("token", "Authorization token"); // TODO validator
}

// ---------------------------------------------------------------------

void CmdHandlerToken::handle(ModelRequest *pRequest) {
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();

    const nlohmann::json & jsonRequest = pRequest->jsonRequest();
    nlohmann::json jsonResponse;

    nlohmann::json jsonData;
    jsonData["cmd"] = nlohmann::json(cmd());

    QString token = QString::fromStdString(jsonRequest.at("token"));
    QSqlDatabase db = *(pDatabase->database());

    QSqlQuery query(db);
    query.prepare("SELECT * FROM users_tokens WHERE token = :token");
    query.bindValue(":token", token);
    if (!query.exec()) {
        Log::err(TAG, query.lastError().text().toStdString());
        pRequest->sendMessageError(cmd(), Error(500, query.lastError().text().toStdString()));
        return;
    }
    if (query.next()) {
        QSqlRecord record = query.record();
        int userid = record.value("userid").toInt();
        QString status = record.value("status").toString();
        QString data = record.value("data").toString();
        QString start_date = record.value("start_date").toString();
        QString end_date = record.value("end_date").toString();
        std::string sLastIP = pRequest->client()->peerAddress().toString().toStdString();
        pRequest->server()->setWSJCppUserSession(pRequest->client(), new WSJCppUserSession(data));
        Log::info(TAG, "userid: " + QString::number(userid).toStdString());
        // TODO redesign this
        RunTasks::UpdateUserLocation(userid, sLastIP);
    } else {
        Log::err(TAG, "Invalid token " + token.toStdString());
        pRequest->sendMessageError(cmd(), Error(401, "Invalid token"));
        return;
    }

    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

/*********************************************
 * User location update
**********************************************/

CmdHandlerUpdateUserLocation::CmdHandlerUpdateUserLocation()
    : CmdHandlerBase("updateuserlocation", "This method will be try update user location by lastip") {

    setAccessUnauthorized(false);
    setAccessUser(false);
    setAccessAdmin(true);

    // validation and description input fields
    requireIntegerParam("userid", "User ID"); // TODO validator
}

// ---------------------------------------------------------------------

void CmdHandlerUpdateUserLocation::handle(ModelRequest *pRequest) {
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();

    const nlohmann::json & jsonRequest = pRequest->jsonRequest();
    nlohmann::json jsonResponse;

    // bool bConvert = false;

    int userid = jsonRequest.at("userid");

    // TODO redesign
    if (userid == 0) {
        pRequest->sendMessageError(cmd(), Error(400, "Parameter 'userid' must be not zero"));
        return;
    }

    QSqlDatabase db = *(pDatabase->database());
    std::string sLastIP = "";
    {
        QSqlQuery query(db);
        query.prepare("SELECT * FROM users WHERE id = :userid");
        query.bindValue(":userid", userid);
        query.exec();
        if (query.next()) {
            QSqlRecord record = query.record();
            sLastIP = record.value("last_ip").toString().toStdString();
        }
    }

    if (sLastIP == "") {
        QSqlQuery query(db);
        query.prepare("SELECT * FROM users_ips WHERE userid = :userid ORDER BY id DESC");
        query.bindValue(":userid", userid);
        query.exec();
        if (query.next()) {
            QSqlRecord record = query.record();
            sLastIP = record.value("ip").toString().toStdString();
        }
    }

    RunTasks::UpdateUserLocation(userid, sLastIP);

    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

/*********************************************
 * User change password
**********************************************/

CmdHandlerUserChangePassword::CmdHandlerUserChangePassword()
    : CmdHandlerBase("user_change_password", "This method for change user password") {

    setAccessUnauthorized(false);
    setAccessUser(true);
    setAccessAdmin(true);

    // validation and description input fields
    requireStringParam("password_old", "Old password");
    requireStringParam("password_new", "New password");
}

// ---------------------------------------------------------------------

void CmdHandlerUserChangePassword::handle(ModelRequest *pRequest) {
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();

    const nlohmann::json& jsonRequest = pRequest->jsonRequest();
    nlohmann::json jsonResponse;

    WSJCppUserSession *pUserSession = pRequest->userSession();
    int nUserID = pUserSession->userid();

    QSqlDatabase db = *(pDatabase->database());

    QSqlQuery query(db);
    query.prepare("SELECT * FROM users WHERE id = :userid");
    query.bindValue(":userid", nUserID);
    if (!query.exec()) {
        pRequest->sendMessageError(cmd(), Error(500, query.lastError().text().toStdString()));
        return;
    }

    QString sPass = "";
    QString sEmail = "";

    if (query.next()) {
        QSqlRecord record = query.record();
        sEmail = record.value("email").toString();
        sPass = record.value("pass").toString();
    } else {
        pRequest->sendMessageError(cmd(), Error(404, "Not found user"));
        return;
    }

    QString sOldPassword = QString::fromStdString(jsonRequest.at("password_old"));
    QString sNewPassword = QString::fromStdString(jsonRequest.at("password_new"));

    QString sOldPassword_sha1 = sEmail.toUpper() + sOldPassword;

    std::string _sOldPassword_sha1 = sha1::calc_string_to_hex(sOldPassword_sha1.toStdString());
    sOldPassword_sha1 = QString(_sOldPassword_sha1.c_str());

    if (sOldPassword_sha1 != sPass) {
        pRequest->sendMessageError(cmd(), Error(401, "Wrong password"));
        return;
    }

    QString sNewPassword_sha1 = sEmail.toUpper() + sNewPassword;

    std::string _sNewPassword_sha1 = sha1::calc_string_to_hex(sNewPassword_sha1.toStdString());
    sNewPassword_sha1 = QString(_sNewPassword_sha1.c_str());

    QSqlQuery query_update(db);
    query_update.prepare("UPDATE users SET pass = :pass WHERE id = :userid AND email = :email");
    query_update.bindValue(":pass", sNewPassword_sha1);
    query_update.bindValue(":userid", nUserID);
    query_update.bindValue(":email", sEmail);

    if (!query_update.exec()) {
        pRequest->sendMessageError(cmd(), Error(500, query_update.lastError().text().toStdString()));
        return;
    }

    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

/*********************************************
 * User create
**********************************************/

CmdHandlerUsersAdd::CmdHandlerUsersAdd()
    : CmdHandlerBase("users_add", "Method for add new user") {

    setAccessUnauthorized(false);
    setAccessUser(false);
    setAccessAdmin(true);
    setActivatedFromVersion("0.2.17");

    // validation and description input fields
    requireStringParam("uuid", "User's Global Unique Identifier")
        .addValidator(new ValidatorUUID());

    requireStringParam("email", "User's E-mail");
    requireStringParam("nick", "User's nick")
        .addValidator(new ValidatorStringLength(4, 127));
    requireStringParam("password", "Password")
        .addValidator(new ValidatorStringLength(4, 127));
    requireStringParam("role", "User's role"); // TODO role validator
    optionalStringParam("university", "University");
}

// ---------------------------------------------------------------------

void CmdHandlerUsersAdd::handle(ModelRequest *pRequest) {
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();

    const nlohmann::json& jsonRequest = pRequest->jsonRequest();

    QRegularExpression regexEmail("^[0-9a-zA-Z-._@]{3,128}$");
    QString sEmail = QString::fromStdString(jsonRequest.at("email"));

    if (!regexEmail.match(sEmail).hasMatch()) {
        Log::err(TAG, "Invalid email format " + sEmail.toStdString());
        pRequest->sendMessageError(cmd(), Error(400, "Expected email format"));
        return;
    }

    QSqlDatabase db = *(pDatabase->database());
    QSqlQuery query(db);
    query.prepare("SELECT * FROM users WHERE email = :email");
    query.bindValue(":email", sEmail);
    if (!query.exec()) {
        Log::err(TAG, query.lastError().text().toStdString());
        pRequest->sendMessageError(cmd(), Error(500, query.lastError().text().toStdString()));
        return;
    }
    if (query.next()) {
        Log::err(TAG, "User already exists " + sEmail.toStdString());
        pRequest->sendMessageError(cmd(), Error(403, "This email already exists"));
        return;
    }

    QString sNick = QString::fromStdString(jsonRequest.at("nick"));
    QString sPassword = QString::fromStdString(jsonRequest.at("password"));

    QString sPassword_sha1 = sEmail.toUpper() + sPassword;

    std::string _sPassword_sha1 = sha1::calc_string_to_hex(sPassword_sha1.toStdString());
    sPassword_sha1 = QString(_sPassword_sha1.c_str());

    QString sRole = QString::fromStdString(jsonRequest.at("role"));
    if (sRole != "user" && sRole != "admin") {
        Log::err(TAG, "Invalid role format " + sRole.toStdString());
        pRequest->sendMessageError(cmd(), Error(400, "This role doesn't exist"));
        return;
    }
    std::string sUniversity = pRequest->getInputString("university", "");

    QSqlQuery query_insert(db);
    query_insert.prepare(""
                         "INSERT INTO users ("
                         "   uuid, "
                         "   email, "
                         "   pass, "
                         "   role, "
                         "   nick,"
                         "   logo,"
                         "   dt_create,"
                         "   dt_last_login,"
                         "   last_ip,"
                         "   status,"
                         "   country,"
                         "   region,"
                         "   city,"
                         "   university,"
                         "   latitude,"
                         "   longitude,"
                         "   rating,"
                         "   about)"
                         "VALUES("
                         "   :uuid, "
                         "   :email, "
                         "   :pass, "
                         "   :role, "
                         "   :nick,"
                         "   :logo,"
                         "   NOW(),"
                         "   NOW(),"
                         "   :last_ip,"
                         "   :status,"
                         "   :country,"
                         "   :region,"
                         "   :city,"
                         "   :university,"
                         "   :latitude,"
                         "   :longitude,"
                         "   :rating,"
                         "   :about);"
    );

    QString sLastIP = pRequest->client()->peerAddress().toString();

    QString sUuid = "";
    if (jsonRequest.find("uuid") == jsonRequest.end()) {
        sUuid = QString::fromStdString(jsonRequest.at("uuid"));
        sUuid = sUuid.toUpper();
    } else {
        sUuid = QUuid::createUuid().toString();
        sUuid = sUuid.mid(1,sUuid.length()-2);
        sUuid = sUuid.toUpper(); // why to upper ??
    }

    query_insert.bindValue(":uuid", sUuid);
    query_insert.bindValue(":email", sEmail);
    query_insert.bindValue(":pass", sPassword_sha1);
    query_insert.bindValue(":role", sRole);
    query_insert.bindValue(":nick", sNick);
    query_insert.bindValue(":logo", "files/users/0.png");
    query_insert.bindValue(":last_ip", "");
    query_insert.bindValue(":status", "activated");
    query_insert.bindValue(":country", "");
    query_insert.bindValue(":region", "");
    query_insert.bindValue(":city", "");
    query_insert.bindValue(":university", QString::fromStdString(sUniversity));
    query_insert.bindValue(":latitude", 0);
    query_insert.bindValue(":longitude", 0);
    query_insert.bindValue(":rating", 0);
    query_insert.bindValue(":about", "");

    if (!query_insert.exec()) {
        pRequest->sendMessageError(cmd(), Error(500, query_insert.lastError().text().toStdString()));
        return;
    }
    
    int nUserID = query_insert.lastInsertId().toInt();

    nlohmann::json jsonResponse;
    nlohmann::json jsonData;
    jsonData["userid"] = nUserID;
    jsonResponse["data"] = jsonData;
    RunTasks::AddPublicEvents("users", "New [user#" + std::to_string(nUserID) + "] " + sNick.toStdString());
    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

/*********************************************
 * User info (deprecated)
**********************************************/

CmdHandlerUser::CmdHandlerUser()
    : CmdHandlerBase("user", "Return user info") {

    setAccessUnauthorized(true);
    setAccessUser(true);
    setAccessAdmin(true);
    setDeprecatedFromVersion("0.2.17");

    // validation and description input fields
    optionalIntegerParam("userid", "Id of user");
}

// ---------------------------------------------------------------------

void CmdHandlerUser::handle(ModelRequest *pRequest) {
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();

    nlohmann::json jsonRequest = pRequest->jsonRequest();
    nlohmann::json jsonResponse;

    WSJCppUserSession *pUserSession = pRequest->userSession();

    if (jsonRequest.find("userid") != jsonRequest.end() && pUserSession == nullptr) {
        pRequest->sendMessageError(cmd(), Error(401, "Not Authorized Request"));
        return;
    }

    bool bCurrentUserOrAdmin = false;

    int nUserID = 0;
    if (pUserSession != nullptr) {
        nUserID = pUserSession->userid();
        bCurrentUserOrAdmin = true;
    }

    if (jsonRequest.find("userid") != jsonRequest.end()) {
        int nUserID_ = jsonRequest.at("userid").get<int>();
        if (nUserID_ != nUserID) {
            bCurrentUserOrAdmin = pRequest->isAdmin();
        }
        nUserID = nUserID_;
    }

    nlohmann::json data;
    nlohmann::json profile;
    QSqlDatabase db = *(pDatabase->database());

    {
        QSqlQuery query(db);
        query.prepare("SELECT * FROM users WHERE id = :userid");
        query.bindValue(":userid", nUserID);
        query.exec();

        if (query.next()) {
            QSqlRecord record = query.record();
            data["id"] = record.value("id").toInt();
            data["uuid"] = record.value("uuid").toString().toStdString();
            data["nick"] = record.value("nick").toString().toHtmlEscaped().toStdString();
            data["role"] = record.value("role").toString().toStdString();
            data["logo"] = record.value("logo").toString().toHtmlEscaped().toStdString();
            data["about"] = record.value("about").toString().toHtmlEscaped().toStdString();
            data["status"] = record.value("status").toString().toStdString();
            data["rating"] = record.value("rating").toString().toStdString();
            data["university"] = record.value("university").toString().toHtmlEscaped().toStdString();

            if (bCurrentUserOrAdmin) {
                data["email"] = record.value("email").toString().toStdString();
                data["dt_create"] = record.value("dt_create").toString().toStdString();
                data["dt_last_login"] = record.value("dt_last_login").toString().toStdString();
                data["last_ip"] = record.value("last_ip").toString().toStdString();
                data["country"] = record.value("country").toString().toStdString();
                data["region"] = record.value("region").toString().toStdString();
                data["city"] = record.value("city").toString().toStdString();
            }
        } else {
            pRequest->sendMessageError(cmd(), Error(404, "Not found user"));
            return;
        }
    }

    {
        QSqlQuery query(db);
        query.prepare("SELECT name, value, date_change FROM users_profile WHERE userid = :userid");
        query.bindValue(":userid", nUserID);
        query.exec();

        while (query.next()) {
            QSqlRecord record = query.record();
            std::string name = record.value("name").toString().toStdString();
            std::string value = record.value("value").toString().toStdString();
            profile[name] = value;

            // TODO clenup 'template' from user profiles
        }
    }

    jsonResponse["data"] = data;
    jsonResponse["profile"] = profile;
    jsonResponse["access"] = bCurrentUserOrAdmin;
    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

/*********************************************
 * User info
**********************************************/

CmdHandlerUsersInfo::CmdHandlerUsersInfo()
    : CmdHandlerBase("users_info", "Return user info") {

    setAccessUnauthorized(true);
    setAccessUser(true);
    setAccessAdmin(true);
    setActivatedFromVersion("0.2.17");

    // validation and description input fields
    // TODO change to uuid
    // TODO wrong
    requireIntegerParam("uuid", "Global unique identify of user");
}

// ---------------------------------------------------------------------

void CmdHandlerUsersInfo::handle(ModelRequest *pRequest) {
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();

    nlohmann::json jsonRequest = pRequest->jsonRequest();
    nlohmann::json jsonResponse;

    WSJCppUserSession *pUserSession = pRequest->userSession();

     if (jsonRequest.find("userid") == jsonRequest.end() && pUserSession == nullptr) {
        pRequest->sendMessageError(cmd(), Error(401, "Not Authorized Request"));
        return;
    }

    bool bCurrentUserOrAdmin = false;

    int nUserID = 0;
    if (pUserSession != nullptr) {
        nUserID = pUserSession->userid();
        bCurrentUserOrAdmin = true;
    }

    if (jsonRequest.find("userid") != jsonRequest.end()) {
        int nUserID_ = jsonRequest.at("userid").get<int>();
        if (nUserID_ != nUserID) {
            bCurrentUserOrAdmin = pRequest->isAdmin();
        }
        nUserID = nUserID_;
    }

    nlohmann::json jsonData;
    nlohmann::json jsonProfile;
    QSqlDatabase db = *(pDatabase->database());

    {
        QSqlQuery query(db);
        query.prepare("SELECT * FROM users WHERE id = :userid");
        query.bindValue(":userid", nUserID);
        query.exec();

        if (query.next()) {
            QSqlRecord record = query.record();
            jsonData["id"] = record.value("id").toInt();
            jsonData["uuid"] = record.value("uuid").toString().toStdString();
            jsonData["nick"] = record.value("nick").toString().toHtmlEscaped().toStdString();
            jsonData["role"] = record.value("role").toString().toStdString();
            jsonData["logo"] = record.value("logo").toString().toHtmlEscaped().toStdString();
            jsonData["about"] = record.value("about").toString().toHtmlEscaped().toStdString();
            jsonData["status"] = record.value("status").toString().toStdString();
            jsonData["rating"] = record.value("rating").toString().toStdString();
            jsonData["university"] = record.value("university").toString().toHtmlEscaped().toStdString();

            if (bCurrentUserOrAdmin) {
                jsonData["email"] = record.value("email").toString().toStdString();
                jsonData["dt_create"] = record.value("dt_create").toString().toStdString();
                jsonData["dt_last_login"] = record.value("dt_last_login").toString().toStdString();
                jsonData["last_ip"] = record.value("last_ip").toString().toStdString();
                jsonData["country"] = record.value("country").toString().toStdString();
                jsonData["region"] = record.value("region").toString().toStdString();
                jsonData["city"] = record.value("city").toString().toStdString();
            }
        } else {
            pRequest->sendMessageError(cmd(), Error(404, "Not found user"));
            return;
        }
    }

    {
        QSqlQuery query(db);
        query.prepare("SELECT name, value, date_change FROM users_profile WHERE userid = :userid");
        query.bindValue(":userid", nUserID);
        query.exec();

        while (query.next()) {
            QSqlRecord record = query.record();
            std::string sName = record.value("name").toString().toStdString();
            std::string sValue = record.value("value").toString().toStdString();
            jsonProfile[sName] = sValue;

            // TODO clenup 'template' from user profiles
        }
    }

    jsonResponse["data"] = jsonData;
    jsonResponse["profile"] = jsonProfile;
    jsonResponse["access"] = bCurrentUserOrAdmin;
    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

/*********************************************
 * User reset password
**********************************************/

CmdHandlerUserResetPassword::CmdHandlerUserResetPassword()
    : CmdHandlerBase("user_reset_password", "Method for reset password") {

    setAccessUnauthorized(true);
    setAccessUser(false);
    setAccessAdmin(false);

    // validation and description input fields
    requireStringParam("email", "E-mail")
        .addValidator(new ValidatorEmail());
}

// ---------------------------------------------------------------------

void CmdHandlerUserResetPassword::handle(ModelRequest *pRequest) {
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();

    const nlohmann::json& jsonRequest = pRequest->jsonRequest();
    nlohmann::json jsonResponse;

    QString sEmail = QString::fromStdString(jsonRequest.at("email"));

    QSqlDatabase db = *(pDatabase->database());
    QSqlQuery query(db);
    query.prepare("SELECT * FROM users WHERE email = :email");
    query.bindValue(":email", sEmail);
    if (!query.exec()) {
        Log::err(TAG, query.lastError().text().toStdString());
        pRequest->sendMessageError(cmd(), Error(500, query.lastError().text().toStdString()));
        return;
    }
    int nUserID = 0;
    QString sNick = "";
    if (query.next()) {
        QSqlRecord record = query.record();
        nUserID = record.value("id").toInt();
        sNick = record.value("nick").toString().toHtmlEscaped();
    } else {
        Log::err(TAG, "User not found" + sEmail.toStdString());
        pRequest->sendMessageError(cmd(), Error(403, "This email not exists"));
        return;
    }

    // // generate random password
    const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    const int randomStringLength = 12; // assuming you want random strings of 12 characters
    QString sPassword;
    for (int i=0; i<randomStringLength; ++i) {
        int index = qrand() % possibleCharacters.length();
        QChar nextChar = possibleCharacters.at(index);
        sPassword.append(nextChar);
    }

    QString sPassword_sha1 = sEmail.toUpper() + sPassword;
    std::string _sPassword_sha1 = sha1::calc_string_to_hex(sPassword_sha1.toStdString());
    sPassword_sha1 = QString(_sPassword_sha1.c_str());

    QSqlQuery query_update(db);
    query_update.prepare("UPDATE users SET pass = :pass WHERE id = :userid AND email = :email;");

    query_update.bindValue(":pass", sPassword_sha1);
    query_update.bindValue(":userid", nUserID);
    query_update.bindValue(":email", sEmail);

    if (!query_update.exec()) {
        pRequest->sendMessageError(cmd(), Error(500, query_update.lastError().text().toStdString()));
        return;
    }

    RunTasks::AddPublicEvents("users", "User comeback [user#" + std::to_string(nUserID) + "] " + sNick.toStdString());

    std::string sSubject = "Reset Password from FreeHackQuest";
    std::string sContext = "Welcome back to FreeHackQuest!\n"
                       "You login: " + sEmail.toStdString() + "\n"
                       "You password: " + sPassword.toStdString() + "\n";

    RunTasks::MailSend(sEmail.toStdString(), sSubject, sContext);

    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

/*********************************************
 * User skill
**********************************************/

CmdHandlerUserSkills::CmdHandlerUserSkills()
    : CmdHandlerBase("user_skills", "Return user skills info") {

    setAccessUnauthorized(true);
    setAccessUser(true);
    setAccessAdmin(true);

    // validation and description input fields
    requireIntegerParam("userid", "Id of user");
}

// ---------------------------------------------------------------------

void CmdHandlerUserSkills::handle(ModelRequest *pRequest) {
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();

    const nlohmann::json& jsonRequest = pRequest->jsonRequest();
    nlohmann::json jsonResponse;


    nlohmann::json jsonSkillsMax;
    nlohmann::json jsonSkillsUser;

    QSqlDatabase db = *(pDatabase->database());

    {
        QSqlQuery query(db);
        query.prepare("SELECT q.subject, sum(q.score) as sum_subject FROM quest q WHERE ! ISNULL( q.subject ) GROUP BY q.subject");
        if (!query.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query.lastError().text().toStdString()));
            return;
        };

        while (query.next()) {
            QSqlRecord record = query.record();
            QString subject = record.value("subject").toString();
            jsonSkillsMax[subject.toStdString()] = record.value("sum_subject").toInt();
        }
    }


    int nUserID = jsonRequest.at("userid");
    {
        QSqlQuery query(db);
        query.prepare("SELECT uq.userid, q.subject, SUM( q.score ) as sum_score FROM users_quests uq INNER JOIN quest q ON uq.questid = q.idquest WHERE ! ISNULL( q.subject ) AND uq.userid = :userid GROUP BY uq.userid, q.subject");
        query.bindValue(":userid", nUserID);
        if (!query.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query.lastError().text().toStdString()));
            return;
        };

        while (query.next()) {
            QSqlRecord record = query.record();
            QString subject = record.value("subject").toString();
            jsonSkillsUser[subject.toStdString()] = record.value("sum_score").toInt();
        }
    }

    jsonResponse["skills_max"] = jsonSkillsMax;
    jsonResponse["skills_user"] = jsonSkillsUser;

    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

/*********************************************
 * User update
**********************************************/

CmdHandlerUserUpdate::CmdHandlerUserUpdate()
    : CmdHandlerBase("user_update", "Update user info") {

    setAccessUnauthorized(false);
    setAccessUser(true);
    setAccessAdmin(true);

    // validation and description input fields
    requireIntegerParam("userid", "Id of user");
    optionalStringParam("nick", "Nick of user");
    optionalStringParam("university", "University of user");
    optionalStringParam("about", "About of user");
    optionalStringParam("country", "Country of user");
}

// ---------------------------------------------------------------------

void CmdHandlerUserUpdate::handle(ModelRequest *pRequest) {
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();

    const nlohmann::json& jsonRequest = pRequest->jsonRequest();
    nlohmann::json jsonResponse;
    nlohmann::json data;

    WSJCppUserSession *pUserSession = pRequest->userSession();
    int nUserIDFromToken = pUserSession->userid();
    int nUserID = pRequest->getInputInteger("userid", 0);
    if (nUserIDFromToken != nUserID && !pRequest->isAdmin()) {
        pRequest->sendMessageError(cmd(), Error(403, "Deny change inmormation about user"));
        return;
    }

    QString sNick = "";
    QString sUniversity = "";
    QString sAbout = "";
    std::string sCountry = "";
    QString sCreated = "";
    QString sRegion = "";
    QString sEmail = "";
    QString sRole = "";
    QString sUpdated = "";
    QString sUuid = "";

    QSqlDatabase db = *(pDatabase->database());
    {
        QSqlQuery query(db);
        query.prepare("SELECT * FROM users WHERE id = :userid");
        query.bindValue(":userid", nUserID);

        if (!query.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query.lastError().text().toStdString()));
            return;
        };

        if (!query.next()) {
            pRequest->sendMessageError(cmd(), Error(404, "User not found"));
            return;
        } else {
            QSqlRecord record = query.record();
            sNick = record.value("nick").toString();
            sUniversity = record.value("university").toString();
            sAbout = record.value("about").toString();
            sCountry = record.value("country").toString().toStdString();
            sCreated = record.value("dt_create").toString();
            sRegion = record.value("region").toString();
            sEmail = record.value("email").toString();
            sRole = record.value("role").toString();
            sUuid = record.value("uuid").toString();
        }
    }

    if (jsonRequest.find("nick") != jsonRequest.end()) {
        sNick = QString::fromStdString(jsonRequest.at("nick").get<std::string>());
    }

    if (jsonRequest.find("university") != jsonRequest.end()) {
        sUniversity = QString::fromStdString(jsonRequest.at("university").get<std::string>());
    }

    if (jsonRequest.find("about") != jsonRequest.end()) {
        sAbout = QString::fromStdString(jsonRequest.at("about").get<std::string>());
    }
    std::string s = jsonRequest.dump();
    Log::warn(TAG, "jsonRequest " + s);
    if (jsonRequest.find("country") != jsonRequest.end()) {
        sCountry = jsonRequest["country"];
    }

    // update
    {
        QSqlQuery query(db);
        query.prepare("UPDATE users SET "
            " nick = :nick, "
            " university = :university, "
            " about = :about, "
            " country = :country "
            " WHERE "
            " id = :userid");

        query.bindValue(":nick", sNick);
        query.bindValue(":university", sUniversity);
        query.bindValue(":about", sAbout);
        query.bindValue(":country", QString::fromStdString(sCountry));
        query.bindValue(":userid", nUserID);
        if (!query.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query.lastError().text().toStdString()));
            return;
        };
    }

    pUserSession->setNick(sNick);
    RunTasks::AddPublicEvents("users", "User [user#" + std::to_string(nUserID) + "]  " + sNick.toStdString()
                              + " updated info");

    data["id"] = nUserID;
    data["nick"] = sNick.toHtmlEscaped().toStdString();
    data["university"] = sUniversity.toHtmlEscaped().toStdString();
    data["about"] = sAbout.toHtmlEscaped().toStdString();
    data["country"] = QString::fromStdString(sCountry).toHtmlEscaped().toStdString();
    data["created"] = sCreated.toHtmlEscaped().toStdString();
    data["region"] = sRegion.toHtmlEscaped().toStdString();
    data["email"] = sEmail.toHtmlEscaped().toStdString();
    data["role"] = sRole.toHtmlEscaped().toStdString();
    data["uuid"] = sUuid.toHtmlEscaped().toStdString();

    jsonResponse["data"] = data;

    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

/*********************************************
 * User delete
**********************************************/

CmdHandlerUserDelete::CmdHandlerUserDelete()
    : CmdHandlerBase("user_delete", "Method for deleting a user") {

    setAccessUnauthorized(false);
    setAccessUser(false);
    setAccessAdmin(true);

    // validation and description input fields
    requireIntegerParam("userid", "User's id");
    requireStringParam("password", "Admin's password");
}

// ---------------------------------------------------------------------

void CmdHandlerUserDelete::handle(ModelRequest *pRequest) {
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();

    const nlohmann::json& jsonRequest = pRequest->jsonRequest();
    nlohmann::json jsonResponse;

    QString sAdminPassword = QString::fromStdString(jsonRequest.at("password"));

    WSJCppUserSession *pUserSession = pRequest->userSession();
    int nAdminUserID = pUserSession->userid();

    QSqlDatabase db = *(pDatabase->database());

    // check admin password
    {
        QSqlQuery query(db);
        query.prepare("SELECT * FROM users WHERE id = :userid");
        query.bindValue(":userid", nAdminUserID);
        if (!query.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query.lastError().text().toStdString()));
            return;
        }

        QString sPass = "";
        QString sEmail = "";

        if (query.next()) {
            QSqlRecord record = query.record();
            sEmail = record.value("email").toString();
            sPass = record.value("pass").toString();
        } else {
            pRequest->sendMessageError(cmd(), Error(404, "Not found user"));
            return;
        }

        QString sAdminPasswordHash = sEmail.toUpper() + sAdminPassword;
        std::string _sAdminPasswordHash = sha1::calc_string_to_hex(sAdminPasswordHash.toStdString());
        sAdminPasswordHash = QString(_sAdminPasswordHash.c_str());

        if (sAdminPasswordHash != sPass) {
            pRequest->sendMessageError(cmd(), Error(401, "Wrong password"));
            return;
        }
    }

    int nUserID = jsonRequest.at("userid");

    // check if the user exists
    {
        QSqlQuery query(db);
        query.prepare("SELECT * FROM users WHERE id = :id");
        query.bindValue(":id", nUserID);

        if (!query.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query.lastError().text().toStdString()));
            return;
        }

        if (!query.next()) {
            pRequest->sendMessageError(cmd(), Error(404, "User not found"));
            return;
        }
    }

    QSqlQuery query_del(db);

    // delete from feedback
    {
        query_del.prepare("DELETE FROM feedback WHERE userid = :userid");
        query_del.bindValue(":userid", nUserID);
        if (!query_del.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query_del.lastError().text().toStdString()));
            return;
        }
    }

    // delete from feedback_msg
    {
        query_del.prepare("DELETE FROM feedback_msg WHERE userid = :userid");
        query_del.bindValue(":userid", nUserID);
        if (!query_del.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query_del.lastError().text().toStdString()));
            return;
        }
    }

    // delete from quest
    {
        query_del.prepare("DELETE FROM quest WHERE userid = :userid");
        query_del.bindValue(":userid", nUserID);
        if (!query_del.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query_del.lastError().text().toStdString()));
            return;
        }
    }

    // delete from users_games
    {
        query_del.prepare("DELETE FROM users_games WHERE userid = :userid");
        query_del.bindValue(":userid", nUserID);
        if (!query_del.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query_del.lastError().text().toStdString()));
            return;
        }
    }

    // delete from users_profile
    {
        query_del.prepare("DELETE FROM users_profile WHERE userid = :userid");
        query_del.bindValue(":userid", nUserID);
        if (!query_del.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query_del.lastError().text().toStdString()));
            return;
        }
    }

    // delete from users_quests
    {
        query_del.prepare("DELETE FROM users_quests WHERE userid = :userid");
        query_del.bindValue(":userid", nUserID);
        if (!query_del.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query_del.lastError().text().toStdString()));
            return;
        }
    }

    // delete from users_tokens
    {
        query_del.prepare("DELETE FROM users_tokens WHERE userid = :userid");
        query_del.bindValue(":userid", nUserID);
        if (!query_del.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query_del.lastError().text().toStdString()));
            return;
        }
    }

    // delete from users_tokens_invalid
    {
        query_del.prepare("DELETE FROM users_tokens_invalid WHERE userid = :userid");
        query_del.bindValue(":userid", nUserID);
        if (!query_del.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query_del.lastError().text().toStdString()));
            return;
        }
    }

    // delete from users_offers
    {
        query_del.prepare("DELETE FROM users_offers WHERE userid = :userid");
        query_del.bindValue(":userid", nUserID);
        if (!query_del.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query_del.lastError().text().toStdString()));
            return;
        }
    }

    // delete from quests_proposal
    {
        query_del.prepare("DELETE FROM quests_proposal WHERE userid = :userid");
        query_del.bindValue(":userid", nUserID);
        if (!query_del.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query_del.lastError().text().toStdString()));
            return;
        }
    }

    // delete from users_quests_answers
    {
        query_del.prepare("DELETE FROM users_quests_answers WHERE userid = :userid");
        query_del.bindValue(":userid", nUserID);
        if (!query_del.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query_del.lastError().text().toStdString()));
            return;
        }
    }

    // delete from users
    {
        query_del.prepare("DELETE FROM users WHERE id = :id");
        query_del.bindValue(":id", nUserID);
        if (!query_del.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query_del.lastError().text().toStdString()));
            return;
        }
    }

    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

/*********************************************
 * Users
**********************************************/

CmdHandlerUsers::CmdHandlerUsers()
    : CmdHandlerBase("users", "Method return list of users") {

    TAG = "CmdUsersHandler";

    setAccessUnauthorized(false);
    setAccessUser(false);
    setAccessAdmin(true);

    // validation and description input fields
    optionalStringParam("filter_text", "Filter by user email or nick");
    optionalStringParam("filter_role", "Filter by user role"); // TODO validator role
    optionalIntegerParam("onpage", "On Page");
    optionalIntegerParam("page", "Number of page");
}

// ---------------------------------------------------------------------

void CmdHandlerUsers::handle(ModelRequest *pRequest) {
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();

    const nlohmann::json& jsonRequest = pRequest->jsonRequest();
    nlohmann::json jsonResponse;


    QStringList filters;
    QMap<QString,QString> filter_values;

    int nPage = 0;
    int nOnPage = 5;
    int nCount = 0;

    if (jsonRequest.find("filter_text") != jsonRequest.end()) {
        QString text = QString::fromStdString(jsonRequest.at("filter_text"));
        if (text != "") {
            filters << "(email LIKE :email OR nick LIKE :nick)";
            filter_values[":email"] = "%" + text + "%";
            filter_values[":nick"] = "%" + text + "%";
        }
    }
    if (jsonRequest.find("filter_role") != jsonRequest.end()) {
        QString role = QString::fromStdString( jsonRequest["filter_role"].get_ref<std::string const&>() ).trimmed();
        if (role != "") {
            filters << "role = :role";
            filter_values[":role"] = role;
        }
    }

    if (jsonRequest.find("page") != jsonRequest.end()) {
        nPage = jsonRequest.at("page");
    }

    if (jsonRequest.find("onpage") != jsonRequest.end()) {
        nOnPage = jsonRequest.at("onpage");
    }

    nlohmann::json jsonUsers = nlohmann::json::array();
    QSqlDatabase db = *(pDatabase->database());
    QString where = filters.join(" AND ");
    if (where.length() > 0) {
        where = "WHERE " + where;
    }

    // calculate count
    {
        QSqlQuery query(db);
        query.prepare("SELECT COUNT(*) cnt FROM users " + where);
        foreach (QString key, filter_values.keys()) {
            query.bindValue(key, filter_values.value(key));
        }
        if (!query.exec()) {
            pRequest->sendMessageError(cmd(), Error(500, query.lastError().text().toStdString()));
            return;
        }
        if (query.next()) {
            QSqlRecord record = query.record();
            nCount = record.value("cnt").toInt();
        }
    }

    // users
    {
        QSqlQuery query(db);
        query.prepare("SELECT * FROM users " + where + " ORDER BY dt_last_login DESC LIMIT " + QString::number(nPage*nOnPage) + "," + QString::number(nOnPage));
        foreach (QString key, filter_values.keys()) {
            query.bindValue(key, filter_values.value(key));
        }
        query.exec();
        while (query.next()) {
            QSqlRecord record = query.record();
            int userid = record.value("id").toInt();
            QString sUuid = record.value("uuid").toString();
            QString sEmail = record.value("email").toString();
            QString sNick = record.value("nick").toString().toHtmlEscaped();
            QString sUniversity = record.value("university").toString().toHtmlEscaped();
            int nRating = record.value("rating").toInt();
            QString sCreated = record.value("dt_create").toString().toHtmlEscaped();
            QString sDTLastLogin = record.value("dt_last_login").toString().toHtmlEscaped();
            QString sLastIP = record.value("last_ip").toString().toHtmlEscaped();
            QString sCountry = record.value("country").toString().toHtmlEscaped();
            QString sRegion = record.value("region").toString().toHtmlEscaped();
            QString sCity = record.value("city").toString().toHtmlEscaped();
            QString sRole = record.value("role").toString().toHtmlEscaped();
            nlohmann::json user;
            user["id"] = userid;
            user["uuid"] = sUuid.toStdString();
            user["nick"] = sNick.toStdString();
            user["email"] = sEmail.toStdString();
            user["created"] = sCreated.toStdString();
            user["dt_last_login"] = sDTLastLogin.toStdString();
            user["last_ip"] = sLastIP.toStdString();
            user["university"] = sUniversity.toStdString();
            user["rating"] = nRating;
            user["country"] = sCountry.toStdString();
            user["region"] = sRegion.toStdString();
            user["city"] = sCity.toStdString();
            user["role"] = sRole.toStdString();
            jsonUsers.push_back(user);
        }
    }

    jsonResponse["data"] = jsonUsers;
    jsonResponse["onpage"] = nOnPage;
    jsonResponse["page"] = nPage;
    jsonResponse["count"] = nCount;
    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

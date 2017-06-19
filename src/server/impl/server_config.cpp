#include "../headers/server_config.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

ServerConfig::ServerConfig(){
	// default settings
    QStringList sFilenames;
    sFilenames << "/etc/freehackquest-backend/conf.ini";
    sFilenames << "etc/freehackquest-backend/conf.ini";
    sFilenames << "conf.ini";
    for(int i = 0; i < sFilenames.size(); i++){
        QString tmp = sFilenames[i];
        if(QFile::exists(tmp)){
            m_sFilename = tmp;
            qDebug() << "[Info] Found config file " << tmp;
            break;
        }else{
            qDebug() << "[Warning] Not found possible config file " << tmp;
        }
    }

	m_bServer_ssl_on = false;
	m_sDatabase_host = "localhost";
	m_sDatabase_name = "freehackquest";
	m_sDatabase_user = "freehackquest_u";
	m_sDatabase_password = "freehackquest_p";

	m_sEmail_smtphost = "smtp.gmail.com";
	m_nEmail_smtpport = 465;
	m_sEmail_username = "";
	m_sEmail_password = "";

	m_nServer_port = 1234;
	m_bServer_ssl_on = false;
	m_nServer_ssl_port = 4613;
	m_sServer_ssl_key_file = "/etc/ssl/private/localhost.key";
	m_sServer_ssl_cert_file = "/etc/ssl/certs/localhost.pem";
};

// ---------------------------------------------------------------------

bool ServerConfig::load(){
    if(m_sFilename == ""){
        qDebug() << "[ERROR] Not found config file ";
		return false;
	}

    QSettings sett(m_sFilename, QSettings::IniFormat);
	m_sDatabase_host = readStringFromSettings(sett, "DATABASE/host", m_sDatabase_host);
	m_sDatabase_name = readStringFromSettings(sett, "DATABASE/name", m_sDatabase_name);
	m_sDatabase_user = readStringFromSettings(sett, "DATABASE/user", m_sDatabase_user);
	m_sDatabase_password = readStringFromSettings(sett, "DATABASE/password", m_sDatabase_password);
	
	qDebug() << "Database_host: " << m_sDatabase_host;
	qDebug() << "Database_name: " << m_sDatabase_name;
	qDebug() << "Database_user: " << m_sDatabase_user;
	
	m_sEmail_smtphost = readStringFromSettings(sett, "EMAIL/host", m_sEmail_smtphost);
	m_nEmail_smtpport = readIntFromSettings(sett, "EMAIL/port", m_nEmail_smtpport);
	m_sEmail_username = readStringFromSettings(sett, "EMAIL/username", m_sEmail_username);
	m_sEmail_password = readStringFromSettings(sett, "EMAIL/password", m_sEmail_password);

	m_nServer_port = readIntFromSettings(sett, "SERVER/port", m_nServer_port);
	m_bServer_ssl_on = readBoolFromSettings(sett, "SERVER/ssl_on", m_bServer_ssl_on);
	m_nServer_ssl_port = readIntFromSettings(sett, "SERVER/ssl_port", m_nServer_ssl_port);
	m_sServer_ssl_key_file = readStringFromSettings(sett, "SERVER/ssl_key_file", m_sServer_ssl_key_file);
	m_sServer_ssl_cert_file = readStringFromSettings(sett, "SERVER/ssl_cert_file", m_sServer_ssl_cert_file);
	return true;
}

// ---------------------------------------------------------------------

QString ServerConfig::readStringFromSettings(QSettings &sett, QString settName, QString defaultValue){
	QString sResult = defaultValue;
	if(sett.contains(settName)){
		sResult = sett.value(settName, sResult).toString();
	}else{
		qDebug() << "[WARNING] " << settName << " - not found in " << m_sFilename << "\n\t Will be used default value: " << defaultValue;
	}
	return sResult;
}

// ---------------------------------------------------------------------

int ServerConfig::readIntFromSettings(QSettings &sett, QString settName, int defaultValue){
	int nResult = defaultValue;
	if(sett.contains(settName)){
		nResult = sett.value(settName, nResult).toInt();
	}else{
		qDebug() << "[WARNING] " << settName << " - not found in " << m_sFilename << "\n\t Will be used default value: " << defaultValue;
	}
	return nResult;
}

// ---------------------------------------------------------------------

bool ServerConfig::readBoolFromSettings(QSettings &sett, QString settName, bool defaultValue){
	bool bResult = defaultValue;
	if(sett.contains(settName)){
		bResult = sett.value(settName, bResult).toBool();
	}else{
		qDebug() << "[WARNING] " << settName << " - not found in " << m_sFilename << "\n\t Will be used default value: " << defaultValue;
	}
	return bResult;
}

// ---------------------------------------------------------------------

QString ServerConfig::databaseHost(){
	return m_sDatabase_host;
}

// ---------------------------------------------------------------------

QString ServerConfig::databaseName(){
	return m_sDatabase_name;
}

// ---------------------------------------------------------------------

QString ServerConfig::databaseUser(){
	return m_sDatabase_user;
}

// ---------------------------------------------------------------------

QString ServerConfig::databasePassword(){
	return m_sDatabase_password;
}

// ---------------------------------------------------------------------

QString ServerConfig::emailUsername(){
	return m_sEmail_username;
}

// ---------------------------------------------------------------------

QString ServerConfig::emailPassword(){
	return m_sEmail_password;
}

// ---------------------------------------------------------------------

QString ServerConfig::emailSmtpHost(){
	return m_sEmail_smtphost;
}

// ---------------------------------------------------------------------

int ServerConfig::emailSmtpPort(){
	return m_nEmail_smtpport;
}

// ---------------------------------------------------------------------

bool ServerConfig::serverSslOn(){
	return m_bServer_ssl_on;
}

// ---------------------------------------------------------------------

int ServerConfig::serverPort(){
	return m_nServer_port;
}

// ---------------------------------------------------------------------

int ServerConfig::serverSslPort(){
	return m_nServer_ssl_port;
}

// ---------------------------------------------------------------------

QString ServerConfig::serverSslKeyFile(){
	return m_sServer_ssl_key_file;
}

// ---------------------------------------------------------------------

QString ServerConfig::serverSslCertFile(){
	return m_sServer_ssl_cert_file;
}

// ---------------------------------------------------------------------

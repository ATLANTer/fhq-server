#include "../headers/update0072.h"
#include <QSqlQuery>


QString Update0072::from_version(){
	return "u0071";
}

QString Update0072::version(){
	return "u0072";
}

QString Update0072::description(){
	return "Added chatmessages table";
}

void Update0072::update(QSqlDatabase &db){
	QSqlQuery query(db);
	query.prepare(
		"CREATE TABLE IF NOT EXISTS `chatmessages` ("
		"	  `id` int(11) NOT NULL AUTO_INCREMENT,"
		"	  `user` varchar(128) NOT NULL,"
		"	  `message` varchar(255) NOT NULL,"
		"	  `dt` datetime NOT NULL,"
		"	  PRIMARY KEY (`id`)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8 AUTO_INCREMENT=1;");
	query.exec();
}

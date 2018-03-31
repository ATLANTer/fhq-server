#include <update0071.h>
#include <QSqlQuery>

Update0071::Update0071(){
    TAG = "Update0071";
}

QString Update0071::from_version(){
	return "u0070";
}

QString Update0071::version(){
	return "u0071";
}

QString Update0071::description(){
	return "Added classbook table";
}

bool Update0071::update(QSqlDatabase &db, QString &error){
	QSqlQuery query(db);
	query.prepare(
		"CREATE TABLE IF NOT EXISTS `classbook` ("
		"	  `id` int(11) NOT NULL AUTO_INCREMENT,"
		"	  `parentid` int(11) NOT NULL,"
		"	  `uuid` varchar(128) NOT NULL,"
		"	  `parentuuid` varchar(128) NOT NULL,"
		"	  `name_ru` varchar(128) NOT NULL,"
		"	  `name_en` varchar(128) NOT NULL,"
		"	  `dt` datetime NOT NULL,"
		"	  PRIMARY KEY (`id`)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8 AUTO_INCREMENT=1;");
	if(!query.exec()){
		error = query.lastError().text();
        Log::err(TAG, "The problem with creating a table " + error);
		return false;
	}
	return true;
}

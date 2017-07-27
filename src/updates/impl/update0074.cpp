#include "../headers/update0074.h"

QString Update0074::from_version(){
	return "u0073";
}

QString Update0074::version(){
	return "u0074";
}

QString Update0074::description(){
	return "Added feedback.from";
}

bool Update0074::update(QSqlDatabase &db, QString &error){
	QSqlQuery query(db);
	query.prepare("ALTER TABLE `feedback` ADD COLUMN `from` VARCHAR(255) DEFAULT '';");
	if(!query.exec()){
		error = query.lastError().text();
		return false;
	}
	return true;
}

#include "../headers/update0079.h"

QString Update0079::from_version(){
	return "u0078";
}

QString Update0079::version(){
	return "u0079";
}

QString Update0079::description(){
	return "Rename table tryanswer to users_quests_answers";
}

bool Update0079::update(QSqlDatabase &db, QString &error){
	QSqlQuery query(db);
	query.prepare("RENAME TABLE tryanswer TO users_quests_answers;");
	if(!query.exec()){
		error = query.lastError().text();
		return false;
	}
	return true;
}

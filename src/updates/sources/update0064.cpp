#include <update0064.h>

Update0064::Update0064(){
    TAG = "Update0064";
}

QString Update0064::from_version(){
	return "u0063";
}

QString Update0064::version(){
	return "u0064";
}

QString Update0064::description(){
	return "Remove personal quests which passed";
}

bool Update0064::update(QSqlDatabase &db, QString &error){
	QSqlQuery query(db);
	query.prepare("SELECT idquest FROM quest LEFT JOIN users_quests ON users_quests.questid = quest.idquest WHERE for_person <> 0");
	if(!query.exec()){
		error = query.lastError().text();
        Log::err(TAG, "The problem with data selection " + error);
		return false;
	}

	while (query.next()) {
		QSqlRecord record = query.record();
		int questid = record.value("idquest").toInt();
		QSqlQuery query2(db);
		query2.prepare("DELETE FROM quest WHERE idquest = :questid");
		query2.bindValue(":questid", questid);
		query2.exec();
		if(!query2.exec()){
			error = query2.lastError().text();
            Log::err(TAG, "The problem with deleting data " + error);
			return false;
		}
	}
	return true;
}

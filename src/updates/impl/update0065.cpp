#include "../headers/update0065.h"
#include <QUuid>

QString Update0065::from_version(){
	return "u0064";
}

QString Update0065::version(){
	return "u0065";
}

QString Update0065::description(){
	return "Update quests uuid";
}

void Update0065::update(QSqlDatabase &db){
	QSqlQuery query(db);
	query.prepare("SELECT idquest FROM quest WHERE isnull(quest_uuid)");
	query.exec();

	while (query.next()) {
		QSqlRecord record = query.record();
		int questid = record.value("idquest").toInt();
		QString questuuid = QUuid::createUuid().toString();
		questuuid = questuuid.mid(1,questuuid.length()-2);
		QSqlQuery query2(db);
		query2.prepare("UPDATE quest SET quest_uuid = :questuuid WHERE idquest = :questid AND isnull(quest_uuid)");
		query2.bindValue(":questuuid", questuuid);
		query2.bindValue(":questid", questid);
		query2.exec();
	}
}

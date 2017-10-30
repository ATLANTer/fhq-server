#include <update0067.h>
#include <QSqlQuery>

Update0067::Update0067(){
    TAG = "Update0067";
}

QString Update0067::from_version(){
	return "u0066";
}

QString Update0067::version(){
	return "u0067";
}

QString Update0067::description(){
	return "Added column copyright to quest";
}

bool Update0067::update(QSqlDatabase &db, QString &error){
	QSqlQuery query(db);
	query.prepare("ALTER TABLE quest ADD COLUMN copyright VARCHAR(255);");
	if(!query.exec()){
		error = query.lastError().text();
        Log::err(TAG, "The problem with altering the table " + error);
		return false;
	}
	return true;
}

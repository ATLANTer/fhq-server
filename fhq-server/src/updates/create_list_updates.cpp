#include "create_list_updates.h"
#include <database0060.h>
#include <update0061_0090.h>
#include <update0091.h>
#include <update0092.h>
#include <update0093.h>
#include <update0094.h>
#include <update0095.h>
#include <update0096.h>
#include <update0097.h>

void create_list_updates(QVector<IUpdate *> &vUpdates){
	vUpdates.push_back(new Database0060());
	vUpdates.push_back(new Update0061());
	vUpdates.push_back(new Update0062());
	vUpdates.push_back(new Update0063());
	vUpdates.push_back(new Update0064());
	vUpdates.push_back(new Update0065());
	vUpdates.push_back(new Update0066());
	vUpdates.push_back(new Update0067());
	vUpdates.push_back(new Update0068());
	vUpdates.push_back(new Update0069());
	vUpdates.push_back(new Update0070());
	vUpdates.push_back(new Update0071());
	vUpdates.push_back(new Update0072());
	vUpdates.push_back(new Update0073());
	vUpdates.push_back(new Update0074());
	vUpdates.push_back(new Update0075());
	vUpdates.push_back(new Update0076());
	vUpdates.push_back(new Update0077());
	vUpdates.push_back(new Update0078());
	vUpdates.push_back(new Update0079());
	vUpdates.push_back(new Update0080());
	vUpdates.push_back(new Update0081());
	vUpdates.push_back(new Update0082());
	vUpdates.push_back(new Update0083());
	vUpdates.push_back(new Update0084());
	vUpdates.push_back(new Update0085());
	vUpdates.push_back(new Update0086());
	vUpdates.push_back(new Update0087());
	vUpdates.push_back(new Update0088());
	vUpdates.push_back(new Update0089());
    vUpdates.push_back(new Update0090());
    vUpdates.push_back(new Update0091());
    vUpdates.push_back(new Update0092());
    vUpdates.push_back(new Update0093());
    vUpdates.push_back(new Update0094());
    vUpdates.push_back(new Update0095());
    vUpdates.push_back(new Update0096());
    vUpdates.push_back(new Update0097());
}

bool tryUpdateDatabase(QSqlDatabase *pDatabase){
	QString TAG = "tryUpdateDatabase";
	QSqlDatabase db = *pDatabase;
	QSqlQuery query(db);

	query.prepare("SELECT * FROM updates ORDER BY id DESC LIMIT 0,1");
	query.exec();
	QString last_version = "";
	if (query.next()) {
		QSqlRecord record = query.record();
		// int updateid = record.value("id").toInt();
		QString version = record.value("version").toString();
		QString description = record.value("description").toString();
		last_version = version;
	}
	
	Log::info(TAG, "last_version: " + last_version);
	QVector<IUpdate *> vUpdates;
	create_list_updates(vUpdates);

	bool bHasUpdates = true;
	while(bHasUpdates){
		bHasUpdates = false;
		for(int i = 0; i < vUpdates.size(); i++){
			IUpdate* pUpdate = vUpdates[i];
			if(last_version == pUpdate->from_version()){
				Log::info(TAG, "Installing update " + pUpdate->from_version() + " -> " + pUpdate->version() + ": " + pUpdate->description());
				last_version = pUpdate->version();
				bHasUpdates = true;
				QString error;
				if(!pUpdate->update(db, error)){
					Log::err(TAG, "Error on install update " + error);
                    return false;
				}

				QSqlQuery insert_query(db);
				insert_query.prepare("INSERT INTO updates (version, description, datetime_update) "
					  "VALUES (:version, :description, NOW())");
				insert_query.bindValue(":version", pUpdate->version());
				insert_query.bindValue(":description", pUpdate->description());
				if(!insert_query.exec()){
					Log::err(TAG, "Could not insert row to updates: " + insert_query.lastError().text());
                    return false;
				}
			}
		}
	}
    return true;
}

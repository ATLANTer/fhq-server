#include <cmd_handler_quest_delete.h>
#include <runtasks.h>
#include <employ_database.h>
#include <employ_server_info.h>

CmdHandlerQuestDelete::CmdHandlerQuestDelete(){

    m_modelCommandAccess.setAccessUnauthorized(false);
    m_modelCommandAccess.setAccessUser(false);
    m_modelCommandAccess.setAccessAdmin(true);

    // validation and description input fields
	m_vInputs.push_back(CmdInputDef("questid").required().integer_().description("Quest ID"));
}

// ---------------------------------------------------------------------

std::string CmdHandlerQuestDelete::cmd(){
    return "quest_delete";
}

// ---------------------------------------------------------------------

const ModelCommandAccess & CmdHandlerQuestDelete::access(){
    return m_modelCommandAccess;
}

// ---------------------------------------------------------------------

const std::vector<CmdInputDef> &CmdHandlerQuestDelete::inputs(){
	return m_vInputs;
}

// ---------------------------------------------------------------------

std::string CmdHandlerQuestDelete::description(){
	return "Method for delete quest";
}

// ---------------------------------------------------------------------

void CmdHandlerQuestDelete::handle(ModelRequest *pRequest){
    EmployDatabase *pDatabase = findEmploy<EmployDatabase>();
    EmployServerInfo *pServerInfo = findEmploy<EmployServerInfo>();

    QJsonObject jsonRequest = pRequest->data();
    QJsonObject jsonResponse;

    int questid = jsonRequest["questid"].toInt();
	QString sName = "";
	QString sSubject = "";
    QSqlDatabase db = *(pDatabase->database());
	{
		QSqlQuery query(db);
		query.prepare("SELECT * FROM quest WHERE idquest = :questid");
		query.bindValue(":questid", questid);
		if(!query.exec()){
            pRequest->sendMessageError(cmd(), Error(500, query.lastError().text()));
			return;
		}
		if (query.next()) {
			QSqlRecord record = query.record();
			sName = record.value("name").toString();
			sSubject = record.value("subject").toString();
		}else{
            pRequest->sendMessageError(cmd(), Error(404, "Quest not found"));
			return;
		}
	}
	
	{
		QSqlQuery query(db);
		query.prepare("DELETE FROM quest WHERE idquest = :questid");
		query.bindValue(":questid", questid);
		if(!query.exec()){
            pRequest->sendMessageError(cmd(), Error(500, query.lastError().text()));
			return;
		}
	}

	// remove from users_quests_answers
	{
		QSqlQuery query(db);
		query.prepare("DELETE FROM users_quests_answers WHERE questid = :questid");
		query.bindValue(":questid", questid);
		if(!query.exec()){
            pRequest->sendMessageError(cmd(), Error(500, query.lastError().text()));
			return;
		}
	}

	// remove from users_quests
	{
		QSqlQuery query(db);
		query.prepare("DELETE FROM users_quests WHERE questid = :questid");
		query.bindValue(":questid", questid);
		if(!query.exec()){
            pRequest->sendMessageError(cmd(), Error(500, query.lastError().text()));
			return;
		}
	}
    pServerInfo->decrementQuests();

    RunTasks::AddPublicEvents("quests", "Removed quest #" + QString::number(questid) + " " + sName + " (subject: " + sSubject + ")");

	// todo recalculate rating/score for users how solved this quest

    jsonResponse["subject"] = sSubject;
    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

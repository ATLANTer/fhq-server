#include <cmd_quests_subjects_handler.h>
#include <QJsonArray>

CmdQuestsSubjectsHandler::CmdQuestsSubjectsHandler(){

    m_modelCommandAccess.setAccessUnauthorized(true);
    m_modelCommandAccess.setAccessUser(true);
    m_modelCommandAccess.setAccessAdmin(true);

    // validation and description input fields
	// m_vInputs.push_back(CmdInputDef("filter_text").string_().optional().description("Filter by user email or nick"));
	// m_vInputs.push_back(CmdInputDef("filter_role").string_().optional().description("Filter by user role"));
}

// ---------------------------------------------------------------------

std::string CmdQuestsSubjectsHandler::cmd(){
    return "quests_subjects";
}

// ---------------------------------------------------------------------

const ModelCommandAccess & CmdQuestsSubjectsHandler::access(){
    return m_modelCommandAccess;
}

// ---------------------------------------------------------------------

const std::vector<CmdInputDef> &CmdQuestsSubjectsHandler::inputs(){
	return m_vInputs;
}

// ---------------------------------------------------------------------

std::string CmdQuestsSubjectsHandler::description(){
	return "Method returned list of games";
}

// ---------------------------------------------------------------------

void CmdQuestsSubjectsHandler::handle(ModelRequest *pRequest){
    QJsonObject jsonRequest = pRequest->data();
    QJsonObject jsonResponse;

    QJsonArray subjects;
    QSqlDatabase db = *(pRequest->server()->database());

	QSqlQuery query(db);
    query.prepare("SELECT subject, COUNT(*) as cnt FROM `quest` WHERE quest.state = :state GROUP BY subject");
    query.bindValue(":state", "open");

    if(!query.exec()){
        pRequest->sendMessageError(cmd(), Error(500, query.lastError().text()));
        return;
    }

	while (query.next()) {
		QSqlRecord record = query.record();
        QJsonObject subject;
        subject["subject"] = record.value("subject").toString();
        subject["count"] = record.value("cnt").toInt();
        subjects.push_back(subject);
	}

    jsonResponse["data"] = subjects;
    pRequest->sendMessageSuccess(cmd(), jsonResponse);

}

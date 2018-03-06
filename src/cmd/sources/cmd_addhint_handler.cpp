#include <cmd_addhint_handler.h>
#include <log.h>
#include <runtasks.h>

CmdAddHintHandler::CmdAddHintHandler(){
	TAG = "CmdAddHintHandler";

    m_modelCommandAccess.setAccessUnauthorized(false);
    m_modelCommandAccess.setAccessUser(false);
    m_modelCommandAccess.setAccessAdmin(true);

    // validation and description input fields
	m_vInputs.push_back(CmdInputDef("questid").required().integer_().description("quest id"));
	m_vInputs.push_back(CmdInputDef("hint").required().string_().description("hint text"));
}

// ---------------------------------------------------------------------

std::string CmdAddHintHandler::cmd(){
	return "addhint";
}

// ---------------------------------------------------------------------

std::string CmdAddHintHandler::description(){
    return "Methid add hint to quest";
}

// ---------------------------------------------------------------------

const ModelCommandAccess & CmdAddHintHandler::access(){
    return m_modelCommandAccess;
}

// ---------------------------------------------------------------------

const std::vector<CmdInputDef> &CmdAddHintHandler::inputs(){
	return m_vInputs;
}

// ---------------------------------------------------------------------

void CmdAddHintHandler::handle(ModelRequest *pRequest){
    QJsonObject jsonRequest = pRequest->data();
    QJsonObject jsonResponse;

    int questid = jsonRequest["questid"].toInt();
	if(questid == 0){
        pRequest->sendMessageError(cmd(), Errors::QuestIDMustBeNotZero());
		return;
	}

    QSqlDatabase db = *(pRequest->server()->database());
	QSqlQuery query(db);
	query.prepare("INSERT INTO quests_hints (questid, text, dt) VALUES (:questid, :text, NOW())");
    query.bindValue(":questid", jsonRequest["questid"].toInt());
    query.bindValue(":text", jsonRequest["hint"].toString());
	if(!query.exec()){
		Log::err(TAG, query.lastError().text());
	}

    RunTasks::AddPublicEvents(pRequest->server(), "quests", "Added hint for quest #" + QString::number(questid));
    pRequest->sendMessageSuccess(cmd(), jsonResponse);
}

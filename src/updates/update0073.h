#ifndef UPDATE_0073_H
#define UPDATE_0073_H

#include "../interfaces/iupdate.h"

#include <QString>
#include <QVariant>

class Update0073 : public IUpdate {
	
	public:
		virtual QString from_version();
		virtual QString version();
		virtual QString name();
		virtual QString description();
		virtual void update(QSqlDatabase &db);
};

#endif // UPDATE_0073_H

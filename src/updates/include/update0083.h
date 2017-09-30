#ifndef UPDATE_0083_H
#define UPDATE_0083_H

#include <iupdate.h>

class Update0083 : public IUpdate {
	
	public:
		virtual QString from_version();
		virtual QString version();
		virtual QString description();
		virtual bool update(QSqlDatabase &db, QString &error);
};

#endif // UPDATE_0083_H

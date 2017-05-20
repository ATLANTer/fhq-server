#ifndef UPDATE_0072_H
#define UPDATE_0072_H

#include <iupdate.h>

#include <QString>
#include <QVariant>

class Update0072 : public IUpdate {
	
	public:
		virtual QString from_version();
		virtual QString version();
		virtual QString name();
		virtual QString description();
		virtual void update(QSqlDatabase &db);
};

#endif // UPDATE_0072_H

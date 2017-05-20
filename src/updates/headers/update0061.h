#ifndef UPDATE_0061_H
#define UPDATE_0061_H

#include <iupdate.h>

class Update0061 : public IUpdate {
	
	public:
		virtual QString from_version();
		virtual QString version();
		virtual QString description();
		virtual void update(QSqlDatabase &db);
};

#endif // UPDATE_0061_H

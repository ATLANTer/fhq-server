#ifndef UPDATE_0074_H
#define UPDATE_0074_H

#include <iupdate.h>

class Update0074 : public IUpdate {
	
	public:
		virtual QString from_version();
		virtual QString version();
		virtual QString description();
		virtual void update(QSqlDatabase &db);
};

#endif // UPDATE_0074_H

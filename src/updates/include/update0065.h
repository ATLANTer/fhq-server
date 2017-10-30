#ifndef UPDATE_0065_H
#define UPDATE_0065_H

#include <iupdate.h>

class Update0065 : public IUpdate {
	
    public:
        Update0065();
        virtual QString from_version();
        virtual QString version();
        virtual QString description();
        virtual bool update(QSqlDatabase &db, QString &error);

    private:
        QString TAG;
};

#endif // UPDATE_0065_H

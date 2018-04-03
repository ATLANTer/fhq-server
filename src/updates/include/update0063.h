#ifndef UPDATE_0063_H
#define UPDATE_0063_H

#include <iupdate.h>

class Update0063 : public IUpdate {
	
    public:
        Update0063();
        virtual QString from_version();
        virtual QString version();
        virtual QString description();
        virtual bool update(QSqlDatabase &db, QString &error);

    private:
        QString TAG;
};

#endif // UPDATE_0063_H
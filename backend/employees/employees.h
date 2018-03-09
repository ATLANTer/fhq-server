#ifndef EMPLOYEES_H
#define EMPLOYEES_H

#include <iemploy.h>
#include <log.h>
#include <map>

extern std::map<std::string, IEmploy*> *g_pEmployees;

// RegistryEmploy

template <typename T> class RegistryEmploy {
public:
    RegistryEmploy() {
        if(g_pEmployees == NULL){
            g_pEmployees = new std::map<std::string, IEmploy*>();
        }

        std::string sEmployName = T::getEmployName();
        std::cout << "Try register employ " << sEmployName << "\n";
        if(g_pEmployees->count(sEmployName)){
            Log::err("RegistryEmploy", sEmployName + " -  already registered");
        }else{
            IEmploy *pEmploy = new T();
            g_pEmployees->insert(std::pair<std::string, IEmploy*>(sEmployName,pEmploy));
        }
    }
};

#define REGISTRY_EMPLOY( classname ) \
    static RegistryEmploy<classname> registry ## classname; \


// findEmploy

template <class T> T* findEmploy() {

    /*if(g_pEmployees == NULL){
        initEmployees();
    }*/
    std::string sEmployName = T::getEmployName();
    // Log::err("findEmploy", "Find employ " + QString(sEmployName.c_str()));
    // Log::err("findEmploy", "Employees count: " + QString::number(g_pEmployees->size()));

    IEmploy *pEmploy = NULL;
    if(g_pEmployees->count(sEmployName)){
        pEmploy = g_pEmployees->at(sEmployName);
    }
    if(pEmploy == NULL){
        Log::err("findEmploy", "Not found employ " + QString(sEmployName.c_str()));
        return NULL;
    }
    T *pTEmploy = dynamic_cast<T*>(pEmploy);
    if(pTEmploy == NULL){
        Log::err("findEmploy", "Employ could not cast to T [" + QString(sEmployName.c_str()) + "]");
        return NULL;
    }
    return pTEmploy;
}




#endif // EMPLOYEES_H

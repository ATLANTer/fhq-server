#ifndef MODEL_LEAK_H
#define MODEL_LEAK_H

#include <string>
#include <json.hpp>

class ModelLeak {
    public:
        ModelLeak();
        
        int id();
        void setId(int nId);
        const std::string &uuid();
        void setUuid(std::string sUuid);
        const std::string &gameUuid();
        void setGameUuid(std::string nGameUuid);
        const std::string &name();
        void setName(std::string sName);
        const std::string &content();
        void setContent(std::string sContent);
        int score();
        void setScore(int nScore);
        const std::string &created();
        void setCreated(std::string sCreated);
        const std::string &updated();
        void setUpdated(std::string sUpdated);
        int sold();
        void setSold(int nSold);
        
        nlohmann::json toJson();
        
    private:
        std::string TAG;
        int m_nId;
        std::string m_sUuid;
        std::string m_sGameUuid;
        std::string m_sName;
        std::string m_sContent;
        int m_nScore;
        std::string m_sCreated;
        std::string m_sUpdated;
        int m_nSold;
};

#endif // MODEL_LEAK_H

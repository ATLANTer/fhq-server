#ifndef MODEL_LXD_CONTAINER
#define MODEL_LXD_CONTAINER

#include <string>
#include <curl/curl.h>
#include <QFile>
#include <employ_settings.h>
#include <json.hpp>

class LXDContainer {
    public:
        LXDContainer(QJsonObject jsonData);
        std::string get_name();
        std::string get_status();
        std::string get_IPv4();
        std::string get_error();
        std::string set_prefix();

        nlohmann::json state();
        bool create(std::string name);
        bool start();
        bool stop();
        bool remove();
        bool read_file(std::string path, QFile & file);
        std::string exec(std::string);


    private:
        std::string response;
        std::string name;
        std::string status;
        std::string IPv4;
        std::string error;
        std::string prefix;
};

#endif // MODEL_LXD_CONTAINER


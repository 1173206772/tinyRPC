#include <tinyxml/tinyxml.h>
#include <string>
#include "rocket/common/config.h"

#define READ_XML_NODE(name, parent)                                                     \
    TiXmlElement* name##_node = parent->FirstChildElement(#name);                       \
    if(!name##_node) {                                                                  \
        printf("Start rocket server error, failed to read node [%s]\n", #name);          \
        exit(0);                                                                        \
    }                                                                                   \

#define READ_STR_FROM_XML_NODE(name, parent)                                            \
    TiXmlElement* name##_node = parent->FirstChildElement(#name);                       \
    if(!name##_node || !name##_node->GetText()) {                                       \
        printf("Start rocket server error, failed to read config file %s\n", #name);    \
        exit(0);                                                                        \
    }                                                                                   \
    std::string name##_str = std::string(name##_node->GetText());                           \


namespace rocket {

static Config* g_config = nullptr;
Config* Config::GetGlobalConfig() {
    return g_config;
}
void Config::SetGlobalConfig(const char* xmlfile) {
    if(!g_config) {
        if(xmlfile != NULL) {
            g_config = new Config(xmlfile);
        }else {
            g_config = new Config();
        }
    }
    
}

Config::Config() {
    m_log_level = "DEBUG";    
}


Config::Config(const char* xmlfile)  {
    TiXmlDocument* xml_document = new TiXmlDocument();
    bool rt = xml_document->LoadFile(xmlfile);
    if(!rt) {
        printf("Start rocket server error, failed to read config file %s error info[%s]\n", xmlfile, xml_document->ErrorDesc()); 
        exit(0);
    }
    READ_XML_NODE(root, xml_document);
    READ_XML_NODE(log, root_node);
    READ_XML_NODE(server, root_node);
    
    READ_STR_FROM_XML_NODE(log_level, log_node);
    READ_STR_FROM_XML_NODE(log_file_name, log_node);
    READ_STR_FROM_XML_NODE(log_file_path, log_node);
    READ_STR_FROM_XML_NODE(log_max_file_size, log_node);
    READ_STR_FROM_XML_NODE(log_async_interval, log_node);

    m_log_level = log_level_str;
    m_log_file_name = log_file_name_str;
    m_log_file_path = log_file_path_str;
    m_log_max_file_size = std::stoi(log_max_file_size_str);
    m_log_async_interval = std::stoi(log_async_interval_str);

    printf("LOG -- CONFIG LEVEL[%s], FILE_NAME [%s], MAX_FILE_SIZE[%d B], SYNC_INTERVAL[%d ms]\n",
            m_log_level.c_str(), (m_log_file_path + m_log_file_name).c_str(), m_log_max_file_size, m_log_async_interval);

    READ_STR_FROM_XML_NODE(io_threads, server_node);
    READ_STR_FROM_XML_NODE(port, server_node);
    m_port = std::stoi(port_str);
    m_io_threads = std::stoi(io_threads_str);

    printf("SERVER -- PORT [%d], IO THREADS NUM [%d]\n",
            m_port, m_io_threads);


}

}
#ifndef _CCONFIGDATA_
#define _CCONFIGDATA_

#include <string>
#include "ini.h"

class CSenderData
{
public:
    CSenderData() : from(""), localSender(false), textDisclaimerPath(""), htmlDisclaimerPath("") {}

    std::string from;
    bool localSender;
    std::string textDisclaimerPath;
    std::string htmlDisclaimerPath;
};

class CConfigData
{
public:
    CConfigData();
    ~CConfigData();

    bool readConfig(const std::string & _file);

    // Milter Config Data
    std::string socketPath;
    std::string socketPathStripped;
    std::string group;
    std::string tmpPath;
    std::string logFile;
    int logLevel;

    bool addDisclaimer;
    bool addExternalNotice;


    // AlterMime COnfig Data
    std::string altermimePath;

    std::unordered_map<std::string, CSenderData> senderData;

    std::string textDisclaimerPath;
    std::string htmlDisclaimerPath;

    void printLoadedConfig();

private:

    void m_readMilterConfigData(const mINI::INIMap<std::string>& _collection);
    void m_readAlterMimeConfigData(const mINI::INIMap<std::string>& _collection);
    void m_readSenderConfigData(const mINI::INIMap<std::string>& _collection);

    void m_getString(const std::pair<std::string, std::string>& _key, const std::string& _aspectedKey, std::string& _value);
    void m_getBool(const std::pair<std::string, std::string>& _key, const std::string& _aspectedKey, bool& _value);
    void m_getInt(const std::pair<std::string, std::string>& _key, const std::string& _aspectedKey, int& _value);
};

#endif
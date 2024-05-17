#include "configdata.h"
#include "spdlog/spdlog.h"
#include <iostream>


CConfigData::CConfigData()
    : socketPath("local:/var/spool/postfix/altermime/altermime.sock")
    , socketPathStripped("/var/spool/postfix/altermime/altermime.sock")
    , group("postfix")

    , tmpPath("/tmp/milter_altermime/")

    , logFile("/tmp/milter_altermime_log/milter.log")
    , logLevel(1)

    , addDisclaimer(true)
    , addExternalNotice(false)

    , altermimePath("/usr/bin/altermime")
    , textDisclaimerPath("/etc/postfix/disclaimer.txt")
    , htmlDisclaimerPath("/etc/postfix/disclaimer.html")
{

}


CConfigData::~CConfigData()
{

}

bool CConfigData::readConfig(const std::string & _file)
{
    // first, create a file instance
    mINI::INIFile file(_file.c_str());

    // next, create a structure that will hold data
    mINI::INIStructure ini;

    // now we can read the file
    file.read(ini);

    for(auto const & e : ini)
    {
        auto const& section = e.first;
        auto const& collection = e.second;

        std::cout << "reading section ..." << std::endl;

        if(section.find("milter") != std::string::npos)
        {
            m_readMilterConfigData(collection);
        }
        else if(section.find("altermime") != std::string::npos)
        {
            m_readAlterMimeConfigData(collection);
        }
        else
        {
            m_readSenderConfigData(collection);
        }
    }

    return true;
}

void CConfigData::m_readMilterConfigData(const mINI::INIMap<std::string>& _collection)
{
    for (auto& it2 : _collection)
	{
        m_getString(it2, "socket", socketPath);
        m_getString(it2, "group", group);
        m_getString(it2, "tmp_path", tmpPath);
        m_getString(it2, "log_file", logFile);
        m_getInt(it2, "log_level", logLevel);


        m_getBool(it2, "add_disclaimer", addDisclaimer);
        m_getBool(it2, "add_external_notice", addExternalNotice);
	}

    if(socketPath.length() > 0)
    {
        size_t idx = socketPath.find(":", 0);

        socketPathStripped = socketPath.substr(idx + 1);
    }
}


void CConfigData::m_readAlterMimeConfigData(const mINI::INIMap<std::string>& _collection)
{
    for (auto& it2 : _collection)
	{
        m_getString(it2, "altermime_path", altermimePath);
	}
}


void CConfigData::m_readSenderConfigData(const mINI::INIMap<std::string>& _collection)
{
    CSenderData data;
    bool enabled = false;

    for (auto& it2 : _collection)
	{
        m_getBool(it2, "enabled", enabled);
        m_getBool(it2, "localsender", data.localSender);
        m_getString(it2, "from", data.from);
        m_getString(it2, "text", data.textDisclaimerPath);
        m_getString(it2, "html", data.htmlDisclaimerPath);
	}

    if(!enabled)
    {
        return;
    }

    senderData[data.from] = data;    
}


void CConfigData::m_getString(const std::pair<std::string, std::string>& _key, const std::string& _aspectedKey, std::string& _value)
{
    if(_key.first.find(_aspectedKey) == std::string::npos)
    {
        return;
    }

    _value = _key.second;
}


void CConfigData::m_getBool(const std::pair<std::string, std::string>& _key, const std::string& _aspectedKey, bool& _value)
{
    if(_key.first.find(_aspectedKey) == std::string::npos)
    {
        return;
    }

    int i = strtol(_key.second.c_str(), nullptr, 10);

    _value = (i == 1) ? true : false;
}

void CConfigData::m_getInt(const std::pair<std::string, std::string>& _key, const std::string& _aspectedKey, int& _value)
{
    if(_key.first.find(_aspectedKey) == std::string::npos)
    {
        return;
    }

    _value = strtol(_key.second.c_str(), nullptr, 10);
}


void CConfigData::printLoadedConfig()
{
    spdlog::get("MilterAM")->debug("SocketPath: " + socketPath);
    spdlog::get("MilterAM")->debug("Group: " + group);
    spdlog::get("MilterAM")->debug("Temp Path: " + tmpPath);
    spdlog::get("MilterAM")->debug("Log Path: " + logFile);
    spdlog::get("MilterAM")->debug("Log Level: " + std::to_string(logLevel));
    spdlog::get("MilterAM")->debug("Add Disclaimer: " + std::to_string(addDisclaimer));
    spdlog::get("MilterAM")->debug("Add External Notice: " + std::to_string(addExternalNotice));
    spdlog::get("MilterAM")->debug("--------");
    spdlog::get("MilterAM")->debug("AlterMime Path: " + altermimePath);
    spdlog::get("MilterAM")->debug("--------");
    spdlog::get("MilterAM")->debug("DomainData");
    
    for(auto & it : senderData)
    {
        spdlog::get("MilterAM")->debug("From: " + it.second.from);
        spdlog::get("MilterAM")->debug("Local sender: " + std::to_string(it.second.localSender));
        spdlog::get("MilterAM")->debug("Text disclaimer path: " + it.second.textDisclaimerPath);
        spdlog::get("MilterAM")->debug("Htlm disclaimer path: " + it.second.htmlDisclaimerPath);
    }

}

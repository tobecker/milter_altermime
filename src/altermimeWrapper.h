#ifndef _CAlterMimeWrapper_
#define _CAlterMimeWrapper_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

#include "configdata.h"

class CAlterMimeWrapper 
{
public:
    CAlterMimeWrapper(std::shared_ptr<CConfigData> _data);
    ~CAlterMimeWrapper();

    bool checkForActions(char **envfrom);

    bool openFile();
    bool closeFile();
    bool writeHeader(std::string _headerField, std::string _headerValue);
    bool writeEOH();
    bool writeBody(unsigned char * _bodyp, size_t _bodylen);
    bool runAlterMime();

    bool getModifiedBody(unsigned char ** _modBodyPtr, size_t & _modBodyLen);
    bool getModifiedBody(std::vector<unsigned char> & _modBodyData);


    bool removeFiles();

    bool isFooterAvaible() { return m_bFooterAvaible; }
    bool isExternalAvaible() { return m_bExternalAvaible; }

    std::vector<std::string> getFilesToDelete();

    std::string ipAddress;
    bool localSender;

private:

    bool m_existsFile (const std::string& _name);
    int m_getFileSize(const std::string& _name);

    bool m_findDisclaimerInBody(std::string _fileDisclaimer, std::vector<unsigned char> & _bodyData);

    std::string m_genRandom(const int len);

    std::shared_ptr<CConfigData> m_configData;

    std::ofstream m_fileHeader;
    std::ofstream m_fileBody;
    std::ofstream m_fileCombined;

    std::string m_baseFilename;

    std::string m_senderAddr;

    bool m_bFooterAvaible;
    bool m_bExternalAvaible;

    std::string m_header;
    bool m_addEOH;
    std::vector<unsigned char> m_body;

};

#endif
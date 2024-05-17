#include <ctime>
#include <unistd.h>
#include <sstream>
#include <sys/stat.h>

#include "altermimeWrapper.h"

#include "spdlog/spdlog.h"

//constexpr char baseDirectory[] = "/tmp/milter_altermime/";

constexpr char headerName[] = "_header.txt";
constexpr char combinedName[] = "_comb.txt";

CAlterMimeWrapper::CAlterMimeWrapper(std::shared_ptr<CConfigData> _data)
    : m_configData(_data)
    , m_baseFilename(_data->tmpPath)

    , m_bFooterAvaible(false)
    , m_bExternalAvaible(false)

    , m_header()
    , m_addEOH(false)
    , m_body()
{
    struct stat junk;
	if (stat(m_baseFilename.c_str(), &junk) != 0)
    {
        mkdir(m_baseFilename.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    m_baseFilename.append(m_genRandom(20));
}

CAlterMimeWrapper::~CAlterMimeWrapper()
{

}

bool CAlterMimeWrapper::checkForActions(char **envfrom)
{
    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : checkForActions : enter");

    // first element of the array holds the sender
    std::string from(envfrom[0]);

    std::cout << "MailWriter::checkForActions from: " << from.c_str() << std::endl;
    std::cout << "MailWriter::checkForActions from length: " << std::to_string(from.length()) << std::endl;

    size_t idx = from.find("@");

    if(idx == std::string::npos && !localSender)
    {
        std::cout << "MailWriter::checkForActions no @ found" << std::endl;
        return false;
    }
    else if(localSender)
    {
        for(auto & it : m_configData->senderData)
        {
            if(it.second.localSender)
            {
                m_senderAddr = it.first;
                m_bFooterAvaible = true;

                std::cout << "MailWriter::checkForActions found: " << m_senderAddr << std::endl;

                return true;
            }
        }
    }

    size_t idxCamp = from.find(">");

    if(idxCamp == std::string::npos)
    {
        idxCamp = from.length();
    }

    std::cout << "MailWriter::checkForActions idx: " << std::to_string(idx) << " / idxCamp: " << std::to_string(idxCamp) << std::endl;


    std::string strWOSender = from.substr(idx, idxCamp - idx);

    std::cout << "MailWriter::checkForActions str wo sender: " << strWOSender << std::endl;

    auto it = m_configData->senderData.find(strWOSender);

    std::cout << "MailWriter::checkForActions str wo sender: " << strWOSender << std::endl;

    if(it == m_configData->senderData.end())
    {
        strWOSender = from.substr(idx + 1, idxCamp - idx);

        std::cout << "MailWriter::checkForActions str wo sender: " << strWOSender << std::endl;
        it = m_configData->senderData.find(strWOSender);

        if(it == m_configData->senderData.end())
        {
            std::cout << "MailWriter::checkForActions not found" << std::endl;
            return false;
        }
    }

    std::cout << "MailWriter::checkForActions found: " << strWOSender << std::endl;
    m_senderAddr = strWOSender;


    // set flag for footer avaibility
    m_bFooterAvaible = true;

    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : checkForActions : exit");

    return true;
}


bool CAlterMimeWrapper::openFile()
{
    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : openFile : enter");

    m_fileHeader.open(m_baseFilename + headerName, std::ios::out);

    if(!m_fileHeader.is_open())
    {
        return false;
    }

    m_fileCombined.open(m_baseFilename + combinedName, std::ios::out);

    if(!m_fileCombined.is_open())
    {
        return false;
    }

    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : openFile : exit");

    return true;
}


bool CAlterMimeWrapper::closeFile()
{
    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : closeFile : enter");

    if(m_fileHeader.is_open())
    {
        m_fileHeader.close();
    }

    if(m_fileCombined.is_open())
    {
        m_fileCombined.close();
    }

    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : closeFile : exit");

    return true;
}

std::vector<std::string> CAlterMimeWrapper::getFilesToDelete()
{
    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : getFilesToDelete : enter");

    std::vector<std::string> vec;

    vec.push_back(m_baseFilename + headerName);
    vec.push_back(m_baseFilename + combinedName);


    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : getFilesToDelete : exit");

    return vec;
}

bool CAlterMimeWrapper::m_existsFile (const std::string& name)
{
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

bool CAlterMimeWrapper::removeFiles()
{
    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : removeFiles : enter");

    //if(m_existsFile(m_baseFilename + headerName))
    //{
        unlink(std::string(m_baseFilename + headerName).c_str());
    //}

    std::cout << "MailWriter::removeFiles 2" << std::endl;

    //if(m_existsFile(m_baseFilename + combinedName))
    //{
        unlink(std::string(m_baseFilename + combinedName).c_str());
    //}

    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : removeFiles : exit");

    return true;
}


bool CAlterMimeWrapper::writeHeader(std::string _headerField, std::string _headerValue)
{
    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : writeHeader : enter");

    if(!m_fileHeader.is_open() || !m_fileCombined.is_open())
    {
        spdlog::get("MilterAM")->error("CAlterMimeWrapper : writeHeader : files not open");
        return false;
    }

    m_fileHeader << _headerField.c_str() << ": " << _headerValue.c_str() << "\r\n";
    m_fileCombined << _headerField.c_str() << ": " << _headerValue.c_str() << "\r\n";

    m_header.append(_headerField + ": " + _headerValue + "\r\n");

    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : writeHeader : exit");

    return true;
}


bool CAlterMimeWrapper::writeEOH()
{
    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : writeEOH : enter");

    if(!m_fileCombined.is_open())
    {
        spdlog::get("MilterAM")->error("CAlterMimeWrapper : writeEOH : files not open");
        return false;
    }

    m_fileCombined << "\r\n";
    m_addEOH = true;

    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : writeEOH : exit");

    return true;
}


bool CAlterMimeWrapper::writeBody(unsigned char * _bodyPtr, size_t _bodyLen)
{
    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : writeBody : enter");

    if(!m_fileCombined.is_open())
    {
        spdlog::get("MilterAM")->error("CAlterMimeWrapper : writeBody : file not open");
        return false;
    }

    m_fileCombined.write(reinterpret_cast<char *>(_bodyPtr), _bodyLen);

    m_body.resize(_bodyLen, 0);
    memcpy(&m_body[0], _bodyPtr, _bodyLen);


    m_findDisclaimerInBody(m_configData->senderData[m_senderAddr].textDisclaimerPath, m_body);
    m_findDisclaimerInBody(m_configData->senderData[m_senderAddr].htmlDisclaimerPath, m_body);

    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : writeBody : exit");

    return true;
}

bool CAlterMimeWrapper::m_findDisclaimerInBody(std::string _fileDisclaimer, std::vector<unsigned char> & _bodyData)
{
    if(_fileDisclaimer.length() == 0 || _bodyData.size() == 0)
    { 
        return false;
    }

    std::ifstream discFile(_fileDisclaimer, std::ios::in);

    if(!discFile.is_open())
    {
        spdlog::get("MilterAM")->error("CAlterMimeWrapper : writeBody  : file discFile not opened");
        return -1;
    }

    // get size of file
    discFile.seekg(0, discFile.end);
    long discFileLen = discFile.tellg();
    discFile.seekg(0);

    std::vector<unsigned char> discData;
    // allocate memory for file content
    discData.resize(discFileLen, 0);

    // read content of modifiedMail
    discFile.read(reinterpret_cast<char*>(&discData[0]), discFileLen);

    discFile.close();

    // std::string bodyDataStr(&_bodyData[0]);
    // std::string discDataStr(&discData){0]};

    // if(bodyDataStr.find(discDataStr) == std::string::npos)
    // {
    //     return false;
    // }

    return true;
}


bool CAlterMimeWrapper::runAlterMime()
{
    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : runAlterMime : enter");

    std::string cmd = "\"";
    cmd.append(m_configData->altermimePath);
    cmd.append("\" --input=");
    cmd.append(m_baseFilename + combinedName);
    cmd.append(" --disclaimer=");
    cmd.append(m_configData->senderData[m_senderAddr].textDisclaimerPath);
    cmd.append(" --disclaimer-html=");
    cmd.append(m_configData->senderData[m_senderAddr].htmlDisclaimerPath);

    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : runAlterMime : cmd: " + cmd);
    std::cout << "Run cmd: " << cmd.c_str() << std::endl;

    system(cmd.c_str());

    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : runAlterMime : exit");

    return true;
}


int CAlterMimeWrapper::m_getFileSize(const std::string& _name)
{
    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : m_getFileSize : enter");

    std::ifstream inFile(_name, std::ios::in);

    if(!inFile.is_open())
    {
        spdlog::get("MilterAM")->error("CAlterMimeWrapper : m_getFileSize : file not opened");
        return -1;
    }

    inFile.seekg(0, inFile.end);
    int fileSize = inFile.tellg();
    inFile.close();

    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : m_getFileSize : exit");

    return fileSize;
}


bool CAlterMimeWrapper::getModifiedBody(std::vector<unsigned char> & _modBodyData)
{
    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : getModifiedBody : enter");

    std::ifstream modifiedMail(m_baseFilename + combinedName, std::ios::in);

    if(!modifiedMail.is_open())
    {
        spdlog::get("MilterAM")->error("CAlterMimeWrapper : getModifiedBody : file not opened");
        return false;
    }

    // get size of file
    long sizeHeader = m_getFileSize(m_baseFilename + headerName);

    if(sizeHeader == -1)
    {
        spdlog::get("MilterAM")->error("CAlterMimeWrapper : getModifiedBody : no header size");
        return false;
    }
    
    // add two bytes for '\r\n' between header and body
    sizeHeader += 2;

    // get size of file
    modifiedMail.seekg(0, modifiedMail.end);
    long modBodyLen = modifiedMail.tellg();
    modifiedMail.seekg(sizeHeader);

    // subtract header size from body size
    modBodyLen -= sizeHeader;

    // allocate memory for file content
    _modBodyData.resize(modBodyLen, 0);

    // read content of modifiedMail
    modifiedMail.read(reinterpret_cast<char*>(&_modBodyData[0]), modBodyLen);

    modifiedMail.close();

    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : getModifiedBody : exit");

    return true;
}


std::string CAlterMimeWrapper::m_genRandom(const int len) 
{
    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : m_genRandom : enter");

    srand((unsigned)time(NULL) * getpid()); 

    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    spdlog::get("MilterAM")->debug("CAlterMimeWrapper : m_genRandom : exit");
    
    return tmp_s;
}

#include "sinks/file_sink.h"

#include "core/logging/logger.h"
#include "custom_io_operators.h"
#include "data/events/datastore_store_event.h"
#include "data/events/file_log_event.h"
#include "data/events/ubx_event.h"
#include "utility/conversion.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/syscall.h>
#include <unistd.h>

using namespace std::literals;

FileSink::FileSink(EventBus& bus, std::uint32_t fileSizeMB)
    : bus_{bus}
    , m_fileSizeMB{fileSizeMB}
    , dailyUploadTime{11h + 11min + 11s}
    , lastRotationDateTime{std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())}
    , nextRotationTime{generateNextDailyTime(dailyUploadTime)} {
    std::string fullPath{MuonPi::Config::data_path};
    configPath = fullPath + "/";
    configFilePath = fullPath + "/currentWorkingFileInformation.conf";
    dataFolderPath = fullPath + "/data/";
    std::filesystem::create_directories(dataFolderPath);
    start();
}

FileSink::~FileSink() {
    std::scoped_lock lock{m_mutex};
    closeFiles();
}

void FileSink::handle(const UbxTimeMarkStruct& tm) {
    std::stringstream sstr;
    sstr << tm.rising << tm.falling << tm.accuracy_ns << " " << tm.evtCounter << " "
         << static_cast<short>(tm.valid) << " " << static_cast<short>(tm.timeBase) << " "
         << static_cast<short>(tm.utcAvailable);
    writeToDataFile(sstr.str());
}

void FileSink::handle(const FileLogEvent& event) {
    writeToLogFile(event.msg);
}

auto FileSink::generateNextDailyTime(std::chrono::system_clock::duration offset)
    -> std::chrono::system_clock::time_point {
    auto now = std::chrono::system_clock::now();

    auto today = floor<std::chrono::days>(now);
    auto candidate = today + offset;

    if (candidate <= now)
        candidate += std::chrono::days(1);

    return candidate;
}
std::string FileSink::getCurrentDataFileName() const {
    std::scoped_lock lock{m_mutex};
    return getCurrentDataFileNameUnlocked();
}

std::string FileSink::getCurrentLogFileName() const {
    std::scoped_lock lock{m_mutex};
    return getCurrentLogFileNameUnlocked();
}

std::string FileSink::getCurrentDataFileNameUnlocked() const {
    return currentWorkingFilePath.empty() ? std::string{} : currentWorkingFilePath.string();
}

std::string FileSink::getCurrentLogFileNameUnlocked() const {
    return currentWorkingLogPath.empty() ? std::string{} : currentWorkingLogPath.string();
}

auto FileSink::dataFileInfo() const -> FileSink::FileInfo {
    std::scoped_lock lock{m_mutex};
    namespace fs = std::filesystem;

    FileInfo info;
    info.path = currentWorkingFilePath;

    std::error_code ec;

    info.size = fs::file_size(currentWorkingFilePath, ec);
    if (ec)
        info.size = 0;

    info.modified = fs::last_write_time(currentWorkingFilePath, ec);

    return info;
}

auto FileSink::logFileInfo() const -> FileSink::FileInfo {
    namespace fs = std::filesystem;

    FileInfo info;
    info.path = currentWorkingLogPath;

    std::error_code ec;

    info.size = fs::file_size(currentWorkingLogPath, ec);
    if (ec)
        info.size = 0;

    info.modified = fs::last_write_time(currentWorkingLogPath, ec);

    return info;
}

// return current log file age in s
auto FileSink::currentLogAge() const -> std::chrono::seconds {
    std::scoped_lock lock{m_mutex};
    return currentLogAgeUnlocked();
}

std::chrono::seconds FileSink::currentLogAgeUnlocked() const {
    namespace fs = std::filesystem;
    using namespace std::chrono;

    if (!dataFile.is_open())
        return seconds{-1};

    if (configFilePath.empty())
        return seconds{-1};

    std::error_code ec;

    auto ftime = fs::last_write_time(configFilePath, ec);
    if (ec)
        return seconds{-1};

    auto now = fs::file_time_type::clock::now();

    auto age = duration_cast<seconds>(now - ftime);

    return age;
}

LogInfoStruct::status_t FileSink::getStatus() const {
    std::scoped_lock lock{m_mutex};
    return getStatusUnlocked();
}

LogInfoStruct::status_t FileSink::getStatusUnlocked() const {
    LogInfoStruct::status_t status{LogInfoStruct::status_t::NORMAL};
    if (std::filesystem::exists(currentWorkingFilePath) == false) {
        status = LogInfoStruct::status_t::ERROR_STATE;
    }
    if (dataFile.is_open() == false) {
        status = LogInfoStruct::status_t::ERROR_STATE;
    }
    if (std::filesystem::exists(currentWorkingLogPath) == false) {
        status = LogInfoStruct::status_t::ERROR_STATE;
    }
    if (logFile.is_open() == false) {
        status = LogInfoStruct::status_t::ERROR_STATE;
    }
    return status;
}

LogInfoStruct FileSink::getInfoUnlocked() const {
    LogInfoStruct lis{};

    lis.logFileName = getCurrentLogFileNameUnlocked();
    lis.dataFileName = getCurrentDataFileNameUnlocked();
    lis.status = getStatusUnlocked();

    try {
        lis.logFileSize = std::filesystem::file_size(currentWorkingLogPath);
    } catch (const std::filesystem::filesystem_error&) {
        lis.logFileSize = 0;
    }

    try {
        lis.dataFileSize = std::filesystem::file_size(currentWorkingFilePath);
    } catch (const std::filesystem::filesystem_error&) {
        lis.dataFileSize = 0;
    }

    lis.logAge = currentLogAgeUnlocked();
    lis.logRotationDuration = logRotationPeriodUnlocked();
    lis.logEnabled = true;

    return lis;
}

void FileSink::publishInfoUnlocked() {
    if (dataFile.is_open()) {
        dataFile.flush();
    }
    if (logFile.is_open()) {
        logFile.flush();
    }
    bus_.publish(DatastoreStoreEvent{getInfoUnlocked()});
}

LogInfoStruct FileSink::getInfo() const {
    std::scoped_lock lock{m_mutex};
    return getInfoUnlocked();
}

std::chrono::seconds FileSink::logRotationPeriod() const {
    std::scoped_lock lock{m_mutex};
    return logRotationPeriodUnlocked();
}

std::chrono::seconds FileSink::logRotationPeriodUnlocked() const {
    return m_logrotate_period;
}

void FileSink::setLogRotationPeriod(std::chrono::seconds period) {
    std::scoped_lock lock{m_mutex};
    setLogRotationPeriodUnlocked(period);
}

void FileSink::setLogRotationPeriodUnlocked(std::chrono::seconds period) {
    m_logrotate_period = period;
}

void FileSink::start() {
    // open files that are currently written
    openFiles();
}

// SLOTS
void FileSink::onRotationRemind() {
    std::scoped_lock lock{m_mutex};
    try {
        if (std::filesystem::file_size(currentWorkingFilePath) > (1024 * 1024 * m_fileSizeMB)) {
            rotateFiles();
        }
    } catch (const std::filesystem::filesystem_error&) {
        logWarn("Cold not read file size: " + currentWorkingFilePath.string());
    }

    try {
        if (std::filesystem::file_size(currentWorkingLogPath) > (1024 * 1024 * m_fileSizeMB)) {
            rotateFiles();
        }
    } catch (const std::filesystem::filesystem_error&) {
        logWarn("Cold not read file size: " + currentWorkingLogPath.string());
    }

    auto now = std::chrono::system_clock::now();
    if (now < nextRotationTime) {
        publishInfoUnlocked();
        return;
    }
    rotateFiles();
}

auto FileSink::makeBasePaths() -> std::pair<std::filesystem::path, std::filesystem::path> {
    std::string fileName = createFileName();

    return {dataFolderPath / ("data_" + fileName), dataFolderPath / ("log_" + fileName)};
}

auto FileSink::resolveUniquePaths(std::filesystem::path baseData, std::filesystem::path baseLog)
    -> std::pair<std::filesystem::path, std::filesystem::path> {
    namespace fs = std::filesystem;

    auto data = baseData;
    auto log = baseLog;

    for (size_t i = 0; i < 1000; ++i) {
        if (!fs::exists(data) && !fs::exists(log))
            return {data, log};

        data = baseData;
        log = baseLog;

        data += "." + std::to_string(i);
        log += "." + std::to_string(i);
    }

    return {data, log}; // fallback
}

auto FileSink::openFilesAtPaths(bool writeHeader) -> bool {
    closeFiles();

    dataFile.open(currentWorkingFilePath, std::ios::app);
    logFile.open(currentWorkingLogPath, std::ios::app);

    if (!dataFile.is_open() || !logFile.is_open())
        return false;

    if (writeHeader) {
        dataFile << "#unix_timestamp_rising(s) unix_timestamp_trailing(s) "
                    "time_accuracy(ns) valid timebase(0=gps,2=utc) utc_available\n";

        logFile << "#time<YYYY-MM-DD_hh-mm-ss> parname value unit\n";
    }
    publishInfoUnlocked();
    return true;
}

auto FileSink::openFiles(bool writeHeader) -> bool {
    std::scoped_lock lock{m_mutex};
    readConfigFile();

    auto [baseData, baseLog] = makeBasePaths();
    auto [data, log] = resolveUniquePaths(baseData, baseLog);

    currentWorkingFilePath = data;
    currentWorkingLogPath = log;

    writeConfigFile();

    return openFilesAtPaths(writeHeader);
}

auto FileSink::rotateFiles() -> bool {
    closeFiles();
    readConfigFile();
    removeOldFiles();

    auto [baseData, baseLog] = makeBasePaths();
    auto [data, log] = resolveUniquePaths(baseData, baseLog);

    currentWorkingFilePath = data;
    currentWorkingLogPath = log;

    lastRotationDateTime = std::chrono::system_clock::now();
    nextRotationTime = generateNextDailyTime(dailyUploadTime);

    writeConfigFile();

    return openFilesAtPaths(true);
}

void FileSink::closeFiles() {
    dataFile.close();
    logFile.close();
}

auto FileSink::writeConfigFile() -> bool {
    std::filesystem::create_directories(configFilePath.parent_path());

    std::ofstream configFile(configFilePath, std::ios::trunc);
    if (!configFile) {
        logWarn(std::string("file open failed at location ") + configFilePath.string());
        return false;
    }

    configFile << currentWorkingFilePath << '\n' << currentWorkingLogPath << '\n';

    return true;
}

auto FileSink::removeOldFiles() -> bool {
    namespace fs = std::filesystem;

    bool ok = true;

    const auto now = std::chrono::system_clock::now();

    for (const auto& fileName : m_filename_list) {
        fs::path filePath = dataFolderPath / fileName;

        if (filePath == currentWorkingFilePath || filePath == currentWorkingLogPath) {
            continue;
        }

        std::error_code ec;
        auto ftime = fs::last_write_time(filePath, ec);

        if (ec) {
            continue; // skip unreadable file
        }

        auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - sctp).count();

        if (age > m_logrotate_period.count()) {
            logDebug("removing file " + filePath.string() + " (age = " + std::to_string(age) +
                     "s)");

            ok = ok && fs::remove(filePath, ec);

            if (ec)
                ok = false;
        }
    }

    return ok;
}

auto FileSink::readConfigFile() -> bool {
    namespace fs = std::filesystem;

    // 1. collect .dat files
    m_filename_list.clear();

    for (const auto& entry : fs::directory_iterator(dataFolderPath)) {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension() == ".dat") {
            m_filename_list.push_back(entry.path().filename().string());
        }
    }

    // 2. open config file
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        logWarn("failed to open config file: " + configFilePath.string());
        return false;
    }

    // 3. empty file case
    if (configFile.peek() == std::ifstream::traits_type::eof()) {
        return true;
    }

    // 4. read paths
    std::string line;

    if (std::getline(configFile, line))
        currentWorkingFilePath = fs::path(line);

    if (std::getline(configFile, line))
        currentWorkingLogPath = fs::path(line);

    return true;
}

void FileSink::writeToDataFile(const std::string& data) {
    std::scoped_lock lock{m_mutex};
    if (dataFile.is_open() == false) {
        logWarn("Could not write to data file " + currentWorkingFilePath.string());
        return;
    }
    dataFile << data << "\n";
    publishInfoUnlocked();
}

void FileSink::writeToLogFile(const std::string& log) {
    std::scoped_lock lock{m_mutex};
    if (logFile.is_open() == false) {
        logWarn("Could not write to log file" + currentWorkingFilePath.string());
        return;
    }
    logFile << log << "\n";
    publishInfoUnlocked();
}

auto FileSink::createFileName() -> std::string {
    // creates a fileName based on date time and mac address
    if (dataFolderPath == "") {
        logWarn("could not open data folder " + dataFolderPath.string());
        return "";
    }
    std::string fileName = dateStringNow();
    fileName = fileName + ".dat";
    return fileName;
}

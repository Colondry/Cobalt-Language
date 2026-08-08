#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <filesystem>
#include <cstdlib>

#include "libmanage.hpp"

namespace fs = std::filesystem;

static const std::string kPackageRepo = "Colondry/Cobalt-Package";
// ---------- small string helpers (platform-independent) ----------

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}


Requirements parseRequirementsText(const std::string& text) {
    Requirements req;
    std::istringstream stream(text);
    std::string line;
    std::string currentSection;

    while (std::getline(stream, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty()) continue;

        if (trimmed == ">end") { currentSection.clear(); continue; }

        {
            std::string candidate = trimmed;
            if (!candidate.empty() && candidate.front() == '>') candidate = candidate.substr(1);
            if (!candidate.empty() && candidate.back() == ':') {
                std::string name = trim(candidate.substr(0, candidate.size() - 1));
                bool isBareWord = !name.empty();
                for (char c : name) {
                    if (!(isalnum((unsigned char)c) || c == '_' || c == '-')) { isBareWord = false; break; }
                }
                if (isBareWord) {
                    currentSection = name;
                    continue;
                }
            }
        }

        if (currentSection == "lib") {
            std::stringstream ls(trimmed);
            std::string item;
            while (std::getline(ls, item, ',')) {
                std::string name = trim(item);
                if (!name.empty()) req.libs.push_back(name);
            }
        }
        else if (currentSection == "rfile") {
            size_t open = trimmed.find('[');
            size_t close = trimmed.find(']');
            RFile rf;
            if (open != std::string::npos && close != std::string::npos && close > open) {
                rf.name = trim(trimmed.substr(0, open));
                rf.dest = trim(trimmed.substr(open + 1, close - open - 1));
            }
            else {
                rf.name = trimmed;
                rf.dest = "./lib/";
            }
            if (!rf.name.empty()) req.rfiles.push_back(rf);
        }
    }
    return req;
}

static bool readFile(const std::string& path, std::string& outContent) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    outContent = ss.str();
    return true;
}

Requirements parseRequirementsFile(const std::string& path) {
    std::string content;
    if (!readFile(path, content)) return Requirements{};
    return parseRequirementsText(content);
}

#ifdef _WIN32

#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

// UTF-8 -> UTF-16
static std::wstring toWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
    if (size <= 0) return L"";
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), result.data(), size);
    return result;
}

static bool crackUrl(const std::wstring& url, bool& isHttps, std::wstring& host, INTERNET_PORT& port, std::wstring& path) {
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);

    wchar_t hostBuffer[512]{};
    wchar_t pathBuffer[8192]{};
    uc.lpszHostName = hostBuffer;
    uc.dwHostNameLength = ARRAYSIZE(hostBuffer);
    uc.lpszUrlPath = pathBuffer;
    uc.dwUrlPathLength = ARRAYSIZE(pathBuffer);

    if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.length()), 0, &uc)) return false;

    isHttps = uc.nScheme == INTERNET_SCHEME_HTTPS;
    host = hostBuffer;
    port = uc.nPort;
    path = pathBuffer;
    if (uc.lpszExtraInfo && uc.dwExtraInfoLength > 0) path.append(uc.lpszExtraInfo, uc.dwExtraInfoLength);
    return true;
}

// ---------- generic HTTP GET (used for both raw file bytes and JSON listings) ----------

static bool httpGet(const std::string& url, const wchar_t* acceptHeader, std::string& outBody, long& outStatus) {
    outStatus = 0;
    std::wstring wUrl = toWide(url);
    if (wUrl.empty()) { std::cerr << "Error: invalid URL.\n"; return false; }

    bool isHttps = false;
    std::wstring host, path;
    INTERNET_PORT port = 0;
    if (!crackUrl(wUrl, isHttps, host, port, path)) {
        std::cerr << "Error: could not parse URL: " << url << "\n";
        return false;
    }

    HINTERNET session = WinHttpOpen(L"Cobalt-Package-Manager/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) { std::cerr << "Error: WinHttpOpen failed: " << GetLastError() << "\n"; return false; }

    HINTERNET connection = WinHttpConnect(session, host.c_str(), port, 0);
    if (!connection) {
        std::cerr << "Error: WinHttpConnect failed: " << GetLastError() << "\n";
        WinHttpCloseHandle(session);
        return false;
    }

    DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connection, L"GET", path.c_str(), nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        std::cerr << "Error: WinHttpOpenRequest failed: " << GetLastError() << "\n";
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    std::wstring headers = L"User-Agent: Cobalt-Package-Manager/1.0\r\n";
    if (acceptHeader) headers += std::wstring(L"Accept: ") + acceptHeader + L"\r\n";
    WinHttpAddRequestHeaders(request, headers.c_str(), static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD);

    bool ok = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (ok) ok = WinHttpReceiveResponse(request, nullptr);

    if (ok) {
        DWORD statusCode = 0, statusSize = sizeof(statusCode);
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
        outStatus = static_cast<long>(statusCode);

        DWORD available = 0;
        while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
            std::vector<char> buffer(available);
            DWORD downloaded = 0;
            if (!WinHttpReadData(request, buffer.data(), available, &downloaded)) {
                std::cerr << "Error: WinHttpReadData failed: " << GetLastError() << "\n";
                ok = false;
                break;
            }
            outBody.append(buffer.data(), downloaded);
        }
    }
    else {
        std::cerr << "Error: HTTP request failed: " << GetLastError() << "\n";
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return ok;
}

bool downloadFile(const std::string& url, const std::string& outputPath) {
    std::string body;
    long status = 0;
    if (!httpGet(url, L"*/*", body, status)) return false;
    if (status != 200) {
        std::cerr << "Error: HTTP " << status << " while downloading " << url << "\n";
        return false;
    }

    fs::path output(outputPath);
    if (output.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(output.parent_path(), ec);
        if (ec) {
            std::cerr << "Error: could not create directory: " << output.parent_path() << "\n";
            return false;
        }
    }

    std::ofstream file(outputPath, std::ios::binary);
    if (!file) { std::cerr << "Error: could not open: " << outputPath << "\n"; return false; }
    file.write(body.data(), static_cast<std::streamsize>(body.size()));
    return true;
}

// ---------- GitHub contents API ----------

struct GitHubEntry {
    std::string name;
    std::string path;
    std::string type; // "file" or "dir"
    std::string downloadUrl;
};

static std::vector<std::string> splitJsonObjects(const std::string& json) {
    std::vector<std::string> objs;
    int depth = 0;
    size_t start = 0;
    bool inStr = false;
    for (size_t i = 0; i < json.size(); i++) {
        char c = json[i];
        if (inStr) {
            if (c == '\\') { i++; continue; }
            if (c == '"') inStr = false;
            continue;
        }
        if (c == '"') { inStr = true; continue; }
        if (c == '{') { if (depth == 0) start = i; depth++; }
        else if (c == '}') { depth--; if (depth == 0) objs.push_back(json.substr(start, i - start + 1)); }
    }
    return objs;
}

static std::string jsonField(const std::string& obj, const std::string& key) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = obj.find(pattern);
    if (pos == std::string::npos) return "";
    pos = obj.find(':', pos + pattern.size());
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < obj.size() && isspace((unsigned char)obj[pos])) pos++;
    if (pos < obj.size() && obj[pos] == '"') {
        pos++;
        std::string val;
        while (pos < obj.size() && obj[pos] != '"') {
            if (obj[pos] == '\\' && pos + 1 < obj.size()) { val += obj[pos + 1]; pos += 2; continue; }
            val += obj[pos];
            pos++;
        }
        return val;
    }
    return ""; // null, or a non-string field we don't read
}

static GitHubEntry entryFromJson(const std::string& obj) {
    GitHubEntry e;
    e.name = jsonField(obj, "name");
    e.path = jsonField(obj, "path");
    e.type = jsonField(obj, "type");
    e.downloadUrl = jsonField(obj, "download_url");
    return e;
}

static std::vector<GitHubEntry> getGitHubEntries(const std::string& repo, const std::string& path) {
    std::vector<GitHubEntry> entries;
    std::string apiUrl = "https://api.github.com/repos/" + repo + "/contents/" + path;

    std::string body;
    long status = 0;
    if (!httpGet(apiUrl, L"application/vnd.github+json", body, status)) return entries;
    if (status != 200) {
        std::cerr << "GitHub API returned HTTP " << status << " for " << path << "\n";
        return entries;
    }

    size_t firstNonSpace = body.find_first_not_of(" \t\r\n");
    if (firstNonSpace != std::string::npos && body[firstNonSpace] == '[') {
        for (const std::string& obj : splitJsonObjects(body)) {
            GitHubEntry e = entryFromJson(obj);
            if (!e.name.empty() && !e.type.empty()) entries.push_back(e);
        }
    }
    else {
        GitHubEntry e = entryFromJson(body);
        if (!e.name.empty() && !e.type.empty()) entries.push_back(e);
    }
    return entries;
}

static bool downloadGitHubDirectory(const std::string& repo, const std::string& remotePath, const fs::path& localPath) {
    std::vector<GitHubEntry> entries = getGitHubEntries(repo, remotePath);
    if (entries.empty()) {
        std::cerr << "Error: directory is empty or could not be accessed: " << remotePath << "\n";
        return false;
    }

    std::error_code ec;
    fs::create_directories(localPath, ec);
    if (ec) { std::cerr << "Error: could not create directory: " << localPath << "\n"; return false; }

    for (const GitHubEntry& entry : entries) {
        fs::path destination = localPath / entry.name;

        if (entry.type == "file") {
            std::cout << "Downloading " << entry.path << "...\n";
            if (entry.downloadUrl.empty()) {
                std::cerr << "Error: no download URL for " << entry.path << "\n";
                return false;
            }
            if (!downloadFile(entry.downloadUrl, destination.string())) {
                std::cerr << "Error: failed to download " << entry.path << "\n";
                return false;
            }
        }
        else if (entry.type == "dir") {
            std::cout << "Entering " << entry.path << "...\n";
            if (!downloadGitHubDirectory(repo, entry.path, destination)) return false;
        }
    }
    return true;
}

bool downloadGitHubLibrary(const std::string& repo, const std::string& remotePath, const std::string& localPath) {
    return downloadGitHubDirectory(repo, remotePath, fs::path(localPath));
}

// ---------- 7-Zip archive extraction ----------

#ifdef _WIN32

#include <process.h>
#include <filesystem>

bool extract(const std::string& archiveFileName,
    const std::string& outputDir) {

    namespace fs = std::filesystem;

    // Find the directory containing cobalt.exe
    fs::path cobaltDir = fs::current_path();

    fs::path sevenZip = cobaltDir / "lib" / "7-Zip" / "7z.exe";
    fs::path archive = fs::path(archiveFileName);
    fs::path output = fs::path(outputDir);

    // Convert everything to absolute paths.
    sevenZip = fs::absolute(sevenZip);
    archive = fs::absolute(archive);
    output = fs::absolute(output);

    if (!fs::exists(sevenZip)) {
        std::cerr << "Error: 7-Zip not found:\n"
            << sevenZip.string() << "\n";
        return false;
    }

    if (!fs::exists(archive)) {
        std::cerr << "Error: archive not found:\n"
            << archive.string() << "\n";
        return false;
    }

    // Create output directory if necessary.
    std::error_code ec;
    fs::create_directories(output, ec);

    if (ec) {
        std::cerr << "Error: could not create output directory:\n"
            << output.string() << "\n";
        return false;
    }


    // Arguments passed directly to 7z.exe.
    // No cmd.exe. No shell. No quoting problems.
    std::string arg1 = "7z.exe";
    std::string arg2 = "x";
    std::string arg3 = archive.string();
    std::string arg4 = "-o" + output.string();
    std::string arg5 = "-y";
    std::string arg6 = "-bso0 -bsp0 -bse0 > nul";

    const char* argv[] = {
        arg1.c_str(),
        arg2.c_str(),
        arg3.c_str(),
        arg4.c_str(),
        arg5.c_str(),
        arg6.c_str(),
        nullptr
    };

    int result = _spawnv(
        _P_WAIT,
        sevenZip.string().c_str(),
        argv
    );
   

    if (result == 0) {
        std::cout << "Archive extracted successfully.\n";
        std::cout << "Clearing Cache....\n"; std::system("del download.zip");
        return true;
    }

    if (result == -1) {
        std::cerr << "Error: failed to launch 7-Zip.\n";
        std::cerr << "Path: " << sevenZip.string() << "\n";
        std::cout << "Clearing Cache....\n"; std::system("del download.zip");
        return false;
    }

    std::cerr << "7-Zip failed with exit code: "
        << result << "\n";
    std::cout << "Clearing Cache....\n"; std::system("del download.zip");

    return false;
}

#else

bool extract(const std::string&, const std::string&) {
    std::cerr << "Error: archive extraction is currently "
        "only supported on Windows.\n";
    return false;
}

#endif

// ---------- recursive package installer ----------

static bool installPackageImpl(const std::string& libName, std::set<std::string>& installed) {
    if (fs::exists("./lib/" + libName + "/") && fs::is_directory("./lib/" + libName + "/")) {
        std::cout << libName + " library is already exist.\n"; return true;
    }
    if (!installed.insert(libName).second) {
        return true; // already installed (or in progress) this run -- diamond/circular dep, nothing more to do
    }

    std::string localDir = "./lib/" + libName + "/";
    if (!downloadGitHubLibrary(kPackageRepo, libName, localDir)) {
        return false;
    }

    std::string reqPath = localDir + "requirement.txt";
    if (!fs::exists(reqPath)) {
        return true; // no further dependencies
    }

    Requirements req = parseRequirementsFile(reqPath);
    bool ok = true;

    for (const std::string& dep : req.libs) {
        std::cout << libName << " requires " << dep << ", installing...\n";
        if (!installPackageImpl(dep, installed)) {
            std::cerr << "Error: failed to install dependency \"" << dep << "\" (required by \"" << libName << "\").\n";
            ok = false;
        }
    }

    for (const RFile& rf : req.rfiles) {
        std::cout << libName << " requires " << rf.name << ", downloading...\n";

        std::string archiveUrl;
        std::string archiveFile;

        if (rf.name.rfind("http://", 0) == 0 || rf.name.rfind("https://", 0) == 0) {
            // A literal URL (may be hosted anywhere, any archive extension --
            // 7-Zip handles .zip and friends as well as .7z).
            archiveUrl = rf.name;
            size_t q = archiveUrl.find('?');
            std::string clean = (q == std::string::npos) ? archiveUrl : archiveUrl.substr(0, q);
            size_t slash = clean.find_last_of('/');
            archiveFile = (slash == std::string::npos) ? clean : clean.substr(slash + 1);
            if (archiveFile.empty()) archiveFile = "download.zip";
        }
        else {
            // A bare name -- resolve via rfiles/<name>.7z in the Cobalt-Package repo.
            std::vector<GitHubEntry> entries = getGitHubEntries(kPackageRepo, "rfiles/" + rf.name + ".zip");
            if (!entries.empty() && !entries[0].downloadUrl.empty()) archiveUrl = entries[0].downloadUrl;
            archiveFile = rf.name + ".zip";
        }

        if (archiveUrl.empty() || !downloadFile(archiveUrl, archiveFile)) {
            std::cerr << "Error: failed to download rfile \"" << rf.name << "\".\n";
            ok = false;
            continue;
        }
        if (!extract(archiveFile, rf.dest)) {
            ok = false;
        }
    }

    return ok;
}

bool installPackage(const std::string& libName) {
    std::set<std::string> installed;
    return installPackageImpl(libName, installed);
}

#else // !_WIN32

bool downloadFile(const std::string&, const std::string&) {
    std::cerr << "Error: Cobalt package installation currently requires Windows.\n";
    return false;
}
bool downloadGitHubLibrary(const std::string&, const std::string&, const std::string&) {
    std::cerr << "Error: Cobalt package installation currently requires Windows.\n";
    return false;
}
bool extract(const std::string&, const std::string&) {
    std::cerr << "Error: Cobalt package installation currently requires Windows.\n";
    return false;
}
bool installPackage(const std::string&) {
    std::cerr << "Error: Cobalt package installation currently requires Windows.\n";
    return false;
}

#endif
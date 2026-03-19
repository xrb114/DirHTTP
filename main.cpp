#define _WIN32_WINNT 0x0A00
#define WINVER       0x0A00
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <shellapi.h>

#include <iostream>
#include <string>
#include <filesystem>
#include <algorithm>
#include <map>
#include <vector>
#include <thread>

#include "lib/httplib.h"

namespace fs = std::filesystem;

std::vector<std::string> getLocalIPs() {
    std::vector<std::string> ips;
    ULONG bufLen = 15000;
    std::vector<BYTE> buf(bufLen);
    PIP_ADAPTER_ADDRESSES addrs = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.data());

    ULONG ret = GetAdaptersAddresses(
        AF_INET,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
        nullptr, addrs, &bufLen);
    if (ret == ERROR_BUFFER_OVERFLOW) {
        buf.resize(bufLen);
        addrs = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.data());
        ret = GetAdaptersAddresses(
            AF_INET,
            GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
            nullptr, addrs, &bufLen);
    }
    if (ret != NO_ERROR) return ips;

    for (auto* a = addrs; a; a = a->Next) {
        if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        if (a->OperStatus != IfOperStatusUp) continue;
        for (auto* ua = a->FirstUnicastAddress; ua; ua = ua->Next) {
            auto* sa = reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sa->sin_addr, ipStr, sizeof(ipStr));
            ips.emplace_back(ipStr);
        }
    }
    return ips;
}

std::string getMime(const std::string& ext) {
    static const std::map<std::string, std::string> mime = {
        {".html", "text/html"}, {".htm", "text/html"},
        {".xml",  "text/xml"}, {".xhtml","application/xhtml+xml"},
        {".css",  "text/css"}, {".js",  "application/javascript"},
        {".mjs",  "application/javascript"}, {".json","application/json"},
        {".txt",  "text/plain"}, {".md",  "text/plain"},
        {".csv",  "text/plain"}, {".log", "text/plain"},
        {".yaml", "text/plain"}, {".yml", "text/plain"},
        {".toml", "text/plain"}, {".ini", "text/plain"},
        {".conf", "text/plain"}, {".sh",  "text/plain"},
        {".bat",  "text/plain"}, {".py",  "text/plain"},
        {".cpp",  "text/plain"}, {".c",   "text/plain"},
        {".h",    "text/plain"}, {".java","text/plain"},
        {".rs",   "text/plain"}, {".go",  "text/plain"},
        {".ts",   "text/plain"},
        {".png",  "image/png"}, {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"}, {".gif", "image/gif"},
        {".svg",  "image/svg+xml"}, {".ico","image/x-icon"},
        {".webp", "image/webp"}, {".bmp", "image/bmp"},
        {".tiff", "image/tiff"}, {".avif","image/avif"},
        {".mp4",  "video/mp4"}, {".webm","video/webm"},
        {".ogg",  "video/ogg"}, {".mp3", "audio/mpeg"},
        {".wav",  "audio/wav"}, {".flac","audio/flac"},
        {".aac",  "audio/aac"}, {".pdf", "application/pdf"},
        {".wasm", "application/wasm"},
    };
    auto it = mime.find(ext);
    return it != mime.end() ? it->second : "application/octet-stream";
}

std::string urlDecode(const std::string& s) {
    std::string result;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int v = std::stoi(s.substr(i + 1, 2), nullptr, 16);
            result += (char)v;
            i += 2;
        } else if (s[i] == '+') {
            result += ' ';
        } else {
            result += s[i];
        }
    }
    return result;
}

std::string urlEncode(const std::string& s) {
    std::string result;
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += c;
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", c);
            result += buf;
        }
    }
    return result;
}

std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring ws(len - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, ws.data(), len);
    return ws;
}

std::string wideToUtf8(const wchar_t* ws) {
    int len = WideCharToMultiByte(CP_UTF8, 0, ws, -1, nullptr, 0, nullptr, nullptr);
    std::string s(len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws, -1, s.data(), len, nullptr, nullptr);
    return s;
}

bool installMenu() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string exe = exePath;

    auto write = [](const std::string& path, const std::string& name, const std::wstring& val) {
        HKEY hKey;
        if (RegCreateKeyExA(HKEY_CLASSES_ROOT, path.c_str(), 0, nullptr,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
            return false;
        bool ok = RegSetValueExW(hKey,
            name.empty() ? nullptr : std::wstring(name.begin(), name.end()).c_str(),
            0, REG_SZ,
            (BYTE*)val.c_str(),
            (DWORD)((val.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
        RegCloseKey(hKey);
        return ok;
    };

    std::wstring wexe(exe.begin(), exe.end());
    std::wstring cmd   = L"\"" + wexe + L"\" \"%1\"";
    std::wstring cmdBg = L"\"" + wexe + L"\" \"%V\"";
    std::wstring icon  = wexe + L",0";

    bool ok = true;
    ok &= write("Directory\\shell\\DirHTTP",          "",       L"开启 HTTP 服务");
    ok &= write("Directory\\shell\\DirHTTP",          "Icon",   icon);
    ok &= write("Directory\\shell\\DirHTTP\\command", "",       cmd);
    ok &= write("Directory\\Background\\shell\\DirHTTP",          "",       L"开启 HTTP 服务");
    ok &= write("Directory\\Background\\shell\\DirHTTP",          "Icon",   icon);
    ok &= write("Directory\\Background\\shell\\DirHTTP\\command", "",       cmdBg);
    return ok;
}

bool uninstallMenu() {
    bool ok = true;
    ok &= (RegDeleteTreeA(HKEY_CLASSES_ROOT, "Directory\\shell\\DirHTTP") == ERROR_SUCCESS);
    ok &= (RegDeleteTreeA(HKEY_CLASSES_ROOT, "Directory\\Background\\shell\\DirHTTP") == ERROR_SUCCESS);
    return ok;
}

int main() {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    int wargc;
    wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);

    auto arg = [&](int i) -> std::string {
        if (i >= wargc) return "";
        return wideToUtf8(wargv[i]);
    };

    if (wargc == 2 && arg(1) == "--install") {
        if (installMenu()) std::cout << "右键菜单已安装！\n";
        else { std::cerr << "安装失败，请以管理员身份运行\n"; LocalFree(wargv); return 1; }
        system("pause >nul");
        LocalFree(wargv);
        return 0;
    }

    if (wargc == 2 && arg(1) == "--uninstall") {
        if (uninstallMenu()) std::cout << "右键菜单已卸载！\n";
        else { std::cerr << "卸载失败，请以管理员身份运行\n"; LocalFree(wargv); return 1; }
        system("pause >nul");
        LocalFree(wargv);
        return 0;
    }

    if (wargc != 2) {
        std::cerr << "Usage:\n";
        std::cerr << "  DirHTTP.exe <directory>    # 启动服务\n";
        std::cerr << "  DirHTTP.exe --install      # 注册右键菜单（需管理员）\n";
        std::cerr << "  DirHTTP.exe --uninstall    # 卸载右键菜单（需管理员）\n";
        LocalFree(wargv);
        return 1;
    }

    fs::path rootDir = fs::absolute(fs::path(wargv[1]));
    LocalFree(wargv);

    if (!fs::exists(rootDir) || !fs::is_directory(rootDir)) {
        std::cerr << "Error: Not a directory\n";
        return 1;
    }

    httplib::Server svr;

    svr.Get("/.*", [&](const httplib::Request& req, httplib::Response& res) {
        std::string urlPath = urlDecode(req.path);

        fs::path target = rootDir;
        std::string seg;
        for (size_t i = 1; i < urlPath.size(); ++i) {
            if (urlPath[i] == '/') {
                if (seg == "..") { target = target.parent_path(); seg.clear(); }
                else if (!seg.empty() && seg != ".") { target /= utf8ToWide(seg); seg.clear(); }
                else seg.clear();
            } else seg += urlPath[i];
        }
        if (seg == "..") target = target.parent_path();
        else if (!seg.empty() && seg != ".") target /= utf8ToWide(seg);
        target = target.lexically_normal();

        if (fs::absolute(target).wstring().find(fs::absolute(rootDir).wstring()) != 0) {
            res.status = 403;
            res.set_content("403 Forbidden", "text/plain");
            return;
        }

        std::error_code ec;
        if (!fs::exists(target, ec)) {
            res.status = 404;
            res.set_content("404 Not Found: " + urlPath, "text/plain; charset=utf-8");
            return;
        }

        if (fs::is_directory(target, ec)) {
            std::string html = "<!DOCTYPE html><html><head>"
                "<meta charset='utf-8'>"
                "<style>"
                "body{font-family:system-ui,sans-serif;padding:24px;max-width:900px;margin:0 auto;}"
                "h2{border-bottom:1px solid #ccc;padding-bottom:8px;word-break:break-all;}"
                "a{display:flex;align-items:center;gap:8px;padding:6px 4px;"
                "text-decoration:none;color:#333;border-radius:4px;}"
                "a:hover{background:#f0f0f0;}"
                ".size{margin-left:auto;color:#999;font-size:0.85em;white-space:nowrap;}"
                "</style>"
                "<title>" + urlPath + "</title></head><body>"
                "<h2>📂 " + urlPath + "</h2>";

            if (urlPath != "/") html += "<a href='../'>⬆️ &nbsp;..</a>";

            std::wstring searchPath = target.wstring() + L"\\*";
            WIN32_FIND_DATAW fd;
            HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
            std::vector<std::pair<std::string, bool>> entries;
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    std::wstring wname = fd.cFileName;
                    if (wname == L"." || wname == L"..") continue;
                    bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                    entries.push_back({wideToUtf8(wname.c_str()), isDir});
                } while (FindNextFileW(hFind, &fd));
                FindClose(hFind);
            }

            for (auto& [name, isDir] : entries) {
                if (!isDir) continue;
                html += "<a href='" + urlEncode(name) + "/'>📁 " + name + "</a>";
            }
            for (auto& [name, isDir] : entries) {
                if (isDir) continue;
                auto sz = fs::file_size(target / utf8ToWide(name), ec);
                std::string sizeStr = ec ? "?" :
                    sz < 1024 ? std::to_string(sz) + " B" :
                    sz < 1024*1024 ? std::to_string(sz/1024) + " KB" :
                    std::to_string(sz/1024/1024) + " MB";
                html += "<a href='" + urlEncode(name) + "'>📄 " + name +
                        "<span class='size'>" + sizeStr + "</span></a>";
            }
            html += "</body></html>";
            res.set_content(html, "text/html; charset=utf-8");
            return;
        }

        HANDLE hFile = CreateFileW(target.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ,
                                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            res.status = 500;
            res.set_content("500 Cannot open file", "text/plain");
            return;
        }
        LARGE_INTEGER fileSize;
        GetFileSizeEx(hFile, &fileSize);
        std::string content(fileSize.QuadPart, 0);
        DWORD bytesRead;
        ReadFile(hFile, content.data(), (DWORD)fileSize.QuadPart, &bytesRead, nullptr);
        CloseHandle(hFile);

        std::string ext = target.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        res.set_content(content, getMime(ext));
    });

    int port = svr.bind_to_any_port("0.0.0.0");
    if (port == -1) {
        std::cerr << "Error: No available port\n";
        return 1;
    }

    std::string portStr = std::to_string(port);
    std::string firstUrl = "http://localhost:" + portStr;
    ShellExecuteW(nullptr, L"open", utf8ToWide(firstUrl).c_str(), nullptr, nullptr, SW_SHOWNORMAL);

    std::cout << "Serving " << rootDir << "\n";
    std::cout << "Listening on port " << port << "\n\n";

    auto ips = getLocalIPs();
    if (ips.empty()) {
        std::cout << "  http://localhost:" << portStr << "\n";
    } else {
        for (const auto& ip : ips) {
            std::cout << "  http://" << ip << ":" << portStr << "\n";
        }
    }

    std::cout << "\nPress Enter to stop...\n";

    std::thread server_thread([&svr]() {
        svr.listen_after_bind();
    });

    std::cin.get();
    svr.stop();
    if (server_thread.joinable()) server_thread.join();

    return 0;
}
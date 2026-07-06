#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <pwd.h>
#include <vector>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

// 获取当前用户家目录
std::string getHomeDir()
{
    struct passwd* pw = getpwuid(getuid());
    if (!pw) return "/root";
    return std::string(pw->pw_dir);
}

// 拼接路径
std::string joinPath(const std::string& a, const std::string& b)
{
    if (a.back() == '/')
        return a + b;
    return a + "/" + b;
}

// 读取kkk.conf，查询软件对应的git地址
bool findGitUrl(const std::string& confPath, const std::string& pkgName, std::string& outUrl)
{
    std::ifstream fin(confPath);
    if (!fin.is_open())
    {
        std::cerr << "[错误] 无法打开配置文件: " << confPath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(fin, line))
    {
        // 跳过空行
        if (line.empty()) continue;
        // 分割 "软件名 = git地址"
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string name = line.substr(0, eqPos);
        std::string url = line.substr(eqPos + 1);

        // 去除首尾空格
        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t"));
            s.erase(s.find_last_not_of(" \t") + 1);
        };
        trim(name);
        trim(url);

        if (name == pkgName)
        {
            outUrl = url;
            return true;
        }
    }
    fin.close();
    std::cerr << "[错误] 配置文件中未找到软件: " << pkgName << std::endl;
    return false;
}

// 安装逻辑 -i pkg
int installPkg(const std::string& pkgName)
{
    std::string home = getHomeDir();
    std::string confDir = joinPath(home, ".config/kkk");
    std::string confFile = joinPath(confDir, "kkk.conf");
    std::string removeDir = joinPath(confDir, "kkkremove");

    // 创建目录
    fs::create_directories(confDir);
    fs::create_directories(removeDir);

    // 查找git地址
    std::string gitUrl;
    if (!findGitUrl(confFile, pkgName, gitUrl))
        return 1;

    std::cout << "[信息] 找到仓库地址: " << gitUrl << std::endl;

    // 1. git clone 到临时目录 ./tmp_kkk_pkg
    std::string tmpDir = "./tmp_kkk_pkg";
    fs::remove_all(tmpDir);
    std::string cloneCmd = "git clone -c http.sslVerify=false " + gitUrl + " " + tmpDir;
    std::cout << "[执行] " << cloneCmd << std::endl;
    if (std::system(cloneCmd.c_str()) != 0)
    {
        std::cerr << "[错误] git克隆失败" << std::endl;
        return 1;
    }

    // 2. 执行 install.sh
std::string installSh = joinPath(tmpDir, "install.sh");
if (!fs::exists(installSh))
{
    std::cerr << "[错误] 仓库内无 install.sh" << std::endl;
    fs::remove_all(tmpDir);
    return 1;
}
std::string runInstall = "chmod +x " + installSh + " && bash " + installSh;
std::cout << "\n===== 开始执行 install.sh 输出 =====\n" << std::endl;
int execRet = std::system(runInstall.c_str());
std::cout << "\n===== install.sh 执行完毕 =====\n" << std::endl;
if (execRet != 0)
{
    std::cerr << "[警告] install.sh 返回非0退出码，安装脚本执行异常" << std::endl;
}


    // 3. 复制 remove.sh 到 ~/.config/kkk/kkkremove/[pkgName].sh
    std::string srcRemove = joinPath(tmpDir, "remove.sh");
    std::string destRemove = joinPath(removeDir, pkgName + ".sh");
    if (fs::exists(srcRemove))
    {
        fs::copy_file(srcRemove, destRemove, fs::copy_options::overwrite_existing);
        std::cout << "[信息] 已保存卸载脚本: " << destRemove << std::endl;
    }
    else
    {
        std::cerr << "[警告] 仓库内无 remove.sh，卸载功能将失效" << std::endl;
    }

    // 清理临时目录
    fs::remove_all(tmpDir);
    std::cout << "[完成] " << pkgName << " 安装流程结束" << std::endl;
    return 0;
}

// 卸载逻辑 -r pkg
int removePkg(const std::string& pkgName)
{
    std::string home = getHomeDir();
    std::string removeDir = joinPath(home, ".config/kkk/kkkremove");
    std::string scriptPath = joinPath(removeDir, pkgName + ".sh");

    if (!fs::exists(scriptPath))
    {
        std::cerr << "[错误] 不存在该软件的卸载脚本: " << scriptPath << std::endl;
        return 1;
    }

    // 执行卸载脚本
    std::string runCmd = "chmod +x " + scriptPath + " && bash " + scriptPath;
    std::cout << "[执行卸载脚本] " << runCmd << std::endl;
    int ret = std::system(runCmd.c_str());

    // 可选：卸载后删除卸载脚本
    fs::remove(scriptPath);
    std::cout << "[信息] 已删除卸载脚本记录" << std::endl;

    if (ret != 0)
    {
        std::cerr << "[警告] 卸载脚本执行异常" << std::endl;
        return 1;
    }
    std::cout << "[完成] " << pkgName << " 卸载完成" << std::endl;
    return 0;
}

void printHelp()
{
    std::cout << "KKK 简易包管理器\n"
              << "用法:\n"
              << "  ./kkkpkg -i <软件名>    安装软件\n"
              << "  ./kkkpkg -r <软件名>    卸载软件\n"
              << "配置文件: ~/.config/kkk/kkk.conf\n"
              << "卸载脚本存放: ~/.config/kkk/kkkremove/\n";
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printHelp();
        return 1;
    }

    std::string op = argv[1];
    std::string pkg = argv[2];

    if (op == "-i")
    {
        return installPkg(pkg);
    }
    else if (op == "-r")
    {
        return removePkg(pkg);
    }
    else
    {
        std::cerr << "[错误] 无效参数，仅支持 -i / -r" << std::endl;
        printHelp();
        return 1;
    }
}


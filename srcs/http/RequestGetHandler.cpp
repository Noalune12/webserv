#include "Request.hpp"
#include "Logger.hpp"

#include <iostream>
#include <sys/stat.h>
#include <sstream>
#include <fstream>
#include <dirent.h>
#include <unistd.h>

void Request::methodGetHandler() {

    Logger::debug("Entering Get Handler");

    if (!_reqLocation->root.empty() && !_trailing.empty()) {
        _getPath = getPath(_reqLocation->root + _uri, _trailing);
    } else if (!_reqLocation->root.empty() && _trailing.empty()) {
        _getPath = getPath(_reqLocation->root + _uri);
    } else if (!_reqLocation->alias.empty() && !_trailing.empty()) {
        _getPath = getPath(_reqLocation->alias, _trailing);
    } else if (!_reqLocation->alias.empty() && _trailing.empty()) {
        _getPath = getPath(_reqLocation->alias);
    }

    Logger::debug("Get: Path is " + _getPath);

    std::vector<std::string>::iterator itIndex = _reqLocation->index.begin();

    struct stat buf;
    if (stat(_getPath.c_str(), &buf) == 0) {
        if (_trailing.empty()) {

            // Case 1: no trailing and path is a folder -> need to get the index

            if (S_ISDIR(buf.st_mode)) {

                Logger::debug("Get: No trailing and is a directory");

                if (access(_getPath.c_str(), R_OK | X_OK) != 0) {
                    if (!_reqLocation->root.empty()) {
                        findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                    }
                    else {
                        findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
                    }
                    Logger::warn("Get: no right on folder");
                    return ;
                }

                for (; itIndex != _reqLocation->index.end() ; itIndex++) {
                    std::string path = getPath(_getPath, *itIndex);

                    struct stat st;

                    if (stat(path.c_str(), &st) == 0) {
                        if (readFile(path, st))
                            return ;
                    }
                }

                if (err == true && _indexFound == true)
                    return;
                else if (_reqLocation->autoIndex == true) {
                    if (!handleAutoindex(_getPath))
                        return ;
                    err = false;
                    status = 200;
                    return;
                } else {
                    if (!_reqLocation->root.empty()) {
                        findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                    }
                    else {
                        findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
                    }
                    Logger::warn("Get: no file found");
                    return ;
                }

            } else {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                }
                else {
                    findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
                }
                Logger::warn("Get: Path is not a folder (with trailing)");
                return ;
            }
        } else {

            // Case 2: trailing and path is a folder -> need to get the file mentionned in trailing

            if (S_ISDIR(buf.st_mode)) {

                Logger::debug("Get: Trailing and is a directory");

                if (access(_getPath.c_str(), R_OK | X_OK) != 0) {
                    if (!_reqLocation->root.empty()) {
                        findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                    }
                    else {
                        findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
                    }
                    Logger::warn("Get: no right on folder");
                    return ;
                }

                for (; itIndex != _reqLocation->index.end() ; itIndex++) {
                    std::string path = getPath(_getPath, *itIndex);

                    struct stat st;

                    if (stat(path.c_str(), &st) == 0) {
                        if (readFile(path, st))
                            return ;
                    }
                }

                if (err == true && _indexFound == true)
                    return;
                else if (_reqLocation->autoIndex == true) {
                    if (!handleAutoindex(_getPath))
                        return ;
                    err = false;
                    status = 200;
                    return;
                } else {
                    if (!_reqLocation->root.empty()) {
                        findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                    }
                    else {
                        findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
                    }
                    Logger::warn("Get: no file found");
                    return ;
                }

            // Case 3: trailing and path is a file -> need to get the file

            } else if (S_ISREG(buf.st_mode)) {

                Logger::debug("Get: Trailing and is a file");

                readFile(_getPath, buf);
                return ;

            }
        }

    } else if (_trailing.empty()) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
        }
        Logger::warn("Get: path non existing (no trailing)");
        return ;

    } else if (!_trailing.empty()) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(404, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(404, _reqLocation->alias, _reqLocation->errPage);
        }
        Logger::warn("Get: path non existing (trailing)");
        return ;
    }

}

std::string Request::getPath(const std::string& folder) {

    std::string path;

    if (folder[folder.size() - 1] != '/' && !_trailing.empty())
        path = folder + "/" + _trailing;
    else
        path = folder + _trailing;

    if (path[0] == '/')
        path = path.substr(1, path.size());

    return path;
}

std::string Request::getPath(const std::string& folder, std::string& file) {

    std::string path;

    if (folder[folder.size() - 1] == '/' && file[0] == '/')
        file = file.substr(1);
    if (folder[folder.size() - 1] == '/')
        path = folder + file;
    else
        path = folder + "/" + file;

    if (path[0] == '/')
        path = path.substr(1, path.size());

    return path;
}

bool Request::readFile(const std::string& path, struct stat buf) {


    if (!S_ISREG(buf.st_mode)) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
        }
        Logger::warn("Get: File " + path + "is not regular");
        return false;
    }

    if (access(path.c_str(), R_OK) != 0) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
        }
        Logger::warn("Get: no rights on file " + path);
        return false;
    }

    if (buf.st_mode & S_IRUSR) {
        std::ifstream file(path.c_str(), std::ios::binary);
        if (!file.is_open()) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(500, _reqLocation->root, _reqLocation->errPage);
            }
            else {
                findErrorPage(500, _reqLocation->alias, _reqLocation->errPage);
            }
            Logger::warn("Could not open file: " + path);
            return false;
	    }
        Logger::debug("Get: File found at " + path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        htmlPage = buffer.str();
        err = false;
        status = 200;
        return true;
    } else {
        if (!_reqLocation->root.empty()) {
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
        }
        Logger::warn("Get: no rights on file " + path);
        _indexFound = true;
        return false;
    }
}

bool Request::handleAutoindex(const std::string& dirPath) {

    Logger::debug("Handling Autodindex");

    if (access(dirPath.c_str(), R_OK | X_OK) != 0) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
        }
        Logger::warn("Get(autoindex): no rights on directory " + dirPath);
        return false;
    }


    std::string path = _reqLocation->path + _trailing;
    if (path[path.size() - 1] == '/')
        path = path.substr(0, path.size() - 1);
    htmlPage =
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "	 <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "    <title>Index of " + path + "</title>\n"
        "</head>\n"
        "<style>\n"
        "* {\n"
		"	margin: 0;\n"
		"	padding: 0;\n"
		"	box-sizing: border-box;\n"
		"}\n"
        "body {\n"
        "   font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, \"Liberation Mono\", monospace;\n"
        "   padding: 4rem 2rem;\n"
        "}\n"
		"h1 {\n"
		"	line-height: 1.6;\n"
		"	margin-bottom: 1.5rem;\n"
		"	color: #444;\n"
		"}\n"
        ".file-link {\n"
        "   font-size: 1rem;\n"
		"	line-height: 2;\n"
        "   color: #000000;\n"
        "   transition: transform 0.2s ease;"
        "}\n"
        ".file-link:visited {\n"
        "    color: #000000;\n"
        "}\n"
        "</style>\n"
        "<body>\n"
        "    <h1>Index of " + path + "</h1>\n";

    DIR* dir = opendir(dirPath.c_str());
    if (dir == NULL) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(500, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(500, _reqLocation->alias, _reqLocation->errPage);
        }
        Logger::warn("Get(autoindex): opendir failed");
        return false;
    }

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {

        std::string name = entry->d_name;

        if (name == "." || name == "..")
            continue;

        std::string fullPath;

        if (dirPath[dirPath.size() - 1] != '/')
            fullPath = dirPath + "/" + name;
        else
            fullPath = dirPath + name;

        struct stat st;

        if (stat(fullPath.c_str(), &st) == 0 ) {

            if (S_ISDIR(st.st_mode) && access(fullPath.c_str(), R_OK | X_OK) == 0) {

                Logger::debug("Get (autoindex): Folder " + fullPath);
                htmlPage += "<span class=\"icon\">📁</span> <a href=\"" + path + "/" + name + "/\" class=\"file-link\">" + name + "</a><br>\n";

            } else if (S_ISREG(st.st_mode) && access(fullPath.c_str(), R_OK) == 0) {

                Logger::debug("Get (autoindex): File " + fullPath);
                htmlPage += "<span class=\"icon\">📄</span> <a href=\"" + path + "/" + name + "\" class=\"file-link\"> " + name + "</a><br>\n";

            }

        }

    }

    htmlPage += "</body>\n"
        "</html>\n";

    closedir(dir);

    return true;
}

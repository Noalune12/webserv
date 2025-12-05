#include "Utils.hpp"

#include <sstream>


bool Utils::isOnlyWSpace(const std::string& line) {
	size_t count = 0;

	for (size_t i = 0; i < line.length(); ++i) {
        if (isspace(line[i]))
            count++;
    }
	if (line.length() == count)
		return true;
	return false;
}

void Utils::printDirectives(const std::vector<std::pair<std::string,
               std::vector<std::string> > >& directives) {
	std::vector<std::pair<std::string, std::vector<std::string> > >::const_iterator it;
	for (it = directives.begin(); it != directives.end(); ++it) {
		std::cout << it->first << ": ";
		
		std::vector<std::string>::const_iterator itv;
		for (itv = it->second.begin(); itv != it->second.end(); ++itv) {
			std::cout << *itv << ", ";
		}
		std::cout << std::endl;
    }
}

std::string Utils::handleWSpaceComments(std::string& line) {
    if (line.empty() || Utils::isOnlyWSpace(line))
		return (line);

    std::istringstream iss(line);
    std::string temp;
    line.clear();

    while (iss) {
        iss >> temp;
        if (temp.empty()) 
            break;
        line.append(temp);
        line.push_back(' ');
        temp.clear();
    }
    std::size_t	index = line.find('#');
    if (index != std::string::npos)
            line = line.substr(0, index);
    return (line);
}

Context Utils::handleContext(std::istringstream& f, std::string& content) {
    int open;
    std::string line;
    std::string contextContent;
    std::size_t index;
    open = 1;
    while (getline(f, line)) {
        if (line.find('{') != std::string::npos)
            open++;
        else if ((index = line.find('}')) != std::string::npos)
            open--;
        if (open == 0) {
            contextContent.append(line.substr(0, index+1));
            contextContent.push_back('\n');    
            break;
        }
        contextContent.append(line);
        contextContent.push_back('\n');    
    }
    Context C(content, contextContent);
    if (open == 0) {
        // std::cout << "Content at the end of context handler : " << content << std::endl;
        content = line.substr(index + 1);
        // std::cout << "Content after context handler : " << content << std::endl;

    }
    return (C);
}
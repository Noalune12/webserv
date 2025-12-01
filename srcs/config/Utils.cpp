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
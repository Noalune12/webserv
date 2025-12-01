#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <iostream>
#include "Context.hpp"

namespace Utils {

    bool isOnlyWSpace(const std::string& line);

    void printDirectives(const std::vector<std::pair<std::string,
                std::vector<std::string> > >& directives);

    std::string handleWSpaceComments(std::string& line);

}

#endif

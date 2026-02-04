#include "Request.hpp"
#include <iostream>

void Request::methodPostHandler() {
    std::cout << "ENTERING POST HANDLER" << std::endl; 
    if (_body.empty()) {
        if (!_reqLocation->root.empty())
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        else
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
        std::cout << "error with POST, no body" << std::endl;
        return ;   
    }
}
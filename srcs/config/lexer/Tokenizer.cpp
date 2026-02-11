#include "Tokenizer.hpp"
#include "Utils.hpp"

Tokenizer::Tokenizer() {}

Tokenizer::~Tokenizer() {}

void Tokenizer::tokenize(const std::string& fileContent) {
    std::istringstream f(fileContent);
    std::string line;
    std::string content;

    while (getline(f, line)) {

        line = Utils::handleWSpaceComments(line);

        if (line.empty() || Utils::isOnlyWSpace(line))
            continue;

        std::size_t index = line.find('{');
        if (!content.empty() && index != 0) {
            while (!content.empty()) {
                std::size_t semiCol = content.find(';');
                if (semiCol == std::string::npos) {
                    addDirective(content);
                    content.clear();
                } else {
                    while (content[semiCol] == ';' || content[semiCol] == ' ')
                        semiCol++;
                    std::string dir = content.substr(0, semiCol);
                    content = content.substr(semiCol);
                    addDirective(dir);
                }
            }
        }

        content.append(line);

        if (index != std::string::npos) {
            _context.push_back(Utils::handleContext(f, content));
        }
    }
    if (!content.empty()) {
        while (!content.empty()) {
            std::size_t semiCol = content.find(';');
            if (semiCol == std::string::npos) {
                addDirective(content);
                content.clear();
            } else {
                while (content[semiCol] == ';' || content[semiCol] == ' ')
                    semiCol++;
                std::string dir = content.substr(0, semiCol);
                content = content.substr(semiCol);
                addDirective(dir);
            }
        }
    }
}

void Tokenizer::addDirective(std::string line) {

    std::istringstream iss(line);
    std::string dir;

    if (!(iss >> dir)) {
        return ;
    }
    size_t index = dir.find(';');

    std::vector<std::string> args;
    std::string arg;

    if (index != 0 && index != std::string::npos) {
        args.push_back(dir.substr(index, dir.size()));
        dir = dir.substr(0, index);
    }

    while (iss >> arg) {
        if (arg[0] == ';' && !args.empty())
            args.back().append(arg);
        else
            args.push_back(arg);
    }

	PairVector::iterator itm = _globalDirectives.begin();
	for (; itm != _globalDirectives.end(); itm++) {
		if (itm->first == dir)
			break ;
	}

    if (itm != _globalDirectives.end()) {
        itm->second.push_back(" ");
        itm->second.insert(itm->second.end(), args.begin(), args.end());
    } else
		_globalDirectives.push_back(std::make_pair(dir, args));
}

void Tokenizer::printContent() const {
	std::cout << "MAIN DIR" << std::endl;
	Utils::printDirectives(_globalDirectives);
	std::cout << std::endl;
	std::vector<Context>::const_iterator it;
	for (it = _context.begin(); it != _context.end(); it++) {
		it->printContext();
	}
}

const PairVector&	Tokenizer::getGlobalDirective(void) const {
	return (this->_globalDirectives);
}

std::vector<Context>& Tokenizer::getVectorContext(void) {
	return (this->_context);
}

#ifndef CONFIG_HPP
# define CONFIG_HPP

# include "Tokenizer.hpp"
# include "ConfigInheritor.hpp"

class Config {

	private:
		const std::string&	_filePath;
		std::string			_fileContent;

		Tokenizer			_tokens;
		ConfigInheritor		_conf;

	public:

		Config(const std::string& configFile);
		~Config();

		Tokenizer&	getTokenizer(void);
		const std::string&	getFilePath(void) const;
		const std::string&	getFileContent(void) const;
		std::vector<server>&	getServers(void);
		globalDir& getGlobalDir(void);
};

#endif

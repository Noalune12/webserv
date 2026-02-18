#ifndef FILEREADER_HPP
# define FILEREADER_HPP

#include <iostream>

class FileReader {

	private:
		std::string _filePath;
		std::string _fileContent;

		void	extensionVerification(const std::string& configFile);
		void	readFile(void);

	public:
		FileReader(const std::string& filePath);
		~FileReader();

		const std::string& getFileContent(void) const;
		const std::string& getFilePath(void) const;
};

#endif

#ifndef MIMETYPES_HPP
# define MIMETYPES_HPP

# include <string>
# include <map>

class MimeTypes {

	private:
		static std::map<std::string, std::string>	_types;
		static bool									_initialized;
		static std::string							_defaultPath;
		static std::string							_lastMimeType;

		static void	parseLine(const std::string& line);
		static bool	load(const std::string& filepath);

		MimeTypes(void);

	public:
		~MimeTypes(void);

		static std::string	getType(const std::string& extensionOrFilename);
		static std::string	getExtension(const std::string& filename);

		static bool			isTextType(const std::string& mimeType);
		static bool			isSupportedType(const std::string& contentType);
};

#endif

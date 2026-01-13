#ifndef MIMETYPES_HPP
# define MIMETYPES_HPP

# include <string>
# include <map>

class MimeTypes {

	private:
		static std::map<std::string, std::string>	_types;
		static bool									_initialized;

		static void	initializeTypes(void);

		MimeTypes(void);

	public:
		~MimeTypes(void);

		static std::string	getType(const std::string& extensionOrFilename);
		static std::string	getExtension(const std::string& filename);
};

#endif

#ifndef RULES_H
# define RULES_H

# define GLOBAL "global"

# define ERR_PAGE "error_page"
# define CL_MAX_B_SYZE "client_max_body_size"
# define SERV "server"
# define SERV_NAME "server_name"
# define LISTEN "listen"
# define ROOT "root"
# define INDEX  "index"
# define LOCATION "location"
# define ALL_METHODS "allow_methods"
# define AUTOINDEX "autoindex"
# define UPLOAD_TO "upload_to"
# define RETURN "return"
# define ALIAS "alias"
# define CGI_PATH "cgi_path"
# define CGI_EXT "cgi_ext"

# define GET "GET"
# define POST "POST"
# define DEL "DELETE"

# define LOWER_K "k"
# define UPPER_K "K"
# define LOWER_M "m"
# define UPPER_M "M"
# define LOWER_G "g"
# define UPPER_G "G"

# define DEFAULT_ERR_LOG_PATH "./logs/error.log" // "logs/error.log" ?
# define DEFAULT_CL_MAX_B_SYZE "200m"
# define DEFAULT_LISTEN_PORT "8000"
# define DEFAULT_SERV_NAME "\"\""
# define DEFAULT_ROOT "html"
# define DEFAULT_INDEX "index.html"
# define DEFAULT_AUTOINDEX "off"

enum {
	GLOBAL_VALUE,
	SERV_VALUE,
	LOCATION_VALUE
};

#endif

#ifndef RULES_H
# define RULES_H

// !!! Will probably delete most of them as they are used only once. No need to include all of those everywhere only one is needed
/* directives */

# define ERR_PAGE "error_page"
# define ERR_LOG "error_log"
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

/* methods*/

# define GET "GET"
# define POST "POST"
# define DEL "DELETE"

/* client_max_body_size unity */

# define LOWER_K "k"
# define UPPER_K "K"
# define LOWER_M "m"
# define UPPER_M "M"
# define LOWER_G "g"
# define UPPER_G "G"

/* default parameters */

# define DEFAULT_ERR_LOG_PATH "./logs/error.log" // "logs/error.log" ?
# define DEFAULT_CL_MAX_B_SYZE "200m"
# define DEFAULT_LISTEN_PORT "8000" // to be defined
# define DEFAULT_SERV_NAME "\"\"" // to be defined
# define DEFAULT_ROOT "html"
# define DEFAULT_INDEX "index.html"
# define DEFAULT_AUTOINDEX "off"

#endif

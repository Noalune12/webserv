override SRCSDIR	:= srcs/
override SRCS		= $(addprefix $(SRCSDIR), $(SRC))

override SUBD_TESTDIR	:= subdirectory_test/


override CGIDIR			:= cgi/
override CONFIGDIR		:= config/
override HTTPDIR		:= http/
override LOGGERDIR		:= logger/
override INHERITORDIR	:= $(CONFIGDIR)inheritor/
override LEXERDIR		:= $(CONFIGDIR)lexer/
override READERDIR		:= $(CONFIGDIR)reader/
override VALIDATORDIR	:= $(CONFIGDIR)validator/

override SERVERDIR		:= server/

override SUBDIRS := \
	$(CGIDIR) \
	$(CONFIGDIR) \
	$(HTTPDIR) \
	$(INHERITORDIR) \
	$(LEXERDIR) \
	$(LOGGERDIR) \
	$(READERDIR) \
	$(SERVERDIR) \
	$(VALIDATORDIR)


SRC	+= $(addsuffix .cpp, $(MAIN))

override MAIN			:= \
	main


SRC += $(addprefix $(SUBD_TESTDIR), $(addsuffix .cpp, $(SUBD_TESTSRC)))

override SUBD_TESTSRC			:= \
	# test \



SRC += $(addprefix $(SERVERDIR), $(addsuffix .cpp, $(SERVERSRC)))

override SERVERSRC	:= \
	Connection \
	EpollUtils \
	EventLoop \
	EventLoopTimeout \
	EventLoopUtils \
	ServerManager

SRC += $(addprefix $(HTTPDIR), $(addsuffix .cpp, $(HTTPSRC)))

override HTTPSRC	:= \
	MimeTypes \
	Request \
	RequestChecker \
	RequestChunkHandler \
	RequestSemantics \
	Response \
	ResponseBuilder \
	ResponseSender \
	StatusCodes \
	RequestGetHandler \
	RequestDeleteHandler

SRC += $(addprefix $(CONFIGDIR), $(addsuffix .cpp, $(CONFIGSRC)))

override CONFIGSRC	:= \
	Config \
	Utils

SRC += $(addprefix $(LOGGERDIR), $(addsuffix .cpp, $(LOGGERSRC)))

override LOGGERSRC	:= \
	Logger

SRC += $(addprefix $(READERDIR), $(addsuffix .cpp, $(READERSRC)))

override READERSRC	:= \
	FileReader

SRC += $(addprefix $(VALIDATORDIR), $(addsuffix .cpp, $(VALIDATORSRC)))

override VALIDATORSRC	:= \
	Validator

SRC += $(addprefix $(LEXERDIR), $(addsuffix .cpp, $(LEXERSRC)))

override LEXERSRC	:= \
	Context \
	Tokenizer

SRC += $(addprefix $(INHERITORDIR), $(addsuffix .cpp, $(INHERITORSRC)))

override INHERITORSRC	:= \
	ConfigInheritor

SRC += $(addprefix $(CGIDIR), $(addsuffix .cpp, $(CGISRC)))

override CGISRC	:= \
	CGIEnvironment \
	CGIExecutor \
	CGIUtils

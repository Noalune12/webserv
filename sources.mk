override SRCSDIR	:= srcs/
override SRCS		= $(addprefix $(SRCSDIR), $(SRC))

override SUBD_TESTDIR	:= subdirectory_test/


override CGIDIR			:= cgi/
override HTTPDIR		:= http/
override CONFIGDIR		:= config/
override INHERITORDIR	:= $(CONFIGDIR)inheritor/
override LEXERDIR		:= $(CONFIGDIR)lexer/
override READERDIR		:= $(CONFIGDIR)reader/
override VALIDATORDIR	:= $(CONFIGDIR)validator/

override SERVERDIR		:= server/

override SUBDIRS := \
	$(CGIDIR) \
	$(HTTPDIR) \
	$(CONFIGDIR) \
	$(INHERITORDIR) \
	$(LEXERDIR) \
	$(READERDIR) \
	$(VALIDATORDIR) \
	$(SERVERDIR)


SRC	+= $(addsuffix .cpp, $(MAIN))

override MAIN			:= \
	main




SRC += $(addprefix $(SUBD_TESTDIR), $(addsuffix .cpp, $(SUBD_TESTSRC)))

override SUBD_TESTSRC			:= \
	test \



SRC += $(addprefix $(SERVERDIR), $(addsuffix .cpp, $(SERVERSRC)))

override SERVERSRC	:= \
	Server

SRC += $(addprefix $(CONFIGDIR), $(addsuffix .cpp, $(CONFIGSRC)))

override CONFIGSRC	:= \
	Config


SRC += $(addprefix $(READERDIR), $(addsuffix .cpp, $(READERSRC)))

override READERSRC	:= \
	FileReader

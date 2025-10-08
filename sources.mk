override SRCSDIR	:= srcs/
override SRCS		= $(addprefix $(SRCSDIR), $(SRC))

override SUBD_TESTDIR	:= subdirectory_test/

SRC	+= $(addsuffix .cpp, $(MAIN))

override MAIN			:= \
	main

SRC += $(addprefix $(SUBD_TESTDIR), $(addsuffix .cpp, $(SUBD_TESTSRC)))

override SUBD_TESTSRC			:= \
	test \

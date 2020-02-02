# Set up the compiler
CFLAGS=-I../../inc -I.
LFLAGS=

# The name of the module we are compiling here
MODULE=toolbox

# Object files that comprise the module
OBJS=toolbox.o init.o newlist.o

#----------------------------------------------------------------
#  You shouldn't need to change anything below here.
#----------------------------------------------------------------

ifdef BSDMOD
ifdef OBSDMOD
EX_REQ_LF = -rdynamic -fpic -Wl,-Bshareable -Wl,-x
endif
EXTRA_LFLAGS = -shared ${EX_REQ_LF}
else
ifndef BSDIMOD
EXTRA_LFLAGS = -shared -Wl,-soname,${MODULE_LIB}
else
EXTRA_LFLAGS = -r
endif
endif

ifdef DYNMOD
MODULE_LIB = ${MODULE}.lpm
ifndef BSDIMOD
ifdef BSDMOD
ifdef OBSDMOD
EXTRA_CFLAGS = -fPIC -DDYNMOD -DBSDMOD -DOBSDMOD
else
EXTRA_CFLAGS = -fPIC -DDYNMOD -DBSDMOD
endif
else
EXTRA_CFLAGS = -fPIC -DDYNMOD
endif
else
EXTRA_CFLAGS = -r -DDYNMOD -DBSDIMOD
endif
else
MODULE_LIB = lib${MODULE}.a
EXTRA_CFLAGS =
EXTRA_LFLAGS =
endif

ifdef SUNOS_5
CFLAGS=-I../../inc -I. -DSUNOS_5
endif

all: ${MODULE_LIB}

${MODULE_LIB}: ${OBJS}
ifdef DYNMOD
	${CC} ${EXTRA_LFLAGS} ${LFLAGS} -o ${MODULE_LIB} ${OBJS}
	cp ${MODULE_LIB} ../../build
else
	ar rc ${MODULE_LIB} ${OBJS}
	ranlib ${MODULE_LIB} || true
	cp ${MODULE_LIB} ../..
endif

clean:
	rm -f ${OBJS} ${MODULE_LIB}

pristine: clean
ifdef DYNMOD
	rm -f ../../build/${MODULE_LIB}
else
	rm -f ../../${MODULE_LIB}
endif

.c.o:
	${CC} ${EXTRA_CFLAGS} ${WFLAGS} ${CFLAGS} ${GNU_STRFTIME} ${STRRCHR} ${STRCHR} -c $*.c

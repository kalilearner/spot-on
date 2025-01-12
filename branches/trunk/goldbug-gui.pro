cache()
include(goldbug-gui-source.pro)
libntru.target = libntru.so
libntru.commands = $(MAKE) -C ../../libNTRU
libntru.depends =

TEMPLATE	= app
LANGUAGE	= C++
QT		+= bluetooth \
                   concurrent \
                   gui \
                   multimedia \
                   network \
                   printsupport \
                   sql \
                   websockets \
                   widgets
CONFIG		+= qt release warn_on

DEFINES	+= SPOTON_BLUETOOTH_ENABLED \
           SPOTON_DATELESS_COMPILATION \
           SPOTON_GOLDBUG \
	   SPOTON_LINKED_WITH_LIBGEOIP \
	   SPOTON_LINKED_WITH_LIBNTRU \
           SPOTON_LINKED_WITH_LIBPTHREAD \
           SPOTON_POPTASTIC_SUPPORTED \
	   SPOTON_SCTP_ENABLED \
	   SPOTON_WEBSOCKETS_ENABLED

# Unfortunately, the clean target assumes too much knowledge
# about the internals of libNTRU.

QMAKE_CLEAN     += ../../libNTRU/*.so \
                   ../../libNTRU/src/*.o \
                   ../../libNTRU/src/*.s \
                   GoldBug
QMAKE_DISTCLEAN += -r temp .qmake.cache .qmake.stash
QMAKE_CXXFLAGS_RELEASE += -Wall \
                          -Wcast-align \
                          -Wcast-qual \
                          -Wdouble-promotion \
                          -Werror \
                          -Wextra \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wstrict-overflow=5 \
                          -fPIE \
                          -fstack-protector-all \
                          -fwrapv \
                          -mtune=native \
                          -pedantic \
                          -pie \
                          -std=c++11
QMAKE_LFLAGS_RELEASE += -Wl,-rpath,/usr/local/goldbug/Lib
QMAKE_EXTRA_TARGETS = libntru
QMAKE_LFLAGS_RPATH =
INCLUDEPATH	+= . \
                   ../../. \
                   /usr/include/postgresql \
                   GUI
LIBS		+= -L../../libNTRU \
                   -lGeoIP \
                   -lcrypto \
                   -lcurl \
                   -lgcrypt \
                   -lgpg-error \
                   -lntru \
                   -lpq \
                   -lpthread \
                   -lsqlite3 \
                   -lssl
PRE_TARGETDEPS = libntru.so
OBJECTS_DIR = temp/obj
UI_DIR = temp/ui
MOC_DIR = temp/moc
RCC_DIR = temp/rcc

TARGET		= GoldBug
PROJECTNAME	= GoldBug

# Prevent qmake from stripping everything.

QMAKE_STRIP	= echo

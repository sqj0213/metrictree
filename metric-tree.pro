TEMPLATE = app CONFIG += console network
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += precompile_header
CONFIG += thread

INCLUDEPATH += /usr/local/include
LIBS += -L/usr/local/lib/

win32-g++ {
	message(win32-g++)
	CONFIG -= exceptions_off
	QMAKE_CXXFLAGS *= $$QMAKE_CXXFLAGS_EXCEPTIONS_ON
	QMAKE_LFLAGS *= $$QMAKE_LFLAGS_EXCEPTIONS_ON
	CONFIG -= rtti_off
	QMAKE_CXXFLAGS *= $$QMAKE_CXXFLAGS_RTTI_ON
	QMAKE_CXXFLAGS += -std=c++11
        LIBS += -lboost_system-mt-d
        LIBS += -lboost_thread-mt-d
        LIBS += -lboost_exception-mt-d
        LIBS += -lboost_date_time-mt-d
}

win32-msvc2010 {
	QMAKE_CXXFLAGS -= -Zc:wchar_t-
	QMAKE_CXXFLAGS += -Zc:wchar_t
}
	QMAKE_CXXFLAGS += -std=c++0x
        LIBS += -lboost_system
        LIBS += -lboost_thread
        LIBS += -lboost_exception
        LIBS += -lboost_date_time
	LIBS += -lboost_serialization
        LIBS += /usr/local/lib/libboost_log_setup.a
        LIBS += /usr/local/lib/libboost_log.a
        LIBS += /usr/local/lib/libboost_filesystem.a
	

LIBS += -std=c++0x

SOURCES += main.cpp \
    tcp_session.cpp \
    log.cpp \

HEADERS += \
    intdef.hpp \
    io_service_pool.hpp \
    tcp_buffer.hpp \
    tcp_session.h \
    tcp_handler.h \
    tcp_server.hpp \
    echo_handler.hpp \
    log.hpp \
    CSafeUnorderedMap.hpp
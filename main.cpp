#include <iostream>
//#include "inc.hpp"
#include "tcp_server.hpp"
#include "echo_handler.hpp"
#include "echo_handler_cmd.hpp"
#include "daemon.h"

#include <boost/ref.hpp>

#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;
using namespace boost::property_tree;
void help( int argc, char *argv[] )
{
        for ( int i = 1; i < argc; i++ )
        {
                if ( strcmp( argv[i], "-h" ) == 0 )
		{
			cout<<"/usr/sbin/metricTree -f /etc/metric-tree-read.ini"<<endl;
			cout<<"/usr/sbin/metricTree [default: -f /etc/metric-tree-read.ini]"<<endl;
			cout<<"/usr/sbin/metricTree [-c] -f /etc/metric-tree-read.ini&"<<endl;
			exit(0);
		}
        }
}

//BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(my_logger,src::logger_mt)

int checkConfigFlag( int argc, char *argv[] )
{
	for ( int i = 1; i < argc; i++ )
	{
		if ( strcmp( argv[i], "-c" ) == 0 )
			return i;
	}
	return argc;
}
int main( int argc, char *argv[] )
{
    ptree ini;
    string errMsg;
    help( argc, argv );
    if ( argc < 2 )
    	ini_parser::read_ini( "/etc/metric-tree.ini", ini );
    else
    {
        if ( checkConfigFlag( argc, argv )  == argc )
    		ini_parser::read_ini( argv[2], ini );
	else
    		ini_parser::read_ini( argv[3], ini );
    }

    if ( !checkConfig( ini ) )
    {
	cout<<"config file error:"<<argv[3]<<endl;
	exit(1);
    }
    else  if ( checkConfigFlag( argc, argv )  <  argc )
    {
	cout<<"config file ok:"<<argv[3]<<endl;
	exit(0);
    }

    //初始化日志文件
    logHandler->Init(ini.get<std::string>("log.logPath"), ini.get<unsigned long>("log.logSize"));
    //初始化全局变量
    pidFile=ini.get<std::string>("main.pidFile");
    nodeType=ini.get<std::string>("main.nodeType");
    metricTreeDataDir=ini.get<std::string>( "main.dataDir" );
    storeLockFile=ini.get<std::string>( "main.storeLockFile" );
    /*
    if ( access( pidFile.c_str(), 0 ) == 0 )
    {
    	cout<<"pidFile is exists:"<<pidFile<<endl;
	exit(1);
    }
    */
    //是否以daemon方式运行程序
    if ( ini.get<std::string>("main.daemon") == "true" )
        init_daemon();

    //创建pid文件并加锁
    /*
    pidFileHandler = fopen( pidFile.c_str(), "w" );
    ostringstream tmpStr; 
    tmpStr<<getpid()<<endl;
    if ( flock( fileno( pidFileHandler ), LOCK_EX ) != 0 )
    {
	cout<<" lock pidFile("<<pidFile<<") failed and exit!"<<endl;
    	BOOST_LOG_TRIVIAL(error)<< " lock pidFile("<<pidFile<<") failed and exit!";
	sleep(2);
	exit(1);
    }
    else
	BOOST_LOG_TRIVIAL(info)<< " lock pidFile("<<pidFile<<") success!";

    fputs( tmpStr.str().c_str(), pidFileHandler );
    fflush( pidFileHandler );
    tmpStr.str("");
    */

    BOOST_LOG_TRIVIAL(info)<< " Hello World!";
    BOOST_LOG_TRIVIAL(info)<< " build pidFile("<<pidFile<<") pid:"<<getpid();

    ifstream fileHandle( metricTreeDataDir );
    //检查存储文件是否存在，load数据到内存
    if ( fileHandle )
    {
    	fileHandle.close();
    	ostringstream os;
    	load( os );
	BOOST_LOG_TRIVIAL(info)<<" cmd:mainInitData "<<os.str();
    	cout<<os.str()<<endl;
    }
    //若为读节点，存储文件不存在则直接退出
    else
    {
	cout<<"data file is not exist:"<<metricTreeDataDir<<endl;
    	if ( nodeType == "read" )
	{
		cout<<"this is readNode and storefile is empty, thread exit!"<<endl;
		exit(1);
	}
    }
    fileHandle.close();

    io_service_pool ios1(ini.get<unsigned>("main.threadCount"));
    if ( nodeType == "write" )
    	ios1.add(new boost::thread(storeThread,ini.get<unsigned>("main.flushInterval")));
    else
    	ios1.add(new boost::thread(reloadStoreThread,ini.get<unsigned>("main.reloadInterval")));
    tcp_server<echo_handler_cmd> srv1(ios1, ini.get<unsigned>( "main.cmdport" ), 1 );
    if ( nodeType == "write" )
    {
    	tcp_server<echo_handler> srv(ios1, ini.get<unsigned>( "main.port" ),0);
    	srv.run();
    }
    srv1.run();

    return 0;
}


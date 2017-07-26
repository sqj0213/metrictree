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
			cout<<"/usr/sbin/metricTree -f /data2/metric-tree/metric-tree.xml"<<endl;
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
    //初始化日志文件
    //初始化全局变量

    ifstream fileHandle( argv[2]);
    //检查存储文件是否存在，load数据到内存

    metricTreeDataDir=argv[2];
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
    }
    
    printTree(  myTree );
    fileHandle.close();
}


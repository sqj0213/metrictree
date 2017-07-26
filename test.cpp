#include <iostream>
#include "tcp_server.hpp"
#include "echo_handler.hpp"
#include "echo_handler_cmd.hpp"
#include "inc.hpp"

#include <boost/ref.hpp>

#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;


void expire11( const string keyList, time_t fromTime, ostringstream &os )
{
        ptree retTree;
        string::size_type  pos=0,startPos=0;
        string key;
        pos = keyList.find('|', pos );
        while ( true )
        {
                key = keyList.substr(startPos,pos-startPos);
                time_t timeValue=myTree.get<time_t>( key, 1 );
                if ( timeValue > 1 && timeValue > fromTime )
                {
                        replace( key, '.', "|" );
                        retTree.put( key, timeValue );
                }
                startPos=pos+1;
                if ( pos == string::npos )
                        break;
                pos = keyList.find('|', pos+1 );
        }
        json_parser::write_json( os, retTree );
        retTree.clear();
        return;
}

int main()
{
	time_t aa=boost::lexical_cast<time_t>(-1);
	cout<<aa<<endl;
	ostringstream os;
	string keyList="sxxxx";
	time_t fromTime=1111;
	expire11(keyList, fromTime, os );
	cout<<os.str()<<endl;
	exit(1);
}

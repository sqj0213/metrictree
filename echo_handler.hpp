#ifndef ECHO_HANDLER_H
#define ECHO_HANDLER_H

#include "tcp_handler.h"
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include "inc.hpp"


class echo_handler : public tcp_handler
{
public:
    echo_handler()
    {
        handle_open = boost::bind(&echo_handler::echo_handle_open,this,_1);
        handle_close = boost::bind(&echo_handler::echo_handle_close,this,_1);
        handle_read = boost::bind(&echo_handler::echo_handle_read,this,_1,_2);
        handle_write = boost::bind(&echo_handler::echo_handle_write,this,_1,_2);
    }

private:
    string remoteIP;
    void echo_handle_open(tcp_session_type session)
    {
    }
    void echo_handle_close(tcp_session_type session)
    {
	    BOOST_LOG_TRIVIAL(info)<<" session close:"<<remoteIP;
        //cout << "session close!" << endl;
    }
/*
	strlen=aa.length();
	strTmp = aa;
        while ( ( pos = strTmp.find( '\n', startPos ) ) != string::npos )
        {
                //截取空格前的key，原始的内容：key timestamp value\n
		if ( startPos >= strlen )
			break;
		cout<<"startPos:"<<startPos<<":pos:"<<pos<<endl;
                strTmp1 = strTmp.substr( startPos, pos-startPos );
		cout<<"step1----strTmp1:"<<strTmp1<<":startPos:"<<startPos<<":pos:"<<pos<<":strlen:"<<strlen<<endl;
                if ( ( pos1 = strTmp1.find( ' ' ) ) != string::npos )
		{
			cout<<"pos1:"<<pos1<<":startPos:"<<startPos<<":s-p1:"<<(pos1-startPos)<<endl;
                        strTmp1 = strTmp1.substr( 0, pos1 );
		}
		cout<<"step2-----strTmp1:"<<strTmp1<<":startPos:"<<startPos<<":pos:"<<pos<<endl;
                startPos=startPos + pos + 1;
		if ( startPos > strlen )
			break;
        }

*/
    void echo_handle_read(tcp_session_type session,std::size_t n)
    {
        std::string strTmpSrc = string(session->read_buf_stream_1().peek(),n);
        size_t leftPos,rightPos,pos,pos1,startPos;
        std::string strTmp1,strTmp;
        if ( strTmpSrc.length() > 1 )
        {
            leftPos = strTmpSrc.find('\n');
            rightPos = strTmpSrc.rfind('\n');
            if ( leftPos == rightPos && leftPos != string::npos )
            {
                if ( strTmpSrc.find( "stats_byhost." ) == 0 ||strTmpSrc.find( "stats_gather." ) == 0 ||strTmpSrc.find( "system_byhost." ) == 0 ||strTmpSrc.find( "noc-status." ) == 0 ||strTmpSrc.find( "weibo_platform_consul." ) == 0)
                    strTmp=strTmpSrc.substr(0, leftPos+1);
                else
                {
                    BOOST_LOG_TRIVIAL(info)<<" packet:'"<<strTmpSrc<<"' start string not in (stats_byhost.|stats_gather.|system_byhost.|noc-status.|weibo_platform_consul.) invalid packet!"<<endl;
                    return;
                }
            }
            else
            { 
                if ( leftPos == rightPos && leftPos == string::npos )
                {
                    BOOST_LOG_TRIVIAL(info)<<" packet:'"<<strTmpSrc<<"' invalid packet!"<<endl;
                    return;
                }
                strTmp = strTmpSrc.substr( leftPos+1 );
                rightPos = strTmp.rfind('\n');
                strTmp = strTmp.substr( 0, rightPos+1 );
            }
            startPos=0; 
            size_t i=0;
            while( ( pos = strTmp.find('\n',startPos)) != string::npos ) 
            {
                strTmp1=strTmp.substr( startPos, pos - startPos );
                //截取空格前的key，原始的内容：key timestamp value\n
                if ( ( pos1 = strTmp1.find( ' ' ) ) != string::npos )
	            {
        	        strTmp1 = strTmp1.substr( 0, pos1 );
		            //第一个段可能是截断的数据，所以要丢弃
		            if ( strTmp1.size() > 2 )
                    {
			            buildTree( strTmp1 );
                        i++;
                    }
	            }
                startPos = pos + 1;
            }
        }
        if(strTmp=="q")
        {
            session->close();
        }
        //cout << string(session->read_buf().peek(),n) << endl;
        //session->write(session->read_buf().peek(),n);
    }
    void echo_handle_write(tcp_session_type session,std::size_t n)
    {
	;
        //cout << "write complete!" << endl;
    }
};

#endif // ECHO_HANDLER_H

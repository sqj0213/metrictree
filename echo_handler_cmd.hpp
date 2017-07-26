#ifndef ECHO_HANDLER_CMD_H
#define ECHO_HANDLER_CMD_H

#include "tcp_handler.h"
#include <boost/bind.hpp>
#include <sstream>
#include "inc.hpp"

class echo_handler_cmd : public tcp_handler
{
public:
    echo_handler_cmd()
    {
        handle_open = boost::bind(&echo_handler_cmd::echo_handle_open,this,_1);
        handle_close = boost::bind(&echo_handler_cmd::echo_handle_close,this,_1);
        handle_read_cmd = boost::bind(&echo_handler_cmd::echo_handle_read,this,_1,_2);
        handle_write = boost::bind(&echo_handler_cmd::echo_handle_write,this,_1,_2);
    }

private:
    void echo_handle_open(tcp_session_type session)
    {
        //BOOST_LOG_TRIVIAL(debug)<<"session start from:"<<remoteIP;
        //cout << "session start from: " << session->socket().remote_endpoint().address() << endl;
    }
    void echo_handle_close(tcp_session_type session)
    {
	//BOOST_LOG_TRIVIAL(debug)<<"session close:"<<remoteIP;
        //cout << "session close!" << endl;
    }
    void echo_handle_read(tcp_session_type session,std::size_t n)
    {
	//return boost::asio::buffer_cast<const char*>(m_read_buf_stream.data());
        std::string strTmp = string(session->read_buf_stream_2().peek(),n);
        //std::string strTmp= string(boost::asio::buffer_cast<const char*>(session->read_buf_stream_2().data()),n);
        // BOOST_LOG_TRIVIAL(info)<<"strTmp:'"<<strTmp<<"',strTmp.size():"<<strTmp.size();
	    if ( strTmp.size() > 1 )
	    {
		    size_t pos=strTmp.rfind('\r');
		    if ( pos != string::npos )
			    strTmp.replace( pos, 1, "" ); 
	    }
        if ( strTmp.size() > 1 )
            strTmp=strTmp.substr(0, strTmp.size()-1);

        ostringstream os;
        os<<session->socket().remote_endpoint().address();
        string remoteIP=os.str();
        os.str("");

        if ( strTmp.find("finder ") == 0 )
        {
                runCmd(strTmp,os);
                //cout<<"finder run cmd:"<<strTmp<<endl;
                BOOST_LOG_TRIVIAL(info)<<"remoteIP:"<<remoteIP<<" finder run cmd:"<<strTmp<<" result.size:"<<os.str().size();
                session->write( os.str().c_str(), os.str().size() );
                os.str("");
                return;
        }
        if ( strTmp.find("expire") == 0 )
        {
        		string retStr;
                runCmd(strTmp,os);
		        retStr = os.str();
		        replace( retStr, '|', "." );
                os.str("");
                //os <<"("<<strTmp<<")file store complete!"<<endl;
                BOOST_LOG_TRIVIAL(info)<<"remoteIP:"<<remoteIP<<" expire run cmd:"<<strTmp<<" result.size:"<<retStr.size();
                //cout<<"finder run cmd:"<<strTmp<<endl;
                session->write( retStr.c_str(), retStr.size() );
                return;
        }
        if ( strTmp.find("erase") == 0 )
        {
                string retStr;
                runCmd(strTmp,os);
                retStr = os.str();
                replace( retStr, '|', "." );
                os.str("");
                //os <<"("<<strTmp<<")file store complete!"<<endl;
                BOOST_LOG_TRIVIAL(info)<<"remoteIP:"<<remoteIP<<" expire run cmd:"<<strTmp<<" result.size:"<<retStr.size();
                //cout<<"finder run cmd:"<<strTmp<<endl;
                session->write( retStr.c_str(), retStr.size() );
                return;
        }
        if ( strTmp.find("expireAll") == 0 )
        {
                string retStr;
                runCmd(strTmp,os);
                retStr = os.str();
                replace( retStr, '|', "." );
                os.str("");
                //os <<"("<<strTmp<<")file store complete!"<<endl;
                BOOST_LOG_TRIVIAL(info)<<"remoteIP:"<<remoteIP<<" expireAll run cmd:"<<strTmp<<" result.size:"<<retStr.size();
                //cout<<"finder run cmd:"<<strTmp<<endl;
                session->write( retStr.c_str(), retStr.size() );
                return;
        }
        if ( strTmp.find("deepTree") == 0 )
        {
                string retStr;
                runCmd(strTmp,os);
                retStr = os.str();
                replace( retStr, '|', "." );
                os.str("");
                //os <<"("<<strTmp<<")file store complete!"<<endl;
                BOOST_LOG_TRIVIAL(info)<<"remoteIP:"<<remoteIP<<" deepTree run cmd:"<<strTmp<<" result.size:"<<retStr.size();
                //cout<<"finder run cmd:"<<strTmp<<endl;
                session->write( retStr.c_str(), retStr.size() );
                return;
        }
        if ( strTmp.find("tree") == 0 )
        {
                string retStr;
                runCmd(strTmp,os);
                retStr = os.str();
                replace( retStr, '|', "." );
                os.str("");
                //os <<"("<<strTmp<<")file store complete!"<<endl;
                BOOST_LOG_TRIVIAL(info)<<"remoteIP:"<<remoteIP<<" tree run cmd:"<<strTmp<<" result.size:"<<retStr.size();
                //cout<<"finder run cmd:"<<strTmp<<endl;
                session->write( retStr.c_str(), retStr.size() );
                return;
        }
	if ( strTmp.compare("print") == 0 )
	{
		BOOST_LOG_TRIVIAL(info)<<"remoteIP:"<<remoteIP<<" cmd:print";
		runCmd(strTmp,os);
		session->write( os.str().c_str(), os.str().size() );
		os.str("");
		return;
	}
	if ( strTmp.compare("status") == 0 )
	{
		BOOST_LOG_TRIVIAL(info)<<"remoteIP:"<<remoteIP<<" cmd:status";
		runCmd("status",os);
		session->write( os.str().c_str(), os.str().size() );
		os.str("");
		return;
	}
        if ( strTmp.find("store") == 0 )
        {
                runCmd(strTmp,os);
		os <<"("<<strTmp<<")file store complete!"<<endl;
		BOOST_LOG_TRIVIAL(info)<<"remoteIP:"<<remoteIP<<" finder run cmd:"<<strTmp<<" result.size:"<<os.str().size();
                //cout<<"finder run cmd:"<<strTmp<<endl;
                session->write( os.str().c_str(), os.str().size() );
		os.str("");
		return;
        }

       	if(strTmp=="q")
        	session->close();
        //cout << string(session->read_buf().peek(),n) << endl;
        //session->write(session->read_buf().peek(),n);
    }
    void echo_handle_write(tcp_session_type session,std::size_t n)
    {
	//BOOST_LOG_TRIVIAL(info)<<"write complete!";
        //cout << "write complete!" << endl;
    }
};

#endif // ECHO_HANDLER_CMD_H

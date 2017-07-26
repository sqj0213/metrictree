#include "tcp_session.h"
#include "log.hpp"
#include <iostream>
#include <string>

using namespace std;

#include <boost/bind.hpp>
using namespace boost;
using namespace boost::asio;

tcp_session::tcp_session(ios_type &ios) : m_socket(ios)
{
}

tcp_session::socket_type& tcp_session::socket()
{
    return m_socket;
}

tcp_session::ios_type& tcp_session::io_service()
{
    return m_socket.get_io_service();
}

tcp_session::buffer_type& tcp_session::read_buf()
{
    return m_read_buf;
}
tcp_session::buffer_type& tcp_session::read_buf_stream_1()
{
	//return boost::asio::buffer_cast<const char*>(m_read_buf_stream.data());
	return m_read_buf_stream_1;
}
tcp_session::buffer_type& tcp_session::read_buf_stream_2()
{
        //return boost::asio::buffer_cast<const char*>(m_read_buf_stream_2.data());
        return m_read_buf_stream_2;
}

tcp_session::buffer_type& tcp_session::write_buf()
{
    return m_write_buf;
}

void tcp_session::start_write(tcp_handler handler)
{
//    BOOST_LOG_SEV(my_logger::get(), info) << "session start";
    //BOOST_LOG_TRIVIAL(info)<<"session start";
    BOOST_LOG_TRIVIAL(debug)<<"session write start from:"<<m_socket.remote_endpoint().address();
    m_handler = handler;
    if(m_handler.handle_open)
    {
        m_handler.handle_open(shared_from_this());
    }
    read();
}
void tcp_session::start_read(tcp_handler handler)
{
//    BOOST_LOG_SEV(my_logger::get(), info) << "session start";

    //BOOST_LOG_TRIVIAL(info)<<"session start";
    BOOST_LOG_TRIVIAL(debug)<<"session read start from:"<<m_socket.remote_endpoint().address();
    m_handler_read = handler;
    if(m_handler_read.handle_open)
    {
        m_handler_read.handle_open(shared_from_this());
    }
    read_cmd();
}

void tcp_session::close()
{
    boost::system::error_code ignored_ec;
    m_socket.shutdown(ip::tcp::socket::shutdown_both,ignored_ec);
    m_socket.close(ignored_ec);


    if(m_handler.handle_close)
    {
        m_handler.handle_close(shared_from_this());
    }
    if(m_handler_read.handle_close)
    {
        m_handler_read.handle_close(shared_from_this());
    }
}

void tcp_session::read()
{
/*
	boost::asio::async_read_until( m_socket, m_read_buf_stream_1, '\n', boost::bind(&tcp_session::handle_read,shared_from_this(),
        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
*/
    m_socket.async_read_some(m_read_buf_stream_1.prepare(),
        boost::bind(&tcp_session::handle_read,shared_from_this(),
        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}
void tcp_session::read_cmd()
{
/*
	boost:asio::async_read_until( m_socket, m_read_buf_stream_2, '\n', boost::bind(&tcp_session::handle_read_cmd,shared_from_this(),
        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred) );
*/
    m_socket.async_read_some(m_read_buf_stream_2.prepare(),
        boost::bind(&tcp_session::handle_read_cmd,shared_from_this(),
        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void tcp_session::handle_read(const system::error_code &ec, std::size_t bytes_transferred)
{
    if(ec)
    {
		close();
		return;
    }

    m_read_buf_stream_1.retrieve(bytes_transferred);

    if( m_handler.handle_read )
    {
        	//cout<<tmpStr<<" pos:"<<pos<<" dropPos:"<<dropPos<<endl;
        	//m_handler.handle_read(shared_from_this(),bytes_transferred);
        	m_handler.handle_read(shared_from_this(),bytes_transferred);
    }
    else
    	BOOST_LOG_TRIVIAL(error)<<" totalSize:"<<m_read_buf_stream_1.size()<<" bytes_transferred:"<<bytes_transferred<<"  errMsg:m_handler.handle_read failed and drop data!";
    m_read_buf_stream_1.consume(bytes_transferred);
    read();
}
void tcp_session::handle_read_cmd(const system::error_code &ec, std::size_t bytes_transferred)
{

    string tmpStr;
    size_t pos=string::npos;
    if(ec)
    {
        close();
        return;
    }

    m_read_buf_stream_2.retrieve(bytes_transferred);
    tmpStr = m_read_buf_stream_2.peek();
    pos = tmpStr.find( '\n' );
    //cout<<tmpStr<<" pos:"<<pos<<" dropPos:"<<dropPos<<endl;

    if ( pos != string::npos )
    {
        if( m_handler_read.handle_read_cmd )
        {
            //cout<<tmpStr<<" pos:"<<pos<<" dropPos:"<<dropPos<<endl;
            //m_handler.handle_read(shared_from_this(),bytes_transferred);
            m_handler_read.handle_read_cmd(shared_from_this(),pos+1);
    }
    else
            BOOST_LOG_TRIVIAL(error)<<" data:"<<tmpStr<<"  errMsg:m_handler.handle_read_cmd failed and drop request!";
        
        m_read_buf_stream_2.consume(pos+1);
    }
    read_cmd();
}


void tcp_session::write(const void *data, std::size_t len)
{
    //cout << "write:" << len << endl;
    //cout << static_cast<const char*>(data) << endl;
	m_write_buf_stream.sputn(static_cast<const char*>(data),boost::numeric_cast<std::streamsize>(len));
    //std::ostream os(&m_write_buf_stream);
    //os <<data;
    write();
    m_write_buf_stream.consume(len);
}

void tcp_session::write()
{
    m_socket.async_write_some(m_write_buf_stream.data(),
        boost::bind(&tcp_session::handle_write,shared_from_this(),
          boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
}

void tcp_session::handle_write(const system::error_code &ec, std::size_t bytes_transferred)
{
    if(ec)
    {
	    BOOST_LOG_TRIVIAL(info)<<" session close()"<<endl;
        close();
        return;
    }

    if(m_handler_read.handle_write)
        m_handler_read.handle_write(shared_from_this(),bytes_transferred);
    write();
}

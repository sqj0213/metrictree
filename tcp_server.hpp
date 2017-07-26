#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "intdef.hpp"
#include "tcp_session.h"
#include "io_service_pool.hpp"
#include <boost/bind.hpp>
#include <boost/functional/factory.hpp>

using namespace boost;
using namespace boost::asio;


template<typename Handler>
class tcp_server
{
public:
    typedef io_service_pool::ios_type ios_type;
    typedef boost::asio::ip::tcp::acceptor acceptor_type;
    typedef boost::shared_ptr<tcp_session> tcp_session_ptr;
    typedef Handler handler_type;
public:
    tcp_server(uint16_t uiPort,int n = 1):
        m_ios_pool(*factory<io_service_pool*>()(n)),
        m_acceptor(m_ios_pool.get(),ip::tcp::endpoint(ip::tcp::v4(),uiPort))
    {
        start_acceptor();
    }

    tcp_server(io_service_pool &ios,uint16_t uiPort,int _serverType=0):
        m_ios_pool(ios),
        m_acceptor(m_ios_pool.get(),ip::tcp::endpoint(ip::tcp::v4(),uiPort))
    {
	serverType=_serverType,
        start_acceptor();
    }

    void start()
    {
        m_ios_pool.start();
    }
    void run()
    {
        m_ios_pool.run();
    }
private:
    void start_acceptor()
    {
        tcp_session_ptr session = factory<tcp_session_ptr>()(m_ios_pool.get());
        m_acceptor.async_accept(session->socket(),boost::bind(&tcp_server::handle_acceptor,this,boost::asio::placeholders::error,session));
    }
    void handle_acceptor(const boost::system::error_code &ec,tcp_session_ptr session)
    {
        start_acceptor();
        if(ec)
        {
            session->close();
            return;
        }
	if ( serverType == 0 )
        	session->start_write(m_handler);
	else
        	session->start_read(m_handler);
		
		
    }
private:
	int serverType;
    handler_type m_handler;
    io_service_pool &m_ios_pool;
    acceptor_type m_acceptor;
};

#endif // TCP_SERVER_H

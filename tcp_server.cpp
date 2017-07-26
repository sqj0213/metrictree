#include "tcp_server.h"
#include <boost/bind.hpp>
#include <boost/functional/factory.hpp>

using namespace boost;
using namespace boost::asio;

template<typename Handler>
tcp_server::tcp_server(uint16_t uiPort,int n,unsigned _serverType=0):
    m_ios_pool(*factory<io_service_pool*>()(n)),
    m_acceptor(m_ios_pool.get(),ip::tcp::endpoint(ip::tcp::v4(),uiPort))
{
    serverType=_serverType;
    start_acceptor();
}

template<typename Handler>
tcp_server::tcp_server(io_service_pool &ios, uint16_t uiPort,int _serverType=0):
    serverType=_serverType,
    m_ios_pool(ios),
    m_acceptor(m_ios_pool.get(),ip::tcp::endpoint(ip::tcp::v4(),uiPort))
{
    start_acceptor();
}

template<typename Handler>
void tcp_server::start_acceptor()
{
    tcp_session_ptr session = factory<tcp_session_ptr>()(m_ios_pool.get());
    m_acceptor.async_accept(session->socket(),boost::bind(&tcp_server::handle_acceptor,this,placeholders::error,session));
}

template<typename Handler>
void tcp_server::handle_acceptor(const system::error_code &ec, tcp_session_ptr session)
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

template<typename Handler>
void tcp_server::start()
{
    m_ios_pool.start();
}

template<typename Handler>
void tcp_server::run()
{
    m_ios_pool.run();
}

#ifndef TCP_SESSION_H
#define TCP_SESSION_H

#include "tcp_buffer.hpp"
#include "tcp_handler.h"
#include <boost/smart_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>


class tcp_session : public boost::enable_shared_from_this<tcp_session>
{
public:
    typedef boost::asio::ip::tcp::socket socket_type;
    typedef boost::asio::io_service ios_type;
    typedef boost::asio::streambuf streambuf_type;
    typedef tcp_buffer buffer_type;
    size_t prePos;
public:
    tcp_session(ios_type &ios);
    socket_type& socket();
    ios_type& io_service();
    buffer_type& read_buf();
    buffer_type& read_buf_stream_1();
    buffer_type& read_buf_stream_2();
    buffer_type& write_buf();

    void start_write(tcp_handler handler=tcp_handler());
    void start_read(tcp_handler handler=tcp_handler());
    void close();
    void write();
    void write(const void *data,std::size_t len);

private:
    void read();
    void replace( string &, const char, const string );
    void handle_read(const boost::system::error_code &ec,std::size_t bytes_transferred);
    void read_cmd();
    void handle_read_cmd(const boost::system::error_code &ec,std::size_t bytes_transferred);
    void handle_write(const boost::system::error_code &ec,std::size_t bytes_transferred);
private:
    tcp_handler m_handler;
    tcp_handler m_handler_read;
    socket_type m_socket;
    buffer_type m_read_buf;
    buffer_type m_write_buf;
    buffer_type m_read_buf_stream_1;
    buffer_type m_read_buf_stream_2;
    streambuf_type m_write_buf_stream;
};

#endif // TCP_SESSION_H

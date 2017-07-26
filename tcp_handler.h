#ifndef TCP_HANDLER_H
#define TCP_HANDLER_H

#include <boost/smart_ptr.hpp>
#include <boost/function.hpp>
#include <iostream>
using namespace std;

class tcp_session;

struct tcp_handler
{
    typedef const boost::shared_ptr<tcp_session>& tcp_session_type;

    //处理打开关闭连接
    boost::function<void(tcp_session_type)> handle_open;
    boost::function<void(tcp_session_type)> handle_close;

    //处理读写数据
    boost::function<void(tcp_session_type,std::size_t)> handle_read;
    boost::function<void(tcp_session_type,std::size_t)> handle_read_cmd;
    boost::function<void(tcp_session_type,std::size_t)> handle_write;
};


#endif // TCP_HANDLER_H

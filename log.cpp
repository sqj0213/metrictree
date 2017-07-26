#include "log.hpp"  
  
#include <iostream>  
#include <string>  
  
BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(my_logger, src::logger_mt)  
boost::log::sources::severity_logger<boost::log::trivial::severity_level > MyLog::s_slg;  
  
MyLog::MyLog()  
{  
  
}  
  
MyLog::~MyLog(void)  
{  
   
}  
  
void MyLog::Init(const string & dir, unsigned long logSize)  
{  
  if (boost::filesystem::exists(dir) == false)  
  {  
    boost::filesystem::create_directories(dir);  
  }  
  
  auto pSink = logging::add_file_log  
  (  
      keywords::open_mode = std::ios::app,  
      keywords::file_name = dir + "/%Y%m%d.log",  
      keywords::rotation_size = logSize,
      keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),   
      //keywords::format = "[%TimeStamp% %ThreadID% %LineID%]: %Message%"  
      keywords::format =  
      (  
        expr::stream  
        << "[" <<expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")  
        << " " << expr::attr< boost::log::aux::thread::id >("ThreadID")  
        << " " << logging::trivial::severity  
        << "] " << expr::smessage  
      )  
  );  
  // 如果不写这个 它不会实时的把日志写下去, 而是等待缓冲区满了,或者程序正常退出时写下,这样做的好处是减少IO操作.提高效率,  不过我这里不需要它. 因为我的程序可能会异常退出.  
  pSink->locked_backend()->auto_flush(true);//使日志实时更新  
  //pSink->imbue(std::locale("zh_CN.UTF-8")); // 本地化   
  logging::add_common_attributes();  
}  
void MyLog::Log(const string & msg )  
{  
  src::logger lg;  
  BOOST_LOG(lg) << msg;  
}  

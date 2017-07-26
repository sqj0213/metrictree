#pragma once  
      
#include <string>  
#include <boost/log/trivial.hpp>  
#include <boost/filesystem.hpp>  
  
#include <boost/log/sources/logger.hpp>  
#include <boost/log/sources/record_ostream.hpp>  
#include <boost/log/sources/global_logger_storage.hpp>  
#include <boost/log/utility/setup/file.hpp>  
#include <boost/log/utility/setup/common_attributes.hpp>  
#include <boost/log/sinks/text_ostream_backend.hpp>  
#include <boost/log/attributes/named_scope.hpp>  
#include <boost/log/expressions.hpp>  
#include <boost/log/support/date_time.hpp>  
#include <boost/log/detail/format.hpp>  
#include <boost/log/detail/thread_id.hpp>  

namespace logging = boost::log;  
namespace src = boost::log::sources;  
namespace keywords = boost::log::keywords;  
namespace sinks = boost::log::sinks;  
namespace expr = boost::log::expressions;  

using std::string;  

#define LOG_DEBUG\
	BOOST_LOG_SEV((MyLog::s_slg),(boost::log::trivial::debug))
#define LOG_INFO\
	BOOST_LOG_SEV((MyLog::s_slg),(boost::log::trivial::info))
#define LOG_ERROR\
	BOOST_LOG_SEV((MyLog::s_slg),(boost::log::trivial::error))
#define LOG_WARNING\
	BOOST_LOG_SEV((MyLog::s_slg),(boost::log::trivial::warning))
// 在使用之前必须先调用 init 
// 使用方式  LOG_DEBUG<<"test string"; 
// 也可以用boost 中的宏  BOOST_LOG_TRIVIAL(info)<<"test msg"; 
class MyLog  
{  
    public:  
      MyLog();  
      ~MyLog(void);  
       
      // 在使用之前必须先调用此函数  
      static void Init(const string & dir, unsigned long logSize);
      static void Log(const string & msg );
      static boost::log::sources::severity_logger<boost::log::trivial::severity_level > s_slg;  
    protected:  
    private:  
      
};  

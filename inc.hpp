/* 
 * File:   inc.hpp
 * Author: apple
 *
 * Created on 2015年10月20日, 下午5:49
 */

#ifndef INC_HPP
#define	INC_HPP

#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>
#include <boost/property_tree/ini_parser.hpp>  
#include <boost/property_tree/info_parser.hpp> 
#include <boost/property_tree/xml_parser.hpp> 
#include <boost/archive/binary_iarchive.hpp>  
#include <boost/thread/mutex.hpp>
#include <boost/property_tree/ptree_serialization.hpp>
#include <boost/archive/binary_oarchive.hpp> 
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/version.hpp>  
#include <boost/property_tree/json_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/typeof/typeof.hpp>
#include <errno.h>
#include <sys/file.h>
#include <list>
#include <sstream>
#include <map>
#include <stdexcept>

#include <sys/stat.h>
#include "log.hpp"

//全局日志变量

//#define DIRREGSTR "([a-zA-Z0-9\\_\\-\\(\\)]{4,200})\\.{4,10}"
#define DIRREGSTR "([a-zA-Z0-9\\(\\)-_]{3,200}\\.[a-zA-Z0-9\\(\\)-_]{3,200}){3,10}"
//写入文件的时间点多久后可读取文件
#define FILESTATSDELAYMIN 3
#define STORESECONDS 90 
//多长时间更新一次key的时间
#define KEY_EXPIRE_TIME 300
string metricTreeDataDir;
string storeLockFile;
string pidFile;
string nodeType;
FILE *pidFileHandler;

using namespace boost;  
using namespace std;  

boost::uint64_t recivedCount=0;
boost::uint64_t readCount=0;
boost::uint64_t dropCount=0;
boost::mutex mtx_;



struct status
{
        string nodeType;
	string over1sLastCMD;
	string over100kLastCMD;
        uint64_t recivedCount;
        uint64_t dropCount;
        uint64_t cmdFinderCount;
        uint64_t cmdEraseCount;
        uint64_t cmdFinderResultSize;

        uint64_t cmdDeepTreeCount;
        uint64_t cmdDeepTreeResultSize;
        uint64_t cmdTreeCount;
        uint64_t cmdTreeResultSize;

        uint64_t cmdExpireCount;
        uint64_t cmdExpireRequestSize;
        uint64_t cmdExpireResultSize;
        uint64_t cmdStoreCount;
        uint64_t totalCMDCount;
        uint64_t totalCMDOver1sCount;
        uint64_t totalCMDOver100kCount;
};
struct status daemonStatus={"","","",0,0,0,0,0,0,0,0,0,0,0,0};

off_t  preStoreFileSize=0;

MyLog *logHandler = new MyLog();

//三个四个字段的查询通过tree的get_child方法获得，四层以内的通过iter的方式进行查询
#define FIND_METHOD_SPLIT 3

bool stopFlag = false;
bool storeFlag=false;
bool swapFlag=false;
  
using namespace boost::property_tree;
//树
ptree myTree;
ptree myTreeSwap;
//树的索引
map <string, map<string, unsigned>>treeMap;

//此函数需要优化，需递归计算所有的子结点
void printTree ( const ptree &pt, string rootStr="" ) {
    string lineStr,tmpRootStr;
    map<string,unsigned>tmpMap;

    //树一层一层遍历
    for ( ptree::const_iterator iter = pt.begin(); iter != pt.end(); ++iter )
    {
        if ( rootStr.empty() )
            tmpRootStr = iter->first;
        else
        {
            tmpRootStr = rootStr + ".";
            tmpRootStr += iter->first;
        }
        if ( !treeMap[tmpRootStr]["subDir"] )
            treeMap[tmpRootStr].insert(make_pair("subDir", 0));
        if ( !treeMap[tmpRootStr]["subLeaf"] )
            treeMap[tmpRootStr].insert(make_pair("subLeaf", 0));

        //叶子结点
        if ( iter->second.data() != "" )
        {
            //leaf=leaf+boost::lexical_cast<unsigned>(iter->second.get_value<std::string>());

            treeMap[tmpRootStr]["val"] = iter->second.get_value<unsigned>();
            treeMap[tmpRootStr]["subLeaf"]++;
                cout<<tmpRootStr<<":"<<iter->second.data()<<std::endl;
        }
        //非叶子结点,需要递归调用
        else
        {
            printTree( iter->second, tmpRootStr );
        }
    }
}



//此函数需要优化，需递归计算所有的子结点
void outPutTree( ostringstream &os, const ptree &pt, unsigned tab=0, string rootStr="", bool indexFlag=false ) {
    tab += 2;
    string lineStr,tmpRootStr;
    map<string,unsigned>tmpMap;

    //树一层一层遍历
    for ( ptree::const_iterator iter = pt.begin(); iter != pt.end(); ++iter )
    {
        if ( !indexFlag )
        {
            os << string( tab, '-' );
            os << iter->first;
        }
        if ( rootStr.empty() )
            tmpRootStr = iter->first;
        else
        {
            tmpRootStr = rootStr + ".";
            tmpRootStr += iter->first;
        }
        if ( !treeMap[tmpRootStr]["subDir"] )
            treeMap[tmpRootStr].insert(make_pair("subDir", 0));
        if ( !treeMap[tmpRootStr]["subLeaf"] )
            treeMap[tmpRootStr].insert(make_pair("subLeaf", 0));
        
        //叶子结点
        if ( iter->second.data() != "" )
        {
            //leaf=leaf+boost::lexical_cast<unsigned>(iter->second.get_value<std::string>());
            
            treeMap[tmpRootStr]["val"] = iter->second.get_value<unsigned>();
            treeMap[tmpRootStr]["subLeaf"]++;
            if ( !indexFlag )
                os<<":"<<iter->second.data()<<std::endl;
        }
        //非叶子结点,需要递归调用
        else
        {
            if ( !indexFlag && iter != pt.end() )
                os<<std::endl;
            outPutTree( os, iter->second, tab, tmpRootStr, indexFlag );
            //下级节点数
            treeMap[tmpRootStr]["subDir"] = iter->second.size();
        }
    }
}

string outPutTreeIndex( ostringstream &os )
{
    for ( map<string,map<string,unsigned>>::const_iterator iter=treeMap.begin();iter != treeMap.end();++iter )
    {
        string dirKey = iter->first;
        map<string, unsigned>tmpMap = iter->second;
        unsigned dirNum = tmpMap["subDir"];
        unsigned leafNum = tmpMap["subLeaf"];
        os<<dirKey<<"\tdir:"<<dirNum<<"\tLeaf:"<<leafNum<<"\t:val:"<<tmpMap["val"]<<endl;
    }
    return os.str();
}
void replace(string& str,const char c, const string replaceStr )  
{  
	std::string::size_type startpos = 0;  
	while (startpos!= std::string::npos)  
	{  
    		startpos = str.find(c);   //找到'.'的位置  
    		if( startpos != std::string::npos ) //std::string::npos表示没有找到该字符  
      			str.replace(startpos,1,replaceStr); //实施替换，注意后面一定要用""引起来，表示字符串  
    	}  
} 
bool treeStatus( const string rootStr, const ptree &pt, unsigned long &subDirs, unsigned long &subFiles, time_t &lastUpdateTime, const time_t startTime, string &maxKey, unsigned long &maxCount, unsigned timeout=5, bool subFind=false )
{
    string lineStr,tmpRootStr;
    //树一层一层遍历
    unsigned long nodeCount=0;
    for ( ptree::const_iterator iter = pt.begin(); iter != pt.end(); ++iter )
    {
        if ( rootStr.empty() )
            tmpRootStr = iter->first;
        else
        {
            tmpRootStr = rootStr + ".";
            tmpRootStr += iter->first;
        }
    	const time_t t = time(NULL);
    	if ( t - startTime > timeout )
    		return false;
        //叶子结点
        if ( iter->second.data() != "" )
        {
            //leaf=leaf+boost::lexical_cast<unsigned>(iter->second.get_value<std::string>());
            time_t tmpTime = iter->second.get_value<time_t>();
            if ( tmpTime > lastUpdateTime )
            		lastUpdateTime = tmpTime;
        	subFiles++;
        }
        //非叶子结点,需要递归调用
        else
        {
            subDirs++;
        	if ( subFind )
        		treeStatus( tmpRootStr, iter->second,subDirs,subFiles,lastUpdateTime, startTime, maxKey,maxCount, timeout, subFind );
        }
	nodeCount++;
    }
    if ( nodeCount > maxCount )
    {
	maxKey = rootStr;
	maxCount = nodeCount;	
    }
    return true;
}





void locateByTree( const string path , ptree &treePos )
{
   if ( swapFlag )
        treePos=myTreeSwap.get_child(path,ptree());
   else
        treePos=myTree.get_child(path,ptree());
   return; 
}

void locateByIter( const ptree &pt, const string key, int currentLevel, int maxLevel, ptree &treePos )
{
        unsigned long pos=key.find(".");

        string callKey,searchKey;
        if ( pos != string::npos )
        {
                searchKey = key.substr( 0, pos );
                callKey = key.substr( pos + 1 );
        }
        else
        {
                searchKey = key;
        }
        if ( key == "" && maxLevel == 0 )
        {
		treePos = pt;
                return;
        }
        if ( currentLevel <= maxLevel )
        {
                for ( ptree::const_iterator iter = pt.begin(); iter != pt.end(); ++iter )
                {
                        if ( iter->first == searchKey )
                        {
                                //cout<<"l="<<__LINE__<<" iter->second.data():"<<iter->second.data()<<" callKey:"<<callKey<<endl;
                                return locateByIter( iter->second, callKey, currentLevel+1, maxLevel, treePos );
                        }
                }
        }
        else
        {
		treePos = pt;
                return;
        }

}











void tree( const string path, ostringstream &os, unsigned timeout=5, bool subFind=false )
{
	ptree tmpTree,retTree;
        int dotNum=0,i=0;
        int strLen=path.length();
        while ( path[i] && i < strLen )
        {
                 if ( path[i++] == '.' )
                          dotNum++;
        }
 BOOST_LOG_TRIVIAL(debug)<<" btime:"<<time(NULL)<<":"<<__LINE__<<"|dotNum:"<<dotNum<<"|FIND:"<<FIND_METHOD_SPLIT<<endl;
        if ( dotNum >= FIND_METHOD_SPLIT )
                locateByTree( path, tmpTree );
        else
        {
                if ( swapFlag )
                        locateByIter( myTreeSwap, path, 0, dotNum, tmpTree);
                else
                        locateByIter( myTree, path, 0, dotNum, tmpTree);
                //finderByTree( path, os );
        }
 BOOST_LOG_TRIVIAL(debug)<<" btime:"<<time(NULL)<<":"<<__LINE__<<endl;


	unsigned long subDirs=0,subFiles=0,dirs=0,files=0,maxCount=1;
	string maxKey;
	bool ret=false;
	time_t startTime = time( NULL );
	time_t lastUpdateTime = 0;
	ret = treeStatus( path, tmpTree, dirs, files, lastUpdateTime, startTime, maxKey,maxCount, timeout, false );
 BOOST_LOG_TRIVIAL(debug)<<" btime:"<<time(NULL)<<":"<<__LINE__<<endl;
	if ( subFind )
		ret = treeStatus( path, tmpTree, subDirs, subFiles, lastUpdateTime, startTime, maxKey,maxCount, timeout, subFind );
 BOOST_LOG_TRIVIAL(debug)<<" btime:"<<time(NULL)<<":"<<__LINE__<<endl;
	string key = path;
	replace( key, '.', "|" );
	retTree.put( key+"|subDirs", subDirs );
	retTree.put( key+"|subFiles", subFiles );
	retTree.put( key+"|lastUpdateTime", lastUpdateTime );
	retTree.put( key+"|dirs", dirs );
	retTree.put( key+"|files", files );
	if ( subFind )
	{
		replace( maxKey, '.', "|" );
		retTree.put("max:"+maxKey, maxCount);
	}
	
	if ( ret )
		retTree.put( key+"|errMsg", "success");
	else
		retTree.put( key+"|errMsg", "failed");
    json_parser::write_json( os, retTree );
    retTree.clear();
    return;
}
void expire( const string keyList, time_t fromTime, ostringstream &os )
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
void expireAll( const string keyList, time_t fromTime, ostringstream &os )
{
        ptree retTree;
        string::size_type  pos=0,startPos=0;
        string key;
        pos = keyList.find('|', pos );
        while ( true )
        {
                key = keyList.substr(startPos,pos-startPos);
                time_t timeValue=myTree.get<time_t>( key, 1 );
                if ( timeValue > fromTime )
                {
                        replace( key, '.', "|" );
                        retTree.put( key, timeValue );
                }
		else if ( timeValue ==  1 )
		{
                        replace( key, '.', "|" );
                        retTree.put( key, -1);
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
void erase( const string path, ostringstream &os )
{
	ptree tmpTree;
	string key,delKey;
	size_t startPos=0,pos=0;	
	size_t delCount=0;
	string rootPath="";
	pos=path.rfind( '.', path.size()) ;
	if ( pos != string::npos )
	{
		rootPath = path.substr( 0, pos );
		delKey=path.substr(pos+1);
	}
	else
	{
		rootPath = "";
		delKey = path;
	}

	if ( rootPath == "" )	
		delCount=myTree.erase( delKey);
	else
	{
		ptree &tmpTree = myTree.get_child( rootPath );
		delCount = tmpTree.erase( delKey);
	}

	key=path;
	replace( key, '.', "|" );
	if ( delCount > 0 )
		tmpTree.put( key, delCount );
	else
		tmpTree.put( key, 0 );
	json_parser::write_json( os, tmpTree );
	tmpTree.clear();
	return ;
}
void finderByTree( const string path , ostringstream &os )
{
   ptree retTree,tmpTree,tmpTree1;
   if ( swapFlag )
   	tmpTree=myTreeSwap.get_child(path,ptree());
   else
   	tmpTree=myTree.get_child(path,ptree());

   string tmpPath;
   if ( !tmpTree.empty() )
   {
        for ( ptree::const_iterator iter = tmpTree.begin(); iter != tmpTree.end(); ++iter )
	{
	       if ( path=="" )
                        tmpPath = iter->first;
                else
                        tmpPath = path +"."+iter->first;	
   		if ( swapFlag )
			tmpTree1 = myTreeSwap.get_child( tmpPath, ptree() );
		else
			tmpTree1 = myTree.get_child( tmpPath, ptree() );
		if ( tmpTree1.empty() )
	  		retTree.put( iter->first, 0 );	
		else
	  		retTree.put( iter->first, 1 );	
		tmpTree1.clear();
	}

   }
   tmpTree.clear();

    json_parser::write_json( os, retTree );
    retTree.clear();
}
void finderByIter( const ptree &pt, const string key, ostringstream &os, int currentLevel, int maxLevel, const ptree &root )
{
	unsigned long pos=key.find(".");	
	
	string callKey,searchKey;
	if ( pos != string::npos )
	{
		searchKey = key.substr( 0, pos );
		callKey = key.substr( pos + 1 );
	}
	else
	{
		searchKey = key;
	}
	if ( key == "" && maxLevel == 0 )
	{
		ptree retTree;
		for ( ptree::const_iterator iter = pt.begin(); iter != pt.end(); ++iter )
                {

 			//cout<<"debug key:"<<key<<" endkey:"<<key + "." + iter->second.data()<<endl;
		//	cout<<"l="<<__LINE__<<" key:"<<key<<" iter->first:"<<iter->first<<" iter->second.data():"<<iter->second.data()<<" callKey:"<<callKey<<endl;
                	string item1 = boost::lexical_cast<std::string>(iter->second.data());
                	if ( item1 == "" )
                        	retTree.put( iter->first, 1 );
			else
                        	retTree.put( iter->first, 0 );
                }
                json_parser::write_json( os, retTree );
		retTree.clear();
                return;
	}
	if ( currentLevel <= maxLevel )
	{
		ptree retTree;
		for ( ptree::const_iterator iter = pt.begin(); iter != pt.end(); ++iter )
		{
			if ( iter->first == searchKey )	
			{
				//cout<<"l="<<__LINE__<<" iter->second.data():"<<iter->second.data()<<" callKey:"<<callKey<<endl;
				return finderByIter( iter->second, callKey, os, currentLevel+1, maxLevel, root );
			}
		}
                json_parser::write_json( os, retTree );
		retTree.clear();
		return ;
	}
	else
	{
		ptree retTree;
                for ( ptree::const_iterator iter = pt.begin(); iter != pt.end(); ++iter )
                {
			
                	//auto item1 = root.get_child_optional( key+"."+iter->second.data() );
			//cout<<"l="<<__LINE__<<" iter->second.data():"<<iter->second.data()<<" callKey:"<<callKey<<endl;
        		//BOOST_LOG_TRIVIAL(info)<<" key:"<<key<<" iter->first:"<<iter->first<<" iter->second.data():"<<iter->second.data();
                	string item1 = boost::lexical_cast<std::string>(iter->second.data());
                	if ( item1 == "" )
                        	retTree.put( iter->first, 1 );
			else
                        	retTree.put( iter->first, 0 );
                }
		json_parser::write_json( os, retTree );
		retTree.clear();
		return;
	}
	
}
void finder( const string path, ostringstream &os )
{
        int dotNum=0,i=0;
        int strLen=path.length();
        while ( path[i] && i < strLen )
        {
                 if ( path[i++] == '.' )
                          dotNum++;
        }
        if ( dotNum >= FIND_METHOD_SPLIT )
                finderByTree( path, os );
        else
	{
		if ( swapFlag )
                	finderByIter( myTreeSwap, path, os, 0, dotNum, myTree );
		else
                	finderByIter( myTree, path, os, 0, dotNum, myTree );
                //finderByTree( path, os );
	}
}
string getCurrentDateHour()
{
        string startTime;
        const time_t t = time(NULL);
        struct tm* current_time = localtime(&t);
        startTime =  boost::lexical_cast<std::string>( current_time->tm_year+1900 ) + "-" + boost::lexical_cast<std::string>( current_time->tm_mon+1);
        startTime =  startTime + "-" + boost::lexical_cast<std::string>( current_time->tm_mday ) + "-" + boost::lexical_cast<std::string>( current_time->tm_hour);
        return startTime;
}
string getCurrentTime()
{
        string startTime;
	const time_t t = time(NULL);
        struct tm* current_time = localtime(&t);
        startTime =  boost::lexical_cast<std::string>( current_time->tm_year+1900 ) + "-" + boost::lexical_cast<std::string>( current_time->tm_mon+1);
        startTime =  startTime + "-" + boost::lexical_cast<std::string>( current_time->tm_mday ) + " " + boost::lexical_cast<std::string>( current_time->tm_hour);
        startTime =  startTime + ":" + boost::lexical_cast<std::string>( current_time->tm_min ) + ":" + boost::lexical_cast<std::string>( current_time->tm_sec );
        return startTime;
}
bool treeSave( const string filePath )
{
	string extName;
	size_t pos = filePath.find(".");
	if ( pos != string::npos )
		extName=filePath.substr( pos, 4 );
	if ( extName == ".ini" )
	{
		try
		{
			ini_parser::write_ini( filePath, myTree );
			BOOST_LOG_TRIVIAL(info)<<" cmd:treeSave filePath:"<<filePath<<" extName:"<<extName;
			return true;
		}
		catch ( info_parser_error &e )
		{
			BOOST_LOG_TRIVIAL(error)<<" cmd:treeSave failed! errMsg:"<<e.message();
			return false;
		}
		catch ( ini_parser::ini_parser_error &e )
		{
			BOOST_LOG_TRIVIAL(error)<<" cmd:treeSave failed! errMsg:"<<e.message();
			return false;
		}
	}
	else if ( extName == ".xml" )
	{
		try
		{
			xml_parser::write_xml( filePath, myTree );
			BOOST_LOG_TRIVIAL(info)<<" cmd:treeSave filePath:"<<filePath<<" extName:"<<extName;
			return true;
		}
		catch ( xml_parser::xml_parser_error &e )
		{
			BOOST_LOG_TRIVIAL(error)<<" cmd:treeSave failed! errMsg:"<<e.message();
			return false;
		}
	}
	else
	{
		try
		{
			ofstream file( filePath, std::ios::binary );
			boost::archive::binary_oarchive oa( file );
			property_tree::save( oa, myTree, 1 );
			file.close();
			BOOST_LOG_TRIVIAL(info)<<" cmd:treeSave filePath:"<<filePath<<" extName:"<<extName;
			return true;
		}
		catch ( bad_alloc )
		{
	        	BOOST_LOG_TRIVIAL(error)<<" cmd:treeSave failed!";
			return false;
		}
	}
}
bool treeLoad( const string filePath, ptree &_myTree )
{
        string extName;
        size_t pos = filePath.find(".");
        if ( pos != string::npos )
                extName=filePath.substr( pos );
        if ( extName == ".ini" )
        {
                try
                {
                    ini_parser::read_ini( filePath, _myTree );
			        return true;
                }
                catch ( info_parser_error &e )
                {
                    BOOST_LOG_TRIVIAL(error)<<" cmd:treeLoad ini info parser failed! filePath:"<<filePath<<" errMsg:"<<e.message();
			        _myTree.clear();
			        return false;
                }
                catch ( ini_parser::ini_parser_error &e )
                {
			        _myTree.clear();
                    BOOST_LOG_TRIVIAL(error)<<" cmd:treeLoad ini parser failed! filePath:"<<filePath<<" errMsg:"<<e.message();
			        return false;
                }
        }
        else if ( extName == ".xml" )
        {
                try
                {
                    xml_parser::read_xml( filePath, _myTree );
			        return true;
                }
                catch ( info_parser_error &e )
                {
                    BOOST_LOG_TRIVIAL(error)<<" cmd:treeLoad xml info parser failed! filePath:"<<filePath<<" errMsg:"<<e.message();
			        _myTree.clear();
			        return false;
                }
                catch ( xml_parser::xml_parser_error &e )
                {
			        _myTree.clear();
                    BOOST_LOG_TRIVIAL(error)<<" cmd:treeLoad xml parser failed! filePath:"<<filePath<<" errMsg:"<<e.message();
			        return false;
                }
        }
        else
        {
		    try
		    {
			    ifstream file( filePath, std::ios::binary);
			    boost::archive::binary_iarchive  ia( file );
			    property_tree::load( ia, _myTree, 1 );
			    file.close();
			    return true;
		    }
		    catch ( bad_alloc )
		    {
			    _myTree.clear();
                BOOST_LOG_TRIVIAL(error)<<" cmd:treeLoad failed! errMsg:bad_alloc";
			    return false;
		    }
	    }
}
void status( ostringstream &os )
{
	ptree retTree;
	if ( daemonStatus.nodeType.empty() )
		daemonStatus.nodeType=nodeType;
	retTree.put( "nodeType", daemonStatus.nodeType );
	retTree.put( "totalCMDCount", daemonStatus.totalCMDCount );
	retTree.put( "totalCMDOver1sCount", daemonStatus.totalCMDOver1sCount);
	if ( daemonStatus.over1sLastCMD.size() > 512)
		retTree.put( "over1sLastCMD", daemonStatus.over1sLastCMD.substr(0, 512)+"...");
	else
		retTree.put( "over1sLastCMD", daemonStatus.over1sLastCMD);
	retTree.put( "totalCMDOver100kCount", daemonStatus.totalCMDOver100kCount);
	if ( daemonStatus.over100kLastCMD.size() > 512 )
		retTree.put( "over100kLastCMD", daemonStatus.over100kLastCMD.substr(0, 512)+"...");
	else
		retTree.put( "over100kLastCMD", daemonStatus.over100kLastCMD);
	retTree.put( "recivedCount", daemonStatus.recivedCount );
	retTree.put( "dropCount", daemonStatus.dropCount );
	retTree.put( "cmdFinderCount", daemonStatus.cmdFinderCount );
	retTree.put( "cmdFinderResultSize", daemonStatus.cmdFinderResultSize);

	retTree.put( "cmdDeepTreeCount", daemonStatus.cmdDeepTreeCount );
	retTree.put( "cmdDeepTreeResultSize", daemonStatus.cmdDeepTreeResultSize);
	retTree.put( "cmdTreeCount", daemonStatus.cmdTreeCount );
	retTree.put( "cmdTreeResultSize", daemonStatus.cmdTreeResultSize);

	retTree.put( "cmdExpireCount", daemonStatus.cmdExpireCount );
	retTree.put( "cmdExpireRequestSize", daemonStatus.cmdExpireRequestSize );
	retTree.put( "cmdExpireResultSize", daemonStatus.cmdExpireResultSize );
	retTree.put( "cmdExpireCount", daemonStatus.cmdExpireCount );
	retTree.put( "cmdEraseCount", daemonStatus.cmdEraseCount );
	json_parser::write_json( os, retTree );
	return;
}
void runCmd( const string cmd, ostringstream &os )
{
/*
daemonStatus.recivedCount=0;
daemonStatus.dropCount=0;
daemonStatus.cmdFinderCount=0;
daemonStatus.cmdFinderResultSize=0;
daemonStatus.cmdExpireCount=0;
daemonStatus.cmdExpireRequestSize=0;
daemonStatus.cmdExpireResultSize=0;
daemonStatus.cmdStoreCount=0;
daemonStatus.totalCMDCount=0;
daemonStatus.totalCMDOver1sCount=0;
daemonStatus.totalCMDOver100kCount=0;
*/
    daemonStatus.totalCMDCount++;
    if ( cmd.compare("print") == 0 )
    {
        time_t start,end;
        start = time(NULL);
        string startTime;
        startTime = getCurrentTime();
        outPutTree( os, myTree );
	end = time( NULL );
        BOOST_LOG_TRIVIAL(info)<<" cmd:print end-start:"<<(end-start);
	//cout<<"["<<startTime<<"] cmd:print end-start:"<<(end-start)<<endl;
	return;
    }
    if ( cmd.compare("status") == 0 )
    {
	status( os );
	return;
	/*
        time_t start,end,end1;
        start = time(NULL);
        string startTime;
        startTime = getCurrentTime();

        outPutTree(os, myTree, 0, "", true  );
        end = time(NULL);
        BOOST_LOG_TRIVIAL(info)<<" cmd:status_index end-start:"<<(end-start)<<" success!";
        //cout<<"["<<startTime<<"] cmd:status_index end-start:"<<(end-start)<<" success!"<<endl;
        outPutTreeIndex( os );
        end1 = time(NULL);
        BOOST_LOG_TRIVIAL(info)<<" cmd:status_output end1-end:"<<(end1-end)<<" success!";
	return;
	*/
        //cout<<"["<<startTime<<"] cmd:status_output end1-end:"<<(end1-end)<<" success!"<<endl;
    }
    if ( cmd.compare("store") == 0  )
    {
        daemonStatus.cmdStoreCount++;
        time_t start,end;
	int ret;
	FILE *fileHandler;
        start = time(NULL);
        string startTime,tmpFileName;
        startTime = getCurrentTime();
	tmpFileName=metricTreeDataDir+"-"+getCurrentDateHour();
	//try
	//{
		//json_parser::write_json( tmpFileName, myTree);
		bool storeFlag=treeSave( tmpFileName );
		if ( storeFlag )
		{
		//获取写存储文件锁
                        while ( true )
                        {
                                fileHandler=fopen( storeLockFile.c_str(), "a+" );
                                if ( fileHandler != NULL )
                                {
                                        BOOST_LOG_TRIVIAL(info)<<" cmd:store-storeLockFile file:"<<storeLockFile<<" success!";
                                        ret = flock(fileno(fileHandler), LOCK_EX);
                                        if ( ret != 0 )
                                                BOOST_LOG_TRIVIAL(error)<<" cmd:store-storeLockFile-flock flock failed and sleep(1) flockerr:"<<strerror(errno);
                                        else
                                        {
                                                BOOST_LOG_TRIVIAL(debug)<<" cmd:reloadStoreThread-storeLockFile-flock success!";
                                                break;
                                        }
                                }
                                else
                                {
                                        BOOST_LOG_TRIVIAL(error)<<" cmd:store-storeLockFile-fopen storeLockFile:"<<storeLockFile<<" open failed and exit openerr:"<<strerror(errno);   
                                        exit(0);
                                }
                                sleep(1);
                        }
		
		ret = rename( tmpFileName.c_str(), metricTreeDataDir.c_str() );
		//写存储文件解锁
		if ( flock(fileno(fileHandler), LOCK_UN) != 0 )
		{
			BOOST_LOG_TRIVIAL(error)<<" cmd:store-storeLockFile-unlock  storeLockFile:"<<storeLockFile<<" unlock failed and exit! unlockerr:"<<strerror(errno);   
			exit(0);
		}
                else
			BOOST_LOG_TRIVIAL(error)<<" cmd:store-storeLockFile-unlock  storeLockFile:"<<storeLockFile<<" success!";
		fclose( fileHandler );
		
        	end = time(NULL);
		if ( ret == 0 )
		{
        		BOOST_LOG_TRIVIAL(info)<<" cmd:store-rename ("<<tmpFileName<<","<<metricTreeDataDir<<") ret="<<ret<<" end-start:"<<(end-start)<<" success!";
		}
		else
		{
        		BOOST_LOG_TRIVIAL(info)<<" cmd:store-rename ("<<tmpFileName<<","<<metricTreeDataDir<<") ret="<<ret<<"end-start:"<<(end-start)<<" failed!";
		}
		}
		else
		{
			end = time( NULL );
        		BOOST_LOG_TRIVIAL(error)<<" cmd:treeSave "<<tmpFileName<<" end-start:"<<(end-start)<<" failed!";
		}
	/*}
	catch ( json_parser::json_parser_error &je )
	{
        	end = time(NULL);
        	BOOST_LOG_TRIVIAL(error)<<" cmd:store errMsg:"<<je.message()<<" end-start:"<<(end-start)<<" file:"<<tmpFileName<<" failed!";
	}
	*/
	//cout<<"["<<startTime<<"] cmd:store end-start:"<<(end-start)<<" file:"<<metricTreeDataDir<<" success!"<<endl;
	return;
    }
    if ( cmd.find("store ") == 0  )
    {
        time_t start,end;
        start = time(NULL);
        string startTime;
        startTime = getCurrentTime();
        string  filePath = cmd.substr(cmd.rfind( ' ' )+1);
        BOOST_LOG_TRIVIAL(info)<<"  cmd:store file:"<<filePath;
	//cout<<"["<<getCurrentTime()<<"] cmd:store file:"<<filePath<<endl;
        if ( filePath.size() > 0 )
	{
	//	try 
	//	{
               		//json_parser::write_json( filePath, myTree);
			treeSave( filePath );
               		end = time(NULL);
               		BOOST_LOG_TRIVIAL(info)<<" cmd:store end-start:"<<(end-start)<<" file:"<<filePath<<" success!";
	//	}
	//	catch ( json_parser::json_parser_error &je )
	//	{
         //      		end = time(NULL);
        //		BOOST_LOG_TRIVIAL(error)<<" cmd:store errMsg:"<<je.message()<<" end-start:"<<(end-start)<<" file:"<<metricTreeDataDir<<" failed!";
	//	}
               //cout<<"["<<startTime<<"] cmd:store end-start:"<<(end-start)<<" file:"<<filePath<<" success!"<<endl;
	}
	else
               BOOST_LOG_TRIVIAL(info)<<" cmd:store file:"<<filePath<<" failed!";
	       //cout<<"["<<startTime<<"] cmd:store file:"<<filePath<<" failed!"<<endl;
	daemonStatus.cmdStoreCount++;
	return;
    }
    if ( cmd.compare("reset") == 0  )
    {
        return outPutTree( os, myTree );
        //myTree=ptree::empty_tree();
    }

    if ( cmd.find("finder ") != string::npos   )
    {
		time_t start,end;
		start=time(NULL);
		string  path = cmd.substr(cmd.rfind( ' ' )+1);
			finder( path, os  );
		end=time(NULL);
		daemonStatus.cmdFinderCount++;
		daemonStatus.cmdFinderResultSize+=os.str().size();
		if ( os.str().size() > 100*1024 )
		{
			daemonStatus.over100kLastCMD = cmd+" resultSize:"+boost::lexical_cast<string>(os.str().size());
			BOOST_LOG_TRIVIAL(warning)<<" cmd:"<<cmd<<" resultSize:"<<os.str().size()<<" result:"<<os.str().c_str();
			daemonStatus.totalCMDOver100kCount++;
		}
		if ( ( end-start ) >=1 )
		{
			daemonStatus.over1sLastCMD=cmd+" end-start:"+boost::lexical_cast<string>(end-start) ;
			BOOST_LOG_TRIVIAL(warning)<<" cmd:"<<cmd<<" over1s:"<<(end-start)<<" result:"<<os.str().c_str();
			daemonStatus.totalCMDOver1sCount++;
		}
		return;
    }
    if ( cmd.find("deepTree ") != string::npos   )
    {
		time_t start,end;
		start=time(NULL);
		string  path = cmd.substr(cmd.rfind( ' ' )+1);
			tree( path, os, 10, true );
		end=time(NULL);
		daemonStatus.cmdDeepTreeCount++;
		daemonStatus.cmdDeepTreeResultSize+=os.str().size();
		if ( os.str().size() > 100*1024 )
		{
			daemonStatus.over100kLastCMD = cmd+" resultSize:"+boost::lexical_cast<string>(os.str().size());
			BOOST_LOG_TRIVIAL(warning)<<" cmd:"<<cmd<<" resultSize:"<<os.str().size()<<" result:"<<os.str().c_str();
			daemonStatus.totalCMDOver100kCount++;
		}
		if ( ( end-start ) >=1 )
		{
			daemonStatus.over1sLastCMD=cmd+" end-start:"+boost::lexical_cast<string>(end-start) ;
			BOOST_LOG_TRIVIAL(warning)<<" cmd:"<<cmd<<" over1s:"<<(end-start)<<" result:"<<os.str().c_str();
			daemonStatus.totalCMDOver1sCount++;
		}
		return;
    }
    if ( cmd.find("tree ") != string::npos   )
    {
		time_t start,end;
		start=time(NULL);
		string  path = cmd.substr(cmd.rfind( ' ' )+1);
		tree( path, os, 10, false );
		end=time(NULL);
		daemonStatus.cmdTreeCount++;
		daemonStatus.cmdTreeResultSize+=os.str().size();
		if ( os.str().size() > 100*1024 )
		{
			daemonStatus.over100kLastCMD = cmd+" resultSize:"+boost::lexical_cast<string>(os.str().size());
			BOOST_LOG_TRIVIAL(warning)<<" cmd:"<<cmd<<" resultSize:"<<os.str().size()<<" result:"<<os.str().c_str();
			daemonStatus.totalCMDOver100kCount++;
		}
		if ( ( end-start ) >=1 )
		{
			daemonStatus.over1sLastCMD=cmd+" end-start:"+boost::lexical_cast<string>(end-start) ;
			BOOST_LOG_TRIVIAL(warning)<<" cmd:"<<cmd<<" over1s:"<<(end-start)<<" result:"<<os.str().c_str();
			daemonStatus.totalCMDOver1sCount++;
		}
		return;
    }


    if ( cmd.find("erase ") != string::npos   )
    {
        time_t start,end;
        start=time(NULL);
        string  path = cmd.substr(cmd.rfind( ' ' )+1);
        erase( path, os  );
        end=time(NULL);
        daemonStatus.cmdEraseCount++;
        if ( ( end-start ) >=1 )
        {
                BOOST_LOG_TRIVIAL(warning)<<" cmd:"<<cmd<<" over1s:"<<(end-start)<<" result:"<<os.str().c_str();
                daemonStatus.totalCMDOver1sCount++;
        }
        return;
    }
    if ( cmd.find("expire ") != string::npos   )
    {
	//+boost::lexical_cast<unsigned>(iter->second.get_value<std::string>());	
	time_t start,end;
	start=time(NULL);
	string::size_type  pos,startPos;
	string keyStrList;
	time_t fromTime;
	pos = cmd.rfind(':');
	if ( pos != string::npos )
	{
		fromTime=boost::lexical_cast<time_t>(cmd.substr( pos+1 ));
		startPos=cmd.rfind( ' ' )+1;
        	keyStrList = cmd.substr(startPos, pos-startPos );
	}
	else
	{
		fromTime=time( NULL ) - 60*60;
        	keyStrList = cmd.substr(cmd.rfind( ' ' )+1);
	}
        expire( keyStrList, fromTime, os );
	end=time(NULL);
	daemonStatus.cmdExpireCount++;
	daemonStatus.cmdExpireRequestSize+=keyStrList.size();
	daemonStatus.cmdFinderResultSize+=os.str().size();
	if ( os.str().size() > 100*1024 )
	{
		daemonStatus.over100kLastCMD = cmd+" resultSize:"+boost::lexical_cast<string>(os.str().size());
		BOOST_LOG_TRIVIAL(warning)<<" cmd:"<<cmd<<" resultSize:"<<os.str().size()<<" result:"<<os.str().c_str();
		daemonStatus.totalCMDOver100kCount++;
	}
	if ( ( end-start ) >=1 )
	{
		daemonStatus.over1sLastCMD=cmd+" end-start:"+boost::lexical_cast<string>(end-start);
		BOOST_LOG_TRIVIAL(warning)<<" cmd:"<<cmd<<" over1s:"<<(end-start)<<" result:"<<os.str().c_str();
		daemonStatus.totalCMDOver1sCount++;
	}
	return;
	
    }
    if ( cmd.find("expireAll ") != string::npos   )
    {
        //+boost::lexical_cast<unsigned>(iter->second.get_value<std::string>());
        time_t start,end;
        start=time(NULL);
        string::size_type  pos,startPos;
        string keyStrList;
        time_t fromTime;
        pos = cmd.rfind(':');
        if ( pos != string::npos )
        {
                fromTime=boost::lexical_cast<time_t>(cmd.substr( pos+1 ));
                startPos=cmd.rfind( ' ' )+1;
                keyStrList = cmd.substr(startPos, pos-startPos );
        }
        else
        {
                fromTime=time( NULL ) - 60*60;
                keyStrList = cmd.substr(cmd.rfind( ' ' )+1);
        }
        expireAll( keyStrList, fromTime, os );
        end=time(NULL);
        daemonStatus.cmdExpireCount++;
        daemonStatus.cmdExpireRequestSize+=keyStrList.size();
        daemonStatus.cmdFinderResultSize+=os.str().size();
        if ( os.str().size() > 100*1024 )
        {
                daemonStatus.over100kLastCMD = cmd+" resultSize:"+boost::lexical_cast<string>(os.str().size());
                BOOST_LOG_TRIVIAL(warning)<<" cmd:"<<cmd<<" resultSize:"<<os.str().size()<<" result:"<<os.str().c_str();
                daemonStatus.totalCMDOver100kCount++;
        }
        if ( ( end-start ) >=1 )
        {
                daemonStatus.over1sLastCMD=cmd+" end-start:"+boost::lexical_cast<string>(end-start);
                BOOST_LOG_TRIVIAL(warning)<<" cmd:"<<cmd<<" over1s:"<<(end-start)<<" result:"<<os.str().c_str();
                daemonStatus.totalCMDOver1sCount++;
        }
        return;

    }

    if ( cmd.compare("load") == 0  )
    {
        time_t start,end;
        start = time(NULL);
        string startTime;
        startTime = getCurrentTime();
	//try 
	//{
 		treeLoad( metricTreeDataDir, myTree );
		end = time( NULL );
        	BOOST_LOG_TRIVIAL(info)<<" cmd:load end-start:"<<(end-start)<<" file:"<<metricTreeDataDir;
	//}
	//catch ( json_parser::json_parser_error &je )
	//{
        //       	end = time(NULL);
       // 	BOOST_LOG_TRIVIAL(error)<<" cmd:store errMsg:"<<je.message()<<" end-start:"<<(end-start)<<" file:"<<metricTreeDataDir<<" failed!";
	//}
	
	//cout<<"["<<startTime<<"] cmd:load end-start:"<<(end-start)<<" file:"<<metricTreeDataDir<<endl;
    }
    return;
}
bool checkStoreFile( )
{
	struct stat buf;	
	ptree tmp;
 	int ret=stat( metricTreeDataDir.c_str(),  &buf ); 
	if ( ret == ENOENT )
	{
		if ( nodeType=="write" )
		{
			//json_parser::write_json( metricTreeDataDir, tmp );	
			ofstream file( metricTreeDataDir, ios::app|ios::binary);
			BOOST_LOG_TRIVIAL(error)<<" cmd:checkStoreFile store file:"<<metricTreeDataDir<<" is not exists and build empty file!";
			file.close();
			return true;
		}
		else
		{
			BOOST_LOG_TRIVIAL(error)<<" cmd:checkStoreFile store file:"<<metricTreeDataDir<<" is not exists!";
			exit(0);
		}
	}
	else
	{
		if ( S_ISDIR( buf.st_mode ) )
		{
			BOOST_LOG_TRIVIAL(error)<<" cmd:checkStoreFile store file:"<<metricTreeDataDir<<" is invalid!";
			exit(0);
		}
	}
	return true;
}
void load( ostringstream &os )
{
        time_t start,end;
        start = time(NULL);
        string startTime;
        startTime = getCurrentTime();
	checkStoreFile();
		
//	try
//	{
		//json_parser::read_json( metricTreeDataDir, myTree );
		treeLoad( metricTreeDataDir, myTree );
		end = time( NULL );
		os<<"["<<startTime<<"] cmd:load end-start:"<<(end-start)<<" file:"<<metricTreeDataDir<<endl;
//	}
 //       catch ( json_parser::json_parser_error &je )
  //      {
   //             end = time(NULL);
    //            BOOST_LOG_TRIVIAL(error)<<" cmd:store errMsg:"<<je.message()<<" end-start:"<<(end-start)<<" file:"<<metricTreeDataDir<<" failed!";
     //   }
}
bool reloadStore( ostringstream &os, ptree &dirTree )
{
        time_t start,end;
        start = time(NULL);
        string startTime;
        startTime = getCurrentTime();
	//try
	//{
        //	json_parser::read_json( metricTreeDataDir, dirTree );
		bool loadFlag=treeLoad( metricTreeDataDir, dirTree );
       		end = time( NULL );
        	os<<"["<<startTime<<"] cmd:reloadStore end-start:"<<(end-start)<<" file:"<<metricTreeDataDir<<endl;
		return loadFlag;
	//}
        //catch ( json_parser::json_parser_error &je )
        //{
         //       end = time(NULL);
          //      BOOST_LOG_TRIVIAL(error)<<" cmd:store errMsg:"<<je.message()<<" end-start:"<<(end-start)<<" file:"<<metricTreeDataDir<<" failed!";
	//	return false;
        //}
}
void reloadStoreThread( unsigned reloadInterval=600 )
{
        time_t start, end, end1;
	int ret;
	FILE *fileHandler,*fileHandler1;
        string startTime, startTime1;
 	struct stat buf;
	ostringstream os;
	end1=time(NULL);
	ptree tmpTree;
	while ( 1==1 )
	{
		//进程外已经load，先sleep
		sleep(reloadInterval);
                start = time(NULL);
                startTime = getCurrentTime();
                if ( stopFlag )
                {
			stopFlag=false;
			end=time(NULL);
        		BOOST_LOG_TRIVIAL(info)<<" cmd:stopReloadStoreThread myTree.size:"<<myTree.size()<<" readCount:"<<readCount<<" end1-end:"<<(end-end1);
			break;
		}
		stat( metricTreeDataDir.c_str(),  &buf );
                startTime1 = getCurrentTime();
		while ( true )
		{
			if ( time(NULL) - buf.st_mtime >= FILESTATSDELAYMIN )
				break;
			else
	                        BOOST_LOG_TRIVIAL(info)<<" cmd:reloadStoreThread store file buf.st_mtime("<<buf.st_mtime<<")<FILESTATSDELAYMIN("<<FILESTATSDELAYMIN<<") and sleep(1);";
			sleep( 1 );
		}
		if ( buf.st_size != preStoreFileSize )
		{
                        //获取写存储文件锁,读写进程部署到一台机器时可用使用此文件锁
                        while ( true )
                        {
                                fileHandler=fopen( storeLockFile.c_str(), "a+" );
                                if ( fileHandler != NULL )
                                {
                                        ret = flock(fileno(fileHandler), LOCK_EX);
                                        if ( ret != 0 )
                                                BOOST_LOG_TRIVIAL(error)<<"cmd:reloadStoreThread-flock flock failed and sleep(1) flockerr:"<<strerror(errno);
                                        else
                                        {
                                        	BOOST_LOG_TRIVIAL(debug)<<"cmd:reloadStoreThread-flock file:"<<storeLockFile<<" success!";
                                                break;
                                        }
                                }
                                else
                                {
                                        BOOST_LOG_TRIVIAL(error)<<"cmd:store-reloadStoreThread-fopen storeLockFile:"<<storeLockFile<<" open failed and exit openerr:"<<strerror(errno);
                                        exit(0);
                                }
                                sleep(1);
                        }

			//获取写存储文件锁,避免外部进程写文件,导致读错误
                	while ( true )
                	{
                        	fileHandler1=fopen( metricTreeDataDir.c_str(), "rb+" );
                        	if ( fileHandler1 != NULL )
                        	{
                                	ret = flock(fileno(fileHandler), LOCK_EX);
                                	if ( ret != 0 )
                                        	BOOST_LOG_TRIVIAL(error)<<"cmd:reloadStoreThread-flock flock failed and sleep(1) flockerr:"<<strerror(errno);
                                	else
                                	{
                                        	BOOST_LOG_TRIVIAL(debug)<<"cmd:reloadStoreThread-flock file:"<<metricTreeDataDir<<" success!";
                                        	break;
                                	}
                        	}
                        	else
                        	{
                                	BOOST_LOG_TRIVIAL(error)<<"cmd:store-reloadStoreThread-fopen metricTreeData:"<<metricTreeDataDir<<" open failed and exit openerr:"<<strerror(errno);    
					exit(0);
                        	}
				sleep(1);
                	}
        		BOOST_LOG_TRIVIAL(info)<<" cmd:reloadStore begin.....";
			bool reloadFlag = reloadStore(os,myTreeSwap);
        		BOOST_LOG_TRIVIAL(info)<<" cmd:reloadStore end.....";
                        if ( flock(fileno(fileHandler1), LOCK_UN) != 0 )
                        {
                                BOOST_LOG_TRIVIAL(error)<<"cmd:store-reloadStoreThread-unlock  metricTreeDataDir:"<<metricTreeDataDir<<" unlock failed and exit! unlockerr:"<<strerror(errno);
                                exit(0);
                        }
			else
				BOOST_LOG_TRIVIAL(debug)<<"cmd:store-reloadStoreThread-unlock  metricTreeDataDir:"<<metricTreeDataDir<<" unlock success!";
                        fclose(fileHandler1);
			//释放写文件锁
			if ( flock(fileno(fileHandler), LOCK_UN) != 0 )
			{
                        	BOOST_LOG_TRIVIAL(error)<<"cmd:store-reloadStoreThread-unlock storeLockFile:"<<storeLockFile<<" unlock failed and exit! unlockerr:"<<strerror(errno);
                        	exit(0);
			}
			else
				 BOOST_LOG_TRIVIAL(debug)<<"cmd:store-reloadStoreThread-unlock storeLockFile:"<<storeLockFile<<" unlock success!";
			fclose(fileHandler);
			if ( reloadFlag )
			{
				swapFlag=true;
				myTree.clear();
				myTree=myTreeSwap;
				swapFlag=false;
			}
			myTreeSwap.clear();
			
                	end = time(NULL);
        		BOOST_LOG_TRIVIAL(info)<<os.str();
			os.str("");
        		BOOST_LOG_TRIVIAL(info)<<" cmd:reloadStore end.....";
			preStoreFileSize = buf.st_size;
		}
		else 
			 BOOST_LOG_TRIVIAL(warning)<<" "<<metricTreeDataDir<<" is not change buf.st_size<preStoreFileSize:"<<buf.st_size<<"<"<<preStoreFileSize; 
                end = time(NULL);
		if ( readCount > 99 )
		{
        		BOOST_LOG_TRIVIAL(info)<<" cmd:reloadStoreThread myTree.size:"<<myTree.size()<<" readCount:"<<readCount<<" end-end1:"<<(end-end1);
			readCount=0;
			end1=end;
		 }
                if ( stopFlag )
                {
			stopFlag=false;
        		BOOST_LOG_TRIVIAL(info)<<" cmd:stopReloadThread end-start:"<<(end-start); 
			break;
		}
	}
}
bool stopApp()
{

	int i=0;
        while ( true )
        {
                if ( stopFlag == false )
		{
			if ( nodeType == "write" )
			{
				ostringstream os;
				BOOST_LOG_TRIVIAL(info)<<" cmd:stopApp and store tree begin...";
				runCmd( "store", os );
				BOOST_LOG_TRIVIAL(info)<<" cmd:stopApp and store tree end... msg:"<<os.str();
				os.str("");
			}
			
			while( i++ < 5)
			{
				/*
				if ( flock( fileno( pidFileHandler ), LOCK_UN ) != 0 )
				{
					BOOST_LOG_TRIVIAL(error)<<" cmd:stopApp unlock pidFile("<<pidFile<<") failed and retry(i="<<i<<")!" ;
					continue;
				}
				fclose( pidFileHandler );
				if ( remove( pidFile.c_str() ) != 0 )
					BOOST_LOG_TRIVIAL(error)<<" cmd:stopApp delete pidFile("<<pidFile<<") failed and retry(i="<<i<<")!" ;
				else
				{
					BOOST_LOG_TRIVIAL(info)<<" cmd:stopApp pidFile("<<pidFile<<") is delete,stop and exit! recivedCount:"<<recivedCount;
					break;
				}
				sleep(1);
				*/
				break;
			}
			exit(0);
		}
                sleep(1);
        }
        return true;
}
void storeThread( unsigned interval=60 )
{
	ostringstream os;
	time_t start,end,end1,end2; 
	end1 = time(NULL); 
	while( true )
	{
		//第一次运行时已经是最新数据无需再存储
		sleep( interval );
		start = time(NULL); 
		string startTime,startTime1;
		startTime = getCurrentTime();
		if ( stopFlag )
		{
			end = time(NULL); 
			BOOST_LOG_TRIVIAL(info)<<" cmd:stop-storeThread recivedCount:"<<recivedCount<<" end1-end:"<<(end-end1);
			//cout<<"["<<startTime<<"] cmd:stop-storeThread recivedCount:"<<recivedCount<<" end1-end:"<<(end-end1)<<endl;
			stopFlag=false;
			break;
		}
		if ( storeFlag == false )
		{
			storeFlag=true;
			startTime = getCurrentTime();
			runCmd( "store", os );
			storeFlag=false;
			end2=time(NULL);
			BOOST_LOG_TRIVIAL(info)<<" cmd:stop-storeThread recivedCount:"<<recivedCount<<" dropCount:"<<dropCount<<" end2-start:"<<(end2-start);
			//cout<<"["<<startTime<<"] cmd:stop-storeThread recivedCount:"<<recivedCount<<" dropCount:"<<dropCount<<" end2-start:"<<(end2-start)<<endl;
			os.str("");
			dropCount=0;
		}
			
		end = time( NULL );
                //10万条数据写一条日志，便于计数
                if ( recivedCount > 99999 )
                {
		            	BOOST_LOG_TRIVIAL(info)<<" cmd:stop-storeThread myTree.size:"<<myTree.size()<<" recivedCount:"<<recivedCount<<" end1-end:"<<(end-end1);
                        //cout<<"["<<startTime<<"] cmd:stop-storeThread myTree.size:"<<myTree.size()<<" recivedCount:"<<recivedCount<<" end1-end:"<<(end-end1)<<endl;
                        recivedCount=0;
                        end1=end;
                }
		BOOST_LOG_TRIVIAL(info)<<" cmd:store "<<"start-end:"<<(end-start)<<" file:"<<metricTreeDataDir;
		//cout<<"["<<startTime<<"] cmd:store "<<"start-end:"<<(end-start)<<" file:"<<metricTreeDataDir<<endl;
		if ( stopFlag )
		{
			stopFlag=false;
			BOOST_LOG_TRIVIAL(info)<<" cmd:stop-storeThread";
			//cout<<"["<<startTime<<"] cmd:stop-storeThread"<<endl;
			break;
		}
	}
}
void buildTree( const string key )
{

    //校验放到最外层去实现
    //regex reg( DIRREGSTR );
    //if ( regex_match( path, reg ) )
    //{
	recivedCount+=1;
	daemonStatus.recivedCount++;
	if ( storeFlag == false )
	{
			time_t timeValue = time( NULL );
            		time_t  nodeValue = myTree.get<time_t>(key, 0 );
            		if ( timeValue - nodeValue >  KEY_EXPIRE_TIME )
			{
				mtx_.lock();
//                BOOST_LOG_TRIVIAL(info)<<" key:'"<<key<<"':timeValue:"<<timeValue;
                myTree.put( key, timeValue );
				mtx_.unlock();
			}
	}
	else
	{
		daemonStatus.dropCount++;
		dropCount++;
	}
    //}
    //else
    //    std::cout<<"dir:("<<path<<") reg("<<DIRREGSTR<<") is invalid"<<endl;
}
/*
[main]
threadCount=32
cmdport=7618
port=6618
dataDir=/data2/metric-tree/metric-tree.json
daemon=true
storeLockFile=/tmp/metric-tree-6618.lock
pidFile=/tmp/metric-tree-6618.pid
flushInterval=600
nodeType=write
readNode=0
[log]
logSize=50000000
logPath=/data2/metric-tree/logs/6618/
*/
bool checkConfig( ptree tmpTree )
{
	auto node = tmpTree.get_child_optional("main.threadCount");
	if ( !node )
        {
		cout<<"config item:main.threadCount not exists!"<<endl;
		return false;
	}
	node = tmpTree.get_child_optional("main.nodeType");
	if ( !node )
        {
		cout<<"config item:main.nodeType not exists!"<<endl;
		return false;
	}
	node = tmpTree.get_child_optional("main.cmdport");
	if ( !node )
        {
		cout<<"config item:main.cmdport not exists!"<<endl;
		return false;
	}
	node = tmpTree.get_child_optional("main.port");
	if ( !node )
        {
		cout<<"config item:main.port not exists!"<<endl;
		return false;
	}
	node = tmpTree.get_child_optional("main.dataDir");
	if ( !node )
        {
		cout<<"config item:main.dataDir not exists!"<<endl;
		return false;
	}
	node = tmpTree.get_child_optional("main.daemon");
	if ( !node )
        {
		cout<<"config item:main.daemon not exists!"<<endl;
		return false;
	}
	node = tmpTree.get_child_optional("main.storeLockFile");
	if ( !node )
        {
		cout<<"config item:main.storeLockFile not exists!"<<endl;
		return false;
	}
	node = tmpTree.get_child_optional("main.pidFile");
	if ( !node )
        {
		cout<<"config item:main.pidFile not exists!"<<endl;
		return false;
	}
	node = tmpTree.get_child_optional("main.flushInterval");
	if ( !node )
        {
		cout<<"config item:main.flushInterval not exists!"<<endl;
		return false;
	}
	node = tmpTree.get_child_optional("log.logSize");
	if ( !node )
        {
		cout<<"config item:log.logSize not exists!"<<endl;
		return false;
	}
	node = tmpTree.get_child_optional("log.logPath");
	if ( !node )
        {
		cout<<"config item:log.logPath not exists!"<<endl;
		return false;
	}
	return true;

}
#endif	/* INC_HPP */

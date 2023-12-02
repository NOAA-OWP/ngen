#ifndef NGEN_FILE_STREAM_HANDLER_HPP
#define NGEN_FILE_STREAM_HANDLER_HPP

#include "StreamHandler.hpp"

namespace utils
{
    class FileStreamHandler : public StreamHandler
    {
      public:
            FileStreamHandler(const char* path) : StreamHandler()
            {
                auto stream = std::make_shared<std::ofstream>();
                stream->open(path, std::ios::trunc);
                output_stream = stream;
            }
            virtual ~FileStreamHandler(){}
    };

    // class FileStreamHandler : public StreamHandler
    // {
    //   //FIXME!!!!!!!!! hack to prevent too many open files---also makes the output's empty :confused:
    //   public:
    //         FileStreamHandler(const char* path) : StreamHandler(), path(path)
    //         {
    //             stream = std::make_shared<std::ofstream>();
    //             stream->open(path, std::ios::trunc); //clear any existing data
    //             stream->close(); //avoid too many open files
    //             output_stream = stream;
    //         }
    //         virtual ~FileStreamHandler(){
    //             if ( stream->is_open() ) stream->close();
    //         }

    //         template<class DataType> std::ostream& operator<<(const DataType& val)
    //         {
    //             if( !stream->is_open() ) 
    //                 stream->open(path, std::ios_base::app);
    //             put(val);
    //             stream->close();
    //             return *output_stream;
    //         }
    //   private:
    //     std::string path;
    //     std::shared_ptr<std::ofstream> stream;
    // };

}



#endif // NGEN_FILE_STREAM_HANDLER_HPP

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

}



#endif // NGEN_FILE_STREAM_HANDLER_HPP

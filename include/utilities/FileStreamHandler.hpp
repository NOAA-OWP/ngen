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

            virtual ~FileStreamHandler() override {
                auto tmp = std::static_pointer_cast<std::ofstream>(output_stream);
                if(tmp){
                    tmp->close();
                }
            }
    };

}



#endif // NGEN_FILE_STREAM_HANDLER_HPP

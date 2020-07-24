#ifndef NGEN_FILE_STREAM_HANDLER_HPP
#define NGEN_FILE_STREAM_HANDLER_HPP

#include "StreamHandler.hpp""

namespace utils
{
    class FileStreamHandler() : public StreamHandler
    {
            FileStreamHandler(const char*) : StreamHandler()
            {
                output_stream = make_shared(new(std::fstream(path)));
            }
    };

}



#endif // NGEN_FILE_STREAM_HANDLER_HPP

#ifndef NGEN_NULL_STREAM_HPP_
#define NGEN_NULL_STREAM_HPP_

#include <boost/iostreams/device/null.hpp>
#include <boost/iostreams/stream.hpp>

namespace utils {
    class NullStream : public boost::iostreams::stream<boost::iostreams::null_sink> {
    public:
        NullStream() : boost::iostreams::stream<boost::iostreams::null_sink>(boost::iostreams::null_sink()) {}
    };
}

#endif // NGEN_NULL_STREAM_HPP_

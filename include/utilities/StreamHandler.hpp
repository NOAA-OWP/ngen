#ifndef NGEN_STREAM_HANDLER_HPP_
#define NGEN_STREAM_HANDLER_HPP_

#include <fstream>
#include <ostream>
#include <iostream>
#include <memory>

namespace utils
{
    /** This class provides a copyable interface to a std::ostream or std::ostream like object that may not support being copied. */

    class StreamHandler
    {
        public:

            /** Default constructor intended for use by containers that need an empty constructor */

            StreamHandler() : output_stream(std::shared_ptr<std::ostream>()), sep(", ")
            {
            }

            /** Create a Stream Handler given a pointer to a ostream object and the seperator to be used in serialization */

            StreamHandler(std::shared_ptr<std::ostream> s) : sep(", ")
            {
                output_stream = s;
            }

            /** Copy constructor for a StreamHandler */

            StreamHandler(StreamHandler& src) : output_stream(src.output_stream), sep(src.sep)
            {}

            /** Move constructor for a StreamHandler */

            StreamHandler(StreamHandler&& src) : output_stream(src.output_stream), sep(src.sep)
            {}

            /** Deconstructor for a StreamHandler */

            virtual ~StreamHandler()
            {

            }

            /** Move Assignment operator for a StreamHandler */
            StreamHandler& operator=(StreamHandler&& other) noexcept
            {
              if( this != &other)
              {
                output_stream = std::move(other.output_stream);
                sep = std::move(other.sep);
              }
              return  *this;
            }
            /** Serialize data onto the stored stream. This function does not preform any formating. */

            template<class DataType> void put(const DataType& val)
            {
                if ( output_stream != nullptr)
                {
                    (*output_stream) << val;
                }
            }

            /** Serialize data onto the stream in the form of "<index><sep><val>\n". */

            template<class IndexType, class DataType> void put_indexed(IndexType idx, const DataType& val)
            {
                if ( output_stream != nullptr )
                {
                    (*output_stream) << idx << sep << val << std::endl;
                }
            }

            /** Serialize data onto the stream in the form of "<index><sep><var><sep><val>\n". */

            template<class IndexType, class DataType> void put_var(IndexType idx, std::string var, const DataType& val)
            {
                if ( output_stream != nullptr)
                {
                    (*output_stream) << idx << sep << var << sep << val;
                }
            }

            /** stream write operator that allows a StreamHandler to be used as a stream object */

            template<class DataType> std::ostream& operator<<(const DataType& val)
            {
                put(val);
                return *output_stream;
            }

        protected:

        std::shared_ptr<std::ostream> output_stream;    /**< The shared pointer to the managed stream object*/
        std::string sep;                                /**< The seperator string to be used in serialization */


    };

    static StreamHandler getStdOut() {    
        std::ostream* raw_pointer = &std::cout; 
        std::shared_ptr<std::ostream> stream_pointer(raw_pointer, [](void*) {});
        return utils::StreamHandler(stream_pointer);
    }

    static StreamHandler getStdErr() {    
        std::ostream* raw_pointer = &std::cerr; 
        std::shared_ptr<std::ostream> stream_pointer(raw_pointer, [](void*) {});
        return utils::StreamHandler(stream_pointer);
    }
}


#endif // NGEN_STREAM_HANDLER_HPP_

#ifndef __OUTPUT_WRITER_HPP__
#define __OUTPUT_WRITER_HPP__

namespace data_output
{
    class OutputWriter
    {
        public:

        OutputWriter() {}
        virtual ~OutputWriter() {}

        virtual bool is_open() = 0;

        private:
    };
}

#endif
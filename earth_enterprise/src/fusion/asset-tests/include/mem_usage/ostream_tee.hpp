#ifndef _MEM_USAGE_OSTREAM_TEE_HPP
#define _MEM_USAGE_OSTREAM_TEE_HPP

#include <fstream>
#include <initializer_list>
#include <list>
#include <ostream>
#include <streambuf>
#include <system_error>
#include <vector>


namespace opengee {

namespace mem_usage {

// From <http://wordaligned.org/articles/cpp-streambufs>:
class StreamBufTee : public std::streambuf
{
    private:

    std::vector<std::streambuf*> output_bufs;

    public:

    // Construct a streambuf which tees output to all given streambufs.
    template <class InputIterator>
    StreamBufTee(
        InputIterator output_bufs_first, InputIterator output_bufs_last)
    : output_bufs(output_bufs_first, output_bufs_last)
    {}

    StreamBufTee()
    {}

    template <class InputIterator>
    void set_output_bufs(
        InputIterator output_bufs_first, InputIterator output_bufs_last)
    {
        output_bufs.assign(output_bufs_first, output_bufs_last);        
    }

    void add_output_buf(std::streambuf* value)
    {
        output_bufs.push_back(value);
    }

    // This tee buffer has no buffer. So every character "overflows"
    // and can be put directly into the teed buffers.
    virtual int overflow(int c)
    {
        if (c == EOF)
        {
            return !EOF;
        }
        else
        {
            int result = c;

            for (auto &output_buf : output_bufs)
            {
                if (output_buf->sputc(c) == EOF)
                {
                    result = EOF;
                }
            }

            return result;
        }
    }
    
    // Sync teed buffers.
    virtual int sync()
    {
        int result = -1;

        for (auto &output_buf : output_bufs)
        {
            if (output_buf->pubsync() == 0)
            {
                result = 0;
            }
        }

        return result;
    }   
};

class OStreamTee : public std::ostream
{
    private:

    StreamBufTee stream_buf;
    std::list<std::shared_ptr<std::ostream>> shared_outut_streams;

    public:

    template <class InputIterator>
    OStreamTee(
        InputIterator output_streams_first, InputIterator output_streams_last)
    : std::ostream(&stream_buf)
    {
        std::vector<std::shared_ptr<std::streambuf>> output_bufs_tmp;

        output_bufs_tmp.reserve(
            output_streams_last - output_streams_first);
        for (InputIterator stream = output_streams_first;
            stream != output_streams_last;
            ++stream)
        {
            output_bufs_tmp.emplace_back(stream->rdbuf());
        }
        stream_buf.set_output_bufs(
            output_bufs_tmp.begin(), output_bufs_tmp.end());
    }

    OStreamTee(std::initializer_list<std::ostream*> output_streams)
    : std::ostream(&stream_buf)
    {
        std::vector<std::streambuf*> output_bufs_tmp;

        output_bufs_tmp.reserve(output_streams.size());
        for (const auto stream : output_streams)
        {
            output_bufs_tmp.emplace_back(stream->rdbuf());
        }
        stream_buf.set_output_bufs(
            output_bufs_tmp.begin(), output_bufs_tmp.end());
    }

    OStreamTee()
    {}

    void add_output_path(std::string path)
    {
        auto output_file_ptr = std::make_shared<std::ofstream>();

        output_file_ptr->exceptions(std::ifstream::failbit | std::ifstream::badbit);
        // The standard G++ exception thrown by ofstream::open is more or less
        // useless, so we add, at least, the file path, and re-throw:
        try
        {
            output_file_ptr->open(path, std::ofstream::out);
        }
        catch (std::system_error &exc)
        {
            throw std::system_error(
                exc.code(),
                "Error opening tee file: " + path + ": " +
                    exc.code().message());
        }
        shared_outut_streams.emplace_back(output_file_ptr);

        stream_buf.add_output_buf(output_file_ptr->rdbuf());
    }
};

}

}

#endif // _MEM_USAGE_OSTREAM_TEE_HPP

// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef PIPE_POSIX_PIPE_IMPL_HPP
#define PIPE_POSIX_PIPE_IMPL_HPP


#include <filesystem>
#include "compare_handles.hpp"
#include "error.hpp"
#include <system_error>
#include <array>
#include <unistd.h>
#include <fcntl.h>
#include <memory>

namespace process::_detail::posix {

class pipe_impl
{
    int _source = -1;
    int _sink   = -1;
public:
    explicit pipe_impl(int source, int sink) : _source(source), _sink(sink) {}

    typedef          int               native_handle_type;

    pipe_impl() = default;
    void open()
    {
        int fds[2];
        if (::pipe(fds) == -1)
            throw_last_error("pipe(2) failed");

        _source = fds[0];
        _sink   = fds[1];
    }
    inline pipe_impl(const pipe_impl& rhs);

    pipe_impl steal_source() &&
    {
        int source = _source;
        _source = -1;
        return pipe_impl(source, -1);
    }

    pipe_impl steal_sink() &&
    {
        int sink = _sink;
        _sink = -1;
        return pipe_impl(-1, sink);
    }

    void open(const std::filesystem::path &name);

    pipe_impl(pipe_impl&& lhs)  : _source(lhs._source), _sink(lhs._sink)
    {
        lhs._source = -1;
        lhs._sink   = -1;
    }
    inline pipe_impl& operator=(const pipe_impl& );
    pipe_impl& operator=(pipe_impl&& lhs)
    {
        _source = lhs._source;
        _sink   = lhs._sink ;

        lhs._source = -1;
        lhs._sink   = -1;

        return *this;
    }
    ~pipe_impl()
    {
        if (_sink   != -1)
            ::close(_sink);
        if (_source != -1)
            ::close(_source);
    }
    native_handle_type native_source() const {return _source;}
    native_handle_type native_sink  () const {return _sink;}

    void assign_source(native_handle_type h) { _source = h;}
    void assign_sink  (native_handle_type h) { _sink = h;}

    std::size_t write(const void * data, std::size_t count)
    {
        std::size_t write_len;
        while ((write_len = ::write(_sink, data, count)) == -1)
        {
            //Try again if interrupted
            auto err = errno;
            if (err != EINTR)
                throw_last_error();
        }
        return write_len;
    }

    std::size_t read(void * data, std::size_t count)
    {
        std::size_t read_len;
        while ((read_len = ::read(_source, data, count)) == -1)
        {
            //Try again if interrupted
            auto err = errno;
            if (err != EINTR)
                throw_last_error();
        }
        return read_len;
    }

    bool is_open() const
    {
        return (_source != -1) ||
               (_sink   != -1);
    }

    void close()
    {
        if (_source != -1)
            ::close(_source);
        if (_sink != -1)
            ::close(_sink);
        _source = -1;
        _sink   = -1;
    }
};

pipe_impl::pipe_impl(const pipe_impl & rhs)
{
       if (rhs._source != -1)
       {
           _source = ::dup(rhs._source);
           if (_source == -1)
               throw_last_error("dup() failed");
       }
    if (rhs._sink != -1)
    {
        _sink = ::dup(rhs._sink);
        if (_sink == -1)
            throw_last_error("dup() failed");

    }
}

pipe_impl &pipe_impl::operator=(const pipe_impl & rhs)
{
       if (rhs._source != -1)
       {
           _source = ::dup(rhs._source);
           if (_source == -1)
               throw_last_error("dup() failed");
       }
    if (rhs._sink != -1)
    {
        _sink = ::dup(rhs._sink);
        if (_sink == -1)
            throw_last_error("dup() failed");

    }
    return *this;
}


void pipe_impl::open(const std::filesystem::path &name)
{
    auto fifo = mkfifo(name.c_str(), 0666 );
            
    if (fifo != 0) 
        throw_last_error("mkfifo() failed");

    int read_fd = ::open(name.c_str(), O_RDONLY);
    if (read_fd == -1)
        throw_last_error();

    int write_fd = ::open(name.c_str(), O_WRONLY);
    if (write_fd == -1)
        throw_last_error();

    _sink = write_fd;
    _source = read_fd;
    ::unlink(name.c_str());
}

}

#endif

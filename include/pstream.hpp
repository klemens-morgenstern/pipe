//
// Created by kleme on 15.04.2020.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef PSTREAM_HPP
#define PSTREAM_HPP

#include <streambuf>
#include <istream>
#include <ostream>
#include <vector>
#include <tuple>

#if defined(_WIN32)

#else
#include "posix/pipe_impl.hpp"
#endif

namespace process
{

#if defined(_WIN32)

#else
namespace _impl = _detail::posix;
#endif



template<
        class CharT,
        class Traits = std::char_traits<CharT>
>
class basic_pstream;

/** Implementation of the stream buffer for a pipe.
 */
template<
        class CharT,
        class Traits = std::char_traits<CharT>>
struct basic_pipebuf : std::basic_streambuf<CharT, Traits>
{

    typedef           CharT            char_type  ;
    typedef           Traits           traits_type;
    typedef  typename Traits::int_type int_type   ;
    typedef  typename Traits::pos_type pos_type   ;
    typedef  typename Traits::off_type off_type   ;

    constexpr static int default_buffer_size = 1024;

    ///Default constructor, will also construct the pipe.
    basic_pipebuf() : _write(default_buffer_size), _read(default_buffer_size)
    {
        this->setg(_read.data(),  _read.data()+ 128,  _read.data() + 128);
        this->setp(_write.data(), _write.data() + _write.size());
    }
    ///Copy Constructor.
    basic_pipebuf(const basic_pipebuf & ) = default;
    ///Move Constructor
    basic_pipebuf(basic_pipebuf && ) = default;

    ///Destructor -> writes the frest of the data
    ~basic_pipebuf()
    {
        if (is_open())
            overflow(Traits::eof());
    }

    ///Copy assign.
    basic_pipebuf& operator=(const basic_pipebuf & ) = delete;
    ///Move assign.
    basic_pipebuf& operator=(basic_pipebuf && ) = default;

    ///Writes characters to the associated output sequence from the put area
    int_type overflow(int_type ch = traits_type::eof()) override
    {
        if (_pipe.is_open() && (ch != traits_type::eof()))
        {
            if (this->pptr() == this->epptr())
            {
                bool wr = this->_write_impl();
                if (wr)
                {
                    *this->pptr() = ch;
                    this->pbump(1);
                    return ch;
                }
            }
            else
            {
                *this->pptr() = ch;
                this->pbump(1);
                if (this->_write_impl())
                    return ch;
            }
        }
        else if (ch == traits_type::eof())
            this->sync();

        return traits_type::eof();
    }
    ///Synchronizes the buffers with the associated character sequence
    int sync() override { return this->_write_impl() ? 0 : -1; }

    ///Reads characters from the associated input sequence to the get area
    int_type underflow() override
    {
        if (!_pipe.is_open())
            return traits_type::eof();

        if (this->egptr() == &_read.back()) //ok, so we're at the end of the buffer
            this->setg(_read.data(),  _read.data()+ 10,  _read.data() + 10);


        auto len = &_read.back() - this->egptr() ;
        auto res = _pipe.read(
                this->egptr(),
                static_cast<std::size_t>(len));
        if (res == 0)
            return traits_type::eof();

        this->setg(this->eback(), this->gptr(), this->egptr() + res);
        auto val = *this->gptr();

        return traits_type::to_int_type(val);
    }

    ///Check if the pipe is open
    bool is_open() const {return _pipe.is_open(); }

    basic_pipebuf<CharT, Traits>* open(const std::filesystem::path & pt) {
        if (is_open())
            return nullptr;
        _pipe.open(pt);
        return this;
    }

    basic_pipebuf<CharT, Traits>* open() {
        if (is_open())
            return nullptr;
        _pipe.open();
        return this;
    }

    ///Flush the buffer & close the pipe
    basic_pipebuf<CharT, Traits>* close()
    {
        if (!is_open())
            return nullptr;
        overflow(Traits::eof());
        return this;
    }
private:
    _impl::pipe_impl _pipe;
    std::vector<char_type> _write;
    std::vector<char_type> _read;

    bool _write_impl()
    {
        if (!_pipe.is_open())
            return false;

        auto base = this->pbase();

        if (base == this->pptr())
            return true;

        std::ptrdiff_t wrt = _pipe.write(base, static_cast<std::size_t>(this->pptr() - base));

        std::ptrdiff_t diff = this->pptr() - base;

        if (wrt < diff)
            std::move(base + wrt, base + diff, base);
        else if (wrt == 0) //broken pipe
            return false;

        this->pbump(-wrt);

        return true;
    }

    basic_pipebuf _steal_sink() &&
    {
        basic_pipebuf wb;
        wb._pipe = std::move(_pipe).steal_sink();
        wb._write = std::move(_write);
        wb.setp(this->pbase(), this->pptr() );

        this->_write = {};
        this->setp(_write.data(), _write.data());

        return wb;
    }

    basic_pipebuf _steal_source() &&
    {

        basic_pipebuf rb;
        rb._pipe = std::move(_pipe).steal_source();
        rb._read = std::move(_read);
        rb.setg(this->eback(), this->gptr(), this->egptr());

        this->_read = {};
        this->setg(_read.data(), _read.data(), _read.data());
        return rb;
    }

    friend class basic_pstream<CharT, Traits>;
};

typedef basic_pipebuf<char>     pipebuf;
typedef basic_pipebuf<wchar_t> wpipebuf;

/** Implementation of a reading pipe stream.
 *
 */
template<
        class CharT,
        class Traits = std::char_traits<CharT>
>
class basic_ipstream : public std::basic_istream<CharT, Traits>
{
    mutable basic_pipebuf<CharT, Traits> _buf;
public:

    typedef           CharT            char_type  ;
    typedef           Traits           traits_type;
    typedef  typename Traits::int_type int_type   ;
    typedef  typename Traits::pos_type pos_type   ;
    typedef  typename Traits::off_type off_type   ;

    ///Get access to the underlying stream_buf
    basic_pipebuf<CharT, Traits>* rdbuf() const {return &_buf;};

    ///Default constructor.
    basic_ipstream() : std::basic_istream<CharT, Traits>(nullptr)
    {
        std::basic_istream<CharT, Traits>::rdbuf(&_buf);
    };
    ///Copy constructor.
    basic_ipstream(const basic_ipstream & ) = delete;
    ///Move constructor.
    basic_ipstream(basic_ipstream && lhs) : std::basic_istream<CharT, Traits>(nullptr), _buf(std::move(lhs._buf))
    {
        std::basic_istream<CharT, Traits>::rdbuf(&_buf);
    }

    ///Copy assignment.
    basic_ipstream& operator=(const basic_ipstream & ) = delete;
    ///Move assignment
    basic_ipstream& operator=(basic_ipstream && lhs)
    {
        std::basic_istream<CharT, Traits>::operator=(std::move(lhs));
        _buf = std::move(lhs._buf);
        std::basic_istream<CharT, Traits>::rdbuf(&_buf);
        return *this;
    };

    ///Check if the pipe is open
    bool is_open() const {return _buf.is_open();}

    ///Flush the buffer & close the pipe
    void close()
    {
        if (_buf.close() == nullptr)
            this->setstate(std::ios_base::failbit);
    }
private:
    friend class basic_pstream<CharT, Traits>;
    basic_ipstream(basic_pipebuf<CharT, Traits> && buf) : _buf(std::move(buf))
    {
        std::basic_istream<CharT, Traits>::rdbuf(&_buf);
    }
};

typedef basic_ipstream<char>     ipstream;
typedef basic_ipstream<wchar_t> wipstream;

/** Implementation of a write pipe stream.
 *
 */
template<
        class CharT,
        class Traits = std::char_traits<CharT>
>
class basic_opstream : public std::basic_ostream<CharT, Traits>
{
    mutable basic_pipebuf<CharT, Traits> _buf;
public:

    typedef           CharT            char_type  ;
    typedef           Traits           traits_type;
    typedef  typename Traits::int_type int_type   ;
    typedef  typename Traits::pos_type pos_type   ;
    typedef  typename Traits::off_type off_type   ;


    ///Get access to the underlying stream_buf
    basic_pipebuf<CharT, Traits>* rdbuf() const {return &_buf;};

    ///Default constructor.
    basic_opstream() : std::basic_ostream<CharT, Traits>(nullptr)
    {
        std::basic_ostream<CharT, Traits>::rdbuf(&_buf);
    };
    ///Copy constructor.
    basic_opstream(const basic_opstream & ) = delete;
    ///Move constructor.
    basic_opstream(basic_opstream && lhs) : std::basic_ostream<CharT, Traits>(nullptr), _buf(std::move(lhs._buf))
    {
        std::basic_ostream<CharT, Traits>::rdbuf(&_buf);
    }

    ///Copy assignment.
    basic_opstream& operator=(const basic_opstream & ) = delete;
    ///Move assignment
    basic_opstream& operator=(basic_opstream && lhs)
    {
        std::basic_ostream<CharT, Traits>::operator=(std::move(lhs));
        _buf = std::move(lhs._buf);
        std::basic_ostream<CharT, Traits>::rdbuf(&_buf);
        return *this;
    };

    ///Flush the buffer & close the pipe
    void close()
    {
        if (_buf.close() == nullptr)
            this->setstate(std::ios_base::failbit);
    }
private:
    friend class basic_pstream<CharT, Traits>;
    basic_opstream(basic_pipebuf<CharT, Traits> && buf) : _buf(std::move(buf))
    {
        std::basic_ostream<CharT, Traits>::rdbuf(&_buf);
    }
};

typedef basic_opstream<char>     opstream;
typedef basic_opstream<wchar_t> wopstream;


//template<std::size_t Index, class CharT, class Traits> auto get(basic_pstream<CharT, Traits> && s) -> typename std::tuple_element<Index, basic_pstream<CharT, Traits>>::type;
template<std::size_t Index, class CharT, class Traits> auto get(basic_pstream<CharT, Traits> && s) -> typename std::tuple_element<Index, basic_pstream<CharT, Traits>>::type
{
    return std::move(s)._get(std::integral_constant<int, Index>());
}

/** Implementation of a read-write pipe stream.
 *
 */
template<
        class CharT,
        class Traits
>
class basic_pstream : public std::basic_iostream<CharT, Traits>
{
    mutable basic_pipebuf<CharT, Traits> _buf;
public:

    typedef           CharT            char_type  ;
    typedef           Traits           traits_type;
    typedef  typename Traits::int_type int_type   ;
    typedef  typename Traits::pos_type pos_type   ;
    typedef  typename Traits::off_type off_type   ;


    ///Get access to the underlying stream_buf
    basic_pipebuf<CharT, Traits>* rdbuf() const {return &_buf;};

    ///Default constructor.
    basic_pstream() : std::basic_iostream<CharT, Traits>(nullptr)
    {
        _buf.open();
        std::basic_iostream<CharT, Traits>::rdbuf(&_buf);
    };

    ///Default constructor.
    basic_pstream(const std::filesystem::path & name) : std::basic_iostream<CharT, Traits>(nullptr)
    {
        _buf.open(name);
        std::basic_iostream<CharT, Traits>::rdbuf(&_buf);
    };
    ///Copy constructor.
    basic_pstream(const basic_pstream & ) = delete;
    ///Move constructor.
    basic_pstream(basic_pstream && lhs) : std::basic_iostream<CharT, Traits>(nullptr), _buf(std::move(lhs._buf))
    {
        std::basic_iostream<CharT, Traits>::rdbuf(&_buf);
    }

    ///Copy assignment.
    basic_pstream& operator=(const basic_pstream & ) = delete;
    ///Move assignment
    basic_pstream& operator=(basic_pstream && lhs)
    {
        std::basic_istream<CharT, Traits>::operator=(std::move(lhs));
        _buf = std::move(lhs._buf);
        std::basic_iostream<CharT, Traits>::rdbuf(&_buf);
        return *this;
    };

    ///Open a new pipe
    void open()
    {
        if (_buf.open() == nullptr)
            this->setstate(std::ios_base::failbit);
        else
            this->clear();
    }

    ///Open a new named pipe
    void open(const std::filesystem::path & name)
    {
        if (_buf.open(name) == nullptr)
            this->setstate(std::ios_base::failbit);
        else
            this->clear();
    }

    ///Flush the buffer & close the pipe
    void close()
    {
        if (_buf.close() == nullptr)
            this->setstate(std::ios_base::failbit);
    }
private:
    template<std::size_t Index, typename CharT_, typename Traits_>
    friend auto get(basic_pstream<CharT_, Traits_> && s) -> typename std::tuple_element<Index, basic_pstream<CharT_, Traits_>>::type;

    basic_ipstream<CharT, Traits> _get(std::integral_constant<int, 0>) &&
    {
        return {std::move(_buf)._steal_source()};
    }
    basic_opstream<CharT, Traits> _get(std::integral_constant<int, 1>) &&
    {
        return {std::move(_buf)._steal_sink()};
    }
};

typedef basic_pstream<char>     pstream;
typedef basic_pstream<wchar_t> wpstream;

}


namespace std
{

template<typename Char>
class tuple_size<process::basic_pstream<Char>> : public std::integral_constant<std::size_t, 2> { };

template<typename Char> class tuple_element<0, process::basic_pstream<Char>>  { public: typedef process::basic_ipstream<Char> type; };
template<typename Char> class tuple_element<1, process::basic_pstream<Char>>  { public: typedef process::basic_opstream<Char> type; };

}

#endif //PIPE_PSTREAM_HPP

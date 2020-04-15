//
// Created by kleme on 16.04.2020.
//

#ifndef PIPE_ERROR_HPP
#define PIPE_ERROR_HPP

#include <system_error>
#include <errno.h>

namespace process::_detail::posix {

inline void throw_last_error(const std::string & description)
{
    throw std::system_error(std::error_code(errno, std::system_category()), description);
}

inline void throw_last_error()
{
    throw std::system_error(std::error_code(errno, std::system_category()));
}

}

#endif //PIPE_ERROR_HPP

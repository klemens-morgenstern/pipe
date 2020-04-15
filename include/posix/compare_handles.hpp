// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef PROCESS_POSIX_COMPARE_HANDLES_HPP_
#define PROCESS_POSIX_COMPARE_HANDLES_HPP_


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "error.hpp"

namespace process::_detail::posix {


inline bool compare_handles(int lhs, int rhs)
{

    if ((lhs == -1) || (rhs == -1))
        return false;

    if (lhs == rhs)
        return true;

    struct ::stat stat1, stat2;
    if(::fstat(lhs, &stat1) < 0) throw_last_error("fstat() failed");
    if(::fstat(rhs, &stat2) < 0) throw_last_error("fstat() failed");
    
    return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);   
}





}



#endif

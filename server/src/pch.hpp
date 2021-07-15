#ifndef BATTLESHIP_SERVER_PCH_HPP
#define BATTLESHIP_SERVER_PCH_HPP

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#define BOOST_BEAST_USE_STD_STRING_VIEW
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <boost/utility/string_view_fwd.hpp>

#endif //BATTLESHIP_SERVER_PCH_HPP 

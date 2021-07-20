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
#include <array>

#define BOOST_BEAST_USE_STD_STRING_VIEW
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/rfc7230.hpp>
#include <boost/beast/http/vector_body.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <boost/utility/string_view_fwd.hpp>

#include <nlohmann/json.hpp>

#include "battleship/common/util/num_types.hpp"

#endif //BATTLESHIP_SERVER_PCH_HPP 

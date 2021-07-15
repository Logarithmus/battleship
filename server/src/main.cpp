// Based on boost-beast/example/http/server/async-ssl/http_server_async_ssl.cpp
//------------------------------------------------------------------------------
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP SSL server, asynchronous
//
//------------------------------------------------------------------------------

#include "pch.hpp"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http  = beast::http; // from <boost/beast/http.hpp>
namespace net   = boost::asio; // from <boost/asio.hpp>
namespace ssl   = boost::asio::ssl; // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

constexpr std::string_view mime_type{"application/cbor"};

// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template<class Body, class Allocator, class Send>
void handle_request(
	http::request<Body, http::basic_fields<Allocator>>&& req,
	Send&& send
) {
    // Returns a bad request response
    const auto bad_request = [&req](std::string_view why) {
		http::response<http::string_body> res{ http::status::bad_request,
						       req.version() };
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = std::string(why);
		res.prepare_payload();
		return res;
    };

    // Returns a not found response
    const auto not_found = [&req](std::string_view target) {
		http::response<http::string_body> res{ http::status::not_found,
						       req.version() };
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = "The resource '" + std::string(target) + "' was not found.";
		res.prepare_payload();
		return res;
    };

    // Returns a server error response
    const auto server_error = [&req](std::string_view what) {
		http::response<http::string_body> res{ http::status::internal_server_error,
						       req.version() };
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = "An error occurred: '" + std::string(what) + "'";
		res.prepare_payload();
		return res;
    };

    // Make sure we can handle the method
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
		return send(bad_request("Unknown HTTP-method"));
	}

    // Request path must be absolute and not contain "..".
    if (req.target().empty()
	 || req.target()[0] != '/'
	 || req.target().find("..") != std::string_view::npos) {
		return send(bad_request("Illegal request-target"));
	}

    // Attempt to open the file
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(req.target().data(), beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if (ec == beast::errc::no_such_file_or_directory) {
		return send(not_found(req.target()));
	}

    // Handle an unknown error
    if (ec) { return send(server_error(ec.message())); }

    // Cache the size since we need it after the move
    const auto size = body.size();

    // Respond to HEAD request
    if (req.method() == http::verb::head) {
		http::response<http::empty_body> res{ http::status::ok, req.version() };
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, mime_type);
		res.content_length(size);
		res.keep_alive(req.keep_alive());
		return send(std::move(res));
    }

    // Respond to GET request
    http::response<http::file_body> res{
		std::piecewise_construct, std::make_tuple(std::move(body)),
		std::make_tuple(http::status::ok, req.version())
    };
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type);
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
}

//------------------------------------------------------------------------------

// Report a failure
void fail(beast::error_code ec, char const* what) {
    // ssl::error::stream_truncated, also known as an SSL "short read",
    // indicates the peer closed the connection without performing the
    // required closing handshake (for example, Google does this to
    // improve performance). Generally this can be a security issue,
    // but if your communication protocol is self-terminated (as
    // it is with both HTTP and WebSocket) then you may simply
    // ignore the lack of close_notify.
    //
    // https://github.com/boostorg/beast/issues/38
    //
    // https://security.stackexchange.com/questions/91435/how-to-handle-a-malicious-ssl-tls-shutdown
    //
    // When a short read would cut off the end of an HTTP message,
    // Beast returns the error beast::http::error::partial_message.
    // Therefore, if we see a short read here, it has occurred
    // after the message has been completed, so it is safe to ignore it.
    if (ec == net::ssl::error::stream_truncated) { return; }
    std::cerr << what << ": " << ec.message() << "\n";
}

// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session> {
    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    struct send_lambda {
		session& self_;

		explicit send_lambda(session& self)
		: self_(self) {}

		template<bool isRequest, class Body, class Fields>
		void operator()(http::message<isRequest, Body, Fields>&& msg) const {
		    // The lifetime of the message has to extend
		    // for the duration of the async operation so
		    // we use a shared_ptr to manage it.
		    auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(
			std::move(msg));

		    // Store a type-erased version of the shared
		    // pointer in the class to keep it alive.
		    self_.res_ = sp;

		    // Write the response
		    http::async_write(self_.stream_, *sp,
				      beast::bind_front_handler(&session::on_write,
								self_.shared_from_this(),
								sp->need_eof()));
		}
    };

    beast::ssl_stream<beast::tcp_stream> stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    std::shared_ptr<void> res_;
    send_lambda lambda_;

	static constexpr std::chrono::seconds TIMEOUT{30};

public:
    // Take ownership of the socket
    explicit session(tcp::socket&& socket, ssl::context& ctx)
    : stream_{std::move(socket), ctx}, lambda_(*this) {}

    // Start the asynchronous operation
    void run() {
		// We need to be executing within a strand to perform async operations
		// on the I/O objects in this session. Although not strictly necessary
		// for single-threaded contexts, this example code is written to be
		// thread-safe by default.
		net::dispatch(
		    stream_.get_executor(),
		    beast::bind_front_handler(&session::on_run, shared_from_this())
		);
    }

    void on_run() {
		// Set the timeout.
		beast::get_lowest_layer(stream_).expires_after(TIMEOUT);

		// Perform the SSL handshake
		stream_.async_handshake(
		    ssl::stream_base::server,
		    beast::bind_front_handler(&session::on_handshake, shared_from_this())
		);
    }

    void on_handshake(beast::error_code ec) {
		if (ec) { return fail(ec, "handshake"); }
		do_read();
    }

    void do_read() {
		// Make the request empty before reading,
		// otherwise the operation behavior is undefined.
		req_ = {};

		// Set the timeout.
		beast::get_lowest_layer(stream_).expires_after(TIMEOUT);

		// Read a request
		http::async_read(
		    stream_, buffer_, req_,
		    beast::bind_front_handler(&session::on_read, shared_from_this()));
    }

	void on_read(beast::error_code ec, std::size_t bytes_transferred) {
		boost::ignore_unused(bytes_transferred);

		// This means they closed the connection
		if (ec == http::error::end_of_stream) { return do_close(); }

		if (ec) { return fail(ec, "read"); }

		// Send the response
		handle_request(std::move(req_), lambda_);
    }

    void on_write(bool close, beast::error_code ec, std::size_t bytes_transferred) {
		boost::ignore_unused(bytes_transferred);

		if (ec) { return fail(ec, "write"); }

		// This means we should close the connection, usually because
		// the response indicated the "Connection: close" semantic.
		if (close) { return do_close(); }

		// We're done with the response so delete it
		res_ = nullptr;

		// Read another request
		do_read();
    }

    void do_close() {
		// Set the timeout.
		beast::get_lowest_layer(stream_).expires_after(TIMEOUT);

		// Perform the SSL shutdown
		stream_.async_shutdown(
		    beast::bind_front_handler(&session::on_shutdown, shared_from_this()));
    }

    void on_shutdown(beast::error_code ec) {
		if (ec) { return fail(ec, "shutdown"); }
		// At this point the connection is closed gracefully
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener> {
    net::io_context& ioc_;
    ssl::context& ctx_;
    tcp::acceptor acceptor_;
public:
    listener(net::io_context& ioc, ssl::context& ctx, const tcp::endpoint& endpoint)
    : ioc_{ioc}, ctx_{ctx}, acceptor_{ioc} {
		beast::error_code ec;

		// Open the acceptor
		acceptor_.open(endpoint.protocol(), ec);
		if (ec) {
		    fail(ec, "open");
		    return;
		}

		// Allow address reuse
		acceptor_.set_option(net::socket_base::reuse_address(true), ec);
		if (ec) {
		    fail(ec, "set_option");
		    return;
		}

		// Bind to the server address
		acceptor_.bind(endpoint, ec);
		if (ec) {
		    fail(ec, "bind");
		    return;
		}

		// Start listening for connections
		acceptor_.listen(net::socket_base::max_listen_connections, ec);
		if (ec) {
		    fail(ec, "listen");
		    return;
		}
    }

    // Start accepting incoming connections
    void run() {
		do_accept();
    }

private:
    void do_accept() {
	// The new connection gets its own strand
	acceptor_.async_accept(
	    net::make_strand(ioc_),
	    beast::bind_front_handler(&listener::on_accept, shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket) {
		if (ec) {
		    fail(ec, "accept");
		} else {
			// Create the session and run it
			std::make_shared<session>(std::move(socket), ctx_)->run();
		}
		// Accept another connection
		do_accept();
    }
};

void load_server_certificate(
	ssl::context& ctx,
	const std::string& cert_path,
	const std::string& chain_path,
	const std::string& privkey_path,
	const std::string& dh_path
) {
	ctx.set_password_callback([](
		[[maybe_unused]] std::size_t size,
		[[maybe_unused]] ssl::context_base::password_purpose password_purpose
	) {
		return "test";
	});
	
	ctx.set_options(
	    boost::asio::ssl::context::default_workarounds |
	    boost::asio::ssl::context::no_sslv2 |
	    boost::asio::ssl::context::single_dh_use
	);
	
	ctx.use_certificate_chain_file(chain_path);
	ctx.use_certificate_file(cert_path, ssl::context::file_format::pem);
	ctx.use_private_key_file(privkey_path, ssl::context::file_format::pem);
	ctx.use_tmp_dh_file(dh_path);
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
    const std::span<char*> args{ argv, static_cast<size_t>(argc) };

	const std::string CERT_PATH     = "tls/fullchain.pem";
	const std::string CHAIN_PATH    = "tls/chain.pem";
	const std::string PRIVKEY_PATH  = "tls/privkey.pem";
	const std::string DH_PATH       = "tls/dhparam.pem";

    // Check command line arguments.
    if (args.size() != 3) {
		std::cerr << "Usage: http-server-async-ssl <address> <port>"
			"Example:\n"
			"    http-server-async-ssl 0.0.0.0 8443\n";
		return EXIT_FAILURE;
    }
    const auto address  = net::ip::make_address(args[1]);
    const auto port     = static_cast<uint16_t>(std::stoul(args[2]));
    const auto threads  = std::thread::hardware_concurrency();

    // The io_context is required for all I/O
    net::io_context io_ctx { static_cast<int>(threads) };

    // The SSL context is required, and holds certificates
    ssl::context ctx{ ssl::context::tlsv12 };

    // This holds the self-signed certificate used by the server
    load_server_certificate(ctx, CERT_PATH, CHAIN_PATH, PRIVKEY_PATH, DH_PATH);

    // Create and launch a listening port
    std::make_shared<listener>(io_ctx, ctx, tcp::endpoint{ address, port })->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i) {
		v.emplace_back([&io_ctx] { io_ctx.run(); });
	}
    io_ctx.run();

    return EXIT_SUCCESS;
}

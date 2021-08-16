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

#include <algorithm>
#include <span>
#include <unordered_map>
#include <boost/beast/http/vector_body.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/uuid/random_generator.hpp>
#include <nlohmann/byte_container_with_subtype.hpp>
#include "game.hpp"
#include "pch.hpp"
#include "battleship/common/common.hpp"

namespace battleship {
	namespace beast = boost::beast; // from <boost/beast.hpp>
	namespace http  = beast::http; // from <boost/beast/http.hpp>
	namespace net   = boost::asio; // from <boost/asio.hpp>
	namespace ssl   = boost::asio::ssl; // from <boost/asio/ssl.hpp>
	using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
	
	UUIDVec vec_from_uuid(const UUID& uuid) {
		return {uuid.begin(), uuid.end()};
	}

	UUID uuid_from_span(const UUIDSpan span) {
		UUID uuid{};
		std::copy(span.begin(), span.end(), uuid.begin());
		return uuid;
	}

	UUIDArray arr_from_uuid(const UUID& uuid) {
		UUIDArray arr;
		std::copy_n(uuid.begin(), uuid.size(), arr.begin());
		return arr;
	}

	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template<class Send>
	void handle_request(http::request<http::vector_body<u8>>&& req, Send&& send) {
		using nlohmann::json;

		// Returns cbor response
		const auto cbor_response = [&req](http::status status, std::vector<u8>&& data) {
			http::response<http::vector_body<u8>> res {status, req.version()};
			res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
			res.set(http::field::content_type, "application/cbor");
			res.keep_alive(req.keep_alive());
			res.body() = std::move(data);
			res.prepare_payload();
			return res;
		};
		const auto cbor_str_response = [&cbor_response](
			http::status status,
			std::string_view msg
		) {
			json j_string{msg};	
			return cbor_response(
				status,
				std::move(json::to_cbor(j_string))
			);
		};

	    // Request path must be absolute and not contain "..".
	    if (req.target().empty()
		 || req.target()[0] != '/'
		 || req.target().find("..") != std::string_view::npos) {
			return send(cbor_str_response(http::status::bad_request, "Illegal request target"));
		}
	
		boost::uuids::random_generator_mt19937 uuid_generator;
		PlayerList players;
		PlayerQueue players_queue;
		std::vector<Room> rooms;
		RoomMap room_map;
	
	    // Start game
		if ( (req.target() == "/start") && (req.method() == http::verb::post) ) {
			// Check ships positions & return the result to player
			auto ships = json::from_cbor(req.body()).get<StdShips>();
			std::cout << "ships: " << json::from_cbor(req.body()) << std::endl;
			auto player_result = Player::try_from_ships(std::move(ships));
			std::cout << "player_result: " << is_ok(player_result) << '\n';
			// if player is ok, generate UUID, put player in a queue & return their UUID
			if (is_ok(player_result)) {
				auto player = std::get<Player>(player_result);
				UUID uuid = uuid_generator();
				players.insert({uuid, player});
				players_queue.push(uuid);
				json response_body;
				response_body["uuid"] = json::binary(vec_from_uuid(uuid));
				response_body["field"] = player;
				std::cout << "Body: \n" << response_body << '\n';
				return send(cbor_response(http::status::ok, json::to_cbor(response_body)));
			}
			// else return bad request & reason
			return send(cbor_str_response(
				http::status::bad_request, "Bad ships layout"
			));
		}
		// Shoot
		if ( (req.target() == "/shoot") && (req.method() == http::verb::patch) ) {
			json req_body = json::from_cbor(req.body());
			UUIDVec uuid_vec {req_body.value("uuid", UUIDVec{})};
			UUID uuid = uuid_from_span(UUIDSpan{uuid_vec});

			// If player provided wrong or no UUID, then return error
			if (!players.contains(uuid)) {
				return send(cbor_str_response(
					http::status::unauthorized, "No player with such UUID"
				));
			}
			auto room = rooms[room_map[uuid]];
			auto enemy_uuid_option = room.my_enemy(uuid);
			if (!enemy_uuid_option.has_value()) {
				return send(cbor_str_response(
					http::status::internal_server_error,
					"This UUID was mapped to the room, but this room doesn't contain this UUID"
				));
			}
			UUID enemy_uuid = enemy_uuid_option.value();
			if (!room.is_my_move(uuid)) {
				return send(cbor_str_response(
					http::status::locked, "Wait for enemy's move"
				));
			}

			Player& enemy = players[enemy_uuid_option.value()];

			std::vector<u8> shot_vec = req_body;
			auto shot_pos = Position{shot_vec[0], shot_vec[1]};
			if (enemy.grid.contains(shot_pos)) {
				bool shot_result = enemy.grid.place_shot(shot_pos);
				if (shot_result) {
					room.move = uuid;
				} else {
					room.move = enemy_uuid;
				}
				return send(cbor_response(
					http::status::ok, json::to_cbor(json{shot_result})
				));
			}
		} else if ( (req.target() == "/field") && (req.method() == http::verb::get) ) {
			json req_body = json::from_cbor(req.body());
			UUIDVec uuid_vec {req_body.value("uuid", UUIDVec{})};
			UUID uuid = uuid_from_span(UUIDSpan{uuid_vec});

			if (!players.contains(uuid)) {
				return send(cbor_str_response(
					http::status::unauthorized, "No player with such UUID"
				));
			}

			Player& me = players[uuid];

			auto room = rooms[room_map[uuid]];
			auto enemy_uuid_option = room.my_enemy(uuid);
			if (!enemy_uuid_option.has_value()) {
				return send(cbor_str_response(
					http::status::internal_server_error,
					"This UUID was mapped to the room, but this room doesn't contain this UUID"
				));
			}
			if (!room.is_my_move(uuid)) {
				return send(cbor_str_response(http::status::locked, "Wait for enemy's move"));
			}

			json j;
			j["field"] = me;
			return send(cbor_response(http::status::ok, std::move(json::to_cbor(j))));
		}
	}
	
	//------------------------------------------------------------------------------
	
	// Report a failure
	void fail(beast::error_code ec, char const* what) {
	    if (ec == net::ssl::error::stream_truncated) { return; }
	    std::cerr << what << ": " << ec.message() << "\n";
	}
	
	// Handles an HTTP server connection
	class Session: public std::enable_shared_from_this<Session> {
	    // This is the C++11 equivalent of a generic lambda.
	    // The function object is used to send an HTTP message.
	    struct SendLambda {
			Session& self_;
	
			explicit SendLambda(Session& self): self_(self) {}
	
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
			    http::async_write(self_.stream_, *sp, beast::bind_front_handler(
					&Session::on_write,
					self_.shared_from_this(),
					sp->need_eof()
				));
			}
	    };
	
		beast::ssl_stream<beast::tcp_stream> stream_;
	    beast::flat_buffer buffer_;
	    http::request<http::vector_body<u8>> req_;
	    std::shared_ptr<void> res_;
		SendLambda lambda_;
	
		static constexpr std::chrono::seconds TIMEOUT{30};
	
	public:
		// Take ownership of the socket
		explicit Session(tcp::socket&& socket, ssl::context& ctx)
		 	: stream_{std::move(socket), ctx}, lambda_(*this) {}
	
	    // Start the asynchronous operation
	    void run() {
			// We need to be executing within a strand to perform async operations
			// on the I/O objects in this session. Although not strictly necessary
			// for single-threaded contexts, this example code is written to be
			// thread-safe by default.
			net::dispatch(
				stream_.get_executor(),
				beast::bind_front_handler(&Session::on_run, shared_from_this())
			);
	    }
	
	    void on_run() {
			// Set the timeout.
			beast::get_lowest_layer(stream_).expires_after(TIMEOUT);
	
			// Perform the SSL handshake
			stream_.async_handshake(
			    ssl::stream_base::server,
			    beast::bind_front_handler(&Session::on_handshake, shared_from_this())
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
				beast::bind_front_handler(&Session::on_read, shared_from_this())
			);
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
			    beast::bind_front_handler(&Session::on_shutdown, shared_from_this()));
	    }
	
	    void on_shutdown(beast::error_code ec) {
			if (ec) { return fail(ec, "shutdown"); }
			// At this point the connection is closed gracefully
	    }
	};
	
	//------------------------------------------------------------------------------
	
	// Accepts incoming connections and launches the sessions
	class Listener: public std::enable_shared_from_this<Listener> {
	    net::io_context& ioc_;
	    ssl::context& ctx_;
	    tcp::acceptor acceptor_;
	public:
	    Listener(net::io_context& ioc, ssl::context& ctx, const tcp::endpoint& endpoint)
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
		    beast::bind_front_handler(&Listener::on_accept, shared_from_this()));
	    }
	
	    void on_accept(beast::error_code ec, tcp::socket socket) {
			if (ec) {
			    fail(ec, "accept");
			} else {
				// Create the session and run it
				std::make_shared<Session>(std::move(socket), ctx_)->run();
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
	
	int server_main(int argc, char** argv) {
	    const std::span<char*> args{ argv, static_cast<size_t>(argc) };
	
		const std::string CERT_PATH    = "tls/fullchain.pem";
		const std::string CHAIN_PATH   = "tls/chain.pem";
		const std::string PRIVKEY_PATH = "tls/privkey.pem";
		const std::string DH_PATH      = "tls/dhparam.pem";
	
	    // Check command line arguments.
	    if (args.size() != 3) {
			std::cerr << "Usage: http-server-async-ssl <address> <port>"
				"Example:\n"
				"    http-server-async-ssl 0.0.0.0 8443\n";
			return EXIT_FAILURE;
	    }
	    const auto address  = net::ip::make_address(args[1]);
	    const auto port     = static_cast<u16>(std::stoul(args[2]));
	    const auto threads  = std::thread::hardware_concurrency();
	
	    // The io_context is required for all I/O
	    net::io_context io_ctx { static_cast<int>(threads) };
	
	    // The SSL context is required, and holds certificates
	    ssl::context ctx{ ssl::context::tls_server };
	
	    // This holds the self-signed certificate used by the server
	    load_server_certificate(ctx, CERT_PATH, CHAIN_PATH, PRIVKEY_PATH, DH_PATH);
	
	    // Create and launch a listening port
	    std::make_shared<Listener>(io_ctx, ctx, tcp::endpoint{ address, port })->run();
	
	    // Run the I/O service on the requested number of threads
	    std::vector<std::thread> v;
	    v.reserve(threads);
	    for (unsigned i = 0; i < threads; ++i) {
			v.emplace_back([&io_ctx] { io_ctx.run(); });
		}
	    io_ctx.run();
	
		std::cout << "Running on " << address << ":" << port << "\n";
	
	    return EXIT_SUCCESS;
	}
} // namespace battleship

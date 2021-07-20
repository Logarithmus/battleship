#ifndef BATTLESHIP_SERVER_GAME_HPP
#define BATTLESHIP_SERVER_GAME_HPP

#include <unordered_map>
#include <span>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/container_hash/hash.hpp>
#include "battleship/common/common.hpp"

namespace battleship {
	constexpr u8 ROWS = 10;
	constexpr u8 COLS = 10;

	using UUID       = boost::uuids::uuid;
	using UUIDArray  = std::array<UUID::value_type, UUID::static_size()>;
	using UUIDVec    = std::vector<UUID::value_type>;
	using UUIDSpan   = std::span<UUID::value_type, UUID::static_size()>;
	using ShotCoords = std::vector<u8>;
	using Player     = PlayerField<ROWS, COLS, 4, POST_SOVIET_RULES>;
	using PlayerList = std::unordered_map<UUID, Player, boost::hash<UUID>>;
	
	struct Room {
		Player player1, player2;
		
		Room(Player&& player1, Player&& player2): player1{player1}, player2{player2} {};
	};
} // namespace battleship

#endif // BATTLESHIP_SERVER_GAME_HPP

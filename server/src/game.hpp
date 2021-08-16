#ifndef BATTLESHIP_SERVER_GAME_HPP
#define BATTLESHIP_SERVER_GAME_HPP

#include <unordered_map>
#include <queue>
#include <span>
#include <optional>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/container_hash/hash.hpp>
#include "battleship/common/common.hpp"

namespace battleship {
	constexpr u8 ROWS = 10;
	constexpr u8 COLS = 10;

	using UUID        = boost::uuids::uuid;
	using UUIDArray   = std::array<UUID::value_type, UUID::static_size()>;
	using UUIDVec     = std::vector<UUID::value_type>;
	using UUIDSpan    = std::span<UUID::value_type, UUID::static_size()>;
	using ShotCoords  = std::vector<u8>;
	using StdShips    = Ships<ROWS, COLS, POST_SOVIET_RULES.size(), POST_SOVIET_RULES>;
	using Player      = PlayerField<ROWS, COLS, POST_SOVIET_RULES.size(), POST_SOVIET_RULES>;
	using PlayerList  = std::unordered_map<UUID, Player, boost::hash<UUID>>;
	using PlayerQueue = std::queue<UUID>;
	using RoomMap     = std::unordered_map<UUID, size_t, boost::hash<UUID>>;
	
	struct Room {
		UUID uuid_player1, uuid_player2, move;
		
		Room(UUID uuid1, UUID uuid2):
			uuid_player1{uuid1}, uuid_player2{uuid2}, move{uuid1} {};

		[[nodiscard]] bool is_my_move(const UUID& uuid) const {
			return uuid == move;
		}

		[[nodiscard]] std::optional<UUID> my_enemy(UUID uuid) const {
			if (uuid == uuid_player1) {
				return uuid_player2;
			}
			if (uuid == uuid_player2) {
				return uuid_player1;
			}
			return {};
		}
	};
} // namespace battleship

#endif // BATTLESHIP_SERVER_GAME_HPP

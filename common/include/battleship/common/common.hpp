#ifndef BATTLESHIP_COMMON_HPP
#define BATTLESHIP_COMMON_HPP

#include <bitset>
#include <span>
#include <nlohmann/json.hpp>
#include <optional>
#include <variant>
#include <array>
#include <numeric>
#include <tuple>
#include <algorithm>
#include <boost/container/static_vector.hpp>
#include "util/geometry.hpp"
#include "util/num_types.hpp"
#include "util/result.hpp"

using nlohmann::json;

namespace battleship {
	template<u8 ROWS, u8 COLS>
	struct Grid {
		static constexpr size_t SIZE = ROWS * COLS;
		static constexpr Rectangle RECT {{0, 0}, {ROWS, COLS}};

		inline bool place_ship(const Position& pos) {
			std::optional<size_t> i = index(pos);
			if (i.has_value()) { ships[*i] = true; }
			return i.has_value();
		}

		[[nodiscard]] inline bool has_ship(const Position& pos) const {
			std::optional<size_t> i = index(pos);
			return i.has_value() && ships[*i];
		}

		bool place_shot(const Position& pos) {
			std::optional<size_t> i = index(pos);
			if (i.has_value()) { shots[*i] = true; }
			return i.has_value();
		}

		[[nodiscard]] inline bool has_shot(const Position& pos) const {
			std::optional<u8> i = index(pos);
			return i.has_value() && shots[*i];
		}

		[[nodiscard]] inline bool contains(const Rectangle& rect) const {
			return RECT.contains(rect);
		}

		[[nodiscard]] inline bool contains(const Position& pos) const {
			return RECT.contains(pos);
		}

	private: 
		std::bitset<SIZE> ships;
		std::bitset<SIZE> shots;

		inline static std::optional<size_t> index(const Position& pos) {
			if (pos.row < ROWS && pos.col < COLS) {
				return COLS * pos.row + pos.col;
			}
			return {};
		}

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Grid, ships, shots)
	};

	struct Ship {
		Rectangle zone;

		Ship() = default;
		explicit Ship(Rectangle&& zone): zone{zone} {}
		explicit Ship(const Rectangle& zone): zone{zone} {}

		[[nodiscard]] inline u8 length() const {
			return std::max(zone.width(), zone.height());
		}

		[[nodiscard]] inline bool touches(const Ship& other) const {
			return zone.touches_or_intersects(other.zone);
		}

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Ship, zone)
	};

	template<u8 SHIP_TYPE_COUNT>
	using Rules = std::array<u8, SHIP_TYPE_COUNT>;

	constexpr Rules<4> POST_SOVIET_RULES {4, 3, 2, 1};
	constexpr Rules<5> AMERICAN_RULES {0, 1, 2, 1, 1};

	template<u8 ROWS, u8 COLS, u8 SHIP_TYPE_COUNT, const std::array<u8, SHIP_TYPE_COUNT>& RULES>
	struct Ships {
		static constexpr size_t SHIP_COUNT {
			std::accumulate(RULES.begin(), RULES.end(), static_cast<u8>(0))
		};

		Ships() = default;

		inline void push(Ship&& ship) {
			ships.emplace_back(ship);
			++count[ship.length() - 1];
		}

		inline void push_all(std::span<Ship> new_ships) {
			for (auto ship: new_ships) {
				ships.push(ship);
			}
		}

		[[nodiscard]] inline bool is_full() const {
			return (ships.size() == ships.capacity())
				&& std::equal(count.begin(), count.end(), RULES.begin());
		}

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Ships, ships)

		boost::container::static_vector<Ship, SHIP_COUNT> ships;
		// how many ships of each type now
		std::array<u8, RULES.size()> count {};
	};

	enum class ShipPlacementError {
		WrongLength,
		OutOfBounds,
		Overlap,
		TooManyShips
	};

	template<u8 ROWS, u8 COLS, u8 SHIP_TYPE_COUNT, const std::array<u8, SHIP_TYPE_COUNT>& RULES>
	struct PlayerField {
		using PlayerShips = Ships<ROWS, COLS, SHIP_TYPE_COUNT, RULES>;

		[[nodiscard]] inline bool is_out_of_bounds(const Ship& ship) const {
			return !grid.contains(ship.zone);
		}

		[[nodiscard]] inline bool overlaps(const Ship& ship) const {
			Offset one {1, 1};
			Rectangle zone {ship.zone.first - one, ship.zone.last + one};
			return std::any_of(zone.begin(), zone.end(),
				[this](const auto& pos){ return grid.has_ship(pos); }
			);
		}

		[[nodiscard]] inline bool is_full() const {
			return ships.is_full();
		}

		inline Result<std::monostate, ShipPlacementError> try_place_ship(Ship&& ship) {
			u8 len = ship.length();
			if (len > SHIP_TYPE_COUNT || len == 0) {
				return ShipPlacementError::WrongLength;
			}
			if (is_out_of_bounds(ship)) {
				return ShipPlacementError::OutOfBounds;
			}
			if (overlaps(ship)) {
				return ShipPlacementError::Overlap;
			}
			if (is_full()) {
				return ShipPlacementError::TooManyShips;
			}

			for (auto pos: ship.zone) { // NOLINT
				grid.place_ship(pos);
			}

			ships.push(std::move(ship));
			return {};
		}

		PlayerField() = default;

		inline static Result<PlayerField, ShipPlacementError> try_from_ships(
			PlayerField::PlayerShips&& ships
		) {
			PlayerField field;
			for (Ship ship: ships.ships) {
				auto result = field.try_place_ship(std::move(ship));
				if (is_err(result)) {
					return std::get<ShipPlacementError>(result);
				}
			}
			return field;
		}

		Grid<ROWS, COLS> grid;
		PlayerShips ships;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(PlayerField, grid, ships)
	};
} // namespace battleship

namespace nlohmann {
	template <typename T, const size_t N>
	struct adl_serializer<boost::container::static_vector<T, N>> {
		static void to_json(json& j, const boost::container::static_vector<T, N>& vec) {
			// TODO: come up with some clever way to do that without copying
			std::array<T, N> arr;
			std::copy(vec.begin(), vec.end(), arr.begin());
			j = arr;
		}

		static void from_json(const json& j, boost::container::static_vector<T, N>& vec) {
			// TODO: come up with some clever way to do that without copying
			std::array<T, N> arr = j;
			vec.assign(arr.begin(), arr.end());
		}
	};

	template <const size_t N>
	struct adl_serializer<std::bitset<N>> {
		static void to_json(json& j, const std::bitset<N>& bits) {
			// TODO: come up with more clever way to do that
			j = bits.to_string();
		}

		static void from_json(const json& j, std::bitset<N>& bits) {
			// TODO: come up with more clever way to do that
			bits = std::bitset<N>(j.get<std::string>());
		}
	};
} // namespace nlohmann

#endif //BATTLESHIP_COMMON_HPP

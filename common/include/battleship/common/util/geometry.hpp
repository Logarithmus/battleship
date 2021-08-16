#ifndef BATTLESHIP_UTIL_GEOMETRY_HPP
#define BATTLESHIP_UTIL_GEOMETRY_HPP

#include <utility>
#include <compare>
#include <nlohmann/json.hpp>
#include "num_types.hpp"

namespace battleship {
	struct Offset {
		i8 rows, cols;

		static const Offset DOWN, UP, RIGHT, LEFT;

		inline Offset operator*(i8 other) const {
			return {static_cast<i8>(other * rows), static_cast<i8>(other * cols)};
		}

		friend Offset operator*(i8 a, const Offset& other);
	};

	struct Position {
		u8 row, col;

		inline Position operator+(const Offset& offset) const {
			return {static_cast<u8>(row + offset.rows), static_cast<u8>(col + offset.cols)};
		}

		inline Position& operator+=(const Offset& offset) {
			row = static_cast<u8>(row + offset.rows);
			col = static_cast<u8>(col + offset.cols);
			return *this;
		}

		inline Position operator-(const Offset& offset) const {
			return {static_cast<u8>(row + offset.rows), static_cast<u8>(col + offset.cols)};
		}

		inline Position& operator-=(const Offset& offset) {
			row = static_cast<u8>(row - offset.rows);
			col = static_cast<u8>(col - offset.cols);
			return *this;
		}

		auto operator<=>(const Position&) const = default; //NOLINT

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Position, row, col)
	};

	struct Rectangle {
		struct Iter {
			Iter(u8 cols, const Position& pos): cols{cols}, pos{pos} {}

			inline Position& operator*() {
				return pos;
			}

			inline Position* operator->() {
				return &pos;
			}

			inline Iter& operator++() {
				if (pos.col < cols - 1) {
					++pos.col;
				} else {
					++pos.row;
					pos.col = 0;
				}
				return *this;
			}

			inline Iter operator++(int) {
				auto old = *this;
				++(*this);
				return old;
			}

			friend auto operator<=>(const Iter& iter1, const Iter& iter2) = default;
		private:
			u8 cols;
			Position pos;
		};

		Position first, last;

		Rectangle() = default;
	
		constexpr Rectangle(Position&& first, Position&& last):
			first{first}, last{last}
		{
			if (first > last) { //NOLINT
				std::swap(this->first, this->last);
			}
		}

		Rectangle(const Position& first, const Position& last):
			first{first}, last{last}
		{
			if (first > last) { //NOLINT
				std::swap(this->first, this->last);
			}
		}

		[[nodiscard]] inline Iter begin() const {
			return {width(), first};
		}

		[[nodiscard]] inline Iter end() const {
			return {width(), {last + Offset{1, 0}}};
		}
	
		[[nodiscard]] inline bool contains(const Rectangle& other) const {
			return (other.first >= first) && (other.last <= last); //NOLINT
		}

		[[nodiscard]] inline bool contains(const Position& point) const {
			return (point.row >= first.row) && (point.row <= last.row)
				&& (point.col >= first.col) && (point.col <= last.col);
		}
	
		[[nodiscard]] inline bool intersects(const Rectangle& other) const {
			return (other.first <= last) && (other.last >= first); //NOLINT
		}
	
		[[nodiscard]] inline bool touches_or_intersects(const Rectangle& other) const {
			Offset one {1, 1};
			Rectangle rect1 {this->first - one, this->last + one};
			Rectangle rect2 {other.first - one, other.last + one};
			return intersects(other);
		}
	
		[[nodiscard]] inline u8 width() const {
			return last.col - first.col + 1;
		}

		[[nodiscard]] inline u8 height() const {
			return last.row - first.row + 1;
		}

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Rectangle, first, last)
	};

}

namespace std {
	template<>
	struct iterator_traits<battleship::Rectangle::Iter> {
	    using difference_type   = ptrdiff_t;
	    using value_type        = battleship::Rectangle;
	    using pointer           = battleship::Rectangle*;
	    using reference         = battleship::Rectangle&;
	    using iterator_category = input_iterator_tag;
	};
}

#endif //BATTLESHIP_UTIL_GEOMETRY_HPP

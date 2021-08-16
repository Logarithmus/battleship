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

		auto operator<=>(const Position& other) const = default; //NOLINT

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Position, row, col)
	};

	struct Rectangle {
		struct Iter {
			Iter(const Rectangle& rect, const Position& pos):
				rect{rect}, pos{pos} {}

			inline Position& operator*() {
				return pos;
			}

			inline Position* operator->() {
				return &pos;
			}

			inline Iter& operator++() {
				// last iteration
				if (pos.row == rect.last.row + 1) {
					return *this;
				}
				if (pos.col < rect.last.col) {
					++pos.col;
				} else if (pos.row <= rect.last.row) {
					++pos.row;
					pos.col = rect.first.col;
				}
				return *this;
			}

			inline Iter operator++(int) {
				auto old = *this;
				++(*this);
				return old;
			}

			friend std::partial_ordering operator<=>(const Iter& iter1, const Iter& iter2);

			inline bool operator==(const Iter& other) const {
				return (*this <=> other) == std::partial_ordering::equivalent;
			}

			inline bool operator!=(const Iter& other) const {
				return (*this <=> other) != std::partial_ordering::equivalent;
			}

		private:
			const Rectangle& rect;
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
			return {*this, first};
		}

		[[nodiscard]] inline Iter end() const {
			return {*this, {static_cast<u8>(last.row + 1), first.col}};
		}
	
		[[nodiscard]] inline bool contains(const Rectangle& other) const {
			return (other.first >= first) && (other.last <= last); //NOLINT
		}

		[[nodiscard]] inline bool contains(const Position& point) const {
			return (point.row >= first.row) && (point.row <= last.row)
				&& (point.col >= first.col) && (point.col <= last.col);
		}
	
		[[nodiscard]] inline u8 width() const {
			return static_cast<u8>(last.col - first.col + 1);
		}

		[[nodiscard]] inline u8 height() const {
			return static_cast<u8>(last.row - first.row + 1);
		}

		auto operator<=>(const Rectangle& other) const = default;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Rectangle, first, last)
	};

	inline std::partial_ordering operator<=>(
		const Rectangle::Iter& iter1, const Rectangle::Iter& iter2
	) {
		if (iter1.rect != iter2.rect) {
			return std::partial_ordering::unordered;
		}
		return iter1.pos <=> iter2.pos;
	}
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

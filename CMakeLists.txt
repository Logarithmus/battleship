cmake_minimum_required(VERSION 3.15)
project(
	"battleship"
	VERSION 0.1.0 
	LANGUAGES CXX
)

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client/library)
add_subdirectory(client/sfml)

add_dependencies(battleship-server battleship-common)
add_dependencies(battleship-client-library battleship-common)
add_dependencies(battleship-client-sfml battleship-client-library)

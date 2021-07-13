cmake_minimum_required(VERSION 3.15)
project(
	"battleship"
	VERSION 0.1.0 
	LANGUAGES CXX
)


add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client/library)
add_subdirectory(client/sfml_client)

add_dependencies(battleship-common BoostContainer)

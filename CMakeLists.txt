cmake_minimum_required(VERSION 3.21)
project(predator-sense-linux VERSION 0.9.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
include(GNUInstallDirs)

option(BUILD_GUI "Build the graphical application" ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core DBus Gui)
add_compile_definitions(PREDATOR_VERSION="${PROJECT_VERSION}" PREDATOR_SOURCE_DIR="${CMAKE_SOURCE_DIR}" QT_NO_KEYWORDS)

add_executable(predator-sensed
    daemon/main.cpp daemon/Daemon.cpp daemon/DBusService.cpp daemon/WmiDevice.cpp
    daemon/AcerHardware.cpp daemon/SmbiosOem.cpp daemon/SystemStats.cpp
    daemon/NvidiaGpu.cpp daemon/ModelConfig.cpp)
target_link_libraries(predator-sensed PRIVATE Qt6::Core Qt6::DBus Qt6::Gui ${CMAKE_DL_LIBS})
target_compile_options(predator-sensed PRIVATE -Wall -Wextra)

add_executable(predatorctl cli/predatorctl.cpp)
target_link_libraries(predatorctl PRIVATE Qt6::Core Qt6::DBus)

if(BUILD_GUI)
    add_subdirectory(gui)
endif()

install(TARGETS predator-sensed DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
install(TARGETS predatorctl DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY data/models/ DESTINATION ${CMAKE_INSTALL_DATADIR}/predator-sense/models)
configure_file(data/predator-sensed.service.in predator-sensed.service @ONLY)
set(SYSTEMD_UNITDIR "${CMAKE_INSTALL_PREFIX}/lib/systemd/system" CACHE PATH "systemd system unit directory")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/predator-sensed.service DESTINATION ${SYSTEMD_UNITDIR})
install(FILES data/org.predatorsense.Daemon.conf DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system.d)
install(FILES data/org.predatorsense.Daemon.service DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system-services)
install(FILES data/org.predatorsense.policy DESTINATION ${CMAKE_INSTALL_DATADIR}/polkit-1/actions)

include(CTest)
if(BUILD_TESTING)
    add_executable(test_smbios tests/test_smbios.cpp daemon/SmbiosOem.cpp)
    target_link_libraries(test_smbios PRIVATE Qt6::Core)
    add_test(NAME smbios_phn16_72 COMMAND test_smbios ${CMAKE_SOURCE_DIR}/tests/data/phn16-72)
    add_test(NAME sim_smoke COMMAND ${CMAKE_SOURCE_DIR}/tests/sim_smoke.sh ${CMAKE_BINARY_DIR})
endif()

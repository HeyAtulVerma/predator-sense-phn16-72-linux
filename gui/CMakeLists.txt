find_package(Qt6 6.4 REQUIRED COMPONENTS Quick QuickControls2 Widgets DBus Svg)
qt_standard_project_setup()
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(predator-sense main.cpp Backend.cpp Backend.h Translator.cpp Translator.h)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(predator-sense
    URI PredatorSense
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/Main.qml qml/Theme.qml qml/HomePage.qml qml/FanPage.qml qml/LightingPage.qml
        qml/MonitorPage.qml qml/SettingsPage.qml qml/Card.qml qml/NavButton.qml qml/Gauge.qml
        qml/PSwitch.qml qml/PSlider.qml qml/OptionRow.qml qml/SegButton.qml qml/ColorPicker.qml
        qml/LineChart.qml
)
qt_add_resources(predator-sense icons PREFIX /icons BASE ../data FILES ../data/predator-sense.svg)

target_link_libraries(predator-sense PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Widgets Qt6::DBus Qt6::Svg)

install(TARGETS predator-sense DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ../data/predator-sense.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES ../data/predator-sense.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

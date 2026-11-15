find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)

# No WIN32 flag on purpose: a console window stays open, which is handy for debug output.
# For a release build add WIN32 and link Qt6::EntryPoint (see Qt docs).
add_executable(seamcarve_app
  main.cpp
  MatQtConvert.hpp  MatQtConvert.cpp
  DropImageView.hpp DropImageView.cpp
  CarveWorker.hpp   CarveWorker.cpp
  ResultDialog.hpp  ResultDialog.cpp
  MainWindow.hpp    MainWindow.cpp
)
target_link_libraries(seamcarve_app PRIVATE seamcore Qt6::Widgets)

# Windows + vcpkg: vcpkg copies the DLLs next to the exe but not Qt's platform plugin, and the trimmed
# qtbase build has no windeployqt. Without platforms/qwindows.dll the app fails with
# 'Could not find the Qt platform plugin "windows"'. Copy the plugin next to the exe after each build
# (Qt looks for <exe folder>/platforms/ by default).
if(WIN32 AND DEFINED VCPKG_INSTALLED_DIR)
  set(_qt_root "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
  set(_qt_plugin_rel "${_qt_root}/Qt6/plugins/platforms/qwindows.dll")
  set(_qt_plugin_dbg "${_qt_root}/debug/Qt6/plugins/platforms/qwindowsd.dll")
  if(EXISTS "${_qt_plugin_rel}")
    add_custom_command(TARGET seamcarve_app POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:seamcarve_app>/platforms"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "$<IF:$<CONFIG:Debug>,${_qt_plugin_dbg},${_qt_plugin_rel}>"
              "$<TARGET_FILE_DIR:seamcarve_app>/platforms/"
      COMMENT "Copying the Qt platform plugin next to seamcarve_app.exe")
  else()
    message(WARNING "Qt platform plugin not found at ${_qt_plugin_rel}: the app may fail to start with "
                    "'Could not find the Qt platform plugin windows'. Copy qwindows.dll into <exe folder>\\platforms\\ by hand.")
  endif()
endif()

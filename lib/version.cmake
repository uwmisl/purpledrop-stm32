# Based on git_watcher.cmake:
# https://raw.githubusercontent.com/andrew-hardin/cmake-git-version-tracking/master/git_watcher.cmake
#
# Released under the MIT License.
# https://raw.githubusercontent.com/andrew-hardin/cmake-git-version-tracking/master/LICENSE


# Short hand for converting paths to absolute.
macro(PATH_TO_ABSOLUTE var_name)
    get_filename_component(${var_name} "${${var_name}}" ABSOLUTE)
endmacro()

# Check that a required variable is set.
macro(CHECK_REQUIRED_VARIABLE var_name)
    if(NOT DEFINED ${var_name})
        message(FATAL_ERROR "The \"${var_name}\" variable must be defined.")
    endif()
    PATH_TO_ABSOLUTE(${var_name})
endmacro()

# Check that an optional variable is set, or, set it to a default value.
macro(CHECK_OPTIONAL_VARIABLE var_name default_value)
    if(NOT DEFINED ${var_name})
        set(${var_name} ${default_value})
    endif()
    PATH_TO_ABSOLUTE(${var_name})
endmacro()

CHECK_REQUIRED_VARIABLE(PRE_CONFIGURE_FILE)
CHECK_REQUIRED_VARIABLE(POST_CONFIGURE_FILE)
CHECK_OPTIONAL_VARIABLE(GIT_STATE_FILE "${CMAKE_BINARY_DIR}/git-state-hash")
CHECK_OPTIONAL_VARIABLE(GIT_WORKING_DIR "${CMAKE_SOURCE_DIR}")

# Check the optional git variable.
# If it's not set, we'll try to find it using the CMake packaging system.
if(NOT DEFINED GIT_EXECUTABLE)
    find_package(Git QUIET REQUIRED)
endif()
CHECK_REQUIRED_VARIABLE(GIT_EXECUTABLE)

# Macro: RunGitCommand
# Description: short-hand macro for calling a git function. Outputs are the
#              "exit_code" and "output" variables.
macro(RunGitCommand)
    execute_process(COMMAND
        "${GIT_EXECUTABLE}" ${ARGV}
        WORKING_DIRECTORY "${_working_dir}"
        RESULT_VARIABLE exit_code
        OUTPUT_VARIABLE output
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT exit_code EQUAL 0)
        set(ENV{GIT_RETRIEVED_STATE} "false")
    endif()
endmacro()


function(UpdateVersion)
    RunGitCommand(describe --tags --always --dirty)
    if(exit_code EQUAL 0)
      set(GIT_DESCRIBE ${output})
    endif()
    string(TIMESTAMP CURTIME "%Y-%m-%d %H:%M")

    set(VERSION_STRING "${GIT_DESCRIBE} ${CURTIME}")

    configure_file(${PRE_CONFIGURE_FILE} ${POST_CONFIGURE_FILE} @ONLY)
endfunction()


# Function: SetupVersionTarget
# Description: this function sets up custom commands that make the build system
#              check the state of git before every build. If the state has
#              changed, then a file is configured.
function(SetupVersionTarget)
    add_custom_target(update_version
        ALL
        DEPENDS ${PRE_CONFIGURE_FILE}
        BYPRODUCTS
            ${POST_CONFIGURE_FILE}
        COMMENT "Updating version string from git"
        COMMAND
            ${CMAKE_COMMAND}
            -D_BUILD_TIME_CHECK_GIT=TRUE
            -DGIT_WORKING_DIR=${GIT_WORKING_DIR}
            -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
            -DPRE_CONFIGURE_FILE=${PRE_CONFIGURE_FILE}
            -DPOST_CONFIGURE_FILE=${POST_CONFIGURE_FILE}
            -P "${CMAKE_CURRENT_LIST_FILE}")
endfunction()

# Function: Main
# Description: primary entry-point to the script. Functions are selected based
#              on whether it's configure or build time.
function(Main)
    if(_BUILD_TIME_CHECK_GIT)
        # Executes at build time
        UpdateVersion()
    else()
        # Executes at configure time.
        SetupVersionTarget()
    endif()
endfunction()

# And off we go...
Main()
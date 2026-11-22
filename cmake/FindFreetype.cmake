# ---------------------------------------------------------------------------
# FindFreetype override
# ---------------------------------------------------------------------------
# SFML builds FreeType from source (FetchContent) and defines the target
# Freetype::Freetype. RmlUi looks FreeType up with find_package(Freetype),
# and CMake's stock module would search the system for a second copy - two
# FreeTypes would clash at link time. When the target already exists, report
# it as found; otherwise fall back to the stock module.
# ---------------------------------------------------------------------------

if(TARGET Freetype::Freetype)
    FetchContent_GetProperties(freetype)

    set(FREETYPE_FOUND TRUE)
    set(Freetype_FOUND TRUE)
    set(FREETYPE_LIBRARIES Freetype::Freetype)
    set(FREETYPE_LIBRARY Freetype::Freetype)

    if(freetype_SOURCE_DIR)
        set(FREETYPE_INCLUDE_DIRS "${freetype_SOURCE_DIR}/include")
        set(FREETYPE_INCLUDE_DIR_ft2build "${freetype_SOURCE_DIR}/include")
        set(FREETYPE_INCLUDE_DIR_freetype2 "${freetype_SOURCE_DIR}/include")

        # FREETYPE_VERSION_STRING, as the stock module reads it.
        set(_meteor_ft_header "${freetype_SOURCE_DIR}/include/freetype/freetype.h")
        if(EXISTS "${_meteor_ft_header}")
            file(STRINGS "${_meteor_ft_header}" _meteor_ft_version
                 REGEX "^#[\t ]*define[\t ]+FREETYPE_(MAJOR|MINOR|PATCH)[\t ]+[0-9]+$")
            foreach(_part MAJOR MINOR PATCH)
                string(REGEX REPLACE ".*FREETYPE_${_part}[\t ]+([0-9]+).*" "\\1" _value "${_meteor_ft_version}")
                set(_meteor_ft_${_part} "${_value}")
            endforeach()
            set(FREETYPE_VERSION_STRING "${_meteor_ft_MAJOR}.${_meteor_ft_MINOR}.${_meteor_ft_PATCH}")
            set(Freetype_VERSION "${FREETYPE_VERSION_STRING}")
        endif()
    endif()
    return()
endif()

include("${CMAKE_ROOT}/Modules/FindFreetype.cmake")

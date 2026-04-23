cmake_minimum_required(VERSION 3.26)

project(meteor_engine)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR}/src)

option(BUILD_EDITOR "Build editor" ON)
option(BUILD_PLAYER "Build player" ON)

set(PLAYER_NAME meteor_player)
set(EDITOR_NAME meteorite)

include(FetchContent)

# ---------------------------------------------------------------------------
# SFML
# ---------------------------------------------------------------------------
set(SFML_VERSION 3.0.2)
set(IMGUI_VERSION 1.91.9-docking)
set(IMGUI_SFML_VERSION 3.0)

# It's nice to get stripped-down release zips instead of cloning
# a repo and checking out a tag
FetchContent_Declare(
        SFML
        URL "https://github.com/SFML/SFML/archive/${SFML_VERSION}.zip"
)

# ---------------------------------------------------------------------------
# IMGUI
# ---------------------------------------------------------------------------
FetchContent_Declare(
        imgui
        URL "https://github.com/ocornut/imgui/archive/v${IMGUI_VERSION}.zip"
)

# ---------------------------------------------------------------------------
# IMGUI-SFML
# ---------------------------------------------------------------------------
FetchContent_Declare(
        imgui-sfml
        URL "https://github.com/SFML/imgui-sfml/archive/v${IMGUI_SFML_VERSION}.zip"
)

## Build

# SFML
option(SFML_BUILD_AUDIO "Build audio" OFF)
option(SFML_BUILD_NETWORK "Build network" OFF)
FetchContent_MakeAvailable(sfml)

# Dear ImGui
FetchContent_MakeAvailable(imgui)

# ImGui-SFML
set(IMGUI_DIR ${imgui_SOURCE_DIR})
option(IMGUI_SFML_FIND_SFML "Use find_package to find SFML" OFF)
option(IMGUI_SFML_IMGUI_DEMO "Build imgui_demo.cpp" ON)
FetchContent_MakeAvailable(imgui-sfml)

# ---------------------------------------------------------------------------
# SERIALIZED-CLASS
# ---------------------------------------------------------------------------
FetchContent_Declare(
        serialized_class
        GIT_REPOSITORY  https://github.com/vishnurajendran/serialized_class.git
        GIT_TAG  master
)
FetchContent_MakeAvailable(serialized_class)

# ---------------------------------------------------------------------------
# SPDLOG
# ---------------------------------------------------------------------------
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.12.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(spdlog)

# ---------------------------------------------------------------------------
# GLM
# ---------------------------------------------------------------------------
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG master
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

# ---------------------------------------------------------------------------
# STB
# ---------------------------------------------------------------------------
FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# ---------------------------------------------------------------------------
# IMGUIZMO
# ---------------------------------------------------------------------------
FetchContent_Declare(
        imguizmo
        GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imguizmo)

add_library(imguizmo STATIC ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp)
target_include_directories(imguizmo PUBLIC
        ${imguizmo_SOURCE_DIR}
        ${imgui_SOURCE_DIR}
)

target_link_libraries(imguizmo PRIVATE ImGui-SFML::ImGui-SFML)

# ---------------------------------------------------------------------------
# PUGIXML
# ---------------------------------------------------------------------------
FetchContent_Declare(
        pugixml
        GIT_REPOSITORY https://github.com/zeux/pugixml.git
        GIT_TAG v1.15
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(pugixml)

# ---------------------------------------------------------------------------
# Vendor Dependencies (GLEW + ASSIMP...)
# ---------------------------------------------------------------------------

# My Libs for Glew and Assimp are stored in this directory using Msys2

include_directories(C:/msys64/mingw64/include)
find_library(ASSIMP_LIBRARY assimp PATHS C:/msys64/mingw64/lib)
include_directories(C:/msys64/mingw64/include)

set(GLEW_INCLUDE_DIR "C:/msys64/mingw64/include")
set(GLEW_LIBRARY "C:/msys64/mingw64/lib/libglew32.dll.a")

include_directories(${GLEW_INCLUDE_DIR})

# ---------------------------------------------------------------------------
# CORE SOURCES (UNCHANGED)
# ---------------------------------------------------------------------------
set(CORE_SOURCES
        src/main.cpp
        src/core/object/object.cpp
        src/core/object/object.h
        src/core/utils/logger.cpp
        src/core/utils/logger.h
        src/core/window/simple/windowing.cpp
        src/core/window/simple/windowing.h
        src/core/object/gc.cpp
        src/core/object/gc.h
        src/core/utils/sstring.h
        src/core/utils/sstring_hash.h
        src/core/utils/guid.cpp
        src/core/utils/guid.h
        src/core/object/objectpointer.h
        src/core/meteor_core.h
        src/core/application/application.cpp
        src/core/application/application.h
        src/editor/window/imgui/imguistyles.h
        src/core/utils/color.cpp
        src/core/utils/color.h
        src/core/utils/dpihelper.h
        src/core/graphics/core/render-pipeline/stages/legacy_render_stage/graphicsrenderer.cpp
        src/core/graphics/core/render-pipeline/stages/legacy_render_stage/graphicsrenderer.h
        src/core/graphics/core/render-pipeline/stages/legacy_render_stage/sfmldrawcall.cpp
        src/core/graphics/core/render-pipeline/stages/legacy_render_stage/sfmldrawcall.h
        src/core/graphics/core/render-pipeline/stages/legacy_render_stage/drawcall.cpp
        src/core/graphics/core/render-pipeline/stages/legacy_render_stage/drawcall.h
        src/core/graphics/core/render-pipeline/stages/legacy_render_stage/opengldrawcall.cpp
        src/core/graphics/core/render-pipeline/stages/legacy_render_stage/opengldrawcall.h
        src/core/utils/glmhelper.h
        src/core/utils/glmhelper.cpp
        src/core/engine/entities/spatial/spatial.cpp
        src/core/engine/entities/spatial/spatial.h
        src/core/engine/entities/spatial/entityflags.h
        src/core/engine/scene/scene.cpp
        src/core/engine/scene/scene.h
        src/core/engine/scene/serialisation/xmlserialisable.h
        src/core/engine/scene/serialisation/sceneentitytypedeserializer.cpp
        src/core/engine/scene/serialisation/sceneentitytypedeserializer.h
        src/core/engine/scene/serialisation/sceneentitytypemap.h
        src/core/engine/scene/serialisation/sceneentitytypemap.cpp
        src/core/utils/str_extensions.h
        src/core/utils/serialisation_utils.h
        src/core/engine/scene/scenemanager.cpp
        src/core/engine/scene/scenemanager.h
        src/core/engine/assetmanagement/assetmanager/assetmanager.cpp
        src/core/engine/assetmanagement/assetmanager/assetmanager.h
        src/core/engine/assetmanagement/asset/asset.cpp
        src/core/engine/assetmanagement/asset/asset.h
        src/core/engine/assetmanagement/textasset/textasset.cpp
        src/core/engine/assetmanagement/textasset/textasset.h
        src/core/engine/scene/sceneasset.cpp
        src/core/engine/scene/sceneasset.h
        src/core/utils/fileio.h
        src/core/utils/fileio.cpp
        src/core/utils/serialisation_utils.cpp
        src/core/engine/entities/spatial/spatialdeserialiser.cpp
        src/core/engine/entities/spatial/spatialdeserialiser.h
        src/core/engine/texture/texture.cpp
        src/core/engine/texture/texture.h
        src/core/engine/assetmanagement/assetmanager/assetimporter.h
        src/core/engine/assetmanagement/assetmanager/assetimporter.cpp
        src/core/engine/assetmanagement/textasset/textassetimporter.cpp
        src/core/engine/assetmanagement/textasset/textassetimporter.h
        src/core/engine/scene/sceneassetimporter.cpp
        src/core/engine/scene/sceneassetimporter.h
        src/core/utils/glmhelper.cpp
        src/core/utils/glmhelper.h
        src/core/engine/3d/staticmesh/staticmeshasset.cpp
        src/core/engine/3d/staticmesh/staticmeshasset.h
        src/core/engine/3d/staticmesh/MStaticMeshAssetImporter.cpp
        src/core/engine/3d/staticmesh/MStaticMeshAssetImporter.h
        src/core/engine/3d/staticmesh/staticmesh.cpp
        src/core/engine/3d/staticmesh/staticmesh.h
        src/core/engine/3d/staticmesh/staticmeshdrawcall.cpp
        src/core/engine/3d/staticmesh/staticmeshdrawcall.h
        src/core/graphics/core/shader/shader.cpp
        src/core/graphics/core/shader/shader.h
        src/core/graphics/core/shader/shadercompiler.cpp
        src/core/graphics/core/shader/shadercompiler.h
        src/core/graphics/core/shader/shaderasset.cpp
        src/core/graphics/core/shader/shaderasset.h
        src/core/graphics/core/shader/shaderassetimporter.cpp
        src/core/graphics/core/shader/shaderassetimporter.h
        src/core/graphics/core/material/material.cpp
        src/core/graphics/core/material/material.h
        src/core/engine/camera/camera.cpp
        src/core/engine/camera/camera.h
        src/core/engine/camera/viewmanagement.cpp
        src/core/engine/camera/viewmanagement.h
        src/core/engine/camera/cameradeserialiser.cpp
        src/core/engine/camera/cameradeserialiser.h
        src/core/engine/3d/staticmesh/staticmeshentity.cpp
        src/core/engine/3d/staticmesh/staticmeshentity.h
        src/core/graphics/core/render-pipeline/interfaces/drawable_interface.h
        src/core/graphics/core/render-pipeline/render_queue.h
        src/core/graphics/core/render-pipeline/render_queue.cpp
        src/core/graphics/core/material/MMaterialAsset.cpp
        src/core/graphics/core/material/MMaterialAsset.h
        src/core/engine/assetmanagement/asset/defferedloadableasset.h
        src/core/graphics/core/material/materialassetimporter.cpp
        src/core/graphics/core/material/materialassetimporter.h
        src/core/engine/3d/staticmesh/staticmeshentitydeserialiser.cpp
        src/core/engine/3d/staticmesh/staticmeshentitydeserialiser.h
        src/core/engine/texture/textureasset.cpp
        src/core/engine/texture/textureasset.h
        src/core/engine/texture/textureassetimporter.cpp
        src/core/engine/texture/textureassetimporter.h
        src/core/engine/lighting/light_entity.cpp
        src/core/engine/lighting/light_entity.h
        src/core/graphics/core/render-pipeline/stages/lighting/light_type.h
        src/core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.cpp
        src/core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h
        src/core/engine/lighting/ambient/ambient_light.cpp
        src/core/engine/lighting/ambient/ambient_light.h
        src/core/engine/lighting/ambient/ambient_light_gpu_struct.h
        src/core/engine/lighting/ambient/ambient_light_entity_deserialiser.cpp
        src/core/engine/lighting/ambient/ambient_light_entity_deserialiser.h
        src/core/engine/lighting/directional/directional_light.cpp
        src/core/engine/lighting/directional/directional_light.h
        src/core/engine/lighting/directional/directional_light_gpu_struct.h
        src/core/engine/lighting/directional/directional_light_entity_deserialiser.cpp
        src/core/engine/lighting/directional/directional_light_entity_deserialiser.h
        src/core/engine/skybox/cubemaptexture.cpp
        src/core/engine/skybox/cubemaptexture.h
        src/core/engine/skybox/skybox.cpp
        src/core/engine/skybox/skybox.h
        src/core/graphics/core/render-pipeline/stages/skybox/skyboxdrawcall.cpp
        src/core/graphics/core/render-pipeline/stages/skybox/skyboxdrawcall.h
        src/core/engine/skybox/cubemapasset.cpp
        src/core/engine/skybox/cubemapasset.h
        src/core/engine/skybox/cubemapassetimporter.cpp
        src/core/engine/skybox/cubemapassetimporter.h
        src/core/engine/skybox/skyboxentitydeserialiser.cpp
        src/core/engine/skybox/skyboxentitydeserialiser.h
        src/core/engine/gizmos/gizmos.cpp
        src/core/engine/gizmos/gizmos.h
        src/core/graphics/core/render-pipeline/stages/lighting/lightbvh/light_bounds.h
        src/core/engine/lighting/dynamiclights/point_light/point_light.cpp
        src/core/engine/lighting/dynamiclights/point_light/point_light.h
        src/core/graphics/core/render-pipeline/stages/lighting/lightbvh/light_bounds.h
        src/core/graphics/core/render-pipeline/stages/lighting/lightbvh/light_bvh_node.h
        src/core/utils/aabb.h
        src/core/utils/aabb.cpp
        src/core/graphics/core/render-pipeline/stages/lighting/lightbvh/lightscene.cpp
        src/core/graphics/core/render-pipeline/stages/lighting/lightbvh/lightscene.h
        src/core/engine/lighting/dynamiclights/dynamic_light.cpp
        src/core/engine/lighting/dynamiclights/dynamic_light.h
        src/core/engine/lighting/dynamiclights/point_light/point_light.cpp
        src/core/engine/lighting/dynamiclights/point_light/point_light.h
        src/core/graphics/core/render-pipeline/stages/lighting/dynamic_light_data.h
        src/core/graphics/core/render-pipeline/stages/lighting/dynamic_light_datatype.h
        src/core/engine/lighting/dynamiclights/point_light/point_light_entity_deserialiser.cpp
        src/core/engine/lighting/dynamiclights/point_light/point_light_entity_deserialiser.h
        src/core/engine/lighting/dynamiclights/spot_light/spot_light.cpp
        src/core/engine/lighting/dynamiclights/spot_light/spot_light.h
        src/core/engine/lighting/dynamiclights/spot_light/spot_light_entitydeserializer.cpp
        src/core/engine/lighting/dynamiclights/spot_light/spot_light_entitydeserializer.h
        src/core/engine/lighting/dynamic_light_shader_container.h
        src/core/graphics/core/render-pipeline/stages/lighting/light_shader_constants.h
        src/core/graphics/core/render-pipeline/render_pipeline.cpp
        src/core/graphics/core/render-pipeline/render_pipeline.h
        src/core/graphics/core/render-pipeline/buffer_registry.cpp
        src/core/graphics/core/render-pipeline/buffer_registry.h
        src/core/graphics/core/render-pipeline/buffers/depth/depthbuffer.cpp
        src/core/graphics/core/render-pipeline/buffers/depth/depthbuffer.h
        src/core/graphics/core/render-pipeline/buffer.h
        src/core/graphics/core/render-pipeline/stages/render_stage.cpp
        src/core/graphics/core/render-pipeline/stages/render_stage.h
        src/core/graphics/core/render-pipeline/stages/depth/depth_render_stage.cpp
        src/core/graphics/core/render-pipeline/stages/depth/depth_render_stage.h
        src/core/graphics/core/render-pipeline/buffers/render/renderbuffer.cpp
        src/core/graphics/core/render-pipeline/buffers/render/renderbuffer.h
        src/core/graphics/core/render-pipeline/render_pipeline_manager.cpp
        src/core/graphics/core/render-pipeline/render_pipeline_manager.h
        src/core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h
        src/core/graphics/core/render-pipeline/interfaces/render_stage_interface.h
        src/core/graphics/core/render-pipeline/render_item.h
        src/core/graphics/core/render-pipeline/interfaces/render_item_collector.h
        src/core/graphics/core/render-pipeline/buffers/buffer_names.cpp
        src/core/graphics/core/render-pipeline/stages/composite/composite_stage.cpp
        src/core/graphics/core/render-pipeline/stages/composite/composite_stage.h
        src/core/graphics/core/render-pipeline/stages/skybox/skybox_queue.cpp
        src/core/graphics/core/render-pipeline/stages/skybox/skybox_queue.h
        src/core/graphics/core/render-pipeline/stages/skybox/skybox_stage.cpp
        src/core/graphics/core/render-pipeline/stages/skybox/skybox_stage.h
        src/core/graphics/core/render-pipeline/buffers/buffer_names.h
        src/core/graphics/core/render-pipeline/buffers/frame/frame_buffer.cpp
        src/core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h
        src/core/graphics/core/render-pipeline/stages/opaque/opaque_stage.cpp
        src/core/graphics/core/render-pipeline/stages/opaque/opaque_stage.h
        src/core/graphics/core/render-pipeline/stages/lighting/lighting_stage.cpp
        src/core/graphics/core/render-pipeline/stages/lighting/lighting_stage.h
        src/core/graphics/core/render-pipeline/buffers/shadow/shadow_buffer.cpp
        src/core/graphics/core/render-pipeline/buffers/shadow/shadow_buffer.h
        src/core/graphics/core/render-pipeline/stages/shadow/shadow_stage.cpp
        src/core/graphics/core/render-pipeline/stages/shadow/shadow_stage.h
        src/core/engine/scene/serialisation/scene_serialisation_map.cpp
        src/core/engine/scene/serialisation/scene_serialisation_map.h
        src/core/engine/scene/scene_io.cpp
        src/core/engine/scene/scene_io.h
        src/core/engine/skybox/procedural_sky/procedural_sky.cpp
        src/core/engine/skybox/procedural_sky/procedural_sky.h
        src/core/engine/skybox/procedural_sky/procedural_skybox_drawcall.cpp
        src/core/engine/skybox/procedural_sky/procedural_skybox_drawcall.h
        src/core/graphics/core/render-pipeline/stages/skybox/procedural_sky/procedural_sky_stage.cpp
        src/core/graphics/core/render-pipeline/stages/skybox/procedural_sky/procedural_sky_stage.h
        src/core/engine/skybox/procedural_sky/procedural_skybox_queue.cpp
        src/core/engine/skybox/procedural_sky/procedural_skybox_queue.h
        src/core/engine/settings/engine_settings.cpp
        src/core/engine/settings/engine_settings.h
        src/core/engine/engine_statics.cpp
        src/core/engine/engine_statics.h
)

set(EDITOR_SOURCES
        src/editor/editorscenemanager/editorscenemanager.cpp
        src/editor/editorscenemanager/editorscenemanager.h
        src/editor/app/editorapplication.cpp
        src/editor/app/editorapplication.h
        src/editor/window/imgui/imguiwindow.cpp
        src/editor/window/imgui/imguiwindow.h
        src/editor/window/imgui/imguisubwindow.cpp
        src/editor/window/imgui/imguisubwindow.h
        src/editor/window/imgui/imguisubwindowmanager.cpp
        src/editor/window/imgui/imguisubwindowmanager.h
        src/editor/window/imgui/imguiwindowconstants.h
        src/editor/editorwindows/console/editorconsolewindow.cpp
        src/editor/editorwindows/console/editorconsolewindow.h
        src/editor/meteorite.h
        src/editor/editorwindows/hierarchy/editorhierarchywindow.cpp
        src/editor/editorwindows/hierarchy/editorhierarchywindow.h
        src/editor/editorwindows/inspector/editorinspectorwindow.cpp
        src/editor/editorwindows/inspector/editorinspectorwindow.h
        src/editor/editorwindows/scene/editorsceneviewwindow.cpp
        src/editor/editorwindows/scene/editorsceneviewwindow.cpp
        src/editor/editorwindows/inspectordrawer/inspectordrawer.cpp
        src/editor/editorwindows/inspectordrawer/inspectordrawer.h
        src/editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.cpp
        src/editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h
        src/editor/editorwindows/inspectordrawer/cameradrawer/cameraentityinspectordrawer.cpp
        src/editor/editorwindows/inspectordrawer/cameradrawer/cameraentityinspectordrawer.h
        src/editor/window/imgui/editorcontrolsbuttons.cpp
        src/editor/window/imgui/editorcontrolsbuttons.h
        src/editor/window/menubar/menubaritem.h
        src/editor/window/menubar/menubaritem.cpp
        src/editor/window/menubar/menubartree.h
        src/editor/window/menubar/menubartree.cpp
        src/editor/window/menubar/default/create_empty_scene/create_empty_scene.cpp
        src/editor/window/menubar/default/create_empty_scene/create_empty_scene.h
        src/editor/window/menubar/default/quit_editor/quite_ditor.cpp
        src/editor/window/menubar/default/quit_editor/quit_editor.h
        src/editor/window/menubar/default/save_scene/save_scene.cpp
        src/editor/window/menubar/default/save_scene/save_scene.h
        src/editor/window/menubar/default/open_scene/open_scene.cpp
        src/editor/window/menubar/default/open_scene/open_scene.h
        src/editor/window/menubar/default/align_selected_to_view/align_selected_to_view.cpp
        src/editor/window/menubar/default/align_selected_to_view/align_selected_to_view.h
        src/editor/window/menubar/default/align_view_to_selected/align_view_to_selected.cpp
        src/editor/window/menubar/default/align_view_to_selected/align_view_to_selected.h
        src/editor/window/menubar/default/reset_camera_to_origin/reset_camera_to_origin.cpp
        src/editor/window/menubar/default/reset_camera_to_origin/reset_camera_to_origin.h
        src/editor/editorscenemanager/editorscenecamera.cpp
        src/editor/editorscenemanager/editorscenecamera.h
        src/editor/editorwindows/inspectordrawer/lights/directional_light/directionalight_inspector_drawer.cpp
        src/editor/editorwindows/inspectordrawer/lights/directional_light/directionalight_inspector_drawer.h
        src/editor/editorwindows/inspectordrawer/lights/ambient_light/ambientlight_inspector_drawer.cpp
        src/editor/editorwindows/inspectordrawer/lights/ambient_light/ambientlight_inspector_drawer.h
        src/editor/editorwindows/inspectordrawer/lights/point_light/point_light_inspector_drawer.cpp
        src/editor/editorwindows/inspectordrawer/lights/point_light/point_light_inspector_drawer.h
        src/editor/editorwindows/inspectordrawer/lights/spot_light/spot_light_inspector_drawer.cpp
        src/editor/editorwindows/inspectordrawer/lights/spot_light/spot_light_inspector_drawer.h
        src/editor/editorwindows/inspectordrawer/lights/point_light/point_light_inspector_drawer.cpp
        src/editor/editorwindows/inspectordrawer/lights/point_light/point_light_inspector_drawer.h
        src/editor/editorwindows/inspectordrawer/lights/spot_light/spot_light_inspector_drawer.cpp
        src/editor/editorwindows/inspectordrawer/lights/spot_light/spot_light_inspector_drawer.h
        src/editor/editorwindows/assetwindow/editorassetwindow.cpp
        src/editor/editorwindows/assetwindow/editorassetwindow.h
        src/editor/editorassetmanager/editorassetmanager.cpp
        src/editor/editorassetmanager/editorassetmanager.h
        src/editor/editorassetmanager/editor_asset_directory_node.cpp
        src/editor/editorassetmanager/editor_asset_directory_node.h
        src/editor/editorwindows/inspectordrawer/controls/asset_reference_controls.cpp
        src/editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h
        src/editor/editorwindows/inspectordrawer/static_mesh/static_mesh_inspector.cpp
        src/editor/editorwindows/inspectordrawer/static_mesh/static_mesh_inspector.h
        src/editor/editorwindows/inspectordrawer/controls/material_properties_controls.cpp
        src/editor/editorwindows/inspectordrawer/controls/material_properties_controls.h
        src/editor/window/menubar/default/add_entity/add_entity.cpp
        src/editor/window/menubar/default/add_entity/add_entity.h
        src/editor/editorwindows/inspectordrawer/skybox/procedural_sky/procedural_sky_inspector_drawer.cpp
        src/editor/editorwindows/inspectordrawer/skybox/procedural_sky/procedural_sky_inspector_drawer.h
)

set(PLAYER_SOURCES
        src/player/app/playerapplication.cpp
        src/player/app/playerapplication.h
)

# ---------------------------------------------------------------------------
# Editor
# ---------------------------------------------------------------------------
if (BUILD_EDITOR)

    add_executable(${EDITOR_NAME}
            ${CORE_SOURCES}
            ${EDITOR_SOURCES}
    )

    target_compile_definitions(${EDITOR_NAME} PRIVATE EDITOR_APPLICATION)

    target_link_libraries(${EDITOR_NAME} PRIVATE
            SFML::Graphics
            SFML::Window
            SFML::System

            ImGui-SFML::ImGui-SFML
            spdlog::spdlog
            glm::glm
            stb
            imguizmo

            ${GLEW_LIBRARY}
            ${ASSIMP_LIBRARY}
            pugixml
            serialized_class
    )
endif()

# ---------------------------------------------------------------------------
# Player
# ---------------------------------------------------------------------------
if (BUILD_PLAYER)

    add_executable(${PLAYER_NAME}
            ${CORE_SOURCES}
            ${PLAYER_SOURCES}
    )

    target_compile_definitions(${PLAYER_NAME} PRIVATE PLAYER_APPLICATION)

    target_link_libraries(${PLAYER_NAME} PRIVATE
            SFML::Graphics
            SFML::Window
            SFML::System

            ImGui-SFML::ImGui-SFML
            spdlog::spdlog
            glm::glm
            stb

            ${GLEW_LIBRARY}
            ${ASSIMP_LIBRARY}
            pugixml
            serialized_class
    )
endif()

# ---------------------------------------------------------------------------
# Platform-specific
# ---------------------------------------------------------------------------
if (APPLE)
    find_library(COREGRAPHICS_LIBRARY CoreGraphics)
    target_link_libraries(${EDITOR_NAME} PRIVATE ${COREGRAPHICS_LIBRARY})
endif()

if (UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(${EDITOR_NAME} PRIVATE X11)
endif()

# ---------------------------------------------------------------------------
# Copy system
# ---------------------------------------------------------------------------

function(add_copy_post_build target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Copying assets for ${target_name}..."

            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SOURCE_COPY_DIR}"
            "${DEST_COPY_DIR}"

            VERBATIM
    )
endfunction()

set(SOURCE_COPY_DIR "${CMAKE_SOURCE_DIR}/post-build-copy-to-bin")
set(DEST_COPY_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_copy_post_build(${EDITOR_NAME})
add_copy_post_build(${PLAYER_NAME})
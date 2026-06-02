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

# ---------------------------------------------------------------------------
# IMGUI-File Dialog  (aiekick/ImGuiFileDialog)
# ---------------------------------------------------------------------------
# The library has its own CMakeLists.txt that produces the ImGuiFileDialog
# target, but its find_package(imgui) check fails in a FetchContent setup.
# We point it at our already-fetched imgui source before MakeAvailable so
# the include path resolves correctly.
set(IMGUI_INCLUDE_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)

FetchContent_Declare(
        imgui-file-dialog
        GIT_REPOSITORY "https://github.com/aiekick/ImGuiFileDialog.git"
        GIT_TAG master
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui-file-dialog)

# Ensure imgui headers are visible even if the library's find_package missed them.
target_include_directories(ImGuiFileDialog PUBLIC ${imgui_SOURCE_DIR})

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

# ImGuizmo now ships its own CMakeLists.txt that creates the target automatically.
# We just patch the existing target with the extra includes/links we need.
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
# Mini Audio
# ---------------------------------------------------------------------------
FetchContent_Declare(
        miniaudio
        GIT_REPOSITORY https://github.com/mackron/miniaudio.git
        GIT_TAG 0.11.25
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(miniaudio)

#----------------------------------------------------------------------------
# Jolt Physics
#----------------------------------------------------------------------------

set(TARGET_UNIT_TESTS       OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD      OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES          OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER           OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG v5.5.0
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR Build
)

FetchContent_MakeAvailable(JoltPhysics)

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
        src/core/application/application.cpp
        src/core/application/application.h
        src/core/utils/color.cpp
        src/core/utils/color.h
        src/core/graphics/core/render-pipeline/draw_calls/drawcall.cpp
        src/core/graphics/core/render-pipeline/draw_calls/drawcall.h
        src/core/graphics/core/render-pipeline/draw_calls/opengldrawcall.cpp
        src/core/graphics/core/render-pipeline/draw_calls/opengldrawcall.h
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
        src/core/engine/camera/camera_spatial_entity.cpp
        src/core/engine/camera/camera_spatial_entity.h
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
        src/core/engine/entities/spatial/spatial_class_macros.h
        src/core/engine/entities/entity_type_registry.h
        src/core/engine/scene/serialisation/scene_serialiser.cpp
        src/core/engine/scene/serialisation/scene_serialiser.h
        src/core/object/type_info.h
        src/core/object/object_class_macros.h
        src/core/utils/field_engine_types.h
        src/core/engine/assetmanagement/asset/asset_handle.h
        src/core/graphics/core/shader/shader_utils.h
        src/core/graphics/core/render-pipeline/stages/clear/clear_stage.cpp
        src/core/graphics/core/render-pipeline/stages/clear/clear_stage.h
        src/core/engine/input/input.cpp
        src/core/engine/input/input.h
        src/editor/app/shortcut_listener.cpp
        src/editor/app/shortcut_listener.h
        src/core/engine/audio/interfaces/engine_interface.h
        src/core/engine/audio/interfaces/audiosource_interface.h
        src/core/engine/audio/interfaces/audiolistener_interface.h
        src/core/engine/audio/impl_miniaudio/audio_engine.cpp
        src/core/engine/audio/impl_miniaudio/audio_engine.h
        src/core/engine/audio/asset/audioclip_asset.cpp
        src/core/engine/audio/asset/audioclip_asset.h
        src/core/engine/audio/impl_miniaudio/audio_listener.cpp
        src/core/engine/audio/impl_miniaudio/audio_listener.h
        src/core/engine/audio/impl_miniaudio/audio_source.cpp
        src/core/engine/audio/impl_miniaudio/audio_source.h
        src/core/engine/subsystem/subsystem_interface.h
        src/core/engine/subsystem/subsystem_registry.cpp
        src/core/engine/subsystem/subsystem_registry.h
        src/core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h
        src/core/engine/assetmanagement/assetmanager/asset_manager_subsystem.cpp
        src/core/graphics/core/render-pipeline/render_pipeline_manager_interface.cpp
        src/core/graphics/core/render-pipeline/render_pipeline_manager_interface.h
        src/core/engine/audio/audio_entity/audio_source_entity.cpp
        src/core/engine/audio/audio_entity/audio_source_entity.h
        src/core/engine/audio/audio_entity/audio_listener_entity.cpp
        src/core/engine/audio/audio_entity/audio_listener_entity.h
        src/core/engine/audio/asset/audio_asset_importer.cpp
        src/core/engine/audio/asset/audio_asset_importer.h
        src/default_engine_icon_paths.h
        src/core/engine/assetmanagement/asset/asset_ref_handle.cpp
        src/core/engine/assetmanagement/asset/asset_ref_handle.h
        src/core/engine/assetmanagement/asset/field_asset_ref_types.h
        src/core/engine/audio/interfaces/audioclip_interface.h
        src/core/engine/audio/impl_miniaudio/audio_clip.cpp
        src/core/engine/audio/impl_miniaudio/audio_clip.h
        src/core/engine/physics/interface/physics_engine_interface.h
        src/core/engine/physics/interface/bodies/box/box_collision_body.h
        src/core/engine/physics/data/box_body_settings.h
        src/core/engine/physics/data/default_body_settings.h
        src/core/engine/physics/interface/bodies/sphere/sphere_collision_body.h
        src/core/engine/physics/data/sphere_body_settings.h
        src/core/engine/physics/interface/bodies/collision_body_interface.h
        src/core/engine/physics/impl/engine/jolt_physics_engine.h
        src/core/engine/physics/impl/contact_listener.h
        src/core/engine/physics/impl/body_activation_listener.h
        src/core/engine/physics/impl/engine/jolt_physics_engine.cpp
        src/core/engine/physics/impl/bodies/jolt_box_collision.cpp
        src/core/engine/physics/impl/bodies/jolt_box_collision.h
        src/core/engine/physics/impl/bodies/jolt_sphere_collision.cpp
        src/core/engine/physics/impl/bodies/jolt_sphere_collision.h
        src/core/engine/physics/impl/jolt_layers_filters.h
        src/core/engine/physics/entities/box_collision_body_entity.cpp
        src/core/engine/physics/entities/box_collision_body_entity.h
        src/core/engine/physics/entities/sphere_collision_body_entity.cpp
        src/core/engine/physics/entities/sphere_collision_body_entity.h
        src/core/engine/physics/data/collision_data.h
        src/core/engine/physics/callback/physics_callback_reciever.h
        src/core/engine/physics/callback/contact_listener.h
        src/core/engine/physics/impl/bodies/macros/jolt_base_collision_body_macros.h
        src/core/engine/physics/impl/jolt_body_interface.h
        src/core/engine/physics/data/shape_axis.h
        src/core/engine/physics/interface/bodies/cylinder/cylinder_collision_body.h
        src/core/engine/physics/interface/bodies/capsule/capsule_collision_body.h
        src/core/engine/physics/interface/bodies/mesh/mesh_collision_body.h
        src/core/engine/physics/interface/bodies/convexhull/convexhull_collsion_body.h
        src/core/engine/physics/data/cylinder_body_settings.h
        src/core/engine/physics/data/capsule_body_settings.h
        src/core/engine/physics/data/mesh_body_settings.h
        src/core/engine/physics/data/convexhull_body_settings.h
        src/core/engine/physics/impl/bodies/jolt_cylinder_collision.cpp
        src/core/engine/physics/impl/bodies/jolt_cylinder_collision.h
        src/core/engine/physics/impl/bodies/jolt_capsule_collision.cpp
        src/core/engine/physics/impl/bodies/jolt_capsule_collision.h
        src/core/engine/physics/impl/bodies/jolt_mesh_collision.cpp
        src/core/engine/physics/impl/bodies/jolt_mesh_collision.h
        src/core/engine/physics/impl/bodies/jolt_convexhull_collision.cpp
        src/core/engine/physics/impl/bodies/jolt_convexhull_collision.h
        src/core/engine/physics/entities/cylinder_collision_body_entity.cpp
        src/core/engine/physics/entities/cylinder_collision_body_entity.h
        src/core/engine/physics/entities/capsule_collision_body_entity.cpp
        src/core/engine/physics/entities/capsule_collision_body_entity.h
        src/core/engine/physics/entities/mesh_collision_body_entity.cpp
        src/core/engine/physics/entities/mesh_collision_body_entity.h
        src/core/engine/physics/entities/convexhull_collision_body_entity.cpp
        src/core/engine/physics/entities/convexhull_collision_body_entity.h
        src/core/engine/settings/physics_layers_settings.cpp
        src/core/engine/settings/physics_layers_settings.h
        src/core/engine/physics/data/layer_filter.cpp
        src/core/engine/physics/data/layer_filter.h
        src/core/engine/physics/data/raycast_data.h
        src/core/engine/physics/entities/collision_body_entity.cpp
        src/core/engine/physics/entities/collision_body_entity.h
        src/core/default_settings_paths.h
        src/core/profiling/simple_profiler/simple_profiler.h
        src/core/profiling/simple_profiler/simple_profiler.cpp
        src/core/profiling/default_profile_keys.h
        src/core/profiling/profiler.h
        src/core/graphics/core/render-pipeline/buffers/headless/headless_render_buffer.cpp
        src/core/graphics/core/render-pipeline/buffers/headless/headless_render_buffer.h
)

set(EDITOR_SOURCES
        src/editor/editorscenemanager/editorscenemanager.cpp
        src/editor/editorscenemanager/editorscenemanager.h
        src/editor/app/editorapplication.cpp
        src/editor/app/editorapplication.h
        src/editor/window/imgui/imguiwindow.h
        src/editor/window/imgui/imguiwindow.cpp
        src/editor/window/imgui/imguisubwindow.cpp
        src/editor/window/imgui/imguisubwindow.h
        src/editor/window/imgui/imguisubwindowmanager.cpp
        src/editor/window/imgui/imguisubwindowmanager.h
        src/editor/window/imgui/imguiwindowconstants.h
        src/editor/editorwindows/console/editorconsolewindow.cpp
        src/editor/editorwindows/console/editorconsolewindow.h
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
        src/editor/window/menubar/default/file/create_empty_scene/create_empty_scene.cpp
        src/editor/window/menubar/default/file/create_empty_scene/create_empty_scene.h
        src/editor/window/menubar/default/file/quit_editor/quite_ditor.cpp
        src/editor/window/menubar/default/file/quit_editor/quit_editor.h
        src/editor/window/menubar/default/file/save_scene/save_scene.cpp
        src/editor/window/menubar/default/file/save_scene/save_scene.h
        src/editor/window/menubar/default/file/open_scene/open_scene.cpp
        src/editor/window/menubar/default/file/open_scene/open_scene.h
        src/editor/window/menubar/default/edit/align_selected_to_view/align_selected_to_view.cpp
        src/editor/window/menubar/default/edit/align_selected_to_view/align_selected_to_view.h
        src/editor/window/menubar/default/edit/align_view_to_selected/align_view_to_selected.cpp
        src/editor/window/menubar/default/edit/align_view_to_selected/align_view_to_selected.h
        src/editor/window/menubar/default/edit/reset_camera_to_origin/reset_camera_to_origin.cpp
        src/editor/window/menubar/default/edit/reset_camera_to_origin/reset_camera_to_origin.h
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
        src/editor/helper/scene_io.cpp
        src/editor/helper/scene_io.h
        src/editor/editorassetmanager/hotreloadwatcher.cpp
        src/editor/editorassetmanager/hotreloadwatcher.h
        src/editor/editorassetmanager/asset_thumbnail_request.h
        src/editor/editorassetmanager/thumbnail_cache.h
        src/editor/editorassetmanager/thumbnail_cache.cpp
        src/editor/editorassetmanager/thumbnail_renderer.cpp
        src/editor/editorassetmanager/thumbnail_renderer.h
        src/editor/editorwindows/inspector/asset_inspector.cpp
        src/editor/editorwindows/inspector/asset_inspector.h
        src/editor/window/menubar/default/create_asset_items.cpp
        src/editor/window/menubar/default/create_asset_items.h
        src/editor/editorwindows/inspectordrawer/skybox/cubemap/skybox_inspector_drawer.cpp
        src/editor/editorwindows/inspectordrawer/skybox/cubemap/skybox_inspector_drawer.h
        src/editor/settings/editor_settings.cpp
        src/editor/settings/editor_settings.h
        src/editor/themes/editor_themes.cpp
        src/editor/themes/editor_themes.h
        src/editor/editorrenderstages/gizmos/gizmo_stage.cpp
        src/editor/editorrenderstages/gizmos/gizmo_stage.h
        src/editor/editorwindows/scene/scene_raycast.cpp
        src/editor/editorwindows/scene/scene_raycast.h
        src/editor/editorwindows/inspectordrawer/audio/audio_source_inspectordrawer.cpp
        src/editor/editorwindows/inspectordrawer/audio/audio_source_inspectordrawer.h
        src/editor/audio_helpers/audio_waveform.cpp
        src/editor/audio_helpers/audio_waveform.h
        src/editor/window/menubar/default/add_entity/menu_additem_macros.h
        src/editor/app/simulation_manager.cpp
        src/editor/app/simulation_manager.h
        src/editor/editor_constants.h
        src/editor/editorwindows/inspectordrawer/physics/box_collision_body_inspectordrawer.cpp
        src/editor/editorwindows/inspectordrawer/physics/box_collision_body_inspectordrawer.h
        src/editor/editorwindows/inspectordrawer/physics/sphere_collision_body_inspectordrawer.cpp
        src/editor/editorwindows/inspectordrawer/physics/sphere_collision_body_inspectordrawer.h
        src/editor/editorwindows/inspectordrawer/physics/cylinder_collision_body_inspectordrawer.cpp
        src/editor/editorwindows/inspectordrawer/physics/cylinder_collision_body_inspectordrawer.h
        src/editor/editorwindows/inspectordrawer/physics/capsule_collision_body_inspectordrawer.cpp
        src/editor/editorwindows/inspectordrawer/physics/capsule_collision_body_inspectordrawer.h
        src/editor/editorwindows/inspectordrawer/physics/mesh_collision_body_inspectordrawer.cpp
        src/editor/editorwindows/inspectordrawer/physics/mesh_collision_body_inspectordrawer.h
        src/editor/editorwindows/inspectordrawer/physics/convexhull_collision_body_inspectordrawer.cpp
        src/editor/editorwindows/inspectordrawer/physics/convexhull_collision_body_inspectordrawer.h
        src/editor/editorwindows/inspectordrawer/physics/collision_body_inspectordrawer.cpp
        src/editor/editorwindows/inspectordrawer/physics/collision_body_inspectordrawer.h
        src/editor/editor_utils/editor_utility.h
        src/editor/editor_utils/editor_utility.cpp
        src/editor/profiling/editor_profile_keys.h
        src/editor/profiling/profiler_stats_displayer.cpp
        src/editor/profiling/profiler_stats_displayer.h
        src/editor/window/menubar/default/help/open_documentation/open_documentation.cpp
        src/editor/window/menubar/default/help/open_documentation/open_documentation.h
        src/editor/editorwindows/editor_window_ids.h
)

set(PLAYER_SOURCES
        src/player/app/playerapplication.cpp
        src/player/app/playerapplication.h
)

# ---------------------------------------------------------------------------
# Editor
# ---------------------------------------------------------------------------
if (BUILD_EDITOR)

    add_compile_definitions(
            METEOR_EDITOR=1
    )

    add_executable( ${EDITOR_NAME}

            ${CORE_SOURCES}
            ${EDITOR_SOURCES}

            resource.rc
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
            ImGuiFileDialog
            miniaudio
            Jolt
    )
endif()

# ---------------------------------------------------------------------------
# Player
# ---------------------------------------------------------------------------
if (BUILD_PLAYER)

    add_compile_definitions(
            METEOR_PLAYER=1
    )

    add_executable( ${PLAYER_NAME}
            ${CORE_SOURCES}
            ${PLAYER_SOURCES}

            resource.rc
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
            miniaudio
            Jolt
    )
endif()

# ---------------------------------------------------------------------------
# Common Defines
# ---------------------------------------------------------------------------
add_compile_definitions(
        #VERBOSE_LOGGING=1
)


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

set(DEST_COPY_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
function(add_copy_post_build target_name source_dir)
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Copying assets for ${target_name}..."

            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${source_dir}"
            "${DEST_COPY_DIR}"

            VERBATIM
    )
endfunction()

set(COMMON_SOURCE_COPY_DIR "${CMAKE_SOURCE_DIR}/post-build-copy-to-bin/common")
set(EDITOR_SOURCE_COPY_DIR "${CMAKE_SOURCE_DIR}/post-build-copy-to-bin/editor")


if (BUILD_EDITOR)
    add_copy_post_build(${EDITOR_NAME} ${COMMON_SOURCE_COPY_DIR})
    add_copy_post_build(${EDITOR_NAME} ${EDITOR_SOURCE_COPY_DIR})
endif()

if (BUILD_PLAYER)
    add_copy_post_build(${PLAYER_NAME} ${COMMON_SOURCE_COPY_DIR})
endif()
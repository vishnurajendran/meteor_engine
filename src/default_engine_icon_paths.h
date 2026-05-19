//
// Created by ssj5v on 18-05-2026.
//

#ifndef EDITOR_ASSET_PATHS_H
#define EDITOR_ASSET_PATHS_H

struct SEditorAssetPaths
{
    // Editor Icons
    static constexpr const char* LOWRES_TEX_STATICMESH        = "meteor_assets/engine_assets/icons/lowres/static_mesh.png";
    static constexpr const char* LOWRES_TEX_DIRECTIONAL_LIGHT = "meteor_assets/engine_assets/icons/lowres/sun.png";
    static constexpr const char* LOWRES_TEX_SPOT_LIGHT        = "meteor_assets/engine_assets/icons/lowres/spot-light.png";
    static constexpr const char* LOWRES_TEX_POINT_LIGHT       = "meteor_assets/engine_assets/icons/lowres/point-light.png";
    static constexpr const char* LOWRES_TEX_AMBIENT_LIGHT     = "meteor_assets/engine_assets/icons/lowres/ambient-light.png";
    static constexpr const char* LOWRES_TEX_AUDIO_LISTENER    = "meteor_assets/engine_assets/icons/lowres/audio_listener.png";
    static constexpr const char* LOWRES_TEX_AUDIO_SOURCE      = "meteor_assets/engine_assets/icons/lowres/audio_source.png";
    static constexpr const char* LOWRES_TEX_SKY               = "meteor_assets/engine_assets/icons/lowres/sky.png";
    static constexpr const char* LOWRES_TEX_PROC_SKY          = "meteor_assets/engine_assets/icons/lowres/sky.png";
    static constexpr const char* LOWRES_TEX_CAMERA            = "meteor_assets/engine_assets/icons/lowres/camera.png";
    static constexpr const char* LOWRES_TEX_SCENE             = "meteor_assets/engine_assets/icons/lowres/scene.png";
    static constexpr const char* LOWRES_TEX_SPATIAL           = "meteor_assets/engine_assets/icons/lowres/spatial.png";

    // Editor Button Icons
    static constexpr const char* LOWRES_TEX_BTTN_PLAY             = "meteor_assets/engine_assets/icons/lowres/play.png";
    static constexpr const char* LOWRES_TEX_BTTN_MOVE             = "meteor_assets/engine_assets/icons/lowres/gizmo_translate.png";
    static constexpr const char* LOWRES_TEX_BTTN_ROTATE           = "meteor_assets/engine_assets/icons/lowres/gizmo_rotate.png";
    static constexpr const char* LOWRES_TEX_BTTN_SCALE            = "meteor_assets/engine_assets/icons/lowres/gizmo_scale.png";
    static constexpr const char* LOWRES_TEX_BTTN_GIZMO_LOCALSPACE = "meteor_assets/engine_assets/icons/lowres/gizmo_local.png";
    static constexpr const char* LOWRES_TEX_BTTN_GIZMO_WORLDSPACE = "meteor_assets/engine_assets/icons/lowres/gizmo_world.png";
    static constexpr const char* LOWRES_TEX_BTTN_GIZMO_ENABLED    = "meteor_assets/engine_assets/icons/lowres/gizmo_visible.png";
    static constexpr const char* LOWRES_TEX_BTTN_GIZMO_DISABLED   = "meteor_assets/engine_assets/icons/lowres/gizmo_hidden.png";


    // Asset Icons
    static constexpr const char* HIGHRES_TEX_ASSET_SCENE        = "meteor_assets/engine_assets/icons/highres/file-scene.png";
    static constexpr const char* HIGHRES_TEX_ASSET_STATICMESH   = "meteor_assets/engine_assets/icons/highres/file-glb.png";
    static constexpr const char* HIGHRES_TEX_ASSET_MATERIAL     = "meteor_assets/engine_assets/icons/highres/file-material.png";
    static constexpr const char* HIGHRES_TEX_ASSET_SHADER       = "meteor_assets/engine_assets/icons/highres/file-shader.png";
    static constexpr const char* HIGHRES_TEX_ASSET_SKYBOX       = "meteor_assets/engine_assets/icons/highres/file-skybox.png";
    static constexpr const char* HIGHRES_TEX_ASSET_TEXT         = "meteor_assets/engine_assets/icons/highres/file-text.png";
    static constexpr const char* HIGHRES_TEX_ASSET_AUDIOCLIP    = "meteor_assets/engine_assets/icons/highres/file-sound.png";
    static constexpr const char* HIGHRES_TEX_ASSET_FOLDER       = "meteor_assets/engine_assets/icons/highres/folder.png";
    static constexpr const char* HIGHRES_TEX_ASSET_DEFAULT      = "meteor_assets/engine_assets/icons/highres/file-default.png";

    // Gizmo Icons
    static constexpr const char* HIGHRES_TEX_GIZMOS_DIRECTIONAL_LIGHT  = "meteor_assets/engine_assets/icons/highres/sun_light.png";
    static constexpr const char* HIGHRES_TEX_GIZMOS_AMBIENT_LIGHT      = "meteor_assets/engine_assets/icons/highres/area_light.png";
    static constexpr const char* HIGHRES_TEX_GIZMOS_POINT_LIGHT        = "meteor_assets/engine_assets/icons/highres/point_light.png";
    static constexpr const char* HIGHRES_TEX_GIZMOS_SPOT_LIGHT         = "meteor_assets/engine_assets/icons/highres/spot_light.png";
    static constexpr const char* HIGHRES_TEX_GIZMOS_CAMERA             = "meteor_assets/engine_assets/icons/highres/camera.png";
    static constexpr const char* HIGHRES_TEX_GIZMOS_SKYBOX             = "meteor_assets/engine_assets/icons/highres/skybox.png";
    static constexpr const char* HIGHRES_TEX_GIZMOS_PROC_SKYBOX        = "meteor_assets/engine_assets/icons/highres/skybox_procedural.png";
    static constexpr const char* HIGHRES_TEX_GIZMOS_AUDIO_LISTENER     = "meteor_assets/engine_assets/icons/highres/audio-listener.png";
    static constexpr const char* HIGHRES_TEX_GIZMOS_AUDIO_SOURCE_2D    = "meteor_assets/engine_assets/icons/highres/audio-source-2d.png";
    static constexpr const char* HIGHRES_TEX_GIZMOS_AUDIO_SOURCE_3D    = "meteor_assets/engine_assets/icons/highres/audio-source-3d.png";
};

#endif //EDITOR_ASSET_PATHS_H

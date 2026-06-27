import unreal


MAP_PATH = "/Game/Maps/L_Hub"
ACTOR_LABEL = "TableVisionDirector"
LEGACY_ACTOR_LABELS = ["SDTableVisionDirector"]
DARKNESS_MATERIAL_PATH = "/Game/ArtTone/M_PP_TableVisionWorldRange.M_PP_TableVisionWorldRange"
FALLBACK_LOCATION = unreal.Vector(650.0, 1932.0, 90.0)


def find_actor_by_label(label):
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def find_location():
    center_actor = find_vision_center_actor()
    if center_actor:
        location = center_actor.get_actor_location()
        if center_actor.get_actor_label() == "SM_carddummyMesh2":
            return unreal.Vector(location.x, location.y, location.z)
        return unreal.Vector(location.x, location.y, location.z + 70.0)
    return FALLBACK_LOCATION


def find_vision_center_actor():
    preferred_labels = ["SM_carddummyMesh2", "BarTable", "BarTable2"]
    for preferred_label in preferred_labels:
        actor = find_actor_by_label(preferred_label)
        if actor:
            return actor
    return None


def set_property_safe(obj, property_name, value):
    try:
        obj.set_editor_property(property_name, value)
        return True
    except Exception as error:
        unreal.log_warning(f"[TableVisionPlacement] Could not set {property_name}: {error}")
        return False


def remove_duplicate_directors(keeper):
    for actor in list(unreal.EditorLevelLibrary.get_all_level_actors()):
        if actor == keeper:
            continue
        if actor.get_actor_label() in LEGACY_ACTOR_LABELS:
            unreal.log(f"[TableVisionPlacement] Removing duplicate {actor.get_actor_label()}")
            unreal.EditorLevelLibrary.destroy_actor(actor)


def main():
    unreal.EditorLevelLibrary.load_level(MAP_PATH)

    actor_class = unreal.load_class(None, "/Script/ShowDown.SDTableVisionDirector")
    if not actor_class:
        raise RuntimeError("Could not load /Script/ShowDown.SDTableVisionDirector")

    existing_actor = find_actor_by_label(ACTOR_LABEL)
    location = find_location()
    center_actor = find_vision_center_actor()

    if existing_actor:
        director = existing_actor
        director.set_actor_location(location, False, False)
        unreal.log(f"[TableVisionPlacement] Moved existing {ACTOR_LABEL} to {location}")
    else:
        director = unreal.EditorLevelLibrary.spawn_actor_from_class(actor_class, location)
        director.set_actor_label(ACTOR_LABEL)
        unreal.log(f"[TableVisionPlacement] Spawned {ACTOR_LABEL} at {location}")

    remove_duplicate_directors(director)

    darkness_material = unreal.EditorAssetLibrary.load_asset(DARKNESS_MATERIAL_PATH)
    if not darkness_material:
        unreal.log_warning(f"[TableVisionPlacement] Could not load {DARKNESS_MATERIAL_PATH}")

    set_property_safe(director, "VisionCenterActor", center_actor)
    set_property_safe(director, "DarknessPostProcessMaterial", darkness_material)
    set_property_safe(director, "bEnableDarknessPostProcess", True)
    set_property_safe(director, "bAutoLoadDefaultDarknessMaterial", True)
    set_property_safe(director, "bUnboundPostProcess", True)
    set_property_safe(director, "bPreviewPostProcessInEditor", False)
    set_property_safe(director, "bPlayRevealOnBeginPlay", True)
    set_property_safe(director, "EditorPreviewRevealAlpha", 0.0)

    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("[TableVisionPlacement] Saved L_Hub with TableVisionDirector world-mask darkness")


if __name__ == "__main__":
    main()

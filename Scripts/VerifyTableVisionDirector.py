import unreal


MAP_PATH = "/Game/Maps/L_Hub"
ACTOR_LABEL = "TableVisionDirector"


def find_actor_by_label(label):
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def get_property_safe(obj, property_name):
    try:
        return obj.get_editor_property(property_name)
    except Exception as error:
        unreal.log_warning(f"[TableVisionVerify] Could not read {property_name}: {error}")
        return None


def main():
    unreal.EditorLevelLibrary.load_level(MAP_PATH)
    actor = find_actor_by_label(ACTOR_LABEL)
    if not actor:
        raise RuntimeError(f"{ACTOR_LABEL} was not found in {MAP_PATH}")

    material = get_property_safe(actor, "DarknessPostProcessMaterial")
    center_actor = get_property_safe(actor, "VisionCenterActor")
    location = actor.get_actor_location()
    material_path = material.get_path_name() if material else "None"
    center_label = center_actor.get_actor_label() if center_actor else "None"

    unreal.log(
        "[TableVisionVerify] "
        f"actor={actor.get_actor_label()} "
        f"location=({location.x:.1f},{location.y:.1f},{location.z:.1f}) "
        f"center={center_label} "
        f"material={material_path}"
    )


if __name__ == "__main__":
    main()

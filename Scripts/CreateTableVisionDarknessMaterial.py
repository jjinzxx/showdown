import unreal


ASSET_PATH = "/Game/ArtTone"
ASSET_NAME = "M_PP_TableVisionWorldRange"
FULL_ASSET_NAME = f"{ASSET_PATH}/{ASSET_NAME}"


def log(message):
    unreal.log(f"[TableVisionMaterial] {message}")


def make_expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(material, expression_class, x, y)


def make_scalar_param(material, name, default_value, x, y):
    node = make_expression(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", default_value)
    return node


def make_vector_param(material, name, default_value, x, y):
    node = make_expression(material, unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", default_value)
    return node


def connect(output_node, output_name, input_node, input_name):
    unreal.MaterialEditingLibrary.connect_material_expressions(output_node, output_name, input_node, input_name)


def set_blendable_location(material):
    if not hasattr(unreal, "BlendableLocation"):
        return

    for enum_name in ("BL_BEFORE_TONEMAPPING", "BL_BeforeTonemapping"):
        blendable_location = getattr(unreal.BlendableLocation, enum_name, None)
        if blendable_location is None:
            continue
        try:
            material.set_editor_property("blendable_location", blendable_location)
            return
        except Exception as error:
            log(f"Could not set blendable_location to {enum_name}: {error}")


def create_or_replace_material():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_library = unreal.EditorAssetLibrary

    if not editor_asset_library.does_directory_exist(ASSET_PATH):
        editor_asset_library.make_directory(ASSET_PATH)

    if editor_asset_library.does_asset_exist(FULL_ASSET_NAME):
        log(f"Deleting existing {FULL_ASSET_NAME}")
        editor_asset_library.delete_asset(FULL_ASSET_NAME)

    material = asset_tools.create_asset(
        ASSET_NAME,
        ASSET_PATH,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if not material:
        raise RuntimeError(f"Failed to create {FULL_ASSET_NAME}")

    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    set_blendable_location(material)

    scene_color = make_expression(material, unreal.MaterialExpressionSceneTexture, -980, -220)
    scene_color.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)

    world_position = make_expression(material, unreal.MaterialExpressionWorldPosition, -980, 160)
    vision_center = make_vector_param(
        material,
        "VisionCenter",
        unreal.LinearColor(650.0, 1932.0, 90.0, 1.0),
        -980,
        360,
    )

    vision_radius = make_scalar_param(material, "VisionRadius", 450.0, -980, 560)
    vision_feather = make_scalar_param(material, "VisionFeather", 120.0, -980, 720)
    darkness_strength = make_scalar_param(material, "DarknessStrength", 1.0, -980, 920)

    distance = make_expression(material, unreal.MaterialExpressionDistance, -520, 260)
    distance_minus_radius = make_expression(material, unreal.MaterialExpressionSubtract, -300, 360)
    feather_divide = make_expression(material, unreal.MaterialExpressionDivide, -80, 440)
    one_minus_mask = make_expression(material, unreal.MaterialExpressionOneMinus, 140, 440)
    visibility_mask = make_expression(material, unreal.MaterialExpressionSaturate, 360, 440)

    one_minus_darkness = make_expression(material, unreal.MaterialExpressionOneMinus, -520, 920)
    full_brightness = make_expression(material, unreal.MaterialExpressionConstant, -300, 780)
    full_brightness.set_editor_property("r", 1.0)
    brightness_multiplier = make_expression(material, unreal.MaterialExpressionLinearInterpolate, 600, 620)
    final_color = make_expression(material, unreal.MaterialExpressionMultiply, 860, 140)

    connect(world_position, "XYZ", distance, "A")
    connect(vision_center, "RGB", distance, "B")
    connect(distance, "", distance_minus_radius, "A")
    connect(vision_radius, "", distance_minus_radius, "B")
    connect(distance_minus_radius, "", feather_divide, "A")
    connect(vision_feather, "", feather_divide, "B")
    connect(feather_divide, "", one_minus_mask, "")
    connect(one_minus_mask, "", visibility_mask, "")

    connect(darkness_strength, "", one_minus_darkness, "")
    connect(one_minus_darkness, "", brightness_multiplier, "A")
    connect(full_brightness, "", brightness_multiplier, "B")
    connect(visibility_mask, "", brightness_multiplier, "Alpha")

    connect(scene_color, "Color", final_color, "A")
    connect(brightness_multiplier, "", final_color, "B")

    unreal.MaterialEditingLibrary.connect_material_property(
        final_color,
        "",
        unreal.MaterialProperty.MP_EMISSIVE_COLOR,
    )

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    editor_asset_library.save_asset(FULL_ASSET_NAME, only_if_is_dirty=False)
    log(f"Created {FULL_ASSET_NAME}")
    return material


if __name__ == "__main__":
    create_or_replace_material()

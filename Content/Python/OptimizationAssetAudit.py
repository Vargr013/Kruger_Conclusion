import json
import os
import unreal


MESH_PATHS = [
    "/Game/Assets/Human_Stuff/Car/LandyBase",
    "/Game/Assets/Human_Stuff/Car/LandyWheel_Pillow",
    "/Game/Assets/Human_Stuff/PepperGun/PepperGunBarrel",
    "/Game/Assets/Human_Stuff/PepperGun/PepperGunHandle",
]


def safe_property(obj, name, default=None):
    try:
        return obj.get_editor_property(name)
    except Exception:
        return default


def mesh_report(path, subsystem):
    mesh = unreal.EditorAssetLibrary.load_asset(path)
    if not mesh:
        return {"path": path, "loaded": False}
    body_setup = safe_property(mesh, "body_setup")
    nanite = safe_property(mesh, "nanite_settings")
    lod_count = subsystem.get_lod_count(mesh)
    vertices = []
    triangles = []
    for lod_index in range(lod_count):
        try:
            vertices.append(subsystem.get_number_vertices(mesh, lod_index))
        except Exception:
            vertices.append(None)
        try:
            triangles.append(subsystem.get_number_triangles(mesh, lod_index))
        except Exception:
            triangles.append(None)
    materials = []
    for slot in safe_property(mesh, "static_materials", []) or []:
        material = safe_property(slot, "material_interface")
        materials.append(material.get_path_name() if material else None)
    return {
        "path": path,
        "loaded": True,
        "lod_count": lod_count,
        "vertices": vertices,
        "triangles": triangles,
        "nanite_enabled": bool(safe_property(nanite, "enabled", False)) if nanite else False,
        "material_slots": materials,
        "collision_trace_flag": str(safe_property(body_setup, "collision_trace_flag")) if body_setup else None,
        "simple_collision_elements": len(safe_property(safe_property(body_setup, "agg_geom"), "box_elems", []) or [])
            + len(safe_property(safe_property(body_setup, "agg_geom"), "sphere_elems", []) or [])
            + len(safe_property(safe_property(body_setup, "agg_geom"), "sphyl_elems", []) or [])
            + len(safe_property(safe_property(body_setup, "agg_geom"), "convex_elems", []) or []),
    }


def substrate_report():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    material_assets = registry.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "Material"), True)
    authored = []
    scanned = 0
    for asset_data in material_assets:
        package_name = str(asset_data.package_name)
        if not package_name.startswith("/Game/"):
            continue
        material = asset_data.get_asset()
        if not material:
            continue
        scanned += 1
        try:
            expressions = unreal.MaterialEditingLibrary.get_material_expressions(material)
        except Exception:
            expressions = []
        substrate_nodes = [expr.get_class().get_name() for expr in expressions if "Substrate" in expr.get_class().get_name() or "Strata" in expr.get_class().get_name()]
        if substrate_nodes:
            authored.append({"material": material.get_path_name(), "nodes": sorted(set(substrate_nodes))})
    return {"materials_scanned": scanned, "authored_substrate_materials": authored}


def placed_import_report():
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    result = []
    for actor in actors:
        actor_text = actor.get_path_name().lower()
        components = actor.get_components_by_class(unreal.StaticMeshComponent)
        mesh_paths = []
        for component in components:
            mesh = safe_property(component, "static_mesh")
            if mesh:
                mesh_paths.append(mesh.get_path_name())
        if "peppergun" in actor_text or any("PepperGun" in path for path in mesh_paths) or any("LandCruiser" in path for path in mesh_paths):
            result.append({
                "actor": actor.get_path_name(),
                "class": actor.get_class().get_name(),
                "location": [actor.get_actor_location().x, actor.get_actor_location().y, actor.get_actor_location().z],
                "tick_enabled": actor.is_actor_tick_enabled(),
                "spatially_loaded": bool(safe_property(actor, "is_spatially_loaded", True)),
                "hlod_layer": str(safe_property(actor, "hlod_layer")),
                "meshes": mesh_paths,
            })
    return result


subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
report = {
    "meshes": [mesh_report(path, subsystem) for path in MESH_PATHS],
    "substrate": substrate_report(),
    "placed_imports": placed_import_report(),
}
output_path = os.path.join(unreal.Paths.project_saved_dir(), "Profiling", "OptimizationAssetAudit.json")
os.makedirs(os.path.dirname(output_path), exist_ok=True)
with open(output_path, "w", encoding="utf-8") as output_file:
    json.dump(report, output_file, indent=2)
unreal.log("OPTIMIZATION_ASSET_AUDIT=" + json.dumps(report))

"""Exercise the packaged GeoJSON FileFormat through OpenUSD.

SPDX-License-Identifier: Apache-2.0
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

from pxr import Plug, Sdf, Tf, Usd, UsdGeom


def open_stage(path: Path):
    layer = Sdf.Layer.FindOrOpen(str(path))
    return layer, Usd.Stage.Open(layer) if layer else None


def is_rejected(path: Path, arguments: dict | None = None) -> bool:
    """A refusal reaches Python either as a null layer or as a posted error."""
    try:
        if arguments is None:
            return not Sdf.Layer.FindOrOpen(str(path))
        return not Sdf.Layer.FindOrOpen(str(path), arguments)
    except Tf.ErrorException:
        return True


def fixture_path(fixtures: Path, name: str) -> Path:
    path = fixtures / name
    if not path.is_file():
        raise RuntimeError(f"packaged fixture is missing: {path}")
    return path


def verify_point_stage(stage) -> None:
    vector = stage.GetPrimAtPath("/Vector")
    points = stage.GetPrimAtPath("/Vector/Features/id_point_1")
    if not vector or not points or not points.IsA(UsdGeom.Points):
        raise RuntimeError("GeoJSON fixture did not author the expected Points prim")
    point_values = UsdGeom.Points(points).GetPointsAttr().Get() or []
    if len(point_values) != 1:
        raise RuntimeError("GeoJSON fixture authored an unexpected point count")
    feature_id = points.GetCustomDataByKey("vector").get("featureId")
    if feature_id != "point-1":
        raise RuntimeError("GeoJSON fixture did not preserve its feature id")
    properties = points.GetAttribute("vector:properties:name").Get()
    if properties != "origin":
        raise RuntimeError("GeoJSON fixture did not preserve its name property")
    vector_data = vector.GetCustomDataByKey("vector")
    if vector_data.get("format") != "GeoJSON":
        raise RuntimeError("GeoJSON format metadata is missing")
    if vector_data.get("localOrigin") != (0.0, 0.0, 0.0):
        raise RuntimeError("GeoJSON local-origin metadata is incorrect")


def verify_stage_policy(fixture: Path) -> dict:
    """Check the direct-read stage policy: default prim, unset stage metrics,
    explicit overrides, rejected values, and default-prim composition."""
    layer, stage = open_stage(fixture)
    if not stage:
        raise RuntimeError("stage-policy fixture did not open")
    if stage.GetDefaultPrim().GetPath() != Sdf.Path("/Vector"):
        raise RuntimeError("direct read did not author /Vector as the default prim")
    if stage.HasAuthoredMetadata("upAxis"):
        raise RuntimeError("direct read authored an up-axis without being asked")
    if UsdGeom.StageHasAuthoredMetersPerUnit(stage):
        raise RuntimeError("direct read authored stage units without being asked")

    arguments = {"upAxis": "z", "metersPerUnit": "0.001"}
    explicit_layer = Sdf.Layer.FindOrOpen(str(fixture), arguments)
    explicit_stage = Usd.Stage.Open(explicit_layer) if explicit_layer else None
    if not explicit_stage:
        raise RuntimeError("explicit stage-policy arguments did not open")
    if UsdGeom.GetStageUpAxis(explicit_stage) != UsdGeom.Tokens.z:
        raise RuntimeError("upAxis=z was not authored")
    if UsdGeom.GetStageMetersPerUnit(explicit_stage) != 0.001:
        raise RuntimeError("metersPerUnit=0.001 was not authored")
    if explicit_layer.identifier == layer.identifier:
        raise RuntimeError("stage-policy arguments did not change layer identity")

    for rejected in ({"upAxis": "x"}, {"metersPerUnit": "0"}, {"metersPerUnit": "far"}):
        if not is_rejected(fixture, rejected):
            raise RuntimeError(f"invalid stage-policy argument was accepted: {rejected}")

    composed = Usd.Stage.CreateInMemory()
    world = composed.DefinePrim("/World")
    world.GetReferences().AddReference(str(fixture))
    if not composed.GetPrimAtPath("/World/Features/id_point_1"):
        raise RuntimeError("default-prim reference did not compose the vector features")
    if world.GetCustomDataByKey("vector").get("format") != "GeoJSON":
        raise RuntimeError("default-prim reference did not compose dataset metadata")

    return {
        "defaultPrim": str(stage.GetDefaultPrim().GetPath()),
        "authoredUpAxis": False,
        "authoredMetersPerUnit": False,
        "explicitUpAxis": str(UsdGeom.GetStageUpAxis(explicit_stage)),
        "explicitMetersPerUnit": UsdGeom.GetStageMetersPerUnit(explicit_stage),
        "explicitLayerIdentifier": explicit_layer.identifier,
        "invalidStageArgumentsRejected": True,
        "defaultPrimComposed": True,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", type=Path, required=True)
    args = parser.parse_args()
    prefix = args.prefix.resolve()
    fixtures = prefix / "bundles/vector-geojson/tests/fixtures"
    fixture = fixture_path(fixtures, "basic.geojson")
    report = {
        "schema": 1,
        "component": "usd-vector-plugins",
        "fixture": str(fixture),
        "status": "failed",
    }
    try:
        plugin = Plug.Registry().GetPluginWithName("UsdVectorGeoJsonFileFormat")
        if not plugin or not plugin.Load():
            raise RuntimeError("vector-geojson plugin did not load")
        file_format = Sdf.FileFormat.FindByExtension("geojson")
        if not file_format or file_format.formatId != "GeoJSON":
            raise RuntimeError("vector-geojson is not registered for .geojson")
        layer, stage = open_stage(fixture)
        if not stage:
            raise RuntimeError("packaged GeoJSON fixture did not open")
        verify_point_stage(stage)

        json_layer, json_stage = open_stage(fixture_path(fixtures, "basic.json"))
        if not json_stage:
            raise RuntimeError("GeoJSON-bearing JSON fixture did not open")
        verify_point_stage(json_stage)

        if not is_rejected(fixture_path(fixtures, "unrelated.json")):
            raise RuntimeError("unrelated JSON was accepted as GeoJSON")

        if not is_rejected(fixture_path(fixtures, "invalid.geojson")):
            raise RuntimeError("invalid GeoJSON was accepted")

        stage_policy = verify_stage_policy(fixture)

        authored = layer.ExportToString()
        json_authored = json_layer.ExportToString()
        points = stage.GetPrimAtPath("/Vector/Features/id_point_1")
        vector = stage.GetPrimAtPath("/Vector")
        point_values = UsdGeom.Points(points).GetPointsAttr().Get() or []
        feature_id = points.GetCustomDataByKey("vector").get("featureId")
        properties = points.GetAttribute("vector:properties:name").Get()
        vector_data = vector.GetCustomDataByKey("vector")
        report.update({
            "status": "passed",
            "formatId": file_format.formatId,
            "layerIdentifier": layer.identifier,
            "authoredLayerDigest": "sha256:" + hashlib.sha256(authored.encode()).hexdigest(),
            "observations": {
                "registered": True,
                "opened": True,
                "jsonProbeOpened": True,
                "unrelatedJsonRejected": True,
                "invalidGeoJsonRejected": True,
                "pointCount": len(point_values),
                "featureId": feature_id,
                "propertyName": properties,
                "localOrigin": list(vector_data["localOrigin"]),
                "jsonAuthoredLayerDigest": "sha256:" + hashlib.sha256(json_authored.encode()).hexdigest(),
                "stagePolicy": stage_policy,
            },
        })
    except Exception as error:
        report["error"] = str(error)
    print(json.dumps(report, indent=2))
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    sys.exit(main())

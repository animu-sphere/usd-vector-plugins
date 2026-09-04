"""Exercise the packaged GeoJSON FileFormat through OpenUSD.

SPDX-License-Identifier: Apache-2.0
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

from pxr import Plug, Sdf, Usd, UsdGeom


def open_stage(path: Path):
    layer = Sdf.Layer.FindOrOpen(str(path))
    return layer, Usd.Stage.Open(layer) if layer else None


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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", type=Path, required=True)
    args = parser.parse_args()
    prefix = args.prefix.resolve()
    fixtures = prefix / "bundles/vector-geojson/tests/fixtures"
    fixture = fixtures / "basic.geojson"
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

        json_layer, json_stage = open_stage(fixtures / "basic.json")
        if not json_stage:
            raise RuntimeError("GeoJSON-bearing JSON fixture did not open")
        verify_point_stage(json_stage)

        unrelated_layer = Sdf.Layer.FindOrOpen(str(fixtures / "unrelated.json"))
        if unrelated_layer:
            raise RuntimeError("unrelated JSON was accepted as GeoJSON")

        invalid_layer = Sdf.Layer.FindOrOpen(str(fixtures / "invalid.geojson"))
        if invalid_layer:
            raise RuntimeError("invalid GeoJSON was accepted")

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
            },
        })
    except Exception as error:
        report["error"] = str(error)
    print(json.dumps(report, indent=2))
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    sys.exit(main())

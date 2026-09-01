#include "usdvector/authoring/triangulation.h"

#include "usdvector/core/validation.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <utility>

namespace usdvector::authoring {
namespace {

constexpr double kEpsilon = 1e-12;

Diagnostic Failure(std::string message, std::optional<std::uint32_t> ring = {}) {
    return {DiagnosticCode::PolygonTriangulationFailed,
            Severity::Error,
            std::move(message),
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            ring,
            std::nullopt,
            std::nullopt};
}

double Cross(const Coordinate& a, const Coordinate& b, const Coordinate& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

double SignedArea(const Ring& ring) {
    double area = 0.0;
    for (std::size_t index = 0; index < ring.size(); ++index) {
        const Coordinate& current = ring[index];
        const Coordinate& next = ring[(index + 1) % ring.size()];
        area += current.x * next.y - next.x * current.y;
    }
    return area / 2.0;
}

bool SamePoint(const Coordinate& left, const Coordinate& right) {
    return left.x == right.x && left.y == right.y;
}

bool OnSegment(const Coordinate& start, const Coordinate& end,
               const Coordinate& point) {
    return std::abs(Cross(start, end, point)) <= kEpsilon &&
           point.x >= std::min(start.x, end.x) - kEpsilon &&
           point.x <= std::max(start.x, end.x) + kEpsilon &&
           point.y >= std::min(start.y, end.y) - kEpsilon &&
           point.y <= std::max(start.y, end.y) + kEpsilon;
}

bool SegmentsIntersect(const Coordinate& a, const Coordinate& b,
                       const Coordinate& c, const Coordinate& d) {
    const double abC = Cross(a, b, c);
    const double abD = Cross(a, b, d);
    const double cdA = Cross(c, d, a);
    const double cdB = Cross(c, d, b);
    const bool separated = ((abC > kEpsilon && abD < -kEpsilon) ||
                            (abC < -kEpsilon && abD > kEpsilon)) &&
                           ((cdA > kEpsilon && cdB < -kEpsilon) ||
                            (cdA < -kEpsilon && cdB > kEpsilon));
    return separated || OnSegment(a, b, c) || OnSegment(a, b, d) ||
           OnSegment(c, d, a) || OnSegment(c, d, b);
}

bool PointInRing(const Coordinate& point, const Ring& ring) {
    bool inside = false;
    for (std::size_t index = 0; index < ring.size(); ++index) {
        const Coordinate& current = ring[index];
        const Coordinate& next = ring[(index + 1) % ring.size()];
        if (OnSegment(current, next, point)) {
            return true;
        }
        const bool crosses = ((current.y > point.y) != (next.y > point.y)) &&
                             (point.x < (next.x - current.x) *
                                                (point.y - current.y) /
                                                (next.y - current.y) +
                                            current.x);
        if (crosses) {
            inside = !inside;
        }
    }
    return inside;
}

bool BridgeIsVisible(const Coordinate& holePoint, const Coordinate& outerPoint,
                     const Ring& outer, const std::vector<Ring>& holes) {
    const Coordinate midpoint{(holePoint.x + outerPoint.x) / 2.0,
                               (holePoint.y + outerPoint.y) / 2.0};
    if (!PointInRing(midpoint, outer)) {
        return false;
    }
    for (const Ring& hole : holes) {
        if (PointInRing(midpoint, hole)) {
            return false;
        }
    }

    auto clearOfRing = [&](const Ring& ring) {
        for (std::size_t index = 0; index < ring.size(); ++index) {
            const Coordinate& start = ring[index];
            const Coordinate& end = ring[(index + 1) % ring.size()];
            if (!SegmentsIntersect(holePoint, outerPoint, start, end)) {
                continue;
            }
            const bool incident = SamePoint(start, holePoint) ||
                                  SamePoint(end, holePoint) ||
                                  SamePoint(start, outerPoint) ||
                                  SamePoint(end, outerPoint);
            if (!incident) {
                return false;
            }
        }
        return true;
    };

    if (!clearOfRing(outer)) {
        return false;
    }
    for (const Ring& hole : holes) {
        if (!clearOfRing(hole)) {
            return false;
        }
    }
    return true;
}

Result<Ring> OrientedRing(const Ring& ring, bool counterClockwise,
                          std::uint32_t ringIndex) {
    auto normalized = NormalizeRing(ring);
    if (!normalized.Succeeded()) {
        return Result<Ring>::Failure({Failure(
            "ring cannot be normalized for triangulation", ringIndex)});
    }
    Ring output = std::move(*normalized.value);
    const bool isCounterClockwise = SignedArea(output) > 0.0;
    if (isCounterClockwise != counterClockwise) {
        std::reverse(output.begin(), output.end());
    }
    if (std::abs(SignedArea(output)) <= kEpsilon) {
        return Result<Ring>::Failure(
            {Failure("ring has zero area", ringIndex)});
    }
    return Result<Ring>::Success(std::move(output));
}

bool IsStrictlyInside(const Coordinate& point, const Coordinate& a,
                      const Coordinate& b, const Coordinate& c) {
    const double first = Cross(a, b, point);
    const double second = Cross(b, c, point);
    const double third = Cross(c, a, point);
    return first > kEpsilon && second > kEpsilon && third > kEpsilon;
}

Result<std::vector<std::uint32_t>> MakeSimplePolygon(
    const Ring& outer, const std::vector<Ring>& holes,
    std::vector<Coordinate>& vertices) {
    std::vector<std::uint32_t> outerIndices;
    outerIndices.reserve(outer.size());
    for (const Coordinate& point : outer) {
        outerIndices.push_back(static_cast<std::uint32_t>(vertices.size()));
        vertices.push_back(point);
    }

    std::vector<std::vector<std::uint32_t>> holeIndices;
    holeIndices.reserve(holes.size());
    for (const Ring& hole : holes) {
        std::vector<std::uint32_t> indices;
        indices.reserve(hole.size());
        for (const Coordinate& point : hole) {
            indices.push_back(static_cast<std::uint32_t>(vertices.size()));
            vertices.push_back(point);
        }
        holeIndices.push_back(std::move(indices));
    }

    std::vector<std::uint32_t> polygon = outerIndices;
    for (std::size_t holeNumber = 0; holeNumber < holes.size(); ++holeNumber) {
        const Ring& hole = holes[holeNumber];
        const auto rightmost = std::min_element(
            hole.begin(), hole.end(), [](const Coordinate& left,
                                         const Coordinate& right) {
                if (left.x != right.x) {
                    return left.x > right.x;
                }
                return left.y < right.y;
            });
        const std::size_t holeStart =
            static_cast<std::size_t>(rightmost - hole.begin());
        const Coordinate& holePoint = *rightmost;

        std::vector<std::size_t> candidates(outer.size());
        std::iota(candidates.begin(), candidates.end(), 0);
        std::stable_sort(candidates.begin(), candidates.end(),
                         [&](std::size_t left, std::size_t right) {
                             const double leftDistance =
                                 std::pow(outer[left].x - holePoint.x, 2) +
                                 std::pow(outer[left].y - holePoint.y, 2);
                             const double rightDistance =
                                 std::pow(outer[right].x - holePoint.x, 2) +
                                 std::pow(outer[right].y - holePoint.y, 2);
                             return leftDistance < rightDistance;
                         });
        std::size_t outerIndex = outer.size();
        for (const std::size_t candidate : candidates) {
            if (BridgeIsVisible(holePoint, outer[candidate], outer, holes)) {
                outerIndex = candidate;
                break;
            }
        }
        if (outerIndex == outer.size()) {
            return Result<std::vector<std::uint32_t>>::Failure(
                {Failure("no visible bridge found for polygon hole",
                         static_cast<std::uint32_t>(holeNumber + 1))});
        }

        const std::uint32_t outerVertex = outerIndices[outerIndex];
        const std::vector<std::uint32_t>& currentHole = holeIndices[holeNumber];
        auto insertion = std::find(polygon.begin(), polygon.end(), outerVertex);
        if (insertion == polygon.end()) {
            return Result<std::vector<std::uint32_t>>::Failure(
                {Failure("polygon bridge anchor is missing")});
        }
        ++insertion;
        std::vector<std::uint32_t> bridged;
        bridged.reserve(polygon.size() + currentHole.size() + 2);
        bridged.insert(bridged.end(), polygon.begin(), insertion);
        for (std::size_t offset = 0; offset <= currentHole.size(); ++offset) {
            bridged.push_back(currentHole[(holeStart + offset) % currentHole.size()]);
        }
        bridged.push_back(outerVertex);
        bridged.insert(bridged.end(), insertion, polygon.end());
        polygon = std::move(bridged);
    }
    return Result<std::vector<std::uint32_t>>::Success(std::move(polygon));
}

Result<std::vector<Triangle>> EarClip(const std::vector<std::uint32_t>& polygon,
                                      const std::vector<Coordinate>& vertices) {
    std::vector<std::uint32_t> remaining = polygon;
    std::vector<Triangle> triangles;
    triangles.reserve(remaining.size() - 2);
    while (remaining.size() > 3) {
        bool clipped = false;
        for (std::size_t index = 0; index < remaining.size(); ++index) {
            const std::uint32_t previous =
                remaining[(index + remaining.size() - 1) % remaining.size()];
            const std::uint32_t current = remaining[index];
            const std::uint32_t next = remaining[(index + 1) % remaining.size()];
            const Coordinate& a = vertices[previous];
            const Coordinate& b = vertices[current];
            const Coordinate& c = vertices[next];
            if (Cross(a, b, c) <= kEpsilon) {
                continue;
            }

            bool containsPoint = false;
            for (const std::uint32_t candidate : remaining) {
                if (candidate == previous || candidate == current ||
                    candidate == next) {
                    continue;
                }
                const Coordinate& point = vertices[candidate];
                if (SamePoint(point, a) || SamePoint(point, b) ||
                    SamePoint(point, c)) {
                    continue;
                }
                if (IsStrictlyInside(point, a, b, c)) {
                    containsPoint = true;
                    break;
                }
            }
            if (containsPoint) {
                continue;
            }
            triangles.push_back({previous, current, next});
            remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(index));
            clipped = true;
            break;
        }
        if (!clipped) {
            return Result<std::vector<Triangle>>::Failure(
                {Failure("ear clipping could not find a valid triangle")});
        }
    }

    if (remaining.size() != 3 ||
        Cross(vertices[remaining[0]], vertices[remaining[1]],
              vertices[remaining[2]]) <= kEpsilon) {
        return Result<std::vector<Triangle>>::Failure(
            {Failure("triangulation produced a degenerate final triangle")});
    }
    triangles.push_back({remaining[0], remaining[1], remaining[2]});
    return Result<std::vector<Triangle>>::Success(std::move(triangles));
}

}  // namespace

Result<PolygonMesh> TriangulatePolygon(const Polygon& polygon) {
    auto outer = OrientedRing(polygon.outer, true, 0);
    if (!outer.Succeeded()) {
        return Result<PolygonMesh>::Failure(std::move(outer.diagnostics));
    }

    std::vector<Ring> holes;
    holes.reserve(polygon.holes.size());
    for (std::size_t index = 0; index < polygon.holes.size(); ++index) {
        auto hole = OrientedRing(
            polygon.holes[index], false, static_cast<std::uint32_t>(index + 1));
        if (!hole.Succeeded()) {
            return Result<PolygonMesh>::Failure(std::move(hole.diagnostics));
        }
        holes.push_back(std::move(*hole.value));
    }

    PolygonMesh mesh;
    auto simple = MakeSimplePolygon(*outer.value, holes, mesh.vertices);
    if (!simple.Succeeded()) {
        return Result<PolygonMesh>::Failure(std::move(simple.diagnostics));
    }
    auto triangles = EarClip(*simple.value, mesh.vertices);
    if (!triangles.Succeeded()) {
        return Result<PolygonMesh>::Failure(std::move(triangles.diagnostics));
    }
    mesh.triangles = std::move(*triangles.value);
    return Result<PolygonMesh>::Success(std::move(mesh));
}

}  // namespace usdvector::authoring

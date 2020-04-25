#ifndef TOOLPOINTBUFFER_HH
#define TOOLPOINTBUFFER_HH

#include <array>
#include <tuple>
#include <vector>
#include <deque>
#include <memory>
#include <mutex>

// WARNING must be a power of two
static constexpr size_t MAX_VERTICES = 1 << 20; // Requires a single allocation of 32MB
static constexpr size_t DEFAULT_VERTICES = 1 << 18;

using RLock = std::lock_guard<std::recursive_mutex>;

struct ColorRGBA {
    unsigned char r, g, b, a;
    bool operator==(const struct ColorRGBA &o) const {
        return std::tie(r, g, b, a) == std::tie(o.r, o.g, o.b, o.a);
    }
    bool operator!=(const struct ColorRGBA &o) const {
        return std::tie(r, g, b, a) != std::tie(o.r, o.g, o.b, o.a);
    }
};

struct ToolPoint {
    float x, y, z;
    struct ColorRGBA c;
    float rx, ry, rz; // or uvw
    struct ColorRGBA c2;
};

using ToolVertexArray = std::array<ToolPoint, MAX_VERTICES>;

struct LineStripInfo {
    LineStripInfo(int t, int p_start, int p_end);
    int tool;
    long long start_id; // Always-rising counter of log points
    long long end_id; // Always-rising counter of log points
};

struct StripVertexDef
{
    ToolPoint const *start_vertex;
    size_t count;
};

/**
 * Defines line strips:
 * strip starts at vertex pt_start % MAX_VERTICES and ends at vertex pt_start % MAX_VERTICES
 * @note it is undefined behavior if the strip wraps around the vertex buffer end (need to ensure that all line strips are contiguous).
 */
typedef std::deque<LineStripInfo> ToolLineStrips;

class ToolPointBuffer
{
protected:
    ToolVertexArray tool_vertices;
    size_t max_vertices;
    ToolLineStrips strips; // WARNING: never clear this fully! it must have one entry for nextVertexID to work
    long long next_id_;
    std::recursive_mutex &line_strip_lock;
    void newStrip(int tool);
    long long advanceVertexID();
    long long getNextVertexID() const;
    long long getStartVertexID() const;
public:
    ToolPointBuffer(std::recursive_mutex &line_strip_lock);

    void setMaxVertices(size_t max_v);
    void clear();
    int lastTool() const;
    bool addPoint(const ToolPoint & pt);
    //bool addPoint(double x, double y, double z, ColorRGBA c);
    void setActiveTool(int tool);
    void dropStaleStrips();
    ToolPoint *getPointFromEnd(int idx);

    ToolVertexArray const* getVertexArray() const;
    ToolLineStrips const& getStrips() const;
    StripVertexDef getStripVertices(LineStripInfo const &strip) const;

    int countPoints() const;
    size_t convertToIndex(long long id) const { return id & (max_vertices - 1);} //WARNING: assumes power-of-two size of max vertices
};

#endif // TOOLPOINTBUFFER_HH

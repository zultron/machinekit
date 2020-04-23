#include "toolpointbuffer.hh"

#include <algorithm>
#include <GL/gl.h>
#include <GL/glext.h>

LineStripInfo::LineStripInfo(int t, int p_start, int p_end) : tool(t), start_id(p_start), end_id(p_end) {}

ToolPointBuffer::ToolPointBuffer(std::recursive_mutex &rlock) : tool_vertices{}, strips(), line_strip_lock(rlock)
{}

void ToolPointBuffer::clear()
{
    RLock lock(line_strip_lock);
    strips.clear();
}

int ToolPointBuffer::lastTool() const
{
    if (strips.empty()) {
        return -1;
    } else {
        return strips.back().tool;
    }
}

bool ToolPointBuffer::addPoint(ToolPoint const &pt)
{
    RLock lock(line_strip_lock);
    // Somewhat naive since this count can't skip
    size_t next_idx = getNextVertexID() % MAX_VERTICES;
    // If a strip has at least one vertex, don't let it wrap around
    if (next_idx == 0 && getStartVertexID() != getNextVertexID()) {
        // The next point will be at the beginning of the vertex array
        dropStaleStrips();
        // Terminate the last strip (so the last valid point in the strip is at the end of the vertex array
        newStrip(lastTool());
        // Copy the last vertex from the back to the front so the new strip appears continuous when drawn
        tool_vertices.front() = tool_vertices.back();
        // Add the new vertex to the new strip
        advanceVertexID();
        ++next_idx;
    }
    dropStaleStrips();
    tool_vertices[next_idx] = pt;
    advanceVertexID();
    return true;
}

long long ToolPointBuffer::advanceVertexID()
{
    // RLock lock(line_strip_lock); Should lock but assume the caller
    if (strips.empty()) {
        return 0;
    }
    return ++strips.back().end_id;
}

/**
 * @pre line strip buffer is locked
 */
long long ToolPointBuffer::getNextVertexID() const
{
    //RLock lock(line_strip_lock);
    if (strips.empty()) {
        return 0;
    }
    return strips.back().end_id;
}

/**
 * @pre line strip buffer is locked
 */
long long ToolPointBuffer::getStartVertexID() const
{
    RLock lock(line_strip_lock);
    if (strips.empty()) {
        return 0;
    }
    return strips.back().start_id;
}

/**
 * @pre line strip buffer is locked
 */
void ToolPointBuffer::setActiveTool(int tool)
{
    RLock lock(line_strip_lock);
    if (strips.empty() || strips.back().tool != tool) {
        newStrip(tool);
    }
}

/** Starts a new line strip for the specified tool */
void ToolPointBuffer::newStrip(int tool)
{
    RLock lock(line_strip_lock);
    strips.emplace_back(tool, getNextVertexID(), getNextVertexID());
}

void ToolPointBuffer::dropStaleStrips()
{
    RLock lock(line_strip_lock);
    // Can be negative if we haven't filled the buffer the first time yet
    auto const cutoff_pt_id = getNextVertexID() - MAX_VERTICES;
    for (auto const &strip : strips) {
        // A valid strip must start later than our earliest possible point
        // Since the line strips are sequential in ID, we can stop searching once we hit the first valid one
        if (strip.start_id > cutoff_pt_id) {
            return;
        }
        strips.pop_front();
    }
}

/**
 * @brief ToolPointBuffer::getPointFromEnd
 * @param cnt positive decrement from end (0 = last)
 * @return
 */
ToolPoint * ToolPointBuffer::getPointFromEnd(int idx_from_end)
{
    RLock lock(line_strip_lock);
    if (strips.empty()) {
        return nullptr;
    }
    size_t end_id = getNextVertexID();
    long long max_available_idx = end_id - strips.back().start_id - 1;
    if (idx_from_end > max_available_idx) {
        return nullptr;
    } else {
        size_t vertex_idx = ((end_id - 1) - idx_from_end) % MAX_VERTICES;
        return &tool_vertices[vertex_idx];
    }
}

const ToolLineStrips & ToolPointBuffer::getStrips() const
{
    return strips;
}

StripVertexDef ToolPointBuffer::getStripVertices(LineStripInfo const &strip) const
{
    long long start = strip.start_id;
    size_t length = std::min(strip.end_id - start, (long long)MAX_VERTICES);
    return {&tool_vertices[ start % MAX_VERTICES], length};
}

int ToolPointBuffer::countPoints() const
{
    if (strips.empty()) {
        return 0;
    }

    RLock lock(line_strip_lock);
    return strips.back().end_id - strips.front().start_id;
}

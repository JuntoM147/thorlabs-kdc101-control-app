#include "algorithm.h"
#include "instructions/instructions.h"

#include <queue>
#include <set>
#include <utility>

namespace algo {

namespace {

const std::vector<std::pair<int, int>> kdirs = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};

bool IsValidPosition(const PixelPosition& pos, const BinaryMatrix& matrix) {
    return pos.x >= 0 && pos.x < matrix.Width() && pos.y >= 0 && pos.y < matrix.Height();
}

PixelPosition BFS(const PixelPosition& start, const BinaryMatrix& matrix)
{
    std::queue<PixelPosition> queue;
    // TODO: Consider unordered_set with a coordinate hash if O(log n) lookup becomes a bottleneck.
    std::set<std::pair<int, int>> visited;

    queue.push(start);
    visited.insert({start.x, start.y});

    while (!queue.empty()) {
        PixelPosition current = queue.front();
        queue.pop();

        if (matrix.At(current.x, current.y) == 1) {
            return current;
        }

        for (const auto& [dx, dy] : kdirs) {
            PixelPosition next{current.x + dx, current.y + dy};
            if (IsValidPosition(next, matrix) && !visited.contains({next.x, next.y})) {
                queue.push(next);
                visited.insert({next.x, next.y});
            }
        }
    }

    return {-1, -1}; // Return an invalid position if no 1 pixel is found
}

// Find the longest horizontal or vertical line of 1s from the starting position
PixelPosition FindLongestLine(const PixelPosition& start, BinaryMatrix& matrix) 
{
    int longest_length = 0;
    PixelPosition end = start;
    std::pair<int, int> dir = {0, 0};
    
    for (const auto& [dx, dy] : kdirs) {
        PixelPosition current = start;
        while (IsValidPosition(current, matrix) && matrix.At(current.x, current.y) == 1) {
            current.x += dx;
            current.y += dy;
        }

        if (longest_length < std::abs(current.x - start.x) + std::abs(current.y - start.y)) {
            longest_length = std::abs(current.x - start.x) + std::abs(current.y - start.y);
            end = {current.x - dx, current.y - dy}; // move back to the last valid position
            dir = {dx, dy};
        }
    }

    // Mark the pixels as visited to avoid counting them again
    PixelPosition current = start;
    while (current.x != end.x || current.y != end.y) {
        matrix.Set(current.x, current.y, false);
        current.x += dir.first;
        current.y += dir.second;
    }
    // Mark the end pixel as well
    matrix.Set(end.x, end.y, false);

    return end;
}

} // namespace


Program GenerateInstructions(PixelPosition start, BinaryMatrix matrix)
{
    Program program;

    PixelPosition current = start;
    PixelPosition next;

    // ensure the laser is off at the start
    program.push_back(Action::kLaserOff);

    while (matrix.PixelCount() > 0) {
        // Where am I currently
        if (matrix.At(current.x, current.y) == 1) {
            // Greedily choose the longest horizontal or vertical line of 1s
            next = FindLongestLine(current, matrix);

            // Reverse positioning may include a controller backlash overshoot and
            // return. Reach the lower endpoint with the laser OFF, then expose
            // the same pixels only in the positive direction.
            PixelPosition draw_start = current;
            PixelPosition draw_end = next;
            if (draw_end.x < draw_start.x || draw_end.y < draw_start.y) {
                std::swap(draw_start, draw_end);
                program.push_back(MoveRelative{draw_start.x - current.x, draw_start.y - current.y});
            }
            program.push_back(Action::kLaserOn);

            // Isolated pixel case
            if (draw_end.x == draw_start.x && draw_end.y == draw_start.y) {
                program.push_back(Action::kWait);
            } else {
                program.push_back(MoveRelative{draw_end.x - draw_start.x, draw_end.y - draw_start.y});
            }

            program.push_back(Action::kLaserOff);
            next = draw_end;
        } else {
            // Go to nearest 1 pixel with BFS
            next = BFS(current, matrix);
            if (next.x == -1) {
                break; // No more 1 pixels to visit
            }

            program.push_back(MoveRelative{next.x - current.x, next.y - current.y});
        }

        current = next;
    }

    program.push_back(Action::kLaserOff); // Ensure the laser is off at the end

    return program;
}

} // namespace algo

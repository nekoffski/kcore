#include <fmt/std.h>

#include <algorithm>
#include <kcore/Functional.hh>
#include <kcore/Profiler.hh>
#include <numeric>

namespace kc {

static constexpr f64 toMS(f64 nanoseconds) { return nanoseconds / 1'000'000.0; }

RegionTimer Profiler::profileRegion(const std::string& name) {
    return RegionTimer{name, threadEvents()};
}

void Profiler::registerThread() { (void)threadEvents(); }

std::shared_ptr<details::ProfilerBuffer> Profiler::threadEvents() {
    std::lock_guard lock{m_mutex};
    auto& buffer = m_threads[std::this_thread::get_id()];
    if (not buffer) {
        buffer = std::make_shared<details::ProfilerBuffer>();
    }
    return buffer;
}

ProfilerSummary Profiler::generateSummary() {
    std::lock_guard registryLock{m_mutex};
    ProfilerEventsPerThread completed;
    for (const auto& [id, buffer] : m_threads) {
        std::lock_guard lock{buffer->mutex};
        std::size_t depth = 0;
        std::size_t end = 0;
        for (std::size_t i = 0; i < buffer->events.size(); ++i) {
            if (buffer->events[i].type == ProfilerEvent::Type::begin) {
                ++depth;
            } else if (depth > 0) {
                --depth;
            }
            if (depth == 0) {
                end = i + 1;
            }
        }
        if (end != 0) {
            auto boundary = buffer->events.begin() + end;
            completed[id].assign(buffer->events.begin(), boundary);
            buffer->events.erase(buffer->events.begin(), boundary);
        }
    }
    return ProfilerSummary{completed};
}

RegionTimer::RegionTimer(std::string name, ProfilerEvents& events)
    : m_name(std::move(name)), m_events(&events) {
    record(ProfilerEvent::Type::begin);
}

RegionTimer::RegionTimer(
    std::string name, std::shared_ptr<details::ProfilerBuffer> buffer
)
    : m_name(std::move(name)),
      m_buffer(std::move(buffer)),
      m_events(&m_buffer->events) {
    record(ProfilerEvent::Type::begin);
}

void RegionTimer::record(ProfilerEvent::Type type) {
    std::unique_lock<std::mutex> lock;
    if (m_buffer) {
        lock = std::unique_lock{m_buffer->mutex};
    }
    auto name = type == ProfilerEvent::Type::begin ? m_name : std::move(m_name);
    m_events->push_back(
        {type, std::move(name), std::chrono::steady_clock::now()}
    );
}

RegionTimer::~RegionTimer() { record(ProfilerEvent::Type::end); }

ProfilerSummary::ProfilerSummary(const ProfilerEventsPerThread& events) {
    generateSummary(events);
}

void ProfilerSummary::print() const {
    log::info("──── Profiler ────────────────────────────────────");
    for (const auto& [threadId, graph] : m_summaryGraphs) {
        log::info("  Thread {}", threadId);
        printGraph(graph, "  ");
    }
    log::info("──────────────────────────────────────────────────");
}

void ProfilerSummary::generateSummary(const ProfilerEventsPerThread& events) {
    for (const auto& [threadId, threadEvents] : events) {
        SummaryGraphNode root{.name = "root"};
        processThread(root, threadEvents);
        m_summaryGraphs[threadId] = std::move(root);
    }
}

void ProfilerSummary::processThread(
    SummaryGraphNode& root, const ProfilerEvents& events
) {
    std::vector<
        std::pair<SummaryGraphNode*, std::chrono::steady_clock::time_point>>
        open;
    for (const auto& [type, name, timestamp] : events) {
        if (type == ProfilerEvent::Type::begin) {
            auto* current = open.empty() ? &root : open.back().first;
            auto [it, inserted] = current->children.try_emplace(name);
            auto& child = it->second;
            if (inserted) {
                child.insertionIndex = current->nextChildIndex++;
                child.name = name;
            }
            open.emplace_back(&child, timestamp);
        } else if (not open.empty() and open.back().first->name == name) {
            const auto [node, started] = open.back();
            if (timestamp >= started) {
                node->times.push_back(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        timestamp - started
                    )
                        .count()
                );
            }
            open.pop_back();
        }
    }
}

void ProfilerSummary::printGraph(
    const SummaryGraphNode& node, const std::string& prefix
) const {
    std::vector<const SummaryGraphNode*> ordered;
    ordered.reserve(node.children.size());
    for (const auto& [_, child] : node.children) {
        if (not child.times.empty()) {
            ordered.push_back(&child);
        }
    }
    std::sort(
        ordered.begin(), ordered.end(),
        [](const SummaryGraphNode* a, const SummaryGraphNode* b) {
            return a->insertionIndex < b->insertionIndex;
        }
    );
    for (size_t i = 0; i < ordered.size(); ++i) {
        const bool isLast = i + 1 == ordered.size();
        const auto& child = *ordered[i];
        const auto stats = computeStats(child);

        const auto* branch = isLast ? "└─ " : "├─ ";
        const auto* stem = isLast ? "   " : "│  ";

        if (stats.count > 1) {
            log::info(
                "{}{}{}  x{}  {:.3f}ms  (avg {:.3f}ms)", prefix, branch,
                child.name, stats.count, toMS(stats.totalTime),
                toMS(stats.averageTime)
            );
        } else {
            log::info(
                "{}{}{}  {:.3f}ms", prefix, branch, child.name,
                toMS(stats.totalTime)
            );
        }
        printGraph(child, prefix + stem);
    }
}

ProfilerSummary::CallStatistics ProfilerSummary::computeStats(
    const SummaryGraphNode& node
) {
    const u64 total =
        std::accumulate(node.times.begin(), node.times.end(), u64{0});
    const u64 count = node.times.size();
    return {
        .count = count,
        .totalTime = static_cast<f64>(total),
        .averageTime = count > 0 ? static_cast<f64>(total) / count : 0.0,
    };
}

}  // namespace kc

#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <latch>
#include <string>
#include <thread>

namespace task_line_calculation {
    class line_calculator {
        uint32_t _num_of_thread{0};
        std::vector<std::thread> _workers;
        std::vector<std::filesystem::path> _paths;
        std::atomic<uint64_t> _total_lines{0};
        uint64_t _number_of_files;
        uint64_t _num_of_file_per_thread;
        uint64_t _rest_num_of_file_per_thread;
        std::filesystem::path _file_path;

        inline bool initialize() {
            _number_of_files = traverse_directory(_file_path);
            if (_number_of_files == 0) {
                return false;
            }
            _num_of_thread = _number_of_files >= _num_of_thread ? _num_of_thread : _number_of_files;
            _workers.reserve(_num_of_thread);
            _num_of_file_per_thread = _number_of_files / _num_of_thread;
            _rest_num_of_file_per_thread = _number_of_files - _num_of_file_per_thread * (_num_of_thread - 1);
            return true;
        }

        inline uint64_t count_lines_in_file(const std::filesystem::path& file_path) {
            std::ifstream file(file_path, std::ios::binary);
            if (!file.is_open()) {
                return 0;
            }

            size_t line_count = 0;
            std::string line;
            while (std::getline(file, line)) {
                line_count++;
            }
            file.close();
            return line_count;
        }

        inline uint64_t traverse_directory(const std::filesystem::path& directory_path) {
            try {
                for (const auto& entry: std::filesystem::recursive_directory_iterator(directory_path)) {
                    if (entry.is_regular_file()) {
                        _paths.push_back(entry.path());
                    }
                }
                return _paths.size();
            } catch (const std::exception&) {
                return 0;
            }
        }

    public:
        line_calculator(const std::filesystem::path& file_path)
                : _num_of_thread(std::thread::hardware_concurrency()),
                  _file_path(file_path) {
        }

        ~line_calculator() {
            for (auto& thread: _workers) {
                thread.join();
            }
        }

        inline uint64_t calculate_lines() {
            if (!initialize()) {
                return 0;
            }
            std::latch latch(_num_of_thread);
            for (uint32_t i = 0; i < _num_of_thread - 1; ++i) {
                _workers.emplace_back([=, self = this, &latch]() {
                    uint64_t total_lines = 0;
                    const auto start_index = i * _num_of_file_per_thread;
                    for (uint64_t idx = start_index; idx < start_index + _num_of_file_per_thread; ++idx) {
                        total_lines += count_lines_in_file(self->_paths[idx]);
                    }
                    uint64_t local_total;
                    do {
                        local_total = _total_lines.load();
                    } while (!_total_lines.compare_exchange_weak(local_total, local_total + total_lines));

                    latch.count_down(1);
                });
            }

            _workers.emplace_back([&, self = this]() {
                uint64_t total_lines = 0;
                const auto start_index = _number_of_files - _rest_num_of_file_per_thread;
                for (uint64_t idx = start_index; idx < _number_of_files; ++idx) {
                    total_lines += count_lines_in_file(self->_paths[idx]);
                }
                uint64_t local_total;
                do {
                    local_total = _total_lines.load();
                } while (!_total_lines.compare_exchange_weak(local_total, local_total + total_lines));
                latch.count_down(1);
            });

            latch.wait();

            return _total_lines.load();
        }
    };
}

// task_line_calculation.cpp : Defines the entry point for the application.
//

#include "task_line_calculation.hpp"

using namespace task_line_calculation;

int main()
{
    //Please add files and subfolders under ./folder directory
    line_calculator calculator(std::filesystem::current_path() / "folder");
    uint64_t total_lines =  calculator.calculate_lines();
	return 0;
}

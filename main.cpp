#include <iostream>
#include "ThreadPool.hpp"

int task(int a, int b) {
	std::this_thread::sleep_for(std::chrono::seconds(5));
	return a + b;
}
int main()
{
    system("chcp 65001");
	ThreadPool pool;
	std::vector<std::future<int>> r;
	for (int i = 0; i < 100; i++)
	{
		r.emplace_back(pool.addTask(task, i, i * 2));
	}
	for (auto& i : r)
	{
		std::cout << "执行结果：" << i.get() << std::endl;
	}
	getchar();
	return 0;
}
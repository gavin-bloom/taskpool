#include <condition_variable>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <functional>
#include <vector>
#include <atomic>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <time.h>

class Task {
public:
    Task(std::function<void()> function, std::vector<std::vector<Task*>> dependencies)
        : m_function(function), m_dependencies(dependencies), m_completed(false) {}

    bool IsCompleted() const { return m_completed; }
    void SetCompleted() { m_completed = true; }

    std::vector<std::vector<Task*>> GetDependencies() const { return m_dependencies; }
    std::function<void()> GetFunction() const { return m_function; }

private:
    std::function<void()> m_function;
    std::vector<std::vector<Task*>> m_dependencies;
    std::atomic<bool> m_completed;
};

class Graph {
public:
    Graph() {}

    void AddTask(Task* task) {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_ready_tasks.push_back(task);

        for (auto set : task->GetDependencies()) {
            m_dependencies[task].push_back(set);
        }

        m_cv.notify_all();
    }

    Task* GetNextTask() {
		
		std::srand ( time(NULL) );
		
        std::unique_lock<std::mutex> lock(m_mutex);

        m_cv.wait(lock, [this](){
            return !m_ready_tasks.empty() || m_completed_tasks.size() == m_dependencies.size();
        });

        if (m_ready_tasks.empty()) {
            return nullptr;
        }

		int RanIndex = rand() % m_ready_tasks.size();
        Task* task = m_ready_tasks[RanIndex];
        m_ready_tasks.erase(m_ready_tasks.begin() + RanIndex);

        if (!task->IsCompleted()) {
            bool can_execute = false;

            for (auto set : task->GetDependencies()) {
                bool all_completed = true;

                for (auto dependency : set) {
                    if (!dependency->IsCompleted()) {
                        all_completed = false;
                        break;
                    }
                }

                if (all_completed) {
                    can_execute = true;
                    break;
                }
            }

            if (!can_execute) {
                m_ready_tasks.push_back(task);
                return nullptr;
            }
			
			//run the task and set completed
			task->GetFunction()();
            task->SetCompleted();
            m_completed_tasks.push_back(task);
        }

        for (auto pair : m_dependencies) {
            auto dependencies = pair.second;

            for (auto set : dependencies) {
                auto it = std::find(set.begin(), set.end(), task);

                if (it != set.end()) {
                    set.erase(it);
                }
            }
        }

        return task;
    }

private:
    std::vector<Task*> m_ready_tasks;
    std::vector<Task*> m_completed_tasks;
    std::unordered_map<Task*, std::vector<std::vector<Task*>>> m_dependencies;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

class ThreadPool {
public:
    ThreadPool(int num_threads) : m_num_threads(num_threads), m_stop(false) {}

    ~ThreadPool() {
        m_stop = true;
        m_cv.notify_all();

        for (auto& thread : m_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void Start() {
        for (int i = 0; i < m_num_threads; ++i) {
            m_threads.emplace_back(std::thread(&ThreadPool::ThreadFunction, this));
        }
    }

    void AddTask(Task* task) {
        m_graph.AddTask(task);
    }

private:
    void ThreadFunction() {
        while (!m_stop) {
            Task* task = m_graph.GetNextTask();
        }
    }

    int m_num_threads;
    std::atomic<bool> m_stop;
    std::vector<std::thread> m_threads;
    Graph m_graph;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

int main() {
    ThreadPool thread_pool(4);
    thread_pool.Start();

    // Create tasks and dependencies
    Task task1([]() { std::cout << "Task 1" << std::endl;}, {{}});
    Task task2([]() { std::cout << "Task 2" << std::endl;}, {{}});
    Task task3([]() { std::cout << "Task 3" << std::endl;}, {{&task1}}); //depends on task1
    Task task4([]() { std::cout << "Task 4" << std::endl;}, {{&task3}});
    Task task5([]() { std::cout << "Task 5" << std::endl;}, {{&task3, &task4}}); //depends on task3 AND task4
    Task task6([]() { std::cout << "Task 6" << std::endl;}, {{&task3}, {&task4}}); //depends on task3 OR task4


    // Add tasks to thread pool
    thread_pool.AddTask(&task1);
    thread_pool.AddTask(&task2);
    thread_pool.AddTask(&task3);
    thread_pool.AddTask(&task4);
    thread_pool.AddTask(&task5);
    thread_pool.AddTask(&task6);
	
	// Wait for all tasks to complete
    while ((!(&task6)->IsCompleted()) || (!(&task5)->IsCompleted()) || 
		   (!(&task4)->IsCompleted()) || (!(&task3)->IsCompleted()) || 
		   (!(&task2)->IsCompleted()) || (!(&task1)->IsCompleted())) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "All tasks completed\n";

	
    return 0;
}
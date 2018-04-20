//
// Created by Nikolay Yakovets on 2018-02-02.
//

#ifndef QS_SIMPLEEVALUATOR_H
#define QS_SIMPLEEVALUATOR_H

#pragma once

#include <cmath>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <future>
#include <memory>
#include <sstream>
#include <string>

#include "SimpleGraph.h"
#include "RPQTree.h"
#include "Evaluator.h"
#include "Graph.h"

// --- begin thread pool class

class ThreadedJobPool {
private:
    std::vector<std::thread*> workers;
    std::queue<std::function<void()>> jobs;

    std::mutex mutex;
    std::condition_variable cv;

    bool stop;

    void workerLoop() {
        while (true) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(mutex);
                cv.wait(lock, [&]{ return stop || !jobs.empty(); });
                if (stop && jobs.empty()) {
                    return;
                }
                job = std::move(jobs.front());
                jobs.pop();
            }
            job();
        }
    }

public:
    explicit ThreadedJobPool(size_t nWorkers);

    template<class F, class... Args>
    std::shared_future<typename std::result_of<F(Args...)>::type> enqueue(F&& f, Args&&... args) {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(mutex);
            jobs.emplace([task](){
                (*task)();
            });
        }
        cv.notify_one();
        return res.share();
    }

    ~ThreadedJobPool();
};

inline ThreadedJobPool::ThreadedJobPool(size_t nWorkers)
        : stop(false)
{
    for (size_t n = 0; n < nWorkers; ++n) {
        std::cout << "Creating worker...\n";
        auto t = new std::thread(&ThreadedJobPool::workerLoop, this);
        workers.push_back(t);
    }
}

inline ThreadedJobPool::~ThreadedJobPool() {
    {
        std::unique_lock<std::mutex> lock(mutex);
        stop = true;
    }
    cv.notify_all();
    for (auto const &worker : workers) {
        std::cout << "Joining worker...\n";
        worker->join();
    }
}

// --- end thread pool class


typedef std::unordered_map<uint32_t, std::vector<uint32_t>> intermediate;
typedef std::vector<std::pair<uint32_t, bool>> query_path;

class SimpleEvaluator : public Evaluator {

    std::shared_ptr<SimpleGraph> graph;
    std::shared_ptr<SimpleEstimator> est;

    std::unordered_map<std::string, std::shared_ptr<intermediate>> evalCache;
    std::unordered_map<std::string, cardStat> statCache;

    void unpackQueryTree(query_path *path, RPQTree *q);
    std::string pathToString(query_path *path);

    ThreadedJobPool threadPool;

public:
    explicit SimpleEvaluator(std::shared_ptr<SimpleGraph> &g);

    ~SimpleEvaluator() = default;

    void prepare() override;

    cardStat evaluate(RPQTree *query) override;

    void attachEstimator(std::shared_ptr<SimpleEstimator> &e);

    std::shared_ptr<intermediate> evaluate_aux(RPQTree *q);
    std::shared_future<std::shared_ptr<intermediate>> evaluate_async(RPQTree *q);

    static std::shared_ptr<intermediate> project(uint32_t label, bool inverse, std::shared_ptr<SimpleGraph> &g);

    static std::shared_ptr<intermediate> join(std::shared_ptr<intermediate> &left, std::shared_ptr<intermediate> &right);

    cardStat computeStats(std::shared_ptr<intermediate> &result);

    RPQTree *optimizeQuery(query_path *path);
};


#endif //QS_SIMPLEEVALUATOR_H

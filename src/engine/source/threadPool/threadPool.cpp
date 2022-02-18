/* Copyright (C) 2015-2021, Wazuh Inc.
 * All rights reserved.
 *
 *  program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "threadPool.hpp"

namespace threadpool
{

threadPool::loop_worker::~loop_worker()
{
}

threadPool::loop_worker::loop_worker(rxcpp::composite_subscription cs, rxcpp::schedulers::worker w,
                                     std::shared_ptr<const rxcpp::schedulers::scheduler_interface> alive)
    : lifetime(cs), controller(w), alive(alive)
{
    auto token = controller.add(cs);
    cs.add([token, w]() { w.remove(token); });
}

threadPool::clock_type::time_point threadPool::loop_worker::now() const
{
    return clock_type::now();
}

void threadPool::loop_worker::schedule(const rxcpp::schedulers::schedulable & scbl) const
{
    controller.schedule(lifetime, scbl.get_action());
}

void threadPool::loop_worker::schedule(clock_type::time_point when, const rxcpp::schedulers::schedulable & scbl) const
{
    controller.schedule(when, lifetime, scbl.get_action());
}

void threadPool::initThreads(void)
{
    // auto remaining = std::max(std::thread::hardware_concurrency(), unsigned(4));
    auto remaining = nThreads > 1 ? nThreads : 1;
    while (remaining--)
    {
        loops.push_back(newthread.create_worker(loops_lifetime));
    }
}

threadPool::threadPool(int _nThreads)
    : nThreads(_nThreads), factory([](std::function<void()> start) { return std::thread(std::move(start)); }),
      newthread(rxcpp::schedulers::make_new_thread()), count(0)
{
    initThreads();
}

threadPool::threadPool(int _nThreads, rxcpp::schedulers::thread_factory tf)
    : nThreads(_nThreads), factory(tf), newthread(rxcpp::schedulers::make_new_thread(tf)), count(0)
{
    initThreads();
}

threadPool::~threadPool()
{
    loops_lifetime.unsubscribe();
}

threadPool::clock_type::time_point threadPool::now() const
{
    return clock_type::now();
}

rxcpp::schedulers::worker threadPool::create_worker(rxcpp::composite_subscription cs) const
{
    return rxcpp::schedulers::worker(
        cs, std::make_shared<loop_worker>(cs, loops[++count % loops.size()], this->shared_from_this()));
}

} // namespace threadpool
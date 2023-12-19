#include "NaiveSkipList.hpp"
#include "ConSkipList.hpp"
#include <thread>


void test_naive_skip_list() {
    NaiveSkipList<int> nsl(4, 0.6);
    // add 0, 1 ,2, 3, 4, 5, 6, 7, 8, 9 in random order
    nsl.Add(7);
    nsl.Add(3);
    nsl.Add(1);
    nsl.Add(0);
    nsl.Add(9);
    nsl.Add(8);
    nsl.Add(2);
    nsl.Add(4);
    nsl.Add(5);
    nsl.Add(6);
    std::cout << "Added tree\n";
    nsl.Print();
    // remove 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 in random order
    nsl.Remove(5);
    nsl.Remove(3);
    nsl.Remove(7);
    nsl.Remove(1);
    nsl.Remove(9);
    nsl.Remove(8);
    nsl.Remove(2);
    nsl.Remove(4);
    nsl.Remove(0);
    nsl.Remove(6);
    std::cout << "Removed tree\n";
    nsl.Print();

    // interleave add and remove
    nsl.Add(7);
    nsl.Add(3);
    nsl.Add(1);
    nsl.Add(0);
    nsl.Add(9);
    nsl.Remove(5);
    nsl.Remove(3);
    nsl.Remove(7);
    nsl.Remove(1);
    nsl.Remove(9);
    nsl.Add(8);
    nsl.Add(2);
    nsl.Add(4);
    nsl.Add(5);
    nsl.Add(6);
    nsl.Remove(8);
    nsl.Remove(2);
    nsl.Remove(4);
    nsl.Remove(0);
    nsl.Remove(6);
    // result should be 5
    std::cout << "Interleaved tree\n";
    nsl.Print();
}

void test_concurrent_skip_list() {
    ConSkipList<int> csl(4, 0.6);
    // create 4 threads to add 0, 1 ,2, 3, 4, 5, 6, 7, 8, 9 in random order
    std::thread t1([&csl]() {
        csl.Add(7);
        csl.Add(3);
        csl.Add(1);
        csl.Add(0);
        csl.Add(9);
    });
    std::thread t2([&csl]() {
        csl.Add(8);
        csl.Add(2);
        csl.Add(4);
        csl.Add(5);
        csl.Add(6);
    });
    t1.join();
    t2.join();
    std::cout << "Added tree\n";
    csl.Print();
    // create 5 threads to remove 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 in random order
    std::thread t3([&csl]() {
        csl.Remove(5);
        csl.Remove(3);
    });
    std::thread t4([&csl]() {
        csl.Remove(7);
        csl.Remove(1);
    });
    std::thread t5([&csl]() {
        csl.Remove(9);
        csl.Remove(8);
    });
    std::thread t6([&csl]() {
        csl.Remove(2);
        csl.Remove(4);
    });
    std::thread t7([&csl]() {
        csl.Remove(0);
        csl.Remove(6);
    });
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    std::cout << "Removed tree\n";
    csl.Print();

    // interleave add and remove
    std::thread t8([&csl]() {
        csl.Add(7);
        csl.Add(3);

    });
    std::thread t9([&csl]() {
        csl.Add(1);
        csl.Add(0);
    });
    std::thread t10([&csl]() {
        csl.Add(9);
        csl.Remove(5);
    });
    std::thread t11([&csl]() {
        csl.Remove(3);
        csl.Remove(7);
    });
    std::thread t12([&csl]() {
        csl.Remove(1);
        csl.Remove(9);
    });
    std::thread t13([&csl]() {
        csl.Add(8);
        csl.Add(2);
    });
    std::thread t14([&csl]() {
        csl.Add(4);
        csl.Add(5);
    });
    std::thread t15([&csl]() {
        csl.Add(6);
        csl.Remove(8);
    });
    std::thread t16([&csl]() {
        csl.Remove(2);
        csl.Remove(4);
    });
    std::thread t17([&csl]() {
        csl.Remove(0);
        csl.Remove(6);
    });

    t8.join();
    t9.join();
    t10.join();
    t11.join();
    t12.join();
    t13.join();
    t14.join();
    t15.join();
    t16.join();
    t17.join();

    std::cout << "Interleaved tree\n";
    csl.Print();
}

void pressure_test() {
    // add ranged from 0-200,000, with 1, 2, 4, 8 threads
    std::atomic<int> progress(0);
    for (int i = 0; i < 4; ++i) {
        int num_threads = 1 << i;
        std::cout << "Number of threads: " << num_threads << std::endl;
        // time start
        auto start = std::chrono::high_resolution_clock::now();
        ConSkipList<int> csl(4, 0.6);
        std::vector<std::thread> threads;
        for (int j = 0; j < num_threads; ++j) {
            threads.emplace_back([&csl, &progress, num_threads, j]() {
                // split the range into num_threads parts
                int start = 200000 / num_threads * j;
                int end = 200000 / num_threads * (j + 1);
                for (int k = start; k < end; ++k) {
                    csl.Add(k);
                    ++progress;
                    if (progress % 100 == 0) {
                        std::cout << "\r" << progress << " / 200000";
                        std::cout.flush();
                    }
                }
            });
        }
        for (auto &t: threads) {
            t.join();
        }
        // time end
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "\nTime elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                  << "ms" << std::endl;
        // reset progress
        progress = 0;
    }
}

void pressure_test_interleave() {
    // 200,000 instructions, 1, 2, 4, 8 threads, in which 180000 are adds and 15000 are removes, 5000 are contains
    // so add should be ranged from 0-180000, remove and contains should be sampled from 0-200000
    std::atomic<int> progress(0);
    for (int i = 0; i < 4; ++i) {
        int num_threads = 1 << i;
        std::cout << "Number of threads: " << num_threads << std::endl;
        // time start
        auto start = std::chrono::high_resolution_clock::now();
        ConSkipList<int> csl(4, 0.6);
        std::vector<std::thread> threads;
        for (int j = 0; j < num_threads; ++j) {
            threads.emplace_back([&csl, &progress, num_threads, j]() {
                // split the range into num_threads parts
                int start = 180000 / num_threads * j;
                int end = 180000 / num_threads * (j + 1);
                for (int k = start; k < end; ++k) {
                    csl.Add(k);
                    ++progress;
                    if (progress % 100 == 0) {
                        std::cout << "\r" << progress << " / 200000";
                        std::cout.flush();
                    }
                }
                // sample 15000 removes and 5000 contains
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, 200000);
                for (int k = 0; k < 15000 / num_threads; ++k) {
                    csl.Remove(dis(gen));
                    ++progress;
                    if (progress % 100 == 0) {
                        std::cout << "\r" << progress << " / 200000";
                        std::cout.flush();
                    }
                }
                for (int k = 0; k < 5000 / num_threads; ++k) {
                    csl.Contains(dis(gen));
                    ++progress;
                    if (progress % 100 == 0) {
                        std::cout << "\r" << progress << " / 200000";
                        std::cout.flush();
                    }
                }
            });
        }
        for (auto &t: threads) {
            t.join();
        }
        // time end
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "\nTime elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                  << "ms" << std::endl;
        // reset progress
        progress = 0;
    }
}


int main(int argc, char const *argv[]) {
//    std::cout<<"Naive Skip List\n";
//    test_naive_skip_list();
//    std::cout<<"Concurrent Skip List\n";
//    test_concurrent_skip_list();
//    pressure_test();
    pressure_test_interleave();
    return 0;
}
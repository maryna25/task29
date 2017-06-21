#include <iostream>
#include <thread>
#include <random>
#include <chrono>
#include <string>

#include "thread_pool.hpp"

using namespace std;

void sum(string text) {
    cout << text << endl;
}

int main() {
    ThreadPool pool(2);
    pool.runAsync([](int a){
        cout << "It's Lambda with parameter a = " << a << endl;
    }, 10);
    string x = "Privet";
    pool.runAsync(&sum, x);
    return 0;
}
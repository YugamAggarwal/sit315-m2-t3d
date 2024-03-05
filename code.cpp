#include <iostream>
#include <iomanip>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <ctime>
#include <algorithm>

using namespace std;

// Define data structures
struct TrafficSignal {
    time_t timestamp;
    int id;
    int numCarsPassed;
};

// Bounded buffer for producer-consumer pattern
class BoundedBuffer {
private:
    mutex mtx;
    condition_variable not_full, not_empty;
    queue<TrafficSignal> buffer;
    int maxSize;

public:
    BoundedBuffer(int size) : maxSize(size) {}

    void produce(TrafficSignal signal) {
        unique_lock<mutex> lock(mtx);
        not_full.wait(lock, [this](){ return buffer.size() < maxSize; });
        buffer.push(signal);
        not_empty.notify_one();
    }

    TrafficSignal consume() {
        unique_lock<mutex> lock(mtx);
        not_empty.wait(lock, [this](){ return !buffer.empty(); });
        TrafficSignal signal = buffer.front();
        buffer.pop();
        not_full.notify_one();
        return signal;
    }
};

// Generate traffic signals
void generateTraffic(BoundedBuffer& buffer, int numSignals) {
    while (true) {
        for (int i = 0; i < numSignals; ++i) {
            TrafficSignal signal = {time(NULL), i, rand() % 100}; // Timestamp and random number of cars passed
            buffer.produce(signal);
        }
        this_thread::sleep_for(chrono::minutes(5)); // Generate every 5 minutes
    }
}

// Process traffic signals
void processTraffic(BoundedBuffer& buffer, int numSignals, int topN) {
    while (true) {
        vector<TrafficSignal> signals;
        for (int i = 0; i < numSignals; ++i) {
            TrafficSignal signal = buffer.consume();
            signals.push_back(signal);
        }
        sort(signals.begin(), signals.end(), [](TrafficSignal a, TrafficSignal b) {
            return a.numCarsPassed > b.numCarsPassed;
        });
        cout << "========================================================" << endl;
        cout << "Top " << topN << " most congested traffic lights at " << put_time(localtime(&signals[0].timestamp), "%Y-%m-%d %H:%M:%S") << ":" << endl;
        cout << setw(10) << "ID" << setw(15) << "Cars Passed" << endl;
        cout << "--------------------------------------------------------" << endl;
        for (int i = 0; i < topN && i < numSignals; ++i) {
            cout << setw(10) << signals[i].id << setw(15) << signals[i].numCarsPassed << endl;
        }
        cout << "========================================================" << endl;
        this_thread::sleep_for(chrono::hours(1)); // Process every hour
    }
}

int main() {
    srand(time(NULL));

    int numSignals = 10; // X - Number of traffic signals
    int bufferSize = 10; // Size of bounded buffer
    int topN = 3; // Top N most congested traffic lights

    BoundedBuffer buffer(bufferSize);

    // Start producer thread
    thread producer(generateTraffic, ref(buffer), numSignals);
    // Start consumer thread
    thread consumer(processTraffic, ref(buffer), numSignals, topN);

    // Join threads
    producer.join();
    consumer.join();

    return 0;
}

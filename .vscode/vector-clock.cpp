#include <bits/stdc++.h>
using namespace std;


class VectorClock{
private:
    vector<int> clock;
    int process_id;
    int numOfProc;

public:
    VectorClock(int pid, int numOfProc):process_id(pid) ,numOfProc(numOfProc) {  // inline intialiser.....process_id(pid) = process_id = pid ...
        clock = vector<int>(0, numOfProc);
    }

    // increment the clock for the internal event
    void tick(){
        clock[process_id]++;
    }

    //update the clock while sending message
    VectorClock getSendClock(){
        tick();
        return *this;
    }

    //receive an event 

    void receiveEvent(VectorClock &receiveVectorClock){
        // p0 = [2,0,0]
        //p1 = [0,0,0]

        clock[process_id]++;
        for (int i=0;i<numOfProc && i!= process_id;i++){
            clock[i] = max(clock[i],receiveVectorClock[i])
        }
    }

    // Compare vector clocks for causality
    enum Relation { BEFORE, AFTER, CONCURRENT };
    
    Relation compare(const VectorClock& other) const {
        bool allLessEqual = true;
        bool allGreaterEqual = true;
        bool someStrictlyLess = false;
        bool someStrictlyGreater = false;

        for (int i = 0; i < numProcesses; i++) {
            if (clock[i] > other.clock[i]) {
                allLessEqual = false;
                someStrictlyGreater = true;
            }
            if (clock[i] < other.clock[i]) {
                allGreaterEqual = false;
                someStrictlyLess = true;
            }
        }

        if (allLessEqual && someStrictlyLess) return BEFORE;
        if (allGreaterEqual && someStrictlyGreater) return AFTER;
        return CONCURRENT;
    }

        std::string toString() const {
        std::stringstream ss;
        ss << "[";
        for (int i = 0; i < numProcesses; i++) {
            ss << clock[i];
            if (i < numProcesses - 1) ss << ", ";
        }
        ss << "]";
        return ss.str();
    }

    // Getters
    const std::vector<int>& getClock() const { return clock; }
    int getProcessId() const { return processId; }



};



/// Message 

struct Message {
    int from;
    int to;
    std::string content;
    VectorClock timestamp;
    int messageId;

    Message(int f, int t, const std::string& c, const VectorClock& ts, int id)
        : from(f), to(t), content(c), timestamp(ts), messageId(id) {}
};



class DistributedProcess{
    VectorClock vc;
    int process_id;
    
}




int main(){
    cout<<"hello"<<endl;
}

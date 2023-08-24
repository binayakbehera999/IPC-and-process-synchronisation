#include<pthread.h>
#include<unistd.h>
#include<bits/stdc++.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

using namespace std;
#define MAX 5
#define e 100
#define c 500
#define k 10
#define s 20
// how to set total running time t?
//
#define RUN_TIME 40 // run for 10 seconds
volatile sig_atomic_t is_running = 1;
void timer_handler(int signum)
{
    is_running=0;
}



/// 
int activeQueries=0;

// Define data structures
struct Event {
    int id;
    int capacity;
    int available;
    vector<bool> booked;
    
};
Event events[e]; // array of events
pthread_mutex_t outputMutex;
struct Table
{
    int eventNo;
    int queryType; // 1 ,2 or 3
    int workerId;
};
Table table[MAX];
pthread_mutex_t tableMutex; // mutex to access the table
pthread_cond_t maxCond; // conditional variable to signal if max entiries there

bool isRead(int eventId) // check if any reads active in table for eventId
{
    // so for eventId and query 1  i have to check
    for(int i=0;i<MAX;i++)
    {
        if(table[i].eventNo==eventId && table[i].queryType==1) return true;
    }
    return false;

}
bool isWrite(int eventId) // any actrive writes for eventId there or not
{
    //check if event id there and query 2 or 3
    for(int i=0;i<MAX;i++)
    {
        if(table[i].eventNo==eventId &&(table[i].queryType==2 || table[i].queryType==3))return true;
    }
    return false;
}

void availableSeats(Event* event,int workerId)
{
    chrono::seconds delay(1);
    this_thread::sleep_for(delay);
    pthread_mutex_lock(&outputMutex);
    cout<<"thread "<<workerId<<" has queried event "<<event->id<<" which has "<<event->available<<" seats\n";
    pthread_mutex_unlock(&outputMutex);
    
    
}
void book(Event* event, int workerId,map<int,vector<int>>&privList)
{
    chrono::seconds delay(1);
    this_thread::sleep_for(delay);
    int noBooking= rand()%k +1;
    if(noBooking > event->available)
    {
        pthread_mutex_lock(&outputMutex);
        cout<<"thread "<<workerId<<" has tried to book event "<<event->id<<" which doesnt have "<< noBooking<<" seats , hence failed\n";
        pthread_mutex_unlock(&outputMutex);
        return;
    }
    event->available-= noBooking;
    pthread_mutex_lock(&outputMutex);
    cout<<"thread "<<workerId<<" has tried to book event "<<event->id<<" for "<< noBooking<<" seats , success\n";
    pthread_mutex_unlock(&outputMutex);
    for(int i=0;i<c;i++)
    {
        if(noBooking==0)continue;
        if(event->booked[i]==false)
        {
            noBooking--;
            event->booked[i]=true;
            privList[event->id].push_back(i);
        }
    }
    
    

}
void cancelTicket(Event* event, int workerId,map<int,vector<int>>&privList)
{
    chrono::seconds delay(1);
    this_thread::sleep_for(delay);
    //choose a random ticket from private booked list of each thread
    int n= privList[event->id].size();
    if(n==0)
    {
        pthread_mutex_lock(&outputMutex);
        cout<<"thread "<<workerId<<" hasnt booked any tickets for event "<<event->id<<" yet and hence cant cancel\n";
        pthread_mutex_unlock(&outputMutex);
        return;
    }

    int pos= rand()%n;
    int ticket= privList[event->id][pos];
    //cancel ticket
    auto it = find(privList[event->id].begin(), privList[event->id].end(), ticket);
    if (it != privList[event->id].end()) {
        privList[event->id].erase(it);
    }
    event->booked[ticket]=false;
    event->available++;
    pthread_mutex_lock(&outputMutex);
    cout<<"thread "<<workerId<<" has canceled ticket " <<ticket<<" for event "<<event->id<<"\n";
    pthread_mutex_unlock(&outputMutex);
   
    
}
int getActiveQueries()
{
    int cnt=0;
    for(int i=0;i<MAX;i++)
    {
        if(table[i].eventNo!=-1) cnt++;
    }
    return cnt;
}
int makeTableEntry(int eventNo,int queryType,int workerId)
{
    for(int i=0;i<MAX;i++)
    {
        if(table[i].eventNo==-1)
        {
            table[i].eventNo=eventNo;
            table[i].queryType=queryType;
            table[i].workerId=workerId;
            return i;
        }
    }
    cout<<"Error in code";

    return -1;
}
void deleteTableEntry(int index)
{
    table[index].eventNo=-1;

}
void eventHandler(Event* event, int workerId,map<int,vector<int>>&privList) {
    
    
    pthread_mutex_lock(&tableMutex);
    while (getActiveQueries() >= MAX) {
        pthread_mutex_lock(&outputMutex);
        cout<<"thread "<<workerId<<" is going to sleep as common active query table is full\n";
        pthread_mutex_unlock(&outputMutex);
        pthread_cond_wait(&maxCond,&tableMutex);
        pthread_mutex_lock(&outputMutex);
        cout<<"thread "<<workerId<<" wokeup and will reaquire the mutex\n";
        pthread_mutex_unlock(&outputMutex);
    }
    pthread_mutex_unlock(&tableMutex);
    int queryType = rand()%3 +1;
    
    
    int entryIndex;
    switch (queryType) {
        case 1: // Inquire
            //read query
            pthread_mutex_lock(&tableMutex);
            while(isWrite(event->id)); // waits for conflicting write to be done
            entryIndex=makeTableEntry(event->id,queryType,workerId);
            availableSeats(event,workerId);
            deleteTableEntry(entryIndex);
            pthread_mutex_unlock(&tableMutex);
            break;
        case 2: // Book k tickets
            // Check if there are enough available seats
            // Book the seats and return success or failure
            //this is write query
            pthread_mutex_lock(&tableMutex);
            while(isRead(event->id) || isWrite(event->id));
            entryIndex=makeTableEntry(event->id,queryType,workerId);
            book(event,workerId,privList);
            deleteTableEntry(entryIndex);
            pthread_mutex_unlock(&tableMutex);
            break;
        case 3: // Cancel //chosen from the private list of each worker
            // Cancel the booked seats and return success or failure
            //write query
            pthread_mutex_lock(&tableMutex);
            while(isRead(event->id) || isWrite(event->id));
            entryIndex=makeTableEntry(event->id,queryType,workerId);
            cancelTicket(event,workerId,privList);
            deleteTableEntry(entryIndex);
            pthread_mutex_unlock(&tableMutex);
            break;
    }
    activeQueries--;
    pthread_cond_signal(&maxCond);
    
    
}
void* worker(void* id)
{
    int workerId= *((int*)id);
    // now the worker will run in a loop to query random events
    
    //each worker has private list of bookings make by it for each of the events
    map<int,vector<int>>privList; 
    int tam=1;
    while(is_running)
    {
        int eventNo= rand()%e;
        
        eventHandler(&events[eventNo],workerId,privList);
        
    }
    pthread_exit(NULL);
}
int main()
{

    FILE* file = freopen("output.txt", "w", stdout);
    srand(time(0));
    signal(SIGALRM, timer_handler);
    alarm(RUN_TIME);

    //main thread
    pthread_mutex_init(&outputMutex,NULL);
    pthread_mutex_init(&tableMutex,NULL);
    pthread_cond_init(&maxCond,NULL);
    // lets intialize the events
    for(int i=0;i<e;i++)
    {
        events[i].id=i;
        events[i].available=c;
        events[i].capacity=c;
        events[i].booked = vector<bool>(c,0); // none of the seats are booked 
        
    }
    //intinalize table
    for(int i=0;i<MAX;i++)
    {
        table[i].eventNo=-1;
    }
    //now lets create s worker threads that will execute concurrently
     pthread_t thread[s];
   
       
        for(int i=0;i<s;i++)
        {
            if(pthread_create(&thread[i],NULL,worker,(void*)&i)!=0)
            {
                cout<<"error creating thread\n";
                exit(0);
            }

        }
    
    //wait for all threads to finish
    for(int i=0;i<s;i++)
    {
        pthread_join(thread[i],NULL);
    }
    // here we will give the status of all events
    cout<<"Status of all bookings:\n\n";
    for(int i=0;i<e;i++)
    {
        cout<<"Booked seats for event "<< i <<" are : ";
        for(int j=0;j<c;j++) if(events[i].booked[j])cout<<j<<" ";
        cout<<"\n";
    }
    fclose(file);
    signal(SIGALRM, SIG_IGN);

}
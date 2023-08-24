#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <time.h>
#define MAX_SIZE 100

using namespace std;

// structure of message for msg queue
struct msg{
    long mtype;
    char text[MAX_SIZE];
};

// structure to store student details
struct student{
    int id;
    key_t msg_queue_id;
    int grade;
    string ans;
};

// helper funtion to generate random variable
int randNumGenerator(){
    return rand() % 4;
}

/* helper funtion to grade students
@params -
    ans - received ans arr
    correct _ans - vector of correct ans of qs
@return
    grade of student
*/
char grade(char ans[], vector<int> correct_ans){
    int correct = 0;
    int nqs = correct_ans.size();

    // calculate no of correct ans
    for (int i = 0; i < correct_ans.size(); i++)
    {   
        int t = ans[i] - '0';
        if(t == correct_ans[i]) correct++;
    }

    float g = (float) correct / (float) nqs; // grade calculation

    // grade allocation
    if(g > 0.8)
        return 'A';
    else if(0.6 <= g and g <= 0.8)
            return 'B';
    else if(0.4 <= g and g <= 0.6)
            return 'C';
    else if(0.2 <= g and g <= 0.4)
            return 'D';
    return 'F';
}

/* funtion to grading of students
@params -
    st - arr containing student details
    num_of_students - number of students
    correct_ans - vector of correct ans
*/
void parentProcess(student st[], int num_of_students, vector<int> correct_ans){
    msg rcv_buffer;
    int count[5] = {0}, status;

    cout << "Grading of students\n" << endl;

    // handling of student response
    for (int i = 0; i < num_of_students; i++)
    {
        msgrcv(st[i].msg_queue_id, &rcv_buffer, sizeof(msg), 2, MSG_NOERROR); // student ans string received

        if(status >= 0){
            char t = grade(rcv_buffer.text, correct_ans);

            if(t == 'A') count[0]++;
            if(t == 'B') count[1]++;
            if(t == 'C') count[2]++;
            if(t == 'D') count[3]++;
            if(t == 'F') count[4]++;

            cout << "Student - " << st[i].id << "\t Grade - " << t << endl;
        }
        else{
            cout << "Msg failed to be received from student " << i << endl;
        }
    }
    
    cout << "\nGrade Distribution\n" << endl;

    cout << "Grade \t Count" << endl;
    // Calulate grade distribution
    for (int i = 0; i < 5; i++)
    {
        cout << char('A' + i) << "\t" << count[i] << endl;
    }
    cout << "\n";
}

/* funtion to generate student response
@params -
    msg_queue_id - id of the msg queue assigned to the student process
    st - pointer to student details
*/
void studentProcess(key_t msg_queue_id, student *st){
    msg rcv_buffer, send_buffer;
    int num_of_qs, status;
    string ans_str = "";

    status = msgrcv(msg_queue_id, &rcv_buffer, sizeof(rcv_buffer), 0, MSG_NOERROR); // questions received
    cout << "\nStudent id - " << st->id << endl;

    if(status >= 0){
        cout << "Received " << status << " bytes of data in process " << getpid() << " student id- " << st->id << endl; 
    }
    else{
        cout << "Msg communication failure to process " << getpid() << endl;
        exit(0);
    }

    cout << "No of qs - " << rcv_buffer.text << "\n";

    num_of_qs = atoi(rcv_buffer.text);

    // generate answers by student
    for (int i = 0; i < num_of_qs; i++)
    {
        int ans = (randNumGenerator() + (int)getpid()) % 4;
        ans_str = ans_str + to_string(ans);
    }

    st->ans = ans_str;

    cout << "Student " << st->id << " Response - " << ans_str << "\n" << endl;
    
    strcpy(send_buffer.text, ans_str.c_str());

    send_buffer.mtype = 2;

    status = msgsnd(msg_queue_id, &send_buffer, sizeof(msg), MSG_NOERROR); // send student ans respomse

    if(status < 0){
            cout << "Failed to send ans to parent process by process " << getpid() << endl;
            exit(0);
    }
}

int main(){
    int num_of_qs, num_of_students, err;
    vector<int> correct_ans;
    msg qs_msg;

    srand(time(0));

    // user input for no of qs and students in the exam
    cout << "Enter the no of students" << endl;
    cin >> num_of_students;
    cout << "Enter the no of questions" << endl;
    cin >> num_of_qs;
    cout << "\n";
    
    for (int i = 0; i < num_of_qs; i++)
    {
        int t2;
        t2 = randNumGenerator() % 4;
        correct_ans.push_back(t2);
    }

    pid_t pid[num_of_students];
    vector<key_t> msg_queue_keys(num_of_students);
    int stardId = 65;
    student st[num_of_students];

    qs_msg.mtype = 1;
    string temp = to_string(num_of_qs);
    strcpy(qs_msg.text, temp.c_str());

    for (int i = 0; i < num_of_students; i++)
    {
        msg_queue_keys[i] = ftok(".", stardId + i); // create unique msg queue id
        st[i].id = i;
        st[i].msg_queue_id = msgget(msg_queue_keys[i], 0666 | IPC_CREAT); // msg queue creation

        if(st[i].msg_queue_id == -1){
            perror("Failure in queue creation");
            exit(0);
        }

        // send qs to students
        err = msgsnd(st[i].msg_queue_id, &qs_msg, sizeof(msg), MSG_NOERROR);
        //create student process
        pid[i] = fork();

        if(pid[i] == 0){
            if(err < 0){
                cout << "Failed to send qs to student process " << i << endl;
                exit(0);
            }

            studentProcess(st[i].msg_queue_id, &st[i]);
            exit(0);
        }
        else if(pid[i] < 0){
            num_of_students--;
            cout << "Student process failed to be created" << endl;
        }
    }
    //parent waits for child processes
    for (int i = 0; i < num_of_students; i++)
    {
        waitpid(pid[i], NULL, 0);
    }

    parentProcess(st, num_of_students, correct_ans);

    for (int i = 0; i < num_of_students; i++)
    {
        msgctl(st[i].msg_queue_id,IPC_RMID,NULL);
    }
    return 0;
}


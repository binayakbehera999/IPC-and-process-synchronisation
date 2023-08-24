#include <iostream>
#include <string>
#include <vector>
#include <fstream> // add this line
#include <sstream> // add this line
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>

using namespace std;

// part for child 1
// --------------------------------------------------------------------

void sendFileFromDir1ToDir2(int pipe1[], int pipe2[], string d1, vector<string> files1)
{
    string msg = "";
    for (const auto &file : files1)
    {
        string filename = d1 + "/" + file;
        ifstream f(filename.c_str());
        if (!f.good())
        {
            msg = "Error: failed to read " + filename;
            cout << msg;
            write(pipe1[1], msg.c_str(), msg.size() + 1);
            exit(EXIT_FAILURE);
        }
        string contents((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
        msg += file + " " + contents + "\n";
    }
    write(pipe1[1], msg.c_str(), msg.size() + 1);
}

void readFileFromDir2ToDir1(int pipe1[], int pipe2[], string d1)
{
    string msg = "";
    char buf1[BUFSIZ];

    if (read(pipe2[0], buf1, BUFSIZ) > 0)
    {
        msg += buf1;
    }

    cout << "message recieved is : " << msg << "\n";

    // vector<string> files2;
    istringstream iss(msg);
    string filePath, file, contents;
    while (iss >> file)
    {
        // files2.push_back(file);
        getline(iss >> std::ws, contents, '\n');
        string filename = d1 + "/" + file;
        ofstream f(filename.c_str());
        if (!f.good())
        {
            msg = "Error: failed to create " + filename;
            cout << msg;
            write(pipe1[1], msg.c_str(), msg.size() + 1);
            exit(EXIT_FAILURE);
        }
        f << contents << "\n";
        f.close();
    }
}

//------------------------------------------------------------------------------

// part for child 2
void readFileFromDir1ToDir2(int pipe1[], int pipe2[], string d2)
{
    string msg = "";
    char buf[BUFSIZ];
    int r;

    cout << "in child 2\n";

    if (read(pipe1[0], buf, BUFSIZ) > 0)
    {
        msg += buf;
    }

    cout << "message recieved in child2 : " << msg << "\n";

    // vector<string> files2;
    istringstream iss(msg);
    string filePath, file, contents;
    while (iss >> file)
    {
        // files2.push_back(file);
        getline(iss >> std::ws, contents, '\n');
        string filename = d2 + "/" + file;
        ofstream f(filename.c_str());
        if (!f.good())
        {
            msg = "Error: failed to create " + filename;
            cout << msg;
            write(pipe2[1], msg.c_str(), msg.size() + 1);
            exit(EXIT_FAILURE);
        }
        f << contents << "\n";
        f.close();
    }
}

void sendFileFromDir2ToDir1(int pipe1[], int pipe2[], string d2, vector<string> files2)
{

    string msg = "";

    for (const auto &file : files2)
    {
        string filename = d2 + "/" + file;
        // cout << "filename:" << filename << "\n";
        ifstream f(filename.c_str());
        if (!f.good())
        {
            msg = "Error: failed to read " + filename;
            cout << msg;
            write(pipe2[1], msg.c_str(), msg.size() + 1);
            exit(EXIT_FAILURE);
        }
        string contents((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        msg += file + " " + contents + "\n";
    }
    write(pipe2[1], msg.c_str(), msg.size() + 1);
}

// ----------------------------------------------------------------------------------------------------

int main()
{
    string d1 = "dir1";
    string d2 = "dir2";
    // Create the directories and files
    system(("mkdir " + d1).c_str());
    system(("echo 'file1 contents' > " + d1 + "/file1").c_str());
    system(("echo 'file2 contents' > " + d1 + "/file2").c_str());

    system(("mkdir " + d2).c_str());
    system(("echo 'file3 contents' > " + d2 + "/file3").c_str());
    system(("echo 'file4 contents' > " + d2 + "/file4").c_str());

    DIR *dir1 = opendir(d1.c_str());
    DIR *dir2 = opendir(d2.c_str());

    vector<string> files1;
    vector<string> files2;

    // vector<string> d1;

    dirent *dir_entry;

    while ((dir_entry = readdir(dir1)) != nullptr)
    {

        if (dir_entry->d_type == DT_REG)
        { // only consider regular files
            files1.push_back(dir_entry->d_name);
        }
    }

    while ((dir_entry = readdir(dir2)) != nullptr)
    {

        if (dir_entry->d_type == DT_REG)
        { // only consider regular files
            files2.push_back(dir_entry->d_name);
        }
    }

    int pipe1[2], pipe2[2];
    if (pipe(pipe1) < 0 || pipe(pipe2) < 0)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork();

    if (pid1 < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid1 == 0)
    {
        close(pipe1[0]); // Close read end of pipe1
        close(pipe2[1]); // Close write end of pipe2

        sendFileFromDir1ToDir2(pipe1, pipe2, d1, files1);
        readFileFromDir2ToDir1(pipe1, pipe2, d1);
        exit(EXIT_SUCCESS);
    }

    pid_t pid2 = fork();

    if (pid2 < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid2 == 0)
    {
        close(pipe1[1]); // Close write end of pipe1
        close(pipe2[0]); // Close read end of pipe2

        readFileFromDir1ToDir2(pipe1, pipe2, d2);
        sendFileFromDir2ToDir1(pipe1, pipe2, d2, files2);
        exit(EXIT_SUCCESS);
    }

    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    // Check that the directories are identical
    system(("diff -r " + d1 + " " + d2).c_str());

    return 0;
}
#include <iostream>
#include <string>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx)] = 0;
}

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

void Tempo(const char* cmd_line){
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(WHITESPACE));
    //string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if(firstWord == "c"){ //tODO ?????
        string tmp = "chprompt"; // TODO ?????
        cmd_s = string(cmd_line);
        cmd_line = (cmd_s.find_first_of(" \n") != string::npos) ? (tmp + cmd_s.substr(cmd_s.find_first_of(" \n"))).c_str() : tmp.c_str();
        firstWord = cmd_s.substr(0, cmd_s.find_first_of(WHITESPACE));
    }
    cout << cmd_line << endl;
}

int main(int argc , char**argv)
{
    Tempo("c ayal");
    return 0;
}

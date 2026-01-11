#include<bits/stdc++.h>
using namespace std;

class Encryption {
public:
    Encryption() {
        key = 42;
        data = "initialized";
        encrypted = false;
        buffer.resize(16, 'x');
        for (int i = 0; i < buffer.size(); ++i) {
            buffer[i] = (char)(buffer[i] + (key % 10));
        }
        encrypted = true;
    }
private:
    int key;
    string data;
    bool encrypted;
    vector<char> buffer;
};

int main() {
    return 0;
}
#include "../include/cli.hpp"
#include "../include/client.hpp"
#include "../include/helper.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;
using Bytes = vector<byte>;

struct Student {
    int rollno;
    string name;
    vector<int> marks;
};

void appendInt(Bytes& bytes, int32_t value) {
    uint32_t number = static_cast<uint32_t>(value);

    for (unsigned shift = 0; shift < 32; shift += 8)
        bytes.push_back(static_cast<byte>((number >> shift) & 0xff));
}

int32_t readInt(const Bytes& bytes, size_t& position) {
    if (position > bytes.size() || bytes.size() - position < 4)
        throw runtime_error("Not enough bytes for an integer");

    uint32_t number = 0;

    for (unsigned shift = 0; shift < 32; shift += 8) {
        number |= to_integer<uint32_t>(bytes[position++]) << shift;
    }

    return static_cast<int32_t>(number);
}

Bytes studentToBytes(const Student& student) {
    if (student.marks.size() != 3)
        throw runtime_error("Student must have exactly 3 marks");

    Bytes bytes;

    appendInt(bytes, student.rollno);

    for (unsigned char character : student.name)
        bytes.push_back(static_cast<byte>(character));

    for (int mark : student.marks)
        appendInt(bytes, mark);

    return bytes;
}

Student bytesToStudent(const Bytes& bytes) {
    if (bytes.size() < 16)
        throw runtime_error("Invalid student data");

    Student student{};
    size_t position = 0;

    student.rollno = readInt(bytes, position);

    const size_t nameLength = bytes.size() - 16;

    for (size_t i = 0; i < nameLength; ++i)
        student.name.push_back(
            to_integer<char>(bytes[position++])
        );

    for (int i = 0; i < 3; ++i)
        student.marks.push_back(readInt(bytes, position));

    return student;
}

void printStudent(const Student& student) {
    cout << "Roll number: " << student.rollno << '\n';
    cout << "Name: " << student.name << '\n';
    cout << "Marks:";

    for (int mark : student.marks)
        cout << ' ' << mark;

    cout << '\n';
}

Bytes keyToBytes(int key) {
    Bytes bytes;
    appendInt(bytes, key);
    return bytes;
}

Response setValue(KVclient& client, int key, const Bytes& value) {
    Query query{
        Command::SET,
        {keyToBytes(key), value}
    };

    return client.send_query(query);
}

Response getValue(KVclient& client, int key) {
    Query query{
        Command::GET,
        {keyToBytes(key)}
    };

    return client.send_query(query);
}

Response deleteValue(KVclient& client, int key) {
    Query query{
        Command::DEL,
        {keyToBytes(key)}
    };

    return client.send_query(query);
}


int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "Usage: program <server> <port>\n";
        return 1;
    }

    try {
        int port = stoi(argv[2]);
        if (port < 1 || port > 65535)
            throw runtime_error("Invalid port");

        KVclient client{argv[1], static_cast<uint16_t>(port)};

        int key = 1234;
        Student student{2, "jaychirmade", {2, 300, 4}};

        // SET
        Response stored = setValue(
            client, key, studentToBytes(student)
        );
        cout << "SET: " << eval_status(stored.status) << '\n';

        // GET
        Response fetched = getValue(client, key);
        cout << "GET: " << eval_status(fetched.status) << '\n';

        // Check your API's success status before decoding.
        Student result = bytesToStudent(fetched.data);
        printStudent(result);

        // DEL — uncomment when needed.
        Response deleted = deleteValue(client, key);
        cout << "DEL: " << eval_status(deleted.status) << '\n';

        // GET
        fetched = getValue(client, key);
        cout << "GET: " << eval_status(fetched.status) << '\n';

    } catch (const exception& error) {
        cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}